/**
 * SPDX-FileCopyrightText: Copyright (c) 2026 AMD
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
		unsigned int reserved: 29;
	} bits;
	unsigned int u32all;
} acp_dma_cntl_0_t;

typedef union acp_dma_ch_sts {
	struct {
		unsigned int dmachrunsts: 8;
		unsigned int reserved: 24;
	} bits;
	unsigned int u32all;
} acp_dma_ch_sts_t;

typedef union acp_dsp_sw_intr_cntl {
	struct {
		unsigned int reserved0: 2;
		unsigned int dsp0_to_host_intr_mask: 1;
		unsigned int reserved1: 29;
	} bits;
	unsigned int u32all;
} acp_dsp_sw_intr_cntl_t;

typedef union acp_external_intr_enb {
	struct {
		unsigned int acpextintrenb: 1;
		unsigned int reserved: 31;
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

/* ITER: TX enable register */
typedef union acp_tdm_iter {
	struct {
		uint32_t tdm_txen: 1;
		uint32_t tdm_tx_protocol_mode: 1;
		uint32_t tdm_tx_data_path_mode: 1;
		uint32_t tdm_tx_samp_len: 3;
		uint32_t reserved0: 1;
		uint32_t reserved1: 25;
	} bits;
	uint32_t u32all;
} acp_tdm_iter_t;

/* IRER: RX enable register */
typedef union acp_tdm_irer {
	struct {
		uint32_t tdm_rx_en: 1;
		uint32_t tdm_rx_protocol_mode: 1;
		uint32_t tdm_rx_data_path_mode: 1;
		uint32_t tdm_rx_samplen: 3;
		uint32_t reserved0: 1;
		uint32_t reserved1: 25;
	} bits;
	uint32_t u32all;
} acp_tdm_irer_t;

/* IER: global instance enable register */
typedef union acp_tdm_ier {
	struct {
		uint32_t tdm_ien: 1;
		uint32_t reserved: 31;
	} bits;
	uint32_t u32all;
} acp_tdm_ier_t;

/* MSTRCLKGEN: master clock generation register */
union acp_tdm_mstrclkgen {
	struct {
		unsigned int tdm_master_mode : 1;
		unsigned int tdm_format_mode : 1;
		unsigned int tdm_lrclk_div_val : 11;
		unsigned int tdm_bclk_div_val : 11;
		unsigned int reserved: 8;
	} bits;
	unsigned int  u32all;
};

/* FRMT: TDM frame format register */
union acp_tdm_frmt {
	struct {
		uint32_t num_slots: 4;
		uint32_t slot_len: 5;
		uint32_t reserved: 23;
	} bits;
	uint32_t u32all;
};

/* TDM context: tracks which DMA channel belongs to which TDM instance */
struct acp_tdm_context {
	uint64_t prev_pos;
	uint32_t buff_size;
	uint32_t tdm_instance;
	uint32_t pin_dir;
	uint32_t dma_channel;
	uint32_t index;
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
