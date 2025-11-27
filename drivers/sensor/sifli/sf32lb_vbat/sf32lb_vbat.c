/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_vbat

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/nvmem.h>

#include <register.h>

#define SYS_CFG_ANAU_CR offsetof(HPSYS_CFG_TypeDef, ANAU_CR)

LOG_MODULE_REGISTER(sf32lb_vbat, CONFIG_SENSOR_LOG_LEVEL);

#define ADC_RATIO_ACCURATE 1000

struct factory_cfg_adc {
	uint16_t vol10;       /*!< Reg value for low voltage. */
	uint16_t vol25;       /*!< Reg value for high voltage. */
	uint16_t low_mv;      /*!< voltage for low with mv . */
	uint16_t high_mv;     /*!< voltage for high with mv. */
	uint16_t vbat_reg;    /*!< Reg value for reference vbat */
	uint16_t vbat_mv;     /*!< voltage for vbat with mv. */
	uint8_t ldovref_flag; /*!< ldo vref flag, if 1, gpadc ldo vref has been calibrated. */
	uint8_t ldovref_sel;  /*!< ldo vref sleect. */
};

struct sf32lb_vbat_config {
	uintptr_t cfg_base;
	const struct device *adc;
	const struct adc_channel_cfg adc_cfg;
	uint32_t ratio;
	const struct nvmem_cell calib_cell; /* NVMEM cell for storing calibration data */
};

struct sf32lb_vbat_data {
	struct adc_sequence adc_seq;
	struct k_mutex lock;
	int16_t sample_buffer;
	int16_t raw;
	int16_t calibration_offset;
	bool calibration_valid;
	struct factory_cfg_adc factory_cfg;
	uint16_t offset;
	float adc_vol_ratio;
	float adc_vbat_factor; /* VBAT calibration factor */
};

static void vbat_fact_calib(uint32_t voltage, uint32_t reg, struct sf32lb_vbat_data *data)
{
	float vol_from_reg;

	vol_from_reg = ((float)(reg - data->offset) * data->adc_vol_ratio) / ADC_RATIO_ACCURATE;
	data->adc_vbat_factor = (float)voltage / vol_from_reg;
}

static void calc_cali_param(const struct device *dev)
{
	struct sf32lb_vbat_data *data = dev->data;
	struct factory_cfg_adc *cfg = &data->factory_cfg;
	uint32_t vol1, vol2;

	if (data->calibration_valid) {
		cfg->vol10 &= 0x7fff;
		cfg->vol25 &= 0x7fff;
		vol1 = cfg->low_mv;
		vol2 = cfg->high_mv;

		float gap1 = (float)(vol2 > vol1 ? vol2 - vol1 : vol1 - vol2);
		float gap2 = (float)(cfg->high_mv > cfg->low_mv ? cfg->high_mv - cfg->low_mv
								: cfg->low_mv - cfg->high_mv);

		if (gap1 != 0) {
			data->adc_vol_ratio = (gap2 * ADC_RATIO_ACCURATE) / gap1;

			data->offset = cfg->vol10 - (uint16_t)((vol1 * ADC_RATIO_ACCURATE) /
							       data->adc_vol_ratio);
		}

		vbat_fact_calib(cfg->vbat_mv, cfg->vbat_reg, data);
	}
}

static void get_adc_cfg(struct factory_cfg_adc *cfg, uint8_t *data)
{
	cfg->vol10 = (uint16_t)data[4] | ((uint16_t)(data[5] & 0xf) << 8);
	cfg->low_mv = ((data[5] & 0xf0) >> 4) | ((data[6] & 1) << 4);
	cfg->vol25 = (uint16_t)((data[6] & 0xfe) >> 1) | ((uint16_t)(data[7] & 0x1f) << 7);
	cfg->high_mv = ((data[7] & 0xe0) >> 5) | ((data[8] & 0x3) << 3);
	cfg->vbat_reg = ((uint16_t)(data[8] & 0xfc) >> 2) | ((uint16_t)(data[9] & 0x3f) << 6);
	cfg->vbat_mv = ((data[9] & 0xc0) >> 6) | ((data[10] & 0xf) << 2);
	cfg->low_mv *= 100;
	cfg->high_mv *= 100;
	cfg->vbat_mv *= 100;
}

