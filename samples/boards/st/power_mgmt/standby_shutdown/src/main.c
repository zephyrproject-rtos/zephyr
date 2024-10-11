/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/poweroff.h>
#include <stm32_ll_pwr.h>
#include <zephyr/dt-bindings/gpio/stm32-gpio.h>

#define STACKSIZE 1024
#define PRIORITY 7
#define SLEEP_TIME_MS   3000

#define WKUP_SRC_NODE DT_ALIAS(wkup_src)
#if !DT_NODE_HAS_STATUS_OKAY(WKUP_SRC_NODE)
#error "Unsupported board: wkup_src devicetree alias is not defined"
#endif


/* Semaphore used to control button pressed value */
static struct k_sem button_sem;

static int led_is_on;

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(WKUP_SRC_NODE, gpios);

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	k_sem_give(&button_sem);
}

void thread_poweroff_standby_mode(void)
{
	k_sem_init(&button_sem, 0, 1);
	k_sem_take(&button_sem, K_FOREVER);
	gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);
	printk("User button pressed\n");

	/* Setup button GPIO pin as a wakeup Source for exiting low power mode */
	if (gpio_pin_configure_dt(&button, STM32_GPIO_WKUP) != 0) {
		printk("Failed to configure wakeup source\n");
		return;
	}
	if (led_is_on == false) {
		printk("Powering off\n");
		printk("Release the user button to wake-up\n\n");
#ifdef CONFIG_LOG
		k_msleep(2000);
#endif /* CONFIG_LOG */
		sys_poweroff();
		/* powered off until wakeup line activated */
	} else {
		printk("Standby Mode requested\n");
		printk("Release the user button to exit from Standby Mode\n\n");
#ifdef CONFIG_LOG
		k_msleep(2000);
#endif /* CONFIG_LOG */
		pm_state_force(0u, &(struct pm_state_info) {PM_STATE_STANDBY, 0, 0});
		/* stay in Standby mode until wakeup line activated */
	}
}

K_THREAD_DEFINE(thread_poweroff_standby_mode_id, STACKSIZE, thread_poweroff_standby_mode,
	NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
	int ret;
	uint32_t cause;

	hwinfo_get_reset_cause(&cause);
	hwinfo_clear_reset_cause();

	if (cause == RESET_LOW_POWER_WAKE) {
		hwinfo_clear_reset_cause();
		printk("\nReset cause: Standby mode\n");
	}
	if (cause == (RESET_PIN | RESET_BROWNOUT)) {
		printk("\nReset cause: Shutdown mode or power up\n");
	}
	if (cause == RESET_PIN) {
		printk("\nReset cause: Reset pin\n");
	}
	/* After flashing the reset cause is  0x3 : RESET_SOFTWARE | RESET_PIN */
	if ((cause & RESET_SOFTWARE) == RESET_SOFTWARE) {
		printk("\nReset cause: Reset soft\n");
	}

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));
	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
			button.port->name);
		return 0;
	}

	/* Configure button to detect key pressed  = low power mode selection */
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
			ret, button.port->name, button.pin);
		return 0;
	}
	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return 0;
	}
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	printk("Device ready: %s\n\n\n", CONFIG_BOARD);

	printk("Press and hold the user button:\n");
	printk("  when LED2 is OFF to power off\n");
	printk("  when LED2 is ON to enter to Standby Mode\n\n");

	led_is_on = true;
	while (true) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set(led.port, led.pin, (int)led_is_on);
		k_msleep(SLEEP_TIME_MS);
		led_is_on = !led_is_on;
	}

	return 0;
}
