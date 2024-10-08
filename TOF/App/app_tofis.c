/**
 ******************************************************************************
 * @file          : app_tof.c
 * @author        : IMG SW Application Team
 * @brief         : This file provides code for the configuration
 *                  of the STMicroelectronics.X-CUBE-TOF1.3.4.2 instances.
 ******************************************************************************
 *
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_tofis.h"
#include "main.h"
#include <stdio.h>

#include "53l8a1_ranging_sensor.h"
#include "app_tof_pin_conf.h"
#include "stm32f4xx_nucleo.h"

#ifdef TOFIS_TRANSMIT_RAW_DATA
#include "tofis_uart.h"
#endif

/* Private typedef -----------------------------------------------------------*/
typedef uint8_t RANGING_SENSOR_Target_Order_t;

/* Private define ------------------------------------------------------------*/
#define TIMING_BUDGET (30U) /* 5 ms < TimingBudget < 100 ms */
#define RANGING_FREQUENCY                                                      \
  (10U) /* Ranging frequency Hz (shall be consistent with TimingBudget value)  \
         */

/* Private variables ---------------------------------------------------------*/
static RANGING_SENSOR_Capabilities_t Cap;
static RANGING_SENSOR_ProfileConfig_t Profile;
static RANGING_SENSOR_Result_t Result;
static RANGING_SENSOR_Target_Order_t TargetOrder =
    VL53L8CX_TARGET_ORDER_CLOSEST;
static int32_t status = 0;
static volatile uint8_t PushButtonDetected = 0;

static tofis_slave_device_t _tofis_slave_device;

// // already defined in app_tof.c
// volatile uint8_t ToF_EventDetected;
extern volatile uint8_t ToF_EventDetected;

/* Private function prototypes -----------------------------------------------*/
static void MX_53L8A1_SimpleRanging_Init(void);
static void MX_53L8A1_SimpleRanging_Process(void);
static void print_result(RANGING_SENSOR_Result_t *Result);
static void toggle_resolution(void);
static void toggle_signal_and_ambient(void);
static void clear_screen(void);
static void display_commands_banner(void);
static void handle_cmd(uint8_t cmd);
static uint8_t get_key(void);
static uint32_t com_has_data(void);

void MX_TOFIS_Init(void) {
  /* USER CODE BEGIN SV */

  /* USER CODE END SV */

  /* USER CODE BEGIN TOF_Init_PreTreatment */

  /* USER CODE END TOF_Init_PreTreatment */

  /* Initialize the peripherals and the TOF components */

  // // Init by MX_TOF_INIT()
  MX_53L8A1_SimpleRanging_Init();

  /* USER CODE BEGIN TOF_Init_PostTreatment */

  /* USER CODE END TOF_Init_PostTreatment */
}

/*
 * LM background task
 */
void MX_TOFIS_Process(void) {
  /* USER CODE BEGIN TOF_Process_PreTreatment */

  /* USER CODE END TOF_Process_PreTreatment */

  MX_53L8A1_SimpleRanging_Process();

  /* USER CODE BEGIN TOF_Process_PostTreatment */

  /* USER CODE END TOF_Process_PostTreatment */
}

/* Call only when device stops */
// VL53L8CX_TARGET_ORDER_CLOSEST or VL53L8CX_TARGET_ORDER_STRONGEST

static uint8_t Tofis_Set_Target_Order(uint8_t target_order) {
  VL53L8CX_Object_t *vl53l8cx_obj_p =
      (VL53L8CX_Object_t *)VL53L8A1_RANGING_SENSOR_CompObj[VL53L8A1_DEV_CENTER];
  return vl53l8cx_set_target_order(&(vl53l8cx_obj_p->Dev), target_order);
}

static uint8_t Tofis_Get_Target_Order(uint8_t target_order) {
  VL53L8CX_Object_t *vl53l8cx_obj_p =
      (VL53L8CX_Object_t *)VL53L8A1_RANGING_SENSOR_CompObj[VL53L8A1_DEV_CENTER];
  vl53l8cx_get_target_order(&(vl53l8cx_obj_p->Dev), &target_order);

  return target_order;
}

static uint8_t toggle_target_order() {
  VL53L8A1_RANGING_SENSOR_Stop(VL53L8A1_DEV_CENTER);

  TargetOrder = (TargetOrder == VL53L8CX_TARGET_ORDER_CLOSEST)
                    ? VL53L8CX_TARGET_ORDER_STRONGEST
                    : VL53L8CX_TARGET_ORDER_CLOSEST;
  
  Tofis_Set_Target_Order(TargetOrder);

  VL53L8A1_RANGING_SENSOR_Start(VL53L8A1_DEV_CENTER, RS_MODE_ASYNC_CONTINUOUS);
}

