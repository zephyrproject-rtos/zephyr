/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for the Texas Instruments ADCxx1C I2C ADC family.
 *
 * Covers ADC081C021, ADC081C027, ADC101C021, ADC101C027, ADC121C021 and
 * ADC121C027, which share one register map and differ only in resolution
 * and in the function of one package pin.
 *
 * Datasheets:
 *	https://www.ti.com/lit/ds/symlink/adc081c021.pdf
 *	https://www.ti.com/lit/ds/symlink/adc101c021.pdf
 *	https://www.ti.com/lit/ds/symlink/adc121c021.pdf
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

LOG_MODULE_REGISTER(adcxx1c, CONFIG_ADC_LOG_LEVEL);

/*
 * Conversion Result register. D15 carries the alert flag when that flag is
 * enabled, D14:D12 are reserved and the sample occupies D11:D0.
 */
#define ADCXX1C_REG_CONV_RES 0x00U
#define ADCXX1C_CONV_MASK    GENMASK(11, 0)

struct adcxx1c_config {
	struct i2c_dt_spec i2c;
	uint8_t resolution;
	uint8_t data_shift;
};

struct adcxx1c_data {
	struct adc_context ctx;
	struct k_sem acq_sem;
	uint16_t *buffer;
	uint16_t *buffer_ptr;
};

static int adcxx1c_read_sample(const struct device *dev, uint16_t *sample)
{
	const struct adcxx1c_config *config = dev->config;
	uint8_t buf[sizeof(uint16_t)];
	int err;

	err = i2c_burst_read_dt(&config->i2c, ADCXX1C_REG_CONV_RES, buf, sizeof(buf));
	if (err != 0) {
		LOG_ERR("Conversion result read failed (err %d)", err);
		return err;
	}

	*sample = (sys_get_be16(buf) & ADCXX1C_CONV_MASK) >> config->data_shift;

	return 0;
}

static int adcxx1c_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("Invalid channel id %u", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain %d", channel_cfg->gain);
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_VDD_1) {
		LOG_ERR("Invalid channel reference %d", channel_cfg->reference);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid acquisition time 0x%x", channel_cfg->acquisition_time);
		return -EINVAL;
	}

	return 0;
}

static int adcxx1c_validate_sequence(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adcxx1c_config *config = dev->config;
	size_t needed = sizeof(uint16_t);

	if (sequence->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %u", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->channels != BIT(0)) {
		LOG_ERR("Unsupported channels 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->oversampling != 0) {
		LOG_ERR("Oversampling is not supported");
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		LOG_ERR("Calibration is not supported");
		return -ENOTSUP;
	}

	if (sequence->options != NULL) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		LOG_ERR("Insufficient buffer size %zu, %zu needed", sequence->buffer_size, needed);
		return -ENOMEM;
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adcxx1c_data *data = CONTAINER_OF(ctx, struct adcxx1c_data, ctx);

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acq_sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adcxx1c_data *data = CONTAINER_OF(ctx, struct adcxx1c_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static int adcxx1c_perform_read(const struct device *dev)
{
	struct adcxx1c_data *data = dev->data;
	int err;

	k_sem_take(&data->acq_sem, K_FOREVER);

	err = adcxx1c_read_sample(dev, data->buffer);
	if (err != 0) {
		return err;
	}

	data->buffer++;
	adc_context_on_sampling_done(&data->ctx, dev);

	return 0;
}

static int adcxx1c_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adcxx1c_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, false, NULL);

	err = adcxx1c_validate_sequence(dev, sequence);
	if (err == 0) {
		data->buffer = sequence->buffer;
		adc_context_start_read(&data->ctx, sequence);

		/*
		 * Reading the conversion result register is what triggers a
		 * conversion, so samples are fetched in the caller's context.
		 * adc_context_complete() posts ctx.sync on both the normal and
		 * the error path, which is what ends this loop.
		 */
		while (k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
			err = adcxx1c_perform_read(dev);
			if (err != 0) {
				adc_context_complete(&data->ctx, err);
			}
		}

		err = data->ctx.status;
	}

	adc_context_release(&data->ctx, err);

	return err;
}

static DEVICE_API(adc, adcxx1c_api) = {
	.channel_setup = adcxx1c_channel_setup,
	.read = adcxx1c_read,
};

static int adcxx1c_init(const struct device *dev)
{
	const struct adcxx1c_config *config = dev->config;
	struct adcxx1c_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus %s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	k_sem_init(&data->acq_sem, 0, 1);
	adc_context_init(&data->ctx);

	/*
	 * The reset value of the configuration register already selects the
	 * alert-less operation this driver relies on, and the supply of the
	 * device may still be gated at this point, so no transfer is made.
	 */
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define ADCXX1C_INIT(t, inst, res, shift)                                                          \
	static struct adcxx1c_data adcxx1c_data_##t##_##inst;                                      \
                                                                                                   \
	static const struct adcxx1c_config adcxx1c_config_##t##_##inst = {                         \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.resolution = (res),                                                               \
		.data_shift = (shift),                                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, adcxx1c_init, NULL, &adcxx1c_data_##t##_##inst,                \
			      &adcxx1c_config_##t##_##inst, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, \
			      &adcxx1c_api);

#define ADC081C021_INIT(n) ADCXX1C_INIT(adc081c021, n, 8, 4)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_adc081c021
DT_INST_FOREACH_STATUS_OKAY(ADC081C021_INIT)

#define ADC081C027_INIT(n) ADCXX1C_INIT(adc081c027, n, 8, 4)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_adc081c027
DT_INST_FOREACH_STATUS_OKAY(ADC081C027_INIT)

#define ADC101C021_INIT(n) ADCXX1C_INIT(adc101c021, n, 10, 2)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_adc101c021
DT_INST_FOREACH_STATUS_OKAY(ADC101C021_INIT)

#define ADC101C027_INIT(n) ADCXX1C_INIT(adc101c027, n, 10, 2)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_adc101c027
DT_INST_FOREACH_STATUS_OKAY(ADC101C027_INIT)

#define ADC121C021_INIT(n) ADCXX1C_INIT(adc121c021, n, 12, 0)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_adc121c021
DT_INST_FOREACH_STATUS_OKAY(ADC121C021_INIT)

#define ADC121C027_INIT(n) ADCXX1C_INIT(adc121c027, n, 12, 0)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_adc121c027
DT_INST_FOREACH_STATUS_OKAY(ADC121C027_INIT)
