/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_dma_axi

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/cache.h>

LOG_MODULE_REGISTER(dma_designware_axi, CONFIG_DMA_LOG_LEVEL);

#define DEV_CFG(_dev)	((const struct dma_dw_axi_dev_cfg *)(_dev)->config)
#define DEV_DATA(_dev)		((struct dma_dw_axi_dev_data *const)(_dev)->data)

/* mask for block transfer size */
#define BLOCK_TS_MASK GENMASK(21, 0)

/* blen : number of data units
 * blen will always be in power of two
 *
 * when blen is 1 then set msize to zero otherwise find most significant bit set
 * and subtract two (as IP doesn't support number  of data items 2)
 */
#define DMA_DW_AXI_GET_MSIZE(blen) ((blen == 1) ? (0U) : (find_msb_set(blen) - 2U))

/* Common_Registers_Address_Block */
#define DMA_DW_AXI_IDREG                         0x0
#define DMA_DW_AXI_COMPVERREG                    0x08
#define DMA_DW_AXI_CFGREG                        0x10
#define DMA_DW_AXI_CHENREG                       0x18
#define DMA_DW_AXI_INTSTATUSREG                  0x30
#define DMA_DW_AXI_COMMONREG_INTCLEARREG         0x38
#define DMA_DW_AXI_COMMONREG_INTSTATUS_ENABLEREG 0x40
#define DMA_DW_AXI_COMMONREG_INTSIGNAL_ENABLEREG 0x48
#define DMA_DW_AXI_COMMONREG_INTSTATUSREG        0x50
#define DMA_DW_AXI_RESETREG                      0x58
#define DMA_DW_AXI_LOWPOWER_CFGREG               0x60

/* Channel enable by setting ch_en and ch_en_we */
#define CH_EN(chan)    (BIT64(8 + chan) | BIT64(chan))
/* Channel enable by setting ch_susp and ch_susp_we */
#define CH_SUSP(chan)  (BIT64(24 + chan) | BIT64(16 + chan))
/* Channel enable by setting ch_abort and ch_abort_we */
#define CH_ABORT(chan) (BIT64(40 + chan) | BIT64(32 + chan))

/* channel susp/resume write enable pos */
#define CH_RESUME_WE(chan) (BIT64(24 + chan))
/* channel resume bit pos */
#define CH_RESUME(chan)    (BIT64(16 + chan))

#define DMA_DW_AXI_CHAN_OFFSET(chan)             (0x100 * chan)

/* source address register for a channel */
#define DMA_DW_AXI_CH_SAR(chan)                  (0x100 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* destination address register for a channel */
#define DMA_DW_AXI_CH_DAR(chan)                  (0x108 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* block transfer size register for a channel */
#define DMA_DW_AXI_CH_BLOCK_TS(chan)             (0x110 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel control register */
#define DMA_DW_AXI_CH_CTL(chan)                  (0x118 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel configuration register */
#define DMA_DW_AXI_CH_CFG(chan)                  (0x120 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* linked list pointer register */
#define DMA_DW_AXI_CH_LLP(chan)                  (0x128 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel status register */
#define DMA_DW_AXI_CH_STATUSREG(chan)            (0x130 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel software handshake source register */
#define DMA_DW_AXI_CH_SWHSSRCREG(chan)           (0x138 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel software handshake destination register */
#define DMA_DW_AXI_CH_SWHSDSTREG(chan)           (0x140 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel block transfer resume request register */
#define DMA_DW_AXI_CH_BLK_TFR_RESUMEREQREG(chan) (0x148 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel AXI ID rester */
#define DMA_DW_AXI_CH_AXI_IDREG(chan)            (0x150 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel AXI QOS register */
#define DMA_DW_AXI_CH_AXI_QOSREG(chan)           (0x158 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel interrupt status enable register */
#define DMA_DW_AXI_CH_INTSTATUS_ENABLEREG(chan)  (0x180 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel interrupt status register */
#define DMA_DW_AXI_CH_INTSTATUS(chan)            (0x188 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel interrupt signal enable register */
#define DMA_DW_AXI_CH_INTSIGNAL_ENABLEREG(chan)  (0x190 + DMA_DW_AXI_CHAN_OFFSET(chan))
/* channel interrupt clear register */
#define DMA_DW_AXI_CH_INTCLEARREG(chan)          (0x198 + DMA_DW_AXI_CHAN_OFFSET(chan))

/* bitfield configuration for multi-block transfer */
#define DMA_DW_AXI_CFG_SRC_MULTBLK_TYPE(x)       FIELD_PREP(GENMASK64(1, 0), x)
#define DMA_DW_AXI_CFG_DST_MULTBLK_TYPE(x)       FIELD_PREP(GENMASK64(3, 2), x)

