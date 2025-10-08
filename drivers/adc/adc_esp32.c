/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_adc

#include <esp_clk_tree.h>
#include <esp_private/sar_periph_ctrl.h>
#include <esp_private/adc_share_hw_ctrl.h>

#include "adc_esp32.h"

#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_esp32, CONFIG_ADC_LOG_LEVEL);

#define ADC_RESOLUTION_MIN SOC_ADC_DIGI_MIN_BITWIDTH
#define ADC_RESOLUTION_MAX SOC_ADC_DIGI_MAX_BITWIDTH

#define VALID_RESOLUTION(r) ((r) >= ADC_RESOLUTION_MIN && (r) <= ADC_RESOLUTION_MAX)

/* Default internal reference voltage */
#define ADC_ESP32_DEFAULT_VREF_INTERNAL (1100)

#define ADC_CLIP_MVOLT_12DB	2550

static void adc_hw_calibration(adc_unit_t unit)
{
#if SOC_ADC_CALIBRATION_V1_SUPPORTED
	adc_hal_calibration_init(unit);
	for (int j = 0; j < SOC_ADC_ATTEN_NUM; j++) {
		adc_calc_hw_calibration_code(unit, j);
#if SOC_ADC_CALIB_CHAN_COMPENS_SUPPORTED
		/* Load the channel compensation from efuse */
		for (int k = 0; k < SOC_ADC_CHANNEL_NUM(unit); k++) {
			adc_load_hw_calibration_chan_compens(unit, k, j);
		}
#endif /* SOC_ADC_CALIB_CHAN_COMPENS_SUPPORTED */
	}
#endif /* SOC_ADC_CALIBRATION_V1_SUPPORTED */
}

/* Convert zephyr,gain property to the ESP32 attenuation */
static inline int gain_to_atten(enum adc_gain gain, adc_atten_t *atten)
{
	switch (gain) {
	case ADC_GAIN_1:
		*atten = ADC_ATTEN_DB_0;
		break;
	case ADC_GAIN_4_5:
		*atten = ADC_ATTEN_DB_2_5;
		break;
	case ADC_GAIN_1_2:
		*atten = ADC_ATTEN_DB_6;
		break;
	case ADC_GAIN_1_4:
		*atten = ADC_ATTEN_DB_12;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

#ifndef CONFIG_ADC_ESP32_DMA

/* Convert voltage by inverted attenuation to support zephyr gain values */
static void atten_to_gain(adc_atten_t atten, uint32_t *val_mv)
{
	uint32_t num, den;

	if (!val_mv) {
		return;
	}

	switch (atten) {
	case ADC_ATTEN_DB_2_5: /* 1/ADC_GAIN_4_5 */
		num = 4;
		den = 5;
		break;
	case ADC_ATTEN_DB_6: /* 1/ADC_GAIN_1_2 */
		num = 1;
		den = 2;
		break;
	case ADC_ATTEN_DB_12: /* 1/ADC_GAIN_1_4 */
		num = 1;
		den = 4;
		break;
	case ADC_ATTEN_DB_0: /* 1/ADC_GAIN_1 */
	default:
		num = 1;
		den = 1;
		break;
	}

	*val_mv = (*val_mv * num) / den;
}

#endif /* CONFIG_ADC_ESP32_DMA */

static int adc_esp32_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct adc_esp32_data *data = dev->data;

	uint8_t channel_id = find_lsb_set(seq->channels) - 1;

	if (seq->buffer_size < 2) {
		LOG_ERR("Sequence buffer space too low '%d'", seq->buffer_size);
		return -ENOMEM;
	}

#ifndef CONFIG_ADC_ESP32_DMA
	if (seq->channels > BIT(channel_id)) {
		LOG_ERR("Multi-channel readings not supported");
		return -ENOTSUP;
	}

	if (seq->options && seq->options->extra_samplings) {
		LOG_ERR("Extra samplings not supported");
		return -ENOTSUP;
	}

	if (seq->options && seq->options->interval_us) {
		LOG_ERR("Interval between samplings not supported");
		return -ENOTSUP;
	}
#endif /* CONFIG_ADC_ESP32_DMA */

	if (!VALID_RESOLUTION(seq->resolution)) {
		LOG_ERR("unsupported resolution (%d)", seq->resolution);
		return -ENOTSUP;
	}

	if (seq->calibrate) {
		/* TODO: Does this mean actual Vref measurement on selected GPIO ?*/
		LOG_ERR("calibration is not supported");
		return -ENOTSUP;
	}

	data->resolution[channel_id] = seq->resolution;

#ifdef CONFIG_ADC_ESP32_DMA
	int err = adc_esp32_dma_read(dev, seq);

	if (err < 0) {
		return err;
	}
#else
	uint32_t acq_raw;

	if (seq->channels > BIT(channel_id)) {
		LOG_ERR("Multi-channel readings not supported");
		return -ENOTSUP;
	}

	if (seq->options && seq->options->extra_samplings) {
		LOG_ERR("Extra samplings not supported");
		return -ENOTSUP;
	}

	if (seq->options && seq->options->interval_us) {
		LOG_ERR("Interval between samplings not supported");
		return -ENOTSUP;
	}

	adc_oneshot_hal_setup(&data->hal, channel_id);

#if SOC_ADC_CALIBRATION_V1_SUPPORTED
	adc_set_hw_calibration_code(data->hal.unit, data->attenuation[channel_id]);
#endif /* SOC_ADC_CALIBRATION_V1_SUPPORTED */

	adc_oneshot_hal_convert(&data->hal, &acq_raw);

	if (data->cal_handle[channel_id]) {
		if (data->meas_ref_internal > 0) {
			uint32_t acq_mv;

			adc_cali_raw_to_voltage(data->cal_handle[channel_id], acq_raw, &acq_mv);

			LOG_DBG("ADC acquisition [unit: %u, chan: %u, acq_raw: %u, acq_mv: %u]",
				data->hal.unit, channel_id, acq_raw, acq_mv);

#if CONFIG_SOC_SERIES_ESP32
			if (data->attenuation[channel_id] == ADC_ATTEN_DB_12 &&
			    acq_mv > ADC_CLIP_MVOLT_12DB) {
				acq_mv = ADC_CLIP_MVOLT_12DB;
			}
#endif /* CONFIG_SOC_SERIES_ESP32 */

			/* Fit according to selected attenuation */
			atten_to_gain(data->attenuation[channel_id], &acq_mv);
			acq_raw = acq_mv * ((1 << data->resolution[channel_id]) - 1) /
				  data->meas_ref_internal;
		} else {
			LOG_WRN("ADC reading is uncompensated");
		}
	} else {
		LOG_WRN("ADC reading is uncompensated");
	}

	/* Store result */
	data->buffer = (uint16_t *)seq->buffer;
	data->buffer[0] = acq_raw;
#endif /* CONFIG_ADC_ESP32_DMA */

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_esp32_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	(void)(dev);
	(void)(sequence);
	(void)(async);

	return -ENOTSUP;
}
#endif /* CONFIG_ADC_ASYNC */

static int adc_esp32_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	const struct adc_esp32_conf *conf = (const struct adc_esp32_conf *)dev->config;
	struct adc_esp32_data *data = (struct adc_esp32_data *)dev->data;
	int err;

	if (cfg->channel_id >= conf->channel_count) {
		LOG_ERR("Unsupported channel id '%d'", cfg->channel_id);
		return -ENOTSUP;
	}

	if (cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference '%d'", cfg->reference);
		return -ENOTSUP;
	}

	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported acquisition_time '%d'", cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	if (gain_to_atten(cfg->gain, &data->attenuation[cfg->channel_id])) {
		LOG_ERR("Unsupported gain value '%d'", cfg->gain);
		return -ENOTSUP;
	}

