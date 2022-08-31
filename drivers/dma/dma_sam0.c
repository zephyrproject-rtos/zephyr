/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_dmac

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_sam0, CONFIG_DMA_LOG_LEVEL);

#define DMA_REGS	((Dmac *)DT_INST_REG_ADDR(0))

struct dma_sam0_channel {
	dma_callback_t cb;
	void *user_data;
};

struct dma_sam0_data {
	__aligned(16) DmacDescriptor descriptors[DMAC_CH_NUM];
	__aligned(16) DmacDescriptor descriptors_wb[DMAC_CH_NUM];
	struct dma_sam0_channel channels[DMAC_CH_NUM];
};

/* Handles DMA interrupts and dispatches to the individual channel */
static void dma_sam0_isr(const struct device *dev)
{
	struct dma_sam0_data *data = dev->data;
	struct dma_sam0_channel *chdata;
	uint16_t pend = DMA_REGS->INTPEND.reg;
	uint32_t channel;

	/* Acknowledge all interrupts for the channel in pend */
	DMA_REGS->INTPEND.reg = pend;

	channel = (pend & DMAC_INTPEND_ID_Msk) >> DMAC_INTPEND_ID_Pos;
	chdata = &data->channels[channel];

	if (pend & DMAC_INTPEND_TERR) {
		if (chdata->cb) {
			chdata->cb(dev, chdata->user_data,
				   channel, -DMAC_INTPEND_TERR);
		}
	} else if (pend & DMAC_INTPEND_TCMPL) {
		if (chdata->cb) {
			chdata->cb(dev, chdata->user_data, channel, 0);
		}
	}

	/*
	 * If more than one channel is pending, we'll just immediately
	 * interrupt again and handle it through a different INTPEND value.
	 */
}

/* Configure a channel */
static int dma_sam0_config(const struct device *dev, uint32_t channel,
			   struct dma_config *config)
{
	struct dma_sam0_data *data = dev->data;
	DmacDescriptor *desc = &data->descriptors[channel];
	struct dma_block_config *block = config->head_block;
	struct dma_sam0_channel *channel_control;
	DMAC_BTCTRL_Type btctrl = { .reg = 0 };
	unsigned int key;

	if (channel >= DMAC_CH_NUM) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (config->block_count > 1) {
		LOG_ERR("Chained transfers not supported");
		/* TODO: add support for chained transfers. */
		return -ENOTSUP;
	}

	if (config->dma_slot >= DMAC_TRIG_NUM) {
		LOG_ERR("Invalid trigger number");
		return -EINVAL;
	}

	/* Lock and page in the channel configuration */
	key = irq_lock();

	/*
	 * The "bigger" DMAC on some SAM0 chips (e.g. SAMD5x) has
	 * independently accessible registers for each channel, while
	 * the other ones require an indirect channel selection before
	 * accessing shared registers.  The simplest way to detect the
	 * difference is the presence of the DMAC_CHID_ID macro from the
	 * ASF HAL (i.e. it's only defined if indirect access is required).
	 */
#ifdef DMAC_CHID_ID
	/* Select the channel for configuration */
	DMA_REGS->CHID.reg = DMAC_CHID_ID(channel);
	DMA_REGS->CHCTRLA.reg = 0;

	/* Connect the peripheral trigger */
	if (config->channel_direction == MEMORY_TO_MEMORY) {
		/*
		 * A single software trigger will start the
		 * transfer
		 */
		DMA_REGS->CHCTRLB.reg = DMAC_CHCTRLB_TRIGACT_TRANSACTION |
				    DMAC_CHCTRLB_TRIGSRC(config->dma_slot);
	} else {
		/* One peripheral trigger per beat */
		DMA_REGS->CHCTRLB.reg = DMAC_CHCTRLB_TRIGACT_BEAT |
				    DMAC_CHCTRLB_TRIGSRC(config->dma_slot);
	}

	/* Set the priority */
	if (config->channel_priority >= DMAC_LVL_NUM) {
		LOG_ERR("Invalid priority");
		goto inval;
	}

	DMA_REGS->CHCTRLB.bit.LVL = config->channel_priority;

	/* Enable the interrupts */
	DMA_REGS->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;
	if (!config->error_callback_en) {
		DMA_REGS->CHINTENSET.reg = DMAC_CHINTENSET_TERR;
	} else {
		DMA_REGS->CHINTENCLR.reg = DMAC_CHINTENSET_TERR;
	}

	DMA_REGS->CHINTFLAG.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL;
#else
	/* Channels have separate configuration registers */
	DmacChannel * chcfg = &DMA_REGS->Channel[channel];

