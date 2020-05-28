/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <init.h>
#include <string.h>
#include <soc.h>
#include <drivers/dma.h>
#include <altera_common.h>
#include "altera_msgdma_csr_regs.h"
#include "altera_msgdma_descriptor_regs.h"
#include "altera_msgdma.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_nios2);

/* Device configuration parameters */
struct nios2_msgdma_dev_cfg {
	alt_msgdma_dev *msgdma_dev;
	alt_msgdma_standard_descriptor desc;
	uint32_t direction;
	struct k_sem sem_lock;
	void *user_data;
	dma_callback_t dma_callback;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	((struct nios2_msgdma_dev_cfg *)(dev)->config)

static void nios2_msgdma_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct nios2_msgdma_dev_cfg *cfg = DEV_CFG(dev);

	/* Call Altera HAL driver ISR */
	alt_handle_irq(cfg->msgdma_dev, MSGDMA_0_CSR_IRQ);
}

static void nios2_msgdma_callback(void *context)
{
	struct device *dev = (struct device *)context;
	struct nios2_msgdma_dev_cfg *dev_cfg = DEV_CFG(dev);
	int err_code;
	uint32_t status;

	status = IORD_ALTERA_MSGDMA_CSR_STATUS(dev_cfg->msgdma_dev->csr_base);

	if (status & ALTERA_MSGDMA_CSR_STOPPED_ON_ERROR_MASK) {
		err_code = -EIO;
	} else if (status & ALTERA_MSGDMA_CSR_BUSY_MASK) {
		err_code = -EBUSY;
	} else {
		err_code = 0;
	}

	LOG_DBG("msgdma csr status Reg: 0x%x", status);

	dev_cfg->dma_callback(dev, dev_cfg->user_data, 0, err_code);
}

static int nios2_msgdma_config(struct device *dev, uint32_t channel,
			       struct dma_config *cfg)
{
	struct nios2_msgdma_dev_cfg *dev_cfg = DEV_CFG(dev);
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

	k_sem_take(&dev_cfg->sem_lock, K_FOREVER);
	dev_cfg->dma_callback = cfg->dma_callback;
	dev_cfg->user_data = cfg->user_data;
	dev_cfg->direction = cfg->channel_direction;
	dma_block = cfg->head_block;
	control =  ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_MASK |
		  ALTERA_MSGDMA_DESCRIPTOR_CONTROL_EARLY_TERMINATION_IRQ_MASK;

	if (dev_cfg->direction == MEMORY_TO_MEMORY) {
		status = alt_msgdma_construct_standard_mm_to_mm_descriptor(
			dev_cfg->msgdma_dev, &dev_cfg->desc,
			(alt_u32 *)dma_block->source_address,
			(alt_u32 *)dma_block->dest_address,
			dma_block->block_size,
			control);
	} else if (dev_cfg->direction == MEMORY_TO_PERIPHERAL) {
		status = alt_msgdma_construct_standard_mm_to_st_descriptor(
			dev_cfg->msgdma_dev, &dev_cfg->desc,
			(alt_u32 *)dma_block->source_address,
			dma_block->block_size,
			control);
	} else if (dev_cfg->direction == PERIPHERAL_TO_MEMORY) {
		status = alt_msgdma_construct_standard_st_to_mm_descriptor(
			dev_cfg->msgdma_dev, &dev_cfg->desc,
			(alt_u32 *)dma_block->dest_address,
			dma_block->block_size,
			control);
	} else {
		LOG_ERR("invalid channel direction");
		status = -EINVAL;
	}

	/* Register msgdma callback */
	alt_msgdma_register_callback(dev_cfg->msgdma_dev,
			nios2_msgdma_callback,
			ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK |
			ALTERA_MSGDMA_CSR_STOP_ON_ERROR_MASK |
			ALTERA_MSGDMA_CSR_STOP_ON_EARLY_TERMINATION_MASK,
			dev);

	/* Clear the IRQ status */
	IOWR_ALTERA_MSGDMA_CSR_STATUS(dev_cfg->msgdma_dev->csr_base,
				      ALTERA_MSGDMA_CSR_IRQ_SET_MASK);
	k_sem_give(&dev_cfg->sem_lock);

	return status;
}

static int nios2_msgdma_transfer_start(struct device *dev, uint32_t channel)
{
	struct nios2_msgdma_dev_cfg *cfg = DEV_CFG(dev);
	int status;

	/* Nios-II mSGDMA supports only one channel per DMA core */
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

static int nios2_msgdma_transfer_stop(struct device *dev, uint32_t channel)
{
	struct nios2_msgdma_dev_cfg *cfg = DEV_CFG(dev);
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

/* DMA0 */
DEVICE_DECLARE(dma0_nios2);

static int nios2_msgdma0_initialize(struct device *dev)
{
	struct nios2_msgdma_dev_cfg *dev_cfg = DEV_CFG(dev);

	/* Initialize semaphore */
	k_sem_init(&dev_cfg->sem_lock, 1, 1);

	alt_msgdma_init(dev_cfg->msgdma_dev, 0, MSGDMA_0_CSR_IRQ);

	IRQ_CONNECT(MSGDMA_0_CSR_IRQ, CONFIG_DMA_0_IRQ_PRI,
		    nios2_msgdma_isr, DEVICE_GET(dma0_nios2), 0);

	irq_enable(MSGDMA_0_CSR_IRQ);

	return 0;
}

ALTERA_MSGDMA_CSR_DESCRIPTOR_SLAVE_INSTANCE(MSGDMA_0, MSGDMA_0_CSR,
				MSGDMA_0_DESCRIPTOR_SLAVE, msgdma_dev0)

static struct nios2_msgdma_dev_cfg dma0_nios2_config = {
	.msgdma_dev = &msgdma_dev0,
};

DEVICE_AND_API_INIT(dma0_nios2, CONFIG_DMA_0_NAME, &nios2_msgdma0_initialize,
		    NULL, &dma0_nios2_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &nios2_msgdma_driver_api);
