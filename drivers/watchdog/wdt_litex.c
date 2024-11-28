/*
 * Copyright (C) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_watchdog

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/device.h>
#include <zephyr/sys_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_litex, CONFIG_WDT_LOG_LEVEL);

#include <soc.h>

struct wdt_litex_data {
	wdt_callback_t callback;
	uint32_t timeout;
	bool reset_soc_mode;
	bool pause_halted;
};

struct wdt_litex_config {
	uint32_t control_addr;
	uint32_t cycles_addr;
	uint32_t cycles_size;
	uint32_t remaining_addr;
	uint32_t ev_status_addr;
	uint32_t ev_pending_addr;
	uint32_t ev_enable_addr;
	void (*irq_cfg_func)(void);
};

#define CONTROL_FEED_BIT         BIT(0)
#define CONTROL_ENABLE_BIT       BIT(8)
#define CONTROL_RESET_BIT        BIT(16)
#define CONTROL_PAUSE_HALTED_BIT BIT(24)

static bool wdt_litex_is_enabled(const struct device *dev)
{
	const struct wdt_litex_config *config = dev->config;

	return litex_read8(config->control_addr) & BIT(0);
}

static void wdt_litex_irq_enable(const struct device *dev)
{
	const struct wdt_litex_config *config = dev->config;
	struct wdt_litex_data *data = dev->data;

	if (!data->callback) {
		return;
	}

	litex_write8(BIT(0), config->ev_pending_addr);

	litex_write8(BIT(0), config->ev_enable_addr);
}

static void wdt_litex_enable(const struct device *dev)
{
	const struct wdt_litex_config *config = dev->config;
	struct wdt_litex_data *data = dev->data;
	uint32_t control;

	if (config->cycles_size <= 4) {
		litex_write32(k_ms_to_cyc_floor32(data->timeout), config->cycles_addr);
	} else {
		litex_write64(k_ms_to_cyc_floor64(data->timeout), config->cycles_addr);
	}

	control = CONTROL_FEED_BIT | CONTROL_ENABLE_BIT;

	if (data->reset_soc_mode) {
		control |= CONTROL_RESET_BIT;
	}
	if (data->pause_halted) {
		control |= CONTROL_PAUSE_HALTED_BIT;
	}

	litex_write32(control, config->control_addr);

	wdt_litex_irq_enable(dev);
}

static int wdt_litex_disable(const struct device *dev)
{
	const struct wdt_litex_config *config = dev->config;

	litex_write8(0, config->ev_enable_addr);

	if (!wdt_litex_is_enabled(dev)) {
		return -EFAULT;
	}
	litex_write16(CONTROL_ENABLE_BIT, config->control_addr);

	return 0;
}

static int wdt_litex_feed(const struct device *dev, int channel_id)
{
	const struct wdt_litex_config *config = dev->config;

	if (channel_id != 0) {
		return -EINVAL;
	}

	litex_write8(CONTROL_FEED_BIT, config->control_addr);

	return 0;
}

static int wdt_litex_setup(const struct device *dev, uint8_t options)
{
	struct wdt_litex_data *data = dev->data;

	data->pause_halted = !!(options & WDT_OPT_PAUSE_HALTED_BY_DBG);

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	if (wdt_litex_is_enabled(dev)) {
		return -EBUSY;
	}

	wdt_litex_enable(dev);
	wdt_litex_feed(dev, 0);

	return 0;
}

static int wdt_litex_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_litex_config *config = dev->config;
	struct wdt_litex_data *data = dev->data;

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	if (cfg->window.max > (config->cycles_size <= 4 ? k_cyc_to_ms_floor32(UINT32_MAX)
							: k_cyc_to_ms_floor64(UINT64_MAX))) {
		return -EINVAL;
	}

	if (wdt_litex_is_enabled(dev)) {
		return -EBUSY;
	}

	data->timeout = cfg->window.max;
	data->callback = cfg->callback;

	/* Set mode of watchdog and callback */
	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		LOG_DBG("Configuring reset SOC mode");
		data->reset_soc_mode = true;
		break;

	case WDT_FLAG_RESET_NONE:
		LOG_DBG("Configuring non-reset mode");
		data->reset_soc_mode = false;
		break;

	default:
		LOG_ERR("Unsupported watchdog config flag");
		return -EINVAL;
	}

	return 0;
}

static void wdt_litex_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct wdt_litex_config *config = dev->config;
	struct wdt_litex_data *data = dev->data;
	unsigned int key = irq_lock();

	if (data->callback) {
		data->callback(dev, 0);
	}

	litex_write8(BIT(0), config->ev_pending_addr);

	irq_unlock(key);
}

static int wdt_litex_init(const struct device *dev)
{
	const struct wdt_litex_config *const config = dev->config;

	config->irq_cfg_func();

#ifndef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_litex_enable(dev);
#endif

	return 0;
}

static DEVICE_API(wdt, wdt_api) = {
	.setup = wdt_litex_setup,
	.disable = wdt_litex_disable,
	.install_timeout = wdt_litex_install_timeout,
	.feed = wdt_litex_feed,
};

#define LITEX_WDT_INIT(n)                                                                          \
	static void wdt_litex_cfg_func_##n(void);                                                  \
                                                                                                   \
	static struct wdt_litex_data wdt_litex_data##n;                                            \
	static struct wdt_litex_config wdt_litex_config##n = {                                     \
		.control_addr = DT_INST_REG_ADDR_BY_NAME(n, control),                              \
		.cycles_addr = DT_INST_REG_ADDR_BY_NAME(n, cycles),                                \
		.cycles_size = DT_INST_REG_SIZE_BY_NAME(n, cycles),                                \
		.remaining_addr = DT_INST_REG_ADDR_BY_NAME(n, remaining),                          \
		.ev_status_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_status),                          \
		.ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_pending),                        \
		.ev_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_enable),                          \
		.irq_cfg_func = wdt_litex_cfg_func_##n,                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, wdt_litex_init, NULL, &wdt_litex_data##n, &wdt_litex_config##n,   \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_api)          \
                                                                                                   \
	static void wdt_litex_cfg_func_##n(void)                                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), wdt_litex_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(LITEX_WDT_INIT)
