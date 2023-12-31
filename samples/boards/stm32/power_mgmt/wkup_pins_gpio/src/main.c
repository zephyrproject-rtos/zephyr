/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/wakeup_source.h>

#define STACKSIZE     1024
#define PRIORITY      7
#define WAIT_TIME_US (1 << 22)

#define SW_WKUP_NODE DT_ALIAS(sw_wkup)
#if !DT_NODE_HAS_STATUS(SW_WKUP_NODE, okay)
#error "Unsupported board: sw_wkup devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW_WKUP_NODE, gpios, {0});

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct wakeup_source *user_button_ws = WAKEUP_SOURCE_DT_GET(SW_WKUP_NODE);

int main(void)
{
	int ret;
	uint32_t cause;

	hwinfo_get_reset_cause(&cause);
	hwinfo_clear_reset_cause();

	if (cause & RESET_BROWNOUT) {
		printk("\nReset cause: wake-up event or reset pin\n\n");
	} else if (cause & RESET_PIN) {
		printk("\nReset cause: Reset pin\n\n");
	}

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));
	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
		       button.pin);
		return 0;
	}

	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	printk("Device is ready\n");

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(led.port, led.pin, 1);

	sys_wakeup_source_enable(user_button_ws);

	printk("Will wait %d s before powering the system off\n", (WAIT_TIME_US >> 20));
	k_busy_wait(WAIT_TIME_US);

	printk("Powering off\n");
	printk("Press the user button to wake-up\n\n");

	sys_poweroff();
	/* powered off until wakeup line activated */

	return 0;
}
