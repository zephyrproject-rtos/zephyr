/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to utilize GPIO on Arduino 101 and Arduino Due.
 *
 * Sensortag CC2650 - ARM
 * --------------------
 *
 * LED1 is on DIO_10
 * LED2 is on DIO_15
 * BUTTON1 is on DIO_4
 * BUTTON2 is on DIO_0
 *
 * This sample toogles LED1 and wait interrupt on BUTTON1.
 * Note that an internet pull-up is set on BUTTON1 as the button
 * only drives low when pressed.
 *
 * Arduino 101 - x86
 * --------------------
 *
 * On x86 side of Arduino 101:
 * 1. GPIO_16 is on IO8
 * 2. GPIO_19 is on IO4
 *
 * The gpio_dw driver is being used.
 *
 * This sample app toggles GPIO_16/IO8. It also waits for
 * GPIO_19/IO4 to go high and display a message.
 *
 * If IO4 and IO8 are connected together, the GPIO should
 * triggers every 2 seconds. And you should see this repeatedly
 * on console:
 * "
 *     Toggling GPIO_16
 *     Toggling GPIO_16
 *     GPIO_19 triggered
 * "
 *
 * Arduino 101 - Sensor Subsystem
 * ------------------------------
 *
 * On Sensor Subsystem of Arduino 101:
 * 1. GPIO_SS[ 2] is on A00
 * 2. GPIO_SS[ 3] is on A01
 * 3. GPIO_SS[ 4] is on A02
 * 4. GPIO_SS[ 5] is on A03
 * 5. GPIO_SS[10] is on IO3
 * 6. GPIO_SS[11] is on IO5
 * 7. GPIO_SS[12] is on IO6
 * 8. GPIO_SS[13] is on IO9
 *
 * There are two 8-bit GPIO controllers on the sensor subsystem.
 * GPIO_SS[0-7] are on GPIO_0 while GPIO_SS[8-15] are on GPIO_1.
 *
 * The gpio_dw driver is being used. The first GPIO controller
 * is mapped to device "GPIO_0", and the second GPIO controller
 * is mapped to device "GPIO_1".
 *
 * This sample app toggles GPIO_SS_2/A0. It also waits for
 * GPIO_SS_3/A1 to go high and display a message.
 *
 * If A0 and A1 are connected together, the GPIO should
 * triggers every 2 seconds. And you should see this repeatedly
 * on console (via ipm_console0):
 * "
 *     Toggling GPIO_SS_2
 *     Toggling GPIO_SS_2
 *     GPIO_SS_3 triggered
 * "
 *
 * Arduino Due
 * -----------
 *
 * On Arduino Due:
 * 1. IO_2 is PB25
 * 2. IO_13 is PB27 (linked to the LED marked "L")
 *
 * The gpio_atmel_sam3 driver is being used.
 *
 * This sample app toggles IO_2. It also waits for
 * IO_13 to go high and display a message.
 *
 * If IO_2 and IO_13 are connected together, the GPIO should
 * triggers every 2 seconds. And you should see this repeatedly
 * on console:
 * "
 *     Toggling GPIO_25
 *     Toggling GPIO_25
 *     GPIO_27 triggered
 * "
 *
 * Quark D2000 CRB
 * ---------------
 *
 * In order to this sample app work with Quark D2000 CRB you should
 * use a "jumper cable" connecting the pins as described below:
 * Header J5 pin 9
 * Header J6 pin 7
 *
 * If the app runs successfully you should see this repeatedly on
 * console:
 * "
 * Toggling GPIO_8
 * Toggling GPIO_8
 * GPIO_24 triggered
 * "
 */

#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <gpio.h>
#include <misc/util.h>

#if defined(CONFIG_SOC_QUARK_SE_C1000_SS)
#define GPIO_OUT_PIN	2
#define GPIO_INT_PIN	3
#define GPIO_NAME	"GPIO_SS_"
#elif defined(CONFIG_SOC_QUARK_SE_C1000)
#define GPIO_OUT_PIN	16
#define GPIO_INT_PIN	19
#define GPIO_NAME	"GPIO_"
#elif defined(CONFIG_SOC_PART_NUMBER_SAM3X8E)
#define GPIO_OUT_PIN	25
#define GPIO_INT_PIN	27
#define GPIO_NAME	"GPIO_"
#elif defined(CONFIG_SOC_QUARK_D2000)
#define GPIO_OUT_PIN	8
#define GPIO_INT_PIN	24
#define GPIO_NAME	"GPIO_"
#elif defined(CONFIG_SOC_CC2650)
#define GPIO_OUT_PIN	10
#define GPIO_INT_PIN	4
#define GPIO_NAME	"GPIO_"
#endif

#if defined(CONFIG_GPIO_DW_0)
#define GPIO_DRV_NAME CONFIG_GPIO_DW_0_NAME
#elif defined(CONFIG_GPIO_QMSI_0) && defined(CONFIG_SOC_QUARK_SE_C1000)
#define GPIO_DRV_NAME CONFIG_GPIO_QMSI_0_NAME
#elif defined(CONFIG_GPIO_QMSI_SS_0)
#define GPIO_DRV_NAME CONFIG_GPIO_QMSI_SS_0_NAME
#elif defined(CONFIG_GPIO_ATMEL_SAM3)
#define GPIO_DRV_NAME CONFIG_GPIO_ATMEL_SAM3_PORTB_DEV_NAME
#else
#define GPIO_DRV_NAME "GPIO_0"
#endif

void gpio_callback(struct device *port,
		   struct gpio_callback *cb, u32_t pins)
{
	printk(GPIO_NAME "%d triggered\n", GPIO_INT_PIN);
}

static struct gpio_callback gpio_cb;

void main(void)
{
	struct device *gpio_dev;
	int ret;
	int toggle = 1;

	gpio_dev = device_get_binding(GPIO_DRV_NAME);
	if (!gpio_dev) {
		printk("Cannot find %s!\n", GPIO_DRV_NAME);
		return;
	}

	/* Setup GPIO output */
	ret = gpio_pin_configure(gpio_dev, GPIO_OUT_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
	}

	/* Setup GPIO input, and triggers on rising edge. */
#ifdef CONFIG_SOC_CC2650
	ret = gpio_pin_configure(gpio_dev, GPIO_INT_PIN,
				 (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH |
				  GPIO_INT_DEBOUNCE | GPIO_PUD_PULL_UP));
#else
	ret = gpio_pin_configure(gpio_dev, GPIO_INT_PIN,
				 (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH |
				  GPIO_INT_DEBOUNCE));
#endif
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_INT_PIN);
	}

	gpio_init_callback(&gpio_cb, gpio_callback, BIT(GPIO_INT_PIN));

	ret = gpio_add_callback(gpio_dev, &gpio_cb);
	if (ret) {
		printk("Cannot setup callback!\n");
	}

	ret = gpio_pin_enable_callback(gpio_dev, GPIO_INT_PIN);
	if (ret) {
		printk("Error enabling callback!\n");
	}

	while (1) {
		printk("Toggling " GPIO_NAME "%d\n", GPIO_OUT_PIN);

		ret = gpio_pin_write(gpio_dev, GPIO_OUT_PIN, toggle);
		if (ret) {
			printk("Error set " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
		}

		if (toggle) {
			toggle = 0;
		} else {
			toggle = 1;
		}

		k_sleep(MSEC_PER_SEC);
	}
}
