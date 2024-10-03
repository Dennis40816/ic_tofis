#include "tofis_uart.h"
#include "check_sum.h"

static tofis_data_packet_t _packet;

/**
 * @brief Initializes the Slave UART device.
 *
 * @param device Pointer to the Slave device structure.
 * @param huart Pointer to the UART handle.
 */
void Tofis_Slave_USART_Init(tofis_slave_device_t *device,
                            UART_HandleTypeDef *huart) {
  device->huart = huart;
  memset(device->buffer, 0, VL53L8A1_PING_PONG_BUFFER_SIZE);
}

/**
 * @brief Sends data from the Slave to the Host.
 *
 * @param device Pointer to the Slave device structure.
 * @param resolution Matrix resolution (4 or 8).
 * @param result Pointer to the Ranging Sensor Result structure.
 * @return HAL_StatusTypeDef Status of the UART transmission.
 */
HAL_StatusTypeDef Tofis_Slave_USART_SendData(tofis_slave_device_t *device,
                                             uint8_t resolution,
                                             RANGING_SENSOR_Result_t *result) {
  uint8_t *buffer = device->buffer;
  int index = 0;

  // Start byte
  buffer[index++] = 0xAA;

  // Resolution byte
  buffer[index++] = resolution;

  // Placeholder for checksum
  int checksum_position = index++;

  // End byte (reserved byte)
  buffer[index++] = 0x55;

  // Serialize NumberOfZones
  buffer[index++] = (result->NumberOfZones >> 24) & 0xFF;
  buffer[index++] = (result->NumberOfZones >> 16) & 0xFF;
  buffer[index++] = (result->NumberOfZones >> 8) & 0xFF;
  buffer[index++] = result->NumberOfZones & 0xFF;

  // Serialize each ZoneResult
  for (uint32_t zone = 0; zone < result->NumberOfZones; zone++) {
    RANGING_SENSOR_ZoneResult_t *zone_result = &result->ZoneResult[zone];

    // NumberOfTargets
    buffer[index++] = zone_result->NumberOfTargets;

    // Distance array
    for (int i = 0; i < RANGING_SENSOR_NB_TARGET_PER_ZONE; i++) {
      buffer[index++] = (zone_result->Distance[i] >> 24) & 0xFF;
      buffer[index++] = (zone_result->Distance[i] >> 16) & 0xFF;
      buffer[index++] = (zone_result->Distance[i] >> 8) & 0xFF;
      buffer[index++] = zone_result->Distance[i] & 0xFF;
    }

    // Status array
    for (int i = 0; i < RANGING_SENSOR_NB_TARGET_PER_ZONE; i++) {
      buffer[index++] = (zone_result->Status[i] >> 24) & 0xFF;
      buffer[index++] = (zone_result->Status[i] >> 16) & 0xFF;
      buffer[index++] = (zone_result->Status[i] >> 8) & 0xFF;
      buffer[index++] = zone_result->Status[i] & 0xFF;
    }

    // Ambient array
    for (int i = 0; i < RANGING_SENSOR_NB_TARGET_PER_ZONE; i++) {
      float ambient = zone_result->Ambient[i];
      uint8_t *ambient_bytes = (uint8_t *)&ambient;
      buffer[index++] = ambient_bytes[3];
      buffer[index++] = ambient_bytes[2];
      buffer[index++] = ambient_bytes[1];
      buffer[index++] = ambient_bytes[0];
    }

    // Signal array
    for (int i = 0; i < RANGING_SENSOR_NB_TARGET_PER_ZONE; i++) {
      float signal = zone_result->Signal[i];
      uint8_t *signal_bytes = (uint8_t *)&signal;
      buffer[index++] = signal_bytes[3];
      buffer[index++] = signal_bytes[2];
      buffer[index++] = signal_bytes[1];
      buffer[index++] = signal_bytes[0];
    }
  }

  // Calculate checksum over resolution and data
  buffer[checksum_position] =
      calculate_checksum((uint8_t *)result, sizeof(RANGING_SENSOR_Result_t));

  // Transmit the buffer up to the current index
  HAL_StatusTypeDef status =
      HAL_UART_Transmit(device->huart, buffer, index, VL53L8A1_UART_MAX_DELAY);
  return status;
}

HAL_StatusTypeDef
Tofis_Slave_USART_SendData_Le(tofis_slave_device_t *device, uint8_t resolution,
                              RANGING_SENSOR_Result_t *result) {
  _packet.start_byte = 0xAA;
  _packet.resolution = resolution;
  _packet.end_byte = 0x55;

  memcpy(&_packet.data, result, sizeof(RANGING_SENSOR_Result_t));

  // calculate checksum of result field
  _packet.checksum =
      calculate_checksum((uint8_t *)result, sizeof(RANGING_SENSOR_Result_t));

  // transmit data packet
  return HAL_UART_Transmit(device->huart, (uint8_t *)&_packet,
                           sizeof(tofis_data_packet_t),
                           VL53L8A1_UART_MAX_DELAY);
}