/*
 * Copyright 2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "uwb_nxp_host.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

const struct device *const s_i2c_device = DEVICE_DT_GET(DT_ALIAS(uwb_i2c_device));

void uwb_host_get_id(uint8_t *aOutUid16B, uint8_t *pOutLen)
{
	const uint8_t uid[] = {"NXP-UWB"};
	if (*pOutLen >= sizeof(uid)) {
		memcpy(aOutUid16B, (uint8_t *)uid, sizeof(uid));
		*pOutLen = sizeof(uid);
	} else {
		*pOutLen = 0;
	}
}

void AddDelayInMicroSec(int delay)
{
	k_sleep(K_USEC(delay));
}

void *uwb_bus_i2c_init(void)
{
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

	if (!device_is_ready(s_i2c_device)) {
		return NULL;
	}
	if (i2c_configure(s_i2c_device, i2c_cfg)) {
		return NULL;
	}

	return (void *)s_i2c_device;
}

void uwb_bus_i2c_deinit(void *const context)
{
	(void)(context);
	return;
}

uwb_bus_status_t uwb_bus_i2c_data_tx(const void *const context, const uint8_t address,
				     const uint8_t *const txData, const uint32_t txLen)
{
	(void)(context);
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	int status = -1;

	if (txData == NULL || txLen == 0) {
		goto exit;
	}

	status = i2c_write(s_i2c_device, (uint8_t *)txData, txLen, (uint16_t)(address >> 1));
	if (status < 0) {
		goto exit;
	}
	status = 0;
	bus_status = kUWB_bus_Status_OK;

exit:
	return status;
}

uwb_bus_status_t uwb_bus_i2c_data_rx(const void *const context, const uint8_t address,
				     uint8_t *const rxData, const uint32_t rxLen)
{
	(void)(context);
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	int status = -1;

	if (rxData == NULL || rxLen == 0) {
		goto exit;
	}

	status = i2c_read(s_i2c_device, (uint8_t *)rxData, rxLen, (uint16_t)(address >> 1));
	if (status < 0) {
		goto exit;
	}

	status = 0;
	bus_status = kUWB_bus_Status_OK;
exit:
	return status;
}
