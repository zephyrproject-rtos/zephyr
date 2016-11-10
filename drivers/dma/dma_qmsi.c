/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <stdio.h>
#include <kernel.h>
#include <board.h>
#include <device.h>
#include <init.h>
#include <dma.h>

#include "qm_dma.h"
#include "qm_isr.h"
#include "clk.h"

struct dma_qmsi_config_info {
	qm_dma_t instance; /* Controller instance. */
};

struct dma_qmsi_context {
	uint32_t index;
	struct device *dev;
};

struct dma_qmsi_driver_data {
	void (*transfer[QM_DMA_CHANNEL_NUM])(struct device *dev, void *data);
	void (*error[QM_DMA_CHANNEL_NUM])(struct device *dev, void *data);
	void *callback_data[QM_DMA_CHANNEL_NUM];
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t device_power_state;
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
	qm_dma_context_t saved_ctx;
#endif
#endif
};


static struct dma_qmsi_context dma_context[QM_DMA_CHANNEL_NUM];
static void dma_qmsi_config(struct device *dev);

static void dma_callback(void *callback_context, uint32_t len,
						 int error_code)
{
	struct dma_qmsi_driver_data *data;
	uint32_t channel;
	struct dma_qmsi_context *context = callback_context;

	channel = context->index;
	data = context->dev->driver_data;
	if (error_code != 0) {
		data->error[channel](context->dev,
				     data->callback_data[channel]);
		return;
	}

	data->transfer[channel](context->dev, data->callback_data[channel]);
}

static int dma_qmsi_channel_config(struct device *dev, uint32_t channel,
				   struct dma_channel_config *config)
{
	qm_dma_channel_config_t qmsi_cfg;
	const struct dma_qmsi_config_info *info = dev->config->config_info;
	struct dma_qmsi_driver_data *data = dev->driver_data;

	qmsi_cfg.handshake_interface = (qm_dma_handshake_interface_t)
					config->handshake_interface;
	qmsi_cfg.handshake_polarity = (qm_dma_handshake_polarity_t)
				       config->handshake_polarity;
	qmsi_cfg.source_transfer_width = (qm_dma_transfer_width_t)
					  config->source_transfer_width;
	qmsi_cfg.channel_direction = (qm_dma_channel_direction_t)
				      config->channel_direction;
	qmsi_cfg.destination_burst_length = (qm_dma_burst_length_t)
					     config->destination_burst_length;
	qmsi_cfg.destination_transfer_width = (qm_dma_transfer_width_t)
					    config->destination_transfer_width;
	qmsi_cfg.source_burst_length = (qm_dma_burst_length_t)
					config->source_burst_length;

	/* TODO: add support for using other DMA transfer types. */
	qmsi_cfg.transfer_type = QM_DMA_TYPE_SINGLE;

	data->callback_data[channel] = config->callback_data;
	data->transfer[channel] = config->dma_transfer;
	data->error[channel] = config->dma_error;

	dma_context[channel].index = channel;
	dma_context[channel].dev = dev;

	qmsi_cfg.callback_context = &dma_context[channel];
	qmsi_cfg.client_callback = dma_callback;

	return qm_dma_channel_set_config(info->instance, channel, &qmsi_cfg);
}

static int dma_qmsi_transfer_config(struct device *dev, uint32_t channel,
				    struct dma_transfer_config *config)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;

	return qm_dma_transfer_set_config(info->instance, channel,
					 (qm_dma_transfer_t *)config);
}

static int dma_qmsi_transfer_start(struct device *dev, uint32_t channel)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;

	return qm_dma_transfer_start(info->instance, channel);
}

static int dma_qmsi_transfer_stop(struct device *dev, uint32_t channel)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;

	return qm_dma_transfer_terminate(info->instance, channel);
}

static const struct dma_driver_api dma_funcs = {
	.channel_config = dma_qmsi_channel_config,
	.transfer_config = dma_qmsi_transfer_config,
	.transfer_start = dma_qmsi_transfer_start,
	.transfer_stop = dma_qmsi_transfer_stop
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void dma_qmsi_set_power_state(struct device *dev, uint32_t power_state)
{
	struct dma_qmsi_driver_data *ctx = dev->driver_data;

	ctx->device_power_state = power_state;
}

static uint32_t dma_qmsi_get_power_state(struct device *dev)
{
	struct dma_qmsi_driver_data *ctx = dev->driver_data;

	return ctx->device_power_state;
}
#else
#define dma_qmsi_set_power_state(...)
#endif

int dma_qmsi_init(struct device *dev)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;

	dma_qmsi_config(dev);
	qm_dma_init(info->instance);
	dma_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);
	return 0;
}

static const struct dma_qmsi_config_info dma_qmsi_config_data = {
	.instance = QM_DMA_0,
};

static struct dma_qmsi_driver_data dma_qmsi_dev_data;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
static int dma_suspend_device(struct device *dev)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;
	struct dma_qmsi_driver_data *ctx = dev->driver_data;

	qm_dma_save_context(info->instance, &ctx->saved_ctx);
	dma_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int dma_resume_device(struct device *dev)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;
	struct dma_qmsi_driver_data *ctx = dev->driver_data;

	qm_dma_restore_context(info->instance, &ctx->saved_ctx);
	dma_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}
#endif

static int dma_qmsi_device_ctrl(struct device *dev, uint32_t ctrl_command,
				void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return dma_suspend_device(dev);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return dma_resume_device(dev);
		}
#endif
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = dma_qmsi_get_power_state(dev);
	}

	return 0;
}
#endif

DEVICE_DEFINE(dma_qmsi, CONFIG_DMA_0_NAME, &dma_qmsi_init, dma_qmsi_device_ctrl,
	      &dma_qmsi_dev_data, &dma_qmsi_config_data, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, (void *)&dma_funcs);

static void dma_qmsi_config(struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_0), CONFIG_DMA_0_IRQ_PRI,
			qm_dma_0_isr_0, DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_0));
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->dma_0_int_0_mask);

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_1), CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_1, DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_1));
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->dma_0_int_1_mask);

#if (CONFIG_SOC_QUARK_SE_C1000)

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_2), CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_2, DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_2));
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->dma_0_int_2_mask);

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_3), CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_3, DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_3));
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->dma_0_int_3_mask);

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_4), CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_4, DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_4));
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->dma_0_int_4_mask);

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_5), CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_5, DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_5));
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->dma_0_int_5_mask);

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_6), CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_6, DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_6));
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->dma_0_int_6_mask);

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_7), CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_7, DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_INT_7));
	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->dma_0_int_7_mask);

#endif /* CONFIG_SOC_QUARK_SE_C1000 */

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_DMA_0_ERROR_INT),
		    CONFIG_DMA_0_IRQ_PRI, qm_dma_0_error_isr,
		    DEVICE_GET(dma_qmsi), 0);
	irq_enable(IRQ_GET_NUMBER(QM_IRQ_DMA_0_ERROR_INT));
#if (QM_LAKEMONT)
	QM_INTERRUPT_ROUTER->dma_0_error_int_mask &= ~QM_IR_DMA_ERROR_HOST_MASK;
#elif (QM_SENSOR)
	QM_INTERRUPT_ROUTER->dma_0_error_int_mask &= ~QM_IR_DMA_ERROR_SS_MASK;
#endif

}
