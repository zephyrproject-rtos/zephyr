/*
 * Copyright (c) 2020, Jiri Kerestes
 *
 * Based on wdt_mcux_wdog32.c, which is:
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright 2018, 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_wwdt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/pm/device.h>
#include <fsl_wwdt.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_mcux_wwdt);

#define MIN_TIMEOUT 0xFF
#define MAX_TIMEOUT WWDT_TC_COUNT_MASK

struct mcux_wwdt_config {
	WWDT_Type *base;
	uint8_t clk_divider;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_wwdt_data {
	wdt_callback_t callback;
	wwdt_config_t wwdt_config;
	bool timeout_valid;
	bool active_before_sleep;
};

static inline int mcux_wwdt_get_clock_frequency(const struct device *dev, uint32_t *freq)
{
	const struct mcux_wwdt_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	switch ((uint32_t)config->clock_subsys) {
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wwdt0))
	case MCUX_WWDT0_CLK:
#if defined(CONFIG_SOC_SERIES_MCXW2XX) || defined(CONFIG_SOC_SERIES_LPC55XXX)
		CLOCK_SetClkDiv(kCLOCK_DivWdtClk, config->clk_divider, true);
#elif defined(CONFIG_SOC_FAMILY_MCXA)
		CLOCK_SetClockDiv(kCLOCK_DivWWDT0, config->clk_divider);
#elif defined(CONFIG_SOC_FAMILY_MCXN)
		CLOCK_SetClkDiv(kCLOCK_DivWdt0Clk, config->clk_divider);
#endif
		break;
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wwdt1))
	case MCUX_WWDT1_CLK:
#if defined(CONFIG_SOC_FAMILY_MCXA)
		CLOCK_SetClockDiv(kCLOCK_DivWWDT1, config->clk_divider);
#elif defined(CONFIG_SOC_FAMILY_MCXN)
		CLOCK_SetClkDiv(kCLOCK_DivWdt1Clk, config->clk_divider);
#endif
		break;
#endif
	default:
		break;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, freq);
	if (ret) {
		LOG_ERR("Failed to get clock frequency: %d", ret);
		return ret;
	}

	return 0;
}

static int mcux_wwdt_setup(const struct device *dev, uint8_t options)
{
	const struct mcux_wwdt_config *config = dev->config;
	struct mcux_wwdt_data *data = dev->data;
	WWDT_Type *base = config->base;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	WWDT_Init(base, &data->wwdt_config);
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int mcux_wwdt_disable(const struct device *dev)
{
	const struct mcux_wwdt_config *config = dev->config;
	struct mcux_wwdt_data *data = dev->data;
	WWDT_Type *base = config->base;

	WWDT_Deinit(base);
	data->timeout_valid = false;
	data->active_before_sleep = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

/*
 * LPC55xxx WWDT has a fixed divide-by-4 clock prescaler.
 * This prescaler is different from the clock divider specified in Device Tree.
 */
#define MSEC_TO_WWDT_TICKS(clock_freq, msec) \
	((uint32_t)((clock_freq / MSEC_PER_SEC) * msec) / 4)

static int mcux_wwdt_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	struct mcux_wwdt_data *data = dev->data;
	uint32_t clock_freq;
	int ret;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	ret = mcux_wwdt_get_clock_frequency(dev, &clock_freq);
	if (ret) {
		return ret;
	}

	WWDT_GetDefaultConfig(&data->wwdt_config);

	data->wwdt_config.clockFreq_Hz = clock_freq;

	data->wwdt_config.timeoutValue =
		MSEC_TO_WWDT_TICKS(clock_freq, cfg->window.max);

	if (data->wwdt_config.timeoutValue > MAX_TIMEOUT ||
	    data->wwdt_config.timeoutValue < MIN_TIMEOUT) {
		LOG_ERR("Timeout value %d out of range %d - %d", data->wwdt_config.timeoutValue,
			MIN_TIMEOUT, MAX_TIMEOUT);
		return -EINVAL;
	}

	/*
	 * WWDT uses a count-down counter.
	 * Window value specifies the highest timer value in which a watchdog
	 * feed can occur. Therefore, calculate the window value as:
	 * windowValue = timeoutValue - min_window_ticks (maybe 0)
	 */
	data->wwdt_config.windowValue =
		data->wwdt_config.timeoutValue - MSEC_TO_WWDT_TICKS(clock_freq, cfg->window.min);

	if (cfg->flags & WDT_FLAG_RESET_SOC) {
		data->wwdt_config.enableWatchdogReset = true;
		LOG_DBG("Enabling SoC reset");
	}

	/*
	 * The user callback is only invoked from the WWDT warning interrupt.
	 * If CONFIG_WDT_MCUX_WWDT_WARNING_INTERRUPT_CFG is 0, the warning interrupt
	 * is disabled (no warningValue programmed), so a callback would never fire.
	 * Reject this configuration to avoid a silent no-op.
	 */
	if (cfg->callback) {
		if (CONFIG_WDT_MCUX_WWDT_WARNING_INTERRUPT_CFG > 0) {
			data->callback = cfg->callback;
			data->wwdt_config.warningValue =
				CONFIG_WDT_MCUX_WWDT_WARNING_INTERRUPT_CFG;
		} else {
			LOG_ERR("Warning interrupt callback requires "
				"CONFIG_WDT_MCUX_WWDT_WARNING_INTERRUPT_CFG > 0");
			return -EINVAL;
		}
	}

	data->timeout_valid = true;
	LOG_DBG("Installed timeout (timeoutValue = %d)",
		data->wwdt_config.timeoutValue);

	return 0;
}

