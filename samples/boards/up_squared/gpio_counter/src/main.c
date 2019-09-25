/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <board.h>
#include <soc.h>
#include <drivers/gpio.h>

#include <sys/printk.h>
#include <sys/util.h>

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
	u32_t		hat_num;
	u32_t		pin;
	const char	*gpio_dev_name;
	struct device	*gpio_dev;
};

struct _pin counter_pins[] = {
	{
		.hat_num = 35,
		.pin = UP2_HAT_PIN_35,
		.gpio_dev_name = UP2_HAT_PIN_35_DEV,
	},
	{
		.hat_num = 37,
		.pin = UP2_HAT_PIN_37,
		.gpio_dev_name = UP2_HAT_PIN_37_DEV,
	},
	{
		.hat_num = 38,
		.pin = UP2_HAT_PIN_38,
		.gpio_dev_name = UP2_HAT_PIN_38_DEV,
	},
	{
		.hat_num = 40,
		.pin = UP2_HAT_PIN_40,
		.gpio_dev_name = UP2_HAT_PIN_40_DEV,
	},
};

struct _pin intr_pin = {
	.hat_num = 16,
	.pin = UP2_HAT_PIN_16,
	.gpio_dev_name = UP2_HAT_PIN_16_DEV,
};

static struct gpio_callback gpio_cb;

static volatile u32_t counter;

K_SEM_DEFINE(counter_sem, 0, 1);

#define INTR_PIN_FLAGS	\
	(GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH | \
	 GPIO_INT_DEBOUNCE | GPIO_PUD_PULL_DOWN)

#define NUM_PINS	ARRAY_SIZE(counter_pins)
#define MASK		(BIT(NUM_PINS) - 1)

#define GPIO_DEV	DT_APL_GPIO_LABEL

void button_cb(struct device *gpiodev, struct gpio_callback *cb, u32_t pin)
{
	counter++;
	k_sem_give(&counter_sem);
}

int get_gpio_dev(struct _pin *pin)
{
	pin->gpio_dev = device_get_binding(pin->gpio_dev_name);
	if (!pin->gpio_dev) {
		printk("ERROR: cannot get device binding for %s\n",
		       pin->gpio_dev_name);
		return -1;
	}

	return 0;
}

void main(void)
{
	u32_t val;
	int i, ret;

	for (i = 0; i < NUM_PINS; i++) {
		if (get_gpio_dev(&counter_pins[i]) != 0) {
			return;
		}
	}

	if (get_gpio_dev(&intr_pin) != 0) {
		return;
	}

	/* Set pins to output */
	for (i = 0; i < NUM_PINS; i++) {
		ret = gpio_pin_configure(counter_pins[i].gpio_dev,
					 counter_pins[i].pin,
					 GPIO_DIR_OUT);
		if (ret) {
			printk("ERROR: cannot set HAT pin %d to OUT (%d)\n",
			       counter_pins[i].hat_num, ret);
			return;
		}
	}

	/* Setup input pin */
	ret = gpio_pin_configure(intr_pin.gpio_dev, intr_pin.pin,
				 INTR_PIN_FLAGS);
	if (ret) {
		printk("ERROR: cannot set HAT pin %d to OUT (%d)\n",
			       intr_pin.hat_num, ret);
			return;
	}

	/* Callback uses pin_mask, so need bit shifting */
	gpio_init_callback(&gpio_cb, button_cb, (1 << intr_pin.pin));
	gpio_add_callback(intr_pin.gpio_dev, &gpio_cb);
	gpio_pin_enable_callback(intr_pin.gpio_dev, intr_pin.pin);

	/* main loop */
	val = 0U;
	while (1) {
		printk("counter: 0x%x\n", val);

		for (i = 0; i < NUM_PINS; i++) {
			ret = gpio_pin_write(counter_pins[i].gpio_dev,
					     counter_pins[i].pin,
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
