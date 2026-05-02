/*
 * Copyright (c) 2025 John Lin
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>

#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

static struct net_mgmt_event_callback wifi_cb;
static K_SEM_DEFINE(scan_done, 0, 1);

static void wifi_scan_result(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			     struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE) {
		printk("WiFi scan complete\n");
		k_sem_give(&scan_done);
	} else if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT) {
		const struct wifi_scan_result *entry =
			(const struct wifi_scan_result *)cb->info;
		printk("  SSID: %-32s CH: %2d RSSI: %d\n",
		       entry->ssid, entry->channel, entry->rssi);
	}
}

static void led_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		gpio_pin_toggle_dt(&led);
		k_sleep(K_MSEC(500));
	}
}

K_THREAD_DEFINE(led_thread, 512, led_thread_fn, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	int ret;

	printk("Pico W WiFi + LED Demo\n");

	if (!gpio_is_ready_dt(&led)) {
		printk("LED device not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Failed to configure LED: %d\n", ret);
		return 0;
	}

	printk("LED blinking started\n");

	/* Wait for WiFi to be ready */
	k_sleep(K_SECONDS(3));

	struct net_if *iface = net_if_get_default();

	if (!iface) {
		printk("No network interface\n");
		return 0;
	}

	net_mgmt_init_event_callback(&wifi_cb, wifi_scan_result,
				     NET_EVENT_WIFI_SCAN_RESULT |
				     NET_EVENT_WIFI_SCAN_DONE);
	net_mgmt_add_event_callback(&wifi_cb);

	printk("Starting WiFi scan...\n");
	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
	if (ret) {
		printk("WiFi scan request failed: %d\n", ret);
		return 0;
	}

	k_sem_take(&scan_done, K_SECONDS(15));
	printk("Demo complete - LED continues blinking\n");
	return 0;
}