/* bitfield configuration to assign handshaking interface to source and destination */
#define DMA_DW_AXI_CFG_SRC_PER(x)                FIELD_PREP(GENMASK64(9, 4), x)
#define DMA_DW_AXI_CFG_DST_PER(x)                FIELD_PREP(GENMASK64(16, 11), x)

/* bitfield configuration for transfer type and flow controller */
#define DMA_DW_AXI_CFG_TT_FC(x)                  FIELD_PREP(GENMASK64(34, 32), x)

#define DMA_DW_AXI_CFG_HW_HS_SRC_BIT_POS         35
#define DMA_DW_AXI_CFG_HW_HS_DST_BIT_POS         36

#define DMA_DW_AXI_CFG_PRIORITY(x)               FIELD_PREP(GENMASK64(51, 47), x)

/* descriptor valid or not */
#define DMA_DW_AXI_CTL_LLI_VALID                 BIT64(63)
/* descriptor is last or not in a link */
#define DMA_DW_AXI_CTL_LLI_LAST                  BIT64(62)
/* interrupt on completion of block transfer */
#define DMA_DW_AXI_CTL_IOC_BLK_TFR               BIT64(58)
/* source status enable bit */
#define DMA_DW_AXI_CTL_SRC_STAT_EN               BIT64(56)
/* destination status enable bit */
#define DMA_DW_AXI_CTL_DST_STAT_EN               BIT64(57)
/* source burst length enable */
#define DMA_DW_AXI_CTL_ARLEN_EN                  BIT64(38)
/* source burst length(considered when corresponding enable bit is set) */
#define DMA_DW_AXI_CTL_ARLEN(x)                  FIELD_PREP(GENMASK64(46, 39), x)
/* destination burst length enable */
#define DMA_DW_AXI_CTL_AWLEN_EN                  BIT64(47)
/* destination burst length(considered when corresponding enable bit is set) */
#define DMA_DW_AXI_CTL_AWLEN(x)                  FIELD_PREP(GENMASK64(55, 48), x)

/* source burst transaction length */
#define DMA_DW_AXI_CTL_SRC_MSIZE(x)              FIELD_PREP(GENMASK64(17, 14), x)
/* destination burst transaction length */
#define DMA_DW_AXI_CTL_DST_MSIZE(x)              FIELD_PREP(GENMASK64(21, 18), x)
/* source transfer width */
#define DMA_DW_AXI_CTL_SRC_WIDTH(x)              FIELD_PREP(GENMASK64(10, 8), x)
/* destination transfer width */
#define DMA_DW_AXI_CTL_DST_WIDTH(x)              FIELD_PREP(GENMASK64(13, 11), x)

/* mask all the interrupts */
#define DMA_DW_AXI_IRQ_NONE                      0
/* enable block completion transfer interrupt */
#define DMA_DW_AXI_IRQ_BLOCK_TFR                 BIT64(0)
/* enable transfer completion interrupt */
#define DMA_DW_AXI_IRQ_DMA_TFR                   BIT64(1)
/* enable interrupts on any dma transfer error */
#define DMA_DW_AXI_IRQ_ALL_ERR                   (GENMASK64(14, 5) | GENMASK64(21, 16))

/* global enable bit for dma controller */
#define DMA_DW_AXI_CFG_EN                        BIT64(0)
/* global enable bit for interrupt */
#define DMA_DW_AXI_CFG_INT_EN                    BIT64(1)

/* descriptor used by dw axi dma controller*/
struct dma_lli {
	uint64_t sar;
	uint64_t dar;
	uint32_t block_ts_lo;
	uint32_t reserved;
	uint64_t llp;
	uint64_t ctl;
	uint32_t sstat;
	uint32_t dstat;
	uint64_t llp_status;
	uint64_t reserved1;
} __aligned(64);

/* status of the channel */
enum dma_dw_axi_ch_state {
	DMA_DW_AXI_CH_IDLE,
	DMA_DW_AXI_CH_SUSPENDED,
	DMA_DW_AXI_CH_ACTIVE,
	DMA_DW_AXI_CH_PREPARED,
};

/* source and destination transfer width */
enum dma_dw_axi_ch_width {
	BITS_8,
	BITS_16,
	BITS_32,
	BITS_64,
	BITS_128,
	BITS_256,
	BITS_512,
};

/* transfer direction and flow controller */
enum dma_dw_axi_tt_fc {
	M2M_DMAC,
	M2P_DMAC,
	P2M_DMAC,
	P2P_DMAC,
	P2M_SRC,
	P2P_SRC,
	M2P_DST,
	P2P_DST,
};