	if (config->channel_direction == MEMORY_TO_MEMORY) {
		/*
		 * A single software trigger will start the
		 * transfer
		 */
		chcfg->CHCTRLA.reg = DMAC_CHCTRLA_TRIGACT_TRANSACTION |
				     DMAC_CHCTRLA_TRIGSRC(config->dma_slot);
	} else if ((config->channel_direction == MEMORY_TO_PERIPHERAL) ||
		(config->channel_direction == PERIPHERAL_TO_MEMORY)) {
		/* One peripheral trigger per beat */
		chcfg->CHCTRLA.reg = DMAC_CHCTRLA_TRIGACT_BURST |
				     DMAC_CHCTRLA_TRIGSRC(config->dma_slot);
	} else {
		LOG_ERR("Direction error. %d", config->channel_direction);
		goto inval;
	}

	/* Set the priority */
	if (config->channel_priority >= DMAC_LVL_NUM) {
		LOG_ERR("Invalid priority");
		goto inval;
	}

	chcfg->CHPRILVL.bit.PRILVL = config->channel_priority;

	/* Set the burst length */
	if (config->source_burst_length != config->dest_burst_length) {
		LOG_ERR("Source and destination burst lengths must be equal");
		goto inval;
	}

	if (config->source_burst_length > 16U) {
		LOG_ERR("Invalid burst length");
		goto inval;
	}

	if (config->source_burst_length > 0U) {
		chcfg->CHCTRLA.reg |= DMAC_CHCTRLA_BURSTLEN(
			config->source_burst_length - 1U);
	}

	/* Enable the interrupts */
	chcfg->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;
	if (!config->error_callback_en) {
		chcfg->CHINTENSET.reg = DMAC_CHINTENSET_TERR;
	} else {
		chcfg->CHINTENCLR.reg = DMAC_CHINTENSET_TERR;
	}

	chcfg->CHINTFLAG.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL;
#endif

	/* Set the beat (single transfer) size */
	if (config->source_data_size != config->dest_data_size) {
		LOG_ERR("Source and destination data sizes must be equal");
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
		LOG_ERR("Invalid data size");
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
		LOG_ERR("Invalid source increment");
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
		LOG_ERR("Invalid destination increment");
		goto inval;
	}

	btctrl.bit.VALID = 1;
	desc->BTCTRL = btctrl;

	channel_control = &data->channels[channel];
	channel_control->cb = config->dma_callback;
	channel_control->user_data = config->user_data;

	LOG_DBG("Configured channel %d for %08X to %08X (%u)",
		channel,
		block->source_address,
		block->dest_address,
		block->block_size);

	irq_unlock(key);
	return 0;

inval:
	irq_unlock(key);
	return -EINVAL;
}

static int dma_sam0_start(const struct device *dev, uint32_t channel)
{
	unsigned int key = irq_lock();

	ARG_UNUSED(dev);

#ifdef DMAC_CHID_ID
	DMA_REGS->CHID.reg = channel;
	DMA_REGS->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

	if (DMA_REGS->CHCTRLB.bit.TRIGSRC == 0) {
		/* Trigger via software */
		DMA_REGS->SWTRIGCTRL.reg = 1U << channel;
	}

#else
	DmacChannel * chcfg = &DMA_REGS->Channel[channel];

	chcfg->CHCTRLA.bit.ENABLE = 1;

	if (chcfg->CHCTRLA.bit.TRIGSRC == 0) {
		/* Trigger via software */
		DMA_REGS->SWTRIGCTRL.reg = 1U << channel;
	}
#endif

	irq_unlock(key);

	return 0;
}

static int dma_sam0_stop(const struct device *dev, uint32_t channel)
{
	unsigned int key = irq_lock();

	ARG_UNUSED(dev);

#ifdef DMAC_CHID_ID
	DMA_REGS->CHID.reg = channel;
	DMA_REGS->CHCTRLA.reg = 0;
#else
	DmacChannel * chcfg = &DMA_REGS->Channel[channel];

	chcfg->CHCTRLA.bit.ENABLE = 0;
#endif

	irq_unlock(key);

	return 0;
}

