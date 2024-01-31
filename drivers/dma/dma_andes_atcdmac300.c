/*
 * Copyright (c) 2023 Andes Technology Corporation.
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

#define DT_DRV_COMPAT andestech_atcdmac300

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_andes_atcdmac300);

#define ATCDMAC100_MAX_CHAN	8

#define DMA_ABORT(dev)		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x24)
#define DMA_INT_STATUS(dev)		\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x30)

#define DMA_CH_OFFSET(ch)	(ch * 0x20)
#define DMA_CH_CTRL(dev, ch)		\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x40 + DMA_CH_OFFSET(ch))
#define DMA_CH_TRANSIZE(dev, ch)	\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x44 + DMA_CH_OFFSET(ch))
#define DMA_CH_SRC_ADDR_L(dev, ch)	\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x48 + DMA_CH_OFFSET(ch))
#define DMA_CH_SRC_ADDR_H(dev, ch)	\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x4C + DMA_CH_OFFSET(ch))
#define DMA_CH_DST_ADDR_L(dev, ch)	\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x50 + DMA_CH_OFFSET(ch))
#define DMA_CH_DST_ADDR_H(dev, ch)	\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x54 + DMA_CH_OFFSET(ch))
#define DMA_CH_LL_PTR_L(dev, ch)	\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x58 + DMA_CH_OFFSET(ch))
#define DMA_CH_LL_PTR_H(dev, ch)	\
		(((struct dma_atcdmac300_cfg *)dev->config)->base + 0x5C + DMA_CH_OFFSET(ch))

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
#define DMA_CH_CTRL_SBSIZE_MASK		BIT(24)
#define DMA_CH_CTRL_SBSIZE(n)		FIELD_PREP(DMA_CH_CTRL_SBSIZE_MASK, (n))
#define DMA_CH_CTRL_SWIDTH_MASK		GENMASK(23, 21)
#define DMA_CH_CTRL_SWIDTH(n)		FIELD_PREP(DMA_CH_CTRL_SWIDTH_MASK, (n))
#define DMA_CH_CTRL_DWIDTH_MASK		GENMASK(20, 18)
#define DMA_CH_CTRL_DWIDTH(n)		FIELD_PREP(DMA_CH_CTRL_DWIDTH_MASK, (n))
#define DMA_CH_CTRL_SMODE_HANDSHAKE	BIT(17)
#define DMA_CH_CTRL_DMODE_HANDSHAKE	BIT(17)
#define DMA_CH_CTRL_SRCADDRCTRL_MASK	GENMASK(15, 14)
#define DMA_CH_CTRL_SRCADDR_INC		FIELD_PREP(DMA_CH_CTRL_DWIDTH_MASK, (0))
#define DMA_CH_CTRL_SRCADDR_DEC		FIELD_PREP(DMA_CH_CTRL_DWIDTH_MASK, (1))
#define DMA_CH_CTRL_SRCADDR_FIX		FIELD_PREP(DMA_CH_CTRL_DWIDTH_MASK, (2))
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

typedef void (*atcdmac300_cfg_func_t)(void);

struct chain_block {
	uint32_t ctrl;
	uint32_t transize;
	uint32_t srcaddrl;
	uint32_t srcaddrh;
	uint32_t dstaddrl;
	uint32_t dstaddrh;
	uint32_t llpointerl;
	uint32_t llpointerh;
#if __riscv_xlen == 32
	uint32_t reserved;
#endif
	struct chain_block *next_block;
};

/* data for each DMA channel */
struct dma_chan_data {
	void *blkuser_data;
	dma_callback_t blkcallback;
	struct chain_block *head_block;
	struct dma_status status;
};

/* Device run time data */
struct dma_atcdmac300_data {
	struct dma_chan_data chan[ATCDMAC100_MAX_CHAN];
	struct k_spinlock lock;
};

/* Device constant configuration parameters */
struct dma_atcdmac300_cfg {
	atcdmac300_cfg_func_t irq_config;
	uint32_t base;
	uint32_t irq_num;
};

static struct __aligned(64)
	chain_block dma_chain[ATCDMAC100_MAX_CHAN][sizeof(struct chain_block) * 16];

