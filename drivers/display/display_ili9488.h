/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9488_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9488_H_

#include <zephyr/device.h>

/* Commands/registers. */
#define ILI9488_FRMCTR1 0xB1
#define ILI9488_DISCTRL 0xB6
#define ILI9488_PWCTRL1 0xC0
#define ILI9488_PWCTRL2 0xC1
#define ILI9488_VMCTRL 0xC5
#define ILI9488_PGAMCTRL 0xE0
#define ILI9488_NGAMCTRL 0xE1

/* Commands/registers length. */
#define ILI9488_FRMCTR1_LEN 2U
#define ILI9488_DISCTRL_LEN 3U
#define ILI9488_PWCTRL1_LEN 2U
#define ILI9488_PWCTRL2_LEN 1U
#define ILI9488_VMCTRL_LEN 4U
#define ILI9488_PGAMCTRL_LEN 15U
#define ILI9488_NGAMCTRL_LEN 15U

/** X resolution (pixels). */
#define ILI9488_X_RES 320U
/** Y resolution (pixels). */
#define ILI9488_Y_RES 480U

/** ILI9488 registers to be initialized. */
struct ili9488_regs {
	uint8_t frmctr1[ILI9488_FRMCTR1_LEN];
	uint8_t disctrl[ILI9488_DISCTRL_LEN];
	uint8_t pwctrl1[ILI9488_PWCTRL1_LEN];
	uint8_t pwctrl2[ILI9488_PWCTRL2_LEN];
	uint8_t vmctrl[ILI9488_VMCTRL_LEN];
	uint8_t pgamctrl[ILI9488_PGAMCTRL_LEN];
	uint8_t ngamctrl[ILI9488_NGAMCTRL_LEN];
};

/* Initializer macro for ILI9488 registers. */
#define ILI9488_REGS_INIT(n)                                                   \
	static const struct ili9488_regs ili9xxx_regs_##n = {                  \
		.frmctr1 = DT_PROP(DT_INST(n, ilitek_ili9488), frmctr1),       \
		.disctrl = DT_PROP(DT_INST(n, ilitek_ili9488), disctrl),       \
		.pwctrl1 = DT_PROP(DT_INST(n, ilitek_ili9488), pwctrl1),       \
		.pwctrl2 = DT_PROP(DT_INST(n, ilitek_ili9488), pwctrl2),       \
		.vmctrl = DT_PROP(DT_INST(n, ilitek_ili9488), vmctrl),         \
		.pgamctrl = DT_PROP(DT_INST(n, ilitek_ili9488), pgamctrl),     \
		.ngamctrl = DT_PROP(DT_INST(n, ilitek_ili9488), ngamctrl),     \
	}

/**
 * @brief Initialize ILI9488 registers with DT values.
 *
 * @param dev ILI9488 device instance
 * @return 0 on success, errno otherwise.
 */
int ili9488_regs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9488_H_ */
