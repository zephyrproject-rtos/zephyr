/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_dma_axi

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dma_designware_axi, CONFIG_DMA_LOG_LEVEL);

#define DEV_CFG(_dev)	((const struct dma_dw_axi_dev_cfg *)(_dev)->config)
#define DEV_DATA(_dev)		((struct dma_dw_axi_dev_data *const)(_dev)->data)

#define BLOCK_TS_MASK GENMASK(21, 0)
#define MAX_BLOCK_TS 32767U

/* Common_Registers_Address_Block */
#define	DMAC_IDREG 0x0
#define DMAC_COMPVERREG 0x08
#define DMAC_CFGREG 0x10
#define DMAC_CHENREG 0x18
#define DMAC_INTSTATUSREG 0x30
#define DMAC_COMMONREG_INTCLEARREG 0x38
#define DMAC_COMMONREG_INTSTATUS_ENABLEREG 0x40
#define DMAC_COMMONREG_INTSIGNAL_ENABLEREG 0x48
#define DMAC_COMMONREG_INTSTATUSREG 0x50
#define DMAC_RESETREG 0x58
#define DMAC_LOWPOWER_CFGREG 0x60

/* Channel enable by setting ch_en and ch_en_we */
#define CH_EN(chan) (BIT(8 + (chan - 1)) | BIT(chan - 1))
/* Channel enable by setting ch_susp and ch_susp_we */
#define CH_SUSP(chan) (BIT(24 + (chan - 1)) | BIT(16 + (chan - 1)))
/* Channel enable by setting ch_abort and ch_abort_we */
#define CH_ABORT(chan) (BIT(40 + (chan - 1)) | BIT(32 + (chan - 1)))

#define DW_CHAN_OFFSET(chan) (0x100 * (chan - 1))

/* Channel_Registers_Address_Block */
#define DMAC_CH_SAR(chan)                  (0x100 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_DAR(chan)                  (0x108 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_BLOCK_TS(chan)             (0x110 + DW_CHAN_OFFSET(chan))

#define DMAC_CH_CTL(chan)                  (0x118 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_CTLL(chan)                 (0x118 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_CTLH(chan)                 (0x11C + DW_CHAN_OFFSET(chan))

#define DMAC_CH_CFGL(chan)                 (0x120 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_CFGH(chan)                 (0x124 + DW_CHAN_OFFSET(chan))

#define DMAC_CH_LLP(chan)                  (0x128 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_STATUSREG(chan)            (0x130 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_SWHSSRCREG(chan)           (0x138 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_SWHSDSTREG(chan)           (0x140 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_BLK_TFR_RESUMEREQREG(chan) (0x148 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_AXI_IDREG(chan)            (0x150 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_AXI_QOSREG(chan)           (0x158 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_INTSTATUS_ENABLEREG(chan)  (0x180 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_INTSTATUS(chan)            (0x188 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_INTSIGNAL_ENABLEREG(chan)  (0x190 + DW_CHAN_OFFSET(chan))
#define DMAC_CH_INTCLEARREG(chan)          (0x198 + DW_CHAN_OFFSET(chan))

/* DW CH CFG LO */
#define DW_CFGL_SRC_MULTBLK_TYPE(x)      FIELD_PREP(GENMASK(1, 0), x)
#define DW_CFGL_DST_MULTBLK_TYPE(x)      FIELD_PREP(GENMASK(3, 2), x)

#define DW_CFGL_SRC_PER(x)               FIELD_PREP(GENMASK(9, 4), x)
#define DW_CFGL_DST_PER(x)               FIELD_PREP(GENMASK(16, 11), x)

/* DW CH CFG HI */
#define DW_CFGH_TT_FC(x)                 FIELD_PREP(GENMASK(2, 0), x)

#define DW_CFGH_HW_HS_SRC_BIT_POS        3
#define DW_CFGH_HW_HS_DST_BIT_POS        4

#define DW_CFGH_PRIORITY(x)              FIELD_PREP(GENMASK(19, 17), x)

