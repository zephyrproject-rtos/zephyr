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

static const struct gpio_dt_spec left_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), left_gpios);
static const struct gpio_dt_spec right_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), right_gpios);

static struct gpio_callback left_cb;
static struct gpio_callback right_cb;

const struct device *i2c_dev;
static int left_line;
static int right_line;
unsigned char buf[3];
unsigned char speed_hex[1];

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

/* Function to convert decimal speed value to hex speed value */
/* It makes possible to transfer that value using I2C bus */
int decimal_to_hex(int speed_decimal)
{
	speed_hex[0] = (speed_decimal & 0x000000FF);
	return speed_hex[0];
}

/* Function to control motors of the DFRobot Maqueen Robot */
/* Send value > 0 motor rotates forward */
/* Send 0 motor stop */
/* Send value < 0 motor rotates backward */
void motor_left_control(int left_speed)
{
	if (left_speed < 0) {
		left_speed = left_speed * (-1);
		/* Command bits to control I2C motordriver of the robot */
		buf[0] = 0x00;
		buf[1] = 0x01;
		buf[2] = decimal_to_hex(left_speed);
	} else {
		buf[0] = 0x00;
		buf[1] = 0x00;
		buf[2] = decimal_to_hex(left_speed);
	}
	/* Left motor write data*/
	/* Address of the I2C motordriver on the robot is 0x10 */
	i2c_write(i2c_dev, buf, 3, 0x10);
}

void motor_right_control(int right_speed)
{
	if (right_speed < 0) {
		right_speed = right_speed * (-1);
		buf[0] = 0x02;
		buf[1] = 0x01;
		buf[2] = decimal_to_hex(right_speed);
	} else {
		buf[0] = 0x02;
		buf[1] = 0x00;
		buf[2] = decimal_to_hex(right_speed);
	}

/* Right motor write data*/
	i2c_write(i2c_dev, buf, 3, 0x10);
}

/* Line follower algorithm for the robot */
void line_follow(void)
{
	if ((left_line == 0) && (right_line == 0)) {
		motor_left_control(200);
		motor_right_control(200);
	} else {
		if ((left_line == 0) && (right_line == 1)) {
			motor_left_control(0);
			motor_right_control(200);
			if ((left_line == 1) && (right_line == 1)) {
				motor_left_control(0);
				motor_right_control(200);
			}
		} else {
			if ((left_line == 1) && (right_line == 0)) {
				motor_left_control(200);
				motor_right_control(0);
				if ((left_line == 1) &&
					(right_line == 1)) {
					motor_left_control(200);
					motor_right_control(0);
				}
				if ((left_line == 1) && (right_line == 0)) {
					motor_left_control(200);
				} else {
					motor_right_control(0);
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
