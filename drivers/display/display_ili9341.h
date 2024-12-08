/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 * Copyright (c) 2022 Konstantinos Papadopoulos <kostas.papadopulos@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9341_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9341_H_

#include <zephyr/device.h>

/* Commands/registers. */
#define ILI9341_GAMSET 0x26
#define ILI9341_IFMODE 0xB0
#define ILI9341_FRMCTR1 0xB1
#define ILI9341_DISCTRL 0xB6
#define ILI9341_ETMOD 0xB7
#define ILI9341_PWCTRL1 0xC0
#define ILI9341_PWCTRL2 0xC1
#define ILI9341_VMCTRL1 0xC5
#define ILI9341_VMCTRL2 0xC7
#define ILI9341_PWCTRLA 0xCB
#define ILI9341_PWCTRLB 0xCF
#define ILI9341_PGAMCTRL 0xE0
#define ILI9341_NGAMCTRL 0xE1
#define ILI9341_TIMCTRLA 0xE8
#define ILI9341_TIMCTRLB 0xEA
#define ILI9341_PWSEQCTRL 0xED
#define ILI9341_ENABLE3G 0xF2
#define ILI9341_IFCTL 0xF6
#define ILI9341_PUMPRATIOCTRL 0xF7

/* Commands/registers length. */
#define ILI9341_GAMSET_LEN 1U
#define ILI9341_IFMODE_LEN 1U
#define ILI9341_FRMCTR1_LEN 2U
#define ILI9341_DISCTRL_LEN 4U
#define ILI9341_PWCTRL1_LEN 1U
#define ILI9341_PWCTRL2_LEN 1U
#define ILI9341_VMCTRL1_LEN 2U
#define ILI9341_VMCTRL2_LEN 1U
#define ILI9341_PGAMCTRL_LEN 15U
#define ILI9341_NGAMCTRL_LEN 15U
#define ILI9341_PWCTRLA_LEN 5U
#define ILI9341_PWCTRLB_LEN 3U
#define ILI9341_PWSEQCTRL_LEN 4U
#define ILI9341_TIMCTRLA_LEN 3U
#define ILI9341_TIMCTRLB_LEN 2U
#define ILI9341_PUMPRATIOCTRL_LEN 1U
#define ILI9341_ENABLE3G_LEN 1U
#define ILI9341_IFCTL_LEN 3U
#define ILI9341_ETMOD_LEN 1U

/** X resolution (pixels). */
#define ILI9341_X_RES 240U
/** Y resolution (pixels). */
#define ILI9341_Y_RES 320U

/** ILI9341 registers to be initialized. */
struct ili9341_regs {
	uint8_t gamset[ILI9341_GAMSET_LEN];
	uint8_t ifmode[ILI9341_IFMODE_LEN];
	uint8_t frmctr1[ILI9341_FRMCTR1_LEN];
	uint8_t disctrl[ILI9341_DISCTRL_LEN];
	uint8_t pwctrl1[ILI9341_PWCTRL1_LEN];
	uint8_t pwctrl2[ILI9341_PWCTRL2_LEN];
	uint8_t vmctrl1[ILI9341_VMCTRL1_LEN];
	uint8_t vmctrl2[ILI9341_VMCTRL2_LEN];
	uint8_t pgamctrl[ILI9341_PGAMCTRL_LEN];
	uint8_t ngamctrl[ILI9341_NGAMCTRL_LEN];
	uint8_t pwctrla[ILI9341_PWCTRLA_LEN];
	uint8_t pwctrlb[ILI9341_PWCTRLB_LEN];
	uint8_t pwseqctrl[ILI9341_PWSEQCTRL_LEN];
	uint8_t timctrla[ILI9341_TIMCTRLA_LEN];
	uint8_t timctrlb[ILI9341_TIMCTRLB_LEN];
	uint8_t pumpratioctrl[ILI9341_PUMPRATIOCTRL_LEN];
	uint8_t enable3g[ILI9341_ENABLE3G_LEN];
	uint8_t ifctl[ILI9341_IFCTL_LEN];
	uint8_t etmod[ILI9341_ETMOD_LEN];
};

