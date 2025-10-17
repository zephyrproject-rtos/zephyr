/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/syna_sr100_clock.h>
#include <zephyr/arch/common/sys_io.h>

#define DT_DRV_COMPAT syna_sr100_clock

#define PLL0_RATE CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

#define DIV_EN    (1 << 2)
#define DIV_BY_3  (1 << 3)
#define DIV_BY_2  ((1 << 4) | DIV_EN)
#define DIV_BY_4  ((2 << 4) | DIV_EN)
#define DIV_BY_6  ((3 << 4) | DIV_EN)
#define DIV_BY_8  ((4 << 4) | DIV_EN)
#define DIV_BY_12 ((5 << 4) | DIV_EN)
#define DIV_MSK   ((7 << 4) | DIV_BY_3 | DIV_EN)

#define GET_CGL_REG(id)	(((id) >> CGL_REG) & 0xFF)
#define GET_AXI_ID(id)	(((id) >> AXI_ID) & 0xFF)
#define GET_CLK_ID(id)	((id) & 0xFF)

struct clock_control_syna_config {
	mem_addr_t regs;
};

static inline uint32_t syna_clk_read(const struct clock_control_syna_config *config, mm_reg_t addr)
{
	return sys_read32(config->regs + addr);
}

static inline void syna_clk_write(const struct clock_control_syna_config *config, uint32_t data,
				  mm_reg_t addr)
{
	sys_write32(data, config->regs + addr);
}

static int syna_clk_set_rate(const struct device *dev, uint32_t id, uint32_t rate)
{
	const struct clock_control_syna_config *config = dev->config;
	uint32_t parent_rate = PLL0_RATE;
	uint32_t offset = GET_CGL_REG(id);
	uint32_t divider = 0;
	uint32_t value;

	if (!offset) {
		return -EINVAL;
	}

	if (rate > parent_rate) {
		return -EINVAL;
	} else if (rate == parent_rate) {
		divider = 0; /* no need to divide */
	} else if (rate >= (parent_rate / 2)) {
		divider = DIV_BY_2;
	} else if (rate >= (parent_rate / 3)) {
		divider = DIV_BY_3;
	} else if (rate >= (parent_rate / 4)) {
		divider = DIV_BY_4;
	} else if (rate >= (parent_rate / 6)) {
		divider = DIV_BY_6;
	} else if (rate >= (parent_rate / 8)) {
		divider = DIV_BY_8;
	} else {
		divider = DIV_BY_12;
	}

	value = syna_clk_read(config, offset);
	value &= ~DIV_MSK;
	value |= divider;
	syna_clk_write(config, value, offset);

	return 0;
}

static uint32_t syna_clk_get_rate(const struct device *dev, uint32_t id)
{
	const struct clock_control_syna_config *config = dev->config;
	uint32_t offset = GET_CGL_REG(id);
	uint32_t parent_rate = PLL0_RATE;
	uint32_t divider = 1;
	uint32_t value;

	if (!offset) {
		return 0;
	}

	value = syna_clk_read(config, offset);
	switch (value & DIV_MSK) {
	case DIV_BY_2:
		divider = 2;
		break;
	case DIV_BY_3:
		divider = 3;
		break;
	case DIV_BY_4:
		divider = 4;
		break;
	case DIV_BY_6:
		divider = 6;
		break;
	case DIV_BY_8:
		divider = 8;
		break;
	case DIV_BY_12:
		divider = 12;
		break;
	default:
		break;
	}

	return parent_rate / divider;
}

static inline int syna_clk_enable(const struct device *dev, uint32_t id, int enable)
{
	const struct clock_control_syna_config *config = dev->config;
	uint32_t offset = CLK_ENABLE1;
	uint32_t value, bit_mask;
	uint32_t cgl = GET_CGL_REG(id);
	int axi_clock;

	axi_clock = GET_AXI_ID(id);
	if (axi_clock > 0) {
		if (axi_clock >= 32U) {
			offset += 4;
			axi_clock -= 32U;
		}

		value = syna_clk_read(config, offset);
		bit_mask = 1U << axi_clock;

		if (enable) {
			value |= bit_mask;
		} else {
			value &= ~(bit_mask);
		}
		syna_clk_write(config, value, offset);
	}

	if (cgl) {
		value = syna_clk_read(config, cgl);
		if (enable) {
			value |= 1;
		} else {
			value &= ~1;
		}
		syna_clk_write(config, value, cgl);
	}

	offset = CLK_ENABLE1;
	id = GET_CLK_ID(id);
	if (id >= 32U) {
		offset += 4;
		id -= 32U;
	}

	value = syna_clk_read(config, offset);
	bit_mask = 1U << id;

	if (enable != 0) {
		value |= bit_mask;
	} else {
		value &= ~(bit_mask);
	}
	syna_clk_write(config, value, offset);

	return 0;
}

static inline int api_on(const struct device *dev, clock_control_subsys_t clkcfg)
{
	uint32_t clock_id = (uint32_t)(uintptr_t)clkcfg;

	return syna_clk_enable(dev, clock_id, 1);
}

static inline int api_off(const struct device *dev, clock_control_subsys_t clkcfg)
{
	uint32_t clock_id = (uint32_t)(uintptr_t)clkcfg;

	return syna_clk_enable(dev, clock_id, 0);
}

static int api_get_rate(const struct device *dev, clock_control_subsys_t clkcfg, uint32_t *rate)
{
	uint32_t clock_id = (uint32_t)(uintptr_t)clkcfg;
	uint32_t clk_rate;

	clk_rate = syna_clk_get_rate(dev, clock_id);
	if (clk_rate == 0) {
		return -EINVAL;
	}

	*rate = clk_rate;

	return 0;
}

static int api_set_rate(const struct device *dev, clock_control_subsys_t clkcfg,
			clock_control_subsys_rate_t rate)
{
	uint32_t clock_id = (uint32_t)(uintptr_t)clkcfg;

	return syna_clk_set_rate(dev, clock_id, (uint32_t)rate);
}

static const struct clock_control_driver_api syna_clkctrl_api = {
	.on = api_on,
	.off = api_off,
	.get_rate = api_get_rate,
	.set_rate = api_set_rate,
};

static const struct clock_control_syna_config syna_config = {
	.regs = DT_INST_REG_ADDR(0),
};

static int syna_clkctrl_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, syna_clkctrl_init, NULL, NULL, &syna_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &syna_clkctrl_api);
