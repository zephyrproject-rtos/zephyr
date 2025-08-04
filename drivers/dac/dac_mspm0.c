/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_dac

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>

/* TI Driverlib includes */
#include <ti/driverlib/dl_dac12.h>

/* DAC valid resolutions */
#define DAC_RESOLUTION_8BIT	8
#define DAC_RESOLUTION_12BIT	12

/* 8-bit Binary Repr Range */
#define DAC8_BINARY_REPR_MIN	0
#define DAC8_BINARY_REPR_MAX	255

/* 12-bit Binary Repr Range */
#define DAC12_BINARY_REPR_MIN	0
#define DAC12_BINARY_REPR_MAX	4095

/* DAC Primary Channel ID */
#define DAC_PRIMARY_CHANNEL_ID	0

struct dac_mspm0_config {
	DAC12_Regs *dac_base;
	DL_DAC12_VREF_SOURCE dac_vref_src;
};

struct dac_mspm0_data {
	struct k_mutex lock;
	uint8_t resolution;
};

static int dac_mspm0_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_mspm0_config *config = dev->config;
	struct dac_mspm0_data *data = dev->data;

	if (channel_cfg->channel_id != DAC_PRIMARY_CHANNEL_ID) {
		return -EINVAL;
	}

	if (channel_cfg->resolution != DAC_RESOLUTION_8BIT &&
			channel_cfg->resolution != DAC_RESOLUTION_12BIT) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* DAC must be disabled before configuration */
	DL_DAC12_disable(config->dac_base);

	DL_DAC12_configDataFormat(config->dac_base, DL_DAC12_REPRESENTATION_BINARY,
				  (channel_cfg->resolution == DAC_RESOLUTION_12BIT) ?
				  DL_DAC12_RESOLUTION_12BIT : DL_DAC12_RESOLUTION_8BIT);

	/* buffered must be true to enable amplifier for output drive */
	DL_DAC12_setAmplifier(config->dac_base,
			      (channel_cfg->buffered) ? DL_DAC12_AMP_ON : DL_DAC12_AMP_OFF_0V);

	DL_DAC12_setReferenceVoltageSource(config->dac_base, config->dac_vref_src);

	/* internal must be true to route output to OPA, ADC, COMP and DAC_OUT pin */
	if (channel_cfg->internal) {
		DL_DAC12_enableOutputPin(config->dac_base);
	} else {
		DL_DAC12_disableOutputPin(config->dac_base);
	}

	DL_DAC12_enable(config->dac_base);

	data->resolution = channel_cfg->resolution;
	DL_DAC12_performSelfCalibrationBlocking(config->dac_base);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int dac_mspm0_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct dac_mspm0_config *config = dev->config;
	struct dac_mspm0_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Validate channel and resolution */
	if (channel != DAC_PRIMARY_CHANNEL_ID || data->resolution == 0) {
		ret = -EINVAL;
		goto unlock;
	}

	if (data->resolution == DAC_RESOLUTION_12BIT) {
		if (value > DAC12_BINARY_REPR_MAX || value < DAC12_BINARY_REPR_MIN) {
			ret = -EINVAL;
			goto unlock;
		}
		DL_DAC12_output12(config->dac_base, value);

	} else {
		if (value > DAC8_BINARY_REPR_MAX || value < DAC8_BINARY_REPR_MIN) {
			ret = -EINVAL;
			goto unlock;
		}
		DL_DAC12_output8(config->dac_base, (uint8_t)value);
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int dac_mspm0_init(const struct device *dev)
{
	const struct dac_mspm0_config *config = dev->config;

	DL_DAC12_enablePower(config->dac_base);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	return 0;
}

static DEVICE_API(dac, dac_mspm0_driver_api) = {
	.channel_setup = dac_mspm0_channel_setup,
	.write_value   = dac_mspm0_write_value
};

#define DAC_MSPM0_DEFINE(id)									\
												\
	static const struct dac_mspm0_config dac_mspm0_config_##id = {				\
		.dac_base = (DAC12_Regs *)DT_INST_REG_ADDR(id),					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(id, vref),                                    \
			    (.dac_vref_src = DL_DAC12_VREF_SOURCE_VEREFP_VEREFN),		\
			    (.dac_vref_src = DL_DAC12_VREF_SOURCE_VDDA_VSSA)),			\
	};											\
												\
	static struct dac_mspm0_data dac_mspm0_data_##id = {					\
		/* Configure resolution at channel setup */					\
		.lock = Z_MUTEX_INITIALIZER(dac_mspm0_data_##id.lock),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(id, &dac_mspm0_init, NULL, &dac_mspm0_data_##id,			\
			      &dac_mspm0_config_##id, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,	\
			      &dac_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_MSPM0_DEFINE)