/* type of multi-block transfer */
enum dma_dw_axi_multi_blk_type {
	MULTI_BLK_CONTIGUOUS,
	MULTI_BLK_RELOAD,
	MULTI_BLK_SHADOW_REG,
	MULTI_BLK_LLI,
};

/* dma driver channel specific information */
struct dma_dw_axi_ch_data {
	/* lli descriptor base */
	struct dma_lli *lli_desc_base;
	/* lli current descriptor */
	struct dma_lli *lli_desc_current;
	/* dma channel state */
	enum dma_dw_axi_ch_state ch_state;
	/* direction of transfer */
	uint32_t direction;
	/* number of descriptors */
	uint32_t lli_desc_count;
	/* cfg register configuration for dma transfer */
	uint64_t cfg;
	/* mask and unmask interrupts */
	uint64_t irq_unmask;
	/* user call back for dma transfer completion */
	dma_callback_t dma_xfer_callback;
	/* user data for dma callback for dma transfer completion */
	void *priv_data_xfer;
	/* user call back for dma block transfer completion */
	dma_callback_t dma_blk_xfer_callback;
	/* user data for dma callback for dma block transfer completion */
	void *priv_data_blk_tfr;
};

/* dma controller driver data structure */
struct dma_dw_axi_dev_data {
	/* dma context */
	struct dma_context dma_ctx;

	/* mmio address mapping info for dma controller */
	DEVICE_MMIO_NAMED_RAM(dma_mmio);
	/* pointer to store channel specific info */
	struct dma_dw_axi_ch_data *chan;
	/* pointer to hold descriptor base address */
	struct dma_lli *dma_desc_pool;
};

/* Device constant configuration parameters */
struct dma_dw_axi_dev_cfg {
	/* dma address space to map */
	DEVICE_MMIO_NAMED_ROM(dma_mmio);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
	/* Reset controller device configurations */
	const struct reset_dt_spec reset;
#endif
	/* dma controller interrupt configuration function pointer */
	void (*irq_config)(void);
};

/**
 * @brief get current status of the channel
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param channel channel number
 *
 * @retval status of the channel
 */
static enum dma_dw_axi_ch_state dma_dw_axi_get_ch_status(const struct device *dev, uint32_t ch)
{
	uint32_t bit_status;
	uint64_t ch_status;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);

	ch_status = sys_read64(reg_base + DMA_DW_AXI_CHENREG);

	/* channel is active/busy in the dma transfer */
	bit_status = ((ch_status >> ch) & 1);
	if (bit_status) {
		return DMA_DW_AXI_CH_ACTIVE;
	}

	/* channel is currently suspended */
	bit_status = ((ch_status >> (16 + ch)) & 1);
	if (bit_status) {
		return DMA_DW_AXI_CH_SUSPENDED;
	}

	/* channel is idle */
	return DMA_DW_AXI_CH_IDLE;
}

static void dma_dw_axi_isr(const struct device *dev)
{
	unsigned int channel;
	uint64_t status, ch_status;
	int ret_status = 0;
	struct dma_dw_axi_ch_data *chan_data;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);

	/* read interrupt status register to find interrupt is for which channel */
	status = sys_read64(reg_base + DMA_DW_AXI_INTSTATUSREG);
	channel = find_lsb_set(status) - 1;
	if (channel < 0) {
		LOG_ERR("Spurious interrupt received channel:%u\n", channel);
		return;
	}

	if (channel > (dw_dev_data->dma_ctx.dma_channels - 1)) {
		LOG_ERR("Interrupt received on invalid channel:%d\n", channel);
		return;
	}

	/* retrieve channel specific data pointer for a channel */
	chan_data = &dw_dev_data->chan[channel];

	/* get dma transfer status */
	ch_status = sys_read64(reg_base + DMA_DW_AXI_CH_INTSTATUS(channel));
	if (!ch_status) {
		LOG_ERR("Spurious interrupt received ch_status:0x%llx\n", ch_status);
		return;
	}

	/* handle dma transfer errors if any */
	if (ch_status & DMA_DW_AXI_IRQ_ALL_ERR) {
		sys_write64(DMA_DW_AXI_IRQ_ALL_ERR,
			reg_base + DMA_DW_AXI_CH_INTCLEARREG(channel));
		LOG_ERR("DMA Error: Channel:%d Channel interrupt status:0x%llx\n",
				channel, ch_status);
		ret_status = -(ch_status & DMA_DW_AXI_IRQ_ALL_ERR);
	}

	/* handle block transfer completion */
	if (ch_status & DMA_DW_AXI_IRQ_BLOCK_TFR) {
		sys_write64(DMA_DW_AXI_IRQ_ALL_ERR | DMA_DW_AXI_IRQ_BLOCK_TFR,
				reg_base + DMA_DW_AXI_CH_INTCLEARREG(channel));

		if (chan_data->dma_blk_xfer_callback) {
			chan_data->dma_blk_xfer_callback(dev,
				chan_data->priv_data_blk_tfr, channel, ret_status);
		}
	}

	/* handle dma transfer completion */
	if (ch_status & DMA_DW_AXI_IRQ_DMA_TFR) {
		sys_write64(DMA_DW_AXI_IRQ_ALL_ERR | DMA_DW_AXI_IRQ_DMA_TFR,
				reg_base + DMA_DW_AXI_CH_INTCLEARREG(channel));

		if (chan_data->dma_xfer_callback) {
			chan_data->dma_xfer_callback(dev, chan_data->priv_data_xfer,
						channel, ret_status);
			chan_data->ch_state = dma_dw_axi_get_ch_status(dev, channel);
		}
	}
}

