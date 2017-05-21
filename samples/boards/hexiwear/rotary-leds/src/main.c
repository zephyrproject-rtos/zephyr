/*
 * Copyright (c) 2017 david d zuhn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <gpio.h>
#include <spi.h>
#include <misc/printk.h>

#include "board.h"

/* When I use the Rotary-G click board in slot 3, I find that the position
 * of the knob affects whether or not I can flash a new image in with my
 * JLink.  That's pretty odd, I know, but it's completely repeatable.
 *
 * So I use slot #2 instead, and everything works as I expect.
 */

#define ROTARY_A_NAME	CLICK2_PWM_NAME
#define ROTARY_A_PIN	CLICK2_PWM_PIN

#define ROTARY_B_NAME	CLICK2_AN_NAME
#define ROTARY_B_PIN	CLICK2_AN_PIN

#define ROTARY_BUTTON_NAME   CLICK2_INT_NAME
#define ROTARY_BUTTON_PIN    CLICK2_INT_PIN

#define ROTARY_SPI_SELECT    CLICK2_SPI_SELECT

static struct gpio_callback rotaryA_cb;
static struct gpio_callback rotaryB_cb;
static struct gpio_callback rotaryButton_cb;

struct device *rotaryA;
struct device *rotaryB;
struct device *button;

struct device *spi_dev;

struct spi_config config = { 0 };

volatile int speed;
volatile int direction = 1;

bool buttonPressed;
s64_t lastButtonPress;

volatile int rotation;
s64_t lastSpeedChange;


void changeDirection(void)
{
	direction = -direction;
}

void button_callback(struct device *port, struct gpio_callback *cb, u32_t pins)
{
	u32_t deltaT = 0;
	bool firstPress = (lastButtonPress == 0);

	if (firstPress) {
		lastButtonPress = k_uptime_get_32();
	} else {
		deltaT = k_uptime_delta_32(&lastButtonPress);
	}

	if (firstPress || (deltaT > 0)) {
		buttonPressed = !buttonPressed;

		if (buttonPressed) {
			changeDirection();
		}
	}

}

int addSpeed(int currentSpeed, int delta)
{
	int newSpeed = currentSpeed + delta;

	if (newSpeed < 0) {
		newSpeed = 0;
	}
	if (newSpeed > 99) {
		newSpeed = 99;
	}

	return newSpeed;
}

void read_rotary(bool aFirst)
{
	int encA, encB;
	int deltaV = rotation;
	u32_t deltaT = 0;
	bool firstTick = (lastSpeedChange == 0);

	/* I don't understand why but when this printk is removed, reading
	 * the encoder fails miserably (erratic behavior)
	 */

	printk("Read encoder: trigger %c\n", aFirst ? 'A' : 'B');

	gpio_pin_read(rotaryA, ROTARY_A_PIN, &encA);
	gpio_pin_read(rotaryB, ROTARY_B_PIN, &encB);

	if (encA != encB) {
		/* first time through, we don't know the direction yet
		 * but we know based on which GPIO triggered first
		 */
		if (rotation == 0) {
			rotation = aFirst ? (+1) : (-1);
		}
		return;
	}

	/* this is now the second time around, and we effect the change
	 * the speed will be changed, by an amount that varies depending on
	 * how quickly the knob is rotated
	 */

	if (firstTick) {
		lastSpeedChange = k_uptime_get_32();
	} else {
		deltaT = k_uptime_delta_32(&lastSpeedChange);

		if (deltaT < 150) {
			deltaV = rotation * 4;
		} else if (deltaT < 400) {
			deltaV = rotation * 3;
		} else if (deltaT < 750) {
			deltaV = rotation * 2;
		}
	}

	speed = addSpeed(speed, deltaV);
	printk("   speed:%d deltaT:%d deltaV:%d\n", speed, deltaT, deltaV);

	/* and now we reset back to the "normal" state */
	rotation = 0;
}

void rotaryA_callback(struct device *port, struct gpio_callback *cb, u32_t pins)
{
	read_rotary(true);
}

void rotaryB_callback(struct device *port, struct gpio_callback *cb, u32_t pins)
{
	read_rotary(false);
}

void write_leds(u16_t val)
{
	int err = 0;

	spi_slave_select(spi_dev, ROTARY_SPI_SELECT);
	err = spi_write(spi_dev, &val, 2);
	if (err != 0) {
		printk("Unable to perform SPI write: %d\n", err);
	}
}

