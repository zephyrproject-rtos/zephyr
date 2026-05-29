/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_tsen

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bflb_tsen, CONFIG_SENSOR_LOG_LEVEL);

/*
 * TSEN formula and constants come from the BFLB SDK (e.g.
 * components/platform/soc/bl70x/bl70x_std/BSP_Driver/std_drv/src/bl702_tsen.c):
 *
 *     T[degC] = (delta - offset) / 7.753
 *
 *   - delta:  |v0 - v1| produced by the BFLB ADC driver's two-phase TSEN read.
 *             The sensor driver sees this as a single raw sample.
 *   - offset: efuse trim value when programmed, else TSEN_DEFAULT_OFFSET.
 *   - 7.753:  slope of the on-chip bandgap-referenced sensor in counts/degC.
 */
#define TSEN_DEFAULT_OFFSET 2042

/* 7.753 counts/degC -> milli-degC = (counts * 1_000_000) / 7753. */
#define TSEN_SLOPE_NUM_MILLI 1000000
#define TSEN_SLOPE_DEN       7753

/* Efuse register offsets and bit positions for TSEN trim */
#if defined(CONFIG_SOC_SERIES_BL61X)
#define TSEN_EFUSE_TRIM_REG        0xF0
#define TSEN_EFUSE_TRIM_EN_BIT     BIT(13)
#define TSEN_EFUSE_TRIM_VALUE_MASK 0xFFF
#elif defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL) ||                     \
	defined(CONFIG_SOC_SERIES_BL60X)
#define TSEN_EFUSE_ENABLE_REG      0x78
#define TSEN_EFUSE_ENABLE_BIT      BIT(0)
#define TSEN_EFUSE_TRIM_REG        0x7C
#define TSEN_EFUSE_TRIM_VALUE_MASK 0xFFF
#endif

struct bflb_tsen_config {
	struct adc_dt_spec adc;
};

struct bflb_tsen_data {
	/* Last measured die temperature in milli-degrees Celsius. */
	int32_t temp_mc;
	uint32_t tsen_offset;
};

/**
 * @brief Convert a TSEN delta sample into a temperature in milli-degC.
 *
 * @param delta  |v0 - v1| returned by the BFLB ADC driver's TSEN read.
 * @param offset Calibration offset (from efuse or TSEN_DEFAULT_OFFSET).
 *
 * Computes T = (delta - offset) / 7.753  [degC] in pure integer math,
 * returned as milli-degC so the result can be negative and fractional.
 */
static int32_t tsen_compute_temp_mc(uint32_t delta, uint32_t offset)
{
	int64_t numerator = (int64_t)((int32_t)delta - (int32_t)offset) * TSEN_SLOPE_NUM_MILLI;

	return (int32_t)(numerator / TSEN_SLOPE_DEN);
}

static int bflb_tsen_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct bflb_tsen_config *cfg = dev->config;
	struct bflb_tsen_data *data = dev->data;
	uint16_t sample;
	struct adc_sequence seq = {
		.buffer = &sample,
		.buffer_size = sizeof(sample),
	};
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	(void)adc_sequence_init_dt(&cfg->adc, &seq);

	ret = adc_read(cfg->adc.dev, &seq);
	if (ret < 0) {
		return ret;
	}

	data->temp_mc = tsen_compute_temp_mc(sample, data->tsen_offset);

	LOG_DBG("delta=%u offset=%u temp=%d.%03d degC", sample, data->tsen_offset,
		data->temp_mc / 1000, (data->temp_mc < 0 ? -data->temp_mc : data->temp_mc) % 1000);

	return 0;
}

static int bflb_tsen_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct bflb_tsen_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	/* milli-degC -> sensor_value (val1 = degC, val2 = micro-degC, same sign). */
	val->val1 = data->temp_mc / 1000;
	val->val2 = (data->temp_mc % 1000) * 1000;
	return 0;
}

static uint32_t tsen_read_efuse_trim(void)
{
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);

	if (!device_is_ready(efuse)) {
		LOG_WRN("efuse device not ready, using default TSEN offset");
		return TSEN_DEFAULT_OFFSET;
	}

#if defined(CONFIG_SOC_SERIES_BL61X)
	uint32_t val;
	int ret;

	ret = syscon_read_reg(efuse, TSEN_EFUSE_TRIM_REG, &val);
	if (ret < 0) {
		LOG_WRN("Failed to read TSEN efuse trim: %d", ret);
		return TSEN_DEFAULT_OFFSET;
	}
	if ((val & TSEN_EFUSE_TRIM_EN_BIT) == 0) {
		LOG_WRN("TSEN efuse trim not programmed, using default");
		return TSEN_DEFAULT_OFFSET;
	}
	return val & TSEN_EFUSE_TRIM_VALUE_MASK;
#elif defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL) ||                     \
	defined(CONFIG_SOC_SERIES_BL60X)
	uint32_t val;
	int ret;

	ret = syscon_read_reg(efuse, TSEN_EFUSE_ENABLE_REG, &val);
	if (ret < 0) {
		LOG_WRN("Failed to read TSEN efuse enable: %d", ret);
		return TSEN_DEFAULT_OFFSET;
	}
	if ((val & TSEN_EFUSE_ENABLE_BIT) == 0) {
		LOG_WRN("TSEN efuse trim not programmed, using default");
		return TSEN_DEFAULT_OFFSET;
	}
	ret = syscon_read_reg(efuse, TSEN_EFUSE_TRIM_REG, &val);
	if (ret < 0) {
		LOG_WRN("Failed to read TSEN efuse trim: %d", ret);
		return TSEN_DEFAULT_OFFSET;
	}
	return val & TSEN_EFUSE_TRIM_VALUE_MASK;
#else
	return TSEN_DEFAULT_OFFSET;
#endif
}

static int bflb_tsen_init(const struct device *dev)
{
	const struct bflb_tsen_config *cfg = dev->config;
	struct bflb_tsen_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&cfg->adc)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&cfg->adc);
	if (ret < 0) {
		LOG_ERR("ADC channel setup failed: %d", ret);
		return ret;
	}

	data->tsen_offset = tsen_read_efuse_trim();
	LOG_DBG("TSEN offset from efuse: %u", data->tsen_offset);

	return 0;
}

static DEVICE_API(sensor, bflb_tsen_api) = {
	.sample_fetch = bflb_tsen_sample_fetch,
	.channel_get = bflb_tsen_channel_get,
};

#define BFLB_TSEN_DEFINE(inst)                                                                     \
	static const struct bflb_tsen_config bflb_tsen_config_##inst = {                           \
		.adc = ADC_DT_SPEC_INST_GET_BY_IDX(inst, 0),                                       \
	};                                                                                         \
	static struct bflb_tsen_data bflb_tsen_data_##inst;                                        \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bflb_tsen_init, NULL, &bflb_tsen_data_##inst,           \
				     &bflb_tsen_config_##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &bflb_tsen_api);

DT_INST_FOREACH_STATUS_OKAY(BFLB_TSEN_DEFINE)
