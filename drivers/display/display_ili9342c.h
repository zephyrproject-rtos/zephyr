/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 * Copyright (c) 2022 Konstantinos Papadopoulos <kostas.papadopulos@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9342C_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9342C_H_

#include <zephyr/device.h>

/* Commands/registers. */
#define ILI9342C_GAMSET			0x26
#define ILI9342C_IFMODE			0xB0
#define ILI9342C_FRMCTR1		0xB1
#define ILI9342C_INVTR			0xB4
#define ILI9342C_DISCTRL		0xB6
#define ILI9342C_ETMOD			0xB7
#define ILI9342C_PWCTRL1		0xC0
#define ILI9342C_PWCTRL2		0xC1
#define ILI9342C_PWCTRL3		0xC2
#define ILI9342C_VMCTRL1		0xC5
#define ILI9342C_SETEXTC		0xC8
#define ILI9342C_PGAMCTRL		0xE0
#define ILI9342C_NGAMCTRL		0xE1
#define ILI9342C_IFCTL			0xF6

/* Commands/registers length. */
#define ILI9342C_GAMSET_LEN		1U
#define ILI9342C_IFMODE_LEN		1U
#define ILI9342C_FRMCTR1_LEN		2U
#define ILI9342C_INVTR_LEN		1U
#define ILI9342C_DISCTRL_LEN		4U
#define ILI9342C_ETMOD_LEN		1U
#define ILI9342C_PWCTRL1_LEN		2U
#define ILI9342C_PWCTRL2_LEN		1U
#define ILI9342C_PWCTRL3_LEN		1U
#define ILI9342C_VMCTRL1_LEN		1U
#define ILI9342C_SETEXTC_LEN		3U
#define ILI9342C_PGAMCTRL_LEN		15U
#define ILI9342C_NGAMCTRL_LEN		15U
#define ILI9342C_IFCTL_LEN		3U

/** X resolution (pixels). */
#define ILI9342c_X_RES 320U
/** Y resolution (pixels). */
#define ILI9342c_Y_RES 240U

/** ILI9342C registers to be initialized. */
struct ili9342c_regs {
	uint8_t gamset[ILI9342C_GAMSET_LEN];
	uint8_t ifmode[ILI9342C_IFMODE_LEN];
	uint8_t frmctr1[ILI9342C_FRMCTR1_LEN];
	uint8_t invtr[ILI9342C_INVTR_LEN];
	uint8_t disctrl[ILI9342C_DISCTRL_LEN];
	uint8_t etmod[ILI9342C_ETMOD_LEN];
	uint8_t pwctrl1[ILI9342C_PWCTRL1_LEN];
	uint8_t pwctrl2[ILI9342C_PWCTRL2_LEN];
	uint8_t pwctrl3[ILI9342C_PWCTRL3_LEN];
	uint8_t vmctrl1[ILI9342C_VMCTRL1_LEN];
	uint8_t setextc[ILI9342C_SETEXTC_LEN];
	uint8_t pgamctrl[ILI9342C_PGAMCTRL_LEN];
	uint8_t ngamctrl[ILI9342C_NGAMCTRL_LEN];
	uint8_t ifctl[ILI9342C_IFCTL_LEN];
};

/* Initializer macro for ILI9342C registers. */
#define ILI9342c_REGS_INIT(n)                                           \
	static const struct ili9342c_regs ili9xxx_regs_##n = {		\
	.gamset = DT_PROP(DT_INST(n, ilitek_ili9342c), gamset),		\
	.ifmode = DT_PROP(DT_INST(n, ilitek_ili9342c), ifmode),		\
	.frmctr1 = DT_PROP(DT_INST(n, ilitek_ili9342c), frmctr1),	\
	.invtr = DT_PROP(DT_INST(n, ilitek_ili9342c), invtr),		\
	.disctrl = DT_PROP(DT_INST(n, ilitek_ili9342c), disctrl),	\
	.etmod = DT_PROP(DT_INST(n, ilitek_ili9342c), etmod),		\
	.pwctrl1 = DT_PROP(DT_INST(n, ilitek_ili9342c), pwctrl1),	\
	.pwctrl2 = DT_PROP(DT_INST(n, ilitek_ili9342c), pwctrl2),	\
	.pwctrl3 = DT_PROP(DT_INST(n, ilitek_ili9342c), pwctrl3),	\
	.vmctrl1 = DT_PROP(DT_INST(n, ilitek_ili9342c), vmctrl1),	\
	.setextc = {0xFF, 0x93, 0x42},                                  \
	.pgamctrl = DT_PROP(DT_INST(n, ilitek_ili9342c), pgamctrl),	\
	.ngamctrl = DT_PROP(DT_INST(n, ilitek_ili9342c), ngamctrl),	\
	.ifctl = DT_PROP(DT_INST(n, ilitek_ili9342c), ifctl),		\
	};

/**
 * @brief Initialize ILI9342C registers with DT values.
 *
 * @param dev ILI9342C device instance
 * @return 0 on success, errno otherwise.
 */
int ili9342c_regs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9342C_H_ */
