/*
 * Copyright (c) 2025 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>

#if defined(CONFIG_DCACHE) && defined(CONFIG_CACHE_MANAGEMENT) && !defined(CONFIG_NOCACHE_MEMORY)
#include <zephyr/cache.h>
#endif

#define DT_DRV_COMPAT andestech_atcdmacx00
#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_andes_atcdmacx00);

#define ATCDMACx00_MAX_CHAN	8
#define ATCDMAC300_VERSION	0x10230
#define ATCDMAC100_VERSION      0x1021

#define DMA_INT_IDREV(dev)		(((struct dma_atcdmacx00_cfg *)dev->config)->base + 0x00)
#define DMA_INT_STATUS(dev)		(((struct dma_atcdmacx00_cfg *)dev->config)->base + 0x30)

#define OFFSET_TABLE(dev)		(((struct dma_atcdmacx00_data *)dev->data)->table)
#define DMA_CH_OFFSET(ch)		(ch * (OFFSET_TABLE(dev).ch_offset))

#define DMA_ABORT(dev)			(((struct dma_atcdmacx00_cfg *)dev->config)->base	\
					+ OFFSET_TABLE(dev).abort)
#define DMA_CH_CTRL(dev, ch)		(((struct dma_atcdmacx00_cfg *)dev->config)->base	\
					+ OFFSET_TABLE(dev).ctrl + DMA_CH_OFFSET(ch))
#define DMA_CH_TRANSIZE(dev, ch)	(((struct dma_atcdmacx00_cfg *)dev->config)->base	\
					+ OFFSET_TABLE(dev).transize + DMA_CH_OFFSET(ch))
#define DMA_CH_SRC_ADDR(dev, ch)	(((struct dma_atcdmacx00_cfg *)dev->config)->base	\
					+ OFFSET_TABLE(dev).srcaddr + DMA_CH_OFFSET(ch))
#define DMA_CH_SRC_ADDR_H(dev, ch)	(((struct dma_atcdmacx00_cfg *)dev->config)->base       \
					+ OFFSET_TABLE(dev).srcaddrh + DMA_CH_OFFSET(ch))
#define DMA_CH_DST_ADDR(dev, ch)	(((struct dma_atcdmacx00_cfg *)dev->config)->base       \
					+ OFFSET_TABLE(dev).dstaddr + DMA_CH_OFFSET(ch))
#define DMA_CH_DST_ADDR_H(dev, ch)	(((struct dma_atcdmacx00_cfg *)dev->config)->base       \
					+ OFFSET_TABLE(dev).dstaddrh + DMA_CH_OFFSET(ch))
#define DMA_CH_LL_PTR(dev, ch)		(((struct dma_atcdmacx00_cfg *)dev->config)->base       \
					+ OFFSET_TABLE(dev).llpointer + DMA_CH_OFFSET(ch))
#define DMA_CH_LL_PTR_H(dev, ch)	(((struct dma_atcdmacx00_cfg *)dev->config)->base       \
					+ OFFSET_TABLE(dev).llpointerh + DMA_CH_OFFSET(ch))

/* Source burst size options */
#define DMA_BSIZE_1		(0)
#define DMA_BSIZE_2		(1)
#define DMA_BSIZE_4		(2)
#define DMA_BSIZE_8		(3)
#define DMA_BSIZE_16		(4)
#define DMA_BSIZE_32		(5)
#define DMA_BSIZE_64		(6)
#define DMA_BSIZE_128		(7)
#define DMA_BSIZE_256		(8)
#define DMA_BSIZE_512		(9)
#define DMA_BSIZE_1024		(10)

/* Source/Destination transfer width options */
#define DMA_WIDTH_BYTE		(0)
#define DMA_WIDTH_HALFWORD	(1)
#define DMA_WIDTH_WORD		(2)
#define DMA_WIDTH_DWORD		(3)
#define DMA_WIDTH_QWORD		(4)
#define DMA_WIDTH_EWORD		(5)

