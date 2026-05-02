/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMA_IPROC_PAX
#define DMA_IPROC_PAX

/* Broadcom PAX-DMA RM register defines */
#define PAX_DMA_REG_ADDR(_base, _offs)	((_base) + (_offs))
#define PAX_DMA_RING_ADDR_OFFSET(_ring)		(0x10000 * (_ring))

/* Per-Ring register offsets */
#define RING_VER			0x000
#define RING_BD_START_ADDR		0x004
#define RING_BD_READ_PTR		0x008
#define RING_BD_WRITE_PTR		0x00c
#define RING_BD_READ_PTR_DDR_LS		0x010
#define RING_BD_READ_PTR_DDR_MS		0x014
#define RING_CMPL_START_ADDR		0x018
#define RING_CMPL_WRITE_PTR		0x01c
#define RING_NUM_REQ_RECV_LS		0x020
#define RING_NUM_REQ_RECV_MS		0x024
#define RING_NUM_REQ_TRANS_LS		0x028
#define RING_NUM_REQ_TRANS_MS		0x02c
#define RING_NUM_REQ_OUTSTAND		0x030
#define RING_CONTROL			0x034
#define RING_FLUSH_DONE			0x038
#define RING_MSI_ADDR_LS		0x03c
#define RING_MSI_ADDR_MS		0x040
#define RING_CMPL_WR_PTR_DDR_CONTROL	0x048
#define RING_BD_READ_PTR_DDR_CONTROL	0x04c
#define RING_WRITE_SEQ_NUM		0x050
#define RING_READ_SEQ_NUM		0x054
#define RING_BD_MEM_WRITE_ADDRESS	0x058
#define RING_AXI_BEAT_CNT		0x05c
#define RING_AXI_BURST_CNT		0x060
#define RING_MSI_DATA_VALUE		0x064
#define RING_PACKET_ALIGNMENT_STATUS0	0x068
#define RING_PACKET_ALIGNMENT_STATUS1	0x06c
#define RING_PACKET_ALIGNMENT_STATUS2	0x070
#define RING_DOORBELL_BD_WRITE_COUNT	0x074

/* RING Manager Common Registers */
#define RM_COMM_CTRL_REG(_ring)		(0x100 * (_ring))
#define RM_MSI_DEVID_REG(_ring)		(0x100 * (_ring) + 0x4)

#define RM_AE0_AE_CONTROL				0x2000
#define RM_AE0_NUMBER_OF_PACKETS_RECEIVED_LS_BITS	0x2004
#define RM_AE0_NUMBER_OF_PACKETS_RECEIVED_MS_BITS	0x2008
#define RM_AE0_NUMBER_OF_PACKETS_TRANSMITTED_LS_BITS	0x200c
#define RM_AE0_NUMBER_OF_PACKETS_TRANSMITTED_MS_BITS	0x2010
#define RM_AE0_OUTSTANDING_PACKET			0x2014
#define RM_AE0_AE_FLUSH_STATUS				0x2018
#define RM_AE0_AE_FIFO_WRITE_POINTER			0x201c
#define RM_AE0_AE_FIFO_READ_POINTER			0x2020
#define RM_AE1_AE_CONTROL				0x2100
#define RM_AE1_NUMBER_OF_PACKETS_RECEIVED_LS_BITS	0x2104
#define RM_AE1_NUMBER_OF_PACKETS_RECEIVED_MS_BITS	0x2108
#define RM_AE1_NUMBER_OF_PACKETS_TRANSMITTED_LS_BITS	0x210c
#define RM_AE1_NUMBER_OF_PACKETS_TRANSMITTED_MS_BITS	0x2110
#define RM_AE1_OUTSTANDING_PACKET			0x2114
#define RM_AE1_AE_FLUSH_STATUS				0x2118
#define RM_AE1_AE_FIFO_WRITE_POINTER			0x211c
#define RM_AE1_AE_FIFO_READ_POINTER			0x2120

