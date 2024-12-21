/*
 * Copyright (c) 2022 Meta
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_FPGA_FPGA_ICE40_COMMON_H_
#define ZEPHYR_SUBSYS_FPGA_FPGA_ICE40_COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

/*
 * Values in Hz, intentionally to be comparable with the spi-max-frequency
 * property from DT bindings in spi-device.yaml.
 */
#define FPGA_ICE40_SPI_HZ_MIN 1000000
#define FPGA_ICE40_SPI_HZ_MAX 25000000

#define FPGA_ICE40_CRESET_DELAY_US_MIN 1 /* 200ns absolute minimum */
#define FPGA_ICE40_CONFIG_DELAY_US_MIN 1200
#define FPGA_ICE40_LEADING_CLOCKS_MIN  8
#define FPGA_ICE40_TRAILING_CLOCKS_MIN 49

#define FPGA_ICE40_CONFIG_DEFINE(inst, derived_config_)                                            \
	BUILD_ASSERT(DT_INST_PROP(inst, spi_max_frequency) >= FPGA_ICE40_SPI_HZ_MIN);              \
	BUILD_ASSERT(DT_INST_PROP(inst, spi_max_frequency) <= FPGA_ICE40_SPI_HZ_MAX);              \
	BUILD_ASSERT(DT_INST_PROP(inst, config_delay_us) >= FPGA_ICE40_CONFIG_DELAY_US_MIN);       \
	BUILD_ASSERT(DT_INST_PROP(inst, config_delay_us) <= UINT16_MAX);                           \
	BUILD_ASSERT(DT_INST_PROP(inst, creset_delay_us) >= FPGA_ICE40_CRESET_DELAY_US_MIN);       \
	BUILD_ASSERT(DT_INST_PROP(inst, creset_delay_us) <= UINT16_MAX);                           \
	BUILD_ASSERT(DT_INST_PROP(inst, leading_clocks) >= FPGA_ICE40_LEADING_CLOCKS_MIN);         \
	BUILD_ASSERT(DT_INST_PROP(inst, leading_clocks) <= UINT8_MAX);                             \
	BUILD_ASSERT(DT_INST_PROP(inst, trailing_clocks) >= FPGA_ICE40_TRAILING_CLOCKS_MIN);       \
	BUILD_ASSERT(DT_INST_PROP(inst, trailing_clocks) <= UINT8_MAX);                            \
	                                                                                           \
	static const struct fpga_ice40_config fpga_ice40_config_##inst = {                         \
		.bus = SPI_DT_SPEC_INST_GET(inst,                                                  \
					    SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA |   \
						    SPI_WORD_SET(8) | SPI_TRANSFER_MSB,            \
					    0),                                                    \
		.creset = GPIO_DT_SPEC_INST_GET(inst, creset_gpios),                               \
		.cdone = GPIO_DT_SPEC_INST_GET(inst, cdone_gpios),                                 \
		.config_delay_us = DT_INST_PROP(inst, config_delay_us),                            \
		.creset_delay_us = DT_INST_PROP(inst, creset_delay_us),                            \
		.leading_clocks = DT_INST_PROP(inst, leading_clocks),                              \
		.trailing_clocks = DT_INST_PROP(inst, trailing_clocks),                            \
		.derived_config = derived_config_,                                                 \
	}

struct fpga_ice40_data {
	uint32_t crc;
	/* simply use crc32 as info */
	char info[2 * sizeof(uint32_t) + 1];
	bool on;
	bool loaded;
	struct k_spinlock lock;
};

struct fpga_ice40_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec cdone;
	struct gpio_dt_spec creset;
	uint16_t creset_delay_us;
	uint16_t config_delay_us;
	uint8_t leading_clocks;
	uint8_t trailing_clocks;
	const void *derived_config;
};

void fpga_ice40_crc_to_str(uint32_t crc, char *s);
enum FPGA_status fpga_ice40_get_status(const struct device *dev);
int fpga_ice40_on(const struct device *dev);
int fpga_ice40_off(const struct device *dev);
int fpga_ice40_reset(const struct device *dev);
const char *fpga_ice40_get_info(const struct device *dev);
int fpga_ice40_init(const struct device *dev);

#endif /* ZEPHYR_SUBSYS_FPGA_FPGA_ICE40_COMMON_H_ */
