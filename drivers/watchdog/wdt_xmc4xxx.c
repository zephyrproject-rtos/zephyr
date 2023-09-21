/*
 * Copyright (C) 2023 SLB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_watchdog

#include <errno.h>
#include <stdint.h>

#include <soc.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>

#include <xmc_scu.h>
#include <xmc_wdt.h>

struct wdt_xmc4xxx_dev_data {
	wdt_callback_t cb;
	uint8_t mode;
	bool timeout_valid;
	bool is_serviced;
};

/* When the watchdog counter rolls over, the SCU will generate a */
/* pre-warning event which gets routed to the ISR below. If the */
/* watchdog is not serviced, the SCU will only reset the MCU */
/* after the second time the counter rolls over. */
/* Hence the reset will only happen after 2*cfg->window.max have elapsed. */
/* We could potentially manually reset the MCU, but this way the context */
/* information (i.e. that reset happened because of a watchdog) is lost. */

static void wdt_xmc4xxx_isr(const struct device *dev)
{
	struct wdt_xmc4xxx_dev_data *data = dev->data;
	uint32_t event = XMC_SCU_INTERUPT_GetEventStatus();

	/* todo add interrupt controller? */
	if ((event & XMC_SCU_INTERRUPT_EVENT_WDT_WARN) == 0) {
		return;
	}

	/* this is a level triggered interrupt. the event must be cleared */
	XMC_SCU_INTERRUPT_ClearEventStatus(XMC_SCU_INTERRUPT_EVENT_WDT_WARN);

	data->is_serviced = false;

	if (data->cb) {
		data->cb(dev, 0);
	}

	/* Ensure that watchdog is serviced if RESET_NONE mode is used */
	if (data->mode == WDT_FLAG_RESET_NONE && !data->is_serviced) {
		XMC_WDT_Service();
	}

	XMC_WDT_ClearAlarm();
}

static int wdt_xmc4xxx_disable(const struct device *dev)
{
	struct wdt_xmc4xxx_dev_data *data = dev->data;

	XMC_WDT_Stop();
	XMC_WDT_Disable();

	data->timeout_valid = false;

	return 0;
}

static int wdt_xmc4xxx_setup(const struct device *dev, uint8_t options)
{
	struct wdt_xmc4xxx_dev_data *data = dev->data;

	if (!data->timeout_valid) {
		return -EINVAL;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		SCU_CLK->SLEEPCR &= ~XMC_SCU_CLOCK_SLEEP_MODE_CONFIG_ENABLE_WDT;
	} else {
		SCU_CLK->SLEEPCR |= XMC_SCU_CLOCK_SLEEP_MODE_CONFIG_ENABLE_WDT;
	}

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0) {
		XMC_WDT_SetDebugMode(XMC_WDT_DEBUG_MODE_STOP);
	} else {
		XMC_WDT_SetDebugMode(XMC_WDT_DEBUG_MODE_RUN);
	}

	XMC_WDT_Start();

	return 0;
}

static int wdt_xmc4xxx_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	XMC_WDT_CONFIG_t wdt_config = {0};
	struct wdt_xmc4xxx_dev_data *data = dev->data;
	uint32_t wdt_clock;

	/* disable the watchdog if timeout was already installed */
	if (data->timeout_valid) {
		wdt_xmc4xxx_disable(dev);
		data->timeout_valid = false;
	}

	if (cfg->window.min != 0 || cfg->window.max == 0) {
		return -EINVAL;
	}

	wdt_clock = XMC_SCU_CLOCK_GetWdtClockFrequency();

	if ((uint64_t)cfg->window.max * wdt_clock / 1000 > UINT32_MAX) {
		return -EINVAL;
	}

	wdt_config.window_upper_bound = (uint64_t)cfg->window.max * wdt_clock / 1000;

	XMC_WDT_Init(&wdt_config);
	XMC_WDT_SetDebugMode(XMC_WDT_MODE_PREWARNING);
	XMC_SCU_INTERRUPT_EnableEvent(XMC_SCU_INTERRUPT_EVENT_WDT_WARN);

	if (cfg->flags == WDT_FLAG_RESET_NONE && cfg->callback == NULL) {
		return -EINVAL;
	}

	data->cb = cfg->callback;
	data->mode = cfg->flags;
	data->timeout_valid = true;

	return 0;
}

static int wdt_xmc4xxx_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);
	struct wdt_xmc4xxx_dev_data *data = dev->data;

	XMC_WDT_Service();
	data->is_serviced = true;

	return 0;
}

static const struct wdt_driver_api wdt_xmc4xxx_api = {
	.setup = wdt_xmc4xxx_setup,
	.disable = wdt_xmc4xxx_disable,
	.install_timeout = wdt_xmc4xxx_install_timeout,
	.feed = wdt_xmc4xxx_feed,
};

static struct wdt_xmc4xxx_dev_data wdt_xmc4xxx_data;

static void wdt_xmc4xxx_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), wdt_xmc4xxx_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

static int wdt_xmc4xxx_init(const struct device *dev)
{
	wdt_xmc4xxx_irq_config();

#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	return 0;
#else
	int ret;
	const struct wdt_timeout_cfg cfg = {.window.max = CONFIG_WDT_XMC4XXX_DEFAULT_TIMEOUT_MAX_MS,
					    .flags = WDT_FLAG_RESET_SOC};

	ret = wdt_xmc4xxx_install_timeout(dev, &cfg);
	if (ret < 0) {
		return ret;
	}

	return wdt_xmc4xxx_setup(dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
#endif
}
DEVICE_DT_INST_DEFINE(0, wdt_xmc4xxx_init, NULL, &wdt_xmc4xxx_data, NULL, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_xmc4xxx_api);