#define RM_COMM_RING_SECURITY_SETTING			0x3000
#define RM_COMM_CONTROL					0x3008
#define RM_COMM_TIMER_CONTROL_0				0x300c
#define RM_COMM_TIMER_CONTROL_1				0x3010
#define RM_COMM_BD_THRESHOLD				0x3014
#define RM_COMM_BURST_LENGTH				0x3018
#define RM_COMM_FIFO_FULL_THRESHOLD			0x301c
#define RM_COMM_MASK_SEQUENCE_MAX_COUNT			0x3020
#define RM_COMM_AE_TIMEOUT				0x3024
#define RM_COMM_RING_OR_AE_STATUS_LOG_ENABLE		0x3028
#define RM_COMM_RING_FLUSH_TIMEOUT			0x302c
#define RM_COMM_MEMORY_CONFIGURATION			0x3030
#define RM_COMM_AXI_CONTROL				0x3034
#define RM_COMM_GENERAL_MSI_DEVICE_ID			0x3038
#define RM_COMM_GENERAL_MSI_ADDRESS_LS			0x303c
#define RM_COMM_GENERAL_MSI_ADDRESS_MS			0x3040
#define RM_COMM_CONFIG_INTERRUPT_STATUS_MASK		0x3044
#define RM_COMM_CONFIG_INTERRUPT_STATUS_CLEAR		0x3048
#define RM_COMM_TOGGLE_INTERRUPT_STATUS_MASK		0x304c
#define RM_COMM_TOGGLE_INTERRUPT_STATUS_CLEAR		0x3050
#define RM_COMM_DDR_ADDR_GEN_INTERRUPT_STATUS_MASK	0x3054
#define RM_COMM_DDR_ADDR_GEN_INTERRUPT_STATUS_CLEAR	0x3058
#define RM_COMM_PACKET_ALIGNMENT_INTERRUPT_STATUS_MASK	0x305c
#define RM_COMM_PACKET_ALIGNMENT_INTERRUPT_STATUS_CLEAR	0x3060
#define RM_COMM_AE_INTERFACE_GROUP_0_INTERRUPT_MASK	0x3064
#define RM_COMM_AE_INTERFACE_GROUP_0_INTERRUPT_CLEAR	0x3068
#define RM_COMM_AE_INTERFACE_GROUP_1_INTERRUPT_MASK	0x306c
#define RM_COMM_AE_INTERFACE_GROUP_1_INTERRUPT_CLEAR	0x3070
#define RM_COMM_AE_INTERFACE_GROUP_2_INTERRUPT_MASK	0x3074
#define RM_COMM_AE_INTERFACE_GROUP_2_INTERRUPT_CLEAR	0x3078
#define RM_COMM_AE_INTERFACE_GROUP_3_INTERRUPT_MASK	0x307c
#define RM_COMM_AE_INTERFACE_GROUP_3_INTERRUPT_CLEAR	0x3080
#define RM_COMM_AE_INTERFACE_GROUP_4_INTERRUPT_MASK	0x3084
#define RM_COMM_AE_INTERFACE_GROUP_4_INTERRUPT_CLEAR	0x3088
#define RM_COMM_AE_INTERFACE_GROUP_5_INTERRUPT_MASK	0x308c
#define RM_COMM_AE_INTERFACE_GROUP_5_INTERRUPT_CLEAR	0x3090
#define RM_COMM_AE_INTERFACE_GROUP_6_INTERRUPT_MASK	0x3094
#define RM_COMM_AE_INTERFACE_GROUP_6_INTERRUPT_CLEAR	0x3098
#define RM_COMM_AE_INTERFACE_GROUP_7_INTERRUPT_MASK	0x309c
#define RM_COMM_AE_INTERFACE_GROUP_7_INTERRUPT_CLEAR	0x30a0
#define RM_COMM_AE_INTERFACE_TOP_INTERRUPT_STATUS_MASK	0x30a4
#define RM_COMM_AE_INTERFACE_TOP_INTERRUPT_STATUS_CLEAR	0x30a8
#define RM_COMM_REORDER_INTERRUPT_STATUS_MASK		0x30ac
#define RM_COMM_REORDER_INTERRUPT_STATUS_CLEAR		0x30b0
#define RM_COMM_DME_INTERRUPT_STATUS_MASK		0x30b4
#define RM_COMM_DME_INTERRUPT_STATUS_CLEAR		0x30b8
#define RM_COMM_REORDER_FIFO_PROG_THRESHOLD		0x30bc
#define RM_COMM_GROUP_PKT_EXTENSION_SUPPORT		0x30c0
#define RM_COMM_GENERAL_MSI_DATA_VALUE			0x30c4
#define RM_COMM_AXI_READ_BURST_THRESHOLD		0x30c8
#define RM_COMM_GROUP_RING_COUNT			0x30cc
#define RM_COMM_MSI_DISABLE				0x30d8
#define RM_COMM_RESERVE					0x30fc
#define RM_COMM_RING_FLUSH_STATUS			0x3100
#define RM_COMM_RING_SEQUENCE_NUMBER_OVERFLOW		0x3104
#define RM_COMM_AE_SEQUENCE_NUMBER_OVERFLOW		0x3108
#define RM_COMM_MAX_SEQUENCE_NUMBER_FOR_ANY_RING	0x310c
#define RM_COMM_MAX_SEQUENCE_NUMBER_ON_MONITOR_RING	0x3110
#define RM_COMM_MAX_SEQUENCE_NUMBER_ON_ANY_AE		0x3114
#define RM_COMM_MAX_SEQUENCE_NUMBER_ON_MONITOR_AE	0x3118
#define RM_COMM_MIN_MAX_LATENCY_MONITOR_RING_TOGGLE	0x311c
#define RM_COMM_MIN_MAX_LATENCY_MONITOR_RING_ADDRESSGEN 0x3120
#define RM_COMM_RING_ACTIVITY				0x3124
#define RM_COMM_AE_ACTIVITY				0x3128
#define RM_COMM_MAIN_HW_INIT_DONE			0x312c
#define RM_COMM_MEMORY_POWER_STATUS			0x3130
#define RM_COMM_CONFIG_STATUS_0				0x3134
#define RM_COMM_CONFIG_STATUS_1				0x3138
#define RM_COMM_TOGGLE_STATUS_0				0x313c
#define RM_COMM_TOGGLE_STATUS_1				0x3140
#define RM_COMM_DDR_ADDR_GEN_STATUS_0			0x3144
#define RM_COMM_DDR_ADDR_GEN_STATUS_1			0x3148
#define RM_COMM_PACKET_ALIGNMENT_STATUS_0		0x314c
#define RM_COMM_PACKET_ALIGNMENT_STATUS_1		0x3150
#define RM_COMM_PACKET_ALIGNMENT_STATUS_2		0x3154
#define RM_COMM_PACKET_ALIGNMENT_STATUS_3		0x3158
#define RM_COMM_AE_INTERFACE_GROUP_0_STATUS_0		0x315c
#define RM_COMM_AE_INTERFACE_GROUP_0_STATUS_1		0x3160
#define RM_COMM_AE_INTERFACE_GROUP_1_STATUS_0		0x3164
#define RM_COMM_AE_INTERFACE_GROUP_1_STATUS_1		0x3168
#define RM_COMM_AE_INTERFACE_GROUP_2_STATUS_0		0x316c
#define RM_COMM_AE_INTERFACE_GROUP_2_STATUS_1		0x3170
#define RM_COMM_AE_INTERFACE_GROUP_3_STATUS_0		0x3174
#define RM_COMM_AE_INTERFACE_GROUP_3_STATUS_1		0x3178
#define RM_COMM_AE_INTERFACE_GROUP_4_STATUS_0		0x317c
#define RM_COMM_AE_INTERFACE_GROUP_4_STATUS_1		0x3180
#define RM_COMM_AE_INTERFACE_GROUP_5_STATUS_0		0x3184
#define RM_COMM_AE_INTERFACE_GROUP_5_STATUS_1		0x3188
#define RM_COMM_AE_INTERFACE_GROUP_6_STATUS_0		0x318c
#define RM_COMM_AE_INTERFACE_GROUP_6_STATUS_1		0x3190
#define RM_COMM_AE_INTERFACE_GROUP_7_STATUS_0		0x3194
#define RM_COMM_AE_INTERFACE_GROUP_7_STATUS_1		0x3198
#define RM_COMM_AE_INTERFACE_TOP_STATUS_0		0x319c
#define RM_COMM_AE_INTERFACE_TOP_STATUS_1		0x31a0
#define RM_COMM_REORDER_STATUS_0			0x31a4
#define RM_COMM_REORDER_STATUS_1			0x31a8
#define RM_COMM_REORDER_STATUS_2			0x31ac
#define RM_COMM_REORDER_STATUS_3			0x31b0
#define RM_COMM_REORDER_STATUS_4			0x31b4
#define RM_COMM_REORDER_STATUS_5			0x31b8
#define RM_COMM_CONFIG_INTERRUPT_STATUS			0x31bc
#define RM_COMM_TOGGLE_INTERRUPT_STATUS			0x31c0
#define RM_COMM_DDR_ADDR_GEN_INTERRUPT_STATUS		0x31c4
#define RM_COMM_PACKET_ALIGNMENT_INTERRUPT_STATUS	0x31c8
#define RM_COMM_AE_INTERFACE_GROUP_0_INTERRUPT_STATUS	0x31cc
#define RM_COMM_AE_INTERFACE_GROUP_1_INTERRUPT_STATUS	0x31d0
#define RM_COMM_AE_INTERFACE_GROUP_2_INTERRUPT_STATUS	0x31d4
#define RM_COMM_AE_INTERFACE_GROUP_3_INTERRUPT_STATUS	0x31d8
#define RM_COMM_AE_INTERFACE_GROUP_4_INTERRUPT_STATUS	0x31dc
#define RM_COMM_AE_INTERFACE_GROUP_5_INTERRUPT_STATUS	0x31e0
#define RM_COMM_AE_INTERFACE_GROUP_6_INTERRUPT_STATUS	0x31e4
#define RM_COMM_AE_INTERFACE_GROUP_7_INTERRUPT_STATUS	0x31e8
#define RM_COMM_AE_INTERFACE_TOP_INTERRUPT_STATUS	0x31ec
#define RM_COMM_REORDER_INTERRUPT_STATUS		0x31f0
#define RM_COMM_DME_INTERRUPT_STATUS			0x31f4
#define RM_COMM_PACKET_ALIGNMENT_STATUS_4		0x31f8
#define RM_COMM_PACKET_ALIGNMENT_STATUS_5		0x31fc
#define RM_COMM_PACKET_ALIGNMENT_STATUS_6		0x3200
#define RM_COMM_MSI_INTR_INTERRUPT_STATUS		0x3204
#define RM_COMM_BD_FETCH_MODE_CONTROL			0x3360

