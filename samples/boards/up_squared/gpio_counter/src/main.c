/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <board.h>
#include <gpio.h>

#include <misc/printk.h>
#include <misc/util.h>

/**
 * @file
 *
 * @brief Example of using GPIOs on UP Squared board
 *
 * This example outputs the value of a counter via 4 GPIO lines
 * as a 4-bit value (bin 0, 1, 2, 3 -> HAT Pin 35, 37, 38, 40).
 * The counter increments for each change from 0 to 1 on HAT Pin 16.
 *
 * Note:
 * Need to change the BIOS settings:
 * () Advanced -> HAT Configurations:
 *    - HD-Audio / I2S6 Selec -> Disabled
 *    - GPIO / PWM3 Selection -> GPIO
 *    - GPIO / I2S2 Selection -> GPIO
 *
 *    - GPIO 19 (Pin16) Confi -> Input
 *
 *    - GPIO 14 (Pin35) Confi -> Output
 *    - GPIO 15 (Pin37) Confi -> Output
 *    - GPIO 27 (Pin38) Confi -> Output
 *    - GPIO 28 (Pin40) Confi -> Output
 */

struct _pin {
	u32_t	hat_num;
	u32_t	pin;
};

struct _pin counter_pins[] = {
	{ 35, UP2_HAT_PIN_35 },
	{ 37, UP2_HAT_PIN_37 },
	{ 38, UP2_HAT_PIN_38 },
	{ 40, UP2_HAT_PIN_40 },
};

struct _pin intr_pin = { 16, UP2_HAT_PIN_16 };

static struct gpio_callback gpio_cb;

static volatile u32_t counter;

K_SEM_DEFINE(counter_sem, 0, 1);

#define INTR_PIN_FLAGS	\
	(GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH | \
	 GPIO_INT_DEBOUNCE | GPIO_PUD_PULL_DOWN)

#define NUM_PINS	ARRAY_SIZE(counter_pins)
#define MASK		(BIT(NUM_PINS) - 1)

#define GPIO_DEV	CONFIG_APL_GPIO_LABEL

void button_cb(struct device *gpiodev, struct gpio_callback *cb, u32_t pin)
{
	counter++;
	k_sem_give(&counter_sem);
}

void main(void)
{
	struct device *gpiodev = device_get_binding(GPIO_DEV);
	u32_t val;
	int i, ret;

	if (!gpiodev) {
		printk("ERROR: cannot get device binding for %s\n",
		       GPIO_DEV);
		return;
	}

	/* Set pins to output */
	for (i = 0; i < NUM_PINS; i++) {
		ret = gpio_pin_configure(gpiodev, counter_pins[i].pin,
					 GPIO_DIR_OUT);
		if (ret) {
			printk("ERROR: cannot set HAT pin %d to OUT (%d)\n",
			       counter_pins[i].hat_num, ret);
			return;
		}
	}

	/* Setup input pin */
	ret = gpio_pin_configure(gpiodev, intr_pin.pin, INTR_PIN_FLAGS);
	if (ret) {
		printk("ERROR: cannot set HAT pin %d to OUT (%d)\n",
			       intr_pin.hat_num, ret);
			return;
	}

	gpio_init_callback(&gpio_cb, button_cb, intr_pin.pin);
	gpio_add_callback(gpiodev, &gpio_cb);
	gpio_pin_enable_callback(gpiodev, intr_pin.pin);

	/* main loop */
	val = 0;
	while (1) {
		printk("counter: 0x%x\n", val);

		for (i = 0; i < NUM_PINS; i++) {
			ret = gpio_pin_write(gpiodev, counter_pins[i].pin,
					     (val & BIT(i)));
			if (ret) {
				printk("ERROR: cannot set HAT pin %d value (%d)\n",
				       counter_pins[i].hat_num, ret);
				return;
			}
		}

		k_sem_take(&counter_sem, K_FOREVER);
		val = counter & MASK;
	};
}
