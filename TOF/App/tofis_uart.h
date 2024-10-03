#pragma once

#include "stm32f4xx_hal.h"
#include "tofis_data.h"

// TODO: change to driver folder?
#define VL53L8A1_UART_MAX_DELAY (500)

/**
 * @brief Structure representing the Slave UART device.
 */
typedef struct {
  UART_HandleTypeDef *huart;                      /**< UART handle */
  uint8_t buffer[VL53L8A1_PING_PONG_BUFFER_SIZE]; /**< UART buffer */
} tofis_slave_device_t;

/**
 * @brief Initializes the Slave UART device.
 *
 * @param device Pointer to the Slave device structure.
 * @param huart Pointer to the UART handle.
 */
void Tofis_Slave_USART_Init(tofis_slave_device_t *device,
                            UART_HandleTypeDef *huart);

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
                                             RANGING_SENSOR_Result_t *result);

/**
 * @brief Sends data from the Slave to the Host (Assume little endian in both
 * side).
 *
 * @param device Pointer to the Slave device structure.
 * @param resolution Matrix resolution (4 or 8).
 * @param result Pointer to the Ranging Sensor Result structure.
 * @return HAL_StatusTypeDef Status of the UART transmission.
 */
HAL_StatusTypeDef
Tofis_Slave_USART_SendData_Le(tofis_slave_device_t *device, uint8_t resolution,
                              RANGING_SENSOR_Result_t *result);
