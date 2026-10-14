/**
 * @file main.cpp
 * @brief RS485 / Modbus RTU bench tool for the HSY pH+EC controller, running
 *        on a Waveshare ESP32-S3-Relay-1CH.
 *
 * Purpose
 *   Bring-up and diagnostics for the pH controller wired to the
 *   relay board's isolated RS485 port. Polls the pH / EC / temperature
 *   registers, can sweep the register map, and can re-scan baud/address
 *   combinations when the controller configuration is unknown.
 *
 * Hardware (Waveshare factory demo pin map, WS_GPIO.h)
 *   RS485 DI  <- GPIO17  (UART1 TX)
 *   RS485 RO  -> GPIO18  (UART1 RX)
 *   RS485 DE  <- GPIO21  (UART TXDEN, hardware half-duplex direction control)
 *   A+ / B-   screw terminal (isolated side)
 *
 * Wiring to the controller (per controller manual)
 *   Controller yellow (RS485 A+) -> board A+
 *   Controller green  (RS485 B-) -> board B-
 *   If there is no response, swap A/B once and re-scan; vendor A/B labels are
 *   not consistent across products.
 *
 * Protocol
 *   Modbus RTU, 8N1. Factory default 9600 baud, slave address 1.
 *   Registers: 0x0000 pH x100, 0x0001 EC uS/cm, 0x0002 temperature x10.
 *   The HX6 fleet firmware also runs this sensor family at 4800 baud /
 *   address 2, so the bus scanner probes both.
 *
 * Calibration
 *   Explicit USB command c 400/686/918 uses the existing HX6 driver sequence:
 *   function 0x06, pH channel register 0x006F=0, point register 0x0069=1/2/3.
 *   Automatic polling and discovery remain read-only. Explicit x commands
 *   permit raw experimental PDUs for authorized bench reverse engineering.
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include "bench_modbus.h"

// ---------------------------------------------------------------------------
// Configuration defaults (override with -D build flags if the board differs)
// ---------------------------------------------------------------------------
#ifndef BENCH_PIN_RS485_TX
  #define BENCH_PIN_RS485_TX 17
#endif
#ifndef BENCH_PIN_RS485_RX
  #define BENCH_PIN_RS485_RX 18
#endif
#ifndef BENCH_PIN_RS485_DE
  #define BENCH_PIN_RS485_DE 21
#endif

namespace {

constexpr char kFirmwareVersion[] = "v1.0.4";

constexpr int kPinRs485Tx = BENCH_PIN_RS485_TX;
constexpr int kPinRs485Rx = BENCH_PIN_RS485_RX;
constexpr int kPinRs485De = BENCH_PIN_RS485_DE;

constexpr uint16_t kReadingBaseReg = 0x0000;  // pH x100, EC, temperature x10
constexpr uint16_t kReadingRegCount = 3;
constexpr uint16_t kPollTimeoutMs = 250;
// A 150 ms sweep admitted late replies as the next register's response on
// this controller. Allow 500 ms and a quiet drain between sweep requests.
constexpr uint16_t kSweepTimeoutMs = 500;
constexpr uint16_t kSweepStartReg = 0x0000;
constexpr uint16_t kSweepEndReg = 0x007F;
constexpr uint32_t kPollIntervalMs = 2000;
constexpr uint32_t kInterFrameGapMs = 5;

constexpr uint32_t kBaudCandidates[] = {9600, 4800, 19200};
constexpr uint8_t kAddressCandidates[] = {1, 2, 3, 4, 5};
constexpr size_t kBaudCount = sizeof(kBaudCandidates) / sizeof(kBaudCandidates[0]);
constexpr size_t kAddressCount = sizeof(kAddressCandidates) / sizeof(kAddressCandidates[0]);

HardwareSerial rs485(1);

uint32_t active_baud = kBaudCandidates[0];
uint8_t active_address = kAddressCandidates[0];
bool device_found = false;
bool polling_enabled = true;
bool raw_frame_dump = false;
uint32_t last_poll_ms = 0;
uint32_t poll_ok_count = 0;
uint32_t poll_fail_count = 0;

/** Print a byte buffer as space separated hex. */
void bench_print_hex(const char *label, const uint8_t *data, size_t len) {
  char line[160];
  size_t used = snprintf(line, sizeof(line), "%s", label);
  for (size_t i = 0; i < len && used + 4 < sizeof(line); ++i) {
    used += snprintf(line + used, sizeof(line) - used, " %02X", data[i]);
  }
  Serial.println(line);
}

