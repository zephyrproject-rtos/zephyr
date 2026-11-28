/*
 * Copyright (c) 2026 Sacra Systems Private Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_H_
#define ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>
#include "scl3300_transport.h"
#include "scl3300_spi.h"

#define SCL3300_ANGLE_SCALE_FACTOR 90U
#define SCL3300_ANGLE_RESOLUTION   16384U
#define SCL3300_MICRO_UNIT         1000000U
#define SCL3300_RESPONSE_MSB_INDEX 1U
#define SCL3300_RESPONSE_LSB_INDEX 2U
#define SCL3300_FRAME_SHIFT_B1     8U
#define SCL3300_FRAME_SHIFT_B2     16U
#define SCL3300_FRAME_SHIFT_B3     24U

/* SCL3300 commands */
#define SCL3300_CMD_WHOAMI       0x40000091U
#define SCL3300_CMD_SW_RESET     0xB4002098U
#define SCL3300_CMD_WAKEUP       0xB4000000U
#define SCL3300_CMD_ANGLE_CTRL   0xB0001F6FU
#define SCL3300_CMD_STATUS       0x180000E5U
#define SCL3300_CMD_READ_ANGLE_X 0x240000C7U
#define SCL3300_CMD_READ_ANGLE_Y 0x280000CDU
#define SCL3300_CMD_READ_ANGLE_Z 0x2C0000CBU
#define SCL3300_CMD_READ_TEMP    0x140000EFU
#define SCL3300_CMD_POWER_DOWN   0xB400046BU

#define SCL3300_CMD_MODE1 0xB400001FU
#define SCL3300_CMD_MODE2 0xB4000102U
#define SCL3300_CMD_MODE3 0xB4000225U
#define SCL3300_CMD_MODE4 0xB4000338U

#define SCL3300_CMD_READ_CURRENT_BANK 0x7C0000B3U
#define SCL3300_CMD_SWITCH_BANK_0     0xFC000073U
#define SCL3300_CMD_SWITCH_BANK_1     0xFC00016EU

struct scl3300_config {
	const struct rtio_iodev *spi_iodev;

	int (*bus_init)(const struct device *dev);

	/* Initialization sequence configuration */
	bool init;
};

/* SCL3300 runtime data */
struct scl3300_data {
	struct rtio *rtio;

	scl3300_ctx_t *ctx;
	scl3300_ctx_t ctx_spi;

	const struct device *dev;

	int16_t angle_x;
	int16_t angle_y;
	int16_t angle_z;
};

int scl3300_spi_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_H_ */
