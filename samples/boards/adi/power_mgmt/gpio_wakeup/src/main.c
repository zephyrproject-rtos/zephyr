/*
 * Copyright (c) 2024-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

static const struct pm_state_info residency_info[] =
	PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));

static const struct gpio_dt_spec wk_btn = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static const struct gpio_dt_spec pwr_off_wk_btn =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(wk_pin), gpios, {0});

static struct gpio_callback button_cb_data;

static volatile uint32_t btn_pressed_ms;

#define LONG_PRESS_MS 3000

K_SEM_DEFINE(button_sem, 0, 1);

static void debounce_work_handler(struct k_work *work)
{
	btn_pressed_ms = k_uptime_get_32();

	k_sem_give(&button_sem);
}

K_WORK_DELAYABLE_DEFINE(debounce_work, debounce_work_handler);

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_schedule(&debounce_work, K_MSEC(200));
}

static void pm_policy_lock_others(const struct pm_state_info *info)
{
	for (size_t i = 0; i < ARRAY_SIZE(residency_info); i++) {
		if (residency_info[i].state != info->state) {
			pm_policy_state_lock_get(residency_info[i].state, 0);
		}
	}
}

static void pm_policy_unlock_others(const struct pm_state_info *info)
{
	for (size_t i = 0; i < ARRAY_SIZE(residency_info); i++) {
		if (residency_info[i].state != info->state) {
			pm_policy_state_lock_put(residency_info[i].state, 0);
		}
	}
}

int main(void)
{
	int ret, state_idx = 0;
	uint32_t now_ms;

	if (!gpio_is_ready_dt(&wk_btn)) {
		printk("Error: button device %s is not ready\n", wk_btn.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&wk_btn, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, wk_btn.port->name,
		       wk_btn.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&wk_btn, GPIO_INT_EDGE_TO_ACTIVE | GPIO_INT_WAKEUP);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
		       wk_btn.port->name, wk_btn.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(wk_btn.pin));

	printk("Press the user button to wake the device up from sleep modes\n");
	printk("Long press the user button to power off the device\n");

	while (true) {
		printk("Entering PM state %s\n", pm_state_to_str(residency_info[state_idx].state));

		/* Lock all PM states except the one we're about to enter to make sure
		 * that the system enters the expected state. pm_state_force() is only
		 * valid for the next entry into the specified state, and the OS can
		 * already consume that entry by the time we call it, so we instead
		 * lock the other states to ensure the system always enters the state
		 * we want to test in each iteration of the loop.
		 */
		pm_policy_lock_others(&residency_info[state_idx]);

		gpio_add_callback(wk_btn.port, &button_cb_data);
		k_sem_take(&button_sem, K_FOREVER);
		gpio_remove_callback(wk_btn.port, &button_cb_data);

		pm_policy_unlock_others(&residency_info[state_idx]);
		if (residency_info[state_idx].state == PM_STATE_SUSPEND_TO_RAM) {
			gpio_pin_configure_dt(&wk_btn, GPIO_INPUT);
			gpio_pin_interrupt_configure_dt(&wk_btn,
							GPIO_INT_EDGE_TO_ACTIVE | GPIO_INT_WAKEUP);
		}

		while (gpio_pin_get_dt(&wk_btn) == 1) {
			now_ms = k_uptime_get_32();
			if (now_ms - btn_pressed_ms > LONG_PRESS_MS) {
				printk("Long button press detected, powering off the device\n");
				gpio_pin_configure_dt(&pwr_off_wk_btn, GPIO_INPUT);
				k_busy_wait(1000);
				sys_poweroff();
			}
			k_msleep(5);
		}

		state_idx++;
		if (state_idx >= ARRAY_SIZE(residency_info)) {
			state_idx = 0;
		}
	}

	return 0;
}
