/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LITEX_MMCM_H
#define LITEX_MMCM_H

#include <zephyr/types.h>

/* Common values */
#define PICOS_IN_SEC		1000000000000
#define BITS_PER_BYTE		8

/* MMCM specific numbers */
#define CLKOUT_MAX		7
#define DELAY_TIME_MAX		63
#define PHASE_MUX_MAX		7
#define HIGH_LOW_TIME_REG_MAX	63
#define PHASE_MUX_RES_FACTOR	8

/* DRP registers index */
#define DRP_RESET		0
#define DRP_LOCKED		1
#define DRP_READ		2
#define DRP_WRITE		3
#define DRP_DRDY		4
#define DRP_ADR			5
#define DRP_DAT_W		6
#define DRP_DAT_R		7

/* Base address */
#define DRP_BASE		DT_REG_ADDR_BY_IDX(MMCM, 0)
/* Register address */
#define DRP_ADDR_RESET		DT_REG_ADDR_BY_NAME(MMCM, drp_reset)
#define DRP_ADDR_LOCKED		DT_REG_ADDR_BY_NAME(MMCM, drp_locked)
#define DRP_ADDR_READ		DT_REG_ADDR_BY_NAME(MMCM, drp_read)
#define DRP_ADDR_WRITE		DT_REG_ADDR_BY_NAME(MMCM, drp_write)
#define DRP_ADDR_DRDY		DT_REG_ADDR_BY_NAME(MMCM, drp_drdy)
#define DRP_ADDR_ADR		DT_REG_ADDR_BY_NAME(MMCM, drp_adr)
#define DRP_ADDR_DAT_W		DT_REG_ADDR_BY_NAME(MMCM, drp_dat_w)
#define DRP_ADDR_DAT_R		DT_REG_ADDR_BY_NAME(MMCM, drp_dat_r)

/* Devicetree global defines */
#define LOCK_TIMEOUT		DT_PROP(MMCM, litex_lock_timeout)
#define DRDY_TIMEOUT		DT_PROP(MMCM, litex_drdy_timeout)
#define SYS_CLOCK_FREQUENCY	DT_PROP(MMCM, litex_sys_clock_frequency)
#define DIVCLK_DIVIDE_MIN	DT_PROP(MMCM, litex_divclk_divide_min)
#define DIVCLK_DIVIDE_MAX	DT_PROP(MMCM, litex_divclk_divide_max)
#define CLKFBOUT_MULT_MIN	DT_PROP(MMCM, litex_clkfbout_mult_min)
#define CLKFBOUT_MULT_MAX	DT_PROP(MMCM, litex_clkfbout_mult_max)
#define VCO_FREQ_MIN		DT_PROP(MMCM, litex_vco_freq_min)
#define VCO_FREQ_MAX		DT_PROP(MMCM, litex_vco_freq_max)
#define CLKOUT_DIVIDE_MIN	DT_PROP(MMCM, litex_clkout_divide_min)
#define CLKOUT_DIVIDE_MAX	DT_PROP(MMCM, litex_clkout_divide_max)
#define VCO_MARGIN		DT_PROP(MMCM, litex_vco_margin)

#define CLKOUT_INIT(N)							       \
	BUILD_ASSERT(CLKOUT_DUTY_DEN(N) > 0 &&				       \
		     CLKOUT_DUTY_NUM(N) > 0 &&				       \
		     CLKOUT_DUTY_NUM(N) <= CLKOUT_DUTY_DEN(N),		       \
				     "Invalid default duty");		       \
	BUILD_ASSERT(CLKOUT_ID(N) < NCLKOUT, "Invalid CLKOUT index");	       \
	lcko = &ldev->clkouts[N];					       \
	lcko->id = CLKOUT_ID(N);					       \
									       \
	lcko->clkout_div = clkout_div;					       \
	lcko->def.freq = CLKOUT_FREQ(N);				       \
	lcko->def.phase = CLKOUT_PHASE(N);				       \
	lcko->def.duty.num = CLKOUT_DUTY_NUM(N);			       \
	lcko->def.duty.den = CLKOUT_DUTY_DEN(N);			       \
	lcko->margin.m = CLKOUT_MARGIN(N);				       \
	lcko->margin.exp = CLKOUT_MARGIN_EXP(N);

/* Devicetree clkout defines */
#define CLKOUT_EXIST(N)		DT_NODE_HAS_STATUS(DT_NODELABEL(clk##N), okay)
#define CLKOUT_ID(N)		DT_REG_ADDR(DT_NODELABEL(clk##N))
#define CLKOUT_FREQ(N)		DT_PROP(DT_NODELABEL(clk##N), \
				litex_clock_frequency)
#define CLKOUT_PHASE(N)		DT_PROP(DT_NODELABEL(clk##N), \
				litex_clock_phase)
#define CLKOUT_DUTY_NUM(N)	DT_PROP(DT_NODELABEL(clk##N), \
				litex_clock_duty_num)
#define CLKOUT_DUTY_DEN(N)	DT_PROP(DT_NODELABEL(clk##N), \
				litex_clock_duty_den)
#define CLKOUT_MARGIN(N)	DT_PROP(DT_NODELABEL(clk##N), \
				litex_clock_margin)
#define CLKOUT_MARGIN_EXP(N)	DT_PROP(DT_NODELABEL(clk##N), \
				litex_clock_margin_exp)