#define RM_COMM_THRESHOLD_CFG_RD_FIFO_MAX_THRESHOLD_SHIFT	16
#define RM_COMM_THRESHOLD_CFG_RD_FIFO_MAX_THRESHOLD_SHIFT_VAL	32
#define RM_COMM_THRESHOLD_CFG_RD_FIFO_MAX_THRESHOLD_MASK	0x1FF

#define RM_COMM_PKT_ALIGNMENT_BD_FIFO_FULL_THRESHOLD_SHIFT	25
#define RM_COMM_PKT_ALIGNMENT_BD_FIFO_FULL_THRESHOLD_VAL	40
#define RM_COMM_PKT_ALIGNMENT_BD_FIFO_FULL_THRESHOLD_MASK	0x7F
#define RM_COMM_BD_FIFO_FULL_THRESHOLD_VAL			224
#define RM_COMM_BD_FIFO_FULL_THRESHOLD_SHIFT			16
#define RM_COMM_BD_FIFO_FULL_THRESHOLD_MASK			0x1FF

/* PAX_DMA_RM_COMM_RM_BURST_LENGTH */
#define RM_COMM_BD_FETCH_CACHE_ALIGNED_DISABLED	BIT(28)
#define RM_COMM_VALUE_FOR_DDR_ADDR_GEN_SHIFT	16
#define RM_COMM_VALUE_FOR_TOGGLE_SHIFT		0
#define RM_COMM_VALUE_FOR_DDR_ADDR_GEN_VAL	32
#define RM_COMM_VALUE_FOR_TOGGLE_VAL		32

