/*
 * Copyright (c) 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_adc

#include <string.h>

#include <esp_adc/adc_cali.h>
#include <esp_clk_tree.h>
#include <esp_private/periph_ctrl.h>
#include <esp_private/sar_periph_ctrl.h>
#include <esp_private/adc_share_hw_ctrl.h>

#include "adc_esp32.h"

#if SOC_GDMA_SUPPORTED || defined(CONFIG_SOC_SERIES_ESP32)
#define ADC_ESP32_DMA_CAPTURE_FACTOR 2U
#else
#define ADC_ESP32_DMA_CAPTURE_FACTOR 1U
#endif

#include <zephyr/cache.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_esp32_dma, CONFIG_ADC_LOG_LEVEL);

#if defined(CONFIG_ADC_ESP32_STREAM)
#include <zephyr/rtio/rtio.h>

K_THREAD_STACK_DEFINE(adc_esp32_stream_rtio_wq_stack, CONFIG_ADC_ESP32_STREAM_RTIO_WQ_STACK_SIZE);
static struct k_work_q adc_esp32_stream_rtio_wq;
static bool adc_esp32_stream_rtio_wq_started;

static void adc_esp32_stream_rtio_wq_init(void)
{
	if (adc_esp32_stream_rtio_wq_started) {
		return;
	}

	(void)k_work_queue_start(&adc_esp32_stream_rtio_wq, adc_esp32_stream_rtio_wq_stack,
				 K_THREAD_STACK_SIZEOF(adc_esp32_stream_rtio_wq_stack),
				 CONFIG_ADC_ESP32_STREAM_RTIO_WQ_PRIORITY, NULL);
	adc_esp32_stream_rtio_wq_started = true;
}

static void adc_esp32_stream_dma_done_work_fn(struct k_work *work);
static void adc_esp32_stream_start_work_fn(struct k_work *work);
static void adc_esp32_stream_rtio_work_fn(struct k_work *work);
static void adc_esp32_stream_resubmit_work_fn(struct k_work *work);

#define ADC_ESP32_STREAM_RESUBMIT_RETRY_MS 2
#endif

#if SOC_GDMA_SUPPORTED
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <hal/adc_ll.h>
#elif defined(CONFIG_SOC_SERIES_ESP32)
#include <zephyr/dt-bindings/clock/esp32_clock.h>
#include <zephyr/dt-bindings/interrupt-controller/esp-xtensa-intmux.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <hal/i2s_ll.h>
#define ADC_DMA_I2S_HOST 0
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#include <zephyr/dt-bindings/clock/esp32s2_clock.h>
#include <zephyr/dt-bindings/interrupt-controller/esp32s2-xtensa-intmux.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <hal/spi_ll.h>
#include <hal/spi_types.h>
#define ADC_DMA_SPI_HOST SPI3_HOST
#endif /* SOC_GDMA_SUPPORTED */

#define ADC_DMA_BUFFER_SIZE        CONFIG_ADC_ESP32_DMA_BUFFER_SIZE
#define ADC_DMA_MAX_CONV_DONE_TIME 1000

#define UNIT_ATTEN_UNINIT UINT32_MAX

/* ADC DMA interrupt mask - I2S RX EOF for ESP32, SPI for ESP32-S2 */
#if defined(CONFIG_SOC_SERIES_ESP32)
#define ADC_DMA_INTR_MASK I2S_LL_EVENT_RX_EOF
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#define ADC_DMA_INTR_MASK SPI_LL_INTR_IN_SUC_EOF
#endif

/*
 * adc_context.h forward-declares driver callbacks referenced from static inline helpers.
 * This file only uses adc_context_* lock/complete paths; SAR samples are DMA-driven.
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	ARG_UNUSED(ctx);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(repeat_sampling);
}

static void adc_context_enable_timer(struct adc_context *ctx)
{
	ARG_UNUSED(ctx);
}

static void adc_context_disable_timer(struct adc_context *ctx)
{
	ARG_UNUSED(ctx);
}

#if SOC_GDMA_SUPPORTED

static void IRAM_ATTR adc_esp32_dma_conv_done(const struct device *dma_dev, void *user_data,
					      uint32_t channel, int status)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(status);

	const struct device *dev = user_data;
	struct adc_esp32_data *data = dev->data;

#ifdef CONFIG_ADC_ESP32_STREAM
	if (data->dma_notify_via_work) {
		(void)k_work_submit(&data->stream_done_work);
		return;
	}
#endif /* CONFIG_ADC_ESP32_STREAM */

	k_sem_give(&data->dma_conv_wait_lock);
}

static int adc_esp32_dma_config_rx(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct adc_esp32_conf *conf = dev->config;
	int err;

	struct dma_config dma_cfg = {0};
	struct dma_status dma_status = {0};
	struct dma_block_config dma_blk = {0};

	err = dma_get_status(conf->dma_dev, conf->dma_channel, &dma_status);
	if (err) {
		LOG_ERR("Unable to get dma channel[%u] status (%d)",
			(unsigned int)conf->dma_channel, err);
		return -EINVAL;
	}

	if (dma_status.busy) {
		for (int retry = 0; retry < 100 && dma_status.busy; retry++) {
			k_busy_wait(10);
			err = dma_get_status(conf->dma_dev, conf->dma_channel, &dma_status);
			if (err != 0) {
				return -EINVAL;
			}
		}
		if (dma_status.busy) {
			LOG_ERR("dma channel[%u] is busy!", (unsigned int)conf->dma_channel);
			return -EBUSY;
		}
	}

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.dma_callback = adc_esp32_dma_conv_done;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_ADC0;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = len;
	dma_blk.dest_address = (uint32_t)buf;

	err = dma_config(conf->dma_dev, conf->dma_channel, &dma_cfg);
	if (err) {
		LOG_ERR("Error configuring dma (%d)", err);
	}

	return err;
}

