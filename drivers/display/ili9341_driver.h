/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 * Copyright (c) 2022 Konstantinos Papadopoulos <kostas.papadopulos@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_ILI9341_DRIVER_H_
#define ZEPHYR_DRIVERS_DISPLAY_ILI9341_DRIVER_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

/* ili9xxx*/
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
#define ILI9XXX_MADCTL 0x36           
#define ILI9XXX_PIXSET 0x3A

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

/* Pixel formats */
#define ILI9XXX_PIXEL_FORMAT_RGB565 0U
#define ILI9XXX_PIXEL_FORMAT_RGB888 1U


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
#define ILI9341_MADCTRL_LEN	1u
#define ILI9341_PIXSET_LEN	1u
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
	uint8_t madctrl[ILI9341_MADCTRL_LEN];
	uint8_t pixset[ILI9341_PIXSET_LEN];
};

/* Initializer macro for ILI9341 registers. */
#define ILI9341_REGS_INIT(inst)                                                                       \
	static const struct ili9341_regs ili9341_regs_##inst = {                                      \
		.gamset = DT_INST_PROP(inst, gamset),                             \
		.ifmode = DT_INST_PROP(inst, ifmode),                             \
		.frmctr1 = DT_INST_PROP(inst, frmctr1),                           \
		.disctrl = DT_INST_PROP(inst, disctrl),                           \
		.pwctrl1 = DT_INST_PROP(inst, pwctrl1),                           \
		.pwctrl2 = DT_INST_PROP(inst, pwctrl2),                           \
		.vmctrl1 = DT_INST_PROP(inst, vmctrl1),                           \
		.vmctrl2 = DT_INST_PROP(inst, vmctrl2),                           \
		.pgamctrl = DT_INST_PROP(inst, pgamctrl),                         \
		.ngamctrl = DT_INST_PROP(inst, ngamctrl),                         \
		.pwctrla = DT_INST_PROP(inst, pwctrla),                           \
		.pwctrlb = DT_INST_PROP(inst, pwctrlb),                           \
		.pwseqctrl = DT_INST_PROP(inst, pwseqctrl),                       \
		.timctrla = DT_INST_PROP(inst, timctrla),                         \
		.timctrlb = DT_INST_PROP(inst, timctrlb),                         \
		.pumpratioctrl = DT_INST_PROP(inst, pumpratioctrl),               \
		.enable3g = DT_INST_PROP(inst, enable3g),                         \
		.ifctl = DT_INST_PROP(inst, ifctl),                               \
		.madctrl = DT_INST_PROP(inst, madctrl),							  \
		.pixset = DT_INST_PROP(inst, pixset),							  \	
		.etmod = DT_INST_PROP(inst, etmod),                               \
	}

/**
 * @brief Initialize ILI9341 registers with DT values.
 *
 * @param dev ILI9341 device instance
 * @return 0 on success, errno otherwise.
 */
int ili9341_regs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_DISPLAY_ILI9341_DRIVER_H_ */