/* DW CH CTL HI */
#define DW_CTLH_LLI_VALID                BIT(31)
#define DW_CTLH_LLI_LAST                 BIT(30)
#define DW_CTLH_IOC_BLK_TFR              BIT(26)
#define DW_CTLH_SRC_STAT_EN              BIT(24)
#define DW_CTLH_DST_STAT_EN              BIT(25)
#define DW_CTLH_ARLEN_EN                 BIT(6)
#define DW_CTLH_ARLEN(x)                 FIELD_PREP(GENMASK(14, 7), x)
#define DW_CTLH_AWLEN_EN                 BIT(15)
#define DW_CTLH_AWLEN(x)                 FIELD_PREP(GENMASK(23, 16), x)

/* DW CH CTL LO */
#define DW_CTLL_SRC_MSIZE(x)             FIELD_PREP(GENMASK(17, 14), x)
#define DW_CTLL_DST_MSIZE(x)             FIELD_PREP(GENMASK(21, 18), x)
#define DW_CTLL_SRC_WIDTH(x)             FIELD_PREP(GENMASK(10, 8), x)
#define DW_CTLL_DST_WIDTH(x)             FIELD_PREP(GENMASK(13, 11), x)

/* DW CH INT STATUS EN */
#define DW_IRQ_NONE                     0
#define DW_IRQ_BLOCK_TFR                BIT(0)
#define DW_IRQ_DMA_TFR                  BIT(1)
#define DW_IRQ_ALL_ERR                  (GENMASK(14, 5) | GENMASK(21, 16))

/* DMAC CFG REG */
#define DMAC_CFG_EN                     BIT(0)
#define DMAC_CFG_INT_EN                 BIT(1)

/* DW AXI DMA descriptor used by DMA controller*/
struct dma_lli {
	uint64_t sar;
	uint64_t dar;
	uint32_t block_ts_lo;
	uint32_t reserved;
	uint64_t llp;
	uint32_t ctl_lo;
	uint32_t ctl_hi;
	uint32_t sstat;
	uint32_t dstat;
	uint64_t llp_status;
	uint64_t reserved1;
} __aligned(64);

enum dma_dw_axi_channel_state {
	DW_AXI_DMA_IDLE,
	DW_AXI_DMA_SUSPENDED,
	DW_AXI_DMA_ACTIVE,
};

enum dma_dw_axi_ch_width {
	BITS_8,
	BITS_16,
	BITS_32,
	BITS_64,
	BITS_128,
	BITS_256,
	BITS_512,
};

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

enum dma_dw_axi_multi_blk_type {
	MULTI_BLK_CONTIGUOUS,
	MULTI_BLK_RELOAD,
	MULTI_BLK_SHADOW_REG,
	MULTI_BLK_LLI,
};

struct dma_dw_axi_ch_data {
	struct dma_lli *lli_desc_base;
	struct dma_lli *lli_desc_current;
	uint32_t cfg_lo;
	uint32_t cfg_hi;
	uint32_t direction;
	uint32_t lli_desc_count;
	uint64_t irq_unmask;
	dma_callback_t dma_xfer_callback;
	void *priv_data_xfer;
	dma_callback_t dma_blk_xfer_callback;
	void *priv_data_blk_tfr;
};

#if defined(CONFIG_DMA_BOTTOM_HALF_WORK_QUEUE)
struct dma_work {
	const struct device *dev;
	struct k_work work;
	uint32_t channel;
	int status;
};
#endif

struct dma_dw_axi_dev_data {
	struct dma_context dma_ctx;

	DEVICE_MMIO_NAMED_RAM(dma_mmio);
#if defined(CONFIG_DMA_BOTTOM_HALF_WORK_QUEUE)
	struct dma_work *work;
#endif
	struct dma_dw_axi_ch_data *chan;
	struct dma_lli *dma_desc_pool;
};

/* Device constant configuration parameters */
struct dma_dw_axi_dev_cfg {
	DEVICE_MMIO_NAMED_ROM(dma_mmio);
	const struct reset_dt_spec reset;
	void (*irq_config)(void);
	uint32_t max_channel;
};

#if defined(CONFIG_DMA_BOTTOM_HALF_WORK_QUEUE)
static void dma_schedule_irq_work(struct k_work *item)
{
	uint32_t channel = 0;
	struct dma_work *dmawork = CONTAINER_OF(item, struct dma_work, work);
	const struct device *dev = dmawork->dev;
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	struct dma_dw_axi_ch_data *chan_data = NULL;
	int status = 0;

	channel = dmawork->channel;
	chan_data = &dw_dev_data->chan[channel - 1];
	status = dmawork->status;

	if (chan_data->dma_blk_xfer_callback) {
		chan_data->dma_blk_xfer_callback(dev,
				chan_data->priv_data_blk_tfr, channel, status);
	} else {
		chan_data->dma_xfer_callback(dev, chan_data->priv_data_xfer,
						channel, status);
	}
}
#endif

