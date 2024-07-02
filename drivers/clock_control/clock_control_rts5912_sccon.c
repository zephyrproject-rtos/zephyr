/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_sccon

#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/dt-bindings/clock/rts5912_clock.h>

#include "reg/reg_system.h"

LOG_MODULE_REGISTER(sccon, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define RC25M_FREQ (25000000UL)
#define PLL_FREQ   (100000000UL)

struct rts5912_sccon_config {
	uint32_t reg_base;
	uint8_t sysclk_src;
	uint8_t sysclk_div;
};

static int rts5912_periph_clock_control(const struct device *dev, clock_control_subsys_t sub_system,
					bool on_off)
{
	const struct rts5912_sccon_config *const config = dev->config;
	struct rts5912_sccon_subsys *subsys = (struct rts5912_sccon_subsys *)sub_system;

	SYSTEM_Type *sys_reg = (SYSTEM_Type *)config->reg_base;

	uint32_t clk_grp = subsys->clk_grp;
	uint32_t clk_idx = subsys->clk_idx;

	uint32_t module_idx;

	switch (clk_grp) {
	case RTS5912_SCCON_I2C:
		module_idx = ((clk_idx - I2C0_CLKPWR) >> 2);

		if (on_off) {
			sys_reg->I2CCLK |= (0x01ul << clk_idx);
		} else {
			sys_reg->I2CCLK &= ~(0x01ul << clk_idx);
		}

		LOG_DBG("Trun I2C%d clock <%s>", module_idx, on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_UART:
		if (on_off) {
			sys_reg->UARTCLK = (0x01ul << clk_idx);
		} else {
			sys_reg->UARTCLK &= ~(0x01ul << clk_idx);
		}

		LOG_DBG("Trun UART0 clock <%s>", on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_ADC:
		if (on_off) {
			sys_reg->ADCCLK = (0x01ul << clk_idx);
		} else {
			sys_reg->ADCCLK &= ~(0x01ul << clk_idx);
		}

		LOG_DBG("Trun ADC clock <%s>", on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_PERIPH_GRP0:
		if (on_off) {
			sys_reg->PERICLKPWR0 |= (0x01ul << clk_idx);
		} else {
			sys_reg->PERICLKPWR0 &= ~(0x01ul << clk_idx);
		}

		LOG_DBG("Trun GRP0-%d clock <%s>", clk_idx, on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_PERIPH_GRP1:
		if (on_off) {
			sys_reg->PERICLKPWR1 |= (0x01ul << clk_idx);
		} else {
			sys_reg->PERICLKPWR1 &= ~(0x01ul << clk_idx);
		}

		LOG_DBG("Trun GRP1-%d clock <%s>", clk_idx, on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_PERIPH_GRP2:
		if (on_off) {
			sys_reg->PERICLKPWR2 |= (0x01ul << clk_idx);
		} else {
			sys_reg->PERICLKPWR2 &= ~(0x01ul << clk_idx);
		}

		LOG_DBG("Trun GRP2-%d clock <%s>", clk_idx, on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_SYS:
		LOG_ERR("Not support peripheral group #%d-%d", clk_grp, clk_idx);
		return -ENOTSUP;
	default:
		LOG_ERR("Unknown peripheral group #%d", clk_grp);
		return -EINVAL;
	}

	return 0;
}

static int rts5912_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	return rts5912_periph_clock_control(dev, sub_system, true);
}

static int rts5912_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	return rts5912_periph_clock_control(dev, sub_system, false);
}

static int rts5912_clock_control_get_rate(const struct device *dev,
					  clock_control_subsys_t sub_system, uint32_t *rate)
{
	const struct rts5912_sccon_config *const config = dev->config;
	struct rts5912_sccon_subsys *subsys = (struct rts5912_sccon_subsys *)sub_system;

	SYSTEM_Type *sys_reg = (SYSTEM_Type *)config->reg_base;

	uint32_t clk_grp = subsys->clk_grp;
	uint32_t clk_idx = subsys->clk_idx;
	uint32_t module_idx;

	uint32_t offset;
	uint32_t src, div, freq = 0;

	switch (clk_grp) {
	case RTS5912_SCCON_I2C:
		module_idx = ((clk_idx - I2C0_CLKPWR) >> 2);

		offset = (SYSTEM_I2CCLK_I2C1CLKSRC_Pos - SYSTEM_I2CCLK_I2C0CLKSRC_Pos) * module_idx;
		src = ((sys_reg->I2CCLK >> offset) & SYSTEM_I2CCLK_I2C0CLKSRC_Msk) ? PLL_FREQ
										   : RC25M_FREQ;

		offset = SYSTEM_I2CCLK_I2C0CLKDIV_Pos;
		offset += ((SYSTEM_I2CCLK_I2C1CLKDIV_Pos - SYSTEM_I2CCLK_I2C0CLKDIV_Pos) *
			   module_idx);
		div = ((sys_reg->I2CCLK >> offset) & SYSTEM_I2CCLK_I2C0CLKDIV_Msk);

		freq = (src >> div);

		LOG_DBG("I2C%d: src<%s> div<%ld> freq<%d>", module_idx,
			(src == PLL_FREQ) ? "PLL" : "RC25M", (0x1ul << div), freq);
		break;
	case RTS5912_SCCON_UART:
		src = (sys_reg->UARTCLK & SYSTEM_UARTCLK_SRC_Msk) ? PLL_FREQ : RC25M_FREQ;
		div = ((sys_reg->UARTCLK & SYSTEM_UARTCLK_DIV_Msk) >> SYSTEM_UARTCLK_DIV_Pos);
		freq = (src >> div);

		LOG_DBG("UART0: src<%s> div<%ld> freq<%d>", (src == PLL_FREQ) ? "PLL" : "RC25M",
			(0x1ul << div), freq);
		break;
	case RTS5912_SCCON_ADC:
		src = (sys_reg->ADCCLK & SYSTEM_ADCCLK_SRC_Msk) ? RC25M_FREQ : PLL_FREQ;
		div = ((sys_reg->ADCCLK & SYSTEM_ADCCLK_DIV_Msk) >> SYSTEM_ADCCLK_DIV_Pos);

		switch (div) {
		case 0:
		case 1:
		case 2:
		case 3:
			freq = (src / (div + 1));
			div += 1;
			break;
		case 4:
			freq = (src / 6);
			div = 6;
			break;
		case 5:
			freq = (src / 8);
			div = 8;
			break;
		case 6:
			freq = (src / 12);
			div = 12;
			break;
		case 7:
			freq = (src / 16);
			div = 16;
			break;
		default:
			return -EINVAL;
		}

		LOG_DBG("ADC0: src<%s> div<%d> freq<%d>", (src == PLL_FREQ) ? "PLL" : "RC25M", div,
			freq);
		break;
	case RTS5912_SCCON_SYS:
		src = (sys_reg->SYSCLK & SYSTEM_SYSCLK_SRC_Msk) ? PLL_FREQ : RC25M_FREQ;
		div = (sys_reg->SYSCLK & SYSTEM_SYSCLK_DIV_Msk) ? 0x01ul : 0x00ul;
		freq = (src >> div);

		LOG_DBG("System Clock: src<%s> div<%d> freq<%d>",
			(src == PLL_FREQ) ? "PLL" : "RC25M", div, freq);
		break;
	case RTS5912_SCCON_PERIPH_GRP0:
	case RTS5912_SCCON_PERIPH_GRP1:
	case RTS5912_SCCON_PERIPH_GRP2:
		LOG_ERR("Not support peripheral group #%d-%d", clk_grp, clk_idx);
		return -ENOTSUP;
	default:;
		LOG_ERR("Unknown peripheral group #%d", clk_grp);
		return -EINVAL;
	}

	*rate = freq;
	return 0;
}

static struct clock_control_driver_api rts5912_clock_control_api = {
	.on = rts5912_clock_control_on,
	.off = rts5912_clock_control_off,
	.get_rate = rts5912_clock_control_get_rate,
};

static int rts5912_clock_control_init(const struct device *dev)
{
	const struct rts5912_sccon_config *const config = dev->config;
	SYSTEM_Type *sys_reg = (SYSTEM_Type *)config->reg_base;

	/* Setup system clock */
	sys_reg->SYSCLK = ((config->sysclk_src << SYSTEM_SYSCLK_SRC_Pos) |
			   (config->sysclk_div << SYSTEM_SYSCLK_DIV_Pos));

	return 0;
}

const struct rts5912_sccon_config rts5912_sccon_config = {
	.reg_base = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.sysclk_src = (uint8_t)DT_INST_PROP_OR(0, sysclk_src, 1),
	.sysclk_div = (uint8_t)DT_INST_PROP_OR(0, sysclk_div, 0),
};

DEVICE_DT_INST_DEFINE(0, &rts5912_clock_control_init, NULL, NULL, &rts5912_sccon_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &rts5912_clock_control_api);