/* Bus interface index */
#define DMA_INF_IDX0		(0)
#define DMA_INF_IDX1		(1)

/* DMA Channel Control Register Definition */
#define DMA_CH_CTRL_SBINF_MASK		BIT(31)
#define DMA_CH_CTRL_DBINF_MASK		BIT(30)
#define DMA_CH_CTRL_PRIORITY_HIGH	BIT(29)
#define DMA_CH_CTRL_SBSIZE_MASK		GENMASK(27, 24)
#define DMA_CH_CTRL_SBSIZE(n)		FIELD_PREP(DMA_CH_CTRL_SBSIZE_MASK, (n))
#define DMA_CH_CTRL_SWIDTH_MASK		GENMASK(23, 21)
#define DMA_CH_CTRL_SWIDTH(n)		FIELD_PREP(DMA_CH_CTRL_SWIDTH_MASK, (n))
#define DMA_CH_CTRL_DWIDTH_MASK		GENMASK(20, 18)
#define DMA_CH_CTRL_DWIDTH(n)		FIELD_PREP(DMA_CH_CTRL_DWIDTH_MASK, (n))
#define DMA_CH_CTRL_SMODE_HANDSHAKE	BIT(17)
#define DMA_CH_CTRL_DMODE_HANDSHAKE	BIT(16)
#define DMA_CH_CTRL_SRCADDRCTRL_MASK	GENMASK(15, 14)
#define DMA_CH_CTRL_SRCADDR_INC		FIELD_PREP(DMA_CH_CTRL_SRCADDRCTRL_MASK, (0))
#define DMA_CH_CTRL_SRCADDR_DEC		FIELD_PREP(DMA_CH_CTRL_SRCADDRCTRL_MASK, (1))
#define DMA_CH_CTRL_SRCADDR_FIX		FIELD_PREP(DMA_CH_CTRL_SRCADDRCTRL_MASK, (2))
#define DMA_CH_CTRL_DSTADDRCTRL_MASK	GENMASK(13, 12)
#define DMA_CH_CTRL_DSTADDR_INC		FIELD_PREP(DMA_CH_CTRL_DSTADDRCTRL_MASK, (0))
#define DMA_CH_CTRL_DSTADDR_DEC		FIELD_PREP(DMA_CH_CTRL_DSTADDRCTRL_MASK, (1))
#define DMA_CH_CTRL_DSTADDR_FIX		FIELD_PREP(DMA_CH_CTRL_DSTADDRCTRL_MASK, (2))
#define DMA_CH_CTRL_SRCREQ_MASK		GENMASK(11, 8)
#define DMA_CH_CTRL_SRCREQ(n)		FIELD_PREP(DMA_CH_CTRL_SRCREQ_MASK, (n))
#define DMA_CH_CTRL_DSTREQ_MASK		GENMASK(7, 4)
#define DMA_CH_CTRL_DSTREQ(n)		FIELD_PREP(DMA_CH_CTRL_DSTREQ_MASK, (n))
#define DMA_CH_CTRL_INTABT		BIT(3)
#define DMA_CH_CTRL_INTERR		BIT(2)
#define DMA_CH_CTRL_INTTC		BIT(1)
#define DMA_CH_CTRL_ENABLE		BIT(0)

/* DMA Interrupt Status Register Definition */
#define	DMA_INT_STATUS_TC_MASK		GENMASK(23, 16)
#define	DMA_INT_STATUS_ABORT_MASK	GENMASK(15, 8)
#define	DMA_INT_STATUS_ERROR_MASK	GENMASK(7, 0)
#define DMA_INT_STATUS_TC_VAL(x)	FIELD_GET(DMA_INT_STATUS_TC_MASK, (x))
#define DMA_INT_STATUS_ABORT_VAL(x)	FIELD_GET(DMA_INT_STATUS_ABORT_MASK, (x))
#define DMA_INT_STATUS_ERROR_VAL(x)	FIELD_GET(DMA_INT_STATUS_ERROR_MASK, (x))
#define DMA_INT_STATUS_CH_MSK(ch)	(0x111 << ch)

