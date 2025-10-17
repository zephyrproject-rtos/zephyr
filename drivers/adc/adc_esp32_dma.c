/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_adc

#include <esp_adc/adc_cali.h>
#include <esp_clk_tree.h>
#include <esp_private/periph_ctrl.h>
#include <esp_private/sar_periph_ctrl.h>
#include <esp_private/adc_share_hw_ctrl.h>

#include "adc_esp32.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_esp32_dma, CONFIG_ADC_LOG_LEVEL);

#if SOC_GDMA_SUPPORTED
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#elif defined(CONFIG_SOC_SERIES_ESP32)
#include <zephyr/dt-bindings/clock/esp32_clock.h>
#include <zephyr/dt-bindings/interrupt-controller/esp-xtensa-intmux.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#define ADC_DMA_I2S_HOST 0
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#include <zephyr/dt-bindings/clock/esp32s2_clock.h>
#include <zephyr/dt-bindings/interrupt-controller/esp32s2-xtensa-intmux.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif /* SOC_GDMA_SUPPORTED */

#define ADC_DMA_BUFFER_SIZE        DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED
#define ADC_DMA_MAX_CONV_DONE_TIME 1000

#define UNIT_ATTEN_UNINIT UINT32_MAX

#if SOC_GDMA_SUPPORTED

static void IRAM_ATTR adc_esp32_dma_conv_done(const struct device *dma_dev, void *user_data,
					      uint32_t channel, int status)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(status);

	const struct device *dev = user_data;
	struct adc_esp32_data *data = dev->data;

	k_sem_give(&data->dma_conv_wait_lock);
}

