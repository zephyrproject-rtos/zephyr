/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_IT8XXX2_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_IT8XXX2_CLOCK_CONTROL_H_

#include <zephyr/types.h>
#include "chip_chipregs.h"

/* The IT8XXX2 ECPM Register Base Address */
#define ECPM_IT8XXX2_REG_BASE \
((struct ecpm_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(ecpm)))

/* The IT8XXX2 Clock Control Helper Macro */
#define IT8XXX2_SPI_CONFIG_DT(inst)					\
	{								\
		.offset = DT_INST_CLOCKS_CELL(inst, offset),		\
		.bitmask = DT_INST_CLOCKS_CELL(inst, bitmask),		\
		.always = DT_INST_CLOCKS_CELL(inst, always_on_bitmask),	\
	}

/*
 * The IT8XXX2 Clock Control properties corresponding to the .yaml file
 *
 * offset:  corresponding register offset
 * bitmask: bit masks for configuring the clock gating
 * always:  few registers has this bit which is requied to be set
 */
#ifndef _ASMLANGUAGE
struct it8xxx2_clock_control_cells {
	uint8_t offset;
	uint8_t bitmask;
	uint8_t always;
};
#endif

/*
 * The IT8XXX2 Clock Control Sybsystem Options enumerations
 * for clock_control_get_rate().
 */
enum it8xxx2_clock {
	IT8XXX2_NULL,
	IT8XXX2_PLL,
	IT8XXX2_CPU,
	IT8XXX2_SMB,
	IT8XXX2_COUNT
};

/*
 * The IT8XXX2 Clock Control Sybsystem Types
 */
struct it8xxx2_clkctrl_subsys {
	uint8_t clk_opt;
};

struct it8xxx2_clkctrl_subsys_rate {
	uint32_t clk_rate;
};

struct it8xxx2_clkctrl_freq {
	uint8_t divisor;
	uint32_t frequency;
};

/*
 * The IT8XXX2 PLL adjustment which is executed in the initialization only
 */
void chip_pll_ctrl(enum chip_pll_mode mode);

#endif
