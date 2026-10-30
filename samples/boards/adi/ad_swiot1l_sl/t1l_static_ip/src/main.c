/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

LOG_MODULE_REGISTER(t1l_static_ip, LOG_LEVEL_INF);

static const struct gpio_dt_spec link_led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static struct net_mgmt_event_callback iface_cb;

static void iface_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
				struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (event == NET_EVENT_IF_UP) {
		LOG_INF("T1L link up");
		gpio_pin_set_dt(&link_led, 1);
	} else if (event == NET_EVENT_IF_DOWN) {
		LOG_INF("T1L link down");
		gpio_pin_set_dt(&link_led, 0);
	}
}

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (gpio_is_ready_dt(&link_led)) {
		gpio_pin_configure_dt(&link_led, GPIO_OUTPUT_INACTIVE);
	}

	net_mgmt_init_event_callback(&iface_cb, iface_event_handler,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&iface_cb);

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return 0;
	}

	LOG_INF("IPv4 %s/%s", CONFIG_NET_CONFIG_MY_IPV4_ADDR, CONFIG_NET_CONFIG_MY_IPV4_NETMASK);

	if (net_if_is_up(iface)) {
		gpio_pin_set_dt(&link_led, 1);
	}

	return 0;
}
