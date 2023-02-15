/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DMA_DMA_DW_COMMON_H_
#define ZEPHYR_DRIVERS_DMA_DMA_DW_COMMON_H_

#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/dma.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MASK(b_hi, b_lo)					\
	(((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL) << (b_lo))
#define SET_BIT(b, x) (((x) & 1) << (b))
#define SET_BITS(b_hi, b_lo, x)	\
	(((x) & ((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL)) << (b_lo))

#define DW_MAX_CHAN		8
#define DW_CH_SIZE		0x58
#define DW_CHAN_OFFSET(chan)	(DW_CH_SIZE * chan)

#define DW_SAR(chan)	\
	(0x0000 + DW_CHAN_OFFSET(chan))
#define DW_DAR(chan) \
	(0x0008 + DW_CHAN_OFFSET(chan))
#define DW_LLP(chan) \
	(0x0010 + DW_CHAN_OFFSET(chan))
#define DW_CTRL_LOW(chan) \
	(0x0018 + DW_CHAN_OFFSET(chan))
#define DW_CTRL_HIGH(chan) \
	(0x001C + DW_CHAN_OFFSET(chan))
#define DW_CFG_LOW(chan) \
	(0x0040 + DW_CHAN_OFFSET(chan))
#define DW_CFG_HIGH(chan) \
	(0x0044 + DW_CHAN_OFFSET(chan))
#define DW_DSR(chan) \
	(0x0050 + DW_CHAN_OFFSET(chan))

/* registers */
#define DW_RAW_TFR		0x02C0
#define DW_RAW_BLOCK		0x02C8
#define DW_RAW_SRC_TRAN		0x02D0
#define DW_RAW_DST_TRAN		0x02D8
#define DW_RAW_ERR		0x02E0
#define DW_STATUS_TFR		0x02E8
#define DW_STATUS_BLOCK		0x02F0
#define DW_STATUS_SRC_TRAN	0x02F8
#define DW_STATUS_DST_TRAN	0x0300
#define DW_STATUS_ERR		0x0308
#define DW_MASK_TFR		0x0310
#define DW_MASK_BLOCK		0x0318
#define DW_MASK_SRC_TRAN	0x0320
#define DW_MASK_DST_TRAN	0x0328
#define DW_MASK_ERR		0x0330
#define DW_CLEAR_TFR		0x0338
#define DW_CLEAR_BLOCK		0x0340
#define DW_CLEAR_SRC_TRAN	0x0348
#define DW_CLEAR_DST_TRAN	0x0350
#define DW_CLEAR_ERR		0x0358
#define DW_INTR_STATUS		0x0360
#define DW_DMA_CFG		0x0398
#define DW_DMA_CHAN_EN		0x03A0
#define DW_FIFO_PART0_LO	0x400
#define DW_FIFO_PART0_HI	0x404
#define DW_FIFO_PART1_LO	0x408
#define DW_FIFO_PART1_HI	0x40C

/* channel bits */
#define DW_CHAN_WRITE_EN_ALL	MASK(2 * DW_MAX_CHAN - 1, DW_MAX_CHAN)
#define DW_CHAN_WRITE_EN(chan)	BIT((chan) + DW_MAX_CHAN)
#define DW_CHAN_ALL		MASK(DW_MAX_CHAN - 1, 0)
#define DW_CHAN(chan)		BIT(chan)
#define DW_CHAN_MASK_ALL	DW_CHAN_WRITE_EN_ALL
#define DW_CHAN_MASK(chan)	DW_CHAN_WRITE_EN(chan)
#define DW_CHAN_UNMASK_ALL	(DW_CHAN_WRITE_EN_ALL | DW_CHAN_ALL)
#define DW_CHAN_UNMASK(chan)	(DW_CHAN_WRITE_EN(chan) | DW_CHAN(chan))

/* CFG_LO */
#define DW_CFGL_RELOAD_DST	BIT(31)
#define DW_CFGL_RELOAD_SRC	BIT(30)
#define DW_CFGL_DRAIN		BIT(10) /* For Intel GPDMA variant only */
#define DW_CFGL_SRC_SW_HS       BIT(10) /* For Synopsys variant only */
#define DW_CFGL_DST_SW_HS       BIT(11) /* For Synopsys variant only */
#define DW_CFGL_FIFO_EMPTY	BIT(9)
#define DW_CFGL_SUSPEND		BIT(8)
#define DW_CFGL_CTL_HI_UPD_EN	BIT(5)

/* CFG_HI */
#define DW_CFGH_DST_PER_EXT(x)		SET_BITS(31, 30, x)
#define DW_CFGH_SRC_PER_EXT(x)		SET_BITS(29, 28, x)
#define DW_CFGH_DST_PER(x)		SET_BITS(7, 4, x)
#define DW_CFGH_SRC_PER(x)		SET_BITS(3, 0, x)
#define DW_CFGH_DST(x) \
	(DW_CFGH_DST_PER_EXT((x) >> 4) | DW_CFGH_DST_PER(x))
#define DW_CFGH_SRC(x) \
	(DW_CFGH_SRC_PER_EXT((x) >> 4) | DW_CFGH_SRC_PER(x))

/* CTL_LO */
#define DW_CTLL_RELOAD_DST	BIT(31)
#define DW_CTLL_RELOAD_SRC	BIT(30)
#define DW_CTLL_LLP_S_EN	BIT(28)
#define DW_CTLL_LLP_D_EN	BIT(27)
#define DW_CTLL_SMS(x)		SET_BIT(25, x)
#define DW_CTLL_DMS(x)		SET_BIT(23, x)
#define DW_CTLL_FC_P2P		SET_BITS(21, 20, 3)
#define DW_CTLL_FC_P2M		SET_BITS(21, 20, 2)
#define DW_CTLL_FC_M2P		SET_BITS(21, 20, 1)
#define DW_CTLL_FC_M2M		SET_BITS(21, 20, 0)
#define DW_CTLL_D_SCAT_EN	BIT(18)
#define DW_CTLL_S_GATH_EN	BIT(17)
#define DW_CTLL_SRC_MSIZE(x)	SET_BITS(16, 14, x)
#define DW_CTLL_DST_MSIZE(x)	SET_BITS(13, 11, x)
#define DW_CTLL_SRC_FIX		SET_BITS(10, 9, 2)
#define DW_CTLL_SRC_DEC		SET_BITS(10, 9, 1)
#define DW_CTLL_SRC_INC		SET_BITS(10, 9, 0)
#define DW_CTLL_DST_FIX		SET_BITS(8, 7, 2)
#define DW_CTLL_DST_DEC		SET_BITS(8, 7, 1)
#define DW_CTLL_DST_INC		SET_BITS(8, 7, 0)
#define DW_CTLL_SRC_WIDTH(x)	SET_BITS(6, 4, x)
#define DW_CTLL_DST_WIDTH(x)	SET_BITS(3, 1, x)
#define DW_CTLL_INT_EN		BIT(0)
#define DW_CTLL_SRC_WIDTH_MASK	MASK(6, 4)
#define DW_CTLL_SRC_WIDTH_SHIFT	4
#define DW_CTLL_DST_WIDTH_MASK	MASK(3, 1)
#define DW_CTLL_DST_WIDTH_SHIFT	1

/* CTL_HI */
#define DW_CTLH_CLASS(x)	SET_BITS(31, 29, x)
#define DW_CTLH_WEIGHT(x)	SET_BITS(28, 18, x)
#define DW_CTLH_DONE(x)		SET_BIT(17, x)
#define DW_CTLH_BLOCK_TS_MASK	MASK(16, 0)

/* DSR */
#define DW_DSR_DSC(x)		SET_BITS(31, 20, x)
#define DW_DSR_DSI(x)		SET_BITS(19, 0, x)

/* FIFO_PART */
#define DW_FIFO_SIZE 0x80
#define DW_FIFO_UPD		BIT(26)
#define DW_FIFO_CHx(x)		SET_BITS(25, 13, x)
#define DW_FIFO_CHy(x)		SET_BITS(12, 0, x)

/* number of tries to wait for reset */
#define DW_DMA_CFG_TRIES	10000

/* channel drain timeout in microseconds */
#define DW_DMA_TIMEOUT	1333

/* min number of elems for config with irq disabled */
#define DW_DMA_CFG_NO_IRQ_MIN_ELEMS	3

#define DW_DMA_CHANNEL_REGISTER_OFFSET_END	0x50
#define DW_DMA_IP_REGISTER_OFFSET_END		0x418
#define DW_DMA_IP_REGISTER_OFFSET_START	0x2C0

/* linked list item address */
#define DW_DMA_LLI_ADDRESS(lli, dir) \
	(((dir) == MEMORY_TO_PERIPHERAL) ? ((lli)->sar) : ((lli)->dar))

/* TODO: add FIFO sizes */
struct dw_chan_arbit_data {
	uint16_t class;
	uint16_t weight;
};

struct dw_drv_plat_data {
	struct dw_chan_arbit_data chan[DW_MAX_CHAN];
};

/* DMA descriptor used by HW */
struct dw_lli {
	uint32_t sar;
	uint32_t dar;
	uint32_t llp;
	uint32_t ctrl_lo;
	uint32_t ctrl_hi;
	uint32_t sstat;
	uint32_t dstat;

	/* align to 32 bytes to not cross cache line
	 * in case of more than two items
	 */
	uint32_t reserved;
} __packed;

/* pointer data for DW DMA buffer */
struct dw_dma_ptr_data {
	uint32_t current_ptr;
	uint32_t start_ptr;
	uint32_t end_ptr;
	uint32_t hw_ptr;
	uint32_t buffer_bytes;
};

/* State tracking for each channel */
enum dw_dma_state {
	DW_DMA_IDLE,
	DW_DMA_PREPARED,
	DW_DMA_SUSPENDED,
	DW_DMA_ACTIVE,
};

/* data for each DMA channel */
struct dw_dma_chan_data {
	uint32_t direction;
	enum dw_dma_state state;
	struct dw_lli *lli; /* allocated array of LLI's */
	uint32_t lli_count; /* number of lli's in the allocation */
	struct dw_lli *lli_current; /* current LLI being used */
	uint32_t cfg_lo;
	uint32_t cfg_hi;
	struct dw_dma_ptr_data ptr_data;	/* pointer data */
	dma_callback_t dma_blkcallback;
	void *blkuser_data;
	dma_callback_t dma_tfrcallback;
	void *tfruser_data;
};

/* use array to get burst_elems for specific slot number setting.
 * the relation between msize and burst_elems should be
 * 2 ^ msize = burst_elems
 */
static const uint32_t burst_elems[] = {1, 2, 4, 8};

/* Device run time data */
struct dw_dma_dev_data {
	struct dma_context dma_ctx;
	struct dw_drv_plat_data *channel_data;
	struct dw_dma_chan_data chan[DW_MAX_CHAN];
	struct dw_lli lli_pool[DW_MAX_CHAN][CONFIG_DMA_DW_LLI_POOL_SIZE] __aligned(64);

	ATOMIC_DEFINE(channels_atomic, DW_MAX_CHAN);
};

/* Device constant configuration parameters */
struct dw_dma_dev_cfg {
	uint32_t base;
	void (*irq_config)(void);
};

static ALWAYS_INLINE void dw_write(uint32_t dma_base, uint32_t reg, uint32_t value)
{
	*((volatile uint32_t *)(dma_base + reg)) = value;
}

static ALWAYS_INLINE uint32_t dw_read(uint32_t dma_base, uint32_t reg)
{
	return *((volatile uint32_t *)(dma_base + reg));
}

int dw_dma_setup(const struct device *dev);

int dw_dma_config(const struct device *dev, uint32_t channel,
		  struct dma_config *cfg);

int dw_dma_reload(const struct device *dev, uint32_t channel,
		  uint32_t src, uint32_t dst, size_t size);

int dw_dma_start(const struct device *dev, uint32_t channel);

int dw_dma_stop(const struct device *dev, uint32_t channel);

int dw_dma_suspend(const struct device *dev, uint32_t channel);

int dw_dma_resume(const struct device *dev, uint32_t channel);

void dw_dma_isr(const struct device *dev);

int dw_dma_get_status(const struct device *dev, uint32_t channel,
		      struct dma_status *stat);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_DMA_DMA_DW_COMMON_H_ */
