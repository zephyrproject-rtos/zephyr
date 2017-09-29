/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

#define CYCLE_NOP \
	 __asm__ __volatile__ ("nop"); \
	 __asm__ __volatile__ ("nop"); \
	 __asm__ __volatile__ ("nop"); \
	 __asm__ __volatile__ ("nop")


struct dma_qmsi_config_info {
	qm_dma_t instance; /* Controller instance. */
};

struct dma_qmsi_context {
	u32_t index;
	struct device *dev;
};

struct dma_qmsi_driver_data {
	void (*transfer[QM_DMA_CHANNEL_NUM])(struct device *dev, void *data);
	void (*error[QM_DMA_CHANNEL_NUM])(struct device *dev, void *data);
	void *callback_data[QM_DMA_CHANNEL_NUM];
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
	qm_dma_context_t saved_ctx;
#endif
	void (*dma_user_callback[QM_DMA_CHANNEL_NUM])(struct device *dev,
						      u32_t channel_id,
						      int error_code);
};


static struct dma_qmsi_context dma_context[QM_DMA_CHANNEL_NUM];
static void dma_qmsi_config(struct device *dev);

static void dma_drv_callback(void *callback_context, u32_t len,
			     int error_code)
{
	struct dma_qmsi_context *context = callback_context;
	struct dma_qmsi_driver_data *data;
	u32_t channel;

	channel = context->index;
	data = context->dev->driver_data;

	data->dma_user_callback[channel](context->dev, channel, error_code);
}

static int width_index(u32_t num_bytes, u32_t *index)
{
	switch (num_bytes) {
	case 1:
		*index = QM_DMA_TRANS_WIDTH_8;
		break;
	case 2:
		*index = QM_DMA_TRANS_WIDTH_16;
		break;
	case 4:
		*index = QM_DMA_TRANS_WIDTH_32;
		break;
	case 8:
		*index = QM_DMA_TRANS_WIDTH_64;
		break;
	case 16:
		*index = QM_DMA_TRANS_WIDTH_128;
		break;
	case 32:
		*index = QM_DMA_TRANS_WIDTH_256;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bst_index(u32_t num_units, u32_t *index)
{
	switch (num_units) {
	case 1:
		*index = QM_DMA_BURST_TRANS_LENGTH_1;
		break;
	case 4:
		*index = QM_DMA_BURST_TRANS_LENGTH_4;
		break;
	case 8:
		*index = QM_DMA_BURST_TRANS_LENGTH_8;
		break;
	case 16:
		*index = QM_DMA_BURST_TRANS_LENGTH_16;
		break;
	case 32:
		*index = QM_DMA_BURST_TRANS_LENGTH_32;
		break;
	case 64:
		*index = QM_DMA_BURST_TRANS_LENGTH_64;
		break;
	case 128:
		*index = QM_DMA_BURST_TRANS_LENGTH_128;
		break;
	case 256:
		*index = QM_DMA_BURST_TRANS_LENGTH_256;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int dma_qmsi_chan_config(struct device *dev, u32_t channel,
				struct dma_config *config)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;
	struct dma_qmsi_driver_data *data = dev->driver_data;
	qm_dma_transfer_t qmsi_transfer_cfg = { 0 };
	qm_dma_channel_config_t qmsi_cfg = { 0 };
	u32_t temp = 0;
	int ret = 0;

	if (config->block_count != 1) {
		return -ENOTSUP;
	}

	qmsi_cfg.handshake_interface = (qm_dma_handshake_interface_t)
					config->dma_slot;
	qmsi_cfg.channel_direction = (qm_dma_channel_direction_t)
				      config->channel_direction;

	ret = width_index(config->source_data_size, &temp);
	if (ret != 0) {
		return ret;
	}
	qmsi_cfg.source_transfer_width = (qm_dma_transfer_width_t) temp;

	ret = width_index(config->dest_data_size, &temp);
	if (ret != 0) {
		return ret;
	}
	qmsi_cfg.destination_transfer_width = (qm_dma_transfer_width_t) temp;

	ret = bst_index(config->dest_burst_length, &temp);
	if (ret != 0) {
		return ret;
	}
	qmsi_cfg.destination_burst_length = (qm_dma_burst_length_t) temp;

	ret = bst_index(config->source_burst_length, &temp);
	if (ret != 0) {
		return ret;
	}
	qmsi_cfg.source_burst_length = (qm_dma_burst_length_t) temp;

	/* TODO: add support for using other DMA transfer types. */
	qmsi_cfg.transfer_type = QM_DMA_TYPE_SINGLE;

	data->dma_user_callback[channel] = config->dma_callback;

	dma_context[channel].index = channel;
	dma_context[channel].dev = dev;

	qmsi_cfg.callback_context = &dma_context[channel];
	qmsi_cfg.client_callback = dma_drv_callback;

	ret = qm_dma_channel_set_config(info->instance, channel, &qmsi_cfg);
	if (ret != 0) {
		return ret;
	}

	qmsi_transfer_cfg.block_size = config->head_block->block_size;
	qmsi_transfer_cfg.source_address = (u32_t *)
					   config->head_block->source_address;
	qmsi_transfer_cfg.destination_address = (u32_t *)
					      config->head_block->dest_address;

	return qm_dma_transfer_set_config(info->instance, channel,
					  &qmsi_transfer_cfg);
}

static int dma_qmsi_start(struct device *dev, u32_t channel)
{
	int ret;
	const struct dma_qmsi_config_info *info = dev->config->config_info;

	ret = qm_dma_transfer_start(info->instance, channel);

	CYCLE_NOP;

	return ret;
}

static int dma_qmsi_stop(struct device *dev, u32_t channel)
{
	const struct dma_qmsi_config_info *info = dev->config->config_info;

	return qm_dma_transfer_terminate(info->instance, channel);
}

static const struct dma_driver_api dma_funcs = {
	.config = dma_qmsi_chan_config,
	.start = dma_qmsi_start,
	.stop = dma_qmsi_stop
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void dma_qmsi_set_power_state(struct device *dev, u32_t power_state)
{
	struct dma_qmsi_driver_data *ctx = dev->driver_data;

	ctx->device_power_state = power_state;
}

static u32_t dma_qmsi_get_power_state(struct device *dev)
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

static int dma_qmsi_device_ctrl(struct device *dev, u32_t ctrl_command,
				void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return dma_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return dma_resume_device(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = dma_qmsi_get_power_state(dev);
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
