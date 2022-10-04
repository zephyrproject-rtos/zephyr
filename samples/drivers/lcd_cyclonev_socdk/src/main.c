/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2022, Intel Corporation
 * Description:
 * Example to use LCD Display in Cyclone V SoC FPGA devkit
 */

#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include "commands.h"
#define I2C_INST DT_NODELABEL(i2c0)
#define LCD_ADDRESS 0x28

const struct device *i2c = DEVICE_DT_GET(I2C_INST);

void send_ascii(char c)
{

	const uint8_t buf = c;
	int ret;

	ret = i2c_write(i2c, &buf, sizeof(buf), LCD_ADDRESS);

	if (ret != 0) {
		printf("Fail!\n");
	} else {
		printf("Success!\n");
	}
}

void send_string(uint8_t *str_ptr, int len)
{
	int ret;

	ret = i2c_write(i2c, str_ptr, len, LCD_ADDRESS);

	if (ret != 0) {
		printf("Fail!\n");
	} else {
		printf("Success!\n");
	}
}

void send_command_no_param(uint8_t command_id)
{
	const uint8_t buf[] = {0xFE, command_id};

	int ret;

	ret = i2c_write(i2c, (const uint8_t *) &buf, sizeof(buf), LCD_ADDRESS);

	if (ret != 0) {
		printf("Fail!\n");
	} else {
		printf("Success!\n");
	}
}

void send_command_one_param(uint8_t command_id, uint8_t param)
{
	const uint8_t buf[] = { 0xFE, command_id, param};

	int ret;

	ret = i2c_write(i2c, (const uint8_t *) &buf, sizeof(buf), LCD_ADDRESS);

	if (ret != 0) {
		printf("Fail!\n");
	} else {
		printf("Success!\n");
	}
}

void clear(void)
{
	send_command_no_param(CLEAR_SCREEN);
}

void next_line(void)
{
	send_command_one_param(SET_CURSOR, 0x40);
}

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);
	static const unsigned char string[] = "Hello world!";
	int len = strlen((const unsigned char *) &string);

	if (!device_is_ready(i2c)) {
		printf("i2c is not ready!\n");
	} else {
		printf("i2c is ready\n");
	}

	send_ascii('A');
	k_sleep(K_MSEC(1000));
	next_line();
	k_sleep(K_MSEC(10));
	send_string((uint8_t *)&string, len);

}
