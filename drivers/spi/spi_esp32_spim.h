/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_ESP32_SPIM_H_
#define ZEPHYR_DRIVERS_SPI_ESP32_SPIM_H_

struct spi_esp32_config {
	spi_dev_t *spi;
	const struct device *clock_dev;
	void (*irq_config_func)(const struct device *dev);

	struct {
		int miso_s;
		int mosi_s;
		int sclk_s;
		int csel_s;
	} signals;

	struct {
		int miso;
		int mosi;
		int sclk;
		int csel;
	} pins;

	clock_control_subsys_t clock_subsys;

	struct {
		int source;
		int line;
	} irq;
};

struct spi_esp32_data {
	struct spi_context ctx;
	spi_hal_context_t hal;
	spi_hal_timing_conf_t timing_conf;
	uint8_t dfs;
};

#endif /* ZEPHYR_DRIVERS_SPI_ESP32_SPIM_H_ */
