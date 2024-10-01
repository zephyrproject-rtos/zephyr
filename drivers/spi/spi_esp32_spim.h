/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_ESP32_SPIM_H_
#define ZEPHYR_DRIVERS_SPI_ESP32_SPIM_H_

#include <zephyr/drivers/pinctrl.h>
#include <hal/spi_hal.h>
#ifdef SOC_GDMA_SUPPORTED
#include <hal/gdma_hal.h>
#endif

#define SPI_MASTER_FREQ_8M      (APB_CLK_FREQ/10)
#define SPI_MASTER_FREQ_9M      (APB_CLK_FREQ/9)    /* 8.89MHz */
#define SPI_MASTER_FREQ_10M     (APB_CLK_FREQ/8)    /* 10MHz */
#define SPI_MASTER_FREQ_11M     (APB_CLK_FREQ/7)    /* 11.43MHz */
#define SPI_MASTER_FREQ_13M     (APB_CLK_FREQ/6)    /* 13.33MHz */
#define SPI_MASTER_FREQ_16M     (APB_CLK_FREQ/5)    /* 16MHz */
#define SPI_MASTER_FREQ_20M     (APB_CLK_FREQ/4)    /* 20MHz */
#define SPI_MASTER_FREQ_26M     (APB_CLK_FREQ/3)    /* 26.67MHz */
#define SPI_MASTER_FREQ_40M     (APB_CLK_FREQ/2)    /* 40MHz */
#define SPI_MASTER_FREQ_80M     (APB_CLK_FREQ/1)    /* 80MHz */

struct spi_esp32_config {
	spi_dev_t *spi;
	const struct device *clock_dev;
	int duty_cycle;
	int input_delay_ns;
	int irq_source;
	const struct pinctrl_dev_config *pcfg;
	clock_control_subsys_t clock_subsys;
	bool use_iomux;
	bool dma_enabled;
	int dma_clk_src;
	int dma_host;
	int cs_setup;
	int cs_hold;
	bool line_idle_low;
	spi_clock_source_t clock_source;
};

struct spi_esp32_data {
	struct spi_context ctx;
	spi_hal_context_t hal;
	spi_hal_config_t hal_config;
#ifdef SOC_GDMA_SUPPORTED
	gdma_hal_context_t hal_gdma;
#endif
	spi_hal_timing_conf_t timing_config;
	spi_hal_dev_config_t dev_config;
	spi_hal_trans_config_t trans_config;
	uint8_t dfs;
	lldesc_t dma_desc_tx;
	lldesc_t dma_desc_rx;
	uint32_t clock_source_hz;
};

#endif /* ZEPHYR_DRIVERS_SPI_ESP32_SPIM_H_ */