/**
 * @brief set data source and destination data width
 *
 * @param lli_desc Pointer to the descriptor
 * @param src_data_width source data width
 * @param dest_data_width destination data width
 *
 * @retval 0 on success, -ENOTSUP if the data width is not supported
 */
static int dma_dw_axi_set_data_width(struct dma_lli *lli_desc,
				uint32_t src_data_width, uint32_t dest_data_width)
{
	if (src_data_width > CONFIG_DMA_DW_AXI_DATA_WIDTH ||
			dest_data_width > CONFIG_DMA_DW_AXI_DATA_WIDTH) {
		LOG_ERR("transfer width more than %u not supported", CONFIG_DMA_DW_AXI_DATA_WIDTH);
		return -ENOTSUP;
	}

	switch (src_data_width) {
	case 1:
		/* one byte transfer */
		lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_WIDTH(BITS_8);
		break;
	case 2:
		/* 2-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_WIDTH(BITS_16);
		break;
	case 4:
		/* 4-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_WIDTH(BITS_32);
		break;
	case 8:
		/* 8-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_WIDTH(BITS_64);
		break;
	case 16:
		/* 16-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_WIDTH(BITS_128);
		break;
	case 32:
		/* 32-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_WIDTH(BITS_256);
		break;
	case 64:
		/* 64-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_WIDTH(BITS_512);
		break;
	default:
		LOG_ERR("Source transfer width not supported");
		return -ENOTSUP;
	}

	switch (dest_data_width) {
	case 1:
		/* one byte transfer */
		lli_desc->ctl |= DMA_DW_AXI_CTL_DST_WIDTH(BITS_8);
		break;
	case 2:
		/* 2-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_DST_WIDTH(BITS_16);
		break;
	case 4:
		/* 4-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_DST_WIDTH(BITS_32);
		break;
	case 8:
		/* 8-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_DST_WIDTH(BITS_64);
		break;
	case 16:
		/* 16-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_DST_WIDTH(BITS_128);
		break;
	case 32:
		/* 32-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_DST_WIDTH(BITS_256);
		break;
	case 64:
		/* 64-bytes transfer width */
		lli_desc->ctl |= DMA_DW_AXI_CTL_DST_WIDTH(BITS_512);
		break;
	default:
		LOG_ERR("Destination transfer width not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int dma_dw_axi_config(const struct device *dev, uint32_t channel,
						 struct dma_config *cfg)
{
	int ret;
	uint32_t msize_src, msize_dst, i, ch_state;
	struct dma_dw_axi_ch_data *chan_data;
	struct dma_block_config *blk_cfg;
	struct dma_lli *lli_desc;
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);

	/* check for invalid parameters before dereferencing them. */
	if (cfg == NULL) {
		LOG_ERR("invalid dma config :%p", cfg);
		return -ENODATA;
	}

	/* check if the channel is valid */
	if (channel > (dw_dev_data->dma_ctx.dma_channels - 1)) {
		LOG_ERR("invalid dma channel %d", channel);
		return -EINVAL;
	}

	/* return if the channel is not idle */
	ch_state = dma_dw_axi_get_ch_status(dev, channel);
	if (ch_state != DMA_DW_AXI_CH_IDLE) {
		LOG_ERR("DMA channel:%d is not idle(status:%d)", channel, ch_state);
		return -EBUSY;
	}

	if (!cfg->block_count) {
		LOG_ERR("no blocks to transfer");
		return -EINVAL;
	}

	/* descriptor should be less than max configured descriptor */
	if (cfg->block_count > CONFIG_DMA_DW_AXI_MAX_DESC) {
		LOG_ERR("dma:%s channel %d descriptor block count: %d larger than"
			" max descriptors in pool: %d", dev->name, channel,
			cfg->block_count, CONFIG_DMA_DW_AXI_MAX_DESC);
		return -EINVAL;
	}

	if (cfg->source_burst_length > CONFIG_DMA_DW_AXI_MAX_BURST_TXN_LEN ||
			cfg->dest_burst_length > CONFIG_DMA_DW_AXI_MAX_BURST_TXN_LEN ||
			cfg->source_burst_length == 0 || cfg->dest_burst_length == 0) {
		LOG_ERR("dma:%s burst length not supported", dev->name);
		return -ENOTSUP;
	}

	/* get channel specific data pointer */
	chan_data = &dw_dev_data->chan[channel];

	/* check if the channel is currently idle */
	if (chan_data->ch_state != DMA_DW_AXI_CH_IDLE) {
		LOG_ERR("DMA channel:%d is busy", channel);
		return -EBUSY;
	}

	/* burst transaction length for source and destination */
	msize_src = DMA_DW_AXI_GET_MSIZE(cfg->source_burst_length);
	msize_dst = DMA_DW_AXI_GET_MSIZE(cfg->dest_burst_length);

	chan_data->cfg = 0;
	chan_data->irq_unmask = 0;

	chan_data->direction = cfg->channel_direction;

	chan_data->lli_desc_base =
			&dw_dev_data->dma_desc_pool[channel * CONFIG_DMA_DW_AXI_MAX_DESC];
	chan_data->lli_desc_count = cfg->block_count;
	memset(chan_data->lli_desc_base, 0,
			sizeof(struct dma_lli) * chan_data->lli_desc_count);

	lli_desc = chan_data->lli_desc_base;
	blk_cfg = cfg->head_block;

	/* max channel priority can be MAX_CHANNEL - 1 */
	if (cfg->channel_priority < dw_dev_data->dma_ctx.dma_channels) {
		chan_data->cfg |= DMA_DW_AXI_CFG_PRIORITY(cfg->channel_priority);
	}

	/* configure all the descriptors in a loop */
	for (i = 0; i < cfg->block_count; i++) {

		ret = dma_dw_axi_set_data_width(lli_desc, cfg->source_data_size,
				cfg->dest_data_size);
		if (ret) {
			return ret;
		}

		lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_STAT_EN |
				DMA_DW_AXI_CTL_DST_STAT_EN | DMA_DW_AXI_CTL_IOC_BLK_TFR;

		lli_desc->sar = blk_cfg->source_address;
		lli_desc->dar = blk_cfg->dest_address;

		/* set block transfer size*/
		lli_desc->block_ts_lo = (blk_cfg->block_size / cfg->source_data_size) - 1;
		if (lli_desc->block_ts_lo > CONFIG_DMA_DW_AXI_MAX_BLOCK_TS) {
			LOG_ERR("block transfer size more than %u not supported",
				CONFIG_DMA_DW_AXI_MAX_BLOCK_TS);
			return -ENOTSUP;
		}

		/* configuration based on channel direction */
		if (cfg->channel_direction == MEMORY_TO_MEMORY) {
			chan_data->cfg |= DMA_DW_AXI_CFG_TT_FC(M2M_DMAC);

			lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_MSIZE(msize_src) |
					DMA_DW_AXI_CTL_DST_MSIZE(msize_dst);

		} else if (cfg->channel_direction == MEMORY_TO_PERIPHERAL) {

			chan_data->cfg |= DMA_DW_AXI_CFG_TT_FC(M2P_DMAC);
			lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_MSIZE(msize_src) |
					DMA_DW_AXI_CTL_DST_MSIZE(msize_dst);
			WRITE_BIT(chan_data->cfg, DMA_DW_AXI_CFG_HW_HS_DST_BIT_POS, 0);

			/* assign a hardware handshake interface */
			chan_data->cfg |= DMA_DW_AXI_CFG_DST_PER(cfg->dma_slot);

		} else if (cfg->channel_direction == PERIPHERAL_TO_MEMORY) {
			lli_desc->ctl |= DMA_DW_AXI_CTL_SRC_MSIZE(msize_src) |
					DMA_DW_AXI_CTL_DST_MSIZE(msize_dst);
			chan_data->cfg |= DMA_DW_AXI_CFG_TT_FC(P2M_DMAC);
			WRITE_BIT(chan_data->cfg, DMA_DW_AXI_CFG_HW_HS_SRC_BIT_POS, 0);

			/* assign a hardware handshake interface */
			chan_data->cfg |= DMA_DW_AXI_CFG_SRC_PER(cfg->dma_slot);

		} else {
			LOG_ERR("%s: dma %s channel %d invalid direction %d",
				__func__, dev->name, channel, cfg->channel_direction);

			return -EINVAL;
		}

		/* set pointer to the next descriptor */
		lli_desc->llp = ((uint64_t)(lli_desc + 1));

#if defined(CONFIG_DMA_DW_AXI_LLI_SUPPORT)
		/* configure multi block transfer size as linked list */
		chan_data->cfg |= DMA_DW_AXI_CFG_SRC_MULTBLK_TYPE(MULTI_BLK_LLI) |
				DMA_DW_AXI_CFG_DST_MULTBLK_TYPE(MULTI_BLK_LLI);

		lli_desc->ctl |= DMA_DW_AXI_CTL_LLI_VALID;
		/* last descriptor*/
		if ((i + 1) == chan_data->lli_desc_count) {
			lli_desc->ctl |= DMA_DW_AXI_CTL_LLI_LAST | DMA_DW_AXI_CTL_LLI_VALID;
			lli_desc->llp = 0;
		}
#else
		/* configure multi-block transfer as contiguous mode */
		chan_data->cfg |= DMA_DW_AXI_CFG_SRC_MULTBLK_TYPE(MULTI_BLK_CONTIGUOUS) |
				DMA_DW_AXI_CFG_DST_MULTBLK_TYPE(MULTI_BLK_CONTIGUOUS);
#endif

		/* next descriptor to configure*/
		lli_desc++;
		blk_cfg = blk_cfg->next_block;
	}

	arch_dcache_flush_range((void *)chan_data->lli_desc_base,
				sizeof(struct dma_lli) * cfg->block_count);

	chan_data->lli_desc_current = chan_data->lli_desc_base;

	/* enable an interrupt depending on whether the callback is requested after dma transfer
	 * completion or dma block transfer completion
	 *
	 * disable an interrupt if callback is not requested
	 */
	if (cfg->dma_callback && cfg->complete_callback_en) {
		chan_data->dma_blk_xfer_callback = cfg->dma_callback;
		chan_data->priv_data_blk_tfr = cfg->user_data;

		chan_data->irq_unmask = DMA_DW_AXI_IRQ_BLOCK_TFR | DMA_DW_AXI_IRQ_DMA_TFR;
	} else if (cfg->dma_callback && !cfg->complete_callback_en) {
		chan_data->dma_xfer_callback = cfg->dma_callback;
		chan_data->priv_data_xfer = cfg->user_data;

		chan_data->irq_unmask = DMA_DW_AXI_IRQ_DMA_TFR;
	} else {
		chan_data->irq_unmask = DMA_DW_AXI_IRQ_NONE;
	}

	/* unmask error interrupts when error_callback_dis is 0 */
	if (!cfg->error_callback_dis) {
		chan_data->irq_unmask |= DMA_DW_AXI_IRQ_ALL_ERR;
	}

	/* dma descriptors are configured, ready to start dma transfer */
	chan_data->ch_state = DMA_DW_AXI_CH_PREPARED;

	return 0;
}

static int dma_dw_axi_start(const struct device *dev, uint32_t channel)
{
	uint32_t ch_state;
	struct dma_dw_axi_ch_data *chan_data;
	struct dma_lli *lli_desc;
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);

	/* validate channel number */
	if (channel > (dw_dev_data->dma_ctx.dma_channels - 1)) {
		LOG_ERR("invalid dma channel %d", channel);
		return -EINVAL;
	}

	/* check whether channel is idle before initiating DMA transfer */
	ch_state = dma_dw_axi_get_ch_status(dev, channel);
	if (ch_state != DMA_DW_AXI_CH_IDLE) {
		LOG_ERR("DMA channel:%d is not idle", channel);
		return -EBUSY;
	}

	/* get channel specific data pointer */
	chan_data = &dw_dev_data->chan[channel];

	if (chan_data->ch_state != DMA_DW_AXI_CH_PREPARED) {
		LOG_ERR("DMA descriptors not configured");
		return -EINVAL;
	}

	/* enable dma controller and global interrupt bit */
	sys_write64(DMA_DW_AXI_CFG_INT_EN | DMA_DW_AXI_CFG_EN, reg_base + DMA_DW_AXI_CFGREG);

	sys_write64(chan_data->cfg, reg_base + DMA_DW_AXI_CH_CFG(channel));

	sys_write64(chan_data->irq_unmask,
				reg_base + DMA_DW_AXI_CH_INTSTATUS_ENABLEREG(channel));
	sys_write64(chan_data->irq_unmask,
				reg_base + DMA_DW_AXI_CH_INTSIGNAL_ENABLEREG(channel));

	lli_desc = chan_data->lli_desc_current;

#if defined(CONFIG_DMA_DW_AXI_LLI_SUPPORT)
	sys_write64(((uint64_t)lli_desc), reg_base + DMA_DW_AXI_CH_LLP(channel));
#else
	/* Program Source and Destination addresses */
	sys_write64(lli_desc->sar, reg_base + DMA_DW_AXI_CH_SAR(channel));
	sys_write64(lli_desc->dar, reg_base + DMA_DW_AXI_CH_DAR(channel));

	sys_write64(lli_desc->block_ts_lo & BLOCK_TS_MASK,
			reg_base + DMA_DW_AXI_CH_BLOCK_TS(channel));

	/* Program CH.CTL register */
	sys_write64(lli_desc->ctl, reg_base + DMA_DW_AXI_CH_CTL(channel));
#endif

	/* Enable the channel which will initiate DMA transfer */
	sys_write64(CH_EN(channel), reg_base + DMA_DW_AXI_CHENREG);

	chan_data->ch_state = dma_dw_axi_get_ch_status(dev, channel);

	return 0;
}

