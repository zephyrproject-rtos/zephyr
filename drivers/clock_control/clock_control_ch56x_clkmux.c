/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch56x_clkmux

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <soc.h>

/* CH32V_SYS_R8_CLK_PLL_DIV_REG */
#define CLK_PLL_DIV_KEY  (0x40)
#define CLK_PLL_DIV(div) (div & BIT_MASK(4))

/* CH32V_SYS_R8_CLK_CFG_CTRL_REG */
#define CLK_CFG_CTRL_KEY (0x80)
#define CLK_SEL_PLL      BIT(1)
#define CLK_SEL_OSC      (0)

struct ch56x_clkmux_config {
	uint32_t hclk_freq;
};

static int ch56x_clkmux_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static int ch56x_clkmux_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static int ch56x_clkmux_get_rate(const struct device *dev, clock_control_subsys_t sys,
				 uint32_t *rate)
{
	const struct ch56x_clkmux_config *cfg = dev->config;

	ARG_UNUSED(sys);

	*rate = cfg->hclk_freq;

	return 0;
}

static enum clock_control_status ch56x_clkmux_get_status(const struct device *dev,
							 clock_control_subsys_t sys)
{
	return CLOCK_CONTROL_STATUS_ON;
}

static int ch56x_clkmux_init(const struct device *dev)
{
	const struct ch56x_clkmux_config *cfg = dev->config;
	uint32_t source, divider;

	switch (cfg->hclk_freq) {
	case MHZ(2):
		source = CLK_SEL_OSC;
		/* For 2MHz, divider is 0 */
		divider = 0;
		break;
	case MHZ(15):
		source = CLK_SEL_OSC;
		divider = 2;
		break;
	case MHZ(30):
		source = CLK_SEL_PLL;
		/* For 30MHz, divider is 0 */
		divider = 0;
		break;
	case MHZ(60):
		source = CLK_SEL_PLL;
		divider = 8;
		break;
	case MHZ(80):
		source = CLK_SEL_PLL;
		divider = 6;
		break;
	case MHZ(96):
		source = CLK_SEL_PLL;
		divider = 5;
		break;
	case MHZ(120):
		source = CLK_SEL_PLL;
		divider = 4;
		break;
	default:
		return -EINVAL;
	}

	/* Configure HCLK source and divider */
	ch32v_sys_unlock();
	sys_write8(CLK_PLL_DIV_KEY | CLK_PLL_DIV(divider), CH32V_SYS_R8_CLK_PLL_DIV_REG);

	ch32v_sys_unlock();
	sys_write8(CLK_CFG_CTRL_KEY | source, CH32V_SYS_R8_CLK_CFG_CTRL_REG);

	ch32v_sys_relock();

	return 0;
}

static const struct clock_control_driver_api ch56x_clkmux_api = {
	.on = ch56x_clkmux_on,
	.off = ch56x_clkmux_off,
	.get_rate = ch56x_clkmux_get_rate,
	.get_status = ch56x_clkmux_get_status,
};

#define CH56X_CLKMUX_INST(n)                                                                       \
	static const struct ch56x_clkmux_config ch56x_clkmux_cfg_##n = {                           \
		.hclk_freq = DT_INST_PROP(n, clock_frequency),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ch56x_clkmux_init, NULL, NULL, &ch56x_clkmux_cfg_##n,             \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                    \
			      &ch56x_clkmux_api);

DT_INST_FOREACH_STATUS_OKAY(CH56X_CLKMUX_INST)
