/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2020 Innoseis BV
 * Copyright (c) 2023 Cruise LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

#define DT_DRV_COMPAT ti_ads1112

LOG_MODULE_REGISTER(ADS1112, CONFIG_ADC_LOG_LEVEL);

#define ADS1112_CONFIG_GAIN(x) ((x)&BIT_MASK(2))
#define ADS1112_CONFIG_DR(x)   (((x)&BIT_MASK(2)) << 2)
#define ADS1112_CONFIG_CM(x)   (((x)&BIT_MASK(1)) << 4)
#define ADS1112_CONFIG_MUX(x)  (((x)&BIT_MASK(2)) << 5)

#define ADS1112_CONFIG_MASK_READY BIT(7)

#define ADS1112_DEFAULT_CONFIG 0x8C
#define ADS1112_REF_INTERNAL   2048

enum ads1112_reg {
	ADS1112_REG_OUTPUT = 0,
	ADS1112_REG_CONFIG = 1,
};

enum {
	ADS1112_CONFIG_MUX_DIFF_0_1 = 0,
	ADS1112_CONFIG_MUX_BOTH_2_3 = 1,
	ADS1112_CONFIG_MUX_SINGLE_0_3 = 2,
	ADS1112_CONFIG_MUX_SINGLE_1_3 = 3,
};

enum {
	ADS1112_CONFIG_DR_RATE_240_RES_12 = 0,
	ADS1112_CONFIG_DR_RATE_60_RES_14 = 1,
	ADS1112_CONFIG_DR_RATE_30_RES_15 = 2,
	ADS1112_CONFIG_DR_RATE_15_RES_16 = 3,
	ADS1112_CONFIG_DR_DEFAULT = ADS1112_CONFIG_DR_RATE_15_RES_16,
};

enum {
	ADS1112_CONFIG_GAIN_1 = 0,
	ADS1112_CONFIG_GAIN_2 = 1,
	ADS1112_CONFIG_GAIN_4 = 2,
	ADS1112_CONFIG_GAIN_8 = 3,
};

enum {
	ADS1112_CONFIG_CM_SINGLE = 0,
	ADS1112_CONFIG_CM_CONTINUOUS = 1,
};

struct ads1112_config {
	const struct i2c_dt_spec bus;
};

struct ads1112_data {
	struct adc_context ctx;
	k_timeout_t ready_time;
	struct k_sem acq_sem;
	int16_t *buffer;
	int16_t *buffer_ptr;
	bool differential;
};

static int ads1112_read_reg(const struct device *dev, enum ads1112_reg reg_addr, uint8_t *reg_val)
{
	const struct ads1112_config *config = dev->config;
	uint8_t buf[3] = {0};
	int rc = i2c_read_dt(&config->bus, buf, sizeof(buf));

	if (reg_addr == ADS1112_REG_OUTPUT) {
		reg_val[0] = buf[0];
		reg_val[1] = buf[1];
	} else {
		reg_val[0] = buf[2];
	}

	return rc;
}

static int ads1112_write_reg(const struct device *dev, uint8_t reg)
{
	uint8_t msg[1] = {reg};
	const struct ads1112_config *config = dev->config;

	/* It's only possible to write the config register, so the ADS1112
	 * assumes all writes are going to that register and omits the register
	 * parameter from write transactions
	 */
	return i2c_write_dt(&config->bus, msg, sizeof(msg));
}

