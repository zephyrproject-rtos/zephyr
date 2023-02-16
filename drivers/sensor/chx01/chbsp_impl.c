/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "chirp_bsp.h"
#include "soniclib.h"
#include "chx01_common.h"

LOG_MODULE_DECLARE(CHIRPMICRO);

int chbsp_i2c_mem_write(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes)
{
	struct chx01_common_config *conf = get_common_config(dev_ptr);
	uint8_t i2c_addr = ch_get_i2c_address(dev_ptr);

	return i2c_burst_write(conf->i2c.bus, i2c_addr, mem_addr, data, num_bytes);
}

int chbsp_i2c_mem_read(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes)
{
	struct chx01_common_config *conf = get_common_config(dev_ptr);
	uint8_t i2c_addr = ch_get_i2c_address(dev_ptr);
	const uint8_t write_buff[] = {mem_addr & 0xff, (mem_addr >> 8) & 0xff};

	return i2c_write_read(conf->i2c.bus, i2c_addr, write_buff, sizeof(write_buff), data,
			      num_bytes);
}

int chbsp_i2c_write(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes)
{
	struct chx01_common_config *conf = get_common_config(dev_ptr);
	uint8_t i2c_addr = ch_get_i2c_address(dev_ptr);

	return i2c_write(conf->i2c.bus, data, num_bytes, i2c_addr);
}

int chbsp_i2c_read(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes)
{
	struct chx01_common_config *conf = get_common_config(dev_ptr);
	uint8_t i2c_addr = ch_get_i2c_address(dev_ptr);

	return i2c_read(conf->i2c.bus, data, num_bytes, i2c_addr);
}

uint8_t chbsp_i2c_get_info(ch_group_t *grp_ptr, uint8_t dev_num, ch_i2c_info_t *info_ptr)
{
	const ch_dev_t *dev_ptr = grp_ptr->device[dev_num];

	info_ptr->address = dev_ptr->app_i2c_address;
	info_ptr->bus_num = 0;
	info_ptr->drv_flags = dev_ptr->i2c_drv_flags;

	return 0;
}

void chbsp_group_set_io_dir_out(ch_group_t *grp_ptr)
{
	for (uint8_t dev_num = 0; dev_num < ch_get_num_ports(grp_ptr); ++dev_num) {
		ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);
		struct chx01_common_config *conf = get_common_config(dev_ptr);

		if (ch_sensor_is_connected(dev_ptr)) {
			gpio_pin_configure_dt(&conf->gpio_int, GPIO_OUTPUT);
		}
	}
}

void chbsp_group_set_io_dir_in(ch_group_t *grp_ptr)
{
	for (uint8_t dev_num = 0; dev_num < ch_get_num_ports(grp_ptr); ++dev_num) {
		ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);
		struct chx01_common_config *conf = get_common_config(dev_ptr);

		if (ch_sensor_is_connected(dev_ptr)) {
			gpio_pin_configure_dt(&conf->gpio_int, GPIO_INPUT);
		}
	}
}

void chbsp_group_io_set(ch_group_t *grp_ptr)
{
	for (uint8_t dev_num = 0; dev_num < ch_get_num_ports(grp_ptr); ++dev_num) {
		ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);
		struct chx01_common_config *conf = get_common_config(dev_ptr);

		if (ch_sensor_is_connected(dev_ptr)) {
			gpio_pin_set_dt(&conf->gpio_int, 1);
		}
	}
}

void chbsp_delay_us(uint32_t us)
{
	k_usleep(us);
}

void chbsp_delay_ms(uint32_t ms)
{
	k_msleep(ms);
}

uint32_t chbsp_timestamp_ms(void)
{
	return k_uptime_get_32();
}

void chbsp_group_io_clear(ch_group_t *grp_ptr)
{
	for (uint8_t dev_num = 0; dev_num < ch_get_num_ports(grp_ptr); ++dev_num) {
		ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);
		struct chx01_common_config *conf = get_common_config(dev_ptr);

		if (ch_sensor_is_connected(dev_ptr)) {
			gpio_pin_set_dt(&conf->gpio_int, 0);
		}
	}
}

void chbsp_program_enable(ch_dev_t *dev_ptr)
{
	struct chx01_common_config *conf = get_common_config(dev_ptr);

	gpio_pin_set_dt(&conf->gpio_program, 1);
}

void chbsp_program_disable(ch_dev_t *dev_ptr)
{
	struct chx01_common_config *conf = get_common_config(dev_ptr);

	gpio_pin_set_dt(&conf->gpio_program, 0);
}

int chbsp_i2c_init(void)
{
	return 0;
}

void chbsp_i2c_reset(ch_dev_t *dev_ptr)
{
	ARG_UNUSED(dev_ptr);
}

void chbsp_reset_assert(ch_group_t *grp_ptr)
{
	for (uint8_t dev_num = 0; dev_num < ch_get_num_ports(grp_ptr); ++dev_num) {
		ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);
		struct chx01_common_config *conf = get_common_config(dev_ptr);

		gpio_pin_set_dt(&conf->gpio_reset, 0);
	}
}

void chbsp_reset_release(ch_group_t *grp_ptr)
{
	for (uint8_t dev_num = 0; dev_num < ch_get_num_ports(grp_ptr); ++dev_num) {
		ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);
		struct chx01_common_config *conf = get_common_config(dev_ptr);

		gpio_pin_set_dt(&conf->gpio_reset, 1);
	}
}

void chbsp_group_pin_init(ch_group_t *grp_ptr)
{
	/* Configure the PROGRAM and RESET pins */
	for (uint8_t dev_num = 0; dev_num < ch_get_num_ports(grp_ptr); ++dev_num) {
		ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);
		struct chx01_common_config *conf = get_common_config(dev_ptr);

		gpio_pin_configure_dt(&conf->gpio_program, GPIO_OUTPUT_LOW);
		gpio_pin_configure_dt(&conf->gpio_reset, GPIO_OUTPUT);
	}

	/* Assert the reset pin */
	chbsp_reset_assert(grp_ptr);

	/* Enable all the program pins */
	for (uint8_t dev_num = 0; dev_num < ch_get_num_ports(grp_ptr); ++dev_num) {
		ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);

		chbsp_program_enable(dev_ptr);
	}

	/* Initialize IO pins */
	chbsp_group_set_io_dir_in(grp_ptr);
}

void chbsp_debug_toggle(uint8_t dbg_pin_num)
{
}

void chbsp_debug_on(uint8_t dbg_pin_num)
{
}

void chbsp_debug_off(uint8_t dbg_pin_num)
{
}
