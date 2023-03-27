/*
 * Copyright (c) 2022 Dronetag
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/bluetooth/hci.h>

#include "controller/hal/ticker.h"
#include "controller/ticker/ticker.h"
#include "controller/include/ll.h"
#include "controller/ll_sw/nordic/hal/nrf5/debug.h"

LOG_MODULE_REGISTER(coex_ticker);

#define COEX_RADIO_WORK_DELAY_US  50
#define COEX_RADIO_WORK_RESCHEDULE_DELAY_US  200

struct coex_ticker_config {
	struct gpio_dt_spec grant_spec;
	size_t grant_delay_us;
};

struct coex_ticker_data {
	const struct device *dev;
	struct gpio_callback grant_irq_cb;
};

static int time_slot_delay(uint32_t ticks_at_expire, uint32_t ticks_delay,
				ticker_timeout_func callback, void *context)
{
	uint8_t instance_index;
	uint8_t ticker_id;
	int err;

	ll_coex_ticker_id_get(&instance_index, &ticker_id);

	/* start a secondary one-shot ticker after ticks_delay,
	 * this will let any radio role to gracefully abort and release the
	 * Radio h/w.
	 */
	err = ticker_start(instance_index, /* Radio instance ticker */
				1, /* user id for link layer ULL_HIGH */
				   /* (MAYFLY_CALL_ID_WORKER) */
				ticker_id, /* ticker_id */
				ticks_at_expire, /* current tick */
				ticks_delay, /* one-shot delayed timeout */
				0, /* periodic timeout  */
				0, /* periodic remainder */
				0, /* lazy, voluntary skips */
				0,
				callback, /* handler for executing radio abort or */
					 /* coex work */
				context, /* the context for the coex operation */
				NULL, /* no op callback */
				NULL);

	return err;
}


static void time_slot_callback_work(uint32_t ticks_at_expire,
					uint32_t ticks_drift,
					uint32_t remainder,
					uint16_t lazy, uint8_t force,
					void *context)
{
	int ret;
	uint8_t instance_index;
	uint8_t ticker_id;
	const struct device *dev = context;
	const struct coex_ticker_config *config = dev->config;

	__ASSERT(ll_radio_state_is_idle(),
		 "Radio is on during coex operation.\n");

	/* Read grant pin */
	if (gpio_pin_get_dt(&config->grant_spec) == 0) {
		DEBUG_COEX_GRANT(1);

		if (!ll_radio_state_is_idle()) {
			ll_radio_state_abort();
		}
		/* Schedule another check for grant pin and abort any events scheduled */
		time_slot_delay(ticker_ticks_now_get(),
				HAL_TICKER_US_TO_TICKS(COEX_RADIO_WORK_RESCHEDULE_DELAY_US),
				time_slot_callback_work,
				context);
	} else {
		LOG_DBG("COEX Inhibit Off");
		DEBUG_COEX_IRQ(0);
		DEBUG_COEX_GRANT(0);

		/* Enable coex pin interrupt */
		gpio_pin_interrupt_configure_dt(&config->grant_spec, GPIO_INT_EDGE_TO_INACTIVE);

		/* Stop the time slot ticker */
		ll_coex_ticker_id_get(&instance_index, &ticker_id);

		ret = ticker_stop(instance_index, 0, ticker_id, NULL, NULL);
		if (ret != TICKER_STATUS_SUCCESS &&
			ret != TICKER_STATUS_BUSY) {
			__ASSERT(0, "Failed to stop ticker.\n");
		}
	}
}

static void time_slot_callback_abort(uint32_t ticks_at_expire,
					 uint32_t ticks_drift,
					 uint32_t remainder,
					 uint16_t lazy, uint8_t force,
					 void *context)
{
	ll_radio_state_abort();
	time_slot_delay(ticks_at_expire,
			HAL_TICKER_US_TO_TICKS(COEX_RADIO_WORK_DELAY_US),
			time_slot_callback_work,
			context);
}

static int coex_ticker_grant_start(const struct device *dev)
{
	const struct coex_ticker_config *cfg = dev->config;
	uint32_t err;

	err = time_slot_delay(
		ticker_ticks_now_get(),
		HAL_TICKER_US_TO_TICKS(cfg->grant_delay_us - COEX_RADIO_WORK_DELAY_US),
		time_slot_callback_abort,
		(void *)dev
	);

	if (err != TICKER_STATUS_SUCCESS && err != TICKER_STATUS_BUSY) {
		return -EBUSY;
	}

	return 0;
}


static void coex_ticker_grant_irq_handler(const struct device *dev,
					struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);
	struct coex_ticker_data *data = CONTAINER_OF(cb, struct coex_ticker_data, grant_irq_cb);
	const struct coex_ticker_config *config = data->dev->config;

	LOG_DBG("COEX Inhibit IRQ Detected");
	DEBUG_COEX_IRQ(1);
	DEBUG_COEX_GRANT(0);

	gpio_pin_interrupt_configure_dt(&config->grant_spec, GPIO_INT_DISABLE);
	coex_ticker_grant_start(data->dev);
}


static int coex_ticker_init(const struct device *dev)
{
	const struct coex_ticker_config *config = dev->config;
	struct coex_ticker_data *data = dev->data;
	int res;

	data->dev = dev;

	DEBUG_COEX_INIT();
	res = gpio_pin_configure_dt(&config->grant_spec, GPIO_INPUT);
	if (res) {
		return res;
	}

	gpio_init_callback(&data->grant_irq_cb,
					   coex_ticker_grant_irq_handler,
					   BIT(config->grant_spec.pin));

	res = gpio_add_callback(config->grant_spec.port, &data->grant_irq_cb);
	if (res) {
		return res;
	}

	res = gpio_pin_interrupt_configure_dt(&config->grant_spec, GPIO_INT_EDGE_TO_INACTIVE);
	if (res) {
		return res;
	}

	return 0;
}

#define RADIO_NODE DT_NODELABEL(radio)
#define COEX_NODE DT_PROP(RADIO_NODE, coex)

#if DT_NODE_EXISTS(COEX_NODE)
static struct coex_ticker_config config = {
	.grant_spec = GPIO_DT_SPEC_GET(COEX_NODE, grant_gpios),
	.grant_delay_us = DT_PROP(COEX_NODE, grant_delay_us)
};
static struct coex_ticker_data data;

DEVICE_DEFINE(coex_ticker, "COEX_TICKER", &coex_ticker_init, NULL,
	&data, &config,
	APPLICATION, 90,
	NULL);
#endif