/* Register values */
#define FULL_REG_16		0xFFFF
#define ZERO_REG		0x0
#define KEEP_IN_MUL_REG1	0xF000
#define KEEP_IN_MUL_REG2	0xFF3F
#define KEEP_IN_DIV		0xC000
#define REG1_FREQ_MASK		0xF000
#define REG2_FREQ_MASK		0x803F
#define REG1_DUTY_MASK		0xF000
#define REG2_DUTY_MASK		0xFF7F
#define REG1_PHASE_MASK		0x1FFF
#define REG2_PHASE_MASK		0xFCC0
#define FILT1_MASK		0x66FF
#define FILT2_MASK		0x666F
#define LOCK1_MASK		0xFC00
#define LOCK23_MASK		0x8000
/* Control bits extraction masks */
#define HL_TIME_MASK		0x3F
#define FRAC_MASK		0x7
#define EDGE_MASK		0x1
#define NO_CNT_MASK		0x1
#define FRAC_EN_MASK		0x1
#define PHASE_MUX_MASK		0x7

/* Bit groups start position in DRP registers */
#define HIGH_TIME_POS		6
#define LOW_TIME_POS		0
#define PHASE_MUX_POS		13
#define FRAC_POS		12
#define FRAC_EN_POS		11
#define FRAC_WF_R_POS		10
#define EDGE_POS		7
#define NO_CNT_POS		6
#define EDGE_DIVREG_POS		13
#define NO_CNT_DIVREG_POS	12
#define DELAY_TIME_POS		0

/* MMCM Register addresses */
#define POWER_REG		0x28
#define DIV_REG			0x16
#define LOCK_REG1		0x18
#define LOCK_REG2		0x19
#define LOCK_REG3		0x1A
#define FILT_REG1		0x4E
#define FILT_REG2		0x4F
#define CLKOUT0_REG1		0x08
#define CLKOUT0_REG2		0x09
#define CLKOUT1_REG1		0x0A
#define CLKOUT1_REG2		0x0B
#define CLKOUT2_REG1		0x0C
#define CLKOUT2_REG2		0x0D
#define CLKOUT3_REG1		0x0E
#define CLKOUT3_REG2		0x0F
#define CLKOUT4_REG1		0x10
#define CLKOUT4_REG2		0x11
#define CLKOUT5_REG1		0x06
#define CLKOUT5_REG2		0x07
#define CLKOUT6_REG1		0x12
#define CLKOUT6_REG2		0x13
#define CLKFBOUT_REG1		0x14
#define CLKFBOUT_REG2		0x15

/* Basic structure for DRP registers */
struct litex_drp_reg {
	uint32_t addr;
	uint32_t size;
};

struct litex_clk_range {
	uint32_t min;
	uint32_t max;
};

struct clk_duty {
	uint32_t num;
	uint32_t den;
};

struct litex_clk_default {
	struct clk_duty duty;
	int phase;
	uint32_t freq;
};

struct litex_clk_glob_params {
	uint64_t freq;
	uint32_t div;
	uint32_t mul;
};

/* Divider configuration bits group */
struct litex_clk_div_params {
	uint8_t high_time;
	uint8_t low_time;
	uint8_t no_cnt;
	uint8_t edge;
};

/* Phase configuration bits group */
struct litex_clk_phase_params {
	uint8_t phase_mux;
	uint8_t delay_time;
	uint8_t mx;
};

/* Fractional configuration bits group */
struct litex_clk_frac_params {
	uint8_t frac_en;
	uint8_t frac;
	uint8_t phase_mux_f;
	uint8_t frac_wf_r;
	uint8_t frac_wf_f;
};

struct litex_clk_params {
	struct clk_duty duty;
	int phase;
	uint32_t freq;
	uint32_t period_off;
	uint8_t div;
};

struct litex_clk_timeout {
	uint32_t lock;
	uint32_t drdy;
};

/* Basic structure for MMCM reg addresses */
struct litex_clk_clkout_addr {
	uint8_t reg1;
	uint8_t reg2;
};

/* Structure for all MMCM regs */
struct litex_clk_regs_addr {
	struct litex_clk_clkout_addr clkout[CLKOUT_MAX];
};

struct litex_clk_clkout_margin {
	uint32_t m;			/* margin factor scaled to integer */
	uint32_t exp;
};

struct litex_clk_device {
	uint32_t *base;
	/*struct clk_hw clk_hw;*/
	struct litex_clk_clkout *clkouts;	/* array of clock outputs */
	struct litex_clk_timeout timeout;	/* timeouts for wait functions*/
	struct litex_clk_glob_params g_config;	/* general MMCM settings */
	struct litex_clk_glob_params ts_g_config;/* settings to set*/
	struct litex_clk_range divclk;		/* divclk_divide_range */
	struct litex_clk_range clkfbout;	/* clkfbout_mult_frange */
	struct litex_clk_range vco;		/* vco_freq_range */
	uint8_t *update_clkout;			/* which clkout needs update */
	uint32_t sys_clk_freq;			/* input frequency */
	uint32_t vco_margin;
	uint32_t nclkout;
};

struct litex_clk_clkout {
	uint32_t *base;
	struct litex_clk_device *ldev;		/* global data */
	struct litex_clk_default def;		/* DTS defaults */
	struct litex_clk_params config;		/* real CLKOUT settings */
	struct litex_clk_params ts_config;	/* CLKOUT settings to set */
	struct litex_clk_div_params div;	/* CLKOUT configuration groups*/
	struct litex_clk_phase_params phase;
	struct litex_clk_frac_params frac;
	struct litex_clk_range clkout_div;	/* clkout_divide_range */
	struct litex_clk_clkout_margin margin;
	uint32_t id;
};

#endif /* LITEX_MMCM_H */