static uint32_t dma_dw_axi_get_ch_status(const struct device *dev, uint32_t channel)
{
	uint64_t ch_status = 0;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	uint32_t bitstatus = 0;

	ch_status = sys_read64(reg_base + DMAC_CHENREG);

	bitstatus = ((ch_status >> (channel - 1)) & 1);
	if (bitstatus) {
		return DW_AXI_DMA_ACTIVE;
	}

	bitstatus = ((ch_status >> (16 + channel - 1)) & 1);
	if (bitstatus) {
		return DW_AXI_DMA_SUSPENDED;
	}

	return DW_AXI_DMA_IDLE;
}

static void dma_dw_axi_isr(const struct device *dev)
{
	uint32_t channel = 0;
	uint32_t status = 0, ch_status = 0;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	const struct dma_dw_axi_dev_cfg *dw_dma_config = DEV_CFG(dev);
	struct dma_dw_axi_ch_data *chan_data = NULL;
	int ret_status = 0;

	status = sys_read64(reg_base + DMAC_INTSTATUSREG);
	channel = find_lsb_set(status);
	if (channel > dw_dma_config->max_channel) {
		LOG_ERR("Interrupt received on invalid channel:%d\n", channel);
		return;
	}

	chan_data = &dw_dev_data->chan[channel - 1];

	ch_status = sys_read64(reg_base + DMAC_CH_INTSTATUS(channel));

	if (ch_status & DW_IRQ_ALL_ERR) {
		sys_write64(DW_IRQ_ALL_ERR,	reg_base + DMAC_CH_INTCLEARREG(channel));
		LOG_ERR("DMA Error: Channel:%d Channel interrupt status:0x%x\n",
				channel, ch_status);
		ret_status = -1;
	}

	if (ch_status & DW_IRQ_BLOCK_TFR) {
		sys_write64(DW_IRQ_ALL_ERR | DW_IRQ_BLOCK_TFR,
				reg_base + DMAC_CH_INTCLEARREG(channel));

		if (chan_data->dma_blk_xfer_callback) {
#if defined(CONFIG_DMA_BOTTOM_HALF_WORK_QUEUE)
			dw_dev_data->work[channel - 1].status = ret_status;
			k_work_submit(&dw_dev_data->work[channel - 1].work);
#else
			chan_data->dma_blk_xfer_callback(dev,
				chan_data->priv_data_blk_tfr, channel, ret_status);
#endif
		}
	}

	if (ch_status & DW_IRQ_DMA_TFR) {
		sys_write64(DW_IRQ_ALL_ERR | DW_IRQ_DMA_TFR,
				reg_base + DMAC_CH_INTCLEARREG(channel));

		if (chan_data->dma_xfer_callback) {
#if defined(CONFIG_DMA_BOTTOM_HALF_WORK_QUEUE)
			dw_dev_data->work[channel - 1].status = ret_status;
			k_work_submit(&dw_dev_data->work[channel - 1].work);
#else
			chan_data->dma_xfer_callback(dev, chan_data->priv_data_xfer,
						channel, ret_status);
#endif
		}
	}
}

static int dma_dw_axi_config(const struct device *dev, uint32_t channel,
						 struct dma_config *cfg)
{
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	const struct dma_dw_axi_dev_cfg *dw_dma_config = DEV_CFG(dev);
	struct dma_dw_axi_ch_data *chan_data = NULL;
	struct dma_block_config *blk_cfg = NULL;
	struct dma_lli *lli_desc = NULL;
	uint32_t msize_src = 0;
	uint32_t msize_dst = 0;
	uint32_t i = 0, ch_state = 0;

	/* Check for invalid parameters before dereferencing them. */
	if (!device_is_ready(dev) || !cfg) {
		LOG_ERR("Invalid DMA controller:%p state or dma config :%p", dev, cfg);
		return -ENODEV;
	}

	if (channel > dw_dma_config->max_channel) {
		LOG_ERR("%s: Invalid dma channel %d", __func__, channel);
		return -EINVAL;
	}

