/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9XXX_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9XXX_H_

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/sys/util.h>

/* Commands/registers. */
#define ILI9XXX_SWRESET 0x01
#define ILI9XXX_SLPOUT 0x11
#define ILI9XXX_DINVON 0x21
#define ILI9XXX_GAMSET 0x26
#define ILI9XXX_DISPOFF 0x28
#define ILI9XXX_DISPON 0x29
#define ILI9XXX_CASET 0x2a
#define ILI9XXX_PASET 0x2b
#define ILI9XXX_RAMWR 0x2c
#define ILI9XXX_RGBSET 0x2d
#define ILI9XXX_RAMRD 0x2e
#define ILI9XXX_MADCTL 0x36
#define ILI9XXX_PIXSET 0x3A
#define ILI9XXX_RAMRD_CONT 0x3e

/* MADCTL register fields. */
#define ILI9XXX_MADCTL_MY BIT(7U)
#define ILI9XXX_MADCTL_MX BIT(6U)
#define ILI9XXX_MADCTL_MV BIT(5U)
#define ILI9XXX_MADCTL_ML BIT(4U)
#define ILI9XXX_MADCTL_BGR BIT(3U)
#define ILI9XXX_MADCTL_MH BIT(2U)

/* PIXSET register fields. */
#define ILI9XXX_PIXSET_RGB_18_BIT 0x60
#define ILI9XXX_PIXSET_RGB_16_BIT 0x50
#define ILI9XXX_PIXSET_MCU_18_BIT 0x06
#define ILI9XXX_PIXSET_MCU_16_BIT 0x05

/** Command/data GPIO level for commands. */
#define ILI9XXX_CMD 1U
/** Command/data GPIO level for data. */
#define ILI9XXX_DATA 0U

/** Sleep out time (ms), ref. 8.2.12 of ILI9XXX manual. */
#define ILI9XXX_SLEEP_OUT_TIME 120

/** Reset pulse time (ms), ref 15.4 of ILI9XXX manual. */
#define ILI9XXX_RESET_PULSE_TIME 1

/** Reset wait time (ms), ref 15.4 of ILI9XXX manual. */
#define ILI9XXX_RESET_WAIT_TIME 5

enum madctl_cmd_set {
	CMD_SET_1,	/* Default for most of ILI9xxx display controllers */
	CMD_SET_2,	/* Used by ILI9342c */
};

struct ili9xxx_quirks {
	enum madctl_cmd_set cmd_set;
};

struct ili9xxx_config {
	const struct ili9xxx_quirks *quirks;
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
	uint8_t pixel_format;
	uint16_t rotation;
	uint16_t x_resolution;
	uint16_t y_resolution;
	bool inversion;
	const void *regs;
	int (*regs_init_fn)(const struct device *dev);
};

int ili9xxx_transmit(const struct device *dev, uint8_t cmd,
		     const void *tx_data, size_t tx_len);

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9XXX_H_ */
