/*
 * Copyright (c) 2020, Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc11u6x_syscon

#include <devicetree.h>
#include <device.h>

#include <drivers/clock_control/lpc11u6x_clock_control.h>
#include <drivers/pinmux.h>

#include "clock_control_lpc11u6x.h"

#define DEV_CFG(dev)  ((const struct lpc11u6x_syscon_config *) \
		      ((dev)->config))

#define DEV_DATA(dev) ((struct lpc11u6x_syscon_data *) \
		      ((dev)->data))

static void syscon_power_up(struct lpc11u6x_syscon_regs *syscon,
			    uint32_t bit, bool enable)
{
	if (enable) {
		syscon->pd_run_cfg = (syscon->pd_run_cfg & ~bit)
			| LPC11U6X_PDRUNCFG_MASK;
	} else {
		syscon->pd_run_cfg = syscon->pd_run_cfg | bit
			| LPC11U6X_PDRUNCFG_MASK;
	}
}

static void syscon_set_pll_src(struct lpc11u6x_syscon_regs *syscon,
			       uint32_t src)
{
	syscon->sys_pll_clk_sel = src;
	syscon->sys_pll_clk_uen = 0;
	syscon->sys_pll_clk_uen = 1;
}

static void set_flash_access_time(uint32_t nr_cycles)
{
	uint32_t *reg = (uint32_t *) LPC11U6X_FLASH_TIMING_REG;

	*reg = (*reg & (~LPC11U6X_FLASH_TIMING_MASK)) | nr_cycles;
}

static void syscon_setup_pll(struct lpc11u6x_syscon_regs *syscon,
			     uint32_t msel, uint32_t psel)
{
	uint32_t val = msel & LPC11U6X_SYS_PLL_CTRL_MSEL_MASK;

	val |= (psel & LPC11U6X_SYS_PLL_CTRL_PSEL_MASK) <<
		LPC11U6X_SYS_PLL_CTRL_PSEL_SHIFT;
	syscon->sys_pll_ctrl = val;
}

static bool syscon_pll_locked(struct lpc11u6x_syscon_regs *syscon)
{
	return (syscon->sys_pll_stat & 0x1) != 0;
}

static void syscon_set_main_clock_source(struct lpc11u6x_syscon_regs *syscon,
					 uint32_t src)
{
	syscon->main_clk_sel = src;
	syscon->main_clk_uen = 0;
	syscon->main_clk_uen = 1;
}

static void syscon_ahb_clock_enable(struct lpc11u6x_syscon_regs *syscon,
				    uint32_t mask, bool enable)
{
	if (enable) {
		syscon->sys_ahb_clk_ctrl |= mask;
	} else {
		syscon->sys_ahb_clk_ctrl &= ~mask;
	}
}

#if defined(CONFIG_CLOCK_CONTROL_LPC11U6X_PLL_SRC_SYSOSC) \
	&& DT_INST_NODE_HAS_PROP(0, pinmuxs)
/**
 * @brief: configure system oscillator pins.
 *
 * This system oscillator pins and their configurations are retrieved from the
 * "pinmuxs" property of the DT clock controller node.
 */
static void pinmux_enable_sysosc(void)
{
	const struct device *pinmux_dev;
	uint32_t pin, func;

	pinmux_dev = device_get_binding(
		DT_LABEL(DT_INST_PHANDLE_BY_NAME(0, pinmuxs, xtalin)));
	if (!pinmux_dev) {
		return;
	}
	pin = DT_INST_PHA_BY_NAME(0, pinmuxs, xtalin, pin);
	func = DT_INST_PHA_BY_NAME(0, pinmuxs, xtalin, function);

	pinmux_pin_set(pinmux_dev, pin, func);

	pinmux_dev = device_get_binding(
		DT_LABEL(DT_INST_PHANDLE_BY_NAME(0, pinmuxs, xtalout)));
	if (!pinmux_dev) {
		return;
	}
	pin = DT_INST_PHA_BY_NAME(0, pinmuxs, xtalout, pin);
	func = DT_INST_PHA_BY_NAME(0, pinmuxs, xtalout, function);

	pinmux_pin_set(pinmux_dev, pin, func);
}
#else
#define pinmux_enable_sysosc() do { } while (0)
#endif

static void syscon_peripheral_reset(struct lpc11u6x_syscon_regs *syscon,
				    uint32_t mask, bool reset)
{
	if (reset) {
		syscon->p_reset_ctrl &= ~mask;
	} else {
		syscon->p_reset_ctrl |= mask;
	}
}
static void syscon_frg_init(struct lpc11u6x_syscon_regs *syscon)
{
	uint32_t div;

	div = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / LPC11U6X_USART_CLOCK_RATE;
	if (!div) {
		div = 1;
	}
	syscon->frg_clk_div = div;

	syscon_peripheral_reset(syscon, LPC11U6X_PRESET_CTRL_FRG, false);
	syscon->uart_frg_div = 0xFF;
	syscon->uart_frg_mult = ((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / div)
				 * 256) / LPC11U6X_USART_CLOCK_RATE;
}