#define RM_COMM_DISABLE_GRP_BD_FIFO_FLOW_CONTROL_FOR_PKT_ALIGNMENT	BIT(1)
#define RM_COMM_DISABLE_PKT_ALIGNMENT_BD_FIFO_FLOW_CONTROL		BIT(0)

/* RM version */
#define RING_VER_MAGIC				0x76303031

/* Register RING_CONTROL fields */
#define RING_CONTROL_MASK_DISABLE_CONTROL	6
#define RING_CONTROL_FLUSH			BIT(5)
#define RING_CONTROL_ACTIVE			BIT(4)

/* Register RING_FLUSH_DONE fields */
#define RING_FLUSH_DONE_MASK			0x1

#define RING_MASK_SEQ_MAX_COUNT_MASK		0x3ff

/* RM_COMM_MAIN_HW_INIT_DONE DONE fields */
#define RM_COMM_MAIN_HW_INIT_DONE_MASK		0x1

/* Register RING_CMPL_WR_PTR_DDR_CONTROL fields */
#define RING_BD_READ_PTR_DDR_TIMER_VAL_SHIFT	16
#define RING_BD_READ_PTR_DDR_TIMER_VAL_MASK	0xffff
#define RING_BD_READ_PTR_DDR_ENABLE_SHIFT	15
#define RING_BD_READ_PTR_DDR_ENABLE_MASK	0x1