static void dma_atcdmac300_isr(const struct device *dev)
{
	uint32_t int_status, int_ch_status, channel;
	struct dma_atcdmac300_data *const data = dev->data;
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

static int dma_atcdmac300_config(const struct device *dev, uint32_t channel,
				 struct dma_config *cfg)
{
	struct dma_atcdmac300_data *const data = dev->data;
	uint32_t src_width, dst_width, src_burst_size, ch_ctrl, tfr_size;
	int32_t ret = 0;
	struct dma_block_config *cfg_blocks;
	k_spinlock_key_t key;

	if (channel >= ATCDMAC100_MAX_CHAN) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(cfg->source_data_size == cfg->dest_data_size);
	__ASSERT_NO_MSG(cfg->source_burst_length == cfg->dest_burst_length);

	if (cfg->source_data_size != 1 && cfg->source_data_size != 2 &&
	    cfg->source_data_size != 4) {
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
	if (!cfg->error_callback_en) {
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
	sys_write32(cfg_blocks->source_address,
					DMA_CH_SRC_ADDR_L(dev, channel));
	sys_write32(0, DMA_CH_SRC_ADDR_H(dev, channel));
	sys_write32(cfg_blocks->dest_address,
					DMA_CH_DST_ADDR_L(dev, channel));
	sys_write32(0, DMA_CH_DST_ADDR_H(dev, channel));

	if (cfg->dest_chaining_en == 1 && cfg_blocks->next_block) {
		uint32_t current_block_idx = 0;

		sys_write32((uint32_t)((long)&dma_chain[channel][current_block_idx]),
							DMA_CH_LL_PTR_L(dev, channel));
		sys_write32(0, DMA_CH_LL_PTR_H(dev, channel));

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
			dma_chain[channel][current_block_idx].ctrl = ch_ctrl;
			dma_chain[channel][current_block_idx].transize =
						cfg_blocks->block_size/cfg->source_data_size;

			dma_chain[channel][current_block_idx].srcaddrl =
						(uint32_t)cfg_blocks->source_address;
			dma_chain[channel][current_block_idx].srcaddrh = 0x0;

			dma_chain[channel][current_block_idx].dstaddrl =
						(uint32_t)((long)cfg_blocks->dest_address);
			dma_chain[channel][current_block_idx].dstaddrh = 0x0;

			if (cfg_blocks->next_block) {
				dma_chain[channel][current_block_idx].llpointerl =
					(uint32_t)&dma_chain[channel][current_block_idx + 1];
				dma_chain[channel][current_block_idx].llpointerh = 0x0;

				current_block_idx = current_block_idx + 1;

			} else {
				dma_chain[channel][current_block_idx].llpointerl = 0x0;
				dma_chain[channel][current_block_idx].llpointerh = 0x0;
				dma_chain[channel][current_block_idx].next_block = NULL;
			}
		}
	} else {
		/* Single transfer is supported, but Chain transfer is still
		 * not supported. Therefore, set LLPointer to zero
		 */
		sys_write32(0, DMA_CH_LL_PTR_L(dev, channel));
		sys_write32(0, DMA_CH_LL_PTR_H(dev, channel));
	}

end:
	return ret;
}

static int dma_atcdmac300_reload(const struct device *dev, uint32_t channel,
				 uint32_t src, uint32_t dst, size_t size)
{
	uint32_t src_width;

	if (channel >= ATCDMAC100_MAX_CHAN) {
		return -EINVAL;
	}

	/* Set source and destination address */
	sys_write32(src, DMA_CH_SRC_ADDR_L(dev, channel));
	sys_write32(0, DMA_CH_SRC_ADDR_H(dev, channel));
	sys_write32(dst, DMA_CH_DST_ADDR_L(dev, channel));
	sys_write32(0, DMA_CH_DST_ADDR_H(dev, channel));

	src_width = FIELD_GET(DMA_CH_CTRL_SWIDTH_MASK, sys_read32(DMA_CH_CTRL(dev, channel)));
	src_width = BIT(src_width);

	/* Set transfer size */
	sys_write32(size/src_width, DMA_CH_TRANSIZE(dev, channel));

	return 0;
}

static int dma_atcdmac300_transfer_start(const struct device *dev,
					 uint32_t channel)
{
	struct dma_atcdmac300_data *const data = dev->data;

	if (channel >= ATCDMAC100_MAX_CHAN) {
		return -EINVAL;
	}

	sys_write32(sys_read32(DMA_CH_CTRL(dev, channel)) | DMA_CH_CTRL_ENABLE,
						DMA_CH_CTRL(dev, channel));

	data->chan[channel].status.busy = true;

	return 0;
}

static int dma_atcdmac300_transfer_stop(const struct device *dev,
					uint32_t channel)
{
	struct dma_atcdmac300_data *const data = dev->data;
	k_spinlock_key_t key;

	if (channel >= ATCDMAC100_MAX_CHAN) {
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

static int dma_atcdmac300_init(const struct device *dev)
{
	const struct dma_atcdmac300_cfg *const config = (struct dma_atcdmac300_cfg *)dev->config;
	uint32_t ch_num;

	/* Disable all channels and Channel interrupts */
	for (ch_num = 0; ch_num < ATCDMAC100_MAX_CHAN; ch_num++) {
		sys_write32(0, DMA_CH_CTRL(dev, ch_num));
	}

	sys_write32(0xFFFFFF, DMA_INT_STATUS(dev));

	/* Configure interrupts */
	config->irq_config();

	irq_enable(config->irq_num);

	return 0;
}

static int dma_atcdmac300_get_status(const struct device *dev,
				     uint32_t channel,
				     struct dma_status *stat)
{
	struct dma_atcdmac300_data *const data = dev->data;

	stat->busy = data->chan[channel].status.busy;
	stat->dir = data->chan[channel].status.dir;
	stat->pending_length = data->chan[channel].status.pending_length;

	return 0;
}

static const struct dma_driver_api dma_atcdmac300_api = {
	.config = dma_atcdmac300_config,
	.reload = dma_atcdmac300_reload,
	.start = dma_atcdmac300_transfer_start,
	.stop = dma_atcdmac300_transfer_stop,
	.get_status = dma_atcdmac300_get_status
};

#define ATCDMAC300_INIT(n)						\
									\
	static void dma_atcdmac300_irq_config_##n(void);		\
									\
	static const struct dma_atcdmac300_cfg dma_config_##n = {	\
		.irq_config = dma_atcdmac300_irq_config_##n,		\
		.base = DT_INST_REG_ADDR(n),				\
		.irq_num = DT_INST_IRQN(n),				\
	};								\
									\
	static struct dma_atcdmac300_data dma_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(0,					\
		dma_atcdmac300_init,					\
		NULL,							\
		&dma_data_##n,						\
		&dma_config_##n,					\
		POST_KERNEL,						\
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		&dma_atcdmac300_api);					\
									\
	static void dma_atcdmac300_irq_config_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    1,						\
			    dma_atcdmac300_isr,				\
			    DEVICE_DT_INST_GET(n),			\
			    0);						\
	}


DT_INST_FOREACH_STATUS_OKAY(ATCDMAC300_INIT)
