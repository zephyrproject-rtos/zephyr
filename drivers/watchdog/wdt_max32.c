/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_watchdog

#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <soc.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_max32, CONFIG_WDT_LOG_LEVEL);

#include <wrap_max32_wdt.h>

struct max32_wdt_config {
	mxc_wdt_regs_t *regs;
	const struct device *clock;
	struct max32_perclk perclk;
	void (*irq_func)(void);
};

struct max32_wdt_data {
	struct wdt_window timeout;
	wdt_callback_t callback;
};

static int wdt_max32_calculate_timeout(uint32_t timeout, uint32_t clock_src)
{
	int i;
	uint32_t clk_frequency = 0;
	uint32_t number_of_tick = 0;

	clk_frequency = ADI_MAX32_GET_PRPH_CLK_FREQ(clock_src);
	if (clk_frequency == 0) {
		LOG_ERR("Unsupported clock source.");
		return -ENOTSUP;
	}

	number_of_tick = ((uint64_t)timeout * (uint64_t)clk_frequency) / 1000;

	i = LOG2CEIL(number_of_tick); /* Find closest bigger 2^i value than number_of_tick. */
	i = CLAMP(i, 16, 31);         /* Limit i between 16 and 31. */

	/* It returns 31 - i because period thresholds are inverse ordered in register. */
	return (31 - i);
}

static int wdt_max32_disable(const struct device *dev)
{
	const struct max32_wdt_config *cfg = dev->config;

	if (!(cfg->regs->ctrl & WRAP_MXC_F_WDT_CTRL_EN)) {
		return -EFAULT;
	}

	MXC_WDT_Disable(cfg->regs);
	return 0;
}

static int wdt_max32_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);
	const struct max32_wdt_config *cfg = dev->config;

	MXC_WDT_ResetTimer(cfg->regs);
	return 0;
}

static int wdt_max32_setup(const struct device *dev, uint8_t options)
{
	const struct max32_wdt_config *cfg = dev->config;

	if (cfg->regs->ctrl & WRAP_MXC_F_WDT_CTRL_EN) {
		return -EBUSY;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	MXC_WDT_ResetTimer(cfg->regs);
	MXC_WDT_Enable(cfg->regs);
	return 0;
}

static int wdt_max32_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	int ret = 0;
	const struct max32_wdt_config *dev_cfg = dev->config;
	struct max32_wdt_data *data = dev->data;
	mxc_wdt_regs_t *regs = dev_cfg->regs;
	wrap_mxc_wdt_cfg_t wdt_cfg;

	if (cfg->window.max == 0U) {
		return -EINVAL;
	}

	if (regs->ctrl & WRAP_MXC_F_WDT_CTRL_EN) {
		return -EBUSY;
	}

	data->timeout = cfg->window;
	data->callback = cfg->callback;

	/* Default values to eliminate warnings */
	wdt_cfg.mode = MXC_WDT_COMPATIBILITY;
	wdt_cfg.upperResetPeriod = 0;
	wdt_cfg.lowerResetPeriod = 0;
	wdt_cfg.upperIntPeriod = 0;
	wdt_cfg.lowerIntPeriod = 0;

	if (data->timeout.min > 0) {
		wdt_cfg.mode = MXC_WDT_WINDOWED;

		ret = Wrap_MXC_WDT_Init(regs, &wdt_cfg);
		if (ret != E_NO_ERROR) {
			LOG_DBG("%s does not support windowed mode.", CONFIG_BOARD);
			return -EINVAL;
		}

		int lower_timeout_period =
			wdt_max32_calculate_timeout(data->timeout.min, dev_cfg->perclk.clk_src);
		if (lower_timeout_period == -ENOTSUP) {
			return -EINVAL;
		}

		if (data->callback == NULL) {
			wdt_cfg.lowerResetPeriod = lower_timeout_period;
			wdt_cfg.lowerIntPeriod = lower_timeout_period; /* Not used */
		} else {
			switch (lower_timeout_period) {
			case MXC_WDT_PERIOD_2_16: /* Min timeout */
				wdt_cfg.lowerResetPeriod = MXC_WDT_PERIOD_2_17;
				wdt_cfg.lowerIntPeriod = MXC_WDT_PERIOD_2_16;
				break;
			default:
				/* Generate interrupt just before reset */
				wdt_cfg.lowerResetPeriod = lower_timeout_period;
				/* +1 means one steps before */
				wdt_cfg.lowerIntPeriod = lower_timeout_period + 1;
				break;
			}
		}
	}

	int upper_timeout_period =
		wdt_max32_calculate_timeout(data->timeout.max, dev_cfg->perclk.clk_src);
	if (upper_timeout_period == -ENOTSUP) {
		return -EINVAL;
	}

	if (data->callback == NULL) {
		wdt_cfg.upperResetPeriod = upper_timeout_period;
		wdt_cfg.upperIntPeriod = upper_timeout_period; /* Not used */
	} else {
		switch (upper_timeout_period) {
		case MXC_WDT_PERIOD_2_16: /* Min timeout */
			wdt_cfg.upperResetPeriod = MXC_WDT_PERIOD_2_17;
			wdt_cfg.upperIntPeriod = MXC_WDT_PERIOD_2_16;
			break;
		default:
			/* Generate interrupt just before reset */
			wdt_cfg.upperResetPeriod = upper_timeout_period;
			/* +1 means one steps before */
			wdt_cfg.upperIntPeriod = upper_timeout_period + 1;
			break;
		}
	}

	Wrap_MXC_WDT_SetResetPeriod(regs, &wdt_cfg);

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		MXC_WDT_EnableReset(regs);
		LOG_DBG("Configuring reset SOC mode.");
		break;

	case WDT_FLAG_RESET_NONE:
		MXC_WDT_DisableReset(regs);
		LOG_DBG("Configuring non-reset mode.");
		break;

	default:
		LOG_ERR("Unsupported watchdog config flag.");
		return -ENOTSUP;
	}

	/* If callback is not null, enable interrupt. */
	if (data->callback) {
		Wrap_MXC_WDT_SetIntPeriod(regs, &wdt_cfg);
		MXC_WDT_EnableInt(regs);
	}

	return ret;
}

