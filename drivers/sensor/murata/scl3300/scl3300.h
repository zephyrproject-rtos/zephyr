
/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_H_
#define ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

#include "scl3300_spi.h"

/* SCL3300 SPI frame size */
#define SCL3300_FRAME_SIZE             4U

/* SCL3300 commands */
#define SCL3300_CMD_WHOAMI             0x40000091U
#define SCL3300_CMD_SW_RESET           0xB4002098U
#define SCL3300_CMD_WAKEUP             0xB4000000U
#define SCL3300_CMD_ANG_CTRL           0xB0001F6FU
#define SCL3300_CMD_STATUS             0x180000E5U
#define SCL3300_CMD_READ_ANG_X         0x240000C7U
#define SCL3300_CMD_READ_ANG_Y         0x280000CDU
#define SCL3300_CMD_READ_ANG_Z         0x2C0000CBU
#define SCL3300_CMD_READ_TEMP          0x140000EFU

#define SCL3300_CMD_MODE1              0xB400001FU
#define SCL3300_CMD_MODE2              0xB4000102U
#define SCL3300_CMD_MODE3              0xB4000225U
#define SCL3300_CMD_MODE4              0xB4000338U

#define SCL3300_CMD_READ_CURRENT_BANK  0x7C0000B3U
#define SCL3300_CMD_SWITCH_BANK_0      0xFC000073U
#define SCL3300_CMD_SWITCH_BANK_1      0xFC00016EU

struct scl3300_config {
	const struct rtio_iodev *spi_iodev;

	/* Initialization sequence configuration */
	bool init;

	/* Measurement configuration */
	uint32_t mode;

	/* Register bank configuration */
	uint8_t initial_bank;
};

/* SCL3300 runtime data */
struct scl3300_data {
	struct rtio *rtio;

	uint8_t tx_buf[SCL3300_FRAME_SIZE];
	uint8_t rx_buf[SCL3300_FRAME_SIZE];

	const struct device *dev;
};

/* SPI communication */
int scl3300_spi_submit(const struct device *dev,
		       const uint8_t *tx_buf,
		       uint8_t *rx_buf,
		       size_t len);

/* Sensor configuration */
int scl3300_safely_configure(const struct device *dev,
			     const struct scl3300_config *cfg);

int scl3300_configure(const struct device *dev,
		      const struct scl3300_config *cfg);

#endif /* ZEPHYR_DRIVERS_SENSOR_SCL3300_SCL3300_H_ */