void tick_leds(void)
{
	static u8_t count;
	static u16_t val = 1;

	if (speed == 0) {
		return;
	} else {
		if (direction == 1) {
			count = (count == 15) ? 0 : (count + 1);
		} else {
			count = (count == 0) ? 15 : (count - 1);
		}

		val = 1 << count;

		write_leds(val);
	}
}

void init_rotary(void)
{
	int ret;
	u32_t rotary_mode, button_mode;

	rotaryA = device_get_binding(ROTARY_A_NAME);
	rotaryB = device_get_binding(ROTARY_B_NAME);
	button = device_get_binding(ROTARY_BUTTON_NAME);

	if (!rotaryA || !rotaryB) {
		printk("Unable to initialize devices\n");
		return;
	} else {
		printk("Devices initialized\n");
	}

	rotary_mode = GPIO_DIR_IN | GPIO_INT |
		      GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE;
	/* Setup GPIO input, and triggers on rising and falling edges. */
	ret = gpio_pin_configure(rotaryA, ROTARY_A_PIN, rotary_mode);
	if (ret) {
		printk("Error configuring %s:%d!\n", ROTARY_A_NAME,
						     ROTARY_A_PIN);
	} else {
		printk("Pin A configured\n");
	}

	/* Setup GPIO input, and triggers on rising and falling edges. */
	ret = gpio_pin_configure(rotaryB, ROTARY_B_PIN, rotary_mode);
	if (ret) {
		printk("Error configuring %s:%d!\n", ROTARY_B_NAME,
						     ROTARY_B_PIN);
	} else {
		printk("Pin B configured\n");
	}

	button_mode = GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
		      GPIO_INT_DOUBLE_EDGE | GPIO_PUD_PULL_DOWN |
		      GPIO_INT_DEBOUNCE;

	ret = gpio_pin_configure(button, ROTARY_BUTTON_PIN, button_mode);
	if (ret) {
		printk("Error configuring %s:%d!\n", ROTARY_BUTTON_NAME,
						     ROTARY_BUTTON_PIN);
	} else {
		printk("Pin BUTTON configured\n");
	}

	gpio_init_callback(&rotaryA_cb, rotaryA_callback, BIT(ROTARY_A_PIN));

	gpio_init_callback(&rotaryB_cb, rotaryB_callback, BIT(ROTARY_B_PIN));

	ret = gpio_add_callback(rotaryA, &rotaryA_cb);
	if (ret) {
		printk("Cannot setup callback!\n");
	} else {
		printk("Callback added\n");
	}

	ret = gpio_add_callback(rotaryB, &rotaryB_cb);
	if (ret) {
		printk("Cannot setup callback B!\n");
	} else {
		printk("Callback B added\n");
	}

	ret = gpio_pin_enable_callback(rotaryA, ROTARY_A_PIN);
	if (ret) {
		printk("Error enabling callback!\n");
	} else {
		printk("Callback enabled\n");
	}

	ret = gpio_pin_enable_callback(rotaryB, ROTARY_B_PIN);
	if (ret) {
		printk("Error enabling callback B!\n");
	} else {
		printk("Callback B enabled\n");
	}

	/* one callback for the push-to-activate knob on the click board */

	gpio_init_callback(&rotaryButton_cb, button_callback,
			   BIT(ROTARY_BUTTON_PIN));

	ret = gpio_add_callback(button, &rotaryButton_cb);
	if (ret) {
		printk("Cannot setup button callback!\n");
	} else {
		printk("button Callback added\n");
	}

	ret = gpio_pin_enable_callback(button, ROTARY_BUTTON_PIN);
	if (ret) {
		printk("Error enabling callback!\n");
	} else {
		printk("button Callback enabled\n");
	}

}

void init_spi(void)
{
	int ret;

	spi_dev = device_get_binding(CONFIG_SPI_0_NAME);
	config.config = SPI_WORD(8);
	config.max_sys_freq = 256;

	ret = spi_configure(spi_dev, &config);
	if (ret) {
		printk("SPI configuration failed\n");
		return;
	} else {
		printk("SPI %s configured\n", CONFIG_SPI_0_NAME);
	}
}

void init_leds(void)
{
	write_leds(1);
}

void main(void)
{
	printk("Hello World! CONFIG_ARCH=%s\n", CONFIG_ARCH);

	init_spi();
	init_rotary();
	init_leds();

	while (1) {

		if (speed != 0) {
			int delay = 1000 - speed * 10;

			if (delay < 1) {
				delay = 1;
			}
			tick_leds();
			k_sleep(delay);

		} else {
			k_sleep(5);
		}
	}
}