/**
 * @brief Open UART1 in hardware RS485 half-duplex mode at the given baud.
 * @details TXDEN (GPIO21) is routed to the UART RTS signal so the ESP32
 *          transceiver direction switching is handled by hardware.
 */
void bench_begin_uart(uint32_t baud, uint32_t config = SERIAL_8N1) {
  rs485.end();
  rs485.begin(baud, config, kPinRs485Rx, kPinRs485Tx);
  if (!rs485.setPins(-1, -1, -1, kPinRs485De)) {
    Serial.println("WARN: setPins(TXDEN) failed");
  }
  if (!rs485.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
    Serial.println("WARN: setMode(UART_MODE_RS485_HALF_DUPLEX) failed");
  }
  active_baud = baud;
  delay(kInterFrameGapMs);
}

/**
 * @brief Read holding registers (Modbus 0x03) and decode the reply.
 *
 * The ESP32 half-duplex RS485 mode echoes the transmitted request back on RX,
 * so the capture may contain request bytes before the reply. The decoder
 * therefore scans the capture for the first frame with a matching address,
 * function 0x03, expected byte count and a valid CRC.
 *
 * @param address      Modbus slave address to query.
 * @param startReg     First register address.
 * @param count        Registers to read (1..8).
 * @param outRegs      Destination for decoded register values (count words).
 * @param timeoutMs    Response timeout in milliseconds.
 * @param dumpFrames   Print raw TX/RX frames for bring-up diagnostics.
 * @return true when a valid response frame was decoded.
 */
bool bench_read_registers(uint8_t address, uint16_t startReg, uint16_t count,
                          uint16_t *outRegs, uint16_t timeoutMs, bool dumpFrames) {
  if (count == 0 || count > 8 || outRegs == nullptr) {
    return false;
  }

  uint8_t request[8];
  request[0] = address;
  request[1] = 0x03;
  request[2] = static_cast<uint8_t>(startReg >> 8);
  request[3] = static_cast<uint8_t>(startReg & 0xFF);
  request[4] = static_cast<uint8_t>(count >> 8);
  request[5] = static_cast<uint8_t>(count & 0xFF);
  const uint16_t requestCrc = bench_modbus_crc16(request, 6);
  request[6] = static_cast<uint8_t>(requestCrc & 0xFF);
  request[7] = static_cast<uint8_t>(requestCrc >> 8);

  while (rs485.available() > 0) {
    rs485.read();
  }
  if (dumpFrames) {
    bench_print_hex("TX:", request, sizeof(request));
  }
  rs485.write(request, sizeof(request));
  rs485.flush();  // wait for the last bit to leave the shift register

  uint8_t capture[64];
  size_t length = 0;
  const uint32_t startMs = millis();
  uint32_t lastByteMs = startMs;
  while (millis() - startMs < timeoutMs) {
    while (rs485.available() > 0 && length < sizeof(capture)) {
      capture[length++] = static_cast<uint8_t>(rs485.read());
      lastByteMs = millis();
    }
    if (length > 0 && millis() - lastByteMs > 20) {
      break;  // inter-frame silence: frame capture complete
    }
    delay(1);
  }

  if (dumpFrames && length > 0) {
    bench_print_hex("RX:", capture, length);
  }

  const size_t expectedByteCount = count * 2;
  for (size_t offset = 0; offset + 5 <= length; ++offset) {
    if (capture[offset] != address) {
      continue;
    }
    if (capture[offset + 1] == 0x83) {  // Modbus exception reply
      if (offset + 5 > length) {
        continue;
      }
      const uint16_t crc = bench_modbus_crc16(capture + offset, 3);
      const uint16_t received = static_cast<uint16_t>(capture[offset + 3]) |
                                (static_cast<uint16_t>(capture[offset + 4]) << 8);
      if (crc == received) {
        Serial.printf("Modbus exception 0x%02X from addr %u reg 0x%04X\n",
                      capture[offset + 2], address, startReg);
        return false;
      }
      continue;
    }
    if (capture[offset + 1] != 0x03 || capture[offset + 2] != expectedByteCount) {
      continue;
    }
    const size_t frameLength = 3 + expectedByteCount + 2;
    if (offset + frameLength > length) {
      continue;
    }
    const uint16_t crc = bench_modbus_crc16(capture + offset, 3 + expectedByteCount);
    const uint16_t received =
        static_cast<uint16_t>(capture[offset + 3 + expectedByteCount]) |
        (static_cast<uint16_t>(capture[offset + 4 + expectedByteCount]) << 8);
    if (crc != received) {
      continue;
    }
    for (uint16_t i = 0; i < count; ++i) {
      outRegs[i] = (static_cast<uint16_t>(capture[offset + 3 + i * 2]) << 8) |
                   capture[offset + 4 + i * 2];
    }
    return true;
  }
  return false;
}