	ch_state = dma_dw_axi_get_ch_status(dev, channel);
	if (ch_state != DW_AXI_DMA_IDLE) {
		LOG_ERR("DMA channel:%d is not idle", channel);
		return -EBUSY;
	}

	if (cfg->block_count > CONFIG_DMA_DW_AXI_MAX_DESC) {
		LOG_ERR("dma:%s channel %d descriptor block count: %d larger than"
			" max descriptors in pool: %d", dev->name, channel,
			cfg->block_count, CONFIG_DMA_DW_AXI_MAX_DESC);
		return -EINVAL;
	}

	chan_data = &dw_dev_data->chan[channel - 1];

	chan_data->cfg_lo = 0;
	chan_data->cfg_hi = 0;
	chan_data->irq_unmask = 0;

	msize_src = find_msb_set(cfg->source_burst_length) - 1;
	msize_dst = find_msb_set(cfg->dest_burst_length) - 1;

	chan_data->direction = cfg->channel_direction;

	chan_data->lli_desc_base =
			&dw_dev_data->dma_desc_pool[(channel - 1) * CONFIG_DMA_DW_AXI_MAX_DESC];
	chan_data->lli_desc_count = cfg->block_count;
	memset(chan_data->lli_desc_base, 0,
			sizeof(struct dma_lli) * chan_data->lli_desc_count);

	lli_desc = chan_data->lli_desc_base;
	blk_cfg = cfg->head_block;

	/* Max channel priority can be MAX_CHANNEL - 1 */
	if (cfg->channel_priority < dw_dma_config->max_channel) {
		chan_data->cfg_hi |= DW_CFGH_PRIORITY(cfg->channel_priority);
	}