/* Register RING_BD_READ_PTR_DDR_CONTROL fields */
#define RING_BD_CMPL_WR_PTR_DDR_TIMER_VAL_SHIFT	16
#define RING_BD_CMPL_WR_PTR_DDR_TIMER_VAL_MASK	0xffff
#define RING_BD_CMPL_WR_PTR_DDR_ENABLE_SHIFT	15
#define RING_BD_CMPL_WR_PTR_DDR_ENABLE_MASK	0x1

/*
 * AE_TIMEOUT is  (2^AE_TIMEOUT_BITS) - (2 * NumOfAEs * 2^FIFO_DEPTH_BITS)
 * AE_TIMEOUT_BITS=32, NumOfAEs=2, FIFO_DEPTH_BITS=5
 * timeout val =  2^32 - 2*2*2^5
 */
#define RM_COMM_AE_TIMEOUT_VAL			0xffffff80

/* RM timer control fields for 4 rings */
#define RM_COMM_TIMER_CONTROL_FAST		0xaf
#define RM_COMM_TIMER_CONTROL_FAST_SHIFT	16
#define RM_COMM_TIMER_CONTROL_MEDIUM		0x15e
#define RM_COMM_TIMER_CONTROL0_VAL		\
	((RM_COMM_TIMER_CONTROL_FAST << RM_COMM_TIMER_CONTROL_FAST_SHIFT) | \
	 (RM_COMM_TIMER_CONTROL_MEDIUM))
#define RM_COMM_TIMER_CONTROL_SLOW		0x2bc
#define RM_COMM_TIMER_CONTROL_SLOW_SHIFT	16
#define RM_COMM_TIMER_CONTROL_IDLE		0x578
#define RM_COMM_TIMER_CONTROL1_VAL		\
	((RM_COMM_TIMER_CONTROL_SLOW << RM_COMM_TIMER_CONTROL_SLOW_SHIFT) | \
	 (RM_COMM_TIMER_CONTROL_IDLE))
#define RM_COMM_RM_BURST_LENGTH			0x80008

/* Register RM_COMM_AXI_CONTROL fields */
#define RM_COMM_AXI_CONTROL_RD_CH_EN_SHIFT	24
#define RM_COMM_AXI_CONTROL_RD_CH_EN		\
	BIT(RM_COMM_AXI_CONTROL_RD_CH_EN_SHIFT)
#define RM_COMM_AXI_CONTROL_WR_CH_EN_SHIFT	28
#define RM_COMM_AXI_CONTROL_WR_CH_EN		\
	BIT(RM_COMM_AXI_CONTROL_WR_CH_EN_SHIFT)