/** Probe each baud/address combination and latch the first one that replies. */
bool bench_scan_bus(bool verbose) {
  for (size_t b = 0; b < kBaudCount; ++b) {
    bench_begin_uart(kBaudCandidates[b]);
    for (size_t a = 0; a < kAddressCount; ++a) {
      const uint8_t address = kAddressCandidates[a];
      uint16_t regs[kReadingRegCount] = {0};
      if (bench_read_registers(address, kReadingBaseReg, 1, regs,
                               kPollTimeoutMs, false)) {
        active_address = address;
        device_found = true;
        if (verbose) {
          Serial.printf("Device found: addr %u @ %lu baud (pH reg = 0x%04X)\n",
                        address, static_cast<unsigned long>(active_baud), regs[0]);
        }
        return true;
      }
      delay(kInterFrameGapMs);
    }
  }
  device_found = false;
  if (verbose) {
    Serial.println("No Modbus reply on any probed baud/address combination.");
  }
  return false;
}

/** Poll the three base registers and print the decoded reading. */
bool bench_poll_once(bool forceVerbose) {
  uint16_t regs[kReadingRegCount] = {0};
  const bool verbose = forceVerbose || raw_frame_dump;
  if (!bench_read_registers(active_address, kReadingBaseReg, kReadingRegCount,
                            regs, kPollTimeoutMs, verbose)) {
    ++poll_fail_count;
    if (verbose) {
      Serial.printf("Poll failed: addr %u @ %lu baud (fail #%lu)\n",
                    active_address, static_cast<unsigned long>(active_baud),
                    static_cast<unsigned long>(poll_fail_count));
    }
    return false;
  }
  ++poll_ok_count;
  const float ph = static_cast<float>(regs[0]) / 100.0f;
  Serial.printf("pH=%.2f  EC=%u uS/cm  temperature=%.1f C   [addr %u @ %lu baud, ok=%lu fail=%lu]\n",
                ph, regs[1], static_cast<float>(regs[2]) / 10.0f, active_address,
                static_cast<unsigned long>(active_baud),
                static_cast<unsigned long>(poll_ok_count),
                static_cast<unsigned long>(poll_fail_count));
  return true;
}

/**
 * @brief Read-only sweep of the low register map.
 * @details Prints every register that answers with a valid frame; missing
 *          registers simply time out. No writes are issued.
 */
