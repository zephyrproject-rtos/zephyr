/*
 * Copyright (c) 2026 Pavel Maloletkov.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MSPI_ESP32_H_
#define ZEPHYR_DRIVERS_MSPI_ESP32_H_

#include <zephyr/drivers/mspi.h>

#include <zephyr/drivers/pinctrl.h>
#include <hal/spi_hal.h>
#ifdef SOC_GDMA_SUPPORTED
#include <hal/gdma_hal.h>
#else
#include <soc/lldesc.h>
#endif

struct mspi_esp32_config {
	spi_dev_t *spi;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct mspi_cfg mspi_config;
	uint32_t peripheral_id;
	soc_module_clk_t clock_source;
	bool dma_enabled;
	/*
	 * dma_host is the SPI host index used for DMA slot selection:
	 *   0 → SPI2, 1 → SPI3.
	 */
	int dma_host;
#ifdef SOC_GDMA_SUPPORTED
	const struct device *dma_dev;
	uint8_t dma_tx_ch;
	uint8_t dma_rx_ch;
#else
	int dma_clk_src;
#endif
	bool line_idle_low;
	bool use_iomux;
	uint32_t duty_cycle;
	uint32_t input_delay_ns;
	uint32_t transfer_timeout;
	int irq_source;
	int irq_priority;
	int irq_flags;
};

struct mspi_esp32_data {
	spi_hal_context_t hal;
	spi_hal_dev_config_t dev_config;
	spi_hal_trans_config_t trans_config;

	struct mspi_dev_cfg mspi_dev_config;
	struct k_mutex lock;
	struct k_sem xfer_sem;

	/*
	 * True when device-level HW registers (mode/clock/CS timing) must be
	 * reprogrammed before the next transfer. Set whenever dev_config()
	 * changes CPP/CE/frequency; io_mode-only changes don't need it.
	 */
	bool dev_hw_dirty;

	bool tx_dma_configured;
	bool rx_dma_configured;

	bool cs_active;

	mspi_callback_handler_t callback;
	void *callback_ctx;
	uint32_t callback_mask;

	uint32_t clock_source_hz;
	uint32_t clock_frequency;

#ifdef SOC_GDMA_SUPPORTED
	gdma_hal_context_t hal_gdma;
#else
	lldesc_t dma_desc_tx;
	lldesc_t dma_desc_rx;
#endif
};

#endif /* ZEPHYR_DRIVERS_MSPI_ESP32_H_ */
