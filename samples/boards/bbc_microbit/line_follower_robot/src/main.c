/*
 * Copyright (c) 2019 Maksim Masalski maxxliferobot@gmail.com
 * A Line follower robot program using DFRobot Maqueen Robot and microbit
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

#define I2C_SLV_ADDR 0x10

enum motor {
	MOTOR_LEFT,
	MOTOR_RIGHT,
};

static const struct gpio_dt_spec left_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), left_gpios);
static const struct gpio_dt_spec right_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), right_gpios);

static struct gpio_callback left_cb;
static struct gpio_callback right_cb;

const struct device *i2c_dev;
static int left_line;
static int right_line;

static void left_irq(const struct device *dev, struct gpio_callback *cb,
		     uint32_t pins)
{
	left_line = gpio_pin_get_dt(&left_gpio);
}

static void right_irq(const struct device *dev, struct gpio_callback *cb,
		      uint32_t pins)
{
	right_line = gpio_pin_get_dt(&right_gpio);
}

static void motor_control(enum motor motor, int16_t speed)
{
	uint8_t buf[3];

	if (motor == MOTOR_LEFT) {
		buf[0] = 0x00U;
	} else {
		buf[0] = 0x02U;
	}

	if (speed < 0) {
		buf[1] = 0x01U;
		buf[2] = (uint8_t)(speed * (-1));
	} else {
		buf[1] = 0x00U;
		buf[2] = (uint8_t)speed;
	}

	i2c_write(i2c_dev, buf, 3, 0x10);
}

/* Line follower algorithm for the robot */
static void line_follow(void)
{
	if ((left_line == 0) && (right_line == 0)) {
		motor_control(MOTOR_LEFT, 200);
		motor_control(MOTOR_RIGHT, 200);
	} else {
		if ((left_line == 0) && (right_line == 1)) {
			motor_control(MOTOR_LEFT, 0);
			motor_control(MOTOR_RIGHT, 200);
			if ((left_line == 1) && (right_line == 1)) {
				motor_control(MOTOR_LEFT, 0);
				motor_control(MOTOR_RIGHT, 200);
			}
		} else {
			if ((left_line == 1) && (right_line == 0)) {
				motor_control(MOTOR_LEFT, 200);
				motor_control(MOTOR_RIGHT, 0);
				if ((left_line == 1) &&
					(right_line == 1)) {
					motor_control(MOTOR_LEFT, 200);
					motor_control(MOTOR_RIGHT, 0);
				}
				if ((left_line == 1) && (right_line == 0)) {
					motor_control(MOTOR_LEFT, 200);
				} else {
					motor_control(MOTOR_RIGHT, 0);
				}
			}
		}
	}
}

void main(void)
{
	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

	if (!device_is_ready(left_gpio.port) ||
	    !device_is_ready(right_gpio.port)) {
		printk("Left/Right GPIO controllers not ready.\n");
		return;
	}

	if (!device_is_ready(i2c_dev)) {
		printk("%s: device not ready.\n", i2c_dev->name);
		return;
	}

	/* Setup gpio to read data from digital line sensors of the robot */
	gpio_pin_configure_dt(&left_gpio, GPIO_INPUT);
	gpio_pin_configure_dt(&right_gpio, GPIO_INPUT);

	gpio_pin_interrupt_configure_dt(&left_gpio, GPIO_INT_EDGE_BOTH);
	gpio_pin_interrupt_configure_dt(&right_gpio, GPIO_INT_EDGE_BOTH);

	gpio_init_callback(&left_cb, left_irq, BIT(left_gpio.pin));
	gpio_add_callback(left_gpio.port, &left_cb);
	gpio_init_callback(&right_cb, right_irq, BIT(right_gpio.pin));
	gpio_add_callback(right_gpio.port, &right_cb);

	while (1) {
		line_follow();
	}
}
