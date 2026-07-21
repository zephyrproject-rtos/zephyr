/*
 * Copyright (c) 2025 CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9163C_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9163C_H_

#include <zephyr/device.h>

/* Commands/registers. */
#define ILI9163C_GAMSET   0x26
#define ILI9163C_FRMCTR1  0xB1
#define ILI9163C_PWCTRL1  0xC0
#define ILI9163C_PWCTRL2  0xC1
#define ILI9163C_PWCTRL3  0xC2
#define ILI9163C_PWCTRL4  0xC3
#define ILI9163C_PWCTRL5  0xC4
#define ILI9163C_VMCTRL1  0xC5
#define ILI9163C_VMCTRL2  0xC7
#define ILI9163C_PGAMCTRL 0xE0
#define ILI9163C_NGAMCTRL 0xE1
#define ILI9163C_GAMARSEL 0xF2

/* Commands/registers length. */
#define ILI9163C_GAMSET_LEN   1U
#define ILI9163C_FRMCTR1_LEN  2U
#define ILI9163C_PGAMCTRL_LEN 15U
#define ILI9163C_NGAMCTRL_LEN 15U
#define ILI9163C_GAMARSEL_LEN 1U
#define ILI9163C_PWCTRL1_LEN  2U
#define ILI9163C_PWCTRL2_LEN  1U
#define ILI9163C_PWCTRL3_LEN  1U
#define ILI9163C_PWCTRL4_LEN  1U
#define ILI9163C_PWCTRL5_LEN  1U
#define ILI9163C_VMCTRL1_LEN  2U
#define ILI9163C_VMCTRL2_LEN  1U

/** ILI9163C registers to be initialized. */
struct ili9163c_regs {
	uint8_t gamset[ILI9163C_GAMSET_LEN];
	uint8_t frmctr1[ILI9163C_FRMCTR1_LEN];
	uint8_t pgamctrl[ILI9163C_PGAMCTRL_LEN];
	uint8_t ngamctrl[ILI9163C_NGAMCTRL_LEN];
	uint8_t pwctrl1[ILI9163C_PWCTRL1_LEN];
	uint8_t pwctrl2[ILI9163C_PWCTRL2_LEN];
	uint8_t pwctrl3[ILI9163C_PWCTRL3_LEN];
	uint8_t pwctrl4[ILI9163C_PWCTRL4_LEN];
	uint8_t pwctrl5[ILI9163C_PWCTRL5_LEN];
	uint8_t vmctrl1[ILI9163C_VMCTRL1_LEN];
	uint8_t vmctrl2[ILI9163C_VMCTRL2_LEN];
	uint8_t gamarsel[ILI9163C_GAMARSEL_LEN];
};

/* Initializer macro for ILI9163C registers. */
#define ILI9163c_REGS_INIT(n)                                                                      \
	static const struct ili9163c_regs ili9163c_regs_##n = {                                    \
		.gamset = DT_PROP(DT_INST(n, ilitek_ili9163c), gamset),                            \
		.frmctr1 = DT_PROP(DT_INST(n, ilitek_ili9163c), frmctr1),                          \
		.pgamctrl = DT_PROP(DT_INST(n, ilitek_ili9163c), pgamctrl),                        \
		.ngamctrl = DT_PROP(DT_INST(n, ilitek_ili9163c), ngamctrl),                        \
		.pwctrl1 = DT_PROP(DT_INST(n, ilitek_ili9163c), pwctrl1),                          \
		.pwctrl2 = DT_PROP(DT_INST(n, ilitek_ili9163c), pwctrl2),                          \
		.pwctrl3 = {DT_ENUM_IDX(DT_INST(n, ilitek_ili9163c), pwctrl3)},                    \
		.pwctrl4 = {DT_ENUM_IDX(DT_INST(n, ilitek_ili9163c), pwctrl4)},                    \
		.pwctrl5 = {DT_ENUM_IDX(DT_INST(n, ilitek_ili9163c), pwctrl5)},                    \
		.vmctrl1 = DT_PROP(DT_INST(n, ilitek_ili9163c), vmctrl1),                          \
		.vmctrl2 = DT_PROP(DT_INST(n, ilitek_ili9163c), vmctrl2),                          \
		.gamarsel = DT_PROP(DT_INST(n, ilitek_ili9163c), gamarsel),                        \
	};

/**
 * @brief Initialize ILI9163C registers with DT values.
 *
 * @param dev ILI9163C device instance
 * @return 0 on success, errno otherwise.
 */
int ili9163c_regs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9163C_H_ */