typedef void (*atcdmacx00_cfg_func_t)(void);

/*
 * The chain block is now an array to support multi-serial.
 * It accommodates various block layouts and is sized to the
 * largest possible size within the series.
 */
struct chain_block {
	uint32_t __aligned(64) array[8];
};

/* data for each DMA channel */
struct dma_chan_data {
	void *blkuser_data;
	dma_callback_t blkcallback;
	struct chain_block *head_block;
	struct dma_status status;
};

/*
 * The register offsets vary slightly across different series.
 * To handle this, using a struct to set the offset values on init time.
 */
struct offset_table {
	uint32_t ch_offset;
	uint32_t abort;
	uint32_t ctrl;
	uint32_t transize;
	uint32_t srcaddr;
	uint32_t srcaddrh;
	uint32_t dstaddr;
	uint32_t dstaddrh;
	uint32_t llpointer;
	uint32_t llpointerh;
};

/* Device run time data */
struct dma_atcdmacx00_data {
	struct dma_context dma_ctx;
	atomic_t channel_flags;
	struct dma_chan_data chan[ATCDMACx00_MAX_CHAN];
	uint32_t version;
	struct offset_table table;
	struct k_spinlock lock;
};

/* Device constant configuration parameters */
struct dma_atcdmacx00_cfg {
	atcdmacx00_cfg_func_t irq_config;
	uint32_t base;
	uint32_t irq_num;
};

#if CONFIG_NOCACHE_MEMORY
static struct chain_block __nocache __aligned(64)
	dma_chain[ATCDMACx00_MAX_CHAN][16];
#else /* CONFIG_NOCACHE_MEMORY */
static struct chain_block __aligned(64)
	dma_chain[ATCDMACx00_MAX_CHAN][16];
#endif /* CONFIG_NOCACHE_MEMORY */

static void dma_atcdmacx00_isr(const struct device *dev)
{
	uint32_t int_status, int_ch_status, channel;
	struct dma_atcdmacx00_data *const data = dev->data;
	struct dma_chan_data *ch_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	int_status = sys_read32(DMA_INT_STATUS(dev));
	/* Clear interrupt*/
	sys_write32(int_status, DMA_INT_STATUS(dev));

	k_spin_unlock(&data->lock, key);

	/* Handle terminal count status */
	int_ch_status = DMA_INT_STATUS_TC_VAL(int_status);
	while (int_ch_status) {
		channel = find_msb_set(int_ch_status) - 1;
		int_ch_status &= ~(BIT(channel));

		ch_data = &data->chan[channel];
		if (ch_data->blkcallback) {
			ch_data->blkcallback(dev, ch_data->blkuser_data, channel, 0);
		}
		data->chan[channel].status.busy = false;
	}

	/* Handle error status */
	int_ch_status = DMA_INT_STATUS_ERROR_VAL(int_status);
	while (int_ch_status) {
		channel = find_msb_set(int_ch_status) - 1;
		int_ch_status &= ~(BIT(channel));

		ch_data = &data->chan[channel];
		if (ch_data->blkcallback) {
			ch_data->blkcallback(dev, ch_data->blkuser_data, channel, -EIO);
		}
	}
}

static int dma_atcdmacx00_config(const struct device *dev, uint32_t channel,
				 struct dma_config *cfg)
{
	struct dma_atcdmacx00_data *const data = dev->data;
	uint32_t src_width, dst_width, src_burst_size, ch_ctrl, tfr_size;
	int32_t ret = 0;
	struct dma_block_config *cfg_blocks;
	k_spinlock_key_t key;

	if (channel >= ATCDMACx00_MAX_CHAN) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(cfg->source_data_size == cfg->dest_data_size);
	__ASSERT_NO_MSG(cfg->source_burst_length == cfg->dest_burst_length);

