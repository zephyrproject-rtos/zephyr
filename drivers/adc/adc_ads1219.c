/*
 * Copyright (c) 2025 Romain Paupe <rpaupe@free.fr>, Franck Duriez <franck.lucien.duriez@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr driver for TI ADS1219 24bit ADC
 *
 * Datasheet:
 * https://www.ti.com/lit/ds/symlink/ads1219.pdf
 */
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

#define DT_DRV_COMPAT ti_ads1219

LOG_MODULE_REGISTER(ADS1219, CONFIG_ADC_LOG_LEVEL);

#define ADS1219_VREF_INTERNAL_VALUE 2048
#define ADS1219_START_CMD           (0x08)
#define ADS1219_WREG_CMD            (0x40)
#define ADS1219_RREG_CMD            (0x20)
#define ADS1219_RDRDY_CMD           (0x24)
#define ADS1219_RDATA_CMD           (0x10)

#define ADS1219_CONFIG_MUX(x)  (((x) & BIT_MASK(3)) << 5)
#define ADS1219_CONFIG_GAIN(x) (((x) & BIT_MASK(1)) << 4)
#define ADS1219_CONFIG_DR(x)   (((x) & BIT_MASK(2)) << 2)
#define ADS1219_CONFIG_CM(x)   (((x) & BIT_MASK(1)) << 1)
#define ADS1219_CONFIG_REF(x)  (((x) & BIT_MASK(1))

enum {
	ADS1219_MUX_AIN0_AIN1 = 0b000,
	ADS1219_MUX_AIN2_AIN3 = 0b001,
	ADS1219_MUX_AIN1_AIN2 = 0b010,
	ADS1219_MUX_AIN0_AGND = 0b011,
	ADS1219_MUX_AIN1_AGND = 0b100,
	ADS1219_MUX_AIN2_AGND = 0b101,
	ADS1219_MUX_AIN3_AGND = 0b110,
};

enum {
	ADS1219_GAIN_1 = 0,
	ADS1219_GAIN_4 = 1,
};

enum {
	ADS1219_DR_20_SPS = 0,
	ADS1219_DR_90_SPS = 1,
	ADS1219_DR_330_SPS = 2,
	ADS1219_DR_1000_SPS = 3,
	ADS1219_DR_DEFAULT = ADS1219_DR_20_SPS,
};

enum {
	ADS1219_CM_SINGLE = 0,
	ADS1219_CM_CONTINUOUS = 1,
};

enum {
	ADS1219_VREF_INTERNAL = 0,
	ADS1219_VREF_EXTERNAL = 1,
};

struct ads1219_config {
	const struct i2c_dt_spec bus;
};