static int adc_sf32lb_read_calibration(const struct device *dev)
{
	const struct sf32lb_vbat_config *config = dev->config;
	struct sf32lb_vbat_data *data = dev->data;
	struct factory_cfg_adc *cfg = &data->factory_cfg;
	int ret;
	uint8_t buf[32] = {0};

	ret = nvmem_cell_read(&config->calib_cell, buf, 0, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read calibration data from NVMEM: %d", ret);
		return ret;
	}
	get_adc_cfg(cfg, buf);

	if (cfg->vol10 == 0 || cfg->vol25 == 0) {
		LOG_WRN("Calibration data is invalid");
		data->calibration_valid = false;
		return 0;
	}

	cfg->vol10 &= 0x7fff;
	cfg->vol25 &= 0x7fff;

	data->calibration_valid = true;
	calc_cali_param(dev);

	return ret;
}

static float sf32lb_vbat_get_mv(const struct device *dev, float raw_value)
{
	struct sf32lb_vbat_data *data = dev->data;
	float offset, ratio;

	offset = (float)data->offset;
	ratio = data->adc_vol_ratio;

	if (raw_value < offset) {
		return 0.0;
	}

	return ((raw_value - offset) * ratio) / ADC_RATIO_ACCURATE;
}

static int sf32lb_vbat_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct sf32lb_vbat_config *cfg = dev->config;
	struct sf32lb_vbat_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	rc = adc_channel_setup(cfg->adc, &cfg->adc_cfg);
	if (rc < 0) {
		LOG_ERR("ADC channel setup failed (%d)", rc);
		goto out;
	}

	rc = adc_read(cfg->adc, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

out:
	k_mutex_unlock(&data->lock);
	return rc;
}

static int sf32lb_vbat_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct sf32lb_vbat_data *data = dev->data;
	float voltage_mv;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	voltage_mv = sf32lb_vbat_get_mv(dev, (float)data->raw);

	sensor_value_from_milli(val, voltage_mv);

	return 0;
}

static const struct sensor_driver_api sf32lb_vbat_driver_api = {
	.sample_fetch = sf32lb_vbat_sample_fetch,
	.channel_get = sf32lb_vbat_channel_get,
};

static int sf32lb_vbat_init(const struct device *dev)
{
	const struct sf32lb_vbat_config *cfg = dev->config;
	struct sf32lb_vbat_data *data = dev->data;
	struct adc_sequence *asp = &data->adc_seq;

	sys_set_bit(cfg->cfg_base + SYS_CFG_ANAU_CR, HPSYS_CFG_ANAU_CR_EN_VBAT_MON_Pos);

	k_mutex_init(&data->lock);

	if (!device_is_ready(cfg->adc)) {
		LOG_ERR("ADC device %s is not ready", cfg->adc->name);
		return -ENODEV;
	}

	*asp = (struct adc_sequence){
		.channels = BIT(cfg->adc_cfg.channel_id),
		.buffer = &data->sample_buffer,
		.buffer_size = sizeof(data->sample_buffer),
		.resolution = 12U,
	};
	adc_sf32lb_read_calibration(dev);
	return 0;
}

#define SF32LB_VBAT_DEFINE(inst)                                                                   \
	static struct sf32lb_vbat_data sf32lb_vbat_data_##inst;                                    \
                                                                                                   \
	static const struct sf32lb_vbat_config sf32lb_vbat_config_##inst = {                       \
		.cfg_base = DT_REG_ADDR(DT_INST_PHANDLE(inst, sifli_cfg)),                         \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                              \
		.adc_cfg = {                                                                       \
			.gain = ADC_GAIN_1,                                                        \
			.reference = ADC_REF_INTERNAL,                                             \
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,                                  \
			.channel_id = DT_INST_IO_CHANNELS_INPUT(inst),                             \
			.differential = 0,                                                         \
		},                                                                                 \
		.ratio = DT_INST_PROP(inst, ratio),                                                \
		.calib_cell = NVMEM_CELL_GET_BY_NAME(DT_NODELABEL(vbat), adc_nvmem_cell)           \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sf32lb_vbat_init, NULL, &sf32lb_vbat_data_##inst,       \
				     &sf32lb_vbat_config_##inst, POST_KERNEL,                      \
				     CONFIG_SENSOR_INIT_PRIORITY, &sf32lb_vbat_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_VBAT_DEFINE)
