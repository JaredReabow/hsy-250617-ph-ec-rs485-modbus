#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/** CRC-16/Modbus, including request/response payload but excluding CRC bytes. */
inline uint16_t bench_modbus_crc16(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
  }
  return crc;
}

/** Map the three supported buffer labels to the existing HX6 point commands. */
inline uint16_t bench_modbus_calibration_point(unsigned int buffer) {
  switch (buffer) {
    case 400: return 1;
    case 686: return 2;
    case 918: return 3;
    default: return 0;
  }
}

/** Build only pH channel selection or a supported calibration-point write. */
inline bool bench_modbus_calibration_request(uint8_t address, uint16_t reg,
                                             uint16_t value, uint8_t *request) {
  if (!request || address < 1 || address > 247 ||
      !((reg == 0x006F && value == 0) ||
        (reg == 0x0069 && value >= 1 && value <= 3))) {
    return false;
  }
  request[0] = address;
  request[1] = 0x06;
  request[2] = static_cast<uint8_t>(reg >> 8);
  request[3] = static_cast<uint8_t>(reg);
  request[4] = static_cast<uint8_t>(value >> 8);
  request[5] = static_cast<uint8_t>(value);
  const uint16_t crc = bench_modbus_crc16(request, 6);
  request[6] = static_cast<uint8_t>(crc);
  request[7] = static_cast<uint8_t>(crc >> 8);
  return true;
}

/** Validate a complete function-06 echo reply; this does not prove calibration. */
inline bool bench_modbus_write_reply(const uint8_t *request,
                                     const uint8_t *reply, size_t length) {
  if (!request || !reply || length != 8) return false;
  for (size_t i = 0; i < 6; ++i) {
    if (reply[i] != request[i]) return false;
  }
  const uint16_t crc = bench_modbus_crc16(reply, 6);
  return reply[6] == static_cast<uint8_t>(crc) &&
         reply[7] == static_cast<uint8_t>(crc >> 8);
}

/** Convert one hexadecimal digit, rejecting all non-hexadecimal input. */
inline int bench_modbus_hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

/** Build an explicit experimental PDU (1..16 bytes) with address and CRC. */
inline size_t bench_modbus_raw_request(uint8_t address, const char *hex,
                                       uint8_t *out, size_t capacity) {
  if (!hex || !out || address < 1 || (address > 247 && address != 254)) return 0;
  const size_t chars = strlen(hex);
  if (chars < 2 || chars > 32 || chars % 2 || capacity < chars / 2 + 3)
    return 0;
  for (size_t i = 0; i < chars; ++i)
    if (bench_modbus_hex_digit(hex[i]) < 0) return 0;
  const size_t bytes = chars / 2;
  out[0] = address;
  for (size_t i = 0; i < bytes; ++i)
    out[i + 1] = static_cast<uint8_t>((bench_modbus_hex_digit(hex[2 * i]) << 4) |
                                      bench_modbus_hex_digit(hex[2 * i + 1]));
  if (!out[1] || out[1] >= 0x80) return 0;
  const uint16_t crc = bench_modbus_crc16(out, bytes + 1);
  out[bytes + 1] = static_cast<uint8_t>(crc);
  out[bytes + 2] = static_cast<uint8_t>(crc >> 8);
  return bytes + 3;
}
