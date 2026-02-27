#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

inline constexpr uint32_t SERIAL_BAUD = 115200;
inline constexpr size_t CMD_BUFFER_SIZE = 64;
inline constexpr size_t SAMPLE_RING_CAPACITY = 256;

inline constexpr uint16_t DEFAULT_SAMPLE_HZ = 100;
inline constexpr uint8_t DEFAULT_ANALOG_PIN = 36;
inline constexpr uint16_t MIN_SAMPLE_HZ = 1;
inline constexpr uint16_t MAX_SAMPLE_HZ = 5000;

inline constexpr char MSG_ONLINE[] = "esp32_sampling_board_online";
inline constexpr char ERR_PIN[] = "Error Code 01: incorrect pin";
inline constexpr char ERR_FREQ[] = "Error Code 02: incorrect sampling frequency";
inline constexpr char ERR_OVERFLOW[] = "Error Code 03: Cache overflow";

struct SamplePacket {
  uint32_t timestamp_ms;
  uint16_t voltage_mv;
};

enum class RunState : uint8_t {
  Idle = 0,
  Sampling = 1,
  Stopping = 2
};
