/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <soc.h>

/* #define SOC_DMA_DEBUG_ENABLE 1 */

int soc_dma_init(void)
{
	mec_pcr_periph_slp_ctrl(PCR_DMA, 0);

	struct dma_regs *regs = (struct dma_regs *)(DMA_BASE);

	regs->ACTRST = MCHP_DMAM_CTRL_SOFT_RESET | MCHP_DMAM_CTRL_ENABLE;

	uint32_t cnt = 256U;

	while (cnt--) {
		if (regs->ACTRST == 0) {
			regs->ACTRST = MCHP_DMAM_CTRL_ENABLE;
			return 0;
		}
	}

	return -ETIMEDOUT;
}

int soc_dma_chan_init(uint8_t chan)
{
	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	if (regs->ACTV) {
		regs->CTRL |= MCHP_DMA_C_XFER_ABORT;

		uint32_t cnt = 256U;

		while (cnt--) {
			if (!(regs->CTRL & MCHP_DMA_C_BUSY_STS)) {
				break;
			}
		}

		if (regs->CTRL & MCHP_DMA_C_BUSY_STS) {
			return -ETIMEDOUT;
		}

		regs->ACTV = 0U;
	}

	regs->CTRL = 0U;
	regs->MSTART = 0U;
	regs->MEND = 0U;
	regs->IEN = 0U;
	regs->ISTS = 0xFFFFFFFFul;

	return 0;
}

int soc_dma_chan_cfg_mem(uint8_t chan, uintptr_t mbase, uint32_t sz)
{
	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	regs->MSTART = mbase;
	regs->MEND = mbase + sz;

	return 0;
}

int soc_dma_chan_cfg_device(uint8_t chan, uint32_t dev_dma_id,
			    uintptr_t dev_addr)
{
	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	uint32_t ctrl = regs->CTRL & ~(MCHP_DMA_C_DEV_NUM_MASK);

	ctrl |= (dev_dma_id << MCHP_DMA_C_DEV_NUM_POS) &
		MCHP_DMA_C_DEV_NUM_MASK;
	regs->CTRL = ctrl;
	regs->DSTART = dev_addr;

	return 0;
}

int soc_dma_chan_cfg_unit_size(uint8_t chan, uint8_t units)
{
	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	uint32_t ctrl = regs->CTRL & ~(MCHP_DMA_C_XFRU_MASK);

	ctrl |= (((uint32_t)units << MCHP_DMA_C_XFRU_POS) &
		MCHP_DMA_C_XFRU_MASK);
	regs->CTRL = ctrl;

	return 0;
}

int soc_dma_chan_cfg_dir(uint8_t chan, uint8_t mem2dev)
{
	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	uint32_t ctrl = regs->CTRL & ~(MCHP_DMA_C_XFRU_MASK);

	if (mem2dev) {
		ctrl |= MCHP_DMA_C_MEM2DEV;
	} else {
		ctrl &= ~(MCHP_DMA_C_MEM2DEV);
	}

	regs->CTRL = ctrl;

	return 0;
}

int soc_dma_chan_cfg_addr_incr(uint8_t chan, uint8_t incr_flags)
{
	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	uint32_t ctrl = regs->CTRL;

	ctrl &= ~(MCHP_DMA_C_INCR_MEM | MCHP_DMA_C_INCR_DEV);

	if (incr_flags & DMA_FLAG_INCR_MEM) {
		ctrl |= MCHP_DMA_C_INCR_MEM;
	}
	if (incr_flags & DMA_FLAG_INCR_DEV) {
		ctrl |= MCHP_DMA_C_INCR_DEV;
	}

	regs->CTRL = ctrl;

	return 0;
}

/*
 * Set properties of DMA channel transfer: unit size, direction, increment
 * memory start address, increment device address, and lock channel.
 */
int soc_dma_chan_cfg_flags(uint8_t chan, uint32_t flags)
{
	uint32_t ctrl = 0U;
	uint32_t units = 0U;

	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	ctrl = regs->CTRL & ~(MCHP_DMA_C_MEM2DEV | MCHP_DMA_C_INCR_MEM |
			      MCHP_DMA_C_INCR_DEV | MCHP_DMA_C_LOCK_CHAN |
			      MCHP_DMA_C_XFRU_MASK);

	units = flags & DMA_FLAG_UNITS_MASK;
	switch (units) {
	case 1U:
	case 2U:
	case 4U:
		break;
	default:
		units = 1U;
		break;
	}
	ctrl |= (units << MCHP_DMA_C_XFRU_POS);

	if (flags & DMA_FLAG_MEM2DEV) {
		ctrl |= MCHP_DMA_C_MEM2DEV;
	}

	if (flags & DMA_FLAG_INCR_MEM) {
		ctrl |= MCHP_DMA_C_INCR_MEM;
	}

	if (flags & DMA_FLAG_INCR_DEV) {
		ctrl |= MCHP_DMA_C_INCR_DEV;
	}

	if (flags & MCHP_DMA_C_LOCK_CHAN) {
		ctrl |= MCHP_DMA_C_LOCK_CHAN;
	}

	regs->CTRL = ctrl;

	return 0;
}

int soc_dma_chan_activate(uint8_t chan, int activate)
{
	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	if (activate) {
		regs->ACTV |= MCHP_DMA_ACTV_VAL;
	} else {
		regs->ACTV &= ~(MCHP_DMA_ACTV_VAL);
	}

	return 0;
}

int mec_dma_start(uint8_t chan, uint32_t start_flags)
{
	if (chan >= MCHP_DMA_CHANNELS) {
		return -EINVAL;
	}

	struct dma_chan_regs *regs =
		(struct dma_chan_regs *)DMA_CHAN_BASE(chan);

	if (start_flags & DMA_START_IEN) {
		regs->IEN = (MCHP_DMA_IEN_BUS_ERR |
			     MCHP_DMA_IEN_FLOW_CTRL_ERR |
			     MCHP_DMA_IEN_DONE);
	}

	regs->ISTS = MCHP_DMA_STS_ALL;

	if (start_flags & DMA_START_SWFC) {
		regs->CTRL |= MCHP_DMA_C_RUN;
	} else {
		regs->CTRL |= MCHP_DMA_C_XFER_GO;
	}

	return 0;
}
