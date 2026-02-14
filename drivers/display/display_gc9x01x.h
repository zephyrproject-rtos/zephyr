/**
 * Copyright (c) 2023 Mr Beam Lasers GmbH.
 * Copyright (c) 2023 Amrith Venkat Kesavamoorthi <amrith@mr-beam.org>
 * Copyright (c) 2023 Martin Kiepfer <mrmarteng@teleschirm.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_GC9X01X_H_
#define ZEPHYR_DRIVERS_DISPLAY_GC9X01X_H_

#include <zephyr/sys/util.h>

/* Command registers */
#define GC9X01X_CMD_DFUNCTR   0xB6U /* Display Function Control */
#define GC9X01X_CMD_PWRCTRL1  0xC1U /* Power Control 1 */
#define GC9X01X_CMD_PWRCTRL2  0xC3U /* Power Control 2 */
#define GC9X01X_CMD_PWRCTRL3  0xC4U /* Power Control 3 */
#define GC9X01X_CMD_PWRCTRL4  0xC9U /* Power Control 4 */
#define GC9X01X_CMD_READID1   0xDAU /* Read ID 1 */
#define GC9X01X_CMD_READID2   0xDBU /* Read ID 2 */
#define GC9X01X_CMD_READID3   0xDCU /* Read ID 3 */
#define GC9X01X_CMD_GAMMA1    0xF0U /* Gamma1 (negative polarity) */
#define GC9X01X_CMD_GAMMA2    0xF1U /* Gamma2 */
#define GC9X01X_CMD_GAMMA3    0xF2U /* Gamma3  (positive polarity) */
#define GC9X01X_CMD_GAMMA4    0xF3U /* Gamma4 */
#define GC9X01X_CMD_INREGEN1  0xFEU /* Inter Register Enable 1 */
#define GC9X01X_CMD_INREGEN2  0xEFU /* Inter Register Enable 2 */
#define GC9X01X_CMD_FRAMERATE 0xE8U /* Frame Rate Control */

/* GC9X01X_CMD_MADCTL register fields */
#define GC9X01X_MADCTL_VAL_MY  BIT(7U)
#define GC9X01X_MADCTL_VAL_MX  BIT(6U)
#define GC9X01X_MADCTL_VAL_MV  BIT(5U)
#define GC9X01X_MADCTL_VAL_ML  BIT(4U)
#define GC9X01X_MADCTL_VAL_BGR BIT(3U)
#define GC9X01X_MADCTL_VAL_MH  BIT(2U)

/* GC9X01X_CMD_PIXFMT register fields */
#define GC9X01X_PIXFMT_VAL_RGB_18_BIT 0x60U
#define GC9X01X_PIXFMT_VAL_RGB_16_BIT 0x50U
#define GC9X01X_PIXFMT_VAL_MCU_18_BIT 0x06U
#define GC9X01X_PIXFMT_VAL_MCU_16_BIT 0x05U

/* Duration to enter/exit  sleep mode (see 6.2.3 and 6.4.2 in datasheet) */
#define GC9X01X_SLEEP_IN_OUT_DURATION_MS 120

/* GC9X01X registers to be intitialized */
#define GC9X01X_CMD_PWRCTRL1_LEN  1U
#define GC9X01X_CMD_PWRCTRL2_LEN  1U
#define GC9X01X_CMD_PWRCTRL3_LEN  1U
#define GC9X01X_CMD_PWRCTRL4_LEN  1U
#define GC9X01X_CMD_GAMMA1_LEN    6U
#define GC9X01X_CMD_GAMMA2_LEN    6U
#define GC9X01X_CMD_GAMMA3_LEN    6U
#define GC9X01X_CMD_GAMMA4_LEN    6U
#define GC9X01X_CMD_FRAMERATE_LEN 1U

struct gc9x01x_regs {
	uint8_t pwrctrl1[GC9X01X_CMD_PWRCTRL1_LEN];
	uint8_t pwrctrl2[GC9X01X_CMD_PWRCTRL2_LEN];
	uint8_t pwrctrl3[GC9X01X_CMD_PWRCTRL3_LEN];
	uint8_t pwrctrl4[GC9X01X_CMD_PWRCTRL4_LEN];
	uint8_t gamma1[GC9X01X_CMD_GAMMA1_LEN];
	uint8_t gamma2[GC9X01X_CMD_GAMMA2_LEN];
	uint8_t gamma3[GC9X01X_CMD_GAMMA3_LEN];
	uint8_t gamma4[GC9X01X_CMD_GAMMA4_LEN];
	uint8_t framerate[GC9X01X_CMD_FRAMERATE_LEN];
};

#define GC9X01X_REGS_INIT(inst)                                                                    \
	static const struct gc9x01x_regs gc9x01x_regs_##inst = {                                   \
		.pwrctrl1 = DT_INST_PROP(inst, pwrctrl1),                                          \
		.pwrctrl2 = DT_INST_PROP(inst, pwrctrl2),                                          \
		.pwrctrl3 = DT_INST_PROP(inst, pwrctrl3),                                          \
		.pwrctrl4 = DT_INST_PROP(inst, pwrctrl4),                                          \
		.gamma1 = DT_INST_PROP(inst, gamma1),                                              \
		.gamma2 = DT_INST_PROP(inst, gamma2),                                              \
		.gamma3 = DT_INST_PROP(inst, gamma3),                                              \
		.gamma4 = DT_INST_PROP(inst, gamma4),                                              \
		.framerate = DT_INST_PROP(inst, framerate),                                        \
	};

#endif /* ZEPHYR_DRIVERS_DISPLAY_GC9X01X_H_ */
