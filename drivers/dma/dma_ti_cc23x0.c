/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_dma

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_cc23x0, CONFIG_DMA_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <driverlib/clkctl.h>
#include <driverlib/udma.h>

#include <inc/hw_evtsvt.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>

/*
 * Channels 0 to 5 are assigned to peripherals.
 * Channels 6 and 7 are assigned to SW-initiated transfers.
 */
#define DMA_CC23_PERIPH_CH_MAX	5
#define DMA_CC23_SW_CH_MIN	6
#define DMA_CC23_SW_CH_MAX	7

#define DMA_CC23_IS_SW_CH(ch)	((ch) >= DMA_CC23_SW_CH_MIN)

/*
 * In basic mode, the DMA controller performs transfers as long as there are more items
 * to transfer, and a transfer request is present. This mode is used with peripherals that
 * assert a DMA request signal whenever the peripheral is ready for a data transfer.
 * Auto mode is similar to basic mode, except that when a transfer request is received,
 * the transfer completes, even if the DMA request is removed. This mode is suitable for
 * software-triggered transfers.
 */
#define DMA_CC23_MODE(ch)	(DMA_CC23_IS_SW_CH(ch) ? UDMA_MODE_AUTO : UDMA_MODE_BASIC)

/* Each DMA channel is multiplexed between two peripherals which ID is in range 0 to 7 */
#define DMA_CC23_IPID_MASK	GENMASK(2, 0)
#define DMA_CC23_CHXSEL_REG(ch)	HWREG(EVTSVT_BASE + EVTSVT_O_DMACH0SEL + sizeof(uint32_t) * (ch))

struct dma_cc23x0_channel {
	uint8_t data_size;
	dma_callback_t cb;
	void *user_data;
};

struct dma_cc23x0_data {
	__aligned(1024) uDMAControlTableEntry desc[UDMA_NUM_CHANNELS];
	struct dma_cc23x0_channel channels[UDMA_NUM_CHANNELS];
};

/*
 * If the channel is a software channel, then the completion will be signaled
 * on this DMA dedicated interrupt.
 * If a peripheral channel is used, then the completion will be signaled on the
 * peripheral's interrupt.
 */
static void dma_cc23x0_isr(const struct device *dev)
{
	struct dma_cc23x0_data *data = dev->data;
	struct dma_cc23x0_channel *ch_data;
	uint32_t done_flags;
	int i;

	done_flags = uDMAIntStatus();

	for (i = DMA_CC23_SW_CH_MIN; i <= DMA_CC23_SW_CH_MAX; i++) {
		if (!(done_flags & BIT(i))) {
			continue;
		}

		LOG_DBG("DMA transfer completed on channel %u", i);

		ch_data = &data->channels[i];
		if (ch_data->cb) {
			ch_data->cb(dev, ch_data->user_data, i, DMA_STATUS_COMPLETE);
		}

		uDMAClearInt(done_flags & BIT(i));
	}
}

