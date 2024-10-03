#pragma once
/**
 ******************************************************************************
 * @file          : app_tof.h
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

/* Exported defines ----------------------------------------------------------*/
// transmit through serial to host program, disable when you want to show result
// in terminal

#define TOFIS_TRANSMIT_RAW_DATA
/* Exported functions --------------------------------------------------------*/
void MX_TOFIS_Init(void);
void MX_TOFIS_Process(void);

#ifdef __cplusplus
}
#endif