	for (i = 0; i < cfg->block_count; i++) {

		switch (cfg->source_data_size) {
		case 1: /* Indicates one byte transfer */
			/* one byte transfer at a time */
			lli_desc->ctl_lo |= DW_CTLL_SRC_WIDTH(BITS_8);
			break;
		case 2:  /* Indicates two byte transfer width */
			lli_desc->ctl_lo |= DW_CTLL_SRC_WIDTH(BITS_16);
			break;
		case 4:
			lli_desc->ctl_lo |= DW_CTLL_SRC_WIDTH(BITS_32);
			break;
		case 8:
			lli_desc->ctl_lo |= DW_CTLL_SRC_WIDTH(BITS_64);
			break;
		case 16:
			lli_desc->ctl_lo |= DW_CTLL_SRC_WIDTH(BITS_128);
			break;
		case 32:
			lli_desc->ctl_lo |= DW_CTLL_SRC_WIDTH(BITS_256);
			break;
		case 64:
			lli_desc->ctl_lo |= DW_CTLL_SRC_WIDTH(BITS_512);
			break;
		default:
			LOG_ERR("Source transfer width not supported");
			return -ENOTSUP;
		}

		switch (cfg->dest_data_size) {
		case 1: /* Indicates one byte transfer */
			/* one byte transfer at a time */
			lli_desc->ctl_lo |= DW_CTLL_DST_WIDTH(BITS_8);
			break;
		case 2:  /* 2-bytes transfer width */
			lli_desc->ctl_lo |= DW_CTLL_DST_WIDTH(BITS_16);
			break;
		case 4: /* 4-bytes transfer width */
			lli_desc->ctl_lo |= DW_CTLL_DST_WIDTH(BITS_32);
			break;
		case 8: /* 8-bytes transfer width */
			lli_desc->ctl_lo |= DW_CTLL_DST_WIDTH(BITS_64);
			break;
		case 16: /* 16-bytes transfer width */
			lli_desc->ctl_lo |= DW_CTLL_DST_WIDTH(BITS_128);
			break;
		case 32: /* 32-bytes transfer width */
			lli_desc->ctl_lo |= DW_CTLL_DST_WIDTH(BITS_256);
			break;
		case 64: /* 64-bytes transfer width */
			lli_desc->ctl_lo |= DW_CTLL_DST_WIDTH(BITS_512);
			break;
		default:
			LOG_ERR("Destination transfer width not supported");
			return -ENOTSUP;
		}

		lli_desc->ctl_hi |= DW_CTLH_SRC_STAT_EN |
				DW_CTLH_DST_STAT_EN | DW_CTLH_IOC_BLK_TFR;

		lli_desc->sar = blk_cfg->source_address;
		lli_desc->dar = blk_cfg->dest_address;

		/* Set block transfer size*/
		lli_desc->block_ts_lo = blk_cfg->block_size / cfg->source_data_size;
		if (lli_desc->block_ts_lo > MAX_BLOCK_TS) {
			LOG_ERR("DMAC does not support Block items more than %d", MAX_BLOCK_TS);
			return -EINVAL;
		}

		if (cfg->channel_direction == MEMORY_TO_MEMORY) {
			chan_data->cfg_hi |= DW_CFGH_TT_FC(M2M_DMAC);

			lli_desc->ctl_lo |= DW_CTLL_SRC_MSIZE(msize_src) |
					DW_CTLL_DST_MSIZE(msize_dst);

		} else if (cfg->channel_direction == MEMORY_TO_PERIPHERAL) {

			chan_data->cfg_hi |= DW_CFGH_TT_FC(M2P_DMAC);
			lli_desc->ctl_lo |= DW_CTLL_SRC_MSIZE(msize_src) |
					DW_CTLL_DST_MSIZE(msize_dst);
			WRITE_BIT(chan_data->cfg_hi, DW_CFGH_HW_HS_DST_BIT_POS, 0);

			/* Assign a hardware handshake interface */
			chan_data->cfg_lo |= DW_CFGL_DST_PER(cfg->dma_slot);

		} else if (cfg->channel_direction == PERIPHERAL_TO_MEMORY) {
			lli_desc->ctl_lo |= DW_CTLL_SRC_MSIZE(msize_src) |
					DW_CTLL_DST_MSIZE(msize_dst);
			chan_data->cfg_hi |= DW_CFGH_TT_FC(P2M_DMAC);
			WRITE_BIT(chan_data->cfg_hi, DW_CFGH_HW_HS_SRC_BIT_POS, 0);

			/* Assign a hardware handshake interface */
			chan_data->cfg_lo |= DW_CFGL_SRC_PER(cfg->dma_slot);

		} else {
			LOG_ERR("%s: dma %s channel %d invalid direction %d",
				__func__, dev->name, channel, cfg->channel_direction);

			return -EINVAL;
		}

		/* Set next descriptor in the list */
		lli_desc->llp = ((uint64_t)(lli_desc + 1));

#if defined(CONFIG_DMA_DW_AXI_LLI_SUPPORT)
		chan_data->cfg_lo |= DW_CFGL_SRC_MULTBLK_TYPE(MULTI_BLK_LLI) |
				DW_CFGL_DST_MULTBLK_TYPE(MULTI_BLK_LLI);

		lli_desc->ctl_hi |= DW_CTLH_LLI_VALID;
		if ((i + 1) == chan_data->lli_desc_count) {	/* last descriptor*/
			lli_desc->ctl_hi |= DW_CTLH_LLI_LAST | DW_CTLH_LLI_VALID;
			lli_desc->llp = 0;
		}
#else
		chan_data->cfg_lo |= DW_CFGL_SRC_MULTBLK_TYPE(MULTI_BLK_CONTIGUOUS) |
				DW_CFGL_DST_MULTBLK_TYPE(MULTI_BLK_CONTIGUOUS);
#endif

		/* Next descriptor to fill */
		lli_desc++;
		blk_cfg = blk_cfg->next_block;
	}

	chan_data->lli_desc_current = chan_data->lli_desc_base;

	/* Always enable error interrupts to capture errors while DMA transfer
	 * Enable DMA block completion interrupt or DMA transfer completion interrupt
	 * based on whether user registered for callback or not
	 */
	if (cfg->dma_callback && cfg->complete_callback_en) {
		chan_data->dma_blk_xfer_callback = cfg->dma_callback;
		chan_data->priv_data_blk_tfr = cfg->user_data;

		chan_data->irq_unmask = DW_IRQ_ALL_ERR | DW_IRQ_BLOCK_TFR | DW_IRQ_DMA_TFR;
	} else if (cfg->dma_callback && !cfg->complete_callback_en) {
		chan_data->dma_xfer_callback = cfg->dma_callback;
		chan_data->priv_data_xfer = cfg->user_data;

		chan_data->irq_unmask = DW_IRQ_ALL_ERR | DW_IRQ_DMA_TFR;
	} else {
		chan_data->irq_unmask = DW_IRQ_ALL_ERR;
	}

