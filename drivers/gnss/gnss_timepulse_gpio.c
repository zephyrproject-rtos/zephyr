/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#include "gnss_timepulse_backend.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gnss_timepulse_gpio, CONFIG_GNSS_LOG_LEVEL);

struct gnss_timepulse_gpio {
	struct gpio_dt_spec gpio;
	struct gpio_callback cb;
	struct gnss_timepulse *tp;
};

static void gnss_timepulse_gpio_isr(const struct device *port, struct gpio_callback *cb,
				    uint32_t pins)
{
	struct gnss_timepulse_gpio *entry = CONTAINER_OF(cb, struct gnss_timepulse_gpio, cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	gnss_timepulse_report(entry->tp, k_uptime_ticks());
}

static int gnss_timepulse_gpio_attach(const struct gnss_timepulse_source *src,
				      struct gnss_timepulse *tp)
{
	struct gnss_timepulse_gpio *entry = src->data;
	gpio_flags_t int_flags;
	int ret;

	if (!gpio_is_ready_dt(&entry->gpio)) {
		LOG_ERR("PPS GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&entry->gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&entry->cb, gnss_timepulse_gpio_isr, BIT(entry->gpio.pin));

	ret = gpio_add_callback_dt(&entry->gpio, &entry->cb);
	if (ret < 0) {
		return ret;
	}

	entry->tp = tp;

	int_flags = GPIO_INT_EDGE_TO_ACTIVE |
		    (entry->gpio.dt_flags & (GPIO_ACTIVE_LOW | GPIO_INT_WAKEUP));
	ret = gpio_pin_interrupt_configure(entry->gpio.port, entry->gpio.pin, int_flags);
	if (ret < 0) {
		gpio_remove_callback(entry->gpio.port, &entry->cb);
		entry->tp = NULL;
		return ret;
	}

	tp->available = true;

	return 0;
}

#define GNSS_TIMEPULSE_GPIO_SOURCE_DEFINE(node_id)                                          \
	static struct gnss_timepulse_gpio gnss_timepulse_gpio_##node_id = {                 \
		.gpio = GPIO_DT_SPEC_GET(node_id, timepulse_gpios),                         \
	};                                                                                    \
	GNSS_TIMEPULSE_SOURCE_DEFINE(gnss_timepulse_source_gpio_##node_id,                  \
				     DEVICE_DT_GET(node_id), gnss_timepulse_gpio_attach,    \
				     &gnss_timepulse_gpio_##node_id);

#define GNSS_TIMEPULSE_GPIO_NODE(node_id)                                                    \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, timepulse_gpios),                              \
		    (GNSS_TIMEPULSE_GPIO_SOURCE_DEFINE(node_id)), ())

/* Limit capture source generation to the current in-tree GNSS consumer. */
DT_FOREACH_STATUS_OKAY(gnss_nmea_generic, GNSS_TIMEPULSE_GPIO_NODE);
