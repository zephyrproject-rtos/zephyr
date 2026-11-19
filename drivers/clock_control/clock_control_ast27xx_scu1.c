/*
 * Copyright (c) 2023 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_ast27xx_scu1_clock

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/dt-bindings/clock/ast27xx_clock.h>
#include <zephyr/sys/util.h>

/* Clock stop registers: write 1 to the first to stop a clock, to the second to run it */
#define SCU1_CLK_STOP0        0x240U
#define SCU1_CLK_STOP0_CLR    0x244U
#define SCU1_CLK_STOP1        0x260U
#define SCU1_CLK_STOP1_CLR    0x264U
#define SCU1_CLK_STOP_BANK_SZ 32U

#define SCU1_CLK_SEL1    0x280U
#define SCU1_CLK_SEL2    0x284U
#define SCU1_HPLL_PARAM  0x300U
#define SCU1_APLL_PARAM  0x310U
#define SCU1_DPLL_PARAM  0x320U
#define SCU1_UXCLK_CTRL  0x330U
#define SCU1_HUXCLK_CTRL 0x334U

#define SCU1_CLKIN_RATE MHZ(25)

/* Fout = CLKIN * ((M + 1) / (N + 1)) / (P + 1), or CLKIN in bypass mode */
#define PLL_M      GENMASK(12, 0)
#define PLL_N      GENMASK(18, 13)
#define PLL_P      GENMASK(22, 19)
#define PLL_BYPASS BIT(24)

/* UXCLK and HUXCLK source select: APLL / 4, APLL / 2, APLL or HPLL */
#define CLK_SEL2_UXCLK  GENMASK(1, 0)
#define CLK_SEL2_HUXCLK GENMASK(4, 3)

/* UART clock generator: Fout = Fin * R / (2 * N) */
#define UART_CLKGEN_R GENMASK(7, 0)
#define UART_CLKGEN_N GENMASK(17, 8)

struct clock_ast27xx_scu1_config {
	const struct device *syscon;
};

static int ast27xx_scu1_gate_reg(uint32_t clk_id, uint32_t *stop, uint32_t *clr, uint32_t *bit)
{
	if (clk_id >= SCU1_CLK_GATE_NUM) {
		return -EINVAL;
	}

	if (clk_id < SCU1_CLK_STOP_BANK_SZ) {
		*stop = SCU1_CLK_STOP0;
		*clr = SCU1_CLK_STOP0_CLR;
		*bit = BIT(clk_id);
	} else {
		*stop = SCU1_CLK_STOP1;
		*clr = SCU1_CLK_STOP1_CLR;
		*bit = BIT(clk_id - SCU1_CLK_STOP_BANK_SZ);
	}

	return 0;
}

static int ast27xx_scu1_clock_control_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_ast27xx_scu1_config *config = dev->config;
	uint32_t stop, clr;
	uint32_t bit;
	int ret;

	ret = ast27xx_scu1_gate_reg((uint32_t)(uintptr_t)sys, &stop, &clr, &bit);
	if (ret != 0) {
		return ret;
	}

	return syscon_write_reg(config->syscon, clr, bit);
}

static int ast27xx_scu1_clock_control_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_ast27xx_scu1_config *config = dev->config;
	uint32_t stop, clr;
	uint32_t bit;
	int ret;

	ret = ast27xx_scu1_gate_reg((uint32_t)(uintptr_t)sys, &stop, &clr, &bit);
	if (ret != 0) {
		return ret;
	}

	return syscon_write_reg(config->syscon, stop, bit);
}

static int ast27xx_scu1_pll_rate(const struct device *syscon, uint32_t param, uint64_t *rate)
{
	uint32_t mult;
	uint32_t post;
	uint32_t val;
	int ret;

	ret = syscon_read_reg(syscon, param, &val);
	if (ret != 0) {
		return ret;
	}

	if ((val & PLL_BYPASS) != 0U) {
		*rate = SCU1_CLKIN_RATE;
		return 0;
	}

	mult = (FIELD_GET(PLL_M, val) + 1U) / (FIELD_GET(PLL_N, val) + 1U);
	post = FIELD_GET(PLL_P, val) + 1U;
	*rate = (uint64_t)SCU1_CLKIN_RATE * mult / post;

	return 0;
}

static int ast27xx_scu1_uxclk_src_rate(const struct device *syscon, uint32_t sel, uint64_t *rate)
{
	uint32_t apll_div;
	int ret;

	switch (sel) {
	case 0U:
		apll_div = 4U;
		break;
	case 1U:
		apll_div = 2U;
		break;
	case 2U:
		apll_div = 1U;
		break;
	default:
		return ast27xx_scu1_pll_rate(syscon, SCU1_HPLL_PARAM, rate);
	}

	ret = ast27xx_scu1_pll_rate(syscon, SCU1_APLL_PARAM, rate);
	if (ret == 0) {
		*rate /= apll_div;
	}

	return ret;
}

