/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_litex.h>
#include "clock_control_litex.h"
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/kernel.h>

#include <soc.h>

LOG_MODULE_REGISTER(CLK_CTRL_LITEX, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static struct litex_clk_device *ldev;	/* global struct for whole driver */
static struct litex_clk_clkout *clkouts;/* clkout array for whole driver */

/* All DRP regs addresses and sizes */
static const struct litex_drp_reg drp[] = {
	{DRP_ADDR_RESET,  1},
	{DRP_ADDR_LOCKED, 1},
	{DRP_ADDR_READ,   1},
	{DRP_ADDR_WRITE,  1},
	{DRP_ADDR_DRDY,   1},
	{DRP_ADDR_ADR,    1},
	{DRP_ADDR_DAT_W,  2},
	{DRP_ADDR_DAT_R,  2},
};

struct litex_clk_regs_addr litex_clk_regs_addr_init(void)
{
	struct litex_clk_regs_addr m;
	uint32_t i, addr;

	addr = CLKOUT0_REG1;
	for (i = 0; i <= CLKOUT_MAX; i++) {
		if (i == 5) {
		/*
		 *special case because CLKOUT5 have its reg addresses
		 *placed lower than other CLKOUTs
		 */
			m.clkout[5].reg1 = CLKOUT5_REG1;
			m.clkout[5].reg2 = CLKOUT5_REG2;
		} else {
			m.clkout[i].reg1 = addr;
			addr++;
			m.clkout[i].reg2 = addr;
			addr++;
		}
	}
	return m;
}

/*
 * These lookup tables are taken from:
 * https://github.com/Digilent/Zybo-hdmi-out/blob/b991fff6e964420ae3c00c3dbee52f2ad748b3ba/sdk/displaydemo/src/dynclk/dynclk.h
 *
 *	2015 Copyright Digilent Incorporated
 *	Author: Sam Bobrowicz
 *
 */

/* MMCM loop filter lookup table */
static const uint32_t litex_clk_filter_table[] = {
	 0b0001011111,
	 0b0001010111,
	 0b0001111011,
	 0b0001011011,
	 0b0001101011,
	 0b0001110011,
	 0b0001110011,
	 0b0001110011,
	 0b0001110011,
	 0b0001001011,
	 0b0001001011,
	 0b0001001011,
	 0b0010110011,
	 0b0001010011,
	 0b0001010011,
	 0b0001010011,
	 0b0001010011,
	 0b0001010011,
	 0b0001010011,
	 0b0001010011,
	 0b0001010011,
	 0b0001010011,
	 0b0001010011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0001100011,
	 0b0010010011,
	 0b0010010011,
	 0b0010010011,
	 0b0010010011,
	 0b0010010011,
	 0b0010010011,
	 0b0010010011,
	 0b0010010011,
	 0b0010010011,
	 0b0010010011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011,
	 0b0010100011
};

/* MMCM lock detection lookup table */
static const uint64_t litex_clk_lock_table[] = {
	0b0011000110111110100011111010010000000001,
	0b0011000110111110100011111010010000000001,
	0b0100001000111110100011111010010000000001,
	0b0101101011111110100011111010010000000001,
	0b0111001110111110100011111010010000000001,
	0b1000110001111110100011111010010000000001,
	0b1001110011111110100011111010010000000001,
	0b1011010110111110100011111010010000000001,
	0b1100111001111110100011111010010000000001,
	0b1110011100111110100011111010010000000001,
	0b1111111111111000010011111010010000000001,
	0b1111111111110011100111111010010000000001,
	0b1111111111101110111011111010010000000001,
	0b1111111111101011110011111010010000000001,
	0b1111111111101000101011111010010000000001,
	0b1111111111100111000111111010010000000001,
	0b1111111111100011111111111010010000000001,
	0b1111111111100010011011111010010000000001,
	0b1111111111100000110111111010010000000001,
	0b1111111111011111010011111010010000000001,
	0b1111111111011101101111111010010000000001,
	0b1111111111011100001011111010010000000001,
	0b1111111111011010100111111010010000000001,
	0b1111111111011001000011111010010000000001,
	0b1111111111011001000011111010010000000001,
	0b1111111111010111011111111010010000000001,
	0b1111111111010101111011111010010000000001,
	0b1111111111010101111011111010010000000001,
	0b1111111111010100010111111010010000000001,
	0b1111111111010100010111111010010000000001,
	0b1111111111010010110011111010010000000001,
	0b1111111111010010110011111010010000000001,
	0b1111111111010010110011111010010000000001,
	0b1111111111010001001111111010010000000001,
	0b1111111111010001001111111010010000000001,
	0b1111111111010001001111111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001,
	0b1111111111001111101011111010010000000001
};
/* End of copied code */

/* Helper function for filter lookup table */
static inline uint32_t litex_clk_lookup_filter(uint32_t glob_mul)
{
	return litex_clk_filter_table[glob_mul - 1];
}

/* Helper function for lock lookup table */
static inline uint64_t litex_clk_lookup_lock(uint32_t glob_mul)
{
	return litex_clk_lock_table[glob_mul - 1];
}

static inline void litex_clk_set_reg(uint32_t reg, uint32_t val)
{
	litex_write(drp[reg].addr, drp[reg].size, val);
}

static inline uint32_t litex_clk_get_reg(uint32_t reg)
{
	return litex_read(drp[reg].addr, drp[reg].size);
}

static inline void litex_clk_assert_reg(uint32_t reg)
{
	int assert = (1 << (drp[reg].size * BITS_PER_BYTE)) - 1;

	litex_clk_set_reg(reg, assert);
}

static inline void litex_clk_deassert_reg(uint32_t reg)
{
	litex_clk_set_reg(reg, ZERO_REG);
}

static int litex_clk_wait(uint32_t reg)
{
	uint32_t timeout;

	__ASSERT(reg == DRP_LOCKED || reg == DRP_DRDY, "Unsupported register! Please provide DRP_LOCKED or DRP_DRDY");

	if (reg == DRP_LOCKED) {
		timeout = ldev->timeout.lock;
	} else {
		timeout = ldev->timeout.drdy;
	}
	/*Waiting for signal to assert in reg*/
	while (!litex_clk_get_reg(reg) && timeout) {
		timeout--;
		k_sleep(K_MSEC(1));
	}
	if (timeout == 0) {
		LOG_WRN("Timeout occured when waiting for the register: 0x%x", reg);
		return -ETIME;
	}
	return 0;
}

/* Read value written in given internal MMCM register*/
static int litex_clk_get_DO(uint8_t clk_reg_addr, uint16_t *res)
{
	int ret;

	litex_clk_set_reg(DRP_ADR, clk_reg_addr);
	litex_clk_assert_reg(DRP_READ);

	litex_clk_deassert_reg(DRP_READ);
	ret = litex_clk_wait(DRP_DRDY);
	if (ret != 0) {
		return ret;
	}

	*res = litex_clk_get_reg(DRP_DAT_R);

	return 0;
}

/* Get global divider and multiplier values and update global config */
static int litex_clk_update_global_config(void)
{
	int ret;
	uint16_t divreg, mult2;
	uint8_t low_time, high_time;

	ret = litex_clk_get_DO(CLKFBOUT_REG2, &mult2);
	if (ret != 0) {
		return ret;
	}
	ret = litex_clk_get_DO(DIV_REG, &divreg);
	if (ret != 0) {
		return ret;
	}

	if (mult2 & (NO_CNT_MASK << NO_CNT_POS)) {
		ldev->g_config.mul = 1;
	} else {
		uint16_t mult1;

		ret = litex_clk_get_DO(CLKFBOUT_REG1, &mult1);
		if (ret != 0) {
			return ret;
		}
		low_time = mult1 & HL_TIME_MASK;
		high_time = (mult1 >> HIGH_TIME_POS) & HL_TIME_MASK;
		ldev->g_config.mul = low_time + high_time;
	}

	if (divreg & (NO_CNT_MASK << NO_CNT_DIVREG_POS)) {
		ldev->g_config.div = 1;
	} else {
		low_time = divreg & HL_TIME_MASK;
		high_time = (divreg >> HIGH_TIME_POS) & HL_TIME_MASK;
		ldev->g_config.div = low_time + high_time;
	}

	return 0;
}

static uint64_t litex_clk_calc_global_frequency(uint32_t mul, uint32_t div)
{
	uint64_t f;

	f = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC * (uint64_t)mul;
	f /= div;

	return f;
}

/* Calculate frequency with real global params and update global config */
static uint64_t litex_clk_get_real_global_frequency(void)
{
	uint64_t f;

	litex_clk_update_global_config();
	f = litex_clk_calc_global_frequency(ldev->g_config.mul,
					    ldev->g_config.div);
	ldev->g_config.freq = f;
	ldev->ts_g_config.div = ldev->g_config.div;
	ldev->ts_g_config.mul = ldev->g_config.mul;
	ldev->ts_g_config.freq = ldev->g_config.freq;

	return f;
}

/* Return dividers of given CLKOUT */
static int litex_clk_get_clkout_divider(struct litex_clk_clkout *lcko,
					uint32_t *divider, uint32_t *fract_cnt)
{
	struct litex_clk_regs_addr drp_addr = litex_clk_regs_addr_init();
	int ret;
	uint16_t div, frac;
	uint8_t clkout_nr = lcko->id;
	uint8_t low_time, high_time;

	ret = litex_clk_get_DO(drp_addr.clkout[clkout_nr].reg1, &div);
	if (ret != 0) {
		return ret;
	}
	ret = litex_clk_get_DO(drp_addr.clkout[clkout_nr].reg2, &frac);
	if (ret != 0) {
		return ret;
	}

	low_time = div & HL_TIME_MASK;
	high_time = (div >> HIGH_TIME_POS) & HL_TIME_MASK;
	*divider = low_time + high_time;
	*fract_cnt = (frac >> FRAC_POS) & FRAC_MASK;

	return 0;
}

/* Debug functions */
#ifdef CONFIG_CLOCK_CONTROL_LOG_LEVEL_DBG

static void litex_clk_check_DO(char *reg_name, uint8_t clk_reg_addr,
					       uint16_t *res)
{
	int ret;

	ret = litex_clk_get_DO(clk_reg_addr, res);
	if (ret != 0)
		LOG_ERR("%s: read error: %d", reg_name, ret);
	else
		LOG_DBG("%s:  0x%x", reg_name, *res);
}

static void litex_clk_print_general_regs(void)
{
	uint16_t power_reg, div_reg, clkfbout_reg1, clkfbout_reg2,
	lock_reg1, lock_reg2, lock_reg3, filt_reg1, filt_reg2;

	litex_clk_check_DO("POWER_REG", POWER_REG, &power_reg);
	litex_clk_check_DO("DIV_REG", DIV_REG, &div_reg);
	litex_clk_check_DO("MUL_REG1", CLKFBOUT_REG1, &clkfbout_reg1);
	litex_clk_check_DO("MUL_REG2", CLKFBOUT_REG2, &clkfbout_reg2);
	litex_clk_check_DO("LOCK_REG1", LOCK_REG1, &lock_reg1);
	litex_clk_check_DO("LOCK_REG2", LOCK_REG2, &lock_reg2);
	litex_clk_check_DO("LOCK_REG3", LOCK_REG3, &lock_reg3);
	litex_clk_check_DO("FILT_REG1", FILT_REG1, &filt_reg1);
	litex_clk_check_DO("FILT_REG2", FILT_REG2, &filt_reg2);
}

static void litex_clk_print_clkout_regs(uint8_t clkout, uint8_t reg1,
							uint8_t reg2)
{
	uint16_t clkout_reg1, clkout_reg2;
	char reg_name[16];

	sprintf(reg_name, "CLKOUT%u REG1", clkout);
	litex_clk_check_DO(reg_name, reg1, &clkout_reg1);

	sprintf(reg_name, "CLKOUT%u REG2", clkout);
	litex_clk_check_DO(reg_name, reg2, &clkout_reg2);
}

static void litex_clk_print_all_regs(void)
{
	struct litex_clk_regs_addr drp_addr = litex_clk_regs_addr_init();
	uint32_t i;

	litex_clk_print_general_regs();
	for (i = 0; i < ldev->nclkout; i++) {
		litex_clk_print_clkout_regs(i, drp_addr.clkout[i].reg1,
					       drp_addr.clkout[i].reg2);
	}
}

static void litex_clk_print_params(struct litex_clk_clkout *lcko)
{
	LOG_DBG("CLKOUT%d DUMP:", lcko->id);
	LOG_DBG("Defaults:");
	LOG_DBG("f: %u d: %u/%u p: %u",
		lcko->def.freq, lcko->def.duty.num,
		lcko->def.duty.den, lcko->def.phase);
	LOG_DBG("Config to set:");
	LOG_DBG("div: %u freq: %u duty: %u/%u phase: %d per_off: %u",
		lcko->ts_config.div, lcko->ts_config.freq,
		lcko->ts_config.duty.num, lcko->ts_config.duty.den,
		lcko->ts_config.phase, lcko->config.period_off);
	LOG_DBG("Config:");
	LOG_DBG("div: %u freq: %u duty: %u/%u phase: %d per_off: %u",
		lcko->config.div, lcko->config.freq,
		lcko->config.duty.num, lcko->config.duty.den,
		lcko->config.phase, lcko->config.period_off);
	LOG_DBG("Divide group:");
	LOG_DBG("e: %u ht: %u lt: %u nc: %u",
		lcko->div.edge, lcko->div.high_time,
		lcko->div.low_time, lcko->div.no_cnt);
	LOG_DBG("Frac group:");
	LOG_DBG("f: %u fen: %u fwff: %u fwfr: %u pmf: %u",
		lcko->frac.frac, lcko->frac.frac_en, lcko->frac.frac_wf_f,
		lcko->frac.frac_wf_r, lcko->frac.phase_mux_f);
	LOG_DBG("Phase group:");
	LOG_DBG("dt: %u pm: %u mx: %u",
		lcko->phase.delay_time, lcko->phase.phase_mux, lcko->phase.mx);
}

static void litex_clk_print_all_params(void)
{
	uint32_t c;

	LOG_DBG("Global Config to set:");
	LOG_DBG("freq: %llu mul: %u div: %u",
		ldev->ts_g_config.freq, ldev->ts_g_config.mul,
					ldev->ts_g_config.div);
	LOG_DBG("Global Config:");
	LOG_DBG("freq: %llu mul: %u div: %u",
		ldev->g_config.freq, ldev->g_config.mul, ldev->g_config.div);
	for (c = 0; c < ldev->nclkout; c++) {
		litex_clk_print_params(&ldev->clkouts[c]);
	}
}
#endif /* CONFIG_CLOCK_CONTROL_LOG_LEVEL_DBG */

/* Returns raw value ready to be written into MMCM */
static inline uint16_t litex_clk_calc_DI(uint16_t DO_val, uint16_t mask,
							  uint16_t bitset)
{
	uint16_t DI_val;

	DI_val = DO_val & mask;
	DI_val |= bitset;

	return DI_val;
}

/* Sets calculated DI value into DI DRP register */
static int litex_clk_set_DI(uint16_t DI_val)
{
	int ret;

	litex_clk_set_reg(DRP_DAT_W, DI_val);
	litex_clk_assert_reg(DRP_WRITE);
	litex_clk_deassert_reg(DRP_WRITE);
	ret = litex_clk_wait(DRP_DRDY);
	return ret;
}

/*
 * Change register value as specified in arguments
 *
 * mask:		preserve or zero MMCM register bits
 *			by selecting 1 or 0 on desired specific mask positions
 * bitset:		set those bits in MMCM register which are 1 in bitset
 * clk_reg_addr:	internal MMCM address of control register
 *
 */
static int litex_clk_change_value(uint16_t mask, uint16_t bitset,
						 uint8_t clk_reg_addr)
{
	uint16_t DO_val, DI_val;
	int ret;

	litex_clk_assert_reg(DRP_RESET);

	ret = litex_clk_get_DO(clk_reg_addr, &DO_val);
	if (ret != 0) {
		return ret;
	}
	DI_val = litex_clk_calc_DI(DO_val, mask, bitset);
	ret = litex_clk_set_DI(DI_val);
	if (ret != 0) {
		return ret;
	}
#ifdef CONFIG_CLOCK_CONTROL_LOG_LEVEL_DBG
	DI_val = litex_clk_get_reg(DRP_DAT_W);
	LOG_DBG("set 0x%x under: 0x%x", DI_val, clk_reg_addr);
#endif
	litex_clk_deassert_reg(DRP_DAT_W);
	litex_clk_deassert_reg(DRP_RESET);
	ret = litex_clk_wait(DRP_LOCKED);
	return ret;
}

/*
 * Set register values for given CLKOUT
 *
 * clkout_nr:		clock output number
 * mask_regX:		preserve or zero MMCM register X bits
 *			by selecting 1 or 0 on desired specific mask positions
 * bitset_regX:		set those bits in MMCM register X which are 1 in bitset
 *
 */
static int litex_clk_set_clock(uint8_t clkout_nr, uint16_t mask_reg1,
			      uint16_t bitset_reg1, uint16_t mask_reg2,
						 uint16_t bitset_reg2)
{
	struct litex_clk_regs_addr drp_addr = litex_clk_regs_addr_init();
	int ret;

	if (!(mask_reg2 == FULL_REG_16 && bitset_reg2 == ZERO_REG)) {
		ret = litex_clk_change_value(mask_reg2, bitset_reg2,
					     drp_addr.clkout[clkout_nr].reg2);
		if (ret != 0) {
			return ret;
		}
	}
	if (!(mask_reg1 == FULL_REG_16 && bitset_reg1 == ZERO_REG)) {
		ret = litex_clk_change_value(mask_reg1, bitset_reg1,
					     drp_addr.clkout[clkout_nr].reg1);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/* Set global divider for all CLKOUTs */
static int litex_clk_set_divreg(void)
{
	int ret;
	uint8_t no_cnt = 0, edge = 0, ht = 0, lt = 0,
		div = ldev->ts_g_config.div;
	uint16_t bitset = 0;

	if (div == 1) {
		no_cnt = 1;
	} else {
		ht = div / 2;
		lt = ht;
		edge = div % 2;
		if (edge) {
			lt += edge;
		}
	}

	bitset = (edge << EDGE_DIVREG_POS)	|
		 (no_cnt << NO_CNT_DIVREG_POS)	|
		 (ht << HIGH_TIME_POS)		|
		 (lt << LOW_TIME_POS);

	ret = litex_clk_change_value(KEEP_IN_DIV, bitset, DIV_REG);
	if (ret != 0) {
		return ret;
	}

	ldev->g_config.div = div;
	LOG_DBG("Global divider set to %u", div);

	return 0;
}

/* Set global multiplier for all CLKOUTs */
static int litex_clk_set_mulreg(void)
{
	int ret;
	uint8_t no_cnt = 0, edge = 0, ht = 0, lt = 0,
		mul = ldev->ts_g_config.mul;
	uint16_t bitset1 = 0;

	if (mul == 1) {
		no_cnt = 1;
	} else {
		ht = mul / 2;
		lt = ht;
		edge = mul % 2;
		if (edge) {
			lt += edge;
		}
	}

	bitset1 = (ht << HIGH_TIME_POS) |
		  (lt << LOW_TIME_POS);

	ret = litex_clk_change_value(KEEP_IN_MUL_REG1, bitset1, CLKFBOUT_REG1);
	if (ret != 0) {
		return ret;
	}

	if (edge || no_cnt) {
		uint16_t bitset2 = (edge << EDGE_POS)	|
				(no_cnt << NO_CNT_POS);

		ret = litex_clk_change_value(KEEP_IN_MUL_REG2,
				       bitset2, CLKFBOUT_REG2);
		if (ret != 0) {
			return ret;
		}
	}

	ldev->g_config.mul = mul;
	LOG_DBG("Global multiplier set to %u", mul);

	return 0;
}

static int litex_clk_set_filt(void)
{
	uint16_t filt_reg;
	uint32_t filt, mul;
	int ret;

	mul = ldev->g_config.mul;
	filt = litex_clk_lookup_filter(mul);

	/*
	 * Preparing and setting filter register values
	 * according to reg map form Xilinx XAPP888
	 */
	filt_reg = (((filt >> 9) & 0x1) << 15) |
		   (((filt >> 7) & 0x3) << 11) |
		   (((filt >> 6) & 0x1) << 8);
	ret = litex_clk_change_value(FILT1_MASK, filt_reg, FILT_REG1);
	if (ret != 0)  {
		return ret;
	}

	filt_reg = (((filt >> 5) & 0x1) << 15) |
		   (((filt >> 3) & 0x3) << 11) |
		   (((filt >> 1) & 0x3) << 7) |
		   (((filt) & 0x1) << 4);
	ret = litex_clk_change_value(FILT2_MASK, filt_reg, FILT_REG2);

	return ret;
}

static int litex_clk_set_lock(void)
{
	uint16_t lock_reg;
	uint32_t mul;
	uint64_t lock;
	int ret;

	mul = ldev->g_config.mul;
	lock = litex_clk_lookup_lock(mul);

	/*
	 * Preparing and setting lock register values
	 * according to reg map form Xilinx XAPP888
	 */
	lock_reg = (lock >> 20) & 0x3FF;
	ret = litex_clk_change_value(LOCK1_MASK, lock_reg, LOCK_REG1);
	if (ret != 0) {
		return ret;
	}

	lock_reg = (((lock >> 30) & 0x1F) << 10) |
		     (lock & 0x3FF);
	ret = litex_clk_change_value(LOCK23_MASK, lock_reg, LOCK_REG2);
	if (ret != 0) {
		return ret;
	}

	lock_reg = (((lock >> 35) & 0x1F) << 10) |
		   ((lock >> 10) & 0x3FF);
	ret = litex_clk_change_value(LOCK23_MASK, lock_reg, LOCK_REG3);

	return ret;
}

/* Set all multiplier-related regs: mul, filt and lock regs */
static int litex_clk_set_mul(void)
{
	int ret;

	ret = litex_clk_set_mulreg();
	if (ret != 0) {
		return ret;
	}
	ret = litex_clk_set_filt();
	if (ret != 0) {
		return ret;
	}
	ret = litex_clk_set_lock();
	return ret;
}

static int litex_clk_set_both_globs(void)
{
	/*
	 * we need to check what change first to prevent
	 * getting our VCO_FREQ out of possible range
	 */
	uint64_t vco_freq;
	int ret;

	/* div-first case */
	vco_freq = litex_clk_calc_global_frequency(
					ldev->g_config.mul,
					ldev->ts_g_config.div);
	if (vco_freq > ldev->vco.max || vco_freq < ldev->vco.min) {
		/* div-first not safe */
		vco_freq = litex_clk_calc_global_frequency(
					ldev->ts_g_config.mul,
					ldev->g_config.div);
		if (vco_freq > ldev->vco.max || vco_freq < ldev->vco.min) {
			/* mul-first not safe */
			ret = litex_clk_set_divreg();
			/* Ignore timeout because we expect that to happen */
			if (ret != -ETIME && ret != 0) {
				return ret;
			} else if (ret == -ETIME) {
				ldev->g_config.div = ldev->ts_g_config.div;
				LOG_DBG("Global divider set to %u",
					 ldev->g_config.div);
			}
			ret = litex_clk_set_mul();
			if (ret != 0) {
				return ret;
			}
		} else {
			/* mul-first safe */
			ret = litex_clk_set_mul();
			if (ret != 0) {
				return ret;
			}
			ret = litex_clk_set_divreg();
			if (ret != 0) {
				return ret;
			}
		}
	} else {
		/* div-first safe */
		ret = litex_clk_set_divreg();
		if (ret != 0) {
			return ret;
		}
		ret = litex_clk_set_mul();
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

/* Set global divider, multiplier, filt and lock values */
static int litex_clk_set_globs(void)
{
	int ret;
	uint8_t set_div = 0,
	   set_mul = 0;

	set_div = ldev->ts_g_config.div != ldev->g_config.div;
	set_mul = ldev->ts_g_config.mul != ldev->g_config.mul;

	if (set_div || set_mul) {
		if (set_div && set_mul) {
			ret = litex_clk_set_both_globs();
			if (ret != 0) {
				return ret;
			}
		} else if (set_div) {
			/* set divider only */
			ret = litex_clk_set_divreg();
			if (ret != 0) {
				return ret;
			}
		} else {
			/* set multiplier only */
			ret = litex_clk_set_mul();
			if (ret != 0) {
				return ret;
			}
		}
		ldev->g_config.freq = ldev->ts_g_config.freq;
	}
	return 0;
}

/* Round scaled value*/
static inline uint32_t litex_round(uint32_t val, uint32_t mod)
{
	if (val % mod > mod / 2) {
		return val / mod + 1;
	}
	return val / mod;
}

/*
 *	Duty Cycle
 */

/* Returns accurate duty ratio of given clkout*/
int litex_clk_get_duty_cycle(struct litex_clk_clkout *lcko,
			     struct clk_duty *duty)
{
	struct litex_clk_regs_addr drp_addr = litex_clk_regs_addr_init();
	int ret;
	uint32_t divider;
	uint16_t clkout_reg1, clkout_reg2;
	uint8_t clkout_nr, high_time, edge, no_cnt, frac_en, frac_cnt;

	clkout_nr = lcko->id;
	/* Check if divider is off */
	ret = litex_clk_get_DO(drp_addr.clkout[clkout_nr].reg2, &clkout_reg2);
	if (ret != 0) {
		return ret;
	}

	edge = (clkout_reg2 >> EDGE_POS) & EDGE_MASK;
	no_cnt = (clkout_reg2 >> NO_CNT_POS) & NO_CNT_MASK;
	frac_en = (clkout_reg2 >> FRAC_EN_POS) & FRAC_EN_MASK;
	frac_cnt = (clkout_reg2 >> FRAC_POS) & FRAC_MASK;

	/* get duty 50% when divider is off or fractional is enabled */
	if (no_cnt || (frac_en && frac_cnt)) {
		duty->num = 1;
		duty->den = 2;
		return 0;
	}

	ret = litex_clk_get_DO(drp_addr.clkout[clkout_nr].reg1, &clkout_reg1);
	if (ret != 0) {
		return ret;
	}

	divider = clkout_reg1 & HL_TIME_MASK;
	high_time = (clkout_reg1 >> HIGH_TIME_POS) & HL_TIME_MASK;
	divider += high_time;

	/* Scaling to consider edge control bit */
	duty->num = high_time * 10 + edge * 5;
	duty->den = (divider + edge) * 10;

	return 0;
}

/* Calculates duty cycle for given ratio in percent, 1% accuracy */
static inline uint8_t litex_clk_calc_duty_percent(struct clk_duty *duty)
{
	uint32_t div, duty_ratio, ht;

	ht = duty->num;
	div = duty->den;
	duty_ratio = ht * 10000 / div;

	return (uint8_t)litex_round(duty_ratio, 100);
}

/* Calculate necessary values for setting duty cycle in normal mode */
static int litex_clk_calc_duty_normal(struct litex_clk_clkout *lcko,
					 int calc_new)
{
	struct clk_duty duty;
	int delta_d;
	uint32_t ht_aprox, synth_duty, min_d;
	uint8_t high_time_it, edge_it, high_duty,
	   divider = lcko->config.div;
	int err;

	if (calc_new) {
		duty = lcko->ts_config.duty;
	} else {
		err = litex_clk_get_duty_cycle(lcko, &duty);
		if (err != 0) {
			return err;
		}
	}

	high_duty = litex_clk_calc_duty_percent(&duty);
	min_d = INT_MAX;
	/* check if duty is available to set */
	ht_aprox = high_duty * divider;

	if (ht_aprox > ((HIGH_LOW_TIME_REG_MAX * 100) + 50) ||
		       ((HIGH_LOW_TIME_REG_MAX * 100) + 50) <
			(divider * 100) - ht_aprox) {
		return -EINVAL;
	}

	/* to prevent high_time == 0 or low_time == 0 */
	for (high_time_it = 1; high_time_it < divider; high_time_it++) {
		for (edge_it = 0; edge_it < 2; edge_it++) {
			synth_duty = (high_time_it * 100 + 50 * edge_it) /
								  divider;
			delta_d = synth_duty - high_duty;
			delta_d = abs(delta_d);
			/* check if low_time won't be above acceptable range */
			if (delta_d < min_d && (divider - high_time_it) <=
						  HIGH_LOW_TIME_REG_MAX) {
				min_d = delta_d;
				lcko->div.high_time = high_time_it;
				lcko->div.low_time = divider - high_time_it;
				lcko->div.edge = edge_it;
				lcko->config.duty.num = high_time_it * 100 + 50
								     * edge_it;
				lcko->config.duty.den = divider * 100;
			}
		}
	}
	/*
	 * Calculating values in normal mode,
	 * clear control bits of fractional part
	 */
	lcko->frac.frac_wf_f = 0;
	lcko->frac.frac_wf_r = 0;

	return 0;
}

/* Calculates duty high_time for given divider and ratio */
static inline int litex_clk_calc_duty_high_time(struct clk_duty *duty,
						   uint32_t divider)
{
	uint32_t high_duty;

	high_duty = litex_clk_calc_duty_percent(duty) * divider;

	return litex_round(high_duty, 100);
}

/* Set duty cycle with given ratio */
static int litex_clk_set_duty_cycle(struct litex_clk_clkout *lcko,
			     struct clk_duty *duty)
{
	int ret;
	uint16_t bitset1, bitset2;
	uint8_t clkout_nr = lcko->id,
	   *edge = &lcko->div.edge,
	   *high_time = &lcko->div.high_time,
	    high_duty = litex_clk_calc_duty_percent(duty),
	   *low_time = &lcko->div.low_time;

	if (lcko->frac.frac == 0) {
		lcko->ts_config.duty = *duty;
		LOG_DBG("CLKOUT%d: setting duty: %u/%u",
			lcko->id, duty->num, duty->den);
		ret = litex_clk_calc_duty_normal(lcko, true);
		if (ret != 0) {
			LOG_ERR("CLKOUT%d: cannot set %d%% duty cycle",
				clkout_nr, high_duty);
			return ret;
		}
	} else {
		LOG_ERR("CLKOUT%d: cannot set duty cycle when fractional divider enabled",
								     clkout_nr);
		return -EACCES;
	}

	bitset1 = (*high_time << HIGH_TIME_POS) |
		  (*low_time << LOW_TIME_POS);
	bitset2 = (*edge << EDGE_POS);

	LOG_DBG("SET DUTY CYCLE: e:%u ht:%u lt:%u\nbitset1: 0x%x bitset2: 0x%x",
		*edge, *high_time, *low_time, bitset1, bitset2);

	ret = litex_clk_set_clock(clkout_nr, REG1_DUTY_MASK, bitset1,
					     REG2_DUTY_MASK, bitset2);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("CLKOUT%d: set duty: %d%%", lcko->id,
		litex_clk_calc_duty_percent(&lcko->config.duty));
	return 0;
}

/*
 *	Phase
 */

/* Calculate necessary values for setting phase in normal mode */
static int litex_clk_calc_phase_normal(struct litex_clk_clkout *lcko)
{
	uint64_t period_buff;
	uint32_t post_glob_div_f, global_period,  clkout_period,
	    *period_off = &lcko->ts_config.period_off;
	uint8_t divider = lcko->config.div;
	/* ps unit */

	post_glob_div_f = (uint32_t)litex_clk_get_real_global_frequency();
	period_buff = PICOS_IN_SEC;
	period_buff /= post_glob_div_f;
	global_period = (uint32_t)period_buff;
	clkout_period = global_period * divider;

	if (lcko->ts_config.phase != 0) {
		int synth_phase, delta_p, min_p, p_o;
		uint8_t delay, p_m;

		*period_off = litex_round(clkout_period * (*period_off), 10000);

		if (*period_off / global_period > DELAY_TIME_MAX) {
			return -EINVAL;
		}

		min_p = INT_MAX;
		p_o = *period_off;
		/* Delay_time: (0-63) */
		for (delay = 0; delay <= DELAY_TIME_MAX; delay++) {
			/* phase_mux: (0-7) */
			for (p_m = 0; p_m <= PHASE_MUX_MAX; p_m++) {
				synth_phase = (delay * global_period) +
				((p_m * ((global_period * 100) / 8) / 100));

				delta_p = synth_phase - p_o;
				delta_p = abs(delta_p);
				if (delta_p < min_p) {
					min_p = delta_p;
					lcko->phase.phase_mux = p_m;
					lcko->phase.delay_time = delay;
					lcko->config.period_off = synth_phase;
				}
			}
		}
	} else {
		/* Don't change phase offset*/
		lcko->phase.phase_mux = 0;
		lcko->phase.delay_time = 0;
	}
	/*
	 * Calculating values in normal mode,
	 * fractional control bits need to be zero
	 */
	lcko->frac.phase_mux_f = 0;

	return 0;
}

/* Convert phase offset to positive lower than 360 deg. and calculate period */
static int litex_clk_prepare_phase(struct litex_clk_clkout *lcko)
{
	int *phase = &lcko->ts_config.phase;

	*phase %= 360;

	if (*phase < 0) {
		*phase += 360;
	}

	lcko->ts_config.period_off = ((*phase * 10000) / 360);

	return 0;
}

/* Calculate necessary values for setting phase */
static int litex_clk_calc_phase(struct litex_clk_clkout *lcko)
{
	litex_clk_prepare_phase(lcko);

	return litex_clk_calc_phase_normal(lcko);
}

/* Returns phase-specific values of given clock output */
static int litex_clk_get_phase_data(struct litex_clk_clkout *lcko,
				    uint8_t *phase_mux, uint8_t *delay_time)
{
	struct litex_clk_regs_addr drp_addr = litex_clk_regs_addr_init();
	int ret;
	uint16_t r1, r2;
	uint8_t clkout_nr = lcko->id;

	ret = litex_clk_get_DO(drp_addr.clkout[clkout_nr].reg1, &r1);
	if (ret != 0) {
		return ret;
	}
	ret = litex_clk_get_DO(drp_addr.clkout[clkout_nr].reg2, &r2);
	if (ret != 0) {
		return ret;
	}

	*phase_mux = (r1 >> PHASE_MUX_POS) & PHASE_MUX_MASK;
	*delay_time = (r2 >> DELAY_TIME_POS) & HL_TIME_MASK;

	return 0;
}

/* Returns phase of given clock output in time offset */
int litex_clk_get_phase(struct litex_clk_clkout *lcko)
{
	uint64_t period_buff;
	uint32_t divider = 0, fract_cnt, post_glob_div_f,
	    pm, global_period, clkout_period, period;
	uint8_t phase_mux = 0, delay_time = 0;
	int err = 0;

	litex_clk_get_phase_data(lcko, &phase_mux, &delay_time);
	err = litex_clk_get_clkout_divider(lcko, &divider, &fract_cnt);
	if (err != 0) {
		return err;
	}

	post_glob_div_f = (uint32_t)litex_clk_get_real_global_frequency();
	period_buff = PICOS_IN_SEC;
	period_buff /= post_glob_div_f;
	/* ps unit */
	global_period = (uint32_t)period_buff;
	clkout_period = global_period * divider;

	pm = (phase_mux * global_period * 1000) / PHASE_MUX_RES_FACTOR;
	pm = litex_round(pm, 1000);

	period = delay_time * global_period + pm;

	period = period * 1000 / clkout_period;
	period = period * 360;

	return litex_round(period, 1000);
}

/* Returns phase of given clock output in degrees */
int litex_clk_get_phase_deg(struct litex_clk_clkout *lcko)
{
	uint64_t post_glob_div_f, buff, clkout_period;

	post_glob_div_f = (uint32_t)litex_clk_get_real_global_frequency();
	buff = PICOS_IN_SEC;
	buff /= post_glob_div_f;
	clkout_period = (uint32_t)buff;
	clkout_period *= lcko->config.div;

	buff = lcko->config.period_off * 1000 / clkout_period;
	buff *= 360;
	buff = litex_round(buff, 1000);

	return (int)buff;
}
/* Sets phase given in degrees on given clock output */
int litex_clk_set_phase(struct litex_clk_clkout *lcko, int degrees)
{
	int ret;
	uint16_t bitset1, bitset2, reg2_mask;
	uint8_t *phase_mux = &lcko->phase.phase_mux,
	   *delay_time = &lcko->phase.delay_time,
	   clkout_nr = lcko->id;

	lcko->ts_config.phase = degrees;
	reg2_mask = REG2_PHASE_MASK;
	LOG_DBG("CLKOUT%d: setting phase: %u deg", lcko->id, degrees);

	ret = litex_clk_calc_phase(lcko);
	if (ret != 0) {
		LOG_ERR("CLKOUT%d: phase offset %d deg is too high",
			 clkout_nr, degrees);
		return ret;
	}

	bitset1 = (*phase_mux << PHASE_MUX_POS);
	bitset2 = (*delay_time << DELAY_TIME_POS);

	ret = litex_clk_set_clock(clkout_nr, REG1_PHASE_MASK, bitset1,
						   reg2_mask, bitset2);
	if (ret != 0) {
		return ret;
	}
	lcko->config.phase = litex_clk_get_phase_deg(lcko);
	LOG_INF("CLKOUT%d: set phase: %d deg", lcko->id, lcko->config.phase);
	LOG_DBG("SET PHASE: pm:%u dt:%u\nbitset1: 0x%x bitset2: 0x%x",
		*phase_mux, *delay_time, bitset1, bitset2);

	return 0;
}

/*
 *	Frequency
 */

/* Returns rate in Hz */
static inline uint32_t litex_clk_calc_rate(struct litex_clk_clkout *lcko)
{
	uint64_t f = litex_clk_calc_global_frequency(ldev->ts_g_config.mul,
						  ldev->ts_g_config.div);

	f /= lcko->config.div;

	return (uint32_t)f;
}

/*
 * Written since there is no pow() in math.h. Only for exponent
 * and base above 0. Used for calculating scaling factor for
 * frequency margin
 *
 */
static uint32_t litex_clk_pow(uint32_t base, uint32_t exp)
{
	int ret = 1;

	while (exp--) {
		ret *= base;
	}

	return ret;
}

/* Returns true when possible to set frequency with given global settings */
static int litex_clk_calc_clkout_params(struct litex_clk_clkout *lcko,
					 uint64_t vco_freq)
{
	int delta_f;
	uint64_t m, clk_freq = 0;
	uint32_t d, margin = 1;

	if (lcko->margin.exp) {
		margin = litex_clk_pow(10, lcko->margin.exp);
	}

	lcko->div.no_cnt = 0;

	for (d = lcko->clkout_div.min; d <= lcko->clkout_div.max; d++) {
		clk_freq = vco_freq;
		clk_freq /= d;
		m = lcko->ts_config.freq * lcko->margin.m;
		/* Scale margin according to its exponent */
		if (lcko->margin.exp) {
			m /= margin;
		}

		delta_f = clk_freq - lcko->ts_config.freq;
		delta_f = abs(delta_f);
		if (delta_f <= m) {
			lcko->config.freq = (uint32_t)clk_freq;
			if (lcko->config.div != d) {
				ldev->update_clkout[lcko->id] = 1;
			}
			lcko->config.div = d;
			/* for sake of completeness */
			lcko->ts_config.div = d;
			/* we are not using fractional divider */
			lcko->frac.frac_en = 0;
			lcko->frac.frac = 0;
			if (d == 1) {
				lcko->div.no_cnt = 1;
			}
			LOG_DBG("CLKOUT%d: freq:%u div:%u gdiv:%u gmul:%u",
				 lcko->id, lcko->config.freq, lcko->config.div,
				 ldev->ts_g_config.div, ldev->ts_g_config.mul);
			return true;
		}
	}

	return false;
}

/* Compute dividers for all active clock outputs */
static int litex_clk_calc_all_clkout_params(uint64_t vco_freq)
{
	struct litex_clk_clkout *lcko;
	uint32_t c;

	for (c = 0; c < ldev->nclkout; c++) {
		lcko = &ldev->clkouts[c];
		if (!litex_clk_calc_clkout_params(lcko, vco_freq)) {
			return false;
		}
	}
	return true;
}

/* Calculate parameters for whole active part of MMCM */
static int litex_clk_calc_all_params(void)
{
	uint32_t div, mul;
	uint64_t vco_freq = 0;

	for (div = ldev->divclk.min; div <= ldev->divclk.max; div++) {
		ldev->ts_g_config.div = div;
		for (mul = ldev->clkfbout.max; mul >= ldev->clkfbout.min;
								 mul--) {
			int below, above, all_valid = true;

			vco_freq = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC * (uint64_t)mul;
			vco_freq /= div;
			below = vco_freq < (ldev->vco.min
					     * (1 + ldev->vco_margin));
			above = vco_freq > (ldev->vco.max
					    * (1 - ldev->vco_margin));

			if (!below && !above) {
				all_valid = litex_clk_calc_all_clkout_params
								     (vco_freq);
				if (all_valid) {
					ldev->ts_g_config.mul = mul;
					ldev->ts_g_config.freq = vco_freq;
					LOG_DBG("GLOBAL: freq:%llu g_div:%u g_mul:%u",
						ldev->ts_g_config.freq,
						ldev->ts_g_config.div,
						ldev->ts_g_config.mul);
					return 0;
				}
			}
		}
	}
	LOG_ERR("Cannot find correct settings for all clock outputs!");
	return -ENOTSUP;
}

int litex_clk_check_rate_range(struct litex_clk_clkout *lcko, uint32_t rate)
{
	uint64_t max, min, m;
	uint32_t div, margin;

	m = rate * lcko->margin.m;
	if (lcko->margin.exp) {
		margin = litex_clk_pow(10, lcko->margin.exp);
	}

	max = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC * (uint64_t)ldev->clkfbout.max;
	div = ldev->divclk.min * lcko->clkout_div.min;
	max /= div;
	max += m;

	min = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC * ldev->clkfbout.min;
	div = ldev->divclk.max * lcko->clkout_div.max;
	min /= div;

	if (min < m) {
		min = 0;
	} else {
		min -= m;
	}

	if ((uint64_t)rate < min || (uint64_t)rate > max) {
		return -EINVAL;
	}
	return 0;
}

/* Returns closest available clock rate in Hz */
long litex_clk_round_rate(struct litex_clk_clkout *lcko, unsigned long rate)
{
	int ret;

	ret = litex_clk_check_rate_range(lcko, rate);
	if (ret != 0) {
		return -EINVAL;
	}

	lcko->ts_config.freq = rate;

	ret = litex_clk_calc_all_params();
	if (ret != 0) {
		return ret;
	}

	return litex_clk_calc_rate(lcko);
}

int litex_clk_write_rate(struct litex_clk_clkout *lcko)
{
	int ret;
	uint16_t bitset1, bitset2;
	uint8_t *divider = &lcko->config.div,
	   *edge = &lcko->div.edge,
	   *high_time = &lcko->div.high_time,
	   *low_time = &lcko->div.low_time,
	   *no_cnt = &lcko->div.no_cnt,
	   *frac = &lcko->frac.frac,
	   *frac_en = &lcko->frac.frac_en,
	   *frac_wf_r = &lcko->frac.frac_wf_r;

	bitset1 = (*high_time << HIGH_TIME_POS)	|
		  (*low_time << LOW_TIME_POS);

	bitset2 = (*frac << FRAC_POS)		|
		  (*frac_en << FRAC_EN_POS)	|
		  (*frac_wf_r << FRAC_WF_R_POS)	|
		  (*edge << EDGE_POS)		|
		  (*no_cnt << NO_CNT_POS);

	LOG_DBG("SET RATE: div:%u f:%u fwfr:%u fen:%u nc:%u e:%u ht:%u lt:%u\nbitset1: 0x%x bitset2: 0x%x",
		*divider, *frac, *frac_wf_r, *frac_en,
		*no_cnt, *edge, *high_time, *low_time, bitset1, bitset2);

	ret = litex_clk_set_clock(lcko->id, REG1_FREQ_MASK, bitset1,
					    REG2_FREQ_MASK, bitset2);
	if (ret != 0) {
		return ret;
	}

	ldev->update_clkout[lcko->id] = 0;

	return 0;
}

int litex_clk_update_clkouts(void)
{
	struct litex_clk_clkout *lcko;
	int ret;
	uint8_t c;

	for (c = 0; c < ldev->nclkout; c++) {
		if (ldev->update_clkout[c]) {
			lcko = &ldev->clkouts[c];
			ret = litex_clk_calc_duty_normal(lcko, false);
			if (ret != 0) {
				return ret;
			}
			ret = litex_clk_write_rate(lcko);
			if (ret != 0) {
				return ret;
			}
			LOG_INF("CLKOUT%d: updated rate: %u to %u HZ",
					lcko->id, lcko->ts_config.freq,
						     lcko->config.freq);
		}
	}

	return 0;
}
/* Set closest available clock rate in Hz, parent_rate ignored */
int litex_clk_set_rate(struct litex_clk_clkout *lcko, unsigned long rate)
{
	int ret;

	LOG_DBG("CLKOUT%d: setting rate: %lu", lcko->id, rate);
	ret = litex_clk_round_rate(lcko, rate);
	if (ret < 0) {
		return ret;
	}
	ret = litex_clk_set_globs();
	if (ret != 0) {
		return ret;
	}
	ret = litex_clk_calc_duty_normal(lcko, false);
	if (ret != 0) {
		return ret;
	}
	ret = litex_clk_write_rate(lcko);
	if (ret != 0) {
		return ret;
	}
	LOG_INF("CLKOUT%d: set rate: %u HZ", lcko->id, lcko->config.freq);
	ret = litex_clk_update_clkouts();
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_CLOCK_CONTROL_LOG_LEVEL_DBG
	litex_clk_print_all_params();
	litex_clk_print_all_regs();
#endif /* CONFIG_CLOCK_CONTROL_LOG_LEVEL_DBG */

	return 0;
}

/* Set default clock value from device tree for given clkout*/
static int litex_clk_set_def_clkout(int clkout_nr)
{
	struct litex_clk_clkout *lcko = &ldev->clkouts[clkout_nr];
	int ret;

	ret = litex_clk_set_rate(lcko, lcko->def.freq);
	if (ret != 0) {
		return ret;
	}
	ret = litex_clk_set_duty_cycle(lcko, &lcko->def.duty);
	if (ret != 0) {
		return ret;
	}
	return litex_clk_set_phase(lcko, lcko->def.phase);
}

static int litex_clk_set_all_def_clkouts(void)
{
	int c, ret;

	for (c = 0; c < ldev->nclkout; c++) {
		ret = litex_clk_set_def_clkout(c);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

/*
 * Returns parameters of given clock output
 *
 * clock:		device structure for driver
 * sub_system:		pointer to struct litex_clk_clkout
 *			casted to clock_control_subsys with
 *			all clkout parameters
 */
static int litex_clk_get_subsys_rate(const struct device *clock,
				     clock_control_subsys_t sys, uint32_t *rate)
{
	struct litex_clk_setup *setup = sys;
	struct litex_clk_clkout *lcko;

	lcko = &ldev->clkouts[setup->clkout_nr];
	*rate = litex_clk_calc_rate(lcko);

	return 0;
}

static enum clock_control_status litex_clk_get_status(const struct device *dev,
						     clock_control_subsys_t sys)
{
	struct litex_clk_setup *setup = sys;
	struct clk_duty duty;
	struct litex_clk_clkout *lcko;
	int ret;

	lcko = &ldev->clkouts[setup->clkout_nr];

	setup->rate = litex_clk_calc_rate(lcko);
	ret = litex_clk_get_duty_cycle(lcko, &duty);
	if (ret != 0) {
		return ret;
	}
	setup->duty = litex_clk_calc_duty_percent(&duty);
	setup->phase = litex_clk_get_phase(lcko);

	return CLOCK_CONTROL_STATUS_ON;
}

static inline int litex_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct litex_clk_setup *setup = sys;
	struct clk_duty duty;
	struct litex_clk_clkout *lcko;
	uint8_t duty_perc;
	int ret;

	lcko = &ldev->clkouts[setup->clkout_nr];

	if (lcko->config.freq != setup->rate) {
		ret = litex_clk_set_rate(lcko, setup->rate);
		if (ret != 0) {
			return ret;
		}
	}
	if (lcko->config.phase != setup->phase) {
		ret = litex_clk_set_phase(lcko, setup->phase);
		if (ret != 0) {
			return ret;
		}
	}
	duty_perc = litex_clk_calc_duty_percent(&lcko->config.duty);
	if (duty_perc != setup->duty) {
		duty.num = setup->duty;
		duty.den = 100;
		ret = litex_clk_set_duty_cycle(lcko, &duty);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

static inline int litex_clk_off(const struct device *dev,
				clock_control_subsys_t sub_system)
{
	return litex_clk_change_value(ZERO_REG, ZERO_REG, POWER_REG);
}

static const struct clock_control_driver_api litex_clk_api = {
	.on = litex_clk_on,
	.off = litex_clk_off,
	.get_rate = litex_clk_get_subsys_rate,
	.get_status = litex_clk_get_status
};

static void litex_clk_dts_clkout_ranges_read(struct litex_clk_range *clkout_div)
{
	clkout_div->min = CLKOUT_DIVIDE_MIN;
	clkout_div->max = CLKOUT_DIVIDE_MAX;
}

static int litex_clk_dts_timeout_read(struct litex_clk_timeout *timeout)
{
	/* Read wait_lock timeout from device property*/
	timeout->lock = LOCK_TIMEOUT;
	if (timeout->lock < 1) {
		LOG_ERR("LiteX CLK driver cannot wait shorter than ca. 1ms\n");
		return -EINVAL;
	}

	/* Read wait_drdy timeout from device property*/
	timeout->drdy = DRDY_TIMEOUT;
	if (timeout->drdy < 1) {
		LOG_ERR("LiteX CLK driver cannot wait shorter than ca. 1ms\n");
		return -EINVAL;
	}

	return 0;
}

static int litex_clk_dts_clkouts_read(void)
{
	struct litex_clk_range clkout_div;
	struct litex_clk_clkout *lcko;

	litex_clk_dts_clkout_ranges_read(&clkout_div);

#if CLKOUT_EXIST(0) == 1
		CLKOUT_INIT(0)
#endif
#if CLKOUT_EXIST(1) == 1
		CLKOUT_INIT(1)
#endif
#if CLKOUT_EXIST(2) == 1
		CLKOUT_INIT(2)
#endif
#if CLKOUT_EXIST(3) == 1
		CLKOUT_INIT(3)
#endif
#if CLKOUT_EXIST(4) == 1
		CLKOUT_INIT(4)
#endif
#if CLKOUT_EXIST(5) == 1
		CLKOUT_INIT(5)
#endif
#if CLKOUT_EXIST(6) == 1
		CLKOUT_INIT(6)
#endif
	return 0;
}

static void litex_clk_init_clkouts(void)
{
	struct litex_clk_clkout *lcko;
	int i;

	for (i = 0; i < ldev->nclkout; i++) {
		lcko = &ldev->clkouts[i];
		lcko->base = ldev->base;
		/* mark defaults to set */
		lcko->ts_config.freq = lcko->def.freq;
		lcko->ts_config.duty = lcko->def.duty;
		lcko->ts_config.phase = lcko->def.phase;
	}
}

static int litex_clk_dts_cnt_clocks(void)
{
	return NCLKOUT;
}

static void litex_clk_dts_global_ranges_read(void)
{
	ldev->divclk.min = DIVCLK_DIVIDE_MIN;
	ldev->divclk.max = DIVCLK_DIVIDE_MAX;
	ldev->clkfbout.min = CLKFBOUT_MULT_MIN;
	ldev->clkfbout.max = CLKFBOUT_MULT_MAX;
	ldev->vco.min = VCO_FREQ_MIN;
	ldev->vco.max = VCO_FREQ_MAX;
	ldev->vco_margin = VCO_MARGIN;
}

static int litex_clk_dts_global_read(void)
{
	int ret;

	ldev->nclkout = litex_clk_dts_cnt_clocks();

	clkouts = k_malloc(sizeof(struct litex_clk_clkout) * ldev->nclkout);
	ldev->update_clkout = k_malloc(sizeof(uint8_t) * ldev->nclkout);
	if (!clkouts || !ldev->update_clkout) {
		LOG_ERR("CLKOUT memory allocation failure!");
		return -ENOMEM;
	}
	ldev->clkouts = clkouts;

	ret = litex_clk_dts_timeout_read(&ldev->timeout);
	if (ret != 0) {
		return ret;
	}

	litex_clk_dts_global_ranges_read();

	return 0;
}

static int litex_clk_init_glob_clk(void)
{
	int ret;

	/* Power on MMCM module */
	ret = litex_clk_change_value(FULL_REG_16, FULL_REG_16, POWER_REG);
	if (ret != 0) {
		LOG_ERR("MMCM initialization failure, ret: %d", ret);
		return ret;
	}

	return 0;
}
/* Enable module, set global divider, multiplier, default clkout parameters */
static int litex_clk_init(const struct device *dev)
{
	int ret;

	ldev = k_malloc(sizeof(struct litex_clk_device));
	if (ldev == NULL) {
		return -ENOMEM;
	}

	ldev->base = (uint32_t *)DRP_BASE;
	if (ldev->base == NULL) {
		return -EIO;
	}

	ret = litex_clk_dts_global_read();
	if (ret != 0) {
		return ret;
	}

	ret = litex_clk_dts_clkouts_read();
	if (ret != 0) {
		return ret;
	}

	litex_clk_init_clkouts();

	ret = litex_clk_init_glob_clk();
	if (ret != 0) {
		return ret;
	}

	ret = litex_clk_set_all_def_clkouts();
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_CLOCK_CONTROL_LOG_LEVEL_DBG
	litex_clk_print_all_params();
	litex_clk_print_all_regs();
#endif /* CONFIG_CLOCK_CONTROL_LOG_LEVEL_DBG */

	LOG_INF("LiteX Clock Control driver initialized");
	return 0;
}

static const struct litex_clk_device ldev_init = {
	.base = (uint32_t *)DRP_BASE,
	.timeout = {LOCK_TIMEOUT, DRDY_TIMEOUT},
	.divclk = {DIVCLK_DIVIDE_MIN, DIVCLK_DIVIDE_MAX},
	.clkfbout = {CLKFBOUT_MULT_MIN, CLKFBOUT_MULT_MAX},
	.vco = {VCO_FREQ_MIN, VCO_FREQ_MAX},
	.vco_margin = VCO_MARGIN,
	.nclkout = NCLKOUT
};

DEVICE_DT_DEFINE(DT_NODELABEL(clock0), litex_clk_init, NULL,
		    NULL, &ldev_init, POST_KERNEL,
		    CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &litex_clk_api);
