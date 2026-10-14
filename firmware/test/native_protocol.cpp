#include "bench_modbus.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <initializer_list>

int main() {
  const uint8_t read[] = {2, 3, 0, 0, 0, 3};
  assert(bench_modbus_crc16(read, sizeof(read)) == 0xF805);
  assert(bench_modbus_calibration_point(400) == 1);
  assert(bench_modbus_calibration_point(686) == 2);
  assert(bench_modbus_calibration_point(918) == 3);
  assert(bench_modbus_calibration_point(700) == 0);
  uint8_t request[8];
  assert(bench_modbus_calibration_request(2, 0x006F, 0, request));
  assert(bench_modbus_write_reply(request, request, sizeof(request)));
  assert(bench_modbus_calibration_request(2, 0x0069, 3, request));
  const uint8_t expected[] = {2, 6, 0, 0x69, 0, 3, 0x19, 0xE4};
  assert(memcmp(request, expected, 8) == 0);
  uint8_t reply[8];
  memcpy(reply, request, 8);
  assert(bench_modbus_write_reply(request, reply, 8));
  for (size_t i = 0; i < 8; ++i) {
    reply[i] ^= 1;
    assert(!bench_modbus_write_reply(request, reply, 8));
    reply[i] ^= 1;
  }
  for (size_t length = 0; length < 8; ++length)
    assert(!bench_modbus_write_reply(request, reply, length));
  assert(!bench_modbus_write_reply(request, reply, 9));
  assert(!bench_modbus_calibration_request(0, 0x0069, 3, request));
  assert(!bench_modbus_calibration_request(248, 0x0069, 3, request));
  assert(!bench_modbus_calibration_request(2, 0x0030, 3, request));
  assert(!bench_modbus_calibration_request(2, 0x006F, 1, request));
  assert(!bench_modbus_calibration_request(2, 0x0069, 0, request));
  assert(!bench_modbus_calibration_request(2, 0x0069, 4, request));
  assert(!bench_modbus_calibration_request(2, 0x0069, 3, nullptr));
  uint8_t raw[19];
  assert(bench_modbus_raw_request(2, "0600690003", raw, sizeof(raw)) == 8);
  assert(memcmp(raw, expected, 8) == 0);
  assert(bench_modbus_raw_request(2, "11", raw, sizeof(raw)) == 4);
  assert(bench_modbus_crc16(raw, 4) == 0);
  assert(bench_modbus_raw_request(2, "2B0E0100", raw, sizeof(raw)) == 7);
  assert(raw[1] == 0x2B && raw[2] == 0x0E);
  assert(bench_modbus_raw_request(2, "03000000010000000000000000000000", raw, sizeof(raw)) == 19);
  for (const char *invalid : {"", "1", "00", "80", "GG", "03 00", "0300000001000000000000000000000000"})
    assert(!bench_modbus_raw_request(2, invalid, raw, sizeof(raw)));
  assert(!bench_modbus_raw_request(0, "11", raw, sizeof(raw)));
  assert(!bench_modbus_raw_request(248, "11", raw, sizeof(raw)));
  assert(bench_modbus_raw_request(254, "0300000001", raw, sizeof(raw)) == 8);
  assert(raw[0] == 254 && bench_modbus_crc16(raw, 8) == 0);
  assert(!bench_modbus_raw_request(2, "11", raw, 3));
  assert(!bench_modbus_raw_request(2, nullptr, raw, sizeof(raw)));
  puts("PASS: Modbus CRC, calibration write allowlist, reply validation");
  puts("PASS: Experimental PDU encoding, bounds and malformed input rejection");
}
