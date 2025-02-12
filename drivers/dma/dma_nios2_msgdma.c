/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT altr_msgdma

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/init.h>
#include <string.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <altera_common.h>
#include "altera_msgdma_csr_regs.h"
#include "altera_msgdma_descriptor_regs.h"
#include "altera_msgdma.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dma_nios2, CONFIG_DMA_LOG_LEVEL);

/* Device configuration parameters */
struct nios2_msgdma_dev_data {
	const struct device *dev;
	alt_msgdma_dev *msgdma_dev;
	alt_msgdma_standard_descriptor desc;
	uint32_t direction;
	struct k_sem sem_lock;
	void *user_data;
	dma_callback_t dma_callback;
};

static void nios2_msgdma_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct nios2_msgdma_dev_data *dev_data = (struct nios2_msgdma_dev_data *)dev->data;

	/* Call Altera HAL driver ISR */
	alt_handle_irq(dev_data->msgdma_dev, DT_INST_IRQN(0));
}

static void nios2_msgdma_callback(void *context)
{
	struct nios2_msgdma_dev_data *dev_data =
		(struct nios2_msgdma_dev_data *)context;
	int dma_status;
	uint32_t status;

	status = IORD_ALTERA_MSGDMA_CSR_STATUS(dev_data->msgdma_dev->csr_base);

	if (status & ALTERA_MSGDMA_CSR_STOPPED_ON_ERROR_MASK) {
		dma_status = -EIO;
	} else if (status & ALTERA_MSGDMA_CSR_BUSY_MASK) {
		dma_status = -EBUSY;
	} else {
		dma_status = DMA_STATUS_COMPLETE;
	}

	LOG_DBG("msgdma csr status Reg: 0x%x", status);

	dev_data->dma_callback(dev_data->dev, dev_data->user_data, 0, dma_status);
}

static int nios2_msgdma_config(const struct device *dev, uint32_t channel,
			       struct dma_config *cfg)
{
	struct nios2_msgdma_dev_data *dev_data = (struct nios2_msgdma_dev_data *)dev->data;
	struct dma_block_config *dma_block;
	int status;
	uint32_t control;

	/* Nios-II MSGDMA supports only one channel per DMA core */
	if (channel != 0U) {
		LOG_ERR("invalid channel number");
		return -EINVAL;
	}

#if MSGDMA_0_CSR_PREFETCHER_ENABLE
	if (cfg->block_count > 1) {
		LOG_ERR("driver yet add support multiple descriptors");
		return -EINVAL;
	}
#else
	if (cfg->block_count != 1U) {
		LOG_ERR("invalid block count!!");
		return -EINVAL;
	}
#endif

	if (cfg->head_block == NULL) {
		LOG_ERR("head_block ptr NULL!!");
		return -EINVAL;
	}

	if (cfg->head_block->block_size > MSGDMA_0_DESCRIPTOR_SLAVE_MAX_BYTE) {
		LOG_ERR("DMA error: Data size too big: %d",
			    cfg->head_block->block_size);
		return -EINVAL;
	}

	k_sem_take(&dev_data->sem_lock, K_FOREVER);
	dev_data->dma_callback = cfg->dma_callback;
	dev_data->user_data = cfg->user_data;
	dev_data->direction = cfg->channel_direction;
	dma_block = cfg->head_block;
	control =  ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_MASK |
		  ALTERA_MSGDMA_DESCRIPTOR_CONTROL_EARLY_TERMINATION_IRQ_MASK;

	if (dev_data->direction == MEMORY_TO_MEMORY) {
		status = alt_msgdma_construct_standard_mm_to_mm_descriptor(
			dev_data->msgdma_dev, &dev_data->desc,
			(alt_u32 *)dma_block->source_address,
			(alt_u32 *)dma_block->dest_address,
			dma_block->block_size,
			control);
	} else if (dev_data->direction == MEMORY_TO_PERIPHERAL) {
		status = alt_msgdma_construct_standard_mm_to_st_descriptor(
			dev_data->msgdma_dev, &dev_data->desc,
			(alt_u32 *)dma_block->source_address,
			dma_block->block_size,
			control);
	} else if (dev_data->direction == PERIPHERAL_TO_MEMORY) {
		status = alt_msgdma_construct_standard_st_to_mm_descriptor(
			dev_data->msgdma_dev, &dev_data->desc,
			(alt_u32 *)dma_block->dest_address,
			dma_block->block_size,
			control);
	} else {
		LOG_ERR("invalid channel direction");
		status = -EINVAL;
	}

