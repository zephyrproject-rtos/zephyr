/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>

/*
 * To use this sample, either the devicetree's /aliases must have a
 * 'watchdog0' property, or one of the following watchdog compatibles
 * must have an enabled node.
 *
 * If the devicetree has a watchdog node, we get the watchdog device
 * from there. Otherwise, the task watchdog will be used without a
 * hardware watchdog fallback.
 */
#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(watchdog0))
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_window_watchdog)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(st_stm32_window_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_watchdog)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(st_stm32_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_wdt)
#elif DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_watchdog)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(espressif_esp32_watchdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(silabs_gecko_wdog)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(silabs_gecko_wdog)
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_wdog32)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_wdog32)
#elif DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_watchdog)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(microchip_xec_watchdog)
#else
#define WDT_NODE DT_INVALID_NODE
#endif

static void task_wdt_callback(int channel_id, void *user_data)
{
	printk("Task watchdog channel %d callback, thread: %s\n",
		channel_id, k_thread_name_get((k_tid_t)user_data));

	/*
	 * If the issue could be resolved, call task_wdt_feed(channel_id) here
	 * to continue operation.
	 *
	 * Otherwise we can perform some cleanup and reset the device.
	 */

	printk("Resetting device...\n");

	sys_reboot(SYS_REBOOT_COLD);
}

int main(void)
{
	int ret;
	const struct device *const hw_wdt_dev = DEVICE_DT_GET_OR_NULL(WDT_NODE);

	printk("Task watchdog sample application.\n");

	if (!device_is_ready(hw_wdt_dev)) {
		printk("Hardware watchdog not ready; ignoring it.\n");
	}

	ret = task_wdt_init(hw_wdt_dev);
	if (ret != 0) {
		printk("task wdt init failure: %d\n", ret);
		return 0;
	}


	/* passing NULL instead of callback to trigger system reset */
	int task_wdt_id = task_wdt_add(1100U, NULL, NULL);

	while (true) {
		printk("Main thread still alive...\n");
		task_wdt_feed(task_wdt_id);
		k_sleep(K_MSEC(1000));
	}
	return 0;
}

/*
 * This high-priority thread needs a tight timing
 */
void control_thread(void)
{
	int task_wdt_id;
	int count = 0;

	printk("Control thread started.\n");

	/*
	 * Add a new task watchdog channel with custom callback function and
	 * the current thread ID as user data.
	 */
	task_wdt_id = task_wdt_add(100U, task_wdt_callback,
		(void *)k_current_get());

	while (true) {
		if (count == 50) {
			printk("Control thread getting stuck...\n");
			k_sleep(K_FOREVER);
		}

		task_wdt_feed(task_wdt_id);
		k_sleep(K_MSEC(50));
		count++;
	}
}

K_THREAD_DEFINE(control, 1024, control_thread, NULL, NULL, NULL, -1, 0, 1000);
