/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ewm

#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
#include <fsl_ewm.h>
LOG_MODULE_REGISTER(wdt_nxp_ewm);

#define NXP_EWM_MAX_TIMEOUT_WINDOW 0xFE

struct nxp_ewm_config {
	EWM_Type *base;
	void (*irq_config_func)(const struct device *dev);
	bool is_input_enabled;
	bool is_input_active_high;
	uint8_t clk_sel;
	uint8_t clk_divider;
};

struct nxp_ewm_data {
	struct wdt_timeout_cfg timeout_cfg;
	bool is_watchdog_setup;
};

static int nxp_ewm_setup(const struct device *dev, uint8_t options)
{
	const struct nxp_ewm_config *config = dev->config;
	struct nxp_ewm_data *data = dev->data;

	if (data->is_watchdog_setup) {
		/* Watchdog cannot be re-configured after enabled. */
		return -EBUSY;
	}

	if (options) {
		/* Unable to halt counter during debugging */
		return -ENOTSUP;
	}

	ewm_config_t ewm_config;

	data->is_watchdog_setup = true;
	EWM_GetDefaultConfig(&ewm_config);
#if defined(FSL_FEATURE_EWM_HAS_PRESCALER) && FSL_FEATURE_EWM_HAS_PRESCALER
	if (config->clk_divider <= 0xFF) {
		ewm_config.prescaler = config->clk_divider;
	} else {
		LOG_ERR("Invalid clock prescaler value: %d", config->clk_divider);
		return -EINVAL;
	}
#endif /* FSL_FEATURE_EWM_HAS_CLOCK_SELECT */
#if defined(FSL_FEATURE_EWM_HAS_CLOCK_SELECT) && FSL_FEATURE_EWM_HAS_CLOCK_SELECT
	if (config->clk_sel <= 3) {
		ewm_config.clockSource = config->clk_sel;
	} else {
		LOG_ERR("Invalid clock select value: %d", config->clk_sel);
		return -EINVAL;
	}
#endif /* FSL_FEATURE_EWM_HAS_CLOCK_SELECT*/
	ewm_config.enableEwmInput = config->is_input_enabled;
	ewm_config.setInputAssertLogic = config->is_input_active_high;
	ewm_config.compareLowValue = data->timeout_cfg.window.min;
	ewm_config.compareHighValue = data->timeout_cfg.window.max;
	ewm_config.enableInterrupt = true;
	EWM_Init(config->base, &ewm_config);

	return 0;
}

static int nxp_ewm_disable(const struct device *dev)
{
	struct nxp_ewm_data *data = dev->data;

	if (!data->is_watchdog_setup) {
		return -EFAULT;
	}

	return -EPERM;
}

static int nxp_ewm_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	struct nxp_ewm_data *data = dev->data;

	if (cfg->flags) {
		return -ENOTSUP;
	}

	if (data->is_watchdog_setup) {
		return -ENOMEM;
	}

	if (cfg && cfg->window.max <= NXP_EWM_MAX_TIMEOUT_WINDOW &&
		cfg->window.min <= cfg->window.max &&
		cfg->window.max > 0) {
		data->timeout_cfg.window = cfg->window;
	} else {
		return -EINVAL;
	}

#if defined(CONFIG_WDT_MULTISTAGE)
	if (cfg->next) {
		return -EINVAL;
	}
#endif

	if (cfg->callback) {
		data->timeout_cfg.callback = cfg->callback;
	}

	return 0;
}

static int nxp_ewm_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);
	const struct nxp_ewm_config *config = dev->config;
	unsigned int key = irq_lock();

	EWM_Refresh(config->base);
	irq_unlock(key);

	return 0;
}

static void nxp_ewm_isr(const struct device *dev)
{
	const struct nxp_ewm_config *config = dev->config;
	struct nxp_ewm_data *data = dev->data;

	EWM_DisableInterrupts(config->base, kEWM_InterruptEnable);

	if (data->timeout_cfg.callback) {
		data->timeout_cfg.callback(dev, 0);
	}
}

static int nxp_ewm_init(const struct device *dev)
{
	const struct nxp_ewm_config *config = dev->config;

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(wdt, nxp_ewm_api) = {
	.setup = nxp_ewm_setup,
	.disable = nxp_ewm_disable,
	.install_timeout = nxp_ewm_install_timeout,
	.feed = nxp_ewm_feed,
};

#define WDT_EWM_INIT(n)							\
	static void nxp_ewm_config_func_##n(const struct device *dev);	\
									\
	static const struct nxp_ewm_config nxp_ewm_config_##n = {	\
		.base = (EWM_Type *)DT_INST_REG_ADDR(n),		\
		.irq_config_func = nxp_ewm_config_func_##n,		\
		.is_input_enabled = DT_INST_PROP(n, input_trigger_en),	\
		.is_input_active_high =					\
			DT_INST_PROP(n, input_trigger_active_high),	\
		.clk_divider = DT_INST_PROP(n, clk_divider),		\
		.clk_sel = DT_INST_PROP_OR(n, clk_sel, 0)		\
	};								\
									\
	static struct nxp_ewm_data nxp_ewm_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			nxp_ewm_init,				        \
			NULL,					        \
			&nxp_ewm_data_##n, &nxp_ewm_config_##n,	        \
			POST_KERNEL,				        \
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			&nxp_ewm_api);					\
									\
	static void nxp_ewm_config_func_##n(const struct device *dev)	\
	{								\
		ARG_UNUSED(dev);					\
									\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    nxp_ewm_isr, DEVICE_DT_INST_GET(n), 0);	\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(WDT_EWM_INIT)