	/* Register msgdma callback */
	alt_msgdma_register_callback(dev_data->msgdma_dev,
			nios2_msgdma_callback,
			ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK |
			ALTERA_MSGDMA_CSR_STOP_ON_ERROR_MASK |
			ALTERA_MSGDMA_CSR_STOP_ON_EARLY_TERMINATION_MASK,
			dev_data);

	/* Clear the IRQ status */
	IOWR_ALTERA_MSGDMA_CSR_STATUS(dev_data->msgdma_dev->csr_base,
				      ALTERA_MSGDMA_CSR_IRQ_SET_MASK);
	k_sem_give(&dev_data->sem_lock);

	return status;
}

static int nios2_msgdma_transfer_start(const struct device *dev,
				       uint32_t channel)
{
	struct nios2_msgdma_dev_data *cfg = (struct nios2_msgdma_dev_data *)dev->data;
	int status;

	/* Nios-II MSGDMA supports only one channel per DMA core */
	if (channel != 0U) {
		LOG_ERR("Invalid channel number");
		return -EINVAL;
	}

	k_sem_take(&cfg->sem_lock, K_FOREVER);
	status = alt_msgdma_standard_descriptor_async_transfer(cfg->msgdma_dev,
								&cfg->desc);
	k_sem_give(&cfg->sem_lock);

	if (status < 0) {
		LOG_ERR("DMA transfer error (%d)", status);
	}

	return status;
}

static int nios2_msgdma_transfer_stop(const struct device *dev,
				      uint32_t channel)
{
	struct nios2_msgdma_dev_data *cfg = (struct nios2_msgdma_dev_data *)dev->data;
	int ret = -EIO;
	uint32_t status;

	k_sem_take(&cfg->sem_lock, K_FOREVER);
	/* Stop the DMA Dispatcher */
	IOWR_ALTERA_MSGDMA_CSR_CONTROL(cfg->msgdma_dev->csr_base,
				       ALTERA_MSGDMA_CSR_STOP_MASK);

	status = IORD_ALTERA_MSGDMA_CSR_STATUS(cfg->msgdma_dev->csr_base);
	k_sem_give(&cfg->sem_lock);

	if (status & ALTERA_MSGDMA_CSR_STOP_STATE_MASK) {
		LOG_DBG("DMA Dispatcher stopped");
		ret = 0;
	}

	LOG_DBG("msgdma csr status Reg: 0x%x", status);

	return status;
}

static const struct dma_driver_api nios2_msgdma_driver_api = {
	.config = nios2_msgdma_config,
	.start = nios2_msgdma_transfer_start,
	.stop = nios2_msgdma_transfer_stop,
};

static int nios2_msgdma0_initialize(const struct device *dev)
{
	struct nios2_msgdma_dev_data *dev_data = (struct nios2_msgdma_dev_data *)dev->data;

	dev_data->dev = dev;

	/* Initialize semaphore */
	k_sem_init(&dev_data->sem_lock, 1, 1);

	alt_msgdma_init(dev_data->msgdma_dev, 0, DT_INST_IRQN(0));

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nios2_msgdma_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

ALTERA_MSGDMA_CSR_DESCRIPTOR_SLAVE_INSTANCE(MSGDMA_0, MSGDMA_0_CSR,
				MSGDMA_0_DESCRIPTOR_SLAVE, msgdma_dev0)

static struct nios2_msgdma_dev_data dma0_nios2_data = {
	.msgdma_dev = &msgdma_dev0,
};

DEVICE_DT_INST_DEFINE(0, &nios2_msgdma0_initialize,
		NULL, &dma0_nios2_data, NULL, POST_KERNEL,
		CONFIG_DMA_INIT_PRIORITY, &nios2_msgdma_driver_api);