static uint32_t dma_cc23x0_set_addr_adj(uint32_t *control, uint16_t addr_adj,
					uint32_t inc_flags, uint32_t no_inc_flags)
{
	switch (addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		*control |= inc_flags;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*control |= no_inc_flags;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int dma_cc23x0_config(const struct device *dev, uint32_t channel,
			     struct dma_config *config)
{
	struct dma_cc23x0_data *data = dev->data;
	struct dma_cc23x0_channel *ch_data;
	struct dma_block_config *block = config->head_block;
	uint32_t control;
	uint32_t data_size;
	uint32_t src_inc_flags;
	uint32_t dst_inc_flags;
	uint32_t xfer_size;
	uint32_t burst_len;
	int ret;

	if (channel >= UDMA_NUM_CHANNELS) {
		LOG_ERR("Invalid channel (%u)", channel);
		return -EINVAL;
	}

	if (config->dma_slot > DMA_CC23_IPID_MASK) {
		LOG_ERR("Invalid trigger (%u)", config->dma_slot);
		return -EINVAL;
	}

	if (config->block_count > 1) {
		LOG_ERR("Chained transfers not supported");
		return -ENOTSUP;
	}

	switch (config->source_data_size) {
	case 1:
		src_inc_flags = UDMA_SRC_INC_8;
		break;
	case 2:
		src_inc_flags = UDMA_SRC_INC_16;
		break;
	case 4:
		src_inc_flags = UDMA_SRC_INC_32;
		break;
	default:
		LOG_ERR("Invalid source data size (%u)", config->source_data_size);
		return -EINVAL;
	}

	switch (config->dest_data_size) {
	case 1:
		dst_inc_flags = UDMA_DST_INC_8;
		break;
	case 2:
		dst_inc_flags = UDMA_DST_INC_16;
		break;
	case 4:
		dst_inc_flags = UDMA_DST_INC_32;
		break;
	default:
		LOG_ERR("Invalid destination data size (%u)", config->dest_data_size);
		return -EINVAL;
	}

	data_size = MIN(config->source_data_size, config->dest_data_size);

	switch (data_size) {
	case 1:
		control = UDMA_SIZE_8;
		break;
	case 2:
		control = UDMA_SIZE_16;
		break;
	case 4:
		control = UDMA_SIZE_32;
		break;
	default:
		LOG_ERR("Invalid data size (%u)", data_size);
		return -EINVAL;
	}

	ret = dma_cc23x0_set_addr_adj(&control, block->source_addr_adj,
				      src_inc_flags, UDMA_SRC_INC_NONE);
	if (ret) {
		LOG_ERR("Invalid source address adjustment type (%u)", block->source_addr_adj);
		return ret;
	}

	ret = dma_cc23x0_set_addr_adj(&control, block->dest_addr_adj,
				      dst_inc_flags, UDMA_DST_INC_NONE);
	if (ret) {
		LOG_ERR("Invalid dest address adjustment type (%u)", block->dest_addr_adj);
		return ret;
	}

	xfer_size = block->block_size / data_size;
	if (!xfer_size || xfer_size > UDMA_XFER_SIZE_MAX) {
		LOG_ERR("Invalid block size (%u - must be in range %u to %u)",
			block->block_size, data_size, data_size * UDMA_XFER_SIZE_MAX);
		return -EINVAL;
	}

	burst_len = config->source_burst_length / data_size;
	if (burst_len <= UDMA_XFER_SIZE_MAX && IS_POWER_OF_TWO(burst_len)) {
		control |= LOG2(burst_len) << UDMA_ARB_S;
	} else {
		LOG_ERR("Invalid burst length (%u - must be a power of 2 between %u and %u)",
			config->source_burst_length, data_size, data_size * UDMA_XFER_SIZE_MAX);
		return -EINVAL;
	}

	ch_data = &data->channels[channel];
	ch_data->data_size = data_size;
	ch_data->cb = config->dma_callback;
	ch_data->user_data = config->user_data;

	if (uDMAIsChannelEnabled(BIT(channel))) {
		return -EBUSY;
	}

	if (DMA_CC23_IS_SW_CH(channel)) {
		uDMAEnableSwEventInt(BIT(channel));
	} else {
		/* Select peripheral */
		DMA_CC23_CHXSEL_REG(channel) = config->dma_slot;
	}

	uDMASetChannelControl(&data->desc[channel], control);

	uDMASetChannelTransfer(&data->desc[channel], DMA_CC23_MODE(channel),
			       (void *)block->source_address,
			       (void *)block->dest_address,
			       xfer_size);

	LOG_DBG("Configured channel %u for %08x to %08x (%u bytes)",
		channel,
		block->source_address,
		block->dest_address,
		block->block_size);

	return 0;
}

static int dma_cc23x0_start(const struct device *dev, uint32_t channel)
{
	if (uDMAIsChannelEnabled(BIT(channel))) {
		return 0;
	}

	uDMAEnableChannel(BIT(channel));

	if (DMA_CC23_IS_SW_CH(channel)) {
		/* Request DMA channel to start a memory to memory transfer */
		uDMARequestChannel(BIT(channel));
	}

	return 0;
}

static int dma_cc23x0_stop(const struct device *dev, uint32_t channel)
{
	uDMADisableChannel(BIT(channel));

	return 0;
}

static int dma_cc23x0_reload(const struct device *dev, uint32_t channel,
			     uint32_t src, uint32_t dst, size_t size)
{
	struct dma_cc23x0_data *data = dev->data;
	struct dma_cc23x0_channel *ch_data = &data->channels[channel];
	uint32_t xfer_size = size / ch_data->data_size;

	if (uDMAIsChannelEnabled(BIT(channel))) {
		return -EBUSY;
	}

	uDMASetChannelTransfer(&data->desc[channel], DMA_CC23_MODE(channel),
			       (void *)src, (void *)dst, xfer_size);

	LOG_DBG("Reloaded channel %u for %08x to %08x (%u bytes)",
		channel, src, dst, size);

	return 0;
}

static int dma_cc23x0_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *stat)
{
	struct dma_cc23x0_data *data = dev->data;
	uint8_t ch_sel;

	if (channel >= UDMA_NUM_CHANNELS || !stat) {
		return -EINVAL;
	}

	ch_sel = DMA_CC23_CHXSEL_REG(channel) & DMA_CC23_IPID_MASK;
	switch (channel) {
	case 0:
		if (ch_sel == 0) {
			/* spi0txtrg */
			stat->dir = MEMORY_TO_PERIPHERAL;
		} else {
			/* uart0rxtrg */
			stat->dir = PERIPHERAL_TO_MEMORY;
		}
		break;
	case 1:
		if (ch_sel == 1) {
			/* spi0rxtrg */
			stat->dir = PERIPHERAL_TO_MEMORY;
		} else {
			/* uart0txtrg */
			stat->dir = MEMORY_TO_PERIPHERAL;
		}
		break;
	case 2:
	case 4:
		/* ch2: uart0txtrg | ch4: laestrga */
		stat->dir = MEMORY_TO_PERIPHERAL;
		break;
	case 3:
	case 5:
		/* ch3: adc0trg or uart0rxtrg | ch5: laestrgb or adc0trg */
		stat->dir = PERIPHERAL_TO_MEMORY;
		break;
	default:
		/* ch6, ch7: SW trg */
		stat->dir = MEMORY_TO_MEMORY;
		break;
	}

	stat->busy = uDMAIsChannelEnabled(BIT(channel));

	stat->pending_length = uDMAGetChannelSize(&data->desc[channel]);
	if (data->desc[channel].control & UDMA_SIZE_16) {
		stat->pending_length <<= 1;
	} else if (data->desc[channel].control & UDMA_SIZE_32) {
		stat->pending_length <<= 2;
	}

	return 0;
}

static int dma_cc23x0_init(const struct device *dev)
{
	struct dma_cc23x0_data *data = dev->data;

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    dma_cc23x0_isr,
		    DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(0));

	/* Enable clock */
	CLKCTLEnable(CLKCTL_BASE, CLKCTL_DMA);

	/* Enable DMA */
	uDMAEnable();

	/* Set base address for channel control table (descriptors) */
	uDMASetControlBase(data->desc);

	return 0;
}

static struct dma_cc23x0_data cc23x0_data;

static const struct dma_driver_api dma_cc23x0_api = {
	.config = dma_cc23x0_config,
	.start = dma_cc23x0_start,
	.stop = dma_cc23x0_stop,
	.reload = dma_cc23x0_reload,
	.get_status = dma_cc23x0_get_status,
};

DEVICE_DT_INST_DEFINE(0, &dma_cc23x0_init, NULL,
		      &cc23x0_data, NULL,
		      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,
		      &dma_cc23x0_api);
