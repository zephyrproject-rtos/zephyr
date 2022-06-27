/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9340_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9340_H_

#include <zephyr/device.h>

/* Commands/registers. */
#define ILI9340_GAMSET 0x26
#define ILI9340_FRMCTR1 0xB1
#define ILI9340_DISCTRL 0xB6
#define ILI9340_PWCTRL1 0xC0
#define ILI9340_PWCTRL2 0xC1
#define ILI9340_VMCTRL1 0xC5
#define ILI9340_VMCTRL2 0xC7
#define ILI9340_PGAMCTRL 0xE0
#define ILI9340_NGAMCTRL 0xE1

/* Commands/registers length. */
#define ILI9340_GAMSET_LEN 1U
#define ILI9340_FRMCTR1_LEN 2U
#define ILI9340_DISCTRL_LEN 3U
#define ILI9340_PWCTRL1_LEN 2U
#define ILI9340_PWCTRL2_LEN 1U
#define ILI9340_VMCTRL1_LEN 2U
#define ILI9340_VMCTRL2_LEN 1U
#define ILI9340_PGAMCTRL_LEN 15U
#define ILI9340_NGAMCTRL_LEN 15U

/** X resolution (pixels). */
#define ILI9340_X_RES 240U
/** Y resolution (pixels). */
#define ILI9340_Y_RES 320U

/** ILI9340 registers to be initialized. */
struct ili9340_regs {
	uint8_t gamset[ILI9340_GAMSET_LEN];
	uint8_t frmctr1[ILI9340_FRMCTR1_LEN];
	uint8_t disctrl[ILI9340_DISCTRL_LEN];
	uint8_t pwctrl1[ILI9340_PWCTRL1_LEN];
	uint8_t pwctrl2[ILI9340_PWCTRL2_LEN];
	uint8_t vmctrl1[ILI9340_VMCTRL1_LEN];
	uint8_t vmctrl2[ILI9340_VMCTRL2_LEN];
	uint8_t pgamctrl[ILI9340_PGAMCTRL_LEN];
	uint8_t ngamctrl[ILI9340_NGAMCTRL_LEN];
};

/* Initializer macro for ILI9340 registers. */
#define ILI9340_REGS_INIT(n)                                                   \
	static const struct ili9340_regs ili9xxx_regs_##n = {                  \
		.gamset = DT_PROP(DT_INST(n, ilitek_ili9340), gamset),         \
		.frmctr1 = DT_PROP(DT_INST(n, ilitek_ili9340), frmctr1),       \
		.disctrl = DT_PROP(DT_INST(n, ilitek_ili9340), disctrl),       \
		.pwctrl1 = DT_PROP(DT_INST(n, ilitek_ili9340), pwctrl1),       \
		.pwctrl2 = DT_PROP(DT_INST(n, ilitek_ili9340), pwctrl2),       \
		.vmctrl1 = DT_PROP(DT_INST(n, ilitek_ili9340), vmctrl1),       \
		.vmctrl2 = DT_PROP(DT_INST(n, ilitek_ili9340), vmctrl2),       \
		.pgamctrl = DT_PROP(DT_INST(n, ilitek_ili9340), pgamctrl),     \
		.ngamctrl = DT_PROP(DT_INST(n, ilitek_ili9340), ngamctrl),     \
	}

/**
 * @brief Initialize ILI9340 registers with DT values.
 *
 * @param dev ILI9340 device instance
 * @return 0 on success, errno otherwise.
 */
int ili9340_regs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9340_H_ */