static int dma_dw_axi_stop(const struct device *dev, uint32_t channel)
{
	bool is_channel_busy;
	uint32_t ch_state;
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);

	/* channel should be valid */
	if (channel > (dw_dev_data->dma_ctx.dma_channels - 1)) {
		LOG_ERR("invalid dma channel %d", channel);
		return -EINVAL;
	}

	/* return if the channel is idle as there is nothing to stop */
	ch_state = dma_dw_axi_get_ch_status(dev, channel);
	if (ch_state == DMA_DW_AXI_CH_IDLE) {
		/* channel is already idle */
		return 0;
	}

	/* To stop transfer or abort the channel in case of abnormal state:
	 * 1. To disable channel, first suspend channel and drain the FIFO
	 * 2. Disable the channel. Channel may get hung and can't be disabled
	 * if there is no response from peripheral
	 * 3. If channel is not disabled, Abort the channel. Aborting channel will
	 * Flush out FIFO and data will be lost. Then corresponding interrupt will
	 * be raised for abort and CH_EN bit will be cleared from CHENREG register
	 */
	sys_write64(CH_SUSP(channel), reg_base + DMA_DW_AXI_CHENREG);

	/* Try to disable the channel */
	sys_clear_bit(reg_base + DMA_DW_AXI_CHENREG, channel);

	is_channel_busy = WAIT_FOR((sys_read64(reg_base + DMA_DW_AXI_CHENREG)) & (BIT(channel)),
						CONFIG_DMA_CHANNEL_STATUS_TIMEOUT, k_busy_wait(10));
	if (is_channel_busy) {
		LOG_WRN("No response from handshaking interface... Aborting a channel...");
		sys_write64(CH_ABORT(channel), reg_base + DMA_DW_AXI_CHENREG);

		is_channel_busy = WAIT_FOR((sys_read64(reg_base + DMA_DW_AXI_CHENREG)) &
				(BIT(channel)), CONFIG_DMA_CHANNEL_STATUS_TIMEOUT,
				k_busy_wait(10));
		if (is_channel_busy) {
			LOG_ERR("Channel abort failed");
			return -EBUSY;
		}
	}

	return 0;
}