/* Register Per-ring RING_COMMON_CONTROL fields */
#define RING_COMM_CTRL_AE_GROUP_SHIFT	0
#define RING_COMM_CTRL_AE_GROUP_MASK	(0x7 << RING_COMM_CTRL_AE_GROUP_SHIFT)

/* Register AE0_AE_CONTROL/AE1_AE_CONTROL fields */
#define RM_AE_CONTROL_ACTIVE	BIT(4)
#define RM_AE_CTRL_AE_GROUP_SHIFT	0
#define RM_AE_CTRL_AE_GROUP_MASK	(0x7 << RM_AE_CTRL_AE_GROUP_SHIFT)

/* Register RING_CMPL_WR_PTR_DDR_CONTROL fields */
#define RING_DDR_CONTROL_COUNT_SHIFT	0
#define RING_DDR_CONTROL_COUNT_MASK	0x3ff
#define RING_DDR_CONTROL_COUNT(x)	(((x) & RING_DDR_CONTROL_COUNT_MASK) \
					  << RING_DDR_CONTROL_COUNT_SHIFT)
#define RING_DDR_CONTROL_COUNT_VAL	0x1U
#define RING_DDR_CONTROL_ENABLE_SHIFT	15
#define RING_DDR_CONTROL_ENABLE		BIT(RING_DDR_CONTROL_ENABLE_SHIFT)
#define RING_DDR_CONTROL_TIMER_SHIFT	16
#define RING_DDR_CONTROL_TIMER_MASK	0xffff
#define RING_DDR_CONTROL_TIMER(x)	(((x) & RING_DDR_CONTROL_TIMER_MASK) \
					  << RING_DDR_CONTROL_TIMER_SHIFT)

/*
 * Set no timeout value for completion write path as it would generate
 * multiple interrupts during large transfers. And if timeout value is
 * set, completion write pointers has to be checked on each interrupt
 * to ensure that transfer is actually done.
 */
#define RING_DDR_CONTROL_TIMER_VAL	(0xFFFF)

/* completion DME status code */
#define PAX_DMA_STATUS_AXI_RRESP_ERR		BIT(0)
#define PAX_DMA_STATUS_AXI_BRESP_ERR		BIT(1)
#define PAX_DMA_STATUS_PCIE_CA_ERR		BIT(2)
#define PAX_DMA_STATUS_PCIE_UR_ERR		BIT(3)
#define PAX_DMA_STATUS_PCIE_CMPL_TOUT_ERR	BIT(4)
#define PAX_DMA_STATUS_PCIE_RX_POISON		BIT(5)
#define PAX_DMA_STATUS_ERROR_MASK	(		\
					 PAX_DMA_STATUS_AXI_RRESP_ERR | \
					 PAX_DMA_STATUS_AXI_BRESP_ERR | \
					 PAX_DMA_STATUS_PCIE_CA_ERR | \
					 PAX_DMA_STATUS_PCIE_UR_ERR | \
					 PAX_DMA_STATUS_PCIE_CMPL_TOUT_ERR | \
					 PAX_DMA_STATUS_PCIE_RX_POISON \
					)
/* completion RM status code */
#define RM_COMPLETION_SUCCESS		0x0
#define RM_COMPLETION_AE_TIMEOUT	0x3FF

#define RM_COMM_MSI_CONFIG_INTERRUPT_ACCESS_ERR_MASK	BIT(9)
#define RM_COMM_MSI_CONFIG_INTERRUPT_BRESP_ERR_MASK	BIT(8)
#define RM_COMM_MSI_DISABLE_MASK			BIT(0)

/* Buffer Descriptor definitions */
#define PAX_DMA_TYPE_RM_HEADER		0x1
#define PAX_DMA_TYPE_NEXT_PTR		0x5

/* one desc ring size( is 4K, 4K aligned */
#define PAX_DMA_RM_DESC_RING_SIZE	4096
#define PAX_DMA_RING_BD_ALIGN_ORDER	12
/* completion ring size(bytes) is 8K, 8K aligned */
#define PAX_DMA_RM_CMPL_RING_SIZE	8192
#define PAX_DMA_RING_CMPL_ALIGN_ORDER	13