static int adc_esp32_dma_start(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct adc_esp32_conf *conf = dev->config;
	int err;

	err = adc_esp32_dma_config_rx(dev, buf, len);
	if (err != 0) {
		return err;
	}

	err = dma_start(conf->dma_dev, conf->dma_channel);
	if (err) {
		LOG_ERR("Error starting dma (%d)", err);
		return err;
	}

	return 0;
}

static int adc_esp32_dma_stop(const struct device *dev)
{
	const struct adc_esp32_conf *conf = dev->config;
	int err;

	err = dma_stop(conf->dma_dev, conf->dma_channel);
	if (err) {
		LOG_ERR("Error stopping dma (%d)", err);
	}

	return err;
}

#else

static IRAM_ATTR void adc_esp32_dma_intr_handler(void *arg)
{
	const struct device *dev = arg;
	struct adc_esp32_data *data = dev->data;

#if defined(CONFIG_SOC_SERIES_ESP32)
	i2s_dev_t *i2s_dev = I2S_LL_GET_HW(ADC_DMA_I2S_HOST);

	if (i2s_ll_get_intr_status(i2s_dev) & ADC_DMA_INTR_MASK) {
		i2s_ll_clear_intr_status(i2s_dev, ADC_DMA_INTR_MASK);
#ifdef CONFIG_ADC_ESP32_STREAM
		if (data->dma_notify_via_work) {
			(void)k_work_submit(&data->stream_done_work);
			return;
		}
#endif /* CONFIG_ADC_ESP32_STREAM */
		k_sem_give(&data->dma_conv_wait_lock);
	}
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
	if (spi_ll_get_intr(SPI_LL_GET_HW(SPI3_HOST), ADC_DMA_INTR_MASK)) {
		spi_ll_clear_intr(SPI_LL_GET_HW(SPI3_HOST), ADC_DMA_INTR_MASK);
#ifdef CONFIG_ADC_ESP32_STREAM
		if (data->dma_notify_via_work) {
			(void)k_work_submit(&data->stream_done_work);
			return;
		}
#endif /* CONFIG_ADC_ESP32_STREAM */
		k_sem_give(&data->dma_conv_wait_lock);
	}
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */
}

#endif /* SOC_GDMA_SUPPORTED */

static int adc_esp32_fill_digi_ctrlr_cfg(const struct device *dev, const struct adc_sequence *seq,
					 uint32_t sample_freq_hz,
					 adc_digi_pattern_config_t *pattern_config,
					 adc_hal_digi_ctrlr_cfg_t *adc_hal_digi_ctrlr_cfg,
					 uint32_t *pattern_len, uint32_t *unit_attenuation)
{
	const struct adc_esp32_conf *conf = dev->config;
	struct adc_esp32_data *data = dev->data;

	uint32_t channels_copy = seq->channels;

	*pattern_len = 0;
	*unit_attenuation = UNIT_ATTEN_UNINIT;

	while (channels_copy != 0U) {
		uint8_t channel_id = find_lsb_set(channels_copy) - 1U;

		channels_copy &= ~BIT(channel_id);

		if (*unit_attenuation == UNIT_ATTEN_UNINIT) {
			*unit_attenuation = data->attenuation[channel_id];
		} else if (*unit_attenuation != data->attenuation[channel_id]) {
			LOG_ERR("Channel[%u] attenuation different of unit[%u] attenuation",
				(unsigned int)channel_id, (unsigned int)conf->unit);
			return -EINVAL;
		}

		pattern_config[*pattern_len] = (adc_digi_pattern_config_t){
			.atten = data->attenuation[channel_id],
			.channel = channel_id,
			.unit = conf->unit,
			.bit_width = seq->resolution,
		};

		*pattern_len += 1;
		if (*pattern_len > SOC_ADC_PATT_LEN_MAX) {
			LOG_ERR("Max pattern len is %d", SOC_ADC_PATT_LEN_MAX);
			return -EINVAL;
		}
	}

	soc_module_clk_t clk_src = ADC_DIGI_CLK_SRC_DEFAULT;
	uint32_t clk_src_freq_hz = 0;

	int err = esp_clk_tree_src_get_freq_hz(clk_src, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
					       &clk_src_freq_hz);
	if (err != ESP_OK) {
		return -EIO;
	}

	adc_hal_digi_ctrlr_cfg->conv_mode =
		(conf->unit == ADC_UNIT_1) ? ADC_CONV_SINGLE_UNIT_1 : ADC_CONV_SINGLE_UNIT_2;
	adc_hal_digi_ctrlr_cfg->clk_src = clk_src;
	adc_hal_digi_ctrlr_cfg->clk_src_freq_hz = clk_src_freq_hz;
	adc_hal_digi_ctrlr_cfg->sample_freq_hz = sample_freq_hz;
	adc_hal_digi_ctrlr_cfg->adc_pattern = pattern_config;
	adc_hal_digi_ctrlr_cfg->adc_pattern_len = *pattern_len;

	return 0;
}

static uint32_t adc_esp32_dma_data_bytes(uint32_t dma_samples)
{
	return dma_samples * SOC_ADC_DIGI_DATA_BYTES_PER_CONV;
}