static int dma_dw_axi_resume(const struct device *dev, uint32_t channel)
{
	uint32_t reg;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	uint32_t ch_state;

	/* channel should be valid */
	if (channel > (dw_dev_data->dma_ctx.dma_channels - 1)) {
		LOG_ERR("invalid dma channel %d", channel);
		return -EINVAL;
	}

	ch_state = dma_dw_axi_get_ch_status(dev, channel);
	if (ch_state != DMA_DW_AXI_CH_SUSPENDED) {
		LOG_INF("channel %u is not in suspended state so cannot resume channel", channel);
		return 0;
	}

	reg = sys_read64(reg_base + DMA_DW_AXI_CHENREG);
	/* channel susp write enable bit has to be asserted */
	WRITE_BIT(reg, CH_RESUME_WE(channel), 1);
	/* channel susp bit must be cleared to resume a channel*/
	WRITE_BIT(reg, CH_RESUME(channel), 0);
	/* resume a channel by writing 0: ch_susp and 1: ch_susp_we */
	sys_write64(reg, reg_base + DMA_DW_AXI_CHENREG);

	return 0;
}

/* suspend a dma channel */
static int dma_dw_axi_suspend(const struct device *dev, uint32_t channel)
{
	int ret;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	uint32_t ch_state;

	/* channel should be valid */
	if (channel > (dw_dev_data->dma_ctx.dma_channels - 1)) {
		LOG_ERR("invalid dma channel %u", channel);
		return -EINVAL;
	}

	ch_state = dma_dw_axi_get_ch_status(dev, channel);
	if (ch_state != DMA_DW_AXI_CH_ACTIVE) {
		LOG_INF("nothing to suspend as dma channel %u is not busy", channel);
		return 0;
	}

	/* suspend dma transfer */
	sys_write64(CH_SUSP(channel), reg_base + DMA_DW_AXI_CHENREG);

	ret = WAIT_FOR(dma_dw_axi_get_ch_status(dev, channel) &
			DMA_DW_AXI_CH_SUSPENDED, CONFIG_DMA_CHANNEL_STATUS_TIMEOUT,
			k_busy_wait(10));
	if (ret == 0) {
		LOG_ERR("channel suspend failed");
		return ret;
	}

	return 0;
}