static int mcux_wwdt_feed(const struct device *dev, int channel_id)
{
	const struct mcux_wwdt_config *config = dev->config;
	WWDT_Type *base = config->base;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	WWDT_Refresh(base);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static void mcux_wwdt_isr(const struct device *dev)
{
	const struct mcux_wwdt_config *config = dev->config;
	struct mcux_wwdt_data *data = dev->data;
	WWDT_Type *base = config->base;
	uint32_t flags;

	flags = WWDT_GetStatusFlags(base);
	WWDT_ClearStatusFlags(base, flags);

	if (data->callback) {
		data->callback(dev, 0);
	}
}

/*
 * Power Management:
 * When the system enters sleep mode, this driver maintains awareness
 * of whether the WWDT was active. After wake up, the WDT counter
 * resets to its reload value.
 */
static int mcux_wwdt_driver_pm_action(const struct device *dev,
				      enum pm_device_action action)
{
	const struct mcux_wwdt_config *config = dev->config;
	struct mcux_wwdt_data *data = dev->data;
	int err = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		if (data->active_before_sleep) {
			err = mcux_wwdt_setup(dev, 0);
			data->active_before_sleep = false;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		if (config->base->MOD & WWDT_MOD_WDEN_MASK) {
			data->active_before_sleep = true;
		}
		break;
	default:
		err = -ENOTSUP;
	}

	return err;
}

static int mcux_wwdt_init(const struct device *dev)
{
	const struct mcux_wwdt_config *config = dev->config;
	int ret;

	ret = clock_control_configure(config->clock_dev, config->clock_subsys, NULL);
	if (ret && ret != -ENOSYS) {
		/* Real error occurred */
		LOG_ERR("Failed to configure clock: %d", ret);
		return ret;
	}

#if FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret) {
		LOG_ERR("Failed to enable clock: %d", ret);
		return ret;
	}
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

	/* The rest of the device init is done from the
	 * PM_DEVICE_ACTION_TURN_ON in the pm callback
	 * which is invoked by pm_device_driver_init.
	 */
	config->irq_config_func(dev);
	return pm_device_driver_init(dev, mcux_wwdt_driver_pm_action);
}

static DEVICE_API(wdt, mcux_wwdt_api) = {
	.setup = mcux_wwdt_setup,
	.disable = mcux_wwdt_disable,
	.install_timeout = mcux_wwdt_install_timeout,
	.feed = mcux_wwdt_feed,
};

#define MCUX_WWDT_INIT_CONFIG(id)                                                                  \
	static void mcux_wwdt_config_func_##id(const struct device *dev);                          \
                                                                                                   \
	static const struct mcux_wwdt_config mcux_wwdt_config_##id = {                             \
		.base = (WWDT_Type *)DT_INST_REG_ADDR(id),                                         \
		.clk_divider = DT_INST_PROP(id, clk_divider),                                      \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),                               \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),             \
		.irq_config_func = mcux_wwdt_config_func_##id,                                     \
	};                                                                                         \
                                                                                                   \
	static struct mcux_wwdt_data mcux_wwdt_data_##id;                                          \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(id, mcux_wwdt_driver_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &mcux_wwdt_init, PM_DEVICE_DT_INST_GET(id),                      \
			      &mcux_wwdt_data_##id, &mcux_wwdt_config_##id, POST_KERNEL,           \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mcux_wwdt_api);                 \
                                                                                                   \
	static void mcux_wwdt_config_func_##id(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mcux_wwdt_isr,            \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_WWDT_INIT_CONFIG)
