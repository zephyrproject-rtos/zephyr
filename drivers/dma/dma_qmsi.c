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
#include <nanokernel.h>
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

int dma_qmsi_init(struct device *dev)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;

	dma_qmsi_config(dev);
	qm_dma_init(info->instance);
	return 0;
}

static const struct dma_qmsi_config_info dma_qmsi_config_data = {
	.instance = QM_DMA_0,
};

static struct dma_qmsi_driver_data dma_qmsi_dev_data;


DEVICE_AND_API_INIT(dma_qmsi, CONFIG_DMA_0_NAME,
		    &dma_qmsi_init, &dma_qmsi_dev_data, &dma_qmsi_config_data,
		    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&dma_funcs);

static void dma_qmsi_config(struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(QM_IRQ_DMA_0, CONFIG_DMA_0_IRQ_PRI,
			qm_dma_0_isr_0, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_0);
	QM_SCSS_INT->int_dma_channel_0_mask &= ~BIT(0);

	IRQ_CONNECT(QM_IRQ_DMA_1, CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_1, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_1);
	QM_SCSS_INT->int_dma_channel_1_mask &= ~BIT(0);

#if (CONFIG_SOC_QUARK_SE_C1000)

	IRQ_CONNECT(QM_IRQ_DMA_2, CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_2, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_2);
	QM_SCSS_INT->int_dma_channel_2_mask &= ~BIT(0);

	IRQ_CONNECT(QM_IRQ_DMA_3, CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_3, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_3);
	QM_SCSS_INT->int_dma_channel_3_mask &= ~BIT(0);

	IRQ_CONNECT(QM_IRQ_DMA_4, CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_4, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_4);
	QM_SCSS_INT->int_dma_channel_4_mask &= ~BIT(0);

	IRQ_CONNECT(QM_IRQ_DMA_5, CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_5, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_5);
	QM_SCSS_INT->int_dma_channel_5_mask &= ~BIT(0);

	IRQ_CONNECT(QM_IRQ_DMA_6, CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_6, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_6);
	QM_SCSS_INT->int_dma_channel_6_mask &= ~BIT(0);

	IRQ_CONNECT(QM_IRQ_DMA_7, CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_7, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_7);
	QM_SCSS_INT->int_dma_channel_7_mask &= ~BIT(0);

#endif /* CONFIG_SOC_QUARK_SE_C1000 */

	IRQ_CONNECT(QM_IRQ_DMA_ERR, CONFIG_DMA_0_IRQ_PRI,
				qm_dma_0_isr_err, DEVICE_GET(dma_qmsi), 0);
	irq_enable(QM_IRQ_DMA_ERR);
	QM_SCSS_INT->int_dma_error_mask &= ~BIT(0);

}