#ifdef CONFIG_ADC_ESP32_DMA
	err = adc_esp32_dma_channel_setup(dev, cfg);

	if (err < 0) {
		return err;
	}
#else
	adc_atten_t old_atten = data->attenuation[cfg->channel_id];

	adc_oneshot_hal_chan_cfg_t config = {
		.atten = data->attenuation[cfg->channel_id],
		.bitwidth = data->resolution[cfg->channel_id],
	};

	adc_oneshot_hal_channel_config(&data->hal, &config, cfg->channel_id);

	if ((data->cal_handle[cfg->channel_id] == NULL) ||
	    (data->attenuation[cfg->channel_id] != old_atten)) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
		adc_cali_curve_fitting_config_t cal_config = {
			.unit_id = conf->unit,
			.chan = cfg->channel_id,
			.atten = data->attenuation[cfg->channel_id],
			.bitwidth = data->resolution[cfg->channel_id],
		};

		LOG_DBG("Curve fitting calib [unit_id: %u, chan: %u, atten: %u, bitwidth: %u]",
			conf->unit, cfg->channel_id, data->attenuation[cfg->channel_id],
			data->resolution[cfg->channel_id]);

		if (data->cal_handle[cfg->channel_id] != NULL) {
			/* delete pre-existing calib scheme */
			adc_cali_delete_scheme_curve_fitting(data->cal_handle[cfg->channel_id]);
		}

		adc_cali_create_scheme_curve_fitting(&cal_config,
						     &data->cal_handle[cfg->channel_id]);
#endif /* ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED */

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
		adc_cali_line_fitting_config_t cal_config = {
			.unit_id = conf->unit,
			.atten = data->attenuation[cfg->channel_id],
			.bitwidth = data->resolution[cfg->channel_id],
#if CONFIG_SOC_SERIES_ESP32
			.default_vref = data->meas_ref_internal
#endif
		};

		LOG_DBG("Line fitting calib [unit_id: %u, chan: %u, atten: %u, bitwidth: %u]",
			conf->unit, cfg->channel_id, data->attenuation[cfg->channel_id],
			data->resolution[cfg->channel_id]);

		if (data->cal_handle[cfg->channel_id] != NULL) {
			/* delete pre-existing calib scheme */
			adc_cali_delete_scheme_line_fitting(data->cal_handle[cfg->channel_id]);
		}

		adc_cali_create_scheme_line_fitting(&cal_config,
						    &data->cal_handle[cfg->channel_id]);
