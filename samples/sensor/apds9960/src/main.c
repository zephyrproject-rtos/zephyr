/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdio.h>
#include <device.h>
#include <gpio.h>
#include <i2c.h>
#include <sys_clock.h>
#include <misc/util.h>

#define SLEEP_MSEC	200

#define GPIO_DATA_PIN	2
#define GPIO_CLK_PIN	3
#define GPIO_NAME	"GPIO_SS_"

#define GPIO_DRV_NAME	"GPIO_0"

#define APA102C_START_FRAME	0x00000000
#define APA102C_END_FRAME	0xFFFFFFFF
#define APA102C_BRIGHTNESS	0xE2000000
#define APA102C_BRIGHTNESS_MASK	0xFF000000

#define I2C_DRV_NAME		"I2C_0"
#define APDS9960_ADDR		0x39

union rgbc_t {
	uint8_t		raw[8];
	struct {
		uint8_t		cdatal;
		uint8_t		cdatah;
		uint8_t		rdatal;
		uint8_t		rdatah;
		uint8_t		gdatal;
		uint8_t		gdatah;
		uint8_t		bdatal;
		uint8_t		bdatah;
	} ch;
};

void apa102c_rgb_send(struct device *gpio_dev, uint32_t rgb)
{
	int i;

	for (i = 0; i < 32; i++) {
		/* MSB goes in first */
		gpio_pin_write(gpio_dev, GPIO_DATA_PIN, !!(rgb & 0x80000000));

		/* Latch data into LED */
		gpio_pin_write(gpio_dev, GPIO_CLK_PIN, 1);
		gpio_pin_write(gpio_dev, GPIO_CLK_PIN, 0);

		rgb <<= 1;
	}
}

void apa102c_led_program(struct device *gpio_dev, uint32_t rgb)
{
	apa102c_rgb_send(gpio_dev, APA102C_START_FRAME);
	apa102c_rgb_send(gpio_dev, rgb);
	apa102c_rgb_send(gpio_dev, APA102C_END_FRAME);
}

int apds9960_reg_write(struct device *i2c_dev,
			uint8_t reg_addr, uint8_t reg_val)
{
	struct i2c_msg msg;
	uint8_t data[2];
	int ret;

	msg.buf = data;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	/* Enable Power (PON) so we can configure the sensor */
	data[0] = reg_addr;
	data[1] = reg_val;
	msg.len = 2;

	ret = i2c_transfer(i2c_dev, &msg, 1, APDS9960_ADDR);
	if (ret) {
		printf("Cannot write APDS9960 reg 0x%X to 0x%X\n",
		      reg_addr, reg_val);
	}

	return ret;
}

int apds9960_reg_read(struct device *i2c_dev, uint8_t reg_addr,
		       uint8_t *data, uint8_t data_len)
{
	struct i2c_msg msgs[2];
	uint8_t reg_data;
	int ret;

	/* Access RGBC data register */
	reg_data = reg_addr;
	msgs[0].buf = &reg_data;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read 8 bytes of RGBC values */
	msgs[1].buf = data;
	msgs[1].len = data_len;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_RESTART | I2C_MSG_STOP;

	ret = i2c_transfer(i2c_dev, msgs, 2, APDS9960_ADDR);
	if (ret) {
		printf("Cannot read from APDS9960 reg 0x%X\n", reg_addr);
	}

	return ret;
}

void apds9960_setup(struct device *i2c_dev, int gain)
{
	/* Enable Power (PON) so we can configure the sensor */
	apds9960_reg_write(i2c_dev, 0x80, 0x01);

	/* Max out the ADC values.
	 * So every ADC cycle is 200ms, and max
	 * ADC value is 65535.
	 */
	apds9960_reg_write(i2c_dev, 0x81, 0xB6);

	/* ALS LEDs Gain */
	apds9960_reg_write(i2c_dev, 0x8F, (gain & 0x03));

	/* Enable Power (PON) and ALS*/
	apds9960_reg_write(i2c_dev, 0x80, 0x03);
}

void apds9960_als_valid_wait(struct device *i2c_dev)
{
	uint8_t status;

	while (1) {
		apds9960_reg_read(i2c_dev, 0x93, &status, 1);

		if (status & BIT(0)) {
			break;
		}

		k_sleep(5);
	}
}

void main(void)
{
	struct device *gpio_dev, *i2c_dev;
	int ret;
	union rgbc_t rgbc;
	int led_rgb;
	int als_gain = 0;

	gpio_dev = device_get_binding(GPIO_DRV_NAME);
	if (!gpio_dev) {
		printf("Cannot find %s!\n", GPIO_DRV_NAME);
		return;
	}

	i2c_dev = device_get_binding(I2C_DRV_NAME);
	if (!i2c_dev) {
		printf("Cannot find %s!\n", I2C_DRV_NAME);
		return;
	}

	/*
	 * Setup GPIO outputs
	 */
	ret = gpio_pin_configure(gpio_dev, GPIO_DATA_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printf("Error configuring " GPIO_NAME "%d!\n", GPIO_DATA_PIN);
	}

	ret = gpio_pin_configure(gpio_dev, GPIO_CLK_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printf("Error configuring " GPIO_NAME "%d!\n", GPIO_CLK_PIN);
	}

	/*
	 * Initialize the APDS9960 sensor with 1x ALS gain
	 */
	apds9960_setup(i2c_dev, als_gain);

	while (1) {
		apds9960_als_valid_wait(i2c_dev);

		apds9960_reg_read(i2c_dev, 0x94, (uint8_t *)&rgbc, 8);

		/* Change the gain if it is too bright or too dark.
		 * Note there is no logic to prevent it from
		 * bouncing between two gain settings.
		 */
		if ((rgbc.ch.cdatah < 0x10) && (als_gain != 3)) {
			/* More gain if too dark */
			als_gain++;
			if (als_gain > 3) {
				als_gain = 3;
			}

			apds9960_setup(i2c_dev, als_gain);
			printf("GAIN ==> %d\n", als_gain);
		} else if ((rgbc.ch.cdatah > 0xEF) && (als_gain != 0)) {
			/* Less gain if too bright */
			als_gain--;
			if (als_gain < 0) {
				als_gain = 0;
			}

			apds9960_setup(i2c_dev, als_gain);
			printf("GAIN ==> %d\n", als_gain);
		} else {
			/* Only program the LED when gain settles.
			 * Or else the LED would suddenly go all
			 * bright or dark before settling to
			 * the final value.
			 */
			led_rgb = (rgbc.ch.bdatah << 16)
				  + (rgbc.ch.gdatah << 8)
				  + (rgbc.ch.rdatah)
				  + APA102C_BRIGHTNESS;

			apa102c_led_program(gpio_dev, led_rgb);
		}
	}
}