static void MX_53L8A1_SimpleRanging_Init(void) {
  /* Initialize Virtual COM Port */
  BSP_COM_DeInit(COM1);
  BSP_COM_Init(COM1);

  /* Initialize button */
  BSP_PB_DeInit(BUTTON_KEY);
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);

  /* Sensor reset */
  HAL_GPIO_WritePin(VL53L8A1_PWR_EN_C_PORT, VL53L8A1_PWR_EN_C_PIN,
                    GPIO_PIN_RESET);
  HAL_Delay(2);
  HAL_GPIO_WritePin(VL53L8A1_PWR_EN_C_PORT, VL53L8A1_PWR_EN_C_PIN,
                    GPIO_PIN_SET);
  HAL_Delay(2);
  HAL_GPIO_WritePin(VL53L8A1_LPn_C_PORT, VL53L8A1_LPn_C_PIN, GPIO_PIN_RESET);
  HAL_Delay(2);
  HAL_GPIO_WritePin(VL53L8A1_LPn_C_PORT, VL53L8A1_LPn_C_PIN, GPIO_PIN_SET);
  HAL_Delay(2);

  clear_screen();

#ifndef TOFIS_TRANSMIT_RAW_DATA
  printf("\033[2H\033[2J");
  printf("TOFIS Simple Ranging demo application\n");
  printf("Sensor initialization...\n");
#endif

  VL53L8A1_RANGING_SENSOR_DeInit(VL53L8A1_DEV_CENTER);
  status = VL53L8A1_RANGING_SENSOR_Init(VL53L8A1_DEV_CENTER);

  if (status != BSP_ERROR_NONE) {
    printf("VL53L8A1_RANGING_SENSOR_Init failed\n");
    while (1)
      ;
  }

  Tofis_Slave_USART_Init(&_tofis_slave_device, &huart2);
}

static void MX_53L8A1_SimpleRanging_Process(void) {
  uint32_t Id;

  VL53L8A1_RANGING_SENSOR_ReadID(VL53L8A1_DEV_CENTER, &Id);
  VL53L8A1_RANGING_SENSOR_GetCapabilities(VL53L8A1_DEV_CENTER, &Cap);

  Profile.RangingProfile = RS_PROFILE_8x8_CONTINUOUS;
  Profile.TimingBudget = TIMING_BUDGET;
  Profile.Frequency =
      RANGING_FREQUENCY;     /* Ranging frequency Hz (shall be consistent with
                                TimingBudget value) */
  Profile.EnableAmbient = 0; /* Enable: 1, Disable: 0 */
  Profile.EnableSignal = 0;  /* Enable: 1, Disable: 0 */

  /* set the profile if different from default one */
  VL53L8A1_RANGING_SENSOR_ConfigProfile(VL53L8A1_DEV_CENTER, &Profile);

  status = Tofis_Set_Target_Order(VL53L8CX_TARGET_ORDER_CLOSEST);

  if (status != BSP_ERROR_NONE) {
    printf("VL53L8A1_RANGING_SENSOR_Set to target order: closest failed!\n");
    while (1)
      ;
  } else {
    printf("VL53L8A1_RANGING_SENSOR_Set to target order: CLOEST!\n");
  }

  status = VL53L8A1_RANGING_SENSOR_Start(VL53L8A1_DEV_CENTER,
                                         RS_MODE_ASYNC_CONTINUOUS);

  if (status != BSP_ERROR_NONE) {
    printf("VL53L8A1_RANGING_SENSOR_Start failed\n");
    while (1)
      ;
  }

  while (1) {
    /* interrupt mode */
    if (ToF_EventDetected != 0) {
      ToF_EventDetected = 0;

      status =
          VL53L8A1_RANGING_SENSOR_GetDistance(VL53L8A1_DEV_CENTER, &Result);

      // transmit data
      if (status == BSP_ERROR_NONE) {
#ifdef TOFIS_TRANSMIT_RAW_DATA

        uint8_t zones_per_line =
            ((Profile.RangingProfile == RS_PROFILE_8x8_AUTONOMOUS) ||
             (Profile.RangingProfile == RS_PROFILE_8x8_CONTINUOUS))
                ? 8
                : 4;

        Tofis_Slave_USART_SendData_Le(&_tofis_slave_device, zones_per_line,
                                      &Result);
#else
        print_result(&Result);
#endif
      }
    }
    if (com_has_data()) {
      handle_cmd(get_key());
    }
  }
}

