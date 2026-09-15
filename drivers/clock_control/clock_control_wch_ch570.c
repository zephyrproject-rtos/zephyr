/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: This clock controller driver only supports the WCH CH570 and CH572 SoCs.
 *
 * Documentation for this driver can be found at:
 * <https://www.wch-ic.com/downloads/CH572DS1_PDF.html> page 49-52 and 40-42.
 */

#define DT_DRV_COMPAT wch_ch570_clkctrl

#include <inttypes.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/dt-bindings/clock/ch570_clock.h>
#include <hal_ch32fun.h>
#include "soc.h"

#define FREQ_LSI     DT_PROP(DT_NODELABEL(clk_lsi), clock_frequency)
#define FREQ_HSE     DT_PROP(DT_NODELABEL(clk_hse), clock_frequency)
#define PLL_SOURCE   DT_CLOCKS_CTLR(DT_NODELABEL(pll))
#define FREQ_PLL     (DT_PROP(PLL_SOURCE, clock_frequency) * 18.75)
#define MAX_SYS_FREQ 100000000 /* 100 MHz */

enum ch570_clock_source {
	CH570_HSE,
	CH570_LSI,
	CH570_PLL
};

struct clock_control_wch_ch570_config {
	enum ch570_clock_source src_clk;
	uint8_t hse_current;
	uint8_t hse_capacity;
	uint8_t divider;
};

/* See kernel/sys_clock_hw_cycles.c */
extern unsigned int z_clock_hw_cycles_per_sec;

static inline void ch570_safe_write(volatile uint8_t *reg, uint8_t val)
{
	uint32_t irq_key;

	irq_key = sys_safe_access_enable();
	*reg = val;
	sys_safe_access_disable(irq_key);
}

static enum clock_control_status clock_control_wch_ch570_get_status(const struct device *dev,
								    clock_control_subsys_t sys)
{
	uint8_t id = (uintptr_t)sys;
	uint8_t mask = BIT(CH570_GET_BIT_IDX(id));
	uint8_t val;

	switch (CH570_GET_REG_NUMBER(id)) {
	case CH570_SLP_CLK_REG0:
		val = R8_SLP_CLK_OFF0;
		break;
	case CH570_SLP_CLK_REG1:
		val = R8_SLP_CLK_OFF1;
		break;
	default:
		return -EINVAL;
	}

	/* On sleep register, 1 means OFF and 0 means ON */
	if ((val & mask) == mask) {
		return CLOCK_CONTROL_STATUS_OFF;
	} else if ((val & mask) == 0) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
}

