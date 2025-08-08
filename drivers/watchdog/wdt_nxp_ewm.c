/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ewm

#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_nxp_ewm);

#define NXP_EWM_FEED_MAGIC_NUMBER 0x2CB4
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
	EWM_Type *base = config->base;

	if (data->is_watchdog_setup) {
		/* Watchdog cannot be re-configured after enabled. */
		return -EBUSY;
	}

	if (options) {
		/* Unable to halt counter during debugging */
		return -ENOTSUP;
	}

	data->is_watchdog_setup = true;
	base->CMPL = EWM_CMPL_COMPAREL(data->timeout_cfg.window.min);
	base->CMPH = EWM_CMPH_COMPAREH(data->timeout_cfg.window.max);

	/*
	 * base->CTRL should be the last thing touched due to
	 * the small watchdog window time.
	 * After this write, only the INTEN bit is writable until reset.
	 *
	 * EWM_CTRL_INTEN enables the interrupt signal
	 * EWM_CTRL_EWMEN enables the watchdog.
	 */
	base->CTRL |= EWM_CTRL_INEN(config->is_input_enabled)	|
		EWM_CTRL_ASSIN(config->is_input_active_high)	|
		EWM_CTRL_INTEN(1) | EWM_CTRL_EWMEN(1);

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
		cfg->window.max > 0 &&
		cfg->window.min >= 0) {
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
	EWM_Type *base = config->base;
	unsigned int key = irq_lock();

	base->SERV = EWM_SERV_SERVICE(NXP_EWM_FEED_MAGIC_NUMBER);
	base->SERV = EWM_SERV_SERVICE((uint8_t)(NXP_EWM_FEED_MAGIC_NUMBER >> 8));
	irq_unlock(key);

	return 0;
}

static void nxp_ewm_isr(const struct device *dev)
{
	const struct nxp_ewm_config *config = dev->config;
	struct nxp_ewm_data *data = dev->data;
	EWM_Type *base = config->base;

	base->CTRL &= (~EWM_CTRL_INTEN_MASK);

	if (data->timeout_cfg.callback) {
		data->timeout_cfg.callback(dev, 0);
	}
}

static int nxp_ewm_init(const struct device *dev)
{
	const struct nxp_ewm_config *config = dev->config;
	EWM_Type *base = config->base;

#if DT_INST_NODE_HAS_PROP(0, clk_sel)
	/* Set clock select value for CLKCTRL register */
	switch (config->clk_sel) {
	case 0:
		base->CLKCTRL = EWM_CLKCTRL_CLKSEL(0);
		break;
	case 1:
		base->CLKCTRL = EWM_CLKCTRL_CLKSEL(1);
		break;
	case 2:
		base->CLKCTRL = EWM_CLKCTRL_CLKSEL(2);
		break;
	case 3:
		base->CLKCTRL = EWM_CLKCTRL_CLKSEL(3);
		break;
	default:
		LOG_ERR("Invalid clock select value: %d", config->clk_sel);
		return -EINVAL;
	}
#endif /* DT_INST_NODE_HAS_PROP(0, clk_sel) */

#if DT_INST_NODE_HAS_PROP(0, clk_divider)
	if (config->clk_divider >= 0 && config->clk_divider <= 0xFF) {
		base->CLKPRESCALER = EWM_CLKPRESCALER_CLK_DIV(config->clk_divider);
	}
#endif /* DT_INST_NODE_HAS_PROP(0, clk_divider) */
	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(wdt, nxp_ewm_api) = {
	.setup = nxp_ewm_setup,
	.disable = nxp_ewm_disable,
	.install_timeout = nxp_ewm_install_timeout,
	.feed = nxp_ewm_feed,
};

#define CLK_SEL_DEFAULT 0
#define EWM_CONFIG_CLK_SEL_INIT(n)\
	.clk_sel = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clk_sel),	\
					(DT_INST_ENUM_IDX(n, clk_sel)),	\
					(CLK_SEL_DEFAULT)),
#define WDT_EWM_INIT(n)							\
	static void nxp_ewm_config_func_##n(const struct device *dev);	\
									\
	static const struct nxp_ewm_config nxp_ewm_config_##n = {	\
		.base = (EWM_Type *)DT_INST_REG_ADDR(n),		\
		.irq_config_func = nxp_ewm_config_func_##n,		\
		.is_input_enabled = DT_INST_PROP(n, input_trigger_en),	\
		.is_input_active_high =					\
			DT_INST_PROP(n, input_trigger_active_high),	\
		EWM_CONFIG_CLK_SEL_INIT(n)	\
		.clk_divider = DT_INST_PROP_OR(n, clk_divider, 0),	\
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