	if ((data->version == ATCDMAC100_VERSION &&
		cfg->source_data_size != 1 && cfg->source_data_size != 2 &&
		cfg->source_data_size != 4) ||
	    (cfg->source_data_size != 1 && cfg->source_data_size != 2 &&
		cfg->source_data_size != 4 && cfg->source_data_size != 8 &&
		cfg->source_data_size != 16 && cfg->source_data_size != 32)) {
		LOG_ERR("Invalid 'source_data_size' value");
		ret = -EINVAL;
		goto end;
	}

	cfg_blocks = cfg->head_block;
	if (cfg_blocks == NULL) {
		ret = -EINVAL;
		goto end;
	}

	tfr_size = cfg_blocks->block_size/cfg->source_data_size;
	if (tfr_size == 0) {
		ret = -EINVAL;
		goto end;
	}

	ch_ctrl = 0;

	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		break;
	case MEMORY_TO_PERIPHERAL:
		ch_ctrl |= DMA_CH_CTRL_DSTREQ(cfg->dma_slot);
		ch_ctrl |= DMA_CH_CTRL_DMODE_HANDSHAKE;
		break;
	case PERIPHERAL_TO_MEMORY:
		ch_ctrl |= DMA_CH_CTRL_SRCREQ(cfg->dma_slot);
		ch_ctrl |= DMA_CH_CTRL_SMODE_HANDSHAKE;
		break;
	default:
		ret = -EINVAL;
		goto end;
	}

	switch (cfg_blocks->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		ch_ctrl |= DMA_CH_CTRL_SRCADDR_INC;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		ch_ctrl |= DMA_CH_CTRL_SRCADDR_DEC;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		ch_ctrl |= DMA_CH_CTRL_SRCADDR_FIX;
		break;
	default:
		ret = -EINVAL;
		goto end;
	}

	switch (cfg_blocks->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		ch_ctrl |= DMA_CH_CTRL_DSTADDR_INC;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		ch_ctrl |= DMA_CH_CTRL_DSTADDR_DEC;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		ch_ctrl |= DMA_CH_CTRL_DSTADDR_FIX;
		break;
	default:
		ret = -EINVAL;
		goto end;
	}

	ch_ctrl |= DMA_CH_CTRL_INTABT;

	/* Disable the error callback */
	if (!cfg->error_callback_dis) {
		ch_ctrl |= DMA_CH_CTRL_INTERR;
	}

	src_width = find_msb_set(cfg->source_data_size) - 1;
	dst_width = find_msb_set(cfg->dest_data_size) - 1;
	src_burst_size = find_msb_set(cfg->source_burst_length) - 1;

	ch_ctrl |=  DMA_CH_CTRL_SWIDTH(src_width)	|
		    DMA_CH_CTRL_DWIDTH(dst_width)	|
		    DMA_CH_CTRL_SBSIZE(src_burst_size);


	/* Reset DMA channel configuration */
	sys_write32(0, DMA_CH_CTRL(dev, channel));

	key = k_spin_lock(&data->lock);
	/* Clear DMA interrupts status */
	sys_write32(DMA_INT_STATUS_CH_MSK(channel), DMA_INT_STATUS(dev));
	k_spin_unlock(&data->lock, key);

	/* Set transfer size */
	sys_write32(tfr_size, DMA_CH_TRANSIZE(dev, channel));

	/* Update the status of channel */
	data->chan[channel].status.dir = cfg->channel_direction;
	data->chan[channel].status.pending_length = cfg->source_data_size;

	/* Configure a callback appropriately depending on whether the
	 * interrupt is requested at the end of transaction completion or
	 * at the end of each block.
	 */
	data->chan[channel].blkcallback = cfg->dma_callback;
	data->chan[channel].blkuser_data = cfg->user_data;

	sys_write32(ch_ctrl, DMA_CH_CTRL(dev, channel));

	/* Set source and destination address */
	sys_write32((uint32_t)FIELD_GET(GENMASK(31, 0), (cfg_blocks->source_address)),
							DMA_CH_SRC_ADDR(dev, channel));
	sys_write32((uint32_t)FIELD_GET(GENMASK(31, 0), (cfg_blocks->dest_address)),
							DMA_CH_DST_ADDR(dev, channel));

	if (data->version == ATCDMAC300_VERSION) {
		sys_write32((uint32_t)FIELD_GET(GENMASK64(63, 32), (cfg_blocks->source_address)),
								DMA_CH_SRC_ADDR_H(dev, channel));
		sys_write32((uint32_t)FIELD_GET(GENMASK64(63, 32), (cfg_blocks->dest_address)),
								DMA_CH_DST_ADDR_H(dev, channel));
	}

	if ((cfg->block_count > 1) && cfg_blocks->next_block) {

		uint32_t current_block_idx = 0;

		sys_write32((uint32_t)FIELD_GET(GENMASK(31, 0),
					((long)&dma_chain[channel][current_block_idx])),
					DMA_CH_LL_PTR(dev, channel));

		if (data->version == ATCDMAC300_VERSION) {
			sys_write32((uint32_t)FIELD_GET(GENMASK64(63, 32),
						((long)&dma_chain[channel][current_block_idx])),
						DMA_CH_LL_PTR_H(dev, channel));
		}

		for (cfg_blocks = cfg_blocks->next_block; cfg_blocks != NULL;
				cfg_blocks = cfg_blocks->next_block) {

			ch_ctrl &= ~(DMA_CH_CTRL_SRCADDRCTRL_MASK |
					DMA_CH_CTRL_DSTADDRCTRL_MASK);

			switch (cfg_blocks->source_addr_adj) {
			case DMA_ADDR_ADJ_INCREMENT:
				ch_ctrl |= DMA_CH_CTRL_SRCADDR_INC;
				break;
			case DMA_ADDR_ADJ_DECREMENT:
				ch_ctrl |= DMA_CH_CTRL_SRCADDR_DEC;
				break;
			case DMA_ADDR_ADJ_NO_CHANGE:
				ch_ctrl |= DMA_CH_CTRL_SRCADDR_FIX;
				break;
			default:
				ret = -EINVAL;
				goto end;
			}

			switch (cfg_blocks->dest_addr_adj) {
			case DMA_ADDR_ADJ_INCREMENT:
				ch_ctrl |= DMA_CH_CTRL_DSTADDR_INC;
				break;
			case DMA_ADDR_ADJ_DECREMENT:
				ch_ctrl |= DMA_CH_CTRL_DSTADDR_DEC;
				break;
			case DMA_ADDR_ADJ_NO_CHANGE:
				ch_ctrl |= DMA_CH_CTRL_DSTADDR_FIX;
				break;
			default:
				ret = -EINVAL;
				goto end;
			}

			uint32_t index = 0;

			dma_chain[channel][current_block_idx].array[index++] = ch_ctrl;
			dma_chain[channel][current_block_idx].array[index++] =
						cfg_blocks->block_size/cfg->source_data_size;

			dma_chain[channel][current_block_idx].array[index++] =
				(uint32_t)FIELD_GET(GENMASK(31, 0), (cfg_blocks->source_address));
			if (data->version == ATCDMAC300_VERSION) {
				dma_chain[channel][current_block_idx].array[index++] =
							(uint32_t)FIELD_GET(GENMASK64(63, 32),
							(cfg_blocks->source_address));
			}

			dma_chain[channel][current_block_idx].array[index++] =
				(uint32_t)FIELD_GET(GENMASK(31, 0), (cfg_blocks->dest_address));
			if (data->version == ATCDMAC300_VERSION) {
				dma_chain[channel][current_block_idx].array[index++] =
							(uint32_t)FIELD_GET(GENMASK64(63, 32),
							(cfg_blocks->dest_address));
			}

			if (cfg_blocks->next_block) {
				dma_chain[channel][current_block_idx].array[index++] =
					(uint32_t)FIELD_GET(GENMASK(31, 0),
					((long)&dma_chain[channel][current_block_idx + 1]));
				dma_chain[channel][current_block_idx].array[index] =
					(uint32_t)FIELD_GET(GENMASK64(63, 32),
					((long)&dma_chain[channel][current_block_idx + 1]));

				current_block_idx = current_block_idx + 1;

			} else {
				dma_chain[channel][current_block_idx].array[index++] = 0x0;
				dma_chain[channel][current_block_idx].array[index] = 0x0;
			}
		}
	} else {
		/* Single transfer is supported, but Chain transfer is still
		 * not supported. Therefore, set LLPointer to zero
		 */
		sys_write32(0, DMA_CH_LL_PTR(dev, channel));
		if (data->version == ATCDMAC300_VERSION) {
			sys_write32(0, DMA_CH_LL_PTR_H(dev, channel));
		}
	}

