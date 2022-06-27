/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WEPLATFORM_WEPLATFORM_H_
#define ZEPHYR_DRIVERS_SENSOR_WEPLATFORM_WEPLATFORM_H_

#include <stdbool.h>
#include <stdint.h>

#include "WeSensorsSDK.h"

/* Read a register's content */
extern int8_t WE_ReadReg(WE_sensorInterface_t *interface, uint8_t regAdr, uint16_t numBytesToRead,
			 uint8_t *data);

/* Write a register's content */
extern int8_t WE_WriteReg(WE_sensorInterface_t *interface, uint8_t regAdr, uint16_t numBytesToWrite,
			  uint8_t *data);

extern int8_t WE_isSensorInterfaceReady(WE_sensorInterface_t *interface);

#endif /* ZEPHYR_DRIVERS_SENSOR_WEPLATFORM_WEPLATFORM_H_ */