static int adc_esp32_dma_start(const struct device *dev, uint8_t *buf, size_t len)
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
		LOG_ERR("dma channel[%u] is busy!", (unsigned int)conf->dma_channel);
		return -EBUSY;
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
	if (i2s_ll_get_intr_status(I2S_LL_GET_HW(ADC_DMA_I2S_HOST)) & ADC_HAL_DMA_INTR_MASK) {
		i2s_ll_clear_intr_status(I2S_LL_GET_HW(ADC_DMA_I2S_HOST), ADC_HAL_DMA_INTR_MASK);
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
	if (spi_ll_get_intr(SPI_LL_GET_HW(SPI3_HOST), ADC_HAL_DMA_INTR_MASK)) {
		spi_ll_clear_intr(SPI_LL_GET_HW(SPI3_HOST), ADC_HAL_DMA_INTR_MASK);
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */
		k_sem_give(&data->dma_conv_wait_lock);
	}
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

	adc_digi_pattern_config_t *adc_digi_pattern_config = pattern_config;
	uint32_t channel_mask = 1, channels_copy = seq->channels;

	*pattern_len = 0;
	*unit_attenuation = UNIT_ATTEN_UNINIT;
	for (uint8_t channel_id = 0; channel_id < conf->channel_count; channel_id++) {
		if (channels_copy & channel_mask) {
			if (*unit_attenuation == UNIT_ATTEN_UNINIT) {
				*unit_attenuation = data->attenuation[channel_id];
			}

			if (*unit_attenuation != data->attenuation[channel_id]) {
				LOG_ERR("Channel[%u] attenuation different of unit[%u] attenuation",
					(unsigned int)channel_id, (unsigned int)conf->unit);
				return -EINVAL;
			}

			adc_digi_pattern_config->atten = data->attenuation[channel_id];
			adc_digi_pattern_config->channel = channel_id;
			adc_digi_pattern_config->unit = conf->unit;
			adc_digi_pattern_config->bit_width = seq->resolution;
			adc_digi_pattern_config++;

			*pattern_len += 1;
			if (*pattern_len > SOC_ADC_PATT_LEN_MAX) {
				LOG_ERR("Max pattern len is %d", SOC_ADC_PATT_LEN_MAX);
				return -EINVAL;
			}

			channels_copy &= ~channel_mask;
			if (!channels_copy) {
				break;
			}
		}
		channel_mask <<= 1;
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

static void adc_esp32_digi_start(const struct device *dev,
				 adc_hal_digi_ctrlr_cfg_t *adc_hal_digi_ctrlr_cfg,
				 uint32_t number_of_adc_samples, uint32_t unit_attenuation)
{
	const struct adc_esp32_conf *conf = dev->config;
	struct adc_esp32_data *data = dev->data;

	periph_module_reset(PERIPH_SARADC_MODULE);
	sar_periph_ctrl_adc_continuous_power_acquire();
	adc_lock_acquire(conf->unit);

#if SOC_ADC_CALIBRATION_V1_SUPPORTED
	adc_set_hw_calibration_code(conf->unit, unit_attenuation);
#endif /* SOC_ADC_CALIBRATION_V1_SUPPORTED */

#if SOC_ADC_ARBITER_SUPPORTED
	if (conf->unit == ADC_UNIT_2) {
		adc_arbiter_t config = ADC_ARBITER_CONFIG_DEFAULT();

		adc_hal_arbiter_config(&config);
	}
#endif /* SOC_ADC_ARBITER_SUPPORTED */

	adc_hal_dma_config_t adc_hal_dma_config = {
#if defined(CONFIG_SOC_SERIES_ESP32)
		.dev = (void *)I2S_LL_GET_HW(ADC_DMA_I2S_HOST),
		.eof_desc_num = 1,
		.eof_step = 1,
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
		.dev = (void *)SPI_LL_GET_HW(SPI3_HOST),
		.eof_desc_num = 1,
		.eof_step = 1,
#endif /* defined(CONFIG_SOC_SERIES_ESP32) */
		.eof_num = number_of_adc_samples,
	};

	adc_hal_dma_ctx_config(&data->adc_hal_dma_ctx, &adc_hal_dma_config);
	adc_hal_set_controller(conf->unit, ADC_HAL_CONTINUOUS_READ_MODE);
	adc_hal_digi_init(&data->adc_hal_dma_ctx);
	adc_hal_digi_controller_config(&data->adc_hal_dma_ctx, adc_hal_digi_ctrlr_cfg);
	adc_hal_digi_start(&data->adc_hal_dma_ctx, data->dma_buffer);
}

static void adc_esp32_digi_stop(const struct device *dev)
{
	const struct adc_esp32_conf *conf = dev->config;
	struct adc_esp32_data *data = dev->data;

	adc_hal_digi_dis_intr(&data->adc_hal_dma_ctx, ADC_HAL_DMA_INTR_MASK);
	adc_hal_digi_clr_intr(&data->adc_hal_dma_ctx, ADC_HAL_DMA_INTR_MASK);
	adc_hal_digi_stop(&data->adc_hal_dma_ctx);

#if ADC_LL_WORKAROUND_CLEAR_EOF_COUNTER
	periph_module_reset(PERIPH_SARADC_MODULE);
	adc_ll_digi_dma_clr_eof();
#endif

	adc_hal_digi_deinit(&data->adc_hal_dma_ctx);
	adc_lock_release(conf->unit);
	sar_periph_ctrl_adc_continuous_power_release();
}

static void adc_esp32_fill_seq_buffer(const void *seq_buffer, const void *dma_buffer,
				      uint32_t number_of_samples)
{
	uint16_t *sample = (uint16_t *)seq_buffer;
	adc_digi_output_data_t *digi_data = (adc_digi_output_data_t *)dma_buffer;

	for (uint32_t k = 0; k < number_of_samples; k++) {
#if SOC_GDMA_SUPPORTED
		*sample++ = (uint16_t)(digi_data++)->type2.data;
#else
		*sample++ = (uint16_t)(digi_data++)->type1.data;
#endif /* SOC_GDMA_SUPPORTED */
	}
}

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

int adc_esp32_dma_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct adc_esp32_data *data = dev->data;
	int err = 0;
	uint32_t adc_pattern_len, unit_attenuation;
	adc_hal_digi_ctrlr_cfg_t adc_hal_digi_ctrlr_cfg;
	adc_digi_pattern_config_t adc_digi_pattern_config[SOC_ADC_MAX_CHANNEL_NUM];

	const struct adc_sequence_options *options = seq->options;
	uint32_t sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_HIGH, number_of_samplings = 1;

	if (options && options->callback) {
		return -ENOTSUP;
	}

	if (options && options->interval_us) {
		sample_freq_hz = MHZ(1) / options->interval_us;
		if (sample_freq_hz < SOC_ADC_SAMPLE_FREQ_THRES_LOW ||
		    sample_freq_hz > SOC_ADC_SAMPLE_FREQ_THRES_HIGH) {
			LOG_ERR("ADC sampling frequency out of range: %uHz", sample_freq_hz);
			return -EINVAL;
		}
	}

	err = adc_esp32_fill_digi_ctrlr_cfg(dev, seq, sample_freq_hz, adc_digi_pattern_config,
					    &adc_hal_digi_ctrlr_cfg, &adc_pattern_len,
					    &unit_attenuation);
	if (err || adc_pattern_len == 0) {
		return -EINVAL;
	}

	if (options != NULL) {
		number_of_samplings = options->extra_samplings + 1;
	}

	if (seq->buffer_size < (number_of_samplings * sizeof(uint16_t))) {
		LOG_ERR("buffer size is not enough to store all samples!");
		return -EINVAL;
	}

	uint32_t number_of_adc_samples = number_of_samplings * adc_pattern_len;
	uint32_t number_of_adc_dma_data_bytes =
		number_of_adc_samples * SOC_ADC_DIGI_DATA_BYTES_PER_CONV;

	if (number_of_adc_dma_data_bytes > ADC_DMA_BUFFER_SIZE) {
		LOG_ERR("dma buffer size insufficient to store a complete sequence!");
		return -EINVAL;
	}

#if SOC_GDMA_SUPPORTED
	err = adc_esp32_dma_start(dev, data->dma_buffer, number_of_adc_dma_data_bytes);
#else
	err = esp_intr_enable(data->irq_handle);
#endif /* SOC_GDMA_SUPPORTED */
	if (err) {
		return err;
	}

	adc_esp32_digi_start(dev, &adc_hal_digi_ctrlr_cfg, number_of_adc_samples, unit_attenuation);

	err = adc_esp32_wait_for_dma_conv_done(dev);
	if (err) {
		return err;
	}

	adc_esp32_digi_stop(dev);

#if SOC_GDMA_SUPPORTED
	err = adc_esp32_dma_stop(dev);
#else
	err = esp_intr_disable(data->irq_handle);
#endif /* SOC_GDMA_SUPPORTED */
	if (err) {
		return err;
	}

	adc_esp32_fill_seq_buffer(seq->buffer, data->dma_buffer, number_of_adc_samples);

	return 0;
}

int adc_esp32_dma_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	__maybe_unused const struct adc_esp32_conf *conf =
		(const struct adc_esp32_conf *)dev->config;

	if (!SOC_ADC_DIG_SUPPORTED_UNIT(conf->unit)) {
		LOG_ERR("ADC2 dma mode is no longer supported, please use ADC1!");
		return -EINVAL;
	}

	return 0;
}