static int adc_esp32_dma_sample_freq_hz(const struct adc_sequence_options *options,
					uint32_t *sample_freq_hz)
{
	*sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_HIGH;

	if (options == NULL) {
		return 0;
	}

	if (options->callback != NULL) {
		return -ENOTSUP;
	}

	if (options->interval_us == 0U) {
		return 0;
	}

	*sample_freq_hz = MHZ(1) / options->interval_us;
	if ((*sample_freq_hz < SOC_ADC_SAMPLE_FREQ_THRES_LOW) ||
	    (*sample_freq_hz > SOC_ADC_SAMPLE_FREQ_THRES_HIGH)) {
		LOG_ERR("ADC sampling frequency out of range: %uHz", *sample_freq_hz);
		return -EINVAL;
	}

	return 0;
}

static void adc_esp32_dma_sample_counts(uint32_t number_of_samplings, uint32_t pattern_len,
					uint32_t *output_samples, uint32_t *dma_samples,
					uint32_t *dma_data_bytes)
{
	*output_samples = number_of_samplings * pattern_len;
	*dma_samples = *output_samples * ADC_ESP32_DMA_CAPTURE_FACTOR;
	*dma_data_bytes = adc_esp32_dma_data_bytes(*dma_samples);
}

#ifdef CONFIG_ADC_ESP32_STREAM
int adc_esp32_dma_stream_validate(const struct device *dev, const struct adc_sequence *seq)
{
	int err = 0;
	uint32_t adc_pattern_len;
	uint32_t unit_attenuation;
	adc_hal_digi_ctrlr_cfg_t adc_hal_digi_ctrlr_cfg_placeholder;
	adc_digi_pattern_config_t adc_digi_pattern_config[SOC_ADC_MAX_CHANNEL_NUM];
	const struct adc_sequence_options *options = seq->options;
	uint32_t sample_freq_hz;
	uint32_t number_of_samplings = 1U;

#if SOC_ADC_PERIPH_NUM >= 2
	const struct adc_esp32_conf *conf = dev->config;

	if (!SOC_ADC_DIG_SUPPORTED_UNIT(conf->unit)) {
		LOG_ERR("ADC2 dma mode is no longer supported, please use ADC1!");
		return -EINVAL;
	}
#endif /* SOC_ADC_PERIPH_NUM >= 2 */

	err = adc_esp32_dma_sample_freq_hz(options, &sample_freq_hz);
	if (err != 0) {
		return err;
	}

	err = adc_esp32_fill_digi_ctrlr_cfg(dev, seq, sample_freq_hz, adc_digi_pattern_config,
					    &adc_hal_digi_ctrlr_cfg_placeholder, &adc_pattern_len,
					    &unit_attenuation);

	if ((err != 0) || (adc_pattern_len == 0)) {
		return err != 0 ? err : -EINVAL;
	}

	if (options != NULL) {
		number_of_samplings = options->extra_samplings + 1U;
	}

	uint32_t number_of_adc_output_samples;
	uint32_t number_of_adc_dma_samples;
	uint32_t number_of_adc_dma_data_bytes;

	adc_esp32_dma_sample_counts(number_of_samplings, adc_pattern_len,
				    &number_of_adc_output_samples, &number_of_adc_dma_samples,
				    &number_of_adc_dma_data_bytes);

	if (number_of_adc_dma_data_bytes > ADC_DMA_BUFFER_SIZE) {
		LOG_ERR("DMA buffer size insufficient (need %u, have %u)",
			number_of_adc_dma_data_bytes, ADC_DMA_BUFFER_SIZE);
		return -ENOMEM;
	}

	return 0;
}
#endif /* CONFIG_ADC_ESP32_STREAM */

static void adc_esp32_dma_eof_ctx_config(struct adc_esp32_data *data, uint32_t sample_count)
{
	adc_hal_dma_config_t adc_hal_dma_config = {
		.eof_desc_num = 1,
		.eof_step = 1,
		.eof_num = sample_count,
	};

	adc_hal_dma_ctx_config(&data->adc_hal_dma_ctx, &adc_hal_dma_config);
}

