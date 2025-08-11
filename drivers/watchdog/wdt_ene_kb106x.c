/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_watchdog

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include <errno.h>

#include <reg/wdt.h>

LOG_MODULE_REGISTER(ene_kb106x_wdt, CONFIG_WDT_LOG_LEVEL);

/* Device config */
struct wdt_kb106x_config {
	struct wdt_regs *wdt;
};

/* Device data */
struct wdt_kb106x_data {
	wdt_callback_t cb;
	bool timeout_installed;
};

/* WDT api functions */
static int wdt_kb106x_setup(const struct device *dev, uint8_t options)
{
	struct wdt_kb106x_config const *cfg = dev->config;
	struct wdt_kb106x_data *data = dev->data;

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		LOG_ERR("WDT_OPT_PAUSE_HALTED_BY_DBG is not supported");
		return -ENOTSUP;
	}

	/* Setting Clock Source */
	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		cfg->wdt->WDTCFG = WDT_ADCO32K;
	} else {
		cfg->wdt->WDTCFG = WDT_PHER32K;
	}
	/* Clear Pending Flag */
	cfg->wdt->WDTPF = (WDT_HALF_WAY_EVENT | WDT_RESET_EVENT);
	/* WDT enable */
	cfg->wdt->WDTCFG |= WDT_FUNCTON_ENABLE;

	return 0;
}

static int wdt_kb106x_disable(const struct device *dev)
{
	struct wdt_kb106x_config const *cfg = dev->config;
	struct wdt_kb106x_data *data = dev->data;

	if (!(cfg->wdt->WDTCFG & WDT_FUNCTON_ENABLE)) {
		LOG_ERR("wdt is not enabled");
		return -EALREADY;
	}
	/* WDT disable, write bit 7~4 = 1001b */
	cfg->wdt->WDTCFG = (cfg->wdt->WDTCFG & ~WDT_FUNCTON_ENABLE) | WDT_DISABLE_PASSWORD;
	/* Clear Pending Flag */
	cfg->wdt->WDTPF = (WDT_HALF_WAY_EVENT | WDT_RESET_EVENT);
	/* Need disable IE,or the wdt-half-event interrupt will be entered */
	cfg->wdt->WDTIE &= ~WDT_HALF_WAY_EVENT;
	data->timeout_installed = false;

	return 0;
}

static int wdt_kb106x_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *config)
{
	struct wdt_kb106x_config const *cfg = dev->config;
	struct wdt_kb106x_data *data = dev->data;

	/* Watchdog Counter Match Value */
	if (config->window.min > 0U) {
		data->timeout_installed = false;
		LOG_ERR("wdt not support window, window.min need set 0");
		return -EINVAL;
	}
	cfg->wdt->WDTM = (config->window.max * 1000) / WDT_TICK_TIME_US;
	/* (HW design) The counter match value must be >= 3 */
	if (cfg->wdt->WDTM < WDT_MIN_CNT) {
		data->timeout_installed = false;
		LOG_ERR("wdt timeout value below the hw min value");
		return -EINVAL;
	}

	/* Watchdog behavior flags */
	if ((config->flags & WDT_FLAG_RESET_MASK) == WDT_FLAG_RESET_SOC) {
		/* Reset: SoC */
		cfg->wdt->WDTCFG_T = WDT_RESET_WHOLE_CHIP_WO_GPIO;
	} else if ((config->flags & WDT_FLAG_RESET_MASK) == WDT_FLAG_RESET_CPU_CORE) {
		/* Reset: CPU core */
		cfg->wdt->WDTCFG_T = WDT_RESET_WHOLE_CHIP;
	} else {
		/* Reset: none */
		cfg->wdt->WDTCFG_T = WDT_RESET_ONLY_MCU;
	}

	/* Watchdog callback function */
	data->cb = config->callback;
	if (data->cb) {
		cfg->wdt->WDTIE |= WDT_HALF_WAY_EVENT;
	} else {
		/* If the callback function is NULL,the SoC will be reset directly.
		 * But still need enable interrupt.
		 */
		cfg->wdt->WDTIE |= WDT_HALF_WAY_EVENT;
	}
	data->timeout_installed = true;
	/* clear fw scratch register */
	cfg->wdt->WDTSCR[0] = 0;
	return 0;
}

static int wdt_kb106x_feed(const struct device *dev, int channel_id)
{
	struct wdt_kb106x_config const *cfg = dev->config;

	if (!(cfg->wdt->WDTCFG & WDT_FUNCTON_ENABLE)) {
		LOG_ERR("wdt is not enabled");
		return -EINVAL;
	}
	/* fw scratch register */
	cfg->wdt->WDTSCR[0] = (uint8_t)channel_id;
	/* Re-enable to reset counter */
	cfg->wdt->WDTCFG |= WDT_FUNCTON_ENABLE;
	/* Clear Pending Flag */
	cfg->wdt->WDTPF = WDT_HALF_WAY_EVENT;

	return 0;
}

static void wdt_kb106x_isr(const struct device *dev)
{
	struct wdt_kb106x_data *data = dev->data;

	if (data->cb) {
		data->cb(dev, 0);
	}
}

static DEVICE_API(wdt, wdt_kb106x_api) = {
	.setup = wdt_kb106x_setup,
	.disable = wdt_kb106x_disable,
	.install_timeout = wdt_kb106x_install_timeout,
	.feed = wdt_kb106x_feed,
};

static int wdt_kb106x_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_kb106x_disable(dev);
	}

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), wdt_kb106x_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static const struct wdt_kb106x_config wdt_kb106x_config = {
	.wdt = (struct wdt_regs *)DT_INST_REG_ADDR(0),
};

static struct wdt_kb106x_data wdt_kb106x_dev_data;

DEVICE_DT_INST_DEFINE(0, wdt_kb106x_init, NULL, &wdt_kb106x_dev_data, &wdt_kb106x_config,
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_kb106x_api);
