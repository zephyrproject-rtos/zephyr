/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_AMD_ACP_7_0_ACP70_CHIP_REG_H_
#define ZEPHYR_SOC_AMD_ACP_7_0_ACP70_CHIP_REG_H_

#include "acp70_chip_offsets.h"
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dai.h>

typedef union acp_dma_cntl_0 {
	struct {
		unsigned int dmachrst: 1;
		unsigned int dmachrun: 1;
		unsigned int dmachiocen: 1;
		unsigned int: 29;
	} bits;
	unsigned int u32all;
} acp_dma_cntl_0_t;

typedef union acp_dma_ch_sts {
	struct {
		unsigned int dmachrunsts: 8;
		unsigned int: 24;
	} bits;
	unsigned int u32all;
} acp_dma_ch_sts_t;

typedef union acp_dsp_sw_intr_cntl {
	struct {
		unsigned int: 2;
		unsigned int dsp0_to_host_intr_mask: 1;
		unsigned int: 29;
	} bits;
	unsigned int u32all;
} acp_dsp_sw_intr_cntl_t;

typedef union acp_external_intr_enb {
	struct {
		unsigned int acpextintrenb: 1;
		unsigned int: 31;
	} bits;
	unsigned int u32all;
} acp_external_intr_enb_t;

struct sdw_pin_data {
	uint32_t pin_num;
	uint32_t pin_dir;
	uint32_t dma_channel;
	uint32_t index;
	uint32_t instance;
	uint32_t acp_sdw_index;
};

struct acp_dma_ptr_data {
	/* base address of dma  buffer */
	uint32_t base;
	/* size of dma  buffer */
	uint32_t size;
	/* write pointer of dma  buffer */
	uint32_t wr_ptr;
	/* read pointer of dma  buffer */
	uint32_t rd_ptr;
	/* read size of dma  buffer */
	uint32_t rd_size;
	/* write size of dma  buffer */
	uint32_t wr_size;
	/* system memory size defined for the stream */
	uint32_t sys_buff_size;
	/* virtual system memory offset for system memory buffer */
	uint32_t phy_off;
	/* probe_channel id  */
	uint32_t probe_channel;
};

/* State tracking for each channel */
enum acp_dma_state {
	ACP_DMA_READY,
	ACP_DMA_PREPARED,
	ACP_DMA_SUSPENDED,
	ACP_DMA_ACTIVE,
};

/* data for each DMA channel */
struct acp_dma_chan_data {
	uint32_t direction;
	enum acp_dma_state state;
	struct acp_dma_ptr_data ptr_data; /* pointer data */
	dma_callback_t dma_tfrcallback;
	void *priv_data; /* unused */
};

/* Device run time data */
struct acp_dma_dev_data {
	struct dma_context dma_ctx;
	struct acp_dma_chan_data chan_data[ACP_DMA_CHAN_COUNT];
	ATOMIC_DEFINE(atomic, ACP_DMA_CHAN_COUNT);
	struct dma_config *dma_config;
	void *dai_index_ptr;
};

static ALWAYS_INLINE void io_reg_write(uint32_t reg, uint32_t val)
{
	*((volatile uint32_t *)(reg)) = val;
}

static ALWAYS_INLINE uint32_t io_reg_read(uint32_t reg)
{
	return *((volatile uint32_t *)(reg));
}

void acp_change_clock_notify(uint32_t clock_freq);

#endif /* ZEPHYR_SOC_AMD_ACP_7_0_ACP70_CHIP_REG_H_ */