static void wdt_max32_isr(const void *param)
{
	const struct device *dev = (const struct device *)param;
	const struct max32_wdt_config *cfg = dev->config;
	struct max32_wdt_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, 0);
	}

	MXC_WDT_ClearIntFlag(cfg->regs);
}

static int wdt_max32_init(const struct device *dev)
{
	int ret = 0;
	const struct max32_wdt_config *cfg = dev->config;
	mxc_wdt_regs_t *regs = cfg->regs;

	/* Enable clock */
	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret) {
		return ret;
	}

	ret = Wrap_MXC_WDT_SelectClockSource(regs, cfg->perclk.clk_src);
	if (ret != E_NO_ERROR) {
		LOG_ERR("WDT instance does not support given clock source.");
		return -ENOTSUP;
	}

	/* Disable all actions */
	MXC_WDT_Disable(regs);
	MXC_WDT_DisableReset(regs);
	MXC_WDT_DisableInt(regs);
	MXC_WDT_ClearResetFlag(regs);
	MXC_WDT_ClearIntFlag(regs);

	cfg->irq_func(); /* WDT IRQ enable*/

	return 0;
}

static DEVICE_API(wdt, max32_wdt_api) = {
	.setup = wdt_max32_setup,
	.disable = wdt_max32_disable,
	.install_timeout = wdt_max32_install_timeout,
	.feed = wdt_max32_feed,
};

#define MAX32_WDT_INIT(_num)                                                                       \
	static void wdt_max32_irq_init_##_num(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(_num), DT_INST_IRQ(_num, priority), wdt_max32_isr,        \
			    DEVICE_DT_INST_GET(_num), 0);                                          \
		irq_enable(DT_INST_IRQN(_num));                                                    \
	}                                                                                          \
	static struct max32_wdt_data max32_wdt_data##_num;                                         \
	static const struct max32_wdt_config max32_wdt_config##_num = {                            \
		.regs = (mxc_wdt_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.clk_src =                                                                  \
			DT_INST_PROP_OR(_num, clock_source, ADI_MAX32_PRPH_CLK_SRC_PCLK),          \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		.irq_func = &wdt_max32_irq_init_##_num,                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(_num, wdt_max32_init, NULL, &max32_wdt_data##_num,                   \
			      &max32_wdt_config##_num, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &max32_wdt_api);

DT_INST_FOREACH_STATUS_OKAY(MAX32_WDT_INIT)
