/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMA_IPROC_PAX_V2
#define DMA_IPROC_PAX_V2

#include "dma_iproc_pax.h"

#define RING_COMPLETION_INTERRUPT_STAT_MASK	0x088
#define RING_COMPLETION_INTERRUPT_STAT_CLEAR	0x08c
#define RING_COMPLETION_INTERRUPT_STAT		0x090
#define RING_DISABLE_MSI_TIMEOUT		0x0a4

/* Register RM_COMM_CONTROL fields */
#define RM_COMM_CONTROL_MODE_MASK			0x3
#define RM_COMM_CONTROL_MODE_SHIFT			0
#define RM_COMM_CONTROL_MODE_DOORBELL			0x0
#define RM_COMM_CONTROL_MODE_TOGGLE			0x2
#define RM_COMM_CONTROL_MODE_ALL_BD_TOGGLE		0x3
#define RM_COMM_CONTROL_CONFIG_DONE			BIT(2)
#define RM_COMM_CONTROL_LINE_INTR_EN			BIT(4)
#define RM_COMM_CONTROL_AE_TIMEOUT_EN			BIT(5)

#define RING_DISABLE_MSI_TIMEOUT_VALUE	1

#define PAX_DMA_TYPE_SRC_DESC		0x2
#define PAX_DMA_TYPE_DST_DESC		0x3
#define PAX_DMA_TYPE_MEGA_SRC_DESC	0x6
#define PAX_DMA_TYPE_MEGA_DST_DESC	0x7
#define PAX_DMA_TYPE_PCIE_DESC		0xB
#define PAX_DMA_NUM_BD_BUFFS		9
/* PCIE DESC, either DST or SRC DESC */
#define PAX_DMA_RM_DESC_BDCOUNT		2

/* ascii signature  'V' 'P' */
#define PAX_DMA_WRITE_SYNC_SIGNATURE	0x5650

#define PAX_DMA_PCI_ADDR_MSB8_SHIFT	56
#define PAX_DMA_PCI_ADDR_HI_MSB8(pci)	((pci) >> PAX_DMA_PCI_ADDR_MSB8_SHIFT)

#define PAX_DMA_MAX_SZ_PER_BD		(512 * 1024)
#define PAX_DMA_MEGA_LENGTH_MULTIPLE	16

/* Maximum DMA block count supported per request */
#define RM_V2_MAX_BLOCK_COUNT		1024
#define MAX_BD_COUNT_PER_HEADER		30

/*
 * Sync payload buffer size is of 4 bytes,4096 Bytes allocated here
 * to make sure BD memories fall in 4K alignment.
 */
#define PAX_DMA_RM_SYNC_BUFFER_MISC_SIZE       4096

/*
 * Per-ring memory, with 8K & 4K alignment
 * Alignment may not be ensured by allocator
 * s/w need to allocate extra upto 8K to
 * ensure aligned memory space.
 */
#define PAX_DMA_PER_RING_ALLOC_SIZE	(PAX_DMA_RM_CMPL_RING_SIZE * 2 + \
					 PAX_DMA_NUM_BD_BUFFS * \
					 PAX_DMA_RM_DESC_RING_SIZE + \
					 PAX_DMA_RM_SYNC_BUFFER_MISC_SIZE)

/* RM header desc field */
struct rm_header {
	uint64_t opq : 16; /*pkt_id 15:0*/
	uint64_t bdf : 16; /*reserved 31:16*/
	uint64_t res1 : 4; /*res 32:35*/
	uint64_t bdcount : 5; /*bdcount 36:40*/
	uint64_t prot : 2; /*prot 41:42*/
	uint64_t res2 : 1; /*res :43:43*/
	uint64_t pcie_addr_msb : 8; /*pcie addr :44:51*/
	uint64_t res3 : 4; /*res :52:55*/
	uint64_t start : 1; /*S :56*/
	uint64_t end : 1; /*E:57*/
	uint64_t res4 : 1; /*res:58*/
	uint64_t toggle : 1; /*T:59*/
	uint64_t type : 4; /*type:60:63*/
} __attribute__ ((__packed__));

/* pcie desc field */
struct pcie_desc {
	uint64_t pcie_addr_lsb : 56; /* pcie_addr_lsb  0:55*/
	uint64_t res1: 3; /*reserved 56:58*/
	uint64_t toggle : 1; /*T:59*/
	uint64_t type : 4; /*type:60:63*/
} __attribute__ ((__packed__));

/* src/dst desc field */
struct src_dst_desc {
	uint64_t axi_addr : 44; /*axi_addr[43:0]*/
	uint64_t length : 15; /*length[44:58]*/
	uint64_t toggle : 1; /*T:59*/
	uint64_t type : 4; /*type:60:63*/
} __attribute__ ((__packed__));

struct next_ptr_desc {
	uint64_t addr : 44; /*Address 43:0*/
	uint64_t res1 : 15;/*Reserved*/
	uint64_t toggle : 1; /*Toggle Bit:59*/
	uint64_t type : 4;/*descriptor type 63:60*/
} __attribute__ ((__packed__));

#endif