struct ads1219_data {
	struct adc_context ctx;
	k_timeout_t ready_time;
	struct k_sem acq_sem;
	uint8_t config_cmd;
	uint8_t gain;
	uint32_t *buffer;
	uint32_t *buffer_ptr;
};

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads1219_data *data = CONTAINER_OF(ctx, struct ads1219_data, ctx);

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acq_sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads1219_data *data = CONTAINER_OF(ctx, struct ads1219_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static int ads1219_read_data(const struct device *dev, uint32_t *output)
{
	const struct ads1219_config *config = dev->config;
	uint8_t cmd = ADS1219_RDATA_CMD;

	return i2c_write_read_dt(&config->bus, &cmd, sizeof(cmd), output, 3);
}

static int ads1219_write_reg(const struct device *dev)
{
	const struct ads1219_data *data = dev->data;
	const struct ads1219_config *config = dev->config;

	uint8_t cmd[2] = {ADS1219_WREG_CMD, data->config_cmd};
	int ret = i2c_write_dt(&config->bus, cmd, sizeof(cmd));

	if (ret != 0) {
		LOG_ERR("Failed to write i2c data");
	}
	return ret;
}

static int ads1219_start_sync(const struct device *dev)
{
	const struct ads1219_config *config = dev->config;
	uint8_t cmd = ADS1219_START_CMD;

	return i2c_write_dt(&config->bus, &cmd, sizeof(cmd));
}

static int ads1219_wait_data_ready(const struct device *dev)
{
	const struct ads1219_config *config = dev->config;
	uint8_t reg = 0;
	uint8_t data_ready = 0;
	int ret = 0;

	for (int time_out = 1000; time_out > 0; --time_out) {
		uint8_t const cmd = ADS1219_RDRDY_CMD;

		ret = i2c_write_read_dt(&config->bus, &cmd, sizeof(cmd), &reg, 1);
		if (ret != 0) {
			LOG_ERR("Failed to read ready state");
			return ret;
		}

		data_ready = reg >> 7;
		if (data_ready) {
			break;
		}

		k_msleep(1);
	}

	return 0;
}

static inline int ads1219_acq_time_to_dr(const struct device *dev, uint16_t acq_time)
{
	struct ads1219_data *data = dev->data;
	int odr = -EINVAL;
	uint16_t acq_value = ADC_ACQ_TIME_VALUE(acq_time);
	uint32_t ready_time_us = 0;

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		acq_time = ADS1219_DR_DEFAULT;
	} else if (ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
		return -EINVAL;
	}

	switch (acq_value) {
	case ADS1219_DR_20_SPS:
		odr = ADS1219_DR_20_SPS;
		ready_time_us = (1000 * 1000) / 20;
		break;
	case ADS1219_DR_90_SPS:
		odr = ADS1219_DR_90_SPS;
		ready_time_us = (1000 * 1000) / 90;
		break;
	case ADS1219_DR_330_SPS:
		odr = ADS1219_DR_330_SPS;
		ready_time_us = (1000 * 1000) / 330;
		break;
	case ADS1219_DR_1000_SPS:
		odr = ADS1219_DR_1000_SPS;
		ready_time_us = (1000 * 1000) / 1000;
		break;
	default:
		break;
	}

	/* Add some additional time to ensure that the data is truly ready,
	 * as chips in this family often require some additional time beyond
	 * the listed times
	 */
	data->ready_time = K_USEC(ready_time_us + 10);

	return odr;
}

static int ads1219_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	struct ads1219_data *data = dev->data;

	data->config_cmd = 0;

	int dr = 0;

	if (channel_cfg->differential) {
		if (channel_cfg->input_positive == 0 && channel_cfg->input_negative == 1) {
			data->config_cmd |= ADS1219_CONFIG_MUX(ADS1219_MUX_AIN0_AIN1);
		} else if (channel_cfg->input_positive == 2 && channel_cfg->input_negative == 3) {
			data->config_cmd |= ADS1219_CONFIG_MUX(ADS1219_MUX_AIN2_AIN3);
		} else if (channel_cfg->input_positive == 1 && channel_cfg->input_negative == 2) {
			data->config_cmd |= ADS1219_CONFIG_MUX(ADS1219_MUX_AIN1_AIN2);
		} else {
			LOG_ERR("Invalid differential config");
			return -EINVAL;
		}
	} else {
		if (channel_cfg->input_positive == 0) {
			data->config_cmd |= ADS1219_CONFIG_MUX(ADS1219_MUX_AIN0_AGND);
		} else if (channel_cfg->input_positive == 1) {
			data->config_cmd |= ADS1219_CONFIG_MUX(ADS1219_MUX_AIN1_AGND);
		} else if (channel_cfg->input_positive == 2) {
			data->config_cmd |= ADS1219_CONFIG_MUX(ADS1219_MUX_AIN2_AGND);
		} else if (channel_cfg->input_positive == 3) {
			data->config_cmd |= ADS1219_CONFIG_MUX(ADS1219_MUX_AIN3_AGND);
		} else {
			LOG_ERR("Invalid differential config");
			return -EINVAL;
		}
	}

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		data->config_cmd |= ADS1219_CONFIG_GAIN(ADS1219_GAIN_1);
		break;
	case ADC_GAIN_4:
		data->config_cmd |= ADS1219_CONFIG_GAIN(ADS1219_GAIN_4);
		break;
	default:
		LOG_ERR("Invalid reference config");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		data->config_cmd |= ADS1219_VREF_INTERNAL;
		break;
	case ADC_REF_EXTERNAL0:
		data->config_cmd |= ADS1219_VREF_EXTERNAL;
		break;
	default:
		LOG_ERR("Invalid reference config");
		return -EINVAL;
	}

	dr = ads1219_acq_time_to_dr(dev, channel_cfg->acquisition_time);
	if (dr < 0) {
		LOG_ERR("Invalid data rate");
		return dr;
	}

	data->config_cmd |= ADS1219_CONFIG_DR(dr);

	data->config_cmd |= ADS1219_CONFIG_CM(ADS1219_CM_CONTINUOUS);
	return ads1219_write_reg(dev);
}

