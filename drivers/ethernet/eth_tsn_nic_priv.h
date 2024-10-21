/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_TSN_NIC_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_TSN_NIC_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#define DMA_ID_H2C 0x1fc0
#define DMA_ID_C2H 0x1fc1

#define DMA_CHANNEL_ID_MASK 0x00000f00
#define DMA_CHANNEL_ID_LSB  8
#define DMA_ENGINE_ID_MASK  0xffff0000
#define DMA_ENGINE_ID_LSB   16

#define DMA_ALIGN_BYTES_MASK       0x00ff0000
#define DMA_ALIGN_BYTES_LSB        16
#define DMA_GRANULARITY_BYTES_MASK 0x0000ff00
#define DMA_GRANULARITY_BYTES_LSB  8
#define DMA_ADDRESS_BITS_MASK      0x000000ff
#define DMA_ADDRESS_BITS_LSB       0

#define DMA_CTRL_RUN_STOP               (1UL << 0)
#define DMA_CTRL_IE_DESC_STOPPED        (1UL << 1)
#define DMA_CTRL_IE_DESC_COMPLETED      (1UL << 2)
#define DMA_CTRL_IE_DESC_ALIGN_MISMATCH (1UL << 3)
#define DMA_CTRL_IE_MAGIC_STOPPED       (1UL << 4)
#define DMA_CTRL_IE_IDLE_STOPPED        (1UL << 6)
#define DMA_CTRL_IE_READ_ERROR          (1UL << 9)
#define DMA_CTRL_IE_DESC_ERROR          (1UL << 19)
#define DMA_CTRL_NON_INCR_ADDR          (1UL << 25)
#define DMA_CTRL_POLL_MODE_WB           (1UL << 26)
#define DMA_CTRL_STM_MODE_WB            (1UL << 27)

#define DMA_CTRL_NON_INCR_ADDR (1UL << 25)

#define DMA_H2C 0
#define DMA_C2H 1

#define DMA_C2H_OFFSET 0x1000

#define DMA_CONFIG_BAR_IDX  1
/* Size of BAR1, it needs to be hard-coded as there is no PCIe API for this */
#define DMA_CONFIG_BAR_SIZE 0x10000

#define DMA_ENGINE_START 16268831
#define DMA_ENGINE_STOP  16268830

#define ETH_ALEN 6

#define BUFFER_SIZE 1500 /* Ethernet MTU */

#define DESC_MAGIC 0xAD4B0000UL

#define DESC_STOPPED   (1UL << 0)
#define DESC_COMPLETED (1UL << 1)
#define DESC_EOP       (1UL << 4)

#define SGDMA_OFFSET_FROM_CHANNEL 0x4000

#define DESC_REG_LO (SGDMA_OFFSET_FROM_CHANNEL + 0x80)
#define DESC_REG_HI (SGDMA_OFFSET_FROM_CHANNEL + 0x84)

#define LS_BYTE_MASK 0x000000FFUL

#define PCI_DMA_H(addr) ((addr >> 16) >> 16)
#define PCI_DMA_L(addr) (addr & 0xffffffffUL)

#define DMA_ENGINE_START 16268831
#define DMA_ENGINE_STOP  16268830

#define TX_METADATA_SIZE (sizeof(struct tx_metadata))
#define RX_METADATA_SIZE (sizeof(struct rx_metadata))
#define CRC_LEN          4

#define ETH_ZLEN 60

#define NS_IN_1S 1000000000

/* Temporary macros: need to be deleted later */
#define RX_ENGINE_REG_ADDR 0x1b08001000
#define RX_SGDMA_REG_ADDR  0x1b08005000
#define RX_REGS_SIZE       0x1000

enum tsn_timestamp_id {
	TSN_TIMESTAMP_ID_NONE = 0,
	TSN_TIMESTAMP_ID_GPTP = 1,
	TSN_TIMESTAMP_ID_NORMAL = 2,
	TSN_TIMESTAMP_ID_RESERVED1 = 3,
	TSN_TIMESTAMP_ID_RESERVED2 = 4,

	TSN_TIMESTAMP_ID_MAX,
};

enum tsn_fail_policy {
	TSN_FAIL_POLICY_DROP = 0,
	TSN_FAIL_POLICY_RETRY = 1,
};

struct dma_tsn_nic_config_regs {
	uint32_t identifier;
	uint32_t _reserved1[4];
	uint32_t msi_enable;
};

struct dma_tsn_nic_engine_regs {
	uint32_t identifier;
	uint32_t control;
	uint32_t control_w1s;
	uint32_t control_w1c;
	uint32_t _reserved1[12]; /* padding */

	uint32_t status;
	uint32_t status_rc;
	uint32_t completed_desc_count;
	uint32_t alignments;
	uint32_t _reserved2[14]; /* padding */

	uint32_t poll_mode_wb_lo;
	uint32_t poll_mode_wb_hi;
	uint32_t interrupt_enable_mask;
	uint32_t interrupt_enable_mask_w1s;
	uint32_t interrupt_enable_mask_w1c;
	uint32_t _reserved3[9]; /* padding */

	uint32_t perf_ctrl;
	uint32_t perf_cyc_lo;
	uint32_t perf_cyc_hi;
	uint32_t perf_dat_lo;
	uint32_t perf_dat_hi;
	uint32_t perf_pnd_lo;
	uint32_t perf_pnd_hi;
} __packed; /* TODO: Move these to header file */

struct dma_tsn_nic_engine_sgdma_regs {
	uint32_t identifier;
	uint32_t _reserved1[31]; /* padding */

	/* bus address to first descriptor in Root Complex Memory */
	uint32_t first_desc_lo;
	uint32_t first_desc_hi;
	/* number of adjacent descriptors at first_desc */
	uint32_t first_desc_adjacent;
	uint32_t credits;
} __packed;

struct dma_tsn_nic_desc {
	uint32_t control;
	uint32_t bytes;
	uint32_t src_addr_lo;
	uint32_t src_addr_hi;
	uint32_t dst_addr_lo;
	uint32_t dst_addr_hi;
	uint32_t next_lo;
	uint32_t next_hi;
};

struct dma_tsn_nic_result {
	uint32_t status;
	uint32_t length;
	uint32_t _reserved1[6]; /* padding */
};

struct tick_count {
	uint32_t tick: 29;
	uint32_t priority: 3;
} __packed __attribute__((scalar_storage_order("big-endian")));

struct tx_metadata {
	struct tick_count from;
	struct tick_count to;
	struct tick_count delay_from;
	struct tick_count delay_to;
	uint16_t frame_length;
	uint16_t timestamp_id;
	uint8_t fail_policy;
	uint8_t _reserved0[3];
	uint32_t _reserved1;
	uint32_t _reserved2;
} __packed __attribute__((scalar_storage_order("big-endian")));

struct tx_buffer {
	struct tx_metadata metadata;
	uint8_t data[BUFFER_SIZE];
} __packed __attribute__((scalar_storage_order("big-endian")));

struct rx_metadata {
	uint64_t timestamp;
	uint16_t frame_length;
} __packed __attribute__((scalar_storage_order("big-endian")));

struct rx_buffer {
	struct rx_metadata metadata;
	uint8_t data[BUFFER_SIZE];
} __packed __attribute__((scalar_storage_order("big-endian")));

int tsn_fill_metadata(const struct device *dev, uint64_t now, struct tx_buffer *buf);

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_TSN_NIC_H_ */