static int dma_dw_axi_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, dma_mmio, K_MEM_CACHE_NONE);
	int i, ret;
	struct dma_dw_axi_ch_data *chan_data;
	const struct dma_dw_axi_dev_cfg *dw_dma_config = DEV_CFG(dev);
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)

	if (dw_dma_config->reset.dev != NULL) {
	/* check if reset manager is in ready state */
		if (!device_is_ready(dw_dma_config->reset.dev)) {
			LOG_ERR("reset controller device not found");
			return -ENODEV;
		}

		/* assert and de-assert dma controller */
		ret = reset_line_toggle(dw_dma_config->reset.dev, dw_dma_config->reset.id);
		if (ret != 0) {
			LOG_ERR("failed to reset dma controller");
			return ret;
		}
	}
#endif

	/* initialize channel state variable */
	for (i = 0; i < dw_dev_data->dma_ctx.dma_channels; i++) {
		chan_data = &dw_dev_data->chan[i];
		/* initialize channel state */
		chan_data->ch_state = DMA_DW_AXI_CH_IDLE;
	}

	/* configure and enable interrupt lines */
	dw_dma_config->irq_config();

	return 0;
}

static DEVICE_API(dma, dma_dw_axi_driver_api) = {
	.config = dma_dw_axi_config,
	.start = dma_dw_axi_start,
	.stop = dma_dw_axi_stop,
	.suspend = dma_dw_axi_suspend,
	.resume = dma_dw_axi_resume,
};

