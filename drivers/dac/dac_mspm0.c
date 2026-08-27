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

#define DAC_RESOLUTION_8BIT	8
#define DAC_RESOLUTION_12BIT	12

#define DAC8_MAX_VALUE		255
#define DAC12_MAX_VALUE		4095

#define DAC_PRIMARY_CHANNEL_ID	0
#define DAC_READY_TIMEOUT_US	1000

struct dac_mspm0_config {
	DEVICE_MMIO_ROM;
	DL_DAC12_VREF_SOURCE dac_vref_src;
};

struct dac_mspm0_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
	uint8_t resolution;
};

static inline DAC12_Regs *dac_mspm0_regs(const struct device *dev)
{
	return (DAC12_Regs *)DEVICE_MMIO_GET(dev);
}

static int dac_mspm0_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_mspm0_config *config = dev->config;
	struct dac_mspm0_data *data = dev->data;
	DAC12_Regs *regs = dac_mspm0_regs(dev);

	if (channel_cfg->channel_id != DAC_PRIMARY_CHANNEL_ID) {
		return -EINVAL;
	}

	if (channel_cfg->resolution != DAC_RESOLUTION_8BIT &&
			channel_cfg->resolution != DAC_RESOLUTION_12BIT) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* DAC must be disabled before configuration */
	DL_DAC12_disable(regs);

	DL_DAC12_configDataFormat(regs, DL_DAC12_REPRESENTATION_BINARY,
				  (channel_cfg->resolution == DAC_RESOLUTION_12BIT) ?
				  DL_DAC12_RESOLUTION_12BIT : DL_DAC12_RESOLUTION_8BIT);

	/* buffered must be true to enable amplifier for output drive */
	DL_DAC12_setAmplifier(regs,
			      (channel_cfg->buffered) ? DL_DAC12_AMP_ON : DL_DAC12_AMP_OFF_0V);

	DL_DAC12_setReferenceVoltageSource(regs, config->dac_vref_src);

	/*
	 * CTL1.OPS controls output to both internal modules (OPA, ADC, COMP)
	 * and the external DAC_OUT pin. HW does not allow separate control.
	 */
	if (channel_cfg->internal) {
		DL_DAC12_enableOutputPin(regs);
	} else {
		DL_DAC12_disableOutputPin(regs);
	}

	DL_DAC12_enable(regs);

	/* Wait for DAC core and output buffer to settle */
	if (!WAIT_FOR(DL_DAC12_getInterruptStatus(regs,
						  DL_DAC12_INTERRUPT_MODULE_READY),
		      DAC_READY_TIMEOUT_US, k_busy_wait(1))) {
		k_mutex_unlock(&data->lock);
		return -ETIMEDOUT;
	}

	data->resolution = channel_cfg->resolution;

	if (channel_cfg->buffered) {
		DL_DAC12_performSelfCalibrationBlocking(regs);
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int dac_mspm0_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	struct dac_mspm0_data *data = dev->data;
	DAC12_Regs *regs = dac_mspm0_regs(dev);
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Validate channel and resolution */
	if (channel != DAC_PRIMARY_CHANNEL_ID || data->resolution == 0) {
		ret = -EINVAL;
		goto unlock;
	}

	if (data->resolution == DAC_RESOLUTION_12BIT) {
		if (value > DAC12_MAX_VALUE) {
			ret = -EINVAL;
			goto unlock;
		}
		DL_DAC12_output12(regs, value);

	} else {
		if (value > DAC8_MAX_VALUE) {
			ret = -EINVAL;
			goto unlock;
		}
		DL_DAC12_output8(regs, (uint8_t)value);
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int dac_mspm0_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	DL_DAC12_enablePower(dac_mspm0_regs(dev));
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
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(id)),						\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(id, vref),                                    \
			    (.dac_vref_src = DL_DAC12_VREF_SOURCE_VEREFP_VEREFN),		\
			    (.dac_vref_src = DL_DAC12_VREF_SOURCE_VDDA_VSSA)),			\
	};											\
												\
	static struct dac_mspm0_data dac_mspm0_data_##id = {					\
		.lock = Z_MUTEX_INITIALIZER(dac_mspm0_data_##id.lock),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(id, &dac_mspm0_init, NULL, &dac_mspm0_data_##id,			\
			      &dac_mspm0_config_##id, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,	\
			      &dac_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_MSPM0_DEFINE)