static int adc_esp32_digi_start(const struct device *dev,
				adc_hal_digi_ctrlr_cfg_t *adc_hal_digi_ctrlr_cfg,
				uint32_t number_of_adc_dma_samples, uint32_t unit_attenuation,
				uint32_t dma_buffer_size)
{
	const struct adc_esp32_conf *conf = dev->config;
	struct adc_esp32_data *data = dev->data;
#if !defined(CONFIG_SOC_SERIES_ESP32) || SOC_GDMA_SUPPORTED
	int err;
#endif

	sar_periph_ctrl_adc_reset();
	sar_periph_ctrl_adc_continuous_power_acquire();
	adc_lock_acquire(conf->unit);
	data->digi_hw_active = true;

#if SOC_ADC_CALIBRATION_V1_SUPPORTED
	adc_hal_calibration_init(conf->unit);
	adc_set_hw_calibration_code(conf->unit, unit_attenuation);
#endif /* SOC_ADC_CALIBRATION_V1_SUPPORTED */

#if SOC_ADC_ARBITER_SUPPORTED
	if (conf->unit == ADC_UNIT_2) {
		adc_arbiter_t config = ADC_ARBITER_CONFIG_DEFAULT();

		adc_hal_arbiter_config(&config);
	}
#endif /* SOC_ADC_ARBITER_SUPPORTED */

	data->digi_clk_src = adc_hal_digi_ctrlr_cfg->clk_src;

#if !defined(CONFIG_SOC_SERIES_ESP32)
	err = esp_clk_tree_enable_src((soc_module_clk_t)adc_hal_digi_ctrlr_cfg->clk_src, true);
	if (err != ESP_OK) {
		goto digi_start_fail;
	}
#endif /* !CONFIG_SOC_SERIES_ESP32 */

	adc_esp32_dma_eof_ctx_config(data, number_of_adc_dma_samples);
	adc_hal_set_controller(conf->unit, ADC_HAL_CONTINUOUS_READ_MODE);
	adc_hal_digi_init(&data->adc_hal_dma_ctx);
	adc_hal_digi_controller_config(&data->adc_hal_dma_ctx, adc_hal_digi_ctrlr_cfg);
	adc_hal_digi_enable(false);

#if SOC_GDMA_SUPPORTED
	err = adc_esp32_dma_stop(dev);
	if (err != 0) {
		goto digi_start_fail;
	}

	adc_hal_digi_connect(false);
	adc_hal_digi_reset();

	err = adc_esp32_dma_start(dev, data->dma_buffer, dma_buffer_size);
	if (err != 0) {
		goto digi_start_fail;
	}
#else
#if defined(CONFIG_SOC_SERIES_ESP32)
	i2s_dev_t *i2s_dev = I2S_LL_GET_HW(ADC_DMA_I2S_HOST);

	i2s_ll_rx_stop_link(i2s_dev);
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
	spi_dev_t *spi_dev = SPI_LL_GET_HW(ADC_DMA_SPI_HOST);

	spi_ll_disable_intr(spi_dev, ADC_DMA_INTR_MASK);
	spi_ll_clear_intr(spi_dev, ADC_DMA_INTR_MASK);
	spi_dma_ll_rx_stop(spi_dev, 0);
#endif

	adc_hal_digi_connect(false);

#if defined(CONFIG_SOC_SERIES_ESP32S2)
	spi_dma_ll_rx_reset(spi_dev, 0);
#endif

	adc_hal_digi_reset();
	adc_hal_digi_dma_link(&data->adc_hal_dma_ctx, data->dma_buffer);

#if defined(CONFIG_SOC_SERIES_ESP32)
	i2s_ll_clear_intr_status(i2s_dev, ADC_DMA_INTR_MASK);
	i2s_ll_enable_intr(i2s_dev, ADC_DMA_INTR_MASK, true);
	i2s_ll_enable_dma(i2s_dev, true);
	i2s_ll_rx_start_link(i2s_dev, (uint32_t)data->adc_hal_dma_ctx.rx_desc);
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
	spi_ll_clear_intr(spi_dev, ADC_DMA_INTR_MASK);
	spi_ll_enable_intr(spi_dev, ADC_DMA_INTR_MASK);
	spi_dma_ll_rx_start(spi_dev, 0, (lldesc_t *)data->adc_hal_dma_ctx.rx_desc);
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */
#endif /* SOC_GDMA_SUPPORTED */

	adc_hal_digi_connect(true);
	adc_hal_digi_enable(true);

	return 0;

#if !defined(CONFIG_SOC_SERIES_ESP32) || SOC_GDMA_SUPPORTED
digi_start_fail:
#if !defined(CONFIG_SOC_SERIES_ESP32)
	if (data->digi_clk_src != 0U) {
		(void)esp_clk_tree_enable_src((soc_module_clk_t)data->digi_clk_src, false);
		data->digi_clk_src = 0U;
	}
#endif /* !CONFIG_SOC_SERIES_ESP32 */
	if (data->digi_hw_active) {
		data->digi_hw_active = false;
		adc_lock_release(conf->unit);
		sar_periph_ctrl_adc_continuous_power_release();
	}
	return -EIO;
#endif /* !defined(CONFIG_SOC_SERIES_ESP32) || SOC_GDMA_SUPPORTED */
}

static void adc_esp32_digi_stop(const struct device *dev)
{
	const struct adc_esp32_conf *conf = dev->config;
	struct adc_esp32_data *data = dev->data;

	if (!data->digi_hw_active) {
		return;
	}

	data->digi_hw_active = false;

	adc_hal_digi_enable(false);
	adc_hal_digi_connect(false);

#if ADC_LL_WORKAROUND_CLEAR_EOF_COUNTER
	adc_hal_digi_clr_eof();
#endif

#if SOC_GDMA_SUPPORTED
	(void)adc_esp32_dma_stop(dev);
#else
#if defined(CONFIG_SOC_SERIES_ESP32)
	i2s_dev_t *i2s_dev = I2S_LL_GET_HW(ADC_DMA_I2S_HOST);

	i2s_ll_enable_intr(i2s_dev, ADC_DMA_INTR_MASK, false);
	i2s_ll_clear_intr_status(i2s_dev, ADC_DMA_INTR_MASK);
	i2s_ll_rx_stop_link(i2s_dev);
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
	spi_dev_t *spi_dev = SPI_LL_GET_HW(ADC_DMA_SPI_HOST);

	spi_ll_disable_intr(spi_dev, ADC_DMA_INTR_MASK);
	spi_ll_clear_intr(spi_dev, ADC_DMA_INTR_MASK);
	spi_dma_ll_rx_stop(spi_dev, 0);
#endif /* CONFIG_SOC_SERIES_ESP32 */
#endif /* SOC_GDMA_SUPPORTED */

	adc_hal_digi_deinit();
	adc_lock_release(conf->unit);
	sar_periph_ctrl_adc_continuous_power_release();

#if !defined(CONFIG_SOC_SERIES_ESP32)
	if (data->digi_clk_src != 0U) {
		(void)esp_clk_tree_enable_src((soc_module_clk_t)data->digi_clk_src, false);
		data->digi_clk_src = 0U;
	}
#endif /* !CONFIG_SOC_SERIES_ESP32 */
}