static int dma_sam0_reload(const struct device *dev, uint32_t channel,
			   uint32_t src, uint32_t dst, size_t size)
{
	struct dma_sam0_data *data = dev->data;
	DmacDescriptor *desc = &data->descriptors[channel];
	unsigned int key = irq_lock();

	switch (desc->BTCTRL.bit.BEATSIZE) {
	case DMAC_BTCTRL_BEATSIZE_BYTE_Val:
		desc->BTCNT.reg = size;
		break;
	case DMAC_BTCTRL_BEATSIZE_HWORD_Val:
		desc->BTCNT.reg = size / 2U;
		break;
	case DMAC_BTCTRL_BEATSIZE_WORD_Val:
		desc->BTCNT.reg = size / 4U;
		break;
	default:
		goto inval;
	}

	if (desc->BTCTRL.bit.SRCINC) {
		desc->SRCADDR.reg = src + size;
	} else {
		desc->SRCADDR.reg = src;
	}

	if (desc->BTCTRL.bit.DSTINC) {
		desc->DSTADDR.reg = dst + size;
	} else {
		desc->DSTADDR.reg = dst;
	}

	LOG_DBG("Reloaded channel %d for %08X to %08X (%u)",
		channel, src, dst, size);

	irq_unlock(key);
	return 0;

inval:
	irq_unlock(key);
	return -EINVAL;
}

static int dma_sam0_get_status(const struct device *dev, uint32_t channel,
			       struct dma_status *stat)
{
	struct dma_sam0_data *data = dev->data;
	uint32_t act;

	if (channel >= DMAC_CH_NUM || stat == NULL) {
		return -EINVAL;
	}

	act = DMA_REGS->ACTIVE.reg;
	if ((act & DMAC_ACTIVE_ABUSY) &&
	    ((act & DMAC_ACTIVE_ID_Msk) >> DMAC_ACTIVE_ID_Pos) == channel) {
		stat->busy = true;
		stat->pending_length = (act & DMAC_ACTIVE_BTCNT_Msk) >>
				       DMAC_ACTIVE_BTCNT_Pos;
	} else {
		stat->busy = false;
		stat->pending_length = data->descriptors_wb[channel].BTCNT.reg;
	}

	switch (data->descriptors[channel].BTCTRL.bit.BEATSIZE) {
	case DMAC_BTCTRL_BEATSIZE_BYTE_Val:
		break;
	case DMAC_BTCTRL_BEATSIZE_HWORD_Val:
		stat->pending_length *= 2U;
		break;
	case DMAC_BTCTRL_BEATSIZE_WORD_Val:
		stat->pending_length *= 4U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#define DMA_SAM0_IRQ_CONNECT(n)						 \
	do {								 \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, n, irq),		 \
			    DT_INST_IRQ_BY_IDX(0, n, priority),		 \
			    dma_sam0_isr, DEVICE_DT_INST_GET(0), 0);	 \
		irq_enable(DT_INST_IRQ_BY_IDX(0, n, irq));		 \
	} while (false)

static int dma_sam0_init(const struct device *dev)
{
	struct dma_sam0_data *data = dev->data;

	/* Enable clocks. */
#ifdef MCLK
	MCLK->AHBMASK.bit.DMAC_ = 1;
#else
	PM->AHBMASK.bit.DMAC_ = 1;
	PM->APBBMASK.bit.DMAC_ = 1;
#endif

	/* Set up the descriptor and write back addresses */
	DMA_REGS->BASEADDR.reg = (uintptr_t)&data->descriptors;
	DMA_REGS->WRBADDR.reg = (uintptr_t)&data->descriptors_wb;

	/* Statically map each level to the same numeric priority */
	DMA_REGS->PRICTRL0.reg =
		DMAC_PRICTRL0_LVLPRI0(0) | DMAC_PRICTRL0_LVLPRI1(1) |
		DMAC_PRICTRL0_LVLPRI2(2) | DMAC_PRICTRL0_LVLPRI3(3);

	/* Enable the unit and enable all priorities */
	DMA_REGS->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0x0F);

#if DT_INST_IRQ_HAS_CELL(0, irq)
	DMA_SAM0_IRQ_CONNECT(0);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 1)
	DMA_SAM0_IRQ_CONNECT(1);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 2)
	DMA_SAM0_IRQ_CONNECT(2);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 3)
	DMA_SAM0_IRQ_CONNECT(3);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 4)
	DMA_SAM0_IRQ_CONNECT(4);
#endif

	return 0;
}

static struct dma_sam0_data dmac_data;

static const struct dma_driver_api dma_sam0_api = {
	.config = dma_sam0_config,
	.start = dma_sam0_start,
	.stop = dma_sam0_stop,
	.reload = dma_sam0_reload,
	.get_status = dma_sam0_get_status,
};

DEVICE_DT_INST_DEFINE(0, &dma_sam0_init, NULL,
		    &dmac_data, NULL, PRE_KERNEL_1,
		    CONFIG_DMA_INIT_PRIORITY, &dma_sam0_api);