static inline int ads1112_acq_time_to_dr(const struct device *dev, uint16_t acq_time)
{
	struct ads1112_data *data = dev->data;
	int odr = -EINVAL;
	uint16_t acq_value = ADC_ACQ_TIME_VALUE(acq_time);
	uint32_t ready_time_us = 0;

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		acq_value = ADS1112_CONFIG_DR_DEFAULT;
	} else if (ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
		return -EINVAL;
	}

	switch (acq_value) {
	case ADS1112_CONFIG_DR_RATE_15_RES_16:
		odr = ADS1112_CONFIG_DR_RATE_15_RES_16;
		ready_time_us = (1000 * 1000) / 15;
		break;
	case ADS1112_CONFIG_DR_RATE_30_RES_15:
		odr = ADS1112_CONFIG_DR_RATE_30_RES_15;
		ready_time_us = (1000 * 1000) / 30;
		break;
	case ADS1112_CONFIG_DR_RATE_60_RES_14:
		odr = ADS1112_CONFIG_DR_RATE_60_RES_14;
		ready_time_us = (1000 * 1000) / 60;
		break;
	case ADS1112_CONFIG_DR_RATE_240_RES_12:
		odr = ADS1112_CONFIG_DR_RATE_240_RES_12;
		ready_time_us = (1000 * 1000) / 240;
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

static int ads1112_wait_data_ready(const struct device *dev)
{
	int rc;
	struct ads1112_data *data = dev->data;

	k_sleep(data->ready_time);
	uint8_t status = 0;

	rc = ads1112_read_reg(dev, ADS1112_REG_CONFIG, &status);
	if (rc != 0) {
		return rc;
	}

	while ((status & ADS1112_CONFIG_MASK_READY) == 0) {

		k_sleep(K_USEC(100));
		rc = ads1112_read_reg(dev, ADS1112_REG_CONFIG, &status);
		if (rc != 0) {
			return rc;
		}
	}

	return 0;
}

static int ads1112_read_sample(const struct device *dev, uint16_t *buff)
{
	int res;
	uint8_t sample[2] = {0};

	res = ads1112_read_reg(dev, ADS1112_REG_OUTPUT, sample);
	buff[0] = sys_get_be16(sample);
	return res;
}

static int ads1112_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	struct ads1112_data *data = dev->data;
	uint8_t config = 0;
	int dr = 0;

	if (channel_cfg->channel_id != 0) {
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		if (channel_cfg->input_positive == 0 && channel_cfg->input_negative == 1) {
			config |= ADS1112_CONFIG_MUX(ADS1112_CONFIG_MUX_DIFF_0_1);
		} else if (channel_cfg->input_positive == 2 && channel_cfg->input_negative == 3) {
			config |= ADS1112_CONFIG_MUX(ADS1112_CONFIG_MUX_BOTH_2_3);
		} else {
			return -EINVAL;
		}
	} else {
		if (channel_cfg->input_positive == 0) {
			config |= ADS1112_CONFIG_MUX(ADS1112_CONFIG_MUX_SINGLE_0_3);
		} else if (channel_cfg->input_positive == 1) {
			config |= ADS1112_CONFIG_MUX(ADS1112_CONFIG_MUX_SINGLE_1_3);
		} else if (channel_cfg->input_positive == 2) {
			config |= ADS1112_CONFIG_MUX(ADS1112_CONFIG_MUX_BOTH_2_3);
		} else {
			return -EINVAL;
		}
	}

	data->differential = channel_cfg->differential;

	dr = ads1112_acq_time_to_dr(dev, channel_cfg->acquisition_time);
	if (dr < 0) {
		return dr;
	}

	config |= ADS1112_CONFIG_DR(dr);

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		config |= ADS1112_CONFIG_GAIN(ADS1112_CONFIG_GAIN_1);
		break;
	case ADC_GAIN_2:
		config |= ADS1112_CONFIG_GAIN(ADS1112_CONFIG_GAIN_2);
		break;
	case ADC_GAIN_3:
		config |= ADS1112_CONFIG_GAIN(ADS1112_CONFIG_GAIN_4);
		break;
	case ADC_GAIN_4:
		config |= ADS1112_CONFIG_GAIN(ADS1112_CONFIG_GAIN_8);
		break;
	default:
		return -EINVAL;
	}

	config |= ADS1112_CONFIG_CM(ADS1112_CONFIG_CM_SINGLE); /* Only single shot supported */

	return ads1112_write_reg(dev, config);
}

static int ads1112_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		LOG_ERR("Insufficient buffer %i < %i", sequence->buffer_size, needed);
		return -ENOMEM;
	}

	return 0;
}

static int ads1112_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	if (sequence->channels != BIT(0)) {
		LOG_ERR("Invalid Channel 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling not supported");
		return -EINVAL;
	}

	return ads1112_validate_buffer_size(sequence);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads1112_data *data = CONTAINER_OF(ctx, struct ads1112_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads1112_data *data = CONTAINER_OF(ctx, struct ads1112_data, ctx);

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acq_sem);
}

static int ads1112_adc_start_read(const struct device *dev, const struct adc_sequence *sequence,
				  bool wait)
{
	int rc = 0;
	struct ads1112_data *data = dev->data;

	rc = ads1112_validate_sequence(dev, sequence);
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

static int ads1112_adc_perform_read(const struct device *dev)
{
	int rc;
	struct ads1112_data *data = dev->data;

	k_sem_take(&data->acq_sem, K_FOREVER);

	rc = ads1112_wait_data_ready(dev);
	if (rc != 0) {
		adc_context_complete(&data->ctx, rc);
		return rc;
	}

	rc = ads1112_read_sample(dev, data->buffer);
	if (rc != 0) {
		adc_context_complete(&data->ctx, rc);
		return rc;
	}
	data->buffer++;

	adc_context_on_sampling_done(&data->ctx, dev);

	return rc;
}

static int ads1112_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int rc;
	struct ads1112_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	rc = ads1112_adc_start_read(dev, sequence, false);

	while (rc == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		rc = ads1112_adc_perform_read(dev);
	}

	adc_context_release(&data->ctx, rc);
	return rc;
}

static int ads1112_init(const struct device *dev)
{
	int rc = 0;
	const struct ads1112_config *config = dev->config;
	struct ads1112_data *data = dev->data;

	adc_context_init(&data->ctx);

	k_sem_init(&data->acq_sem, 0, 1);

	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

	rc = ads1112_write_reg(dev, ADS1112_DEFAULT_CONFIG);
	if (rc) {
		LOG_ERR("Could not set default config 0x%x", ADS1112_DEFAULT_CONFIG);
		return rc;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return rc;
}

static const struct adc_driver_api api = {
	.channel_setup = ads1112_channel_setup,
	.read = ads1112_read,
	.ref_internal = ADS1112_REF_INTERNAL,
};
#define ADC_ADS1112_INST_DEFINE(n)                                                                 \
	static const struct ads1112_config config_##n = {.bus = I2C_DT_SPEC_INST_GET(n)};  \
	static struct ads1112_data data_##n;                                                       \
	DEVICE_DT_INST_DEFINE(n, ads1112_init, NULL, &data_##n, &config_##n, POST_KERNEL,          \
			      CONFIG_ADC_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS1112_INST_DEFINE);