/* Initializer macro for ILI9341 registers. */
#define ILI9341_REGS_INIT(n)                                                                       \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), gamset) == ILI9341_GAMSET_LEN,        \
		     "ili9341: Error length gamma set (GAMSET) register");                         \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), ifmode) == ILI9341_IFMODE_LEN,        \
		     "ili9341: Error length frame rate control (IFMODE) register");                \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), frmctr1) == ILI9341_FRMCTR1_LEN,      \
		     "ili9341: Error length frame rate control (FRMCTR1) register");               \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), disctrl) == ILI9341_DISCTRL_LEN,      \
		     "ili9341: Error length display function control (DISCTRL) register");         \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), pwctrl1) == ILI9341_PWCTRL1_LEN,      \
		     "ili9341: Error length power control 1 (PWCTRL1) register");                  \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), pwctrl2) == ILI9341_PWCTRL2_LEN,      \
		     "ili9341: Error length power control 2 (PWCTRL2) register");                  \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), vmctrl1) == ILI9341_VMCTRL1_LEN,      \
		     "ili9341: Error length VCOM control 1 (VMCTRL1) register");                   \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), vmctrl2) == ILI9341_VMCTRL2_LEN,      \
		     "ili9341: Error length VCOM control 2 (VMCTRL2) register");                   \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), pgamctrl) == ILI9341_PGAMCTRL_LEN,    \
		     "ili9341: Error length positive gamma correction (PGAMCTRL) register");       \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), ngamctrl) == ILI9341_NGAMCTRL_LEN,    \
		     "ili9341: Error length negative gamma correction (NGAMCTRL) register");       \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), pwctrla) == ILI9341_PWCTRLA_LEN,      \
		     "ili9341: Error length power control A (PWCTRLA) register");                  \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), pwctrlb) == ILI9341_PWCTRLB_LEN,      \
		     "ili9341: Error length power control B (PWCTRLB) register");                  \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), pwseqctrl) == ILI9341_PWSEQCTRL_LEN,  \
		     "ili9341: Error length power on sequence control (PWSEQCTRL) register");      \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), timctrla) == ILI9341_TIMCTRLA_LEN,    \
		     "ili9341: Error length driver timing control A (TIMCTRLA) register");         \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), timctrlb) == ILI9341_TIMCTRLB_LEN,    \
		     "ili9341: Error length driver timing control B (TIMCTRLB) register");         \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), pumpratioctrl) ==                     \
			     ILI9341_PUMPRATIOCTRL_LEN,                                            \
		     "ili9341: Error length Pump ratio control (PUMPRATIOCTRL) register");         \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), enable3g) == ILI9341_ENABLE3G_LEN,    \
		     "ili9341: Error length enable 3G (ENABLE3G) register");                       \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), ifctl) == ILI9341_IFCTL_LEN,          \
		     "ili9341: Error length frame rate control (IFCTL) register");                 \
	BUILD_ASSERT(DT_PROP_LEN(DT_INST(n, ilitek_ili9341), etmod) == ILI9341_ETMOD_LEN,          \
		     "ili9341: Error length entry Mode Set (ETMOD) register");                     \
	static const struct ili9341_regs ili9341_regs_##n = {                                      \
		.gamset = DT_PROP(DT_INST(n, ilitek_ili9341), gamset),                             \
		.ifmode = DT_PROP(DT_INST(n, ilitek_ili9341), ifmode),                             \
		.frmctr1 = DT_PROP(DT_INST(n, ilitek_ili9341), frmctr1),                           \
		.disctrl = DT_PROP(DT_INST(n, ilitek_ili9341), disctrl),                           \
		.pwctrl1 = DT_PROP(DT_INST(n, ilitek_ili9341), pwctrl1),                           \
		.pwctrl2 = DT_PROP(DT_INST(n, ilitek_ili9341), pwctrl2),                           \
		.vmctrl1 = DT_PROP(DT_INST(n, ilitek_ili9341), vmctrl1),                           \
		.vmctrl2 = DT_PROP(DT_INST(n, ilitek_ili9341), vmctrl2),                           \
		.pgamctrl = DT_PROP(DT_INST(n, ilitek_ili9341), pgamctrl),                         \
		.ngamctrl = DT_PROP(DT_INST(n, ilitek_ili9341), ngamctrl),                         \
		.pwctrla = DT_PROP(DT_INST(n, ilitek_ili9341), pwctrla),                           \
		.pwctrlb = DT_PROP(DT_INST(n, ilitek_ili9341), pwctrlb),                           \
		.pwseqctrl = DT_PROP(DT_INST(n, ilitek_ili9341), pwseqctrl),                       \
		.timctrla = DT_PROP(DT_INST(n, ilitek_ili9341), timctrla),                         \
		.timctrlb = DT_PROP(DT_INST(n, ilitek_ili9341), timctrlb),                         \
		.pumpratioctrl = DT_PROP(DT_INST(n, ilitek_ili9341), pumpratioctrl),               \
		.enable3g = DT_PROP(DT_INST(n, ilitek_ili9341), enable3g),                         \
		.ifctl = DT_PROP(DT_INST(n, ilitek_ili9341), ifctl),                               \
		.etmod = DT_PROP(DT_INST(n, ilitek_ili9341), etmod),                               \
	}

/**
 * @brief Initialize ILI9341 registers with DT values.
 *
 * @param dev ILI9341 device instance
 * @return 0 on success, errno otherwise.
 */
int ili9341_regs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI9341_H_ */