#if defined(CONFIG_DCACHE) && defined(CONFIG_CACHE_MANAGEMENT) && !defined(CONFIG_NOCACHE_MEMORY)
	cache_data_flush_range((void *)&dma_chain, sizeof(dma_chain));
#elif defined(CONFIG_DCACHE) && !defined(CONFIG_CACHE_MANAGEMENT) && !defined(CONFIG_NOCACHE_MEMORY)
#error "Data cache is enabled; please flush the cache after \
	setting dma_chain to ensure memory coherence."
#endif

end:
	return ret;
}

#ifdef CONFIG_DMA_64BIT
static int dma_atcdmacx00_reload(const struct device *dev, uint32_t channel,
				 uint64_t src, uint64_t dst, size_t size)
#else
static int dma_atcdmacx00_reload(const struct device *dev, uint32_t channel,
				 uint32_t src, uint32_t dst, size_t size)
#endif
{
	struct dma_atcdmacx00_data *const data = dev->data;
	uint32_t src_width;

	if (channel >= ATCDMACx00_MAX_CHAN) {
		return -EINVAL;
	}

	/* Set source and destination address */
	sys_write32((uint32_t)FIELD_GET(GENMASK(31, 0), (src)),	DMA_CH_SRC_ADDR(dev, channel));
	sys_write32((uint32_t)FIELD_GET(GENMASK(31, 0), (dst)), DMA_CH_DST_ADDR(dev, channel));

	if (data->version == ATCDMAC300_VERSION) {
		sys_write32((uint32_t)FIELD_GET(GENMASK64(63, 32), (src)),
					DMA_CH_SRC_ADDR_H(dev, channel));
		sys_write32((uint32_t)FIELD_GET(GENMASK64(63, 32), (dst)),
					DMA_CH_DST_ADDR_H(dev, channel));
	}

	src_width = FIELD_GET(DMA_CH_CTRL_SWIDTH_MASK, sys_read32(DMA_CH_CTRL(dev, channel)));
	src_width = BIT(src_width);

	/* Set transfer size */
	sys_write32(size/src_width, DMA_CH_TRANSIZE(dev, channel));

	return 0;
}

