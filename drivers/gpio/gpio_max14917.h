/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MAX14917_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MAX14917_H_

#define MAX14917_CHANNELS 8

#define MAX14917_MAX_PKT_SIZE 2

#define MAX14917_COMM_ERR  BIT(7)
#define MAX14917_VERR      BIT(6)
#define MAX14917_THERM_ERR BIT(5)

#define MAX14917_CRC_POLY       0x15
#define MAX14917_CRC_INI_VAL    0x1F
#define MAX14917_CRC_EXTRA_BYTE 0x00
#define MAX14917_CRC_MASK       0x1F

struct max14917_config {
	struct spi_dt_spec spi;
	/* Input gpios */
	struct gpio_dt_spec vddok_gpio;
	struct gpio_dt_spec ready_gpio;
	struct gpio_dt_spec comerr_gpio;
	struct gpio_dt_spec fault_gpio;
	/* Output gpios */
	struct gpio_dt_spec en_gpio;
	struct gpio_dt_spec sync_gpio;
	struct gpio_dt_spec crcen_gpio;
	bool crc_en;
	uint8_t pkt_size;
};

struct max14917_data {
	uint8_t gpios_ON; /* GPIO states */
	uint8_t gpios_fault;
	bool comm_err;
	bool verr;
	bool therm_err;
};

#endif