static int ast27xx_scu1_uart_rate(const struct device *syscon, bool huart, uint32_t *rate)
{
	uint32_t sel2;
	uint32_t ctrl;
	uint32_t sel;
	uint64_t src;
	uint32_t r;
	uint32_t n;
	int ret;

	ret = syscon_read_reg(syscon, SCU1_CLK_SEL2, &sel2);
	if (ret != 0) {
		return ret;
	}

	if (huart) {
		sel = FIELD_GET(CLK_SEL2_HUXCLK, sel2);
		ret = syscon_read_reg(syscon, SCU1_HUXCLK_CTRL, &ctrl);
	} else {
		sel = FIELD_GET(CLK_SEL2_UXCLK, sel2);
		ret = syscon_read_reg(syscon, SCU1_UXCLK_CTRL, &ctrl);
	}
	if (ret != 0) {
		return ret;
	}

	r = FIELD_GET(UART_CLKGEN_R, ctrl);
	n = FIELD_GET(UART_CLKGEN_N, ctrl);
	if (n == 0U) {
		return -EIO;
	}

	ret = ast27xx_scu1_uxclk_src_rate(syscon, sel, &src);
	if (ret != 0) {
		return ret;
	}

	*rate = (uint32_t)(src * r / (2U * n));

	return 0;
}

/* UART index of a UART clock gate, which is also its UARTX/HUARTX select bit in CLK_SEL1 */
static int ast27xx_scu1_uart_index(uint32_t clk_id)
{
	if (clk_id >= SCU1_CLK_GATE_UART0CLK && clk_id <= SCU1_CLK_GATE_UART3CLK) {
		return (int)(clk_id - SCU1_CLK_GATE_UART0CLK);
	}

	if (clk_id >= SCU1_CLK_GATE_UART5CLK && clk_id <= SCU1_CLK_GATE_UART12CLK) {
		return (int)(clk_id - SCU1_CLK_GATE_UART5CLK) + 5;
	}

	return -EINVAL;
}

static int ast27xx_scu1_clock_control_get_rate(const struct device *dev, clock_control_subsys_t sys,
					       uint32_t *rate)
{
	const struct clock_ast27xx_scu1_config *config = dev->config;
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	uint64_t pll;
	uint32_t sel1;
	int uart;
	int ret;

	switch (clk_id) {
	case SCU1_CLK_HPLL:
		ret = ast27xx_scu1_pll_rate(config->syscon, SCU1_HPLL_PARAM, &pll);
		break;
	case SCU1_CLK_APLL:
		ret = ast27xx_scu1_pll_rate(config->syscon, SCU1_APLL_PARAM, &pll);
		break;
	case SCU1_CLK_DPLL:
		ret = ast27xx_scu1_pll_rate(config->syscon, SCU1_DPLL_PARAM, &pll);
		break;
	case SCU1_CLK_UARTX:
		return ast27xx_scu1_uart_rate(config->syscon, false, rate);
	case SCU1_CLK_HUARTX:
		return ast27xx_scu1_uart_rate(config->syscon, true, rate);
	default:
		uart = ast27xx_scu1_uart_index(clk_id);
		if (uart < 0) {
			return -ENOTSUP;
		}

		ret = syscon_read_reg(config->syscon, SCU1_CLK_SEL1, &sel1);
		if (ret != 0) {
			return ret;
		}

		return ast27xx_scu1_uart_rate(config->syscon, (sel1 & BIT(uart)) != 0U, rate);
	}

	if (ret == 0) {
		*rate = (uint32_t)pll;
	}

	return ret;
}

static DEVICE_API(clock_control, ast27xx_scu1_clock_api) = {
	.on = ast27xx_scu1_clock_control_on,
	.off = ast27xx_scu1_clock_control_off,
	.get_rate = ast27xx_scu1_clock_control_get_rate,
};

#define AST27XX_SCU1_CLOCK_INIT(n)                                                                 \
	static const struct clock_ast27xx_scu1_config clock_ast27xx_scu1_config_##n = {            \
		.syscon = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &clock_ast27xx_scu1_config_##n, PRE_KERNEL_1,   \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ast27xx_scu1_clock_api);

DT_INST_FOREACH_STATUS_OKAY(AST27XX_SCU1_CLOCK_INIT)
