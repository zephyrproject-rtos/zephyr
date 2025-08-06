/*
 * Copyright (c) 2025 Pavel Maloletkov.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/clock_control.h>

#include <hal/spi_hal.h>

#ifdef SOC_GDMA_SUPPORTED
#include <hal/gdma_hal.h>
#include <hal/gdma_ll.h>
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
	int dma_host;
#if defined(SOC_GDMA_SUPPORTED)
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
};

struct mspi_esp32_data {
	spi_hal_context_t hal_ctx;
	spi_hal_config_t hal_config;
	spi_hal_dev_config_t hal_dev_config;
	struct mspi_dev_cfg mspi_dev_config;
	struct k_mutex lock;
	mspi_callback_handler_t callback;
	void *callback_ctx;
	uint32_t callback_mask;
	uint32_t clock_source_hz;
	uint32_t clock_frequency;
	spi_hal_trans_config_t trans_config;
	lldesc_t dma_desc_tx;
	lldesc_t dma_desc_rx;
#ifdef SOC_GDMA_SUPPORTED
	gdma_hal_context_t hal_gdma;
#endif
};