#if SOC_GDMA_SUPPORTED
static bool adc_esp32_digi_sample_parse(const adc_digi_output_data_t *digi_data, uint8_t *channel,
					uint16_t *data)
{
	uint8_t ch = digi_data->type2.channel;

#if SOC_ADC_PERIPH_NUM >= 2
	adc_unit_t unit = digi_data->type2.unit ? ADC_UNIT_2 : ADC_UNIT_1;

	if (unit == ADC_UNIT_2) {
		ch -= ADC_LL_UNIT2_CHANNEL_SUBSTRATION;
	}

	if (ch >= SOC_ADC_CHANNEL_NUM(unit)) {
		return false;
	}
#else
	if (ch >= SOC_ADC_CHANNEL_NUM(ADC_UNIT_1)) {
		return false;
	}
#endif /* SOC_ADC_PERIPH_NUM >= 2 */

	*channel = ch;
	*data = (uint16_t)digi_data->type2.data;

	return true;
}

static const adc_digi_output_data_t *adc_esp32_gdma_dma_sample(const void *dma_buffer, uint32_t idx)
{
	return (const adc_digi_output_data_t *)((const uint8_t *)dma_buffer +
						(idx * SOC_ADC_DIGI_DATA_BYTES_PER_CONV));
}
#endif /* SOC_GDMA_SUPPORTED */

#if !SOC_GDMA_SUPPORTED
static void adc_esp32_fill_seq_buffer_type1(uint16_t *sample, const void *dma_buffer,
					    uint32_t dma_sample_count, unsigned int repeats,
					    uint32_t pattern_len,
					    const adc_digi_pattern_config_t *pattern)
{
	uint32_t raw_samples =
		adc_esp32_dma_data_bytes(dma_sample_count) / SOC_ADC_DIGI_RESULT_BYTES;
	uint8_t round_counts[SOC_ADC_PATT_LEN_MAX] = {0};

	for (uint32_t i = 0U; i < raw_samples; i++) {
		const adc_digi_output_data_t *digi =
			(const adc_digi_output_data_t *)((const uint8_t *)dma_buffer +
							 (i * SOC_ADC_DIGI_RESULT_BYTES));
		uint8_t ch = digi->type1.channel;

		if (ch >= SOC_ADC_CHANNEL_NUM(ADC_UNIT_1)) {
			continue;
		}

		for (uint32_t p = 0U; p < pattern_len; p++) {
			if (pattern[p].channel != ch) {
				continue;
			}

			unsigned int round = round_counts[p];

			if (round < repeats) {
				round_counts[p]++;
				sample[(round * pattern_len) + p] = (uint16_t)digi->type1.data;
			}
			break;
		}
	}
}
#endif /* !SOC_GDMA_SUPPORTED */

static void adc_esp32_fill_seq_buffer(const void *seq_buffer, const void *dma_buffer,
				      uint32_t dma_sample_count, uint32_t output_sample_count,
				      const adc_digi_pattern_config_t *pattern,
				      uint32_t pattern_len)
{
	uint16_t *sample = (uint16_t *)seq_buffer;

	if ((seq_buffer == NULL) || (dma_buffer == NULL) || (dma_sample_count == 0U) ||
	    (output_sample_count == 0U) || (pattern == NULL) || (pattern_len == 0U) ||
	    ((output_sample_count % pattern_len) != 0U)) {
		return;
	}

	unsigned int repeats = output_sample_count / pattern_len;

	memset(sample, 0, output_sample_count * sizeof(uint16_t));

#if SOC_GDMA_SUPPORTED
	uint32_t words_per_round = dma_sample_count / repeats;

	for (unsigned int r = 0U; r < repeats; r++) {
		uint32_t out_base = r * pattern_len;
		uint32_t round_lo = r * words_per_round;
		uint32_t round_hi = round_lo + words_per_round;

		for (uint32_t p = 0U; p < pattern_len; p++) {
			uint8_t want_ch = pattern[p].channel;

			sample[out_base + p] = 0U;
			for (uint32_t k = round_hi; k-- > round_lo;) {
				uint8_t ch;
				uint16_t val;
				const adc_digi_output_data_t *digi =
					adc_esp32_gdma_dma_sample(dma_buffer, k);

				if (!adc_esp32_digi_sample_parse(digi, &ch, &val) ||
				    (ch != want_ch)) {
					continue;
				}

				sample[out_base + p] = val;
				break;
			}
		}
	}
#else
	adc_esp32_fill_seq_buffer_type1(sample, dma_buffer, dma_sample_count, repeats, pattern_len,
					pattern);
#endif /* SOC_GDMA_SUPPORTED */
}

#if !SOC_GDMA_SUPPORTED
static int adc_esp32_dma_intr_set(struct adc_esp32_data *data, bool enable)
{
	return enable ? esp_intr_enable(data->irq_handle) : esp_intr_disable(data->irq_handle);
}
#endif /* !SOC_GDMA_SUPPORTED */

static int adc_esp32_wait_for_dma_conv_done(const struct device *dev)
{
	struct adc_esp32_data *data = dev->data;
	int err = 0;

	err = k_sem_take(&data->dma_conv_wait_lock, K_MSEC(ADC_DMA_MAX_CONV_DONE_TIME));
	if (err) {
		LOG_ERR("Error taking dma_conv_wait_lock (%d)", err);
	}

	return err;
}