void bench_sweep_registers() {
  Serial.printf("Sweeping registers 0x%04X..0x%04X (read-only) on addr %u @ %lu baud\n",
                kSweepStartReg, kSweepEndReg, active_address,
                static_cast<unsigned long>(active_baud));
  for (uint16_t reg = kSweepStartReg; reg <= kSweepEndReg; ++reg) {
    delay(250);
    while (rs485.available()) rs485.read();
    uint16_t value = 0;
    if (bench_read_registers(active_address, reg, 1, &value, kSweepTimeoutMs,
                             false)) {
      Serial.printf("  reg 0x%04X = %5u (0x%04X)%s\n", reg, value, value,
                    reg == kReadingBaseReg ? "   <- pH x100" : "");
    }
    delay(kInterFrameGapMs);
  }
  Serial.println("Sweep complete.");
}

/** Send an allowlisted calibration write and capture its Modbus echo reply. */
bool bench_write_calibration_register(uint16_t reg, uint16_t value) {
  uint8_t request[8];
  if (!bench_modbus_calibration_request(active_address, reg, value, request))
    return false;
  delay(10);  // At 4800 baud, allow at least 3.5 character times of silence.
  while (rs485.available()) rs485.read();
  bench_print_hex("WRITE TX:", request, sizeof(request));
  rs485.write(request, sizeof(request));
  rs485.flush();
  uint8_t capture[64];
  size_t length = 0;
  const uint32_t started = millis();
  uint32_t lastByte = started;
  while (millis() - started < 500) {
    while (rs485.available() && length < sizeof(capture)) {
      capture[length++] = static_cast<uint8_t>(rs485.read());
      lastByte = millis();
    }
    if (length && millis() - lastByte > 30) break;
    delay(1);
  }
  bench_print_hex("WRITE RX:", capture, length);
  for (size_t offset = 0; offset + 8 <= length; ++offset) {
    if (bench_modbus_write_reply(request, capture + offset, 8)) {
      Serial.println("Write echo received; calibration outcome not yet verified.");
      return true;
    }
  }
  Serial.println("No valid write echo; outcome unknown, no automatic retry.");
  return false;
}

/** Request a calibration point, then observe pH after the HX6 8-second holdoff. */
void bench_calibrate(unsigned int buffer) {
  const uint16_t point = bench_modbus_calibration_point(buffer);
  if (!device_found || !point) {
    Serial.println("Calibration requires a discovered device and buffer 400/686/918.");
    return;
  }
  Serial.printf("Calibration request: pH %.2f; probe must be in this buffer.\n",
                buffer / 100.0);
  if (!bench_write_calibration_register(0x006F, 0)) return;
  if (!bench_write_calibration_register(0x0069, point)) return;
  Serial.println("Waiting 8 seconds before measurement; echo is not calibration success.");
  delay(8000);
  bench_poll_once(true);
  last_poll_ms = millis();
}

/** Print a complete diagnostic frame in chunks without silently truncating it. */
void bench_raw_print_frame(const uint8_t *frame, size_t length, uint32_t elapsed,
                           bool overflow) {
  const bool crcOk = !overflow && length >= 4 && bench_modbus_crc16(frame, length) == 0;
  Serial.printf("RAW RX @%lu ms length=%u CRC=%s overflow=%s\n",
                static_cast<unsigned long>(elapsed), static_cast<unsigned>(length),
                crcOk ? "OK" : "BAD", overflow ? "yes" : "no");
  for (size_t offset = 0; offset < length; offset += 40) {
    const size_t remaining = length - offset;
    bench_print_hex("RAW BYTES:", frame + offset, remaining > 40 ? 40 : remaining);
  }
  if (crcOk && length == 5 && frame[0] == active_address && (frame[1] & 0x80))
    Serial.printf("RAW exception: function=0x%02X code=0x%02X\n", frame[1] & 0x7F, frame[2]);
}

