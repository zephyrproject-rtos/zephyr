/*
 * Copyright (c) 2022 Trackunit A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

#define DT_DRV_COMPAT dummy_driver
#define MAGIC_NUMBER 3421

/* Shared data struct */
struct dummy_data {
	uint32_t magic;
};

/* API implementations shared for testing */
const struct uart_driver_api dummy_uart_api;
const struct i2c_driver_api dummy_i2c_api;
const struct spi_driver_api dummy_spi_api;

/* Extern counter used to verify driver init run count */
int dummy_driver_init_run_cnt;

/* Init */
static int dummy_init(const struct device *dev)
{
	dummy_driver_init_run_cnt++;
	return 0;
}

/* Instanciation macro */
#define ZEPHYR_DUMMY_DEVICE(id)                                 \
	static struct dummy_data dummy_data_##id = {                \
		.magic = MAGIC_NUMBER,                                  \
	};                                                          \
	                                                            \
	DEVICE_DT_INST_NEW_DEFINE(id, dummy_init, NULL,             \
		&dummy_data_##id, NULL, POST_KERNEL, 99,                \
		DEVICE_API(&dummy_i2c_api, i2c),                        \
		DEVICE_API(&dummy_uart_api, uart),                      \
		DEVICE_API(&dummy_spi_api, spi));

DT_INST_FOREACH_STATUS_OKAY(ZEPHYR_DUMMY_DEVICE)