static int adc_esp32_dma_prep_read(const struct device *dev, const struct adc_sequence *seq,
				   adc_hal_digi_ctrlr_cfg_t *adc_hal_digi_ctrlr_cfg,
				   adc_digi_pattern_config_t *adc_digi_pattern_config,
				   uint32_t *adc_pattern_len, uint32_t *unit_attenuation,
				   uint32_t *number_of_adc_output_samples,
				   uint32_t *number_of_adc_dma_samples,
				   uint32_t *number_of_adc_dma_data_bytes)
{
	uint32_t sample_freq_hz;
	uint32_t number_of_samplings = 1;
	const struct adc_sequence_options *options = seq->options;
	int err;

	err = adc_esp32_dma_sample_freq_hz(options, &sample_freq_hz);
	if (err != 0) {
		return err;
	}

	err = adc_esp32_fill_digi_ctrlr_cfg(dev, seq, sample_freq_hz, adc_digi_pattern_config,
					    adc_hal_digi_ctrlr_cfg, adc_pattern_len,
					    unit_attenuation);
	if (err != 0) {
		return err;
	}
	if (*adc_pattern_len == 0) {
		return -EINVAL;
	}

	if (options != NULL) {
		number_of_samplings = options->extra_samplings + 1;
	}

	adc_esp32_dma_sample_counts(number_of_samplings, *adc_pattern_len,
				    number_of_adc_output_samples, number_of_adc_dma_samples,
				    number_of_adc_dma_data_bytes);

	if (seq->buffer_size < (*number_of_adc_output_samples * sizeof(uint16_t))) {
		LOG_ERR("buffer size is not enough to store all samples!");
		return -EINVAL;
	}

	if (*number_of_adc_dma_data_bytes > ADC_DMA_BUFFER_SIZE) {
		LOG_ERR("DMA buffer size insufficient (need %u, have %u)",
			*number_of_adc_dma_data_bytes, ADC_DMA_BUFFER_SIZE);
		return -ENOMEM;
	}

	return 0;
}

int adc_esp32_dma_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct adc_esp32_data *data = dev->data;
	int err = 0;
	uint32_t adc_pattern_len, unit_attenuation;
	uint32_t number_of_adc_output_samples;
	uint32_t number_of_adc_dma_samples;
	uint32_t number_of_adc_dma_data_bytes;
	adc_hal_digi_ctrlr_cfg_t adc_hal_digi_ctrlr_cfg;
	adc_digi_pattern_config_t adc_digi_pattern_config[SOC_ADC_MAX_CHANNEL_NUM];

	err = adc_esp32_dma_prep_read(dev, seq, &adc_hal_digi_ctrlr_cfg, adc_digi_pattern_config,
				      &adc_pattern_len, &unit_attenuation,
				      &number_of_adc_output_samples, &number_of_adc_dma_samples,
				      &number_of_adc_dma_data_bytes);
	if (err != 0) {
		return err;
	}

	err = adc_esp32_digi_start(dev, &adc_hal_digi_ctrlr_cfg, number_of_adc_dma_samples,
				   unit_attenuation, number_of_adc_dma_data_bytes);
	if (err != 0) {
		return err;
	}

#if !SOC_GDMA_SUPPORTED
	err = adc_esp32_dma_intr_set(data, true);
	if (err != 0) {
		adc_esp32_digi_stop(dev);
		return err;
	}
#endif /* !SOC_GDMA_SUPPORTED */

	err = adc_esp32_wait_for_dma_conv_done(dev);
	if (err != 0) {
		adc_esp32_digi_stop(dev);

#if !SOC_GDMA_SUPPORTED
		(void)adc_esp32_dma_intr_set(data, false);
#endif /* !SOC_GDMA_SUPPORTED */

		return err;
	}

	adc_esp32_digi_stop(dev);

#if !SOC_GDMA_SUPPORTED
	err = adc_esp32_dma_intr_set(data, false);
	if (err != 0) {
		return err;
	}
#endif /* !SOC_GDMA_SUPPORTED */

	sys_cache_data_flush_and_invd_range(data->dma_buffer, number_of_adc_dma_data_bytes);
	adc_esp32_fill_seq_buffer(seq->buffer, data->dma_buffer, number_of_adc_dma_samples,
				  number_of_adc_output_samples, adc_digi_pattern_config,
				  adc_pattern_len);
#if defined(CONFIG_ADC_ESP32_DMA_DECODE_VOLTAGE_CALIBRATED)
	adc_esp32_dma_calibrate_samples(data, seq->buffer, adc_digi_pattern_config, adc_pattern_len,
					number_of_adc_output_samples / adc_pattern_len,
					seq->resolution);
#endif /* CONFIG_ADC_ESP32_DMA_DECODE_VOLTAGE_CALIBRATED */

	return 0;
}

#if defined(CONFIG_ADC_ESP32_STREAM)

/*
 * Pause the ADC between stream frames: same teardown as oneshot DMA reads so the
 * next adc_esp32_digi_start() can re-acquire SAR power and locks cleanly.
 */
static int adc_esp32_stream_hw_stop_and_pack(struct adc_esp32_data *data, const struct device *dev)
{
	uint32_t dma_bytes = adc_esp32_dma_data_bytes(data->stream_num_adc_dma_samples);

	adc_esp32_digi_stop(dev);

	sys_cache_data_flush_and_invd_range(data->dma_buffer, dma_bytes);
	adc_esp32_fill_seq_buffer(data->stream_seq_snap.buffer, data->dma_buffer,
				  data->stream_num_adc_dma_samples,
				  data->stream_num_adc_output_samples, data->stream_pattern,
				  data->stream_digi_cfg.adc_pattern_len);
#if defined(CONFIG_ADC_ESP32_DMA_DECODE_VOLTAGE_CALIBRATED)
	adc_esp32_stream_calibrate_frame(data);
#endif /* CONFIG_ADC_ESP32_DMA_DECODE_VOLTAGE_CALIBRATED */
	return 0;
}