/** Send one operator-specified PDU and observe all replies for a bounded window. */
void bench_raw_exchange(const char *hex, uint32_t timeout) {
  uint8_t request[19];
  const size_t requestLength = bench_modbus_raw_request(active_address, hex, request, sizeof(request));
  if (!requestLength || timeout < 100 || timeout > 15000) {
    Serial.println("RAW rejected: require valid address, 100..15000 ms, 1..16 PDU bytes.");
    return;
  }
  delay(10);
  while (rs485.available()) rs485.read();
  bench_print_hex("RAW TX:", request, requestLength);
  rs485.write(request, requestLength);
  rs485.flush();
  const uint32_t started = millis();
  uint32_t lastByte = started;
  uint8_t frame[256];
  size_t length = 0;
  bool overflow = false;
  unsigned int frames = 0;
  while (millis() - started < timeout) {
    while (rs485.available()) {
      const uint8_t value = static_cast<uint8_t>(rs485.read());
      if (length < sizeof(frame)) frame[length++] = value;
      else overflow = true;
      lastByte = millis();
    }
    if (length && millis() - lastByte >= 30) {
      bench_raw_print_frame(frame, length, lastByte - started, overflow);
      ++frames;
      length = 0;
      overflow = false;
    }
    delay(1);
  }
  if (length) {
    bench_raw_print_frame(frame, length, lastByte - started, overflow);
    ++frames;
  }
  Serial.printf("RAW done: %u frame(s); no automatic retry.\n", frames);
  last_poll_ms = millis();
}

void bench_print_help() {
  Serial.println("Commands:");
  Serial.println("  p        toggle 2 s polling on/off");
  Serial.println("  r        read the pH/EC/temperature registers once");
  Serial.println("  s        read-only sweep of registers 0x0000..0x007F");
  Serial.println("  b        re-scan baud rates x slave addresses");
  Serial.println("  g A C    read C registers (1..8) from hex address A, e.g. g 30 4");
  Serial.println("  c B      calibrate in buffer B=400/686/918 (WRITES controller)");
  Serial.println("  x MS HEX send experimental PDU, observe 100..15000 ms (CAN WRITE/RESET)");
  Serial.println("  u B A F  manual UART: baud, decimal address, 8N1/8E1/8O1/8N2");
  Serial.println("  f        toggle raw frame dump");
  Serial.println("  i        print current bench settings");
  Serial.println("  ?        this help");
}

void bench_print_status() {
  Serial.printf("Bench %s: addr=%u baud=%lu device=%s polling=%s frames=%s ok=%lu fail=%lu\n",
                kFirmwareVersion, active_address,
                static_cast<unsigned long>(active_baud),
                device_found ? "found" : "not found",
                polling_enabled ? "on" : "off", raw_frame_dump ? "on" : "off",
                static_cast<unsigned long>(poll_ok_count),
                static_cast<unsigned long>(poll_fail_count));
}