#define PAX_DMA_RING_BD_ALIGN_CHECK(addr) \
	(!((addr) & ((0x1 << RING_BD_ALIGN_ORDER) - 1)))
#define RING_CMPL_ALIGN_CHECK(addr) \
	(!((addr) & ((0x1 << RING_CMPL_ALIGN_ORDER) - 1)))

/* RM descriptor width: 8 bytes */
#define PAX_DMA_RM_DESC_BDWIDTH		8
/* completion msg desc takes 1 BD */
#define PAX_DMA_CMPL_DESC_SIZE		PAX_DMA_RM_DESC_BDWIDTH
/* Next table desc takes 1 BD */
#define PAX_DMA_NEXT_TBL_DESC_SIZE	PAX_DMA_RM_DESC_BDWIDTH
/* Header desc takes 1 BD */
#define PAX_DMA_HEADER_DESC_SIZE	PAX_DMA_RM_DESC_BDWIDTH
/* Total BDs in ring: 4K/8bytes = 512 BDs */
#define PAX_DMA_RM_RING_BD_COUNT	(PAX_DMA_RM_DESC_RING_SIZE / \
					 PAX_DMA_RM_DESC_BDWIDTH)

/* Initial RM header is first BD in ring */
#define PAX_DMA_HEADER_INDEX		0
#define PAX_DMA_HEADER_ADDR(_ring)	(void *)((uintptr_t)(_ring) + \
	PAX_DMA_HEADER_INDEX * PAX_DMA_RM_DESC_BDWIDTH)

/* NEXT TABLE desc offset is last BD in ring */
#define PAX_DMA_NEXT_TBL_INDEX		(PAX_DMA_RM_RING_BD_COUNT - 1)
#define PAX_DMA_NEXT_TBL_ADDR(_ring)	(void *)((uintptr_t)(_ring) + \
	PAX_DMA_NEXT_TBL_INDEX * PAX_DMA_RM_DESC_BDWIDTH)

/* DMA transfers supported from 4 bytes thru 16M, size aligned to 4 bytes */
#define PAX_DMA_MIN_SIZE	4
#define PAX_DMA_MAX_SIZE	(16 * 1024 * 1024)

/* Host and Card address need 4-byte alignment */
#define PAX_DMA_ADDR_ALIGN 4

#define RM_RING_REG(_pd, _r, _write_ptr) \
	((_pd)->ring[_r].ring_base + (_write_ptr))
#define RM_COMM_REG(_pd, _write_ptr) ((_pd)->rm_comm_base + (_write_ptr))
#define PAX_DMA_REG(_pd, _write_ptr) ((_pd)->dma_base + (_write_ptr))

#define PAX_DMA_MAX_CMPL_COUNT		1024
#define PAX_DMA_LAST_CMPL_IDX		(PAX_DMA_MAX_CMPL_COUNT - 1)

#define PAX_DMA_RING_ALIGN		BIT(PAX_DMA_RING_CMPL_ALIGN_ORDER)
/* num of completions received, circular buffer */
#define PAX_DMA_GET_CMPL_COUNT(wptr, rptr) (((wptr) >= (rptr)) ? \
	((wptr) - (rptr)) : (PAX_DMA_MAX_CMPL_COUNT - (rptr) + (wptr)))

/* location of current cmpl pkt, take care of pointer wrap-around */
#define PAX_DMA_CURR_CMPL_IDX(wptr)	\
	(((wptr) == 0) ? PAX_DMA_LAST_CMPL_IDX : (wptr) - 1)

/* Timeout (milliseconds) for completion alert in interrupt mode */
#define PAX_DMA_TIMEOUT			10000

/* TODO: add macro to enable data memory barrier, to ensure writes to memory */
#define dma_mb()

/* Max polling cycles for completion wait, >= 1 second */
#define PAX_DMA_MAX_POLL_WAIT		1000000
/* Max polling cycles for posted write sync >= 1 second */
#define PAX_DMA_MAX_SYNC_WAIT		1000000

