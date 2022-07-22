/*
 * Copyright (c) 2022 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_ast10x0_clock
#include <errno.h>
#include <zephyr/dt-bindings/clock/ast10x0_clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control_ast10x0);

#define HPLL_FREQ			MHZ(1000)

/*
 * CLK_STOP_CTRL0/1_SET registers:
 *   - Each bit in these registers controls a clock gate
 *   - Write '1' to a bit: turn OFF the corresponding clock
 *   - Write '0' to a bit: no effect
 * CLK_STOP_CTRL0/1_CLEAR register:
 *   - Write '1' to a bit: clear the corresponding bit in CLK_STOP_CTRL0/1.
 *                         (turn ON the corresponding clock)
 */
#define CLK_STOP_CTRL0_SET		0x80
#define CLK_STOP_CTRL0_CLEAR		0x84
#define CLK_STOP_CTRL1_SET		0x90
#define CLK_STOP_CTRL1_CLEAR		0x94

#define CLK_SELECTION_REG4		0x310
#define   I3C_CLK_SRC_SEL		BIT(31)
#define     I3C_CLK_SRC_HPLL		0
#define     I3C_CLK_SRC_480M		1
#define   I3C_CLK_DIV_SEL		GENMASK(30, 28)
#define     I3C_CLK_DIV_REG_TO_VAL(x)	((x == 0) ? 2 : (x + 1))
#define   PCLK_DIV_SEL			GENMASK(11, 8)
#define     PCLK_DIV_REG_TO_VAL(x)	((x + 1) << 1)
#define CLK_SELECTION_REG5		0x314
#define   HCLK_DIV_SEL			GENMASK(30, 28)
#define     HCLK_DIV_REG_TO_VAL(x)	((x == 0) ? 2 : x + 1)

struct clock_aspeed_config {
	const struct device *syscon;
};

#define DEV_CFG(dev) ((const struct clock_aspeed_config *const)(dev)->config)

static int aspeed_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	const struct device *syscon = DEV_CFG(dev)->syscon;
	uint32_t clk_gate = (uint32_t)sub_system;
	uint32_t addr = CLK_STOP_CTRL0_CLEAR;

	/* there is no on/off control for group2 clocks */
	if (clk_gate >= ASPEED_CLK_GRP_2_OFFSET) {
		return 0;
	}

	if (clk_gate >= ASPEED_CLK_GRP_1_OFFSET) {
		clk_gate -= ASPEED_CLK_GRP_1_OFFSET;
		addr = CLK_STOP_CTRL1_CLEAR;
	}

	syscon_write_reg(syscon, addr, BIT(clk_gate));

	return 0;
}

static int aspeed_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	const struct device *syscon = DEV_CFG(dev)->syscon;
	uint32_t clk_gate = (uint32_t)sub_system;
	uint32_t addr = CLK_STOP_CTRL0_SET;

	/* there is no on/off control for group2 clocks */
	if (clk_gate >= ASPEED_CLK_GRP_2_OFFSET) {
		return 0;
	}

	if (clk_gate >= ASPEED_CLK_GRP_1_OFFSET) {
		clk_gate -= ASPEED_CLK_GRP_1_OFFSET;
		addr = CLK_STOP_CTRL1_SET;
	}

	syscon_write_reg(syscon, addr, BIT(clk_gate));

	return 0;
}

static int aspeed_clock_control_get_rate(const struct device *dev,
					 clock_control_subsys_t sub_system, uint32_t *rate)
{
	const struct device *syscon = DEV_CFG(dev)->syscon;
	uint32_t clk_id = (uint32_t)sub_system;
	uint32_t reg, src, clk_div;

	switch (clk_id) {
	case ASPEED_CLK_I3C0:
	case ASPEED_CLK_I3C1:
	case ASPEED_CLK_I3C2:
	case ASPEED_CLK_I3C3:
		syscon_read_reg(syscon, CLK_SELECTION_REG4, &reg);
		if (FIELD_GET(I3C_CLK_SRC_SEL, reg) == I3C_CLK_SRC_HPLL) {
			src = HPLL_FREQ;
		} else {
			src = MHZ(480);
		}
		clk_div = I3C_CLK_DIV_REG_TO_VAL(FIELD_GET(I3C_CLK_DIV_SEL, reg));
		*rate = src / clk_div;
		break;
	case ASPEED_CLK_HCLK:
		src = HPLL_FREQ;
		syscon_read_reg(syscon, CLK_SELECTION_REG5, &reg);
		clk_div = HCLK_DIV_REG_TO_VAL(FIELD_GET(HCLK_DIV_SEL, reg));
		*rate = src / clk_div;
		break;
	case ASPEED_CLK_PCLK:
		src = HPLL_FREQ;
		syscon_read_reg(syscon, CLK_SELECTION_REG4, &reg);
		clk_div = PCLK_DIV_REG_TO_VAL(FIELD_GET(PCLK_DIV_SEL, reg));
		*rate = src / clk_div;
		break;
	case ASPEED_CLK_UART1:
	case ASPEED_CLK_UART2:
	case ASPEED_CLK_UART3:
	case ASPEED_CLK_UART4:
	case ASPEED_CLK_UART5:
	case ASPEED_CLK_UART6:
	case ASPEED_CLK_UART7:
	case ASPEED_CLK_UART8:
	case ASPEED_CLK_UART9:
	case ASPEED_CLK_UART10:
	case ASPEED_CLK_UART11:
	case ASPEED_CLK_UART12:
	case ASPEED_CLK_UART13:
		*rate = MHZ(24) / 13;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int aspeed_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct clock_control_driver_api aspeed_clk_api = {
	.on = aspeed_clock_control_on,
	.off = aspeed_clock_control_off,
	.get_rate = aspeed_clock_control_get_rate,
};

#define ASPEED_CLOCK_INIT(n)                                                                       \
	static const struct clock_aspeed_config clock_aspeed_cfg_##n = {                           \
		.syscon = DEVICE_DT_GET(DT_NODELABEL(syscon)),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &aspeed_clock_control_init, NULL, NULL, &clock_aspeed_cfg_##n,    \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &aspeed_clk_api);

DT_INST_FOREACH_STATUS_OKAY(ASPEED_CLOCK_INIT)
