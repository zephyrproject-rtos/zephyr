/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DMA_LEVEL
#define SYS_LOG_DOMAIN "dma"
#include <logging/sys_log.h>

#include <board.h>
#include <dma.h>

typedef void (*dma_callback)(struct device *dev, u32_t channel,
			     int error_code);

struct dma_sam0_device {
	__aligned(16) DmacDescriptor descriptors[DMAC_CH_NUM];
	__aligned(16) DmacDescriptor descriptors_wb[DMAC_CH_NUM];
	dma_callback cb[DMAC_CH_NUM];
};

/* Handles DMA channel interrupts and dispatches to the callback */
static void dma_sam0_channel_isr(struct device *dev,
				 struct dma_sam0_device *data, int channel)
{
	u8_t ints = DMAC->CHINTFLAG.reg & DMAC->CHINTENSET.reg;
	dma_callback cb = data->cb[channel];

	/* Acknowledge all interrupts */
	DMAC->CHINTFLAG.reg = ints;

	if (cb != NULL) {
		if ((ints & DMAC_CHINTFLAG_TCMPL) != 0) {
			cb(dev, channel, 0);
		}
		if ((ints & DMAC_CHINTFLAG_TERR) != 0) {
			cb(dev, channel, DMAC_CHINTFLAG_TERR);
		}
	}
}

/* Handles DMA interrupts and dispatches to the individual channel */
static void dma_sam0_isr(void *arg)
{
	struct device *dev = arg;
	struct dma_sam0_device *data = dev->driver_data;
	int channel;
	u32_t intstatus = DMAC->INTSTATUS.reg;

	for (channel = 0; intstatus != 0; intstatus >>= 1) {
		if ((intstatus & 1) != 0) {
			dma_sam0_channel_isr(dev, data, channel);
		}
		channel++;
	}
}

/* Configure a channel */
static int dma_sam0_config(struct device *dev, u32_t channel,
			   struct dma_config *config)
{
	struct dma_sam0_device *data = dev->driver_data;
	DmacDescriptor *desc = &data->descriptors[channel];
	struct dma_block_config *block = config->head_block;
	DMAC_BTCTRL_Type btctrl = {.reg = 0};
	int key;

	if (channel >= DMAC_CH_NUM) {
		return -EINVAL;
	}
	if (config->block_count > 1) {
		/* TODO: add support for chained transfers. */
		return -ENOTSUP;
	}

	/* Lock and page in the channel configuration */
	key = irq_lock();
	DMAC->CHID.reg = channel;

	/* Connect the peripheral trigger */
	if (config->dma_slot >= DMAC_TRIG_NUM) {
		goto inval;
	}
	if (DMAC->CHCTRLB.bit.TRIGSRC != config->dma_slot) {
		DMAC->CHCTRLA.reg = 0;
		DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
		while (DMAC->CHCTRLA.bit.SWRST) {
		}
		if (config->channel_direction == MEMORY_TO_MEMORY) {
			/* A single software trigger will start the
			 * transfer
			 */
			DMAC->CHCTRLB.reg = DMAC_CHCTRLB_TRIGACT_TRANSACTION |
				DMAC_CHCTRLB_TRIGSRC(config->dma_slot);
		} else {
			/* One peripheral trigger per beat */
			DMAC->CHCTRLB.reg = DMAC_CHCTRLB_TRIGACT_BEAT |
				DMAC_CHCTRLB_TRIGSRC(config->dma_slot);
		}
	}

	/* Set the priority */
	if (config->channel_priority >= DMAC_LVL_NUM) {
		goto inval;
	}
	DMAC->CHCTRLB.bit.LVL = config->channel_priority;

	/* Set the beat (single transfer) size */
	if (config->source_data_size != config->dest_data_size) {
		goto inval;
	}
	switch (config->source_data_size) {
	case 1:
		btctrl.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
		break;
	case 2:
		btctrl.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_HWORD_Val;
		break;
	case 4:
		btctrl.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_WORD_Val;
		break;
	default:
		goto inval;
	}

	/* Set up the one and only block */
	desc->BTCNT.reg = block->block_size / config->source_data_size;
	desc->DESCADDR.reg = 0;

	/* Set the automatic source / dest increment */
	switch (block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		desc->SRCADDR.reg = block->source_address + block->block_size;
		btctrl.bit.SRCINC = 1;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		desc->SRCADDR.reg = block->source_address;
		break;
	default:
		goto inval;
	}

	switch (block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		desc->DSTADDR.reg = block->dest_address + block->block_size;
		btctrl.bit.DSTINC = 1;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		desc->DSTADDR.reg = block->dest_address;
		break;
	default:
		goto inval;
	}

	btctrl.bit.VALID = 1;
	desc->BTCTRL = btctrl;
	data->cb[channel] = config->dma_callback;
	/* Enable the interrupts */
	DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL | DMAC_CHINTENSET_TERR;

	irq_unlock(key);
	return 0;

inval:
	irq_unlock(key);
	return -EINVAL;
}

static int dma_sam0_start(struct device *dev, u32_t channel)
{
	int key = irq_lock();

	DMAC->CHID.reg = channel;
	DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

	if (DMAC->CHCTRLB.bit.TRIGSRC == 0) {
		/* Trigger via software */
		DMAC->SWTRIGCTRL.reg = 1 << channel;
	}

	irq_unlock(key);

	return 0;
}

static int dma_sam0_stop(struct device *dev, u32_t channel)
{
	int key = irq_lock();

	DMAC->CHID.reg = channel;
	DMAC->CHCTRLA.reg = 0;

	irq_unlock(key);

	return 0;
}

DEVICE_DECLARE(dma_sam0_0);

static int dma_sam0_init(struct device *dev)
{
	struct dma_sam0_device *data = dev->driver_data;

	/* Enable clocks. */
	PM->AHBMASK.reg |= PM_AHBMASK_DMAC;
	PM->APBBMASK.reg |= PM_APBBMASK_DMAC;

	/* Disable and reset. */
	DMAC->CTRL.bit.DMAENABLE = 0;
	DMAC->CTRL.bit.SWRST = 1;

	/* Set up the descriptor and write back addresses */
	DMAC->BASEADDR.reg = (uintptr_t)&data->descriptors;
	DMAC->WRBADDR.reg = (uintptr_t)&data->descriptors_wb;

	/* Statically map each level to the same numeric priority */
	DMAC->PRICTRL0.reg =
		DMAC_PRICTRL0_LVLPRI0(0) | DMAC_PRICTRL0_LVLPRI1(1) |
		DMAC_PRICTRL0_LVLPRI2(2) | DMAC_PRICTRL0_LVLPRI3(3);

	/* Enable the unit and enable all priorities */
	DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0x0F);

	IRQ_CONNECT(CONFIG_DMA_SAM0_IRQ, CONFIG_DMA_SAM0_IRQ_PRIORITY,
		    dma_sam0_isr, DEVICE_GET(dma_sam0_0), 0);
	irq_enable(CONFIG_DMA_SAM0_IRQ);

	return 0;
}

static struct dma_sam0_device dma_sam0_device_0;

static const struct dma_driver_api dma_sam0_api = {
	.config = dma_sam0_config,
	.start = dma_sam0_start,
	.stop = dma_sam0_stop,
};

DEVICE_AND_API_INIT(dma_sam0_0, CONFIG_DMA_SAM0_LABEL, &dma_sam0_init,
		    &dma_sam0_device_0, NULL, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dma_sam0_api);
