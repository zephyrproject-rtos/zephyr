/*
 * Copyright (c) 2019 Maksim Masalski maxxliferobot@gmail.com
 * A Line follower robot program using DFRobot Maqueen Robot and microbit
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

enum motor {
	MOTOR_LEFT,
	MOTOR_RIGHT,
};

static const struct gpio_dt_spec left_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), left_gpios);
static const struct gpio_dt_spec right_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), right_gpios);

static const struct i2c_dt_spec motorctl =
	I2C_DT_SPEC_GET(DT_NODELABEL(motorctl));

static struct gpio_callback left_cb;
static struct gpio_callback right_cb;

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

	i2c_write_dt(&motorctl, buf, sizeof(buf));
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

int main(void)
{
	if (!gpio_is_ready_dt(&left_gpio) ||
	    !gpio_is_ready_dt(&right_gpio)) {
		printk("Left/Right GPIO controllers not ready.\n");
		return 0;
	}

	if (!device_is_ready(motorctl.bus)) {
		printk("Motor controller I2C bus not ready.\n");
		return 0;
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
	return 0;
}
