/*
 * Copyright (c) 2023 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_ast27xx_scu1_clock

#include <errno.h>
#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/ast27xx_clock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Clock stop registers: write 1 to the first to stop a clock, to the second to run it */
#define SCU1_CLK_STOP0        0x240
#define SCU1_CLK_STOP0_CLR    0x244
#define SCU1_CLK_STOP1        0x260
#define SCU1_CLK_STOP1_CLR    0x264
#define SCU1_CLK_STOP_BANK_SZ 32U

#define SCU1_CLK_SEL1      0x280
#define SCU1_CLK_SEL2      0x284
#define SCU1_HPLL_PARAM    0x300
#define SCU1_APLL_PARAM    0x310
#define SCU1_DPLL_PARAM    0x320
#define SCU1_UXCLK_CTRL    0x330
#define SCU1_HUXCLK_CTRL   0x334

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
	mem_addr_t base;
};

static int ast27xx_scu1_gate_reg(uint32_t clk_id, mem_addr_t *stop, mem_addr_t *clr,
				 uint32_t *bit)
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
	mem_addr_t stop, clr;
	uint32_t bit;
	int ret;

	ret = ast27xx_scu1_gate_reg((uint32_t)(uintptr_t)sys, &stop, &clr, &bit);
	if (ret != 0) {
		return ret;
	}

	sys_write32(bit, config->base + clr);

	return 0;
}

static int ast27xx_scu1_clock_control_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_ast27xx_scu1_config *config = dev->config;
	mem_addr_t stop, clr;
	uint32_t bit;
	int ret;

	ret = ast27xx_scu1_gate_reg((uint32_t)(uintptr_t)sys, &stop, &clr, &bit);
	if (ret != 0) {
		return ret;
	}

	sys_write32(bit, config->base + stop);

	return 0;
}

static uint64_t ast27xx_scu1_pll_rate(mem_addr_t base, mem_addr_t param)
{
	uint32_t val = sys_read32(base + param);
	uint32_t mul;
	uint32_t div;

	if ((val & PLL_BYPASS) != 0U) {
		return SCU1_CLKIN_RATE;
	}

	mul = (FIELD_GET(PLL_M, val) + 1U) / (FIELD_GET(PLL_N, val) + 1U);
	div = FIELD_GET(PLL_P, val) + 1U;

	return (uint64_t)SCU1_CLKIN_RATE * mul / div;
}

static uint64_t ast27xx_scu1_uxclk_src_rate(mem_addr_t base, uint32_t sel)
{
	switch (sel) {
	case 0U:
		return ast27xx_scu1_pll_rate(base, SCU1_APLL_PARAM) / 4U;
	case 1U:
		return ast27xx_scu1_pll_rate(base, SCU1_APLL_PARAM) / 2U;
	case 2U:
		return ast27xx_scu1_pll_rate(base, SCU1_APLL_PARAM);
	default:
		return ast27xx_scu1_pll_rate(base, SCU1_HPLL_PARAM);
	}
}

static int ast27xx_scu1_uart_rate(mem_addr_t base, bool huart, uint32_t *rate)
{
	uint32_t sel2 = sys_read32(base + SCU1_CLK_SEL2);
	uint32_t ctrl;
	uint32_t sel;
	uint64_t src;
	uint32_t r;
	uint32_t n;

	if (huart) {
		sel = FIELD_GET(CLK_SEL2_HUXCLK, sel2);
		ctrl = sys_read32(base + SCU1_HUXCLK_CTRL);
	} else {
		sel = FIELD_GET(CLK_SEL2_UXCLK, sel2);
		ctrl = sys_read32(base + SCU1_UXCLK_CTRL);
	}

	r = FIELD_GET(UART_CLKGEN_R, ctrl);
	n = FIELD_GET(UART_CLKGEN_N, ctrl);
	if (n == 0U) {
		return -EIO;
	}

	src = ast27xx_scu1_uxclk_src_rate(base, sel);
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

static int ast27xx_scu1_clock_control_get_rate(const struct device *dev,
					       clock_control_subsys_t sys, uint32_t *rate)
{
	const struct clock_ast27xx_scu1_config *config = dev->config;
	uint32_t clk_id = (uint32_t)(uintptr_t)sys;
	int uart;

	switch (clk_id) {
	case SCU1_CLK_HPLL:
		*rate = (uint32_t)ast27xx_scu1_pll_rate(config->base, SCU1_HPLL_PARAM);
		return 0;
	case SCU1_CLK_APLL:
		*rate = (uint32_t)ast27xx_scu1_pll_rate(config->base, SCU1_APLL_PARAM);
		return 0;
	case SCU1_CLK_DPLL:
		*rate = (uint32_t)ast27xx_scu1_pll_rate(config->base, SCU1_DPLL_PARAM);
		return 0;
	case SCU1_CLK_UARTX:
		return ast27xx_scu1_uart_rate(config->base, false, rate);
	case SCU1_CLK_HUARTX:
		return ast27xx_scu1_uart_rate(config->base, true, rate);
	default:
		break;
	}

	uart = ast27xx_scu1_uart_index(clk_id);
	if (uart < 0) {
		return -ENOTSUP;
	}

	return ast27xx_scu1_uart_rate(config->base,
				      (sys_read32(config->base + SCU1_CLK_SEL1) & BIT(uart)) != 0U,
				      rate);
}

static DEVICE_API(clock_control, ast27xx_scu1_clock_api) = {
	.on = ast27xx_scu1_clock_control_on,
	.off = ast27xx_scu1_clock_control_off,
	.get_rate = ast27xx_scu1_clock_control_get_rate,
};

#define AST27XX_SCU1_CLOCK_INIT(n)                                                                 \
	static const struct clock_ast27xx_scu1_config clock_ast27xx_scu1_config_##n = {            \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &clock_ast27xx_scu1_config_##n, PRE_KERNEL_1,   \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ast27xx_scu1_clock_api);

DT_INST_FOREACH_STATUS_OKAY(AST27XX_SCU1_CLOCK_INIT)
