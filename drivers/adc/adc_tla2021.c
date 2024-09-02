/*
 * Copyright (c) 2023 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER

/*
 * This requires to be included _after_  `#define ADC_CONTEXT_USES_KERNEL_TIMER`
 */
#include "adc_context.h"

#define DT_DRV_COMPAT ti_tla2021

LOG_MODULE_REGISTER(tla2021, CONFIG_ADC_LOG_LEVEL);

#define ACQ_THREAD_PRIORITY   CONFIG_ADC_TLA2021_ACQUISITION_THREAD_PRIORITY
#define ACQ_THREAD_STACK_SIZE CONFIG_ADC_TLA2021_ACQUISITION_THREAD_STACK_SIZE

#define ADC_CHANNEL_msk BIT(0)
#define ADC_RESOLUTION  12

/*
 * Conversion Data Register (RP = 00h) [reset = 0000h]
 */
#define REG_DATA     0x00
#define REG_DATA_pos 4

/*
 * Configuration Register (RP = 01h) [reset = 8583h]
 */
#define REG_CONFIG          0x01
#define REG_CONFIG_DEFAULT  0x8583
#define REG_CONFIG_DR_pos   5
#define REG_CONFIG_MODE_pos 8
#define REG_CONFIG_PGA_pos  9  /* TLA2022 and TLA2024 Only */
#define REG_CONFIG_MUX_pos  12 /* TLA2024 Only */
#define REG_CONFIG_OS_pos   15
#define REG_CONFIG_OS_msk   (BIT_MASK(1) << REG_CONFIG_OS_pos)

typedef int16_t tla2021_reg_data_t;
typedef uint16_t tla2021_reg_config_t;

struct tla2021_config {
	const struct i2c_dt_spec bus;
};

struct tla2021_data {
	const struct device *dev;
	struct adc_context ctx;
#ifdef CONFIG_ADC_ASYNC
	struct k_sem acq_lock;
#endif
	tla2021_reg_data_t *buffer;
	tla2021_reg_data_t *repeat_buffer;

	/*
	 * Shadow register
	 */
	tla2021_reg_config_t reg_config;
};

static int tla2021_read_register(const struct device *dev, uint8_t reg, uint16_t *value)
{
	int ret;

	const struct tla2021_config *config = dev->config;
	uint8_t tmp[2];

	ret = i2c_write_read_dt(&config->bus, &reg, sizeof(reg), tmp, sizeof(tmp));
	if (ret) {
		return ret;
	}

	*value = sys_get_be16(tmp);

	return 0;
}

static int tla2021_write_register(const struct device *dev, uint8_t reg, uint16_t value)
{
	int ret;

	const struct tla2021_config *config = dev->config;
	uint8_t tmp[3] = {reg};

	sys_put_be16(value, &tmp[1]);

	ret = i2c_write_dt(&config->bus, tmp, sizeof(tmp));
	if (ret) {
		return ret;
	}

	return 0;
}

static int tla2021_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	if (cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid gain");
		return -EINVAL;
	}

	if (cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid reference");
		return -EINVAL;
	}

	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid acquisition time");
		return -EINVAL;
	}

	return 0;
}

static int tla2021_start_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct tla2021_data *data = dev->data;

	const size_t num_extra_samples = seq->options ? seq->options->extra_samplings : 0;
	const size_t num_samples = (1 + num_extra_samples) * POPCOUNT(seq->channels);

	if (!(seq->channels & ADC_CHANNEL_msk)) {
		LOG_ERR("Selected channel(s) not supported: %x", seq->channels);
		return -EINVAL;
	}

	if (seq->resolution != ADC_RESOLUTION) {
		LOG_ERR("Selected resolution not supported: %d", seq->resolution);
		return -EINVAL;
	}

	if (seq->oversampling) {
		LOG_ERR("Oversampling is not supported");
		return -EINVAL;
	}

	if (seq->calibrate) {
		LOG_ERR("Calibration is not supported");
		return -EINVAL;
	}

	if (!seq->buffer) {
		LOG_ERR("Buffer invalid");
		return -EINVAL;
	}

	if (seq->buffer_size < (num_samples * sizeof(tla2021_reg_data_t))) {
		LOG_ERR("buffer size too small");
		return -EINVAL;
	}

	data->buffer = seq->buffer;

	adc_context_start_read(&data->ctx, seq);

	return adc_context_wait_for_completion(&data->ctx);
}

static int tla2021_read_async(const struct device *dev, const struct adc_sequence *seq,
			      struct k_poll_signal *async)
{
	int ret;