	return 0;
}

static int dma_dw_axi_start(const struct device *dev, uint32_t channel)
{
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	const struct dma_dw_axi_dev_cfg *dw_dma_config = DEV_CFG(dev);
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	struct dma_dw_axi_ch_data *chan_data = NULL;
	struct dma_lli *lli_desc = NULL;
	uint32_t ch_state = 0;

	/* DMA driver should be initialized with ready state */
	if (!device_is_ready(dev)) {
		LOG_ERR("%s: Device:%p is not ready", __func__, dev);
		return -ENODEV;
	}

	/* Validate channel number */
	if (channel > dw_dma_config->max_channel) {
		LOG_ERR("%s: Invalid dma channel %d", __func__, channel);
		return -EINVAL;
	}

	/* Check whether channel is idle before initiating DMA transfer */
	ch_state = dma_dw_axi_get_ch_status(dev, channel);
	if (ch_state != DW_AXI_DMA_IDLE) {
		LOG_ERR("DMA channel:%d is not idle", channel);
		return -EBUSY;
	}

	sys_write64(DMAC_CFG_INT_EN | DMAC_CFG_EN, reg_base + DMAC_CFGREG);

	chan_data = &dw_dev_data->chan[channel - 1];

	if (!chan_data->lli_desc_base) {
		LOG_ERR("dma %s channel %d invalid stream", dev->name, channel);
		return -EINVAL;
	}

	sys_write32(chan_data->cfg_lo, reg_base + DMAC_CH_CFGL(channel));
	sys_write32(chan_data->cfg_hi, reg_base + DMAC_CH_CFGH(channel));

	sys_write64(chan_data->irq_unmask,
				reg_base + DMAC_CH_INTSTATUS_ENABLEREG(channel));
	sys_write64(chan_data->irq_unmask,
				reg_base + DMAC_CH_INTSIGNAL_ENABLEREG(channel));

	lli_desc = chan_data->lli_desc_current;

#if defined(CONFIG_DMA_DW_AXI_LLI_SUPPORT)
	sys_write64(((uint64_t)lli_desc), reg_base + DMAC_CH_LLP(channel));
#else
	/* Program Source and Destination addresses */
	sys_write64(lli_desc->sar, reg_base + DMAC_CH_SAR(channel));
	sys_write64(lli_desc->dar, reg_base + DMAC_CH_DAR(channel));

	sys_write32(lli_desc->block_ts_lo & BLOCK_TS_MASK, reg_base + DMAC_CH_BLOCK_TS(channel));

	/* Program CH.CTL_lo CH.CTL_hi register */
	sys_write32(lli_desc->ctl_hi, reg_base + DMAC_CH_CTLH(channel));
	sys_write32(lli_desc->ctl_lo, reg_base + DMAC_CH_CTLL(channel));
#endif

	/* Enable the channel which will initiate DMA transfer */
	sys_write64(CH_EN(channel), reg_base + DMAC_CHENREG);

	return 0;
}

static int dma_dw_axi_stop(const struct device *dev, uint32_t channel)
{
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);
	const struct dma_dw_axi_dev_cfg *dw_dma_config = DEV_CFG(dev);
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	bool is_channel_free = true;
	struct dma_dw_axi_ch_data *chan_data = NULL;
	uint32_t ch_state = 0;

	/*TODO: dma_stop has not been tested*/

	chan_data = &dw_dev_data->chan[channel - 1];

	if (!device_is_ready(dev)) {
		LOG_ERR("%s: Device:%p is not ready", __func__, dev);
		return -ENODEV;
	}

	if (channel > dw_dma_config->max_channel) {
		LOG_ERR("%s: Invalid dma channel %d", __func__, channel);
		return -EINVAL;
	}

	ch_state = dma_dw_axi_get_ch_status(dev, channel);
	if (ch_state == DW_AXI_DMA_IDLE) {
		LOG_ERR("Channel:%d is already idle", channel);
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
	sys_write64(CH_SUSP(channel), reg_base + DMAC_CHENREG);

	sys_write64(0, reg_base + DMAC_CHENREG);

	is_channel_free = WAIT_FOR((!sys_read64(reg_base + DMAC_CHENREG)) & (BIT(channel-1)),
						CONFIG_DMA_CHANNEL_STATUS_TIMEOUT, k_busy_wait(10));
	if (!is_channel_free) {
		LOG_ERR("No response from handshaking interface... Aborting a channel...");
		sys_write64(CH_ABORT(channel), reg_base + DMAC_CHENREG);

		is_channel_free = WAIT_FOR((!sys_read64(reg_base + DMAC_CHENREG)) &
				(BIT(channel-1)), CONFIG_DMA_CHANNEL_STATUS_TIMEOUT,
				k_busy_wait(10));
		LOG_ERR("is channel free:%d", is_channel_free);
	}

	return 0;
}