static void syscon_frg_deinit(struct lpc11u6x_syscon_regs *syscon)
{
	syscon->uart_frg_div = 0x0;
	syscon_peripheral_reset(syscon, LPC11U6X_PRESET_CTRL_FRG, true);
}

static int lpc11u6x_clock_control_on(const struct device *dev,
				     clock_control_subsys_t sub_system)
{
	const struct lpc11u6x_syscon_config *cfg = DEV_CFG(dev);
	struct lpc11u6x_syscon_data *data = DEV_DATA(dev);
	uint32_t clk_mask = 0, reset_mask = 0;
	int ret = 0, init_frg = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	switch ((int) sub_system) {
	case LPC11U6X_CLOCK_I2C0:
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_I2C0;
		reset_mask = LPC11U6X_PRESET_CTRL_I2C0;
		break;
	case LPC11U6X_CLOCK_I2C1:
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_I2C1;
		reset_mask = LPC11U6X_PRESET_CTRL_I2C1;
		break;
	case LPC11U6X_CLOCK_GPIO:
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_GPIO |
			LPC11U6X_SYS_AHB_CLK_CTRL_PINT;
		break;
	case LPC11U6X_CLOCK_USART0:
		cfg->syscon->usart0_clk_div = 1;
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART0;
		break;
	case LPC11U6X_CLOCK_USART1:
		if (!data->frg_in_use++) {
			init_frg = 1;
		}
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART1;
		reset_mask = LPC11U6X_PRESET_CTRL_USART1;
		break;
	case LPC11U6X_CLOCK_USART2:
		if (!data->frg_in_use++) {
			init_frg = 1;
		}
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART2;
		reset_mask = LPC11U6X_PRESET_CTRL_USART2;
		break;
	case LPC11U6X_CLOCK_USART3:
		if (!data->frg_in_use++) {
			init_frg = 1;
		}
		data->usart34_in_use++;
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART3_4;
		reset_mask = LPC11U6X_PRESET_CTRL_USART3;
		break;
	case LPC11U6X_CLOCK_USART4:
		if (!data->frg_in_use++) {
			init_frg = 1;
		}
		data->usart34_in_use++;
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART3_4;
		reset_mask = LPC11U6X_PRESET_CTRL_USART4;
		break;
	default:
		k_mutex_unlock(&data->mutex);
		return -EINVAL;
	}

	syscon_ahb_clock_enable(cfg->syscon, clk_mask, true);
	if (init_frg) {
		syscon_frg_init(cfg->syscon);
	}
	syscon_peripheral_reset(cfg->syscon, reset_mask, false);
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int lpc11u6x_clock_control_off(const struct device *dev,
				      clock_control_subsys_t sub_system)
{
	const struct lpc11u6x_syscon_config *cfg = DEV_CFG(dev);
	struct lpc11u6x_syscon_data *data = DEV_DATA(dev);
	uint32_t clk_mask = 0, reset_mask = 0;
	int ret = 0, deinit_frg = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	switch ((int) sub_system) {
	case LPC11U6X_CLOCK_I2C0:
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_I2C0;
		reset_mask = LPC11U6X_PRESET_CTRL_I2C0;
		break;
	case LPC11U6X_CLOCK_I2C1:
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_I2C1;
		reset_mask = LPC11U6X_PRESET_CTRL_I2C1;
		break;
	case LPC11U6X_CLOCK_GPIO:
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_GPIO |
			LPC11U6X_SYS_AHB_CLK_CTRL_PINT;
		break;
	case LPC11U6X_CLOCK_USART0:
		cfg->syscon->usart0_clk_div = 0;
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART0;
		break;
	case LPC11U6X_CLOCK_USART1:
		if (!(--data->frg_in_use)) {
			deinit_frg = 1;
		}
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART1;
		reset_mask = LPC11U6X_PRESET_CTRL_USART1;
		break;
	case LPC11U6X_CLOCK_USART2:
		if (!(--data->frg_in_use)) {
			deinit_frg = 1;
		}
		clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART2;
		reset_mask = LPC11U6X_PRESET_CTRL_USART2;
		break;
	case LPC11U6X_CLOCK_USART3:
		if (!(--data->frg_in_use)) {
			deinit_frg = 1;
		}
		if (!(--data->usart34_in_use)) {
			clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART3_4;
		}
		reset_mask = LPC11U6X_PRESET_CTRL_USART3;
		break;
	case LPC11U6X_CLOCK_USART4:
		if (!(--data->frg_in_use)) {
			deinit_frg = 1;
		}
		if (!(--data->usart34_in_use)) {
			clk_mask = LPC11U6X_SYS_AHB_CLK_CTRL_USART3_4;
		}
		reset_mask = LPC11U6X_PRESET_CTRL_USART4;
		break;
	default:
		k_mutex_unlock(&data->mutex);
		return -EINVAL;
	}

	syscon_ahb_clock_enable(cfg->syscon, clk_mask, false);
	if (deinit_frg) {
		syscon_frg_deinit(cfg->syscon);
	}
	syscon_peripheral_reset(cfg->syscon, reset_mask, true);
	k_mutex_unlock(&data->mutex);
	return ret;

}

static int lpc11u6x_clock_control_get_rate(const struct device *dev,
					   clock_control_subsys_t sub_system,
					   uint32_t *rate)
{
	switch ((int) sub_system) {
	case LPC11U6X_CLOCK_I2C0:
	case LPC11U6X_CLOCK_I2C1:
	case LPC11U6X_CLOCK_GPIO:
	case LPC11U6X_CLOCK_USART0:
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
		break;
	case LPC11U6X_CLOCK_USART1:
	case LPC11U6X_CLOCK_USART2:
	case LPC11U6X_CLOCK_USART3:
	case LPC11U6X_CLOCK_USART4:
		*rate = LPC11U6X_USART_CLOCK_RATE;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int lpc11u6x_syscon_init(const struct device *dev)
{
	const struct lpc11u6x_syscon_config *cfg = DEV_CFG(dev);
	struct lpc11u6x_syscon_data *data = DEV_DATA(dev);
	uint32_t val;

	k_mutex_init(&data->mutex);
	data->frg_in_use = 0;
	data->usart34_in_use = 0;
	/* Enable SRAM1 and USB ram if needed */
	val = 0;
#ifdef CONFIG_CLOCK_CONTROL_LPC11U6X_ENABLE_SRAM1
	val |= LPC11U6X_SYS_AHB_CLK_CTRL_SRAM1;
#endif /* CONFIG_CLOCK_CONTROL_LPC11U6X_ENABLE_SRAM1 */
#ifdef CONFIG_CLOCK_CONTROL_LPC11U6X_ENABLE_USB_RAM
	val |= LPC11U6X_SYS_AHB_CLK_CTRL_USB_SRAM;
#endif /* CONFIG_CLOCK_CONTROL_LPC11U6X_ENABLE_USB_RAM */

	/* Enable IOCON (I/O Control) clock. */
	val |= LPC11U6X_SYS_AHB_CLK_CTRL_IOCON;

	syscon_ahb_clock_enable(cfg->syscon, val, true);

	/* Configure PLL output as the main clock source, with a frequency of
	 * 48MHz
	 */
#ifdef CONFIG_CLOCK_CONTROL_LPC11U6X_PLL_SRC_SYSOSC
	syscon_power_up(cfg->syscon, LPC11U6X_PDRUNCFG_SYSOSC_PD, true);

	/* Wait ~500us */
	for (int i = 0; i < 2500; i++) {
	}

	/* Configure PLL input */
	syscon_set_pll_src(cfg->syscon, LPC11U6X_SYS_PLL_CLK_SEL_SYSOSC);

	pinmux_enable_sysosc();

#elif defined(CONFIG_CLOCK_CONTROL_LPC11U6X_PLL_SRC_IRC)
	syscon_power_up(cfg->syscon, LPC11U6X_PDRUNCFG_IRC_PD, true);
	syscon_set_pll_src(cfg->syscon, LPC11U6X_SYS_PLL_CLK_SEL_IRC);
#endif
	/* Flash access takes 3 clock cycles for main clock frequencies
	 * between 40MHz and 50MHz
	 */
	set_flash_access_time(LPC11U6X_FLASH_TIMING_3CYCLES);

	/* Shutdown PLL to change divider/mult ratios */
	syscon_power_up(cfg->syscon, LPC11U6X_PDRUNCFG_PLL_PD, false);

	/* Setup PLL to have 48MHz output */
	syscon_setup_pll(cfg->syscon, 3, 1);

	/* Power up pll and wait */
	syscon_power_up(cfg->syscon, LPC11U6X_PDRUNCFG_PLL_PD, true);

	while (!syscon_pll_locked(cfg->syscon)) {
	}

	cfg->syscon->sys_ahb_clk_div = 1;
	syscon_set_main_clock_source(cfg->syscon, LPC11U6X_MAIN_CLK_SRC_PLLOUT);
	return 0;
}

static const struct clock_control_driver_api lpc11u6x_clock_control_api = {
	.on = lpc11u6x_clock_control_on,
	.off = lpc11u6x_clock_control_off,
	.get_rate = lpc11u6x_clock_control_get_rate,
};

static const struct lpc11u6x_syscon_config syscon_config = {
	.syscon = (struct lpc11u6x_syscon_regs *) DT_INST_REG_ADDR(0),
};

static struct lpc11u6x_syscon_data syscon_data;

DEVICE_DT_INST_DEFINE(0,
		    &lpc11u6x_syscon_init,
		    device_pm_control_nop,
		    &syscon_data, &syscon_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		    &lpc11u6x_clock_control_api);