static int dma_atcdmacx00_transfer_start(const struct device *dev,
					 uint32_t channel)
{
	struct dma_atcdmacx00_data *const data = dev->data;

	if (channel >= ATCDMACx00_MAX_CHAN) {
		return -EINVAL;
	}

	sys_write32(sys_read32(DMA_CH_CTRL(dev, channel)) | DMA_CH_CTRL_ENABLE,
						DMA_CH_CTRL(dev, channel));

	data->chan[channel].status.busy = true;

	return 0;
}

static int dma_atcdmacx00_transfer_stop(const struct device *dev,
					uint32_t channel)
{
	struct dma_atcdmacx00_data *const data = dev->data;
	k_spinlock_key_t key;

	if (channel >= ATCDMACx00_MAX_CHAN) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	sys_write32(BIT(channel), DMA_ABORT(dev));
	sys_write32(0, DMA_CH_CTRL(dev, channel));
	sys_write32(FIELD_GET(DMA_INT_STATUS_ABORT_MASK, (channel)), DMA_INT_STATUS(dev));
	data->chan[channel].status.busy = false;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int dma_atcdmacx00_init(const struct device *dev)
{
	struct dma_atcdmacx00_data *const data = dev->data;
	const struct dma_atcdmacx00_cfg *const config = (struct dma_atcdmacx00_cfg *)dev->config;
	uint32_t ch_num;

	if (FIELD_GET(GENMASK(31, 8), sys_read32(DMA_INT_IDREV(dev))) == ATCDMAC300_VERSION) {
		data->version = ATCDMAC300_VERSION;
		data->table.ch_offset = 0x20;
		data->table.abort = 0x24;
		data->table.ctrl = 0x40;
		data->table.transize = 0x44;
		data->table.srcaddr = 0x48;
		data->table.srcaddrh = 0x4c;
		data->table.dstaddr = 0x50;
		data->table.dstaddrh = 0x54;
		data->table.llpointer = 0x58;
		data->table.llpointerh = 0x5c;
	} else {
#ifndef CONFIG_DMA_64BIT
		data->version = ATCDMAC100_VERSION;
		data->table.ch_offset = 0x14;
		data->table.abort = 0x40;
		data->table.ctrl = 0x44;
		data->table.transize = 0x50;
		data->table.srcaddr = 0x48;
		data->table.dstaddr = 0x4c;
		data->table.llpointer = 0x54;
#else
		LOG_ERR("ATCDMAC100 doesn't support 64bit dma.\n");
#endif
	}

	data->channel_flags = ATOMIC_INIT(0);
	data->dma_ctx.atomic = &data->channel_flags;

	/* Disable all channels and Channel interrupts */
	for (ch_num = 0; ch_num < ATCDMACx00_MAX_CHAN; ch_num++) {
		sys_write32(0, DMA_CH_CTRL(dev, ch_num));
	}

	sys_write32(0xFFFFFF, DMA_INT_STATUS(dev));

	/* Configure interrupts */
	config->irq_config();

	irq_enable(config->irq_num);

	return 0;
}

static int dma_atcdmacx00_get_status(const struct device *dev,
				     uint32_t channel,
				     struct dma_status *stat)
{
	struct dma_atcdmacx00_data *const data = dev->data;

	stat->busy = data->chan[channel].status.busy;
	stat->dir = data->chan[channel].status.dir;
	stat->pending_length = data->chan[channel].status.pending_length;

	return 0;
}

static const struct dma_driver_api dma_atcdmacx00_api = {
	.config = dma_atcdmacx00_config,
	.reload = dma_atcdmacx00_reload,
	.start = dma_atcdmacx00_transfer_start,
	.stop = dma_atcdmacx00_transfer_stop,
	.get_status = dma_atcdmacx00_get_status
};

#define ATCDMACx00_INIT(n)						\
									\
	static struct dma_atcdmacx00_data dma_data_##n = {		\
		.dma_ctx.magic = DMA_MAGIC,				\
		.dma_ctx.dma_channels = DT_INST_PROP(n, dma_channels),	\
		.dma_ctx.atomic = ATOMIC_INIT(0),			\
	};								\
	static void dma_atcdmacx00_irq_config_##n(void);		\
	static const struct dma_atcdmacx00_cfg dma_config_##n = {	\
		.irq_config = dma_atcdmacx00_irq_config_##n,		\
		.base = DT_INST_REG_ADDR(n),				\
		.irq_num = DT_INST_IRQN(n),				\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
		dma_atcdmacx00_init,					\
		NULL,							\
		&dma_data_##n,						\
		&dma_config_##n,					\
		POST_KERNEL,						\
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		&dma_atcdmacx00_api);					\
									\
	static void dma_atcdmacx00_irq_config_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    1,						\
			    dma_atcdmacx00_isr,				\
			    DEVICE_DT_INST_GET(n),			\
			    0);						\
	}

DT_INST_FOREACH_STATUS_OKAY(ATCDMACx00_INIT)
