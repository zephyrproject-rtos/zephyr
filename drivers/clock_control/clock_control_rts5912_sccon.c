/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_sccon

#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rts5912_clock.h>

#include <reg/reg_system.h>

LOG_MODULE_REGISTER(sccon, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define RC25M_FREQ DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, rc25m), clock_frequency)
#define PLL_FREQ   DT_PROP(DT_INST_CLOCKS_CTLR_BY_NAME(0, pll), clock_frequency)

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
		module_idx = (clk_idx - I2C0_CLKPWR) >> 2;

		if (on_off) {
			sys_reg->I2CCLK |= BIT(clk_idx);
		} else {
			sys_reg->I2CCLK &= ~BIT(clk_idx);
		}

		LOG_DBG("Turn I2C%d clock <%s>", module_idx, on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_UART:
		if (on_off) {
			sys_reg->UARTCLK = BIT(clk_idx);
		} else {
			sys_reg->UARTCLK &= ~BIT(clk_idx);
		}

		LOG_DBG("Turn UART0 clock <%s>", on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_ADC:
		if (on_off) {
			sys_reg->ADCCLK = BIT(clk_idx);
		} else {
			sys_reg->ADCCLK &= ~BIT(clk_idx);
		}

		LOG_DBG("Turn ADC clock <%s>", on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_PERIPH_GRP0:
		if (on_off) {
			sys_reg->PERICLKPWR0 |= BIT(clk_idx);
		} else {
			sys_reg->PERICLKPWR0 &= ~BIT(clk_idx);
		}

		LOG_DBG("Turn GRP0-%d clock <%s>", clk_idx, on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_PERIPH_GRP1:
		if (on_off) {
			sys_reg->PERICLKPWR1 |= BIT(clk_idx);
		} else {
			sys_reg->PERICLKPWR1 &= ~BIT(clk_idx);
		}

		LOG_DBG("Turn GRP1-%d clock <%s>", clk_idx, on_off ? "ON" : "OFF");
		break;
	case RTS5912_SCCON_PERIPH_GRP2:
		if (on_off) {
			sys_reg->PERICLKPWR2 |= BIT(clk_idx);
		} else {
			sys_reg->PERICLKPWR2 &= ~BIT(clk_idx);
		}

		LOG_DBG("Turn GRP2-%d clock <%s>", clk_idx, on_off ? "ON" : "OFF");
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
	uint32_t src, divide, freq = 0;

	switch (clk_grp) {
	case RTS5912_SCCON_I2C:
		module_idx = (clk_idx - I2C0_CLKPWR) >> 2;

		offset = (SYSTEM_I2CCLK_I2C1CLKSRC_Pos - SYSTEM_I2CCLK_I2C0CLKSRC_Pos) * module_idx;
		src = ((sys_reg->I2CCLK >> offset) & SYSTEM_I2CCLK_I2C0CLKSRC_Msk) ? PLL_FREQ
										   : RC25M_FREQ;

		offset = SYSTEM_I2CCLK_I2C0CLKDIV_Pos;
		offset += (SYSTEM_I2CCLK_I2C1CLKDIV_Pos - SYSTEM_I2CCLK_I2C0CLKDIV_Pos) *
			   module_idx;
		divide = (sys_reg->I2CCLK >> offset) & SYSTEM_I2CCLK_I2C0CLKDIV_Msk;

		freq = src >> divide;

		LOG_DBG("I2C%d: src<%s> divide<%ld> freq<%d>", module_idx,
			(src == PLL_FREQ) ? "PLL" : "RC25M", BIT(divide), freq);
		break;
	case RTS5912_SCCON_UART:
		src = (sys_reg->UARTCLK & SYSTEM_UARTCLK_SRC_Msk) ? PLL_FREQ : RC25M_FREQ;
		divide = (sys_reg->UARTCLK & SYSTEM_UARTCLK_DIV_Msk) >> SYSTEM_UARTCLK_DIV_Pos;
		freq = src >> divide;

		LOG_DBG("UART0: src<%s> divide<%ld> freq<%d>", (src == PLL_FREQ) ? "PLL" : "RC25M",
			BIT(divide), freq);
		break;
	case RTS5912_SCCON_ADC:
		src = (sys_reg->ADCCLK & SYSTEM_ADCCLK_SRC_Msk) ? RC25M_FREQ : PLL_FREQ;
		divide = (sys_reg->ADCCLK & SYSTEM_ADCCLK_DIV_Msk) >> SYSTEM_ADCCLK_DIV_Pos;

		switch (divide) {
		case 0:
			__fallthrough;
		case 1:
			__fallthrough;
		case 2:
			__fallthrough;
		case 3:
			divide += 1;
			freq = src / divide;
			break;
		case 4:
			freq = src / 6;
			divide = 6;
			break;
		case 5:
			freq = src / 8;
			divide = 8;
			break;
		case 6:
			freq = src / 12;
			divide = 12;
			break;
		case 7:
			freq = src / 16;
			divide = 16;
			break;
		default:
			return -EINVAL;
		}

		LOG_DBG("ADC0: src<%s> divide<%d> freq<%d>", (src == PLL_FREQ) ? "PLL" : "RC25M",
			divide, freq);
		break;
	case RTS5912_SCCON_SYS:
		src = (sys_reg->SYSCLK & SYSTEM_SYSCLK_SRC_Msk) ? PLL_FREQ : RC25M_FREQ;
		divide = (sys_reg->SYSCLK & SYSTEM_SYSCLK_DIV_Msk) ? 0x01ul : 0x00ul;
		freq = src >> divide;

		LOG_DBG("System Clock: src<%s> divide<%d> freq<%d>",
			(src == PLL_FREQ) ? "PLL" : "RC25M", divide, freq);
		break;
	case RTS5912_SCCON_PERIPH_GRP0:
		__fallthrough;
	case RTS5912_SCCON_PERIPH_GRP1:
		__fallthrough;
	case RTS5912_SCCON_PERIPH_GRP2:
		LOG_ERR("Not support peripheral group #%d-%d", clk_grp, clk_idx);
		return -ENOTSUP;
	default:
		LOG_ERR("Unknown peripheral group #%d", clk_grp);
		return -EINVAL;
	}

	*rate = freq;
	return 0;
}

static DEVICE_API(clock_control, rts5912_clock_control_api) = {
	.on = rts5912_clock_control_on,
	.off = rts5912_clock_control_off,
	.get_rate = rts5912_clock_control_get_rate,
};

static int rts5912_clock_control_init(const struct device *dev)
{
	const struct rts5912_sccon_config *const config = dev->config;
	SYSTEM_Type *sys_reg = (SYSTEM_Type *)config->reg_base;

	/* Setup system clock */
	sys_reg->SYSCLK = (config->sysclk_src << SYSTEM_SYSCLK_SRC_Pos) |
			   (config->sysclk_div << SYSTEM_SYSCLK_DIV_Pos);

	return 0;
}

const struct rts5912_sccon_config rts5912_sccon_config = {
	.reg_base = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.sysclk_src = 1,
	.sysclk_div = 0,
};

DEVICE_DT_INST_DEFINE(0, &rts5912_clock_control_init, NULL, NULL, &rts5912_sccon_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &rts5912_clock_control_api);