/** Parse and execute one USB serial command line. */
void bench_handle_serial() {
  static char line[48];
  static size_t length = 0;
  static bool overflow = false;
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r' || c == '\n') {
      if (overflow) {
        overflow = false;
        length = 0;
        Serial.println("Command too long; discarded without execution.");
        continue;
      }
      if (length == 0) {
        continue;
      }
      line[length] = '\0';
      length = 0;
      char command = line[0];
      if (command >= 'A' && command <= 'Z') {
        command = static_cast<char>(command - 'A' + 'a');
      }
      switch (command) {
        case 'p':
          polling_enabled = !polling_enabled;
          Serial.printf("Polling %s\n", polling_enabled ? "on" : "off");
          break;
        case 'r':
          bench_poll_once(true);
          break;
        case 's':
          bench_sweep_registers();
          break;
        case 'b':
          bench_scan_bus(true);
          bench_print_status();
          break;
        case 'f':
          raw_frame_dump = !raw_frame_dump;
          Serial.printf("Frame dump %s\n", raw_frame_dump ? "on" : "off");
          break;
        case 'i':
          bench_print_status();
          break;
        case 'g': {
          unsigned int reg = 0;
          unsigned int count = 0;
          if (sscanf(line + 1, "%x %u", &reg, &count) == 2 && count >= 1 &&
              count <= 8 && reg <= 0xFFFF) {
            uint16_t regs[8] = {0};
            if (bench_read_registers(active_address, static_cast<uint16_t>(reg),
                                     static_cast<uint16_t>(count), regs,
                                     kPollTimeoutMs, true)) {
              for (unsigned int i = 0; i < count; ++i) {
                Serial.printf("  reg 0x%04X = %5u (0x%04X)\n",
                              reg + i, regs[i], regs[i]);
              }
            } else {
              Serial.println("Read failed.");
            }
          } else {
            Serial.println("Usage: g <hex address> <count 1..8>");
          }
          break;
        }
        case 'c': {
          unsigned int buffer = 0;
          char extra = 0;
          if (sscanf(line + 1, "%u %c", &buffer, &extra) == 1 &&
              bench_modbus_calibration_point(buffer)) {
            bench_calibrate(buffer);
          } else {
            Serial.println("Usage: c <400|686|918>; probe must be in matching buffer.");
          }
          break;
        }
        case 'x': {
          unsigned int timeout = 0;
          char hex[33] = {};
          char extra = 0;
          if (sscanf(line + 1, "%u %32s %c", &timeout, hex, &extra) == 2)
            bench_raw_exchange(hex, timeout);
          else Serial.println("Usage: x <100..15000 ms> <compact hex PDU, max 16 bytes>");
          break;
        }
        case 'u': {
          unsigned int baud = 0, address = 0;
          char framing[4] = {}, extra = 0;
          uint32_t config = 0;
          if (sscanf(line + 1, "%u %u %3s %c", &baud, &address, framing, &extra) == 3) {
            if (!strcmp(framing, "8N1")) config = SERIAL_8N1;
            if (!strcmp(framing, "8E1")) config = SERIAL_8E1;
            if (!strcmp(framing, "8O1")) config = SERIAL_8O1;
            if (!strcmp(framing, "8N2")) config = SERIAL_8N2;
          }
          if (config && baud >= 1200 && baud <= 115200 &&
              ((address >= 1 && address <= 247) || address == 254)) {
            polling_enabled = false;
            device_found = false;
            active_address = static_cast<uint8_t>(address);
            bench_begin_uart(baud, config);
            Serial.printf("UART configured: baud=%u addr=%u framing=%s; raw requests enabled, discovery unverified.\n",
                          baud, address, framing);
          } else Serial.println("Usage: u <1200..115200 baud> <1..247 or 254 address> <8N1|8E1|8O1|8N2>");
          break;
        }
        case '?':
          bench_print_help();
          break;
        default:
          Serial.printf("Unknown command '%c'\n", command);
          bench_print_help();
          break;
      }
      continue;
    }
    if (length + 1 < sizeof(line)) {
      line[length++] = c;
    } else {
      overflow = true;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  const uint32_t serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) {
    delay(10);
  }
  delay(300);

  Serial.printf("\n=== pH controller RS485 bench %s ===\n", kFirmwareVersion);
  Serial.printf("Board: Waveshare ESP32-S3-Relay-1CH | TX=GPIO%d RX=GPIO%d DE=GPIO%d\n",
                kPinRs485Tx, kPinRs485Rx, kPinRs485De);
  Serial.println("Modbus RTU 8N1; read polling, explicit calibration/raw diagnostics.");

  bench_begin_uart(kBaudCandidates[0]);
  bench_scan_bus(true);
  bench_print_help();
}

void loop() {
  bench_handle_serial();

  if (polling_enabled && device_found &&
      millis() - last_poll_ms >= kPollIntervalMs) {
    last_poll_ms = millis();
    if (!bench_poll_once(false)) {
      // Two consecutive misses usually mean the wiring or baud changed.
      if (poll_fail_count % 2 == 0) {
        Serial.println("Poll failing; run 'b' to re-scan or check A/B wiring.");
      }
    }
  }
  delay(10);
}
