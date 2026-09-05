/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define XEN_WDT_CHANNELS 2
#define XEN_WDT_TIMEOUT_MS 3000U
#define XEN_WDT_FEED_INTERVAL_MS 1000U
#define XEN_WDT_FEED_RETRY_DELAY_MS 10U
#define XEN_WDT_FAIL_AFTER_FEEDS 4
#define XEN_WDT_FEED_STACK_SIZE 1024
#define XEN_WDT_FEED_PRIORITY 5
#define XEN_WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(xen_watchdog)

#ifndef CONFIG_XEN_WDT_FAIL_CHANNEL
#define CONFIG_XEN_WDT_FAIL_CHANNEL -1
#endif

struct feed_context {
	const struct device *wdt;
	int channels[XEN_WDT_CHANNELS];
};

static struct feed_context feed_ctx;
static K_THREAD_STACK_DEFINE(feed_stack0, XEN_WDT_FEED_STACK_SIZE);
static K_THREAD_STACK_DEFINE(feed_stack1, XEN_WDT_FEED_STACK_SIZE);
static struct k_thread feed_thread0;
static struct k_thread feed_thread1;

static int install_channel(const struct device *wdt, int index)
{
	const struct wdt_timeout_cfg cfg = {
		.flags = WDT_FLAG_RESET_SOC,
		.window = {
			.min = 0U,
			.max = XEN_WDT_TIMEOUT_MS,
		},
	};
	int channel;

	channel = wdt_install_timeout(wdt, &cfg);
	if (channel < 0) {
		printk("xen-wdt: install channel %d failed: %d\n", index, channel);
		return channel;
	}

	printk("xen-wdt: installed logical channel %d as Zephyr channel %d\n",
	       index, channel);
	return channel;
}

static void feed_channel(void *arg1, void *arg2, void *arg3)
{
	struct feed_context *ctx = arg1;
	int index = POINTER_TO_INT(arg2);
	int feed_count = 0;

	ARG_UNUSED(arg3);

	while (1) {
		int ret;

		if (index == CONFIG_XEN_WDT_FAIL_CHANNEL &&
		    feed_count >= XEN_WDT_FAIL_AFTER_FEEDS) {
			printk("xen-wdt: intentionally skip channel %d\n", index);
			k_msleep(XEN_WDT_FEED_INTERVAL_MS);
			continue;
		}

		do {
			ret = wdt_feed(ctx->wdt, ctx->channels[index]);
			if (ret == -EAGAIN) {
				k_msleep(XEN_WDT_FEED_RETRY_DELAY_MS);
			}
		} while (ret == -EAGAIN);

		feed_count++;
		k_msleep(XEN_WDT_FEED_INTERVAL_MS);
	}
}

int main(void)
{
	const struct device *const wdt = DEVICE_DT_GET(XEN_WDT_NODE);
	int ret;

	printk("xen-wdt: two-channel domain watchdog sample\n");
	if (CONFIG_XEN_WDT_FAIL_CHANNEL < 0) {
		printk("xen-wdt: no channel selected for failure\n");
	} else {
		printk("xen-wdt: fail channel %d\n", CONFIG_XEN_WDT_FAIL_CHANNEL);
	}

	if (!device_is_ready(wdt)) {
		printk("xen-wdt: %s is not ready\n", wdt->name);
		return 0;
	}

	feed_ctx.wdt = wdt;

	for (int i = 0; i < XEN_WDT_CHANNELS; i++) {
		feed_ctx.channels[i] = install_channel(wdt, i);
		if (feed_ctx.channels[i] < 0) {
			return 0;
		}
	}

	ret = wdt_setup(wdt, 0);
	if (ret < 0) {
		printk("xen-wdt: setup failed: %d\n", ret);
		return 0;
	}

	k_thread_create(&feed_thread0, feed_stack0, K_THREAD_STACK_SIZEOF(feed_stack0),
			feed_channel, &feed_ctx, INT_TO_POINTER(0), NULL,
			XEN_WDT_FEED_PRIORITY, 0, K_NO_WAIT);
	k_thread_create(&feed_thread1, feed_stack1, K_THREAD_STACK_SIZEOF(feed_stack1),
			feed_channel, &feed_ctx, INT_TO_POINTER(1), NULL,
			XEN_WDT_FEED_PRIORITY, 0, K_NO_WAIT);

	if (CONFIG_XEN_WDT_FAIL_CHANNEL >= 0) {
		printk("xen-wdt: channel %d will stop after %d feeds\n",
		       CONFIG_XEN_WDT_FAIL_CHANNEL, XEN_WDT_FAIL_AFTER_FEEDS);
	}

	return 0;
}