int adc_esp32_dma_init(const struct device *dev)
{
	struct adc_esp32_data *data = (struct adc_esp32_data *)dev->data;

	if (k_sem_init(&data->dma_conv_wait_lock, 0, 1)) {
		LOG_ERR("dma_conv_wait_lock initialization failed!");
		return -EINVAL;
	}

	data->adc_hal_dma_ctx.rx_desc = k_aligned_alloc(sizeof(uint32_t), sizeof(dma_descriptor_t));
	if (!data->adc_hal_dma_ctx.rx_desc) {
		LOG_ERR("rx_desc allocation failed!");
		return -ENOMEM;
	}
	LOG_DBG("rx_desc = 0x%08X", (unsigned int)data->adc_hal_dma_ctx.rx_desc);

	data->dma_buffer = k_aligned_alloc(sizeof(uint32_t), ADC_DMA_BUFFER_SIZE);
	if (!data->dma_buffer) {
		LOG_ERR("dma buffer allocation failed!");
		k_free(data->adc_hal_dma_ctx.rx_desc);
		return -ENOMEM;
	}
	LOG_DBG("data->dma_buffer = 0x%08X", (unsigned int)data->dma_buffer);

#ifdef CONFIG_SOC_SERIES_ESP32
	periph_module_enable(PERIPH_I2S0_MODULE);
	i2s_ll_enable_clock(I2S_LL_GET_HW(ADC_DMA_I2S_HOST));

	int err = esp_intr_alloc(I2S0_INTR_SOURCE, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_INTRDISABLED,
				 adc_esp32_dma_intr_handler, (void *)dev, &(data->irq_handle));
	if (err != 0) {
		LOG_ERR("Could not allocate interrupt (err %d)", err);
		k_free(data->adc_hal_dma_ctx.rx_desc);
		k_free(data->dma_buffer);
		return err;
	}
#endif /* CONFIG_SOC_SERIES_ESP32 */

#ifdef CONFIG_SOC_SERIES_ESP32S2
	periph_module_enable(PERIPH_HSPI_MODULE);
	periph_module_enable(PERIPH_SPI3_DMA_MODULE);

	int err = esp_intr_alloc(SPI3_DMA_INTR_SOURCE,
				 ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_INTRDISABLED,
				 adc_esp32_dma_intr_handler, (void *)dev, &(data->irq_handle));
	if (err != 0) {
		LOG_ERR("Could not allocate interrupt (err %d)", err);
		k_free(data->adc_hal_dma_ctx.rx_desc);
		k_free(data->dma_buffer);
		return err;
	}
#endif /* CONFIG_SOC_SERIES_ESP32S2 */

	return 0;
}