static int clock_control_wch_ch570_on(const struct device *dev, clock_control_subsys_t sys)
{
	uint8_t id = (uintptr_t)sys;
	uint8_t val;

	switch (CH570_GET_REG_NUMBER(id)) {
	case CH570_SLP_CLK_REG0:
		val = R8_SLP_CLK_OFF0;
		val &= ~BIT(CH570_GET_BIT_IDX(id));
		ch570_safe_write(&R8_SLP_CLK_OFF0, val);
		break;
	case CH570_SLP_CLK_REG1:
		val = R8_SLP_CLK_OFF1;
		val &= ~BIT(CH570_GET_BIT_IDX(id));
		ch570_safe_write(&R8_SLP_CLK_OFF1, val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int clock_control_wch_ch570_off(const struct device *dev, clock_control_subsys_t sys)
{
	uint8_t id = (uintptr_t)sys;
	uint8_t val;

	switch (CH570_GET_REG_NUMBER(id)) {
	case CH570_SLP_CLK_REG0:
		val = R8_SLP_CLK_OFF0;
		val |= BIT(CH570_GET_BIT_IDX(id));
		ch570_safe_write(&R8_SLP_CLK_OFF0, val);
		break;
	case CH570_SLP_CLK_REG1:
		val = R8_SLP_CLK_OFF1;
		val |= BIT(CH570_GET_BIT_IDX(id));
		ch570_safe_write(&R8_SLP_CLK_OFF1, val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int clock_control_wch_ch570_get_rate(const struct device *dev, clock_control_subsys_t sys,
					    uint32_t *rate)
{
	ARG_UNUSED(sys);
	const struct clock_control_wch_ch570_config *config = dev->config;

	/*
	 * FckLSI = 24~42KHz;
	 * Fck32m = 32MHz;
	 * Fpll = Fck32m * 18.75 = 600MHz;
	 * Fsys = CLK_MODE == LSI ? FckLSI : (CLK_MODE == PLL ? Fpll : Fck32m) / CLK_DIV;
	 */
	switch (config->src_clk) {
	case CH570_HSE:
		*rate = FREQ_HSE / config->divider;
		break;
	case CH570_PLL:
		*rate = FREQ_PLL / config->divider;
		break;
	case CH570_LSI:
		*rate = FREQ_LSI;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(clock_control, clock_control_wch_ch570_api) = {
	.on = clock_control_wch_ch570_on,
	.off = clock_control_wch_ch570_off,
	.get_rate = clock_control_wch_ch570_get_rate,
	.get_status = clock_control_wch_ch570_get_status,
};

static int clock_control_wch_ch570_init(const struct device *dev)
{
	int err;
	uint32_t irq_key;
	uint8_t hse_tune, clk_cfg;
	const struct clock_control_wch_ch570_config *config = dev->config;

	/* Set runtime frequency */
	err = clock_control_wch_ch570_get_rate(dev, NULL, &z_clock_hw_cycles_per_sec);
	if (err != 0) {
		return err;
	}

	/* Check that the resulting frequency is within the allowed range */
	if (z_clock_hw_cycles_per_sec > MAX_SYS_FREQ) {
		return -ENOTSUP;
	}

	/* Configure HSE */
	hse_tune = FIELD_PREP(RB_XT32M_I_BIAS, config->hse_current) |
		   FIELD_PREP(RB_XT32M_C_LOAD, config->hse_capacity);
	ch570_safe_write(&R8_XT32M_TUNE, hse_tune);

	/* Configure source clock (for HCLK) */
	clk_cfg = 0x00;

	/* Source mode: 00/10 = HSE, 01 = PLL, 11 = LSI */
	switch (config->src_clk) {
	case CH570_HSE:
		clk_cfg |= FIELD_PREP(RB_CLK_SYS_MOD, 0b00);
		break;
	case CH570_PLL:
		clk_cfg |= FIELD_PREP(RB_CLK_SYS_MOD, 0b01);
		break;
	case CH570_LSI:
		clk_cfg |= FIELD_PREP(RB_CLK_SYS_MOD, 0b11);
		break;
	default:
		return -EINVAL;
	}

	/* Divider field range: 0-31, 0 is used to divide by 32 and 1 to disable the system clock */
	clk_cfg |= FIELD_PREP(RB_CLK_PLL_DIV, (config->divider) % 32);
	ch570_safe_write(&R8_CLK_SYS_CFG, clk_cfg);

	/* Disable clock for all devices by default */
	irq_key = sys_safe_access_enable();
	R8_SLP_CLK_OFF0 = 0x1F;
	R8_SLP_CLK_OFF1 = 0xFF;
	sys_safe_access_disable(irq_key);

	return 0;
}

#define CLOCK_CONTROL_WCH_SOURCE(idx)                                                              \
	(DT_SAME_NODE(DT_INST_CLOCKS_CTLR(idx), DT_NODELABEL(clk_hse))   ? CH570_HSE               \
	 : DT_SAME_NODE(DT_INST_CLOCKS_CTLR(idx), DT_NODELABEL(clk_lsi)) ? CH570_LSI               \
	 : DT_SAME_NODE(DT_INST_CLOCKS_CTLR(idx), DT_NODELABEL(pll))     ? CH570_PLL               \
									 : CH570_HSE)

#define CLOCK_CONTROL_WCH_INIT(idx)                                                                \
	static const struct clock_control_wch_ch570_config                                         \
		clock_control_wch_ch570_##idx##_config = {                                         \
			.src_clk = CLOCK_CONTROL_WCH_SOURCE(idx),                                  \
			.hse_current = DT_INST_ENUM_IDX(idx, hse_bias_current_percent),            \
			.hse_capacity = DT_INST_ENUM_IDX(idx, hse_load_capacitance_pf),            \
			.divider = DT_INST_PROP(idx, clock_divider),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, clock_control_wch_ch570_init, NULL, NULL,                       \
			      &clock_control_wch_ch570_##idx##_config, PRE_KERNEL_1,               \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_wch_ch570_api);

/* There is only ever one */
CLOCK_CONTROL_WCH_INIT(0)
