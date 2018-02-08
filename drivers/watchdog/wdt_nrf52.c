/*
 * Copyright (c) 2018 Intellinium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <watchdog.h>
#include <nrf_wdt.h>
#include <string.h>
#include <logging/sys_log.h>
#include <irq.h>
#include <soc.h>

static struct {
	nrf_wdt_behaviour_t behaviour;
	nrf_wdt_rr_register_t channel_id;
	struct device *self;
	struct wdt_config config;
	bool config_set;
} nrf52_wdt_config = {
	.behaviour = NRF_WDT_BEHAVIOUR_RUN_SLEEP,
	.channel_id = NRF_WDT_RR0
};

/* Internal functions */
static uint32_t convert_reload(uint32_t t)
{
	return (uint32_t) ((uint64_t)t * 32768 / 1000);
}

static bool is_reload_valid(uint32_t t)
{
	return convert_reload(t) <= UINT32_MAX;
}

/* API functions*/
static int nrf52_wdt_set_config(__unused struct device *dev,
	struct wdt_config *config)
{
	if (!config) {
		return -ENOTSUP;
	}

	if (!is_reload_valid(config->timeout)) {
		return -E2BIG;
	}

	memcpy(&nrf52_wdt_config.config, config, sizeof(*config));
	nrf_wdt_reload_value_set(
		convert_reload(nrf52_wdt_config.config.timeout));
	nrf52_wdt_config.config_set = true;

	return 0;
}

static void nrf52_wdt_get_config(__unused struct device *dev,
	struct wdt_config *config)
{
	if (!config) {
		return;
	}

	memcpy(config, &nrf52_wdt_config.config, sizeof(*config));
}

static void nrf52_wdt_reload(__unused struct device *dev)
{
	nrf_wdt_reload_request_set(nrf52_wdt_config.channel_id);
}

static void nrf52_wdt_enable(__unused struct device *dev)
{
	if (!nrf52_wdt_config.config_set) {
		SYS_LOG_ERR("The config must be set before enabling the "
			"watchdog");
		return;
	}

	nrf_wdt_int_enable(NRF_WDT_INT_TIMEOUT_MASK);
	nrf_wdt_task_trigger(NRF_WDT_TASK_START);
}

static void nrf52_wdt_disable(__unused struct device *dev)
{
	SYS_LOG_ERR("Operation %s not supported", __func__);
}

static void alloc_wdt_channel(void)
{
	nrf_wdt_reload_request_enable(nrf52_wdt_config.channel_id);
}

static void enable_wdt_irq(void);
static int nrf52_wdt_init(__unused struct device *dev)
{
	nrf_wdt_behaviour_set(nrf52_wdt_config.behaviour);

	enable_wdt_irq();

	alloc_wdt_channel();

	nrf52_wdt_config.self = dev;

	return 0;
}

static const struct wdt_driver_api wdt_api = {
	.enable = nrf52_wdt_enable,
	.disable = nrf52_wdt_disable,
	.get_config = nrf52_wdt_get_config,
	.set_config = nrf52_wdt_set_config,
	.reload = nrf52_wdt_reload
};

DEVICE_AND_API_INIT(nrf52_wdt, "WDT_0",
	nrf52_wdt_init, &nrf52_wdt_config, NULL,
	PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	&wdt_api);

static void nrf52_wdt_isr(void)
{
	if (nrf52_wdt_config.config.interrupt_fn) {
		nrf52_wdt_config.config.interrupt_fn(nrf52_wdt_config.self);
	}
}

static void enable_wdt_irq(void)
{
	IRQ_CONNECT(NRF5_IRQ_WDT_IRQn, CONFIG_WDT_IRQ_PRI, nrf52_wdt_isr,
		DEVICE_GET(nrf52_wdt), 0);
}
