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

#ifndef CONFIG_ADC_ESP32_DMA
#define ADC_CONTEXT_USES_KERNEL_TIMER
#endif

#if defined(CONFIG_ADC_ESP32_DMA) && defined(CONFIG_ADC_ESP32_STREAM)
struct rtio_iodev_sqe;
#endif

#include "adc_context.h"

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
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *seq_buf;
	uint16_t *repeat_buf;
#ifdef CONFIG_ADC_ESP32_DMA
#include <hal/dma_types.h>

#define ADC_ESP32_DMA_RX_DESC_COUNT                                                                \
	((CONFIG_ADC_ESP32_DMA_BUFFER_SIZE + DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED - 1U) /     \
		 DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED +                                       \
	 1U)

	adc_hal_dma_ctx_t adc_hal_dma_ctx;
	dma_descriptor_t dma_rx_desc[ADC_ESP32_DMA_RX_DESC_COUNT];
	uint8_t dma_buffer[CONFIG_ADC_ESP32_DMA_BUFFER_SIZE];
	struct k_sem dma_conv_wait_lock;
	soc_module_clk_t digi_clk_src;
#if !SOC_GDMA_SUPPORTED
	struct intr_handle_data_t *irq_handle;
#endif /* !SOC_GDMA_SUPPORTED */
#if defined(CONFIG_ADC_ASYNC)
	struct k_work dma_async_work;
#endif /* CONFIG_ADC_ASYNC */
#if defined(CONFIG_ADC_ESP32_STREAM)
	struct k_work stream_done_work;
	struct k_work stream_start_work;
	struct k_work stream_rtio_work;
	struct k_work_delayable stream_resubmit_dwork;
	struct rtio_iodev_sqe *stream_rtio_sqe;
	struct rtio_iodev_sqe *stream_iodev_sqe;
	struct rtio_iodev_sqe *stream_resubmit_sqe;
	int stream_rtio_err;
	struct adc_sequence stream_seq_snap;
	struct adc_sequence_options stream_seq_opt_snap;
	adc_hal_digi_ctrlr_cfg_t stream_digi_cfg;
	adc_digi_pattern_config_t stream_pattern[SOC_ADC_MAX_CHANNEL_NUM];
	uint32_t stream_num_adc_output_samples;
	uint32_t stream_num_adc_dma_samples;
	bool dma_notify_via_work;
#endif /* CONFIG_ADC_ESP32_STREAM */
	bool digi_hw_active;
#endif /* CONFIG_ADC_ESP32_DMA */
};

int adc_esp32_dma_read(const struct device *dev, const struct adc_sequence *seq);

int adc_esp32_dma_execute_read(const struct device *dev, const struct adc_sequence *seq);

int adc_esp32_dma_finish_read(const struct device *dev, const struct adc_sequence *seq);

int adc_esp32_dma_async_submit(const struct device *dev);

#if defined(CONFIG_ADC_ESP32_DMA_DECODE_VOLTAGE_CALIBRATED)
void adc_esp32_dma_calibrate_samples(struct adc_esp32_data *data, uint16_t *samples,
				     const adc_digi_pattern_config_t *pattern, uint32_t pattern_len,
				     unsigned int repeats, uint8_t resolution);
#endif /* CONFIG_ADC_ESP32_DMA_DECODE_VOLTAGE_CALIBRATED */

#if defined(CONFIG_ADC_ESP32_STREAM)
int adc_esp32_dma_stream_validate(const struct device *dev, const struct adc_sequence *seq);

int adc_esp32_dma_stream_start(const struct device *dev);

int adc_esp32_dma_stream_queue_start(const struct device *dev);

int adc_esp32_stream_arm(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

#if defined(CONFIG_ADC_ESP32_DMA_DECODE_VOLTAGE_CALIBRATED)
void adc_esp32_stream_calibrate_frame(struct adc_esp32_data *data);
#endif /* CONFIG_ADC_ESP32_DMA_DECODE_VOLTAGE_CALIBRATED */

#endif /* CONFIG_ADC_ESP32_STREAM */

int adc_esp32_dma_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg);

int adc_esp32_dma_init(const struct device *dev);