	struct tla2021_data *data = dev->data;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = tla2021_start_read(dev, seq);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int tla2021_read(const struct device *dev, const struct adc_sequence *seq)
{
	return tla2021_read_async(dev, seq, NULL);
}

static void tla2021_perform_read(const struct device *dev)
{
	struct tla2021_data *data = dev->data;
	tla2021_reg_config_t reg;
	tla2021_reg_data_t res;
	int ret;

	/*
	 * Wait until sampling is done
	 */
	do {
		ret = tla2021_read_register(dev, REG_CONFIG, &reg);
		if (ret < 0) {
			adc_context_complete(&data->ctx, ret);
		}
	} while (!(reg & REG_CONFIG_OS_msk));

	/*
	 * Read result
	 */
	ret = tla2021_read_register(dev, REG_DATA, &res);
	if (ret) {
		adc_context_complete(&data->ctx, ret);
	}

	/*
	 * ADC data is stored in the upper 12 bits
	 */
	res >>= REG_DATA_pos;
	*data->buffer++ = res;

	adc_context_on_sampling_done(&data->ctx, data->dev);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	int ret;

	struct tla2021_data *data = CONTAINER_OF(ctx, struct tla2021_data, ctx);
	const struct device *dev = data->dev;

	tla2021_reg_config_t reg = data->reg_config;

	/*
	 * Start single-shot conversion
	 */
	WRITE_BIT(reg, REG_CONFIG_MODE_pos, 1);
	WRITE_BIT(reg, REG_CONFIG_OS_pos, 1);
	ret = tla2021_write_register(dev, REG_CONFIG, reg);
	if (ret) {
		LOG_WRN("Failed to start conversion");
	}

	data->repeat_buffer = data->buffer;

#ifdef CONFIG_ADC_ASYNC
	k_sem_give(&data->acq_lock);
#else
	tla2021_perform_read(dev);
#endif
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct tla2021_data *data = CONTAINER_OF(ctx, struct tla2021_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

#ifdef CONFIG_ADC_ASYNC
static void tla2021_acq_thread_fn(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct tla2021_data *data = dev->data;

	while (true) {
		k_sem_take(&data->acq_lock, K_FOREVER);

		tla2021_perform_read(dev);
	}
}
#endif

static int tla2021_init(const struct device *dev)
{
	int ret;

	const struct tla2021_config *config = dev->config;
	struct tla2021_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("Bus not ready");
		return -EINVAL;
	}

	ret = tla2021_write_register(dev, REG_CONFIG, data->reg_config);
	if (ret) {
		LOG_ERR("Device reset failed: %d", ret);
		return ret;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api tla2021_driver_api = {
	.channel_setup = tla2021_channel_setup,
	.read = tla2021_read,
	.ref_internal = 4096,
#ifdef CONFIG_ADC_ASYNC
	.read_async = tla2021_read_async,
#endif
};

#define TLA2021_THREAD_INIT(n)                                                                     \
	K_THREAD_DEFINE(adc_tla2021_##n##_thread, ACQ_THREAD_STACK_SIZE, tla2021_acq_thread_fn,    \
			DEVICE_DT_INST_GET(n), NULL, NULL, ACQ_THREAD_PRIORITY, 0, 0);

#define TLA2021_INIT(n)                                                                            \
	static const struct tla2021_config inst_##n##_config;                                      \
	static struct tla2021_data inst_##n##_data;                                                \
	IF_ENABLED(CONFIG_ADC_ASYNC, (TLA2021_THREAD_INIT(n)))                                     \
	static const struct tla2021_config inst_##n##_config = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
	static struct tla2021_data inst_##n##_data = {                                             \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
		ADC_CONTEXT_INIT_LOCK(inst_##n##_data, ctx),                                       \
		ADC_CONTEXT_INIT_TIMER(inst_##n##_data, ctx),                                      \
		ADC_CONTEXT_INIT_SYNC(inst_##n##_data, ctx),                                       \
		.reg_config = REG_CONFIG_DEFAULT,                                                  \
		IF_ENABLED(CONFIG_ADC_ASYNC,                                                       \
			   (.acq_lock = Z_SEM_INITIALIZER(inst_##n##_data.acq_lock, 0, 1),))       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &tla2021_init, NULL, &inst_##n##_data, &inst_##n##_config,        \
			      POST_KERNEL, CONFIG_ADC_TLA2021_INIT_PRIORITY, &tla2021_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLA2021_INIT)

BUILD_ASSERT(CONFIG_I2C_INIT_PRIORITY < CONFIG_ADC_TLA2021_INIT_PRIORITY);
