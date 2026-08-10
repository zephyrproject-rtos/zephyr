/*
 * Copyright (C) 2026 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __UWB_NXP_HOST_H__
#define __UWB_NXP_HOST_H__

#include <stdio.h>
#include <string.h>
#include <uwb_board_values.h>

#include "uwb_bus_board.h"
#include "uwb_bus_interface.h"

#define I2C_IDLE             0
#define I2C_STARTED          1
#define I2C_RESTARTED        2
#define I2C_REPEATED_START   3
#define DATA_ACK             4
#define DATA_NACK            5
#define I2C_BUSY             6
#define I2C_NO_DATA          7
#define I2C_NACK_ON_ADDRESS  8
#define I2C_NACK_ON_DATA     9
#define I2C_ARBITRATION_LOST 10
#define I2C_TIME_OUT         11
#define I2C_OK               12
#define I2C_FAILED           13

/* Crete Extender 7 bit I2C Address */
#define CRETE_GPIO_EXTENDER_SLAVE_ADDR 0x20
/* Virgo EVK Extender 7 bit I2C Address */
#define VIRGO_GPIO_EXTENDER_SLAVE_ADDR 0x34

typedef enum Evk_revision {
	CRETE_REV_B = 0,    // With IO Expander
	VIRGO_REV_A = 1,    // With IO Expander
	NO_IO_EXPANDER = 2, // Without IO Expander
} eEvkRevision;

extern i2c_bus_board_ctx_t i2cCtx;

uwb_bus_status_t BOARD_I2C_Init(i2c_bus_board_ctx_t *pCtx);
uwb_bus_status_t BOARD_I2C_Send(i2c_bus_board_ctx_t *pCtx, unsigned char slave_addr, uint8_t *pBuf,
				size_t bufLen);
uwb_bus_status_t BOARD_I2C_Receive(i2c_bus_board_ctx_t *pCtx, unsigned char slave_addr,
				   uint8_t *pBuf, size_t pBufLen);
uwb_bus_status_t BOARD_Get_Sr2xxEvkVersion(void);
void uwb_host_get_id(uint8_t *aOutUid16B, uint8_t *pOutLen);
void uwb_host_i2c_initialize(void);
int uwb_host_i2c_send(unsigned char slave_addr, uint8_t *pBuf, size_t bufLen);
int uwb_host_i2c_receive(unsigned char slave_addr, uint8_t *pBuf, size_t pBufLen);
int uwb_host_i2c_deinitialize(void);
void AddDelayInMicroSec(int delay);

#endif // __UWB_NXP_HOST_H__
