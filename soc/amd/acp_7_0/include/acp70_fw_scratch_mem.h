/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_AMD_ACP_7_0_ACP70_FW_SCRATCH_MEM_H_
#define ZEPHYR_SOC_AMD_ACP_7_0_ACP70_FW_SCRATCH_MEM_H_

#include <sof/lib/memory.h>
#define MAILBOX_DSPBOX_SIZE   SRAM_OUTBOX_SIZE
#define MAILBOX_DSPBOX_BASE   SRAM_OUTBOX_BASE
#define MAILBOX_DSPBOX_OFFSET SRAM_OUTBOX_OFFSET

#define MAILBOX_HOSTBOX_SIZE   SRAM_INBOX_SIZE
#define MAILBOX_HOSTBOX_BASE   SRAM_INBOX_BASE
#define MAILBOX_HOSTBOX_OFFSET SRAM_INBOX_OFFSET

#define MAILBOX_DEBUG_SIZE   SRAM_DEBUG_SIZE
#define MAILBOX_DEBUG_BASE   SRAM_DEBUG_BASE
#define MAILBOX_DEBUG_OFFSET SRAM_DEBUG_OFFSET

#define MAILBOX_TRACE_SIZE   SRAM_TRACE_SIZE
#define MAILBOX_TRACE_BASE   SRAM_TRACE_BASE
#define MAILBOX_TRACE_OFFSET SRAM_TRACE_OFFSET

#define MAILBOX_EXCEPTION_SIZE   SRAM_EXCEPT_SIZE
#define MAILBOX_EXCEPTION_BASE   SRAM_EXCEPT_BASE
#define MAILBOX_EXCEPTION_OFFSET SRAM_EXCEPT_OFFSET

#define MAILBOX_STREAM_SIZE SRAM_STREAM_SIZE

#define MAILBOX_STREAM_BASE   SRAM_STREAM_BASE
#define MAILBOX_STREAM_OFFSET SRAM_STREAM_OFFSET

static inline void mailbox_sw_reg_write(size_t offset, uint32_t src)
{
	volatile uint32_t *ptr;

	ptr = (volatile uint32_t *)(MAILBOX_DEBUG_BASE + offset);
	*ptr = src;
}

/* MAX number of DMA descriptors */
#define MAX_NUM_DMA_DESC_DSCR 64
#define SCRATCH_REG_OFFSET    0x1250000
#define ACP_SRAM              0x03800000

typedef struct acp_atu_grp_pte {
	uint32_t low_part;
	uint32_t high_part;
} __packed __aligned(4) acp_atu_grp_pte_t;

typedef union acp_cfg_dma_trns_cnt {
	struct {
		uint32_t trns_cnt: 19;
		uint32_t reserved: 12;
		uint32_t ioc: 1;
	} bits;
	unsigned int u32all;
} __packed __aligned(4) acp_cfg_dma_trns_cnt_t;

typedef struct acp_config_dma_descriptor {
	uint32_t src_addr;
	uint32_t dest_addr;
	acp_cfg_dma_trns_cnt_t trns_cnt;
	uint32_t reserved;
} __packed __aligned(4) acp_cfg_dma_descriptor_t;

typedef struct acp_config_dma_misc {
	uint32_t channelstatus;
	uint32_t channel;
	uint32_t flag;
} __packed __aligned(4) acp_cfg_dma_misc_t;

typedef struct acp_scratch_memory_config {
	/* ACP out box buffer */
	uint8_t acp_outbox_buffer[SRAM_OUTBOX_SIZE];

	/* ACP in box buffer */
	uint8_t acp_inbox_buffer[MAILBOX_HOSTBOX_SIZE];

	/* ACP debug box buffer */
	uint8_t acp_debug_buffer[MAILBOX_DEBUG_SIZE];

	/* ACP exception box buffer */
	uint8_t acp_except_buffer[MAILBOX_EXCEPTION_SIZE];

	/* ACP stream buffer */
	uint8_t acp_stream_buffer[MAILBOX_STREAM_SIZE];

	/* ACP trace buffer */
	uint8_t acp_trace_buffer[MAILBOX_TRACE_SIZE];

	/* Host msg write flag */
	uint32_t acp_host_msg_write;

	/* Host ack flag */
	uint32_t acp_host_ack_write;

	/* Dsp msg write flag */
	uint32_t acp_dsp_msg_write;

	/* Dsp ack flag */
	uint32_t acp_dsp_ack_write;

	/* ACP pte1 table */
	acp_atu_grp_pte_t acp_atugrp1_pte[16];

	/* ACP pte2 table */
	acp_atu_grp_pte_t acp_atugrp2_pte[16];

	/* ACP pte3 table */
	acp_atu_grp_pte_t acp_atugrp3_pte[16];

	/* ACP pte4 table */
	acp_atu_grp_pte_t acp_atugrp4_pte[16];

	/* ACP pte5 table */
	acp_atu_grp_pte_t acp_atugrp5_pte[16];

	/* ACP pte6 table */
	acp_atu_grp_pte_t acp_atugrp6_pte[16];

	/* ACP pte7 table */
	acp_atu_grp_pte_t acp_atugrp7_pte[16];

	/* ACP pte8 table */
	acp_atu_grp_pte_t acp_atugrp8_pte[16];

	/* ACP DMA Descriptor */
	acp_cfg_dma_descriptor_t acp_cfg_dma_descriptor[MAX_NUM_DMA_DESC_DSCR];

	/* Stream physical offset  */
	uint32_t phy_offset[8];

	/* Stream system memory size */
	uint32_t syst_buff_size[8];

	/* Fifo buffers are not part of scratch memory on ACP_7_0 */
	/* Added fifo members to align with Driver structure */
#define ACP_FIFO_BUFFER_SIZE 256

	/* ACP transmit fifo buffer */
	uint8_t acp_transmit_fifo_buffer[ACP_FIFO_BUFFER_SIZE] __aligned(128);

	/* ACP receive fifo buffer */
	uint8_t acp_receive_fifo_buffer[ACP_FIFO_BUFFER_SIZE] __aligned(128);

	uint32_t reserve[];
} __packed __aligned(4) acp_scratch_mem_config_t;

#endif /* ZEPHYR_SOC_AMD_ACP_7_0_ACP70_FW_SCRATCH_MEM_H_ */
