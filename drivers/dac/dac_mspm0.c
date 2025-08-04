/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_dac

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

/* TI Driverlib includes */
#include <ti/devices/msp/peripherals/hw_dac12.h>
#include <ti/driverlib/dl_dac12.h>

/* DAC Primary Channel ID */
#define DAC_CHANNEL_ID_PRIMARY		0

/* DAC valid resolutions */
#define DAC_RESOLUTION_8BIT		8
#define DAC_RESOLUTION_12BIT		12

/* 8-bit Binary Repr Range */
#define DAC8_BINARY_REPR_MIN 		0
#define DAC8_BINARY_REPR_MAX 		255

/* 12-bit Binary Repr Range */
#define DAC12_BINARY_REPR_MIN 		0
#define DAC12_BINARY_REPR_MAX 		4095

/* Timeout to wait until the DAC module is ready after enabling */
#define DAC_MOD_RDY_TIMEOUT K_MSEC(CONFIG_DAC_MSPM0_TIMEOUT_MS)

struct dac_mspm0_config {
	DAC12_Regs *dac_base;
	void (*irq_config_func)(const struct device *dev);
};

struct dac_mspm0_data {
	uint8_t resolution;
	struct k_sem mod_rdy;
};

static void dac_mspm0_isr(const struct device *dev)
{
	const struct dac_mspm0_config *config = dev->config;
	struct dac_mspm0_data *data = dev->data;

	if (DL_DAC12_getPendingInterrupt(config->dac_base) == DL_DAC12_IIDX_MODULE_READY) {
		k_sem_give(&data->mod_rdy);
	}
}

static int dac_mspm0_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_mspm0_config *config = dev->config;
	struct dac_mspm0_data *data = dev->data;

	if (channel_cfg->channel_id != DAC_CHANNEL_ID_PRIMARY) {
		return -EINVAL;
	}

	if (channel_cfg->resolution != DAC_RESOLUTION_8BIT &&
			channel_cfg->resolution != DAC_RESOLUTION_12BIT) {
		return -ENOTSUP;
	}

	k_sem_reset(&data->mod_rdy);

	/* Ensure DAC is disabled before configuration */
	DL_DAC12_disable(config->dac_base);

	DL_DAC12_configDataFormat(config->dac_base, DL_DAC12_REPRESENTATION_BINARY,
				  (channel_cfg->resolution == DAC_RESOLUTION_12BIT) ?
				  DL_DAC12_RESOLUTION_12BIT : DL_DAC12_RESOLUTION_8BIT);

	/* buffered must be true to enable amplifier for output drive */
	DL_DAC12_setAmplifier(config->dac_base,
			      (channel_cfg->buffered) ? DL_DAC12_AMP_ON : DL_DAC12_AMP_OFF_0V);

	if (IS_ENABLED(CONFIG_DAC_MSPM0_VREF_SOURCE)) {
		DL_DAC12_setReferenceVoltageSource(config->dac_base,
						   DL_DAC12_VREF_SOURCE_VEREFP_VEREFN);
	} else {
		DL_DAC12_setReferenceVoltageSource(config->dac_base,
						   DL_DAC12_VREF_SOURCE_VDDA_VSSA);
	}

	/* internal must be true to route output to OPA, ADC, COMP and DAC_OUT pin */
	if (channel_cfg->internal) {
		DL_DAC12_enableOutputPin(config->dac_base);
	} else {
		DL_DAC12_disableOutputPin(config->dac_base);
	}

	DL_DAC12_enable(config->dac_base);

	if (k_sem_take(&data->mod_rdy, DAC_MOD_RDY_TIMEOUT) != 0) {
		return -ETIMEDOUT;
	}

	data->resolution = channel_cfg->resolution;
	DL_DAC12_performSelfCalibrationBlocking(config->dac_base);

	return 0;
}

static int dac_mspm0_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct dac_mspm0_config *config = dev->config;
	struct dac_mspm0_data *data = dev->data;

	/* Validate channel and resolution */
	if (channel != DAC_CHANNEL_ID_PRIMARY || data->resolution == 0) {
		return -EINVAL;
	}

	if (data->resolution == DAC_RESOLUTION_12BIT) {
		if (value > DAC12_BINARY_REPR_MAX || value < DAC12_BINARY_REPR_MIN) {
			return -EINVAL;
		}
		DL_DAC12_output12(config->dac_base, value);

	} else {
		if (value > DAC8_BINARY_REPR_MAX || value < DAC8_BINARY_REPR_MIN) {
			return -EINVAL;
		}
		DL_DAC12_output8(config->dac_base, (uint8_t)value);
	}

	return 0;
}

static int dac_mspm0_init(const struct device *dev)
{
	const struct dac_mspm0_config *config = dev->config;

	DL_DAC12_enablePower(config->dac_base);

	config->irq_config_func(dev);
	DL_DAC12_enableInterrupt(config->dac_base, DL_DAC12_INTERRUPT_MODULE_READY);

	return 0;
}

static DEVICE_API(dac, dac_mspm0_driver_api) = {
	.channel_setup = dac_mspm0_channel_setup,
	.write_value   = dac_mspm0_write_value
};

#define DAC_MSPM0_DEFINE(id)									\
												\
	static void dac_mspm0_irq_config_##id(const struct device *dev)				\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), dac_mspm0_isr,		\
				DEVICE_DT_INST_GET(id), 0);					\
		irq_enable(DT_INST_IRQN(id));							\
	};											\
												\
	static const struct dac_mspm0_config dac_mspm0_config_##id = {				\
		.dac_base = (DAC12_Regs *)DT_INST_REG_ADDR(id),					\
		.irq_config_func = dac_mspm0_irq_config_##id,					\
	};											\
												\
	static struct dac_mspm0_data dac_mspm0_data_##id = {					\
		/* Configure resolution at channel setup */					\
		.mod_rdy = Z_SEM_INITIALIZER(dac_mspm0_data_##id.mod_rdy, 0, 1),		\
	};											\
												\
	DEVICE_DT_INST_DEFINE(id, &dac_mspm0_init, NULL, &dac_mspm0_data_##id,			\
			      &dac_mspm0_config_##id, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,	\
			      &dac_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_MSPM0_DEFINE)