static void adc_esp32_stream_rtio_work_fn(struct k_work *work)
{
	struct adc_esp32_data *data = CONTAINER_OF(work, struct adc_esp32_data, stream_rtio_work);
	struct rtio_iodev_sqe *sqe = data->stream_rtio_sqe;
	int err = data->stream_rtio_err;

	data->stream_rtio_sqe = NULL;

	if (sqe == NULL) {
		return;
	}

	if (err != 0) {
		rtio_iodev_sqe_err(sqe, err);
		return;
	}

	/*
	 * Deliver the CQE before allocating the next mempool buffer. The default
	 * multishot path in rtio_iodev_sqe_ok() resubmits first and can exhaust
	 * the pool while the application still holds prior buffers.
	 */
	rtio_cqe_submit(sqe->r, 0, sqe->sqe.userdata, rtio_cqe_compute_flags(sqe));

	if (FIELD_GET(RTIO_SQE_MEMPOOL_BUFFER, sqe->sqe.flags) == 1 && sqe->sqe.op == RTIO_OP_RX) {
		sqe->sqe.rx.buf = NULL;
		sqe->sqe.rx.buf_len = 0;
	}

	data->stream_resubmit_sqe = sqe;
	(void)k_work_cancel_delayable(&data->stream_resubmit_dwork);
	(void)k_work_schedule_for_queue(&adc_esp32_stream_rtio_wq, &data->stream_resubmit_dwork,
					K_NO_WAIT);
}

static void adc_esp32_stream_resubmit_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct adc_esp32_data *data =
		CONTAINER_OF(dwork, struct adc_esp32_data, stream_resubmit_dwork);
	const struct device *dev = data->dev;
	struct rtio_iodev_sqe *sqe = data->stream_resubmit_sqe;
	int err;

	if (sqe == NULL) {
		return;
	}

	err = adc_esp32_stream_arm(dev, sqe);
	if (err == -ENOMEM) {
		(void)k_work_schedule_for_queue(&adc_esp32_stream_rtio_wq,
						&data->stream_resubmit_dwork,
						K_MSEC(ADC_ESP32_STREAM_RESUBMIT_RETRY_MS));
		return;
	}

	data->stream_resubmit_sqe = NULL;

	if (err < 0) {
		(void)k_work_cancel_delayable(&data->stream_resubmit_dwork);
		LOG_ERR("stream resubmit failed (%d)", err);
		rtio_iodev_sqe_err(sqe, err);
	}
}

static void adc_esp32_stream_dma_done_work_fn(struct k_work *work)
{
	struct adc_esp32_data *data = CONTAINER_OF(work, struct adc_esp32_data, stream_done_work);
	const struct device *dev = data->dev;
	struct rtio_iodev_sqe *sqe = data->stream_iodev_sqe;
	int err;

	err = adc_esp32_stream_hw_stop_and_pack(data, dev);
	data->dma_notify_via_work = false;
	data->stream_iodev_sqe = NULL;

	if (sqe == NULL) {
		return;
	}

	if (err != 0) {
		rtio_iodev_sqe_err(sqe, err);
		return;
	}

	data->stream_rtio_sqe = sqe;
	data->stream_rtio_err = 0;
	(void)k_work_submit_to_queue(&adc_esp32_stream_rtio_wq, &data->stream_rtio_work);
}

static void adc_esp32_stream_start_work_fn(struct k_work *work)
{
	struct adc_esp32_data *data = CONTAINER_OF(work, struct adc_esp32_data, stream_start_work);
	const struct device *dev = data->dev;
	struct rtio_iodev_sqe *sqe = data->stream_iodev_sqe;
	int err;

	err = adc_esp32_dma_stream_start(dev);
	if (err != 0) {
		data->dma_notify_via_work = false;
		data->stream_iodev_sqe = NULL;
		adc_context_release(&data->ctx, err);
		LOG_ERR("stream hw start failed (%d)", err);
		if (sqe != NULL) {
			rtio_iodev_sqe_err(sqe, err);
		}
		return;
	}

	adc_context_release(&data->ctx, 0);
}

int adc_esp32_dma_stream_queue_start(const struct device *dev)
{
	struct adc_esp32_data *data = dev->data;

	adc_esp32_stream_rtio_wq_init();
	return k_work_submit_to_queue(&adc_esp32_stream_rtio_wq, &data->stream_start_work);
}

int adc_esp32_dma_stream_start(const struct device *dev)
{
	struct adc_esp32_data *data = dev->data;
	const struct adc_sequence *seq = &data->stream_seq_snap;
	int err;
	uint32_t adc_pattern_len, unit_attenuation;
	uint32_t number_of_adc_output_samples;
	uint32_t number_of_adc_dma_samples;
	uint32_t number_of_adc_dma_data_bytes;

	err = adc_esp32_dma_prep_read(dev, seq, &data->stream_digi_cfg, data->stream_pattern,
				      &adc_pattern_len, &unit_attenuation,
				      &number_of_adc_output_samples, &number_of_adc_dma_samples,
				      &number_of_adc_dma_data_bytes);
	if (err != 0) {
		return err;
	}

	data->stream_num_adc_output_samples = number_of_adc_output_samples;
	data->stream_num_adc_dma_samples = number_of_adc_dma_samples;

	(void)k_sem_reset(&data->dma_conv_wait_lock);
	sys_cache_data_flush_and_invd_range(data->dma_buffer, number_of_adc_dma_data_bytes);

	err = adc_esp32_digi_start(dev, &data->stream_digi_cfg, number_of_adc_dma_samples,
				   unit_attenuation, number_of_adc_dma_data_bytes);
	if (err != 0) {
		LOG_ERR("digi_start failed (%d)", err);
		return err;
	}

#if !SOC_GDMA_SUPPORTED
	err = adc_esp32_dma_intr_set(data, true);
	if (err != 0) {
		adc_esp32_digi_stop(dev);
		return err;
	}
#endif /* !SOC_GDMA_SUPPORTED */

	data->dma_notify_via_work = true;

	return 0;
}

