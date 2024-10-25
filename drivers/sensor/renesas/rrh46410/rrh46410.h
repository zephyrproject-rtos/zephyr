/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_RRH46410_RRH46410_H_
#define ZEPHYR_DRIVERS_SENSOR_RRH46410_RRH46410_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#define RRH46410_OPERATION_MODE_IAQ_2ND_GEN	0x01
#define RRH46410_GET_OPERATION_MODE			0x10
#define RRH46410_SET_OPERATION_MODE			0x11
#define RRH46410_SET_HUMIDITY				0x12
#define RRH46410_GET_MEASUREMENT_RESULTS	0x18

/*1 byte status, 1 byte sample counter, 8 bytes data, 1 byte checksum*/
#define RRH46410_BUFFER_LENGTH 11

#define RRH46410_MAX_RESPONSE_DELAY 150 /* Add margin to the specified 50 in datasheet */

union rrh46410_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(uart)
	const struct device *uart_dev;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(uart) */
};

struct rrh46410_config {
	int (*bus_init)(const struct device *dev);
	const union rrh46410_bus_cfg bus_cfg;
	struct gpio_dt_spec reset_gpio;
};

struct rrh46410_transfer_function {
	int (*read_data)(const struct device *dev, uint8_t *rx_buff, size_t data_size);
	int (*write_data)(const struct device *dev, uint8_t *command_data, size_t data_size);
};

struct rrh46410_data {
	struct k_mutex uart_mutex;
	struct k_sem uart_rx_sem;
	uint8_t read_index;
	uint8_t read_buffer[RRH46410_BUFFER_LENGTH];
	uint8_t sample_counter;
	uint8_t iaq_sample;
	uint16_t tvoc_sample;
	uint16_t etoh_sample;
	uint16_t eco2_sample;
	uint8_t reliaq_sample;
	const struct rrh46410_transfer_function *hw_tf;
};

int rrh46410_i2c_init(const struct device *dev);
int rrh46410_uart_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_RRH46410_RRH46410_H_ */