enum ring_idx {
	PAX_DMA_RING0 = 0,
	PAX_DMA_RING1,
	PAX_DMA_RING2,
	PAX_DMA_RING3,
	PAX_DMA_RINGS_MAX
};

/*
 * DMA direction
 */
enum pax_dma_dir {
	CARD_TO_HOST = 0x1,
	HOST_TO_CARD = 0x2
};

/* Completion packet */
struct cmpl_pkt {
	uint64_t opq : 16; /*pkt_id 15:0*/
	uint64_t res : 16; /*reserved 16:31*/
	uint64_t dma_status : 16; /*PAX DMA status 32:47*/
	uint64_t ae_num : 6; /*RM status[47:53] processing AE number */
	uint64_t rm_status : 10; /*RM status[54:63] completion/timeout status*/
} __attribute__ ((__packed__));

/* Driver internal structures */

struct dma_iproc_pax_addr64 {
	uint32_t addr_lo;
	uint32_t addr_hi;
} __attribute__((__packed__));

/* DMA payload for RM internal API */
struct dma_iproc_pax_payload {
	uint64_t pci_addr;
	uint64_t axi_addr;
	uint32_t xfer_sz;
	enum pax_dma_dir direction;
};

/* magic to sync completion of posted writes to host */
struct dma_iproc_pax_write_sync_data {
	/* sglist count, max 254 */
	uint32_t total_pkts:9;
	/* ring-id 0-3 */
	uint32_t ring:2;
	/* opaque-id 0-31 */
	uint32_t opaque:5;
	/* magic pattern */
	uint32_t signature:16;
};

/* BD ring status */
struct dma_iproc_pax_ring_status {
	/* current desc write_ptret, write pointer */
	void *write_ptr;
	/* current valid toggle  */
	uint32_t toggle;
	/* completion queue read offset */
	uint32_t cmpl_rd_offs;
	/* opaque value for current payload */
	uint32_t opq;
	/* posted write sync data */
	struct dma_iproc_pax_write_sync_data sync_data;
};

struct dma_iproc_pax_ring_data {
	/* ring index */
	uint32_t idx;
	/* Per-Ring register base */
	uint32_t ring_base;
	/* Allocated mem for BD and CMPL */
	void *ring_mem;
	/* Buffer descriptors, 4K aligned */
	void *bd;
	/* Completion descriptors, 8K aligned */
	void *cmpl;
	/* payload struct for internal API */
	struct dma_iproc_pax_payload *payload;
	/* ring current status */
	struct dma_iproc_pax_ring_status curr;
	/* assigned packet id upto 32 values */
	uint32_t pkt_id;
	/* per-ring lock */
	struct k_mutex lock;
	/* alert for the ring */
	struct k_sem alert;
	/* posted write sync src location */
	struct dma_iproc_pax_write_sync_data *sync_loc;
	/* posted write sync pci dst address */
	struct dma_iproc_pax_addr64 sync_pci;
	/* ring status */
	int ring_active;
	/* dma callback and argument */
	dma_callback_t dma_callback;
	void *callback_arg;
	uint32_t descs_inflight;
	uint32_t non_hdr_bd_count;
	uint32_t total_pkt_count;
	uintptr_t current_hdr;
};

struct dma_iproc_pax_data {
	/* PAXB0 PAX DMA registers */
	uint32_t dma_base;
	/* Ring manager common registers */
	uint32_t rm_comm_base;
	/* Num of rings to use in s/w */
	int used_rings;
	/* DMA lock */
	struct k_mutex dma_lock;
	/* Per-Ring data */
	struct dma_iproc_pax_ring_data ring[PAX_DMA_RINGS_MAX];
};

/* PAX DMA config */
struct dma_iproc_pax_cfg {
	/* PAXB0 PAX DMA registers */
	uint32_t dma_base;
	/* Per-Ring register base addr */
	uint32_t rm_base;
	/* Ring manager common registers */
	uint32_t rm_comm_base;
	/* Num of rings to be used */
	int use_rings;
	void *bd_memory_base;
	uint32_t scr_addr_loc;
	const struct device *pcie_dev;
};

#endif