/* enable irq lines */
#define CONFIGURE_DMA_IRQ(idx, inst) \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, idx), ( \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, idx, irq), \
			DT_INST_IRQ_BY_IDX(inst, idx, priority), \
			dma_dw_axi_isr, \
			DEVICE_DT_INST_GET(inst), 0); \
			irq_enable(DT_INST_IRQ_BY_IDX(inst, idx, irq)); \
	))

#define DW_AXI_DMA_RESET_SPEC_INIT(inst) \
	.reset = RESET_DT_SPEC_INST_GET(inst), \

#define DW_AXI_DMAC_INIT(inst)								\
	static struct dma_dw_axi_ch_data chan_##inst[DT_INST_PROP(inst, dma_channels)];	\
	static struct dma_lli								\
		dma_desc_pool_##inst[DT_INST_PROP(inst, dma_channels) *			\
			CONFIG_DMA_DW_AXI_MAX_DESC];					\
	ATOMIC_DEFINE(dma_dw_axi_atomic##inst,						\
		      DT_INST_PROP(inst, dma_channels));				\
	static struct dma_dw_axi_dev_data dma_dw_axi_data_##inst = {			\
		.dma_ctx = {								\
			.magic = DMA_MAGIC,						\
			.atomic = dma_dw_axi_atomic##inst,				\
			.dma_channels = DT_INST_PROP(inst, dma_channels),		\
		},									\
		.chan = chan_##inst,							\
		.dma_desc_pool = dma_desc_pool_##inst,					\
		};									\
	static void dw_dma_irq_config_##inst(void);					\
	static const struct dma_dw_axi_dev_cfg dma_dw_axi_config_##inst = {		\
		DEVICE_MMIO_NAMED_ROM_INIT(dma_mmio, DT_DRV_INST(inst)),		\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, resets),				\
			(DW_AXI_DMA_RESET_SPEC_INIT(inst)))				\
		.irq_config = dw_dma_irq_config_##inst,					\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst,							\
				&dma_dw_axi_init,					\
				NULL,							\
				&dma_dw_axi_data_##inst,				\
				&dma_dw_axi_config_##inst, POST_KERNEL,			\
				CONFIG_DMA_INIT_PRIORITY,				\
				&dma_dw_axi_driver_api);				\
											\
	static void dw_dma_irq_config_##inst(void)					\
	{										\
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(inst)), CONFIGURE_DMA_IRQ, (), inst)	\
	}

DT_INST_FOREACH_STATUS_OKAY(DW_AXI_DMAC_INIT)
