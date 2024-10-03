#pragma once

#include "53l8a1_ranging_sensor.h"
#include <stdint.h>

#define VL53L8A1_MAX_RESOLUTION (8)
#define VL53L8A1_MAX_DATA_SIZE                                                 \
  (VL53L8A1_MAX_RESOLUTION * VL53L8A1_MAX_RESOLUTION)
#define VL53L8A1_PING_PONG_BUFFER_SIZE (2048)
#define VL53L8A1_TOFIS_CHECK_SUM_START_BYTE (4)

typedef struct {
  uint8_t start_byte;           // Fixed to 0xAA
  uint8_t resolution;           // 4 or 8
  uint8_t checksum;             // XOR checksum
  uint8_t end_byte;             // Fixed to 0x55
  RANGING_SENSOR_Result_t data; // Data
} tofis_data_packet_t;