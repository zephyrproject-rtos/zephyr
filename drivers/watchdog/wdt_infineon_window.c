/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_window_wdt

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(wdt_infineon_window, CONFIG_WDT_LOG_LEVEL);

#define IFX_WWDT_CTL         0x00U
#define IFX_WWDT_LOWER_LIMIT 0x04U
#define IFX_WWDT_UPPER_LIMIT 0x08U
#define IFX_WWDT_WARN_LIMIT  0x0CU
#define IFX_WWDT_CONFIG      0x10U
#define IFX_WWDT_CNT         0x14U
#define IFX_WWDT_LOCK        0x40U
#define IFX_WWDT_SERVICE     0x44U
#define IFX_WWDT_INTR        0x50U
#define IFX_WWDT_INTR_MASK   0x58U
#define IFX_WWDT_INTR_MASKED 0x5CU

#define IFX_WWDT_CTL_ENABLED BIT(0)
#define IFX_WWDT_CTL_ENABLE  BIT(31)

#define IFX_WWDT_CONFIG_LOWER_ACTION BIT(0)
#define IFX_WWDT_CONFIG_UPPER_ACTION BIT(4)
#define IFX_WWDT_CONFIG_WARN_ACTION  BIT(8)
#define IFX_WWDT_CONFIG_DEBUG_RUN    BIT(31)

#define IFX_WWDT_LOCK_CLR0  1U
#define IFX_WWDT_LOCK_CLR1  2U
#define IFX_WWDT_LOCK_SET01 3U

#define IFX_WWDT_SERVICE_BIT BIT(0)
#define IFX_WWDT_INTR_WDT    BIT(0)

/*
 * TRM, WDT_CTL.ENABLED: "May lag ENABLE by up to three clk_ilo0 cycles."
 * One extra cycle covers ILO0 running below its nominal rate.
 */
#define IFX_WWDT_SYNC_CYCLES 4U

struct ifx_wwdt_limits {
	uint32_t lower;
	uint32_t warn;
	uint32_t upper;
	uint32_t actions;
};

struct ifx_wwdt_config {
	mem_addr_t base;
	const struct device *clk_dev;
	void (*irq_config)(void);
};

struct ifx_wwdt_data {
	struct k_spinlock lock;
	struct ifx_wwdt_limits limits;
	wdt_callback_t callback;
	uint32_t rate;
	uint32_t sync_us;
	bool installed;
};

static void ifx_wwdt_unlock(const struct device *dev)
{
	const struct ifx_wwdt_config *config = dev->config;

	sys_write32(IFX_WWDT_LOCK_CLR0, config->base + IFX_WWDT_LOCK);
	sys_write32(IFX_WWDT_LOCK_CLR1, config->base + IFX_WWDT_LOCK);
}

static void ifx_wwdt_lock(const struct device *dev)
{
	const struct ifx_wwdt_config *config = dev->config;

	sys_write32(IFX_WWDT_LOCK_SET01, config->base + IFX_WWDT_LOCK);
}

static bool ifx_wwdt_is_enabled(const struct device *dev)
{
	const struct ifx_wwdt_config *config = dev->config;

	return (sys_read32(config->base + IFX_WWDT_CTL) & IFX_WWDT_CTL_ENABLE) != 0U;
}

static void ifx_wwdt_ctl_enable(const struct device *dev)
{
	const struct ifx_wwdt_config *config = dev->config;
	struct ifx_wwdt_data *data = dev->data;

	sys_write32(IFX_WWDT_CTL_ENABLE, config->base + IFX_WWDT_CTL);

	(void)WAIT_FOR((sys_read32(config->base + IFX_WWDT_CTL) & IFX_WWDT_CTL_ENABLED) != 0U,
		       data->sync_us, NULL);
}

static void ifx_wwdt_ctl_disable(const struct device *dev)
{
	const struct ifx_wwdt_config *config = dev->config;
	struct ifx_wwdt_data *data = dev->data;

	sys_write32(0U, config->base + IFX_WWDT_CTL);

	(void)WAIT_FOR((sys_read32(config->base + IFX_WWDT_CTL) & IFX_WWDT_CTL_ENABLED) == 0U,
		       data->sync_us, NULL);
}

static int ifx_wwdt_ms_to_ticks(const struct ifx_wwdt_data *data, uint32_t ms, bool round_up,
				uint32_t *ticks)
{
	uint64_t count = (uint64_t)ms * data->rate;

	count = round_up ? DIV_ROUND_UP(count, MSEC_PER_SEC) : (count / MSEC_PER_SEC);
	if (count > UINT32_MAX) {
		return -EINVAL;
	}

	*ticks = (uint32_t)count;

	return 0;
}

static const struct wdt_timeout_cfg *ifx_wwdt_next_stage(const struct wdt_timeout_cfg *cfg)
{
#ifdef CONFIG_WDT_MULTISTAGE
	return cfg->next;
#else
	ARG_UNUSED(cfg);

	return NULL;
#endif
}

static int ifx_wwdt_two_stage(const struct ifx_wwdt_data *data,
			      const struct wdt_timeout_cfg *warn_stage,
			      const struct wdt_timeout_cfg *reset_stage,
			      struct ifx_wwdt_limits *lim)
{
	int ret;

	if ((warn_stage->flags != WDT_FLAG_RESET_NONE) || (warn_stage->callback == NULL) ||
	    (reset_stage->flags != WDT_FLAG_RESET_SOC) || (reset_stage->window.min != 0U) ||
	    (reset_stage->callback != NULL) || (ifx_wwdt_next_stage(reset_stage) != NULL)) {
		return -ENOTSUP;
	}

	ret = ifx_wwdt_ms_to_ticks(data, warn_stage->window.min, false, &lim->lower);
	if (ret != 0) {
		return ret;
	}

	ret = ifx_wwdt_ms_to_ticks(data, warn_stage->window.max, false, &lim->warn);
	if (ret != 0) {
		return ret;
	}

	ret = ifx_wwdt_ms_to_ticks(data, reset_stage->window.max, true, &lim->upper);
	if (ret != 0) {
		return ret;
	}

	if ((lim->lower > lim->warn) || (lim->warn >= lim->upper)) {
		return -EINVAL;
	}

	lim->actions = IFX_WWDT_CONFIG_WARN_ACTION | IFX_WWDT_CONFIG_UPPER_ACTION;

	return 0;
}

static int ifx_wwdt_single_stage(const struct ifx_wwdt_data *data,
				 const struct wdt_timeout_cfg *cfg, struct ifx_wwdt_limits *lim)
{
	int ret;

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		if (cfg->callback != NULL) {
			return -ENOTSUP;
		}

		ret = ifx_wwdt_ms_to_ticks(data, cfg->window.min, false, &lim->lower);
		if (ret != 0) {
			return ret;
		}

		ret = ifx_wwdt_ms_to_ticks(data, cfg->window.max, true, &lim->upper);
		if (ret != 0) {
			return ret;
		}

		lim->actions = IFX_WWDT_CONFIG_UPPER_ACTION;

		return 0;

	case WDT_FLAG_RESET_NONE:
		if (cfg->window.min != 0U) {
			return -ENOTSUP;
		}

		ret = ifx_wwdt_ms_to_ticks(data, cfg->window.max, true, &lim->warn);
		if (ret != 0) {
			return ret;
		}

		lim->lower = 0U;
		lim->upper = UINT32_MAX;
		lim->actions = (cfg->callback != NULL) ? IFX_WWDT_CONFIG_WARN_ACTION : 0U;

		return 0;

	default:
		return -ENOTSUP;
	}
}

static int ifx_wwdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct ifx_wwdt_config *config = dev->config;
	struct ifx_wwdt_data *data = dev->data;
	const struct wdt_timeout_cfg *reset_stage = ifx_wwdt_next_stage(cfg);
	struct ifx_wwdt_limits lim = {0};
	k_spinlock_key_t key;
	int ret;

	if ((cfg->window.max == 0U) || (cfg->window.min > cfg->window.max)) {
		return -EINVAL;
	}

	if ((cfg->callback != NULL) && (config->irq_config == NULL)) {
		return -ENOTSUP;
	}

	if (reset_stage != NULL) {
		ret = ifx_wwdt_two_stage(data, cfg, reset_stage, &lim);
	} else {
		ret = ifx_wwdt_single_stage(data, cfg, &lim);
	}

	if (ret != 0) {
		return ret;
	}

	if (lim.lower > 0U) {
		lim.actions |= IFX_WWDT_CONFIG_LOWER_ACTION;
	}

	key = k_spin_lock(&data->lock);

	if (ifx_wwdt_is_enabled(dev)) {
		ret = -EBUSY;
	} else if (data->installed) {
		ret = -ENOMEM;
	} else {
		data->limits = lim;
		data->callback = cfg->callback;
		data->installed = true;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_wwdt_setup(const struct device *dev, uint8_t options)
{
	const struct ifx_wwdt_config *config = dev->config;
	struct ifx_wwdt_data *data = dev->data;
	uint32_t wdt_config;
	uint32_t mask;
	k_spinlock_key_t key;
	int ret;

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0U) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (!data->installed) {
		ret = -EINVAL;
		goto out;
	}

	if (ifx_wwdt_is_enabled(dev)) {
		ret = -EBUSY;
		goto out;
	}

	wdt_config = data->limits.actions;

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U) {
		wdt_config |= IFX_WWDT_CONFIG_DEBUG_RUN;
	}

	ifx_wwdt_unlock(dev);
	ifx_wwdt_ctl_disable(dev);

	sys_write32(0U, config->base + IFX_WWDT_CNT);
	sys_write32(data->limits.lower, config->base + IFX_WWDT_LOWER_LIMIT);
	sys_write32(data->limits.warn, config->base + IFX_WWDT_WARN_LIMIT);
	sys_write32(data->limits.upper, config->base + IFX_WWDT_UPPER_LIMIT);
	sys_write32(wdt_config, config->base + IFX_WWDT_CONFIG);

	sys_write32(IFX_WWDT_INTR_WDT, config->base + IFX_WWDT_INTR);
	(void)sys_read32(config->base + IFX_WWDT_INTR);
	mask = ((wdt_config & IFX_WWDT_CONFIG_WARN_ACTION) != 0U) ? IFX_WWDT_INTR_WDT : 0U;
	sys_write32(mask, config->base + IFX_WWDT_INTR_MASK);

	ifx_wwdt_ctl_enable(dev);
	ifx_wwdt_lock(dev);
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_wwdt_disable(const struct device *dev)
{
	const struct ifx_wwdt_config *config = dev->config;
	struct ifx_wwdt_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = -EFAULT;

	key = k_spin_lock(&data->lock);

	if (ifx_wwdt_is_enabled(dev)) {
		ifx_wwdt_unlock(dev);
		ifx_wwdt_ctl_disable(dev);
		sys_write32(0U, config->base + IFX_WWDT_INTR_MASK);
		sys_write32(IFX_WWDT_INTR_WDT, config->base + IFX_WWDT_INTR);
		ifx_wwdt_lock(dev);
		ret = 0;
	}

	data->installed = false;
	data->callback = NULL;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_wwdt_feed(const struct device *dev, int channel_id)
{
	const struct ifx_wwdt_config *config = dev->config;
	struct ifx_wwdt_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if (channel_id != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!ifx_wwdt_is_enabled(dev)) {
		ret = -EINVAL;
	} else if ((sys_read32(config->base + IFX_WWDT_SERVICE) & IFX_WWDT_SERVICE_BIT) != 0U) {
		ret = -EAGAIN;
	} else {
		ifx_wwdt_unlock(dev);
		sys_write32(IFX_WWDT_SERVICE_BIT, config->base + IFX_WWDT_SERVICE);
		ifx_wwdt_lock(dev);
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
static void ifx_wwdt_isr(const struct device *dev)
{
	const struct ifx_wwdt_config *config = dev->config;
	struct ifx_wwdt_data *data = dev->data;

	if ((sys_read32(config->base + IFX_WWDT_INTR_MASKED) & IFX_WWDT_INTR_WDT) == 0U) {
		return;
	}

	sys_write32(IFX_WWDT_INTR_WDT, config->base + IFX_WWDT_INTR);
	(void)sys_read32(config->base + IFX_WWDT_INTR);

	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}
#endif

static int ifx_wwdt_init(const struct device *dev)
{
	const struct ifx_wwdt_config *config = dev->config;
	struct ifx_wwdt_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clk_dev)) {
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clk_dev, NULL, &data->rate);
	if ((ret != 0) || (data->rate == 0U)) {
		LOG_ERR("Failed to get the ILO0 rate");
		return (ret != 0) ? ret : -EINVAL;
	}

	data->sync_us = DIV_ROUND_UP(IFX_WWDT_SYNC_CYCLES * USEC_PER_SEC, data->rate);

	if (config->irq_config != NULL) {
		config->irq_config();
	}

	return 0;
}

static DEVICE_API(wdt, ifx_wwdt_api) = {
	.setup = ifx_wwdt_setup,
	.disable = ifx_wwdt_disable,
	.install_timeout = ifx_wwdt_install_timeout,
	.feed = ifx_wwdt_feed,
};

#define IFX_WWDT_IRQ_CONFIG(inst)                                                                  \
	static void ifx_wwdt_irq_config_##inst(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), ifx_wwdt_isr,         \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define IFX_WWDT_INIT(inst)                                                                        \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 0), (IFX_WWDT_IRQ_CONFIG(inst)))                      \
                                                                                                   \
	static const struct ifx_wwdt_config ifx_wwdt_config_##inst = {                             \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.irq_config = COND_CODE_1(DT_INST_IRQ_HAS_IDX(inst, 0),                           \
					  (ifx_wwdt_irq_config_##inst), (NULL)),                   \
	};                                                                                         \
                                                                                                   \
	static struct ifx_wwdt_data ifx_wwdt_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ifx_wwdt_init, NULL, &ifx_wwdt_data_##inst,                    \
			      &ifx_wwdt_config_##inst, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ifx_wwdt_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_WWDT_INIT)
