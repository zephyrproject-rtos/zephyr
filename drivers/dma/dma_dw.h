/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DMA_DMA_DW_H_
#define ZEPHYR_DRIVERS_DMA_DMA_DW_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DW_CTLL_INT_EN		(1 << 0)
#define DW_CTLL_DST_WIDTH(x)	(x << 1)
#define DW_CTLL_SRC_WIDTH(x)	(x << 4)
#define DW_CTLL_DST_INC		(0 << 8)
#define DW_CTLL_DST_FIX		(1 << 8)
#define DW_CTLL_SRC_INC		(0 << 10)
#define DW_CTLL_SRC_FIX		(1 << 10)
#define DW_CTLL_DST_MSIZE(x)	(x << 11)
#define DW_CTLL_SRC_MSIZE(x)	(x << 14)
#define DW_CTLL_FC(x)		(x << 20)
#define DW_CTLL_FC_M2M		(0 << 20)
#define DW_CTLL_FC_M2P		(1 << 20)
#define DW_CTLL_FC_P2M		(2 << 20)
#define DW_CTLL_FC_P2P		(3 << 20)
#define DW_CTLL_LLP_D_EN	(1 << 27)
#define DW_CTLL_LLP_S_EN	(1 << 28)

/* data for each DMA channel */
struct dma_chan_data {
	uint32_t direction;
	void *blkuser_data;
	dma_callback_t dma_blkcallback;
	void *tfruser_data;
	dma_callback_t dma_tfrcallback;
};

#define DW_MAX_CHAN		8
#define DW_CH_SIZE		0x58
#define BYT_CHAN_OFFSET(chan)	(DW_CH_SIZE * chan)

#define DW_SAR(chan)	\
	(0x0000 + BYT_CHAN_OFFSET(chan))
#define DW_DAR(chan) \
	(0x0008 + BYT_CHAN_OFFSET(chan))
#define DW_LLP(chan) \
	(0x0010 + BYT_CHAN_OFFSET(chan))
#define DW_CTRL_LOW(chan) \
	(0x0018 + BYT_CHAN_OFFSET(chan))
#define DW_CTRL_HIGH(chan) \
	(0x001C + BYT_CHAN_OFFSET(chan))
#define DW_CFG_LOW(chan) \
	(0x0040 + BYT_CHAN_OFFSET(chan))
#define DW_CFG_HIGH(chan) \
	(0x0044 + BYT_CHAN_OFFSET(chan))

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

/* channel bits */
#define INT_MASK(chan)		(0x100 << chan)
#define INT_UNMASK(chan)	(0x101 << chan)
#define INT_MASK_ALL		0xFF00
#define INT_UNMASK_ALL		0xFFFF
#define CHAN_ENABLE(chan)	(0x101 << chan)
#define CHAN_DISABLE(chan)	(0x100 << chan)

/* TODO: add FIFO sizes */
struct chan_arbit_data {
	uint16_t class;
	uint16_t weight;
};

struct dw_drv_plat_data {
	struct chan_arbit_data chan[DW_MAX_CHAN];
};

/* Device run time data */
struct dw_dma_dev_data {
	struct dw_drv_plat_data *channel_data;
	struct dma_chan_data chan[DW_MAX_CHAN];
};

/* Device constant configuration parameters */
struct dw_dma_dev_cfg {
	uint32_t base;
	void (*irq_config)(void);
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_DMA_DMA_DW_H_ */
