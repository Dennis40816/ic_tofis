// tofis_data.h
#pragma once

#include <stdint.h>

#define RANGING_SENSOR_NB_TARGET_PER_ZONE 1
#define RANGING_SENSOR_MAX_NB_ZONES 64

typedef struct {
    uint8_t NumberOfTargets;
    uint32_t Distance[RANGING_SENSOR_NB_TARGET_PER_ZONE]; /*!< millimeters */
    uint32_t Status[RANGING_SENSOR_NB_TARGET_PER_ZONE];   /*!< OK: 0, NOK: !0 */
    float Ambient[RANGING_SENSOR_NB_TARGET_PER_ZONE];     /*!< kcps / spad */
    float Signal[RANGING_SENSOR_NB_TARGET_PER_ZONE];      /*!< kcps / spad */
} RANGING_SENSOR_ZoneResult_t;

typedef struct {
    uint32_t NumberOfZones;
    RANGING_SENSOR_ZoneResult_t ZoneResult[RANGING_SENSOR_MAX_NB_ZONES];
} RANGING_SENSOR_Result_t;

typedef struct {
    uint8_t start_byte;           // Fixed to 0xAA
    uint8_t resolution;           // 4 or 8
    uint8_t checksum;             // XOR checksum
    uint8_t end_byte;             // Fixed to 0x55
    RANGING_SENSOR_Result_t data; // Data
} tofis_data_packet_t;