#endif /* CONFIG_ADC_ESP32_STREAM */

int adc_esp32_dma_execute_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct adc_esp32_data *data = dev->data;
	uint32_t chans = seq->channels;

	while (chans != 0U) {
		uint8_t ch = find_lsb_set(chans) - 1;

		data->resolution[ch] = seq->resolution;
		chans &= ~BIT(ch);
	}

	return adc_esp32_dma_read(dev, seq);
}

#if defined(CONFIG_ADC_ASYNC)

static void adc_esp32_dma_async_work_fn(struct k_work *work)
{
	struct adc_esp32_data *data = CONTAINER_OF(work, struct adc_esp32_data, dma_async_work);
	const struct device *dev = data->dev;
	int err;

	err = adc_esp32_dma_execute_read(dev, &data->ctx.sequence);
	adc_context_complete(&data->ctx, err);
}

int adc_esp32_dma_async_submit(const struct device *dev)
{
	struct adc_esp32_data *data = dev->data;
	int ret = k_work_submit(&data->dma_async_work);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

#endif /* CONFIG_ADC_ASYNC */

int adc_esp32_dma_finish_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct adc_esp32_data *data = dev->data;
	int err;

	data->ctx.status = 0;
	err = adc_esp32_dma_execute_read(dev, seq);

	adc_context_complete(&data->ctx, err);

	return adc_context_wait_for_completion(&data->ctx);
}

int adc_esp32_dma_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
#if SOC_ADC_PERIPH_NUM >= 2
	__maybe_unused const struct adc_esp32_conf *conf = dev->config;

	if (!SOC_ADC_DIG_SUPPORTED_UNIT(conf->unit)) {
		LOG_ERR("ADC2 dma mode is no longer supported, please use ADC1!");
		return -EINVAL;
	}
#else
	ARG_UNUSED(cfg);
#endif /* SOC_ADC_PERIPH_NUM >= 2 */

	return 0;
}

int adc_esp32_dma_init(const struct device *dev)
{
	struct adc_esp32_data *data = (struct adc_esp32_data *)dev->data;

	if (k_sem_init(&data->dma_conv_wait_lock, 0, 1)) {
		LOG_ERR("dma_conv_wait_lock initialization failed!");
		return -EINVAL;
	}

	data->adc_hal_dma_ctx.rx_desc = data->dma_rx_desc;

	LOG_DBG("rx_desc = 0x%08X (%u entries)", (unsigned int)data->adc_hal_dma_ctx.rx_desc,
		ADC_ESP32_DMA_RX_DESC_COUNT);
	LOG_DBG("data->dma_buffer = 0x%08X (%u bytes)", (unsigned int)data->dma_buffer,
		ADC_DMA_BUFFER_SIZE);

#ifdef CONFIG_SOC_SERIES_ESP32
	periph_module_enable(PERIPH_I2S0_MODULE);

	i2s_dev_t *i2s_dev = I2S_LL_GET_HW(ADC_DMA_I2S_HOST);

	i2s_ll_enable_core_clock(i2s_dev, true);

	int err = esp_intr_alloc(I2S0_INTR_SOURCE, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_INTRDISABLED,
				 adc_esp32_dma_intr_handler, (void *)dev, &(data->irq_handle));
	if (err != 0) {
		LOG_ERR("Could not allocate interrupt (err %d)", err);
		return err;
	}
#endif /* CONFIG_SOC_SERIES_ESP32 */

#ifdef CONFIG_SOC_SERIES_ESP32S2
	spi_ll_enable_bus_clock(ADC_DMA_SPI_HOST, true);
	spi_ll_reset_register(ADC_DMA_SPI_HOST);

	periph_module_enable(PERIPH_SPI2_DMA_MODULE);
	periph_module_enable(PERIPH_SPI3_DMA_MODULE);

	spi_dev_t *spi_dev = SPI_LL_GET_HW(ADC_DMA_SPI_HOST);

	spi_dma_ll_rx_enable_burst_desc(spi_dev, 0, true);

	int err = esp_intr_alloc(SPI3_DMA_INTR_SOURCE,
				 ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_INTRDISABLED,
				 adc_esp32_dma_intr_handler, (void *)dev, &(data->irq_handle));
	if (err != 0) {
		LOG_ERR("Could not allocate interrupt (err %d)", err);
		return err;
	}
#endif /* CONFIG_SOC_SERIES_ESP32S2 */

#if SOC_GDMA_SUPPORTED
	adc_apb_periph_claim();
#endif /* SOC_GDMA_SUPPORTED */

	data->digi_clk_src = 0U;
	data->digi_hw_active = false;

#if defined(CONFIG_ADC_ASYNC)
	k_work_init(&data->dma_async_work, adc_esp32_dma_async_work_fn);
#endif /* CONFIG_ADC_ASYNC */

#if defined(CONFIG_ADC_ESP32_STREAM)
	adc_esp32_stream_rtio_wq_init();
	k_work_init(&data->stream_done_work, adc_esp32_stream_dma_done_work_fn);
	k_work_init(&data->stream_start_work, adc_esp32_stream_start_work_fn);
	k_work_init(&data->stream_rtio_work, adc_esp32_stream_rtio_work_fn);
	k_work_init_delayable(&data->stream_resubmit_dwork, adc_esp32_stream_resubmit_work_fn);
	data->dma_notify_via_work = false;
	data->stream_iodev_sqe = NULL;
	data->stream_rtio_sqe = NULL;
	data->stream_resubmit_sqe = NULL;
	data->stream_num_adc_output_samples = 0U;
	data->stream_num_adc_dma_samples = 0U;
#endif /* CONFIG_ADC_ESP32_STREAM */

	return 0;
}
