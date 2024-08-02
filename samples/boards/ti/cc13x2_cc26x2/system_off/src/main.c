/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2020 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/poweroff.h>

#include <driverlib/ioc.h>

static const struct gpio_dt_spec sw0_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

#define BUSY_WAIT_S 5U
#define SLEEP_US 2000U
#define SLEEP_S     3U

extern void CC1352R1_LAUNCHXL_shutDownExtFlash(void);

int main(void)
{
	uint32_t config, status;

	printk("\n%s system off demo\n", CONFIG_BOARD);

	/* Shut off external flash to save power */
	CC1352R1_LAUNCHXL_shutDownExtFlash();

	/* Configure to generate PORT event (wakeup) on button 1 press. */
	if (!gpio_is_ready_dt(&sw0_gpio)) {
		printk("%s: device not ready.\n", sw0_gpio.port->name);
		return 0;
	}

	gpio_pin_configure_dt(&sw0_gpio, GPIO_INPUT);

	/* Set wakeup bits for button gpio */
	config = IOCPortConfigureGet(sw0_gpio.pin);
	config |= IOC_WAKE_ON_LOW;
	IOCPortConfigureSet(sw0_gpio.pin, IOC_PORT_GPIO, config);

	printk("Busy-wait %u s\n", BUSY_WAIT_S);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);

	printk("Sleep %u us (IDLE)\n", SLEEP_US);
	k_usleep(SLEEP_US);

	printk("Sleep %u s (STANDBY)\n", SLEEP_S);
	k_sleep(K_SECONDS(SLEEP_S));

	printk("Powering off; press BUTTON1 to restart\n");

	/* Clear GPIO interrupt */
	status = GPIO_getEventMultiDio(GPIO_DIO_ALL_MASK);
	GPIO_clearEventMultiDio(status);

	sys_poweroff();

	return 0;
}
