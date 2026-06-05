/*
 * Copyright (c) 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal/adc_hal.h>
#include <hal/adc_oneshot_hal.h>
#include <hal/adc_periph.h>
#include <hal/adc_types.h>
#include <soc/clk_tree_defs.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>

struct adc_esp32_conf {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	adc_unit_t unit;
	uint8_t channel_count;
	const struct device *gpio_port;
#if defined(CONFIG_ADC_ESP32_DMA) && SOC_GDMA_SUPPORTED
	const struct device *dma_dev;
	uint8_t dma_channel;
#endif /* defined(CONFIG_ADC_ESP32_DMA) && SOC_GDMA_SUPPORTED */
};

struct adc_esp32_data {
	adc_oneshot_hal_ctx_t hal;
	adc_atten_t attenuation[SOC_ADC_MAX_CHANNEL_NUM];
	uint8_t resolution[SOC_ADC_MAX_CHANNEL_NUM];
	adc_cali_handle_t cal_handle[SOC_ADC_MAX_CHANNEL_NUM];
	uint16_t meas_ref_internal;
	uint16_t *buffer;
#ifdef CONFIG_ADC_ESP32_DMA
#include <hal/dma_types.h>

#define ADC_ESP32_DMA_RX_DESC_COUNT                                                                \
	((DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED + DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED -  \
	  1U) / DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED +                                        \
	 1U)

	adc_hal_dma_ctx_t adc_hal_dma_ctx;
	dma_descriptor_t dma_rx_desc[ADC_ESP32_DMA_RX_DESC_COUNT];
	uint8_t dma_buffer[DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED];
	struct k_sem dma_conv_wait_lock;
	soc_module_clk_t digi_clk_src;
#if !SOC_GDMA_SUPPORTED
	struct intr_handle_data_t *irq_handle;
#endif /* !SOC_GDMA_SUPPORTED */
	bool digi_hw_active;
#endif /* CONFIG_ADC_ESP32_DMA */
};

int adc_esp32_dma_read(const struct device *dev, const struct adc_sequence *seq);

int adc_esp32_dma_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg);

int adc_esp32_dma_init(const struct device *dev);
