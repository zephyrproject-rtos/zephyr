/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/edac.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define STACKSIZE 1024
#define PRIORITY  15

static atomic_t handled;

/**
 * Callback called from ISR. Runs in supervisor mode
 */
static void notification_callback(const struct device *dev, void *data)
{
	/* Special care need to be taken for NMI callback:
	 * delayed_work, mutex and semaphores are not working stable
	 * here, using integer increment for now
	 */
	atomic_set(&handled, true);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_edac));

	if (!device_is_ready(dev)) {
		printk("%s: device not ready.\n", dev->name);
		return 0;
	}

	if (edac_notify_callback_set(dev, notification_callback)) {
		LOG_ERR("Cannot set notification callback");
		return 0;
	}

	LOG_INF("EDAC shell application initialized");
	return 0;
}

void thread_function(void)
{
	LOG_DBG("Thread started");

	while (true) {
		if (atomic_cas(&handled, true, false)) {
			printk("Got notification about ECC event\n");
			k_sleep(K_MSEC(300));
		}
	}
}

K_THREAD_DEFINE(thread_edac, STACKSIZE, thread_function, NULL, NULL, NULL, PRIORITY, 0, 0);