static int dma_dw_axi_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, dma_mmio, K_MEM_CACHE_NONE);
	const struct dma_dw_axi_dev_cfg *dw_dma_config = DEV_CFG(dev);

	if (!device_is_ready(dw_dma_config->reset.dev)) {
		LOG_ERR("%s: %s is not initialized", __func__, dw_dma_config->reset.dev->name);
		return -ENODEV;
	}

	reset_line_assert(dw_dma_config->reset.dev, dw_dma_config->reset.id);
	reset_line_deassert(dw_dma_config->reset.dev, dw_dma_config->reset.id);

#if defined(CONFIG_DMA_BOTTOM_HALF_WORK_QUEUE)
	int i = 0;
	struct dma_dw_axi_dev_data *const dw_dev_data = DEV_DATA(dev);

	for (i = 0; i < dw_dma_config->max_channel; i++) {
		dw_dev_data->work[i].dev = dev;
		k_work_init(&dw_dev_data->work[i].work, dma_schedule_irq_work);
		dw_dev_data->work[i].channel = i + 1;
	}
#endif

	dw_dma_config->irq_config();

	return 0;
}

static const struct dma_driver_api dma_dw_axi_driver_api = {
	.config = dma_dw_axi_config,
	.start = dma_dw_axi_start,
	.stop = dma_dw_axi_stop,
};

#define CONFIGURE_DMA_IRQ(idx, inst) \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, idx), ( \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, idx, irq), \
			DT_INST_IRQ_BY_IDX(inst, idx, priority), \
			dma_dw_axi_isr, \
			DEVICE_DT_INST_GET(inst), 0); \
			irq_enable(DT_INST_IRQ_BY_IDX(inst, idx, irq)); \
	))

#define DW_AXI_DMAC_INIT(inst) \
	static struct dma_dw_axi_ch_data chan_##inst[DT_INST_PROP(inst, dma_channels)]; \
	IF_ENABLED(CONFIG_DMA_BOTTOM_HALF_WORK_QUEUE, \
		(static struct dma_work work_##inst[DT_INST_PROP(inst, dma_channels)];)) \
	static struct dma_lli \
		dma_desc_pool_##inst[DT_INST_PROP(inst, dma_channels) * \
			CONFIG_DMA_DW_AXI_MAX_DESC]; \
	static struct dma_dw_axi_dev_data dma_dw_axi_data_##inst = { \
		.chan = chan_##inst, \
		IF_ENABLED(CONFIG_DMA_BOTTOM_HALF_WORK_QUEUE, \
		(.work = work_##inst,)) \
		.dma_desc_pool = dma_desc_pool_##inst, \
		}; \
	static void dw_dma_irq_config_##inst(void); \
	static const struct dma_dw_axi_dev_cfg dma_dw_axi_config_##inst = { \
		DEVICE_MMIO_NAMED_ROM_INIT(dma_mmio, DT_DRV_INST(inst)), \
		.reset = RESET_DT_SPEC_INST_GET(inst), \
		.irq_config = dw_dma_irq_config_##inst, \
		.max_channel = DT_INST_PROP(inst, dma_channels), \
	}; \
	\
	DEVICE_DT_INST_DEFINE(inst, \
				&dma_dw_axi_init, \
				NULL, \
				&dma_dw_axi_data_##inst, \
				&dma_dw_axi_config_##inst, POST_KERNEL, \
				CONFIG_DMA_INIT_PRIORITY, \
				&dma_dw_axi_driver_api); \
				\
	static void dw_dma_irq_config_##inst(void) \
	{ \
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(inst)), CONFIGURE_DMA_IRQ, (), inst) \
	}

DT_INST_FOREACH_STATUS_OKAY(DW_AXI_DMAC_INIT)
