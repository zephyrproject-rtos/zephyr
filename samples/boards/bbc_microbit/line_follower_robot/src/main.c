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
#define I2C0_LABEL DT_LABEL(DT_NODELABEL(i2c0))
#define EXT_P13_GPIO_PIN 23     /* P13, SPI1 SCK */
#define EXT_P14_GPIO_PIN 22     /* P14, SPI1 MISO */

static const struct device *gpio;
const struct device *i2c_dev;
unsigned int left_line[1];
unsigned int right_line[1];
unsigned char buf[3];
unsigned char speed_hex[1];

/* Setup gpio of the microbit board */
static void line_detection(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	left_line[0] = gpio_pin_get_raw(gpio, EXT_P13_GPIO_PIN);
	right_line[0] = gpio_pin_get_raw(gpio, EXT_P14_GPIO_PIN);
	/* printk("%d  %d\n", left_line[0], right_line[0]); */
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
	if ((left_line[0] == 0) && (right_line[0] == 0)) {
		motor_left_control(200);
		motor_right_control(200);
	} else {
		if ((left_line[0] == 0) && (right_line[0] == 1)) {
			motor_left_control(0);
			motor_right_control(200);
			if ((left_line[0] == 1) && (right_line[0] == 1)) {
				motor_left_control(0);
				motor_right_control(200);
			}
		} else {
			if ((left_line[0] == 1) && (right_line[0] == 0)) {
				motor_left_control(200);
				motor_right_control(0);
				if ((left_line[0] == 1) &&
					(right_line[0] == 1)) {
					motor_left_control(200);
					motor_right_control(0);
				}
				if ((left_line[0] == 1) &&
					(right_line[0] == 0)) {
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
	static struct gpio_callback line_sensors;

	gpio = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));
	i2c_dev = device_get_binding(I2C0_LABEL);
	/* Setup gpio to read data from digital line sensors of the robot */
	gpio_pin_configure(gpio, EXT_P13_GPIO_PIN, GPIO_INPUT);
	gpio_pin_configure(gpio, EXT_P14_GPIO_PIN, GPIO_INPUT);

	gpio_pin_interrupt_configure(gpio, EXT_P13_GPIO_PIN,
				     GPIO_INT_EDGE_BOTH);

	gpio_pin_interrupt_configure(gpio, EXT_P14_GPIO_PIN,
				     GPIO_INT_EDGE_BOTH);

	gpio_init_callback(&line_sensors, line_detection,
			   BIT(EXT_P13_GPIO_PIN) | BIT(EXT_P14_GPIO_PIN));

	gpio_add_callback(gpio, &line_sensors);

	while (1) {
		line_follow();
	}
}