#endif /* ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED */
	}
#endif /* CONFIG_ADC_ESP32_DMA */

	/* GPIO config for ADC mode */

	int io_num = adc_channel_io_map[conf->unit][cfg->channel_id];

	if (io_num < 0) {
		LOG_ERR("Channel %u not supported!", cfg->channel_id);
		return -ENOTSUP;
	}

	struct gpio_dt_spec gpio = {
		.port = conf->gpio_port,
		.dt_flags = 0,
		.pin = io_num,
	};

	err = gpio_pin_configure_dt(&gpio, GPIO_DISCONNECTED);

	if (err) {
		LOG_ERR("Error disconnecting io (%d)", io_num);
		return err;
	}

	return 0;
}

static int adc_esp32_init(const struct device *dev)
{
	struct adc_esp32_data *data = (struct adc_esp32_data *)dev->data;
	const struct adc_esp32_conf *conf = (struct adc_esp32_conf *)dev->config;
	uint32_t clock_src_hz;


#if SOC_ADC_DIG_CTRL_SUPPORTED && (!SOC_ADC_RTC_CTRL_SUPPORTED || CONFIG_ADC_ESP32_DMA)
	if (!device_is_ready(conf->clock_dev)) {
		return -ENODEV;
	}

	clock_control_on(conf->clock_dev, conf->clock_subsys);
#endif

	esp_clk_tree_src_get_freq_hz(ADC_DIGI_CLK_SRC_DEFAULT,
				     ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &clock_src_hz);

	if (!device_is_ready(conf->gpio_port)) {
		LOG_ERR("gpio0 port not ready");
		return -ENODEV;
	}

#ifdef CONFIG_ADC_ESP32_DMA
	int err = adc_esp32_dma_init(dev);

	if (err < 0) {
		return err;
	}
#else
	adc_oneshot_hal_cfg_t config = {
		.unit = conf->unit,
		.work_mode = ADC_HAL_SINGLE_READ_MODE,
		.clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
		.clk_src_freq_hz = clock_src_hz,
	};

	adc_oneshot_hal_init(&data->hal, &config);

	sar_periph_ctrl_adc_oneshot_power_acquire();
#endif /* CONFIG_ADC_ESP32_DMA */

	for (uint8_t i = 0; i < SOC_ADC_MAX_CHANNEL_NUM; i++) {
		data->resolution[i] = ADC_RESOLUTION_MAX;
		data->attenuation[i] = ADC_ATTEN_DB_0;
		data->cal_handle[i] = NULL;
	}

	/* Default reference voltage. This could be calibrated externaly */
	data->meas_ref_internal = ADC_ESP32_DEFAULT_VREF_INTERNAL;

	adc_hw_calibration(conf->unit);

	return 0;
}

static DEVICE_API(adc, api_esp32_driver_api) = {
	.channel_setup = adc_esp32_channel_setup,
	.read = adc_esp32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_esp32_read_async,
#endif /* CONFIG_ADC_ASYNC */
	.ref_internal = ADC_ESP32_DEFAULT_VREF_INTERNAL,
};

#if defined(CONFIG_ADC_ESP32_DMA) && SOC_GDMA_SUPPORTED
#define ADC_ESP32_CONF_INIT(n)                                                                     \
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(n, 0)),                                  \
	.dma_channel = DT_INST_DMAS_CELL_BY_IDX(n, 0, channel)
#else
#define ADC_ESP32_CONF_INIT(n)
#endif /* defined(SOC_GDMA_SUPPORTED) && SOC_GDMA_SUPPORTED */

#define ADC_ESP32_CHECK_CHANNEL_REF(chan)                                                          \
	BUILD_ASSERT(DT_ENUM_HAS_VALUE(chan, zephyr_reference, adc_ref_internal),                  \
		     "adc_esp32 only supports ADC_REF_INTERNAL as a reference");

#define ESP32_ADC_INIT(inst)                                                                       \
                                                                                                   \
	DT_INST_FOREACH_CHILD(inst, ADC_ESP32_CHECK_CHANNEL_REF)                                   \
                                                                                                   \
	static const struct adc_esp32_conf adc_esp32_conf_##inst = {                               \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, offset),         \
		.unit = DT_PROP(DT_DRV_INST(inst), unit) - 1,                                      \
		.channel_count = DT_PROP(DT_DRV_INST(inst), channel_count),                        \
		.gpio_port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),                                   \
		ADC_ESP32_CONF_INIT(inst)};                                                        \
                                                                                                   \
	static struct adc_esp32_data adc_esp32_data_##inst = {                                     \
		.hal =                                                                             \
			{                                                                          \
				.dev = (adc_oneshot_soc_handle_t)DT_INST_REG_ADDR(inst),           \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, adc_esp32_init, NULL, &adc_esp32_data_##inst,                  \
			      &adc_esp32_conf_##inst, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,       \
			      &api_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_ADC_INIT)