static int ads1219_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int32_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		LOG_ERR("Insufficient buffer %d < %d", (int)sequence->buffer_size, (int)needed);
		return -ENOMEM;
	}

	return 0;
}

static int ads1219_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	if (sequence->oversampling) {
		LOG_ERR("Oversampling not supported");
		return -EINVAL;
	}

	return ads1219_validate_buffer_size(sequence);
}

static int ads1219_adc_start_read(const struct device *dev, const struct adc_sequence *sequence,
				  bool wait)
{
	int rc = 0;
	struct ads1219_data *data = dev->data;

	rc = ads1219_validate_sequence(dev, sequence);
	if (rc != 0) {
		return rc;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	if (wait) {
		rc = adc_context_wait_for_completion(&data->ctx);
	}
	return rc;
}

static int ads1219_adc_perform_read(const struct device *dev)
{
	struct ads1219_data *data = dev->data;

	k_sem_take(&data->acq_sem, K_FOREVER);

	int rc = 0;

	rc = ads1219_start_sync(dev);
	if (rc != 0) {
		LOG_ERR("Failed to start sync");
		goto failure;
	}

	rc = ads1219_wait_data_ready(dev);
	if (rc != 0) {
		LOG_ERR("Failed to wait for data");
		goto failure;
	}

	rc = ads1219_read_data(dev, data->buffer);
	if (rc != 0) {
		LOG_ERR("Failed to read data");
		goto failure;
	}

	LOG_DBG("value: %x", *data->buffer);
	*data->buffer = sys_be24_to_cpu(*data->buffer);
	LOG_DBG("value: %x %" PRIi32, *data->buffer, *data->buffer);
	++data->buffer;

	adc_context_on_sampling_done(&data->ctx, dev);

	return rc;

failure:
	adc_context_complete(&data->ctx, rc);
	return rc;
}

static int ads1219_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int rc = 0;
	struct ads1219_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);

	rc = ads1219_adc_start_read(dev, sequence, false);
	while (rc == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		rc = ads1219_adc_perform_read(dev);
	}

	adc_context_release(&data->ctx, rc);
	return rc;
}

static int ads1219_init(const struct device *dev)
{
	const struct ads1219_config *config = dev->config;
	struct ads1219_data *data = dev->data;

	adc_context_init(&data->ctx);
	k_sem_init(&data->acq_sem, 0, 1);
	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static DEVICE_API(adc, api) = {
	.channel_setup = ads1219_channel_setup,
	.ref_internal = ADS1219_VREF_INTERNAL_VALUE,
	.read = ads1219_read,
};

#define ADC_ADS1219_INST_DEFINE(n)                                                                 \
	static const struct ads1219_config config_##n = {                                          \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
	static struct ads1219_data data_##n;                                                       \
	DEVICE_DT_INST_DEFINE(n, ads1219_init, NULL, &data_##n, &config_##n, POST_KERNEL,          \
			      CONFIG_ADC_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS1219_INST_DEFINE);