static void print_result(RANGING_SENSOR_Result_t *Result) {
  int8_t i;
  int8_t j;
  int8_t k;
  int8_t l;
  uint8_t zones_per_line;

  zones_per_line = ((Profile.RangingProfile == RS_PROFILE_8x8_AUTONOMOUS) ||
                    (Profile.RangingProfile == RS_PROFILE_8x8_CONTINUOUS))
                       ? 8
                       : 4;

  display_commands_banner();

  printf("Cell Format :\n\n");
  for (l = 0; l < RANGING_SENSOR_NB_TARGET_PER_ZONE; l++) {
    printf(" \033[38;5;10m%20s\033[0m : %20s\n", "Distance [mm]", "Status");
    if ((Profile.EnableAmbient != 0) || (Profile.EnableSignal != 0)) {
      printf(" %20s : %20s\n", "Signal [kcps/spad]", "Ambient [kcps/spad]");
    }
  }

  printf("\n\n");

  for (j = 0; j < Result->NumberOfZones; j += zones_per_line) {
    for (i = 0; i < zones_per_line; i++) /* number of zones per line */
    {
      printf(" -----------------");
    }
    printf("\n");

    for (i = 0; i < zones_per_line; i++) {
      printf("|                 ");
    }
    printf("|\n");

    for (l = 0; l < RANGING_SENSOR_NB_TARGET_PER_ZONE; l++) {
      /* Print distance and status */
      // inverse k logic
      for (k = (zones_per_line - 1); k >= 0; k--) {
        //   for (k = 0; k < zones_per_line; k++) {
        if (Result->ZoneResult[j + k].NumberOfTargets > 0)
          printf("| \033[38;5;10m%5ld\033[0m  :  %5ld ",
                 (long)Result->ZoneResult[j + k].Distance[l],
                 (long)Result->ZoneResult[j + k].Status[l]);
        else
          printf("| %5s  :  %5s ", "X", "X");
      }
      printf("|\n");

      if ((Profile.EnableAmbient != 0) || (Profile.EnableSignal != 0)) {
        /* Print Signal and Ambient */
        for (k = (zones_per_line - 1); k >= 0; k--) {
          if (Result->ZoneResult[j + k].NumberOfTargets > 0) {
            if (Profile.EnableSignal != 0) {
              printf("| %5ld  :  ", (long)Result->ZoneResult[j + k].Signal[l]);
            } else
              printf("| %5s  :  ", "X");

            if (Profile.EnableAmbient != 0) {
              printf("%5ld ", (long)Result->ZoneResult[j + k].Ambient[l]);
            } else
              printf("%5s ", "X");
          } else
            printf("| %5s  :  %5s ", "X", "X");
        }
        printf("|\n");
      }
    }
  }

  for (i = 0; i < zones_per_line; i++) {
    printf(" -----------------");
  }
  printf("\n");
}

static void toggle_resolution(void) {
  VL53L8A1_RANGING_SENSOR_Stop(VL53L8A1_DEV_CENTER);

  switch (Profile.RangingProfile) {
  case RS_PROFILE_4x4_AUTONOMOUS:
    Profile.RangingProfile = RS_PROFILE_8x8_AUTONOMOUS;
    break;

  case RS_PROFILE_4x4_CONTINUOUS:
    Profile.RangingProfile = RS_PROFILE_8x8_CONTINUOUS;
    break;

  case RS_PROFILE_8x8_AUTONOMOUS:
    Profile.RangingProfile = RS_PROFILE_4x4_AUTONOMOUS;
    break;

  case RS_PROFILE_8x8_CONTINUOUS:
    Profile.RangingProfile = RS_PROFILE_4x4_CONTINUOUS;
    break;

  default:
    break;
  }

  VL53L8A1_RANGING_SENSOR_ConfigProfile(VL53L8A1_DEV_CENTER, &Profile);
  VL53L8A1_RANGING_SENSOR_Start(VL53L8A1_DEV_CENTER, RS_MODE_ASYNC_CONTINUOUS);
}

static void toggle_signal_and_ambient(void) {
  VL53L8A1_RANGING_SENSOR_Stop(VL53L8A1_DEV_CENTER);

  Profile.EnableAmbient = (Profile.EnableAmbient) ? 0U : 1U;
  Profile.EnableSignal = (Profile.EnableSignal) ? 0U : 1U;

  VL53L8A1_RANGING_SENSOR_ConfigProfile(VL53L8A1_DEV_CENTER, &Profile);
  VL53L8A1_RANGING_SENSOR_Start(VL53L8A1_DEV_CENTER, RS_MODE_ASYNC_CONTINUOUS);
}

static void clear_screen(void) {
  // printf("%c[2J", 27); /* 27 is ESC command */
  printf("\033[2J\033[H");
}

static void display_commands_banner(void) {
  // move to second row
  printf("%c[2H", 27);

  printf("TOFIS Simple Ranging demo application\n");
  printf("--------------------------------------\n\n");

  printf("Use the following keys to control application\n");
  printf(" 'r' : change resolution\n");
  printf(" 's' : enable signal and ambient\n");
  printf(" 'c' : clear screen\n");
  printf(" 't' : toggle target order\n");
  printf("\n");
}

static void handle_cmd(uint8_t cmd) {
  switch (cmd) {
  case 'r':
    toggle_resolution();
    clear_screen();
    break;

  case 's':
    toggle_signal_and_ambient();
    clear_screen();
    break;

  case 'c':
    clear_screen();
    break;

  case 't':
    toggle_target_order();
    break;

  default:
    break;
  }
}

static uint8_t get_key(void) {
  uint8_t cmd = 0;

  HAL_UART_Receive(&hcom_uart[COM1], &cmd, 1, HAL_MAX_DELAY);

  return cmd;
}

static uint32_t com_has_data(void) {
  return __HAL_UART_GET_FLAG(&hcom_uart[COM1], UART_FLAG_RXNE);
  ;
}

// // declared in app_tof.c
// void BSP_PB_Callback(Button_TypeDef Button) { PushButtonDetected = 1; }

#ifdef __cplusplus
}
#endif
