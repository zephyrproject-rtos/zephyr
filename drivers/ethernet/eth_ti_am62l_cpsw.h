/* TI AM62L CPSW3G Ethernet driver
 *
 * Copyright (c) 2026 Texas Instruments Incorporated
 * Author: Siddharth Vadapalli <s-vadapalli@ti.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_TI_AM62L_CPSW_H_
#define ETH_TI_AM62L_CPSW_H_

#include <zephyr/device.h>
#include <zephyr/drivers/dma/dma_ti_am62l_pktdma.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

/* CPSW Subsystem (CPSW_NUSS) register offsets from SS_BASE */
#define CPSW_NUSS_VER			0x0000
#define CPSW_NUSS_CONTROL		0x0004
#define CPSW_NUSS_STAT_PORT_EN		0x0014
#define CPSW_NUSS_PTYPE			0x0018

/* CPSW_NU inner module base offset from SS_BASE */
#define CPSW_NU_BASE			0x20000

/* Sub-module offsets relative to CPSW_NU_BASE */
#define CPSW_PORT0_BASE			0x01000
#define CPSW_PORT1_BASE			0x02000
#define CPSW_PORT2_BASE			0x03000
#define CPSW_ALE_BASE			0x1E000

/* CPSW NUSS Control Register fields */
#define CPSW_NUSS_CTL_P0_ENABLE		BIT(2)
#define CPSW_NUSS_CTL_P0_TX_CRC_REMOVE	BIT(13)
#define CPSW_NUSS_CTL_P0_RX_PAD	BIT(14)

/* CPSW NUSS Port Statistics Enable */
#define CPSW_NUSS_STAT_P0_EN		BIT(0)

/* Per-port register offsets (relative to port base) */
#define CPSW_P0_FLOW_ID_REG		0x0008 /* Host Port only */
#define CPSW_PN_RX_MAXLEN		0x0024
#define CPSW_PN_SA_L			0x0308
#define CPSW_PN_SA_H			0x030c

/* MACSL module offset relative to port_base */
#define CPSW_MACSL_OFS			0x0330

/* Register offsets relative to CPSW_MACSL_OFS */
#define CPSW_MACSL_CTL_REG		0x0000
#define CPSW_MACSL_STATUS_REG		0x0004
#define CPSW_MACSL_RESET_REG		0x0008

/* GMII_SEL register bit fields */
#define GMII_SEL_MODE_MASK		GENMASK(2, 0)
#define GMII_SEL_MODE_RMII		1U
#define GMII_SEL_MODE_RGMII		2U
#define GMII_SEL_RGMII_ID_BIT		BIT(4)

/* MACSL CTL register fields */
#define MACSL_CTL_FULL_DUPLEX		BIT(0)
#define MACSL_CTL_GMII_EN		BIT(5)
#define MACSL_CTL_GIG			BIT(7)
#define MACSL_RESET_BIT			BIT(0)

/* ALE register offsets (relative to CPSW_ALE_BASE) */
#define ALE_CONTROL			0x08
#define ALE_PORTCTL0			0x40
#define ALE_PORTCTL1			0x44
#define ALE_PORTCTL2			0x48
#define ALE_THREADMAPDEF		0x134

/* ALE Control Register fields */
#define ALE_CONTROL_ENABLE		BIT(31)
#define ALE_CONTROL_CLEAR_TBL		BIT(30)
#define ALE_CONTROL_BYPASS		BIT(4)

/* ALE Port Control register fields */
#define ALE_PORTCTL_FORWARD		0x3U
#define ALE_PORTCTL_MAC_ONLY		BIT(11)

/* ALE Default Thread Map register fields */
#define ALE_DEFTHREAD_EN		BIT(15)

#define CPSW_NUM_MAC_PORTS		2U

/**
 * @brief PHY connection type for a CPSW MAC port
 *
 * Resolved at compile time from the phy-connection-type DT property via
 * CPSW_PHY_CONN_TYPE().  Stored as an integer in cpsw_dt_config so that
 * cpsw_gmii_sel_set() can use a switch rather than strcmp().
 */
enum cpsw_phy_conn_type {
	CPSW_PHY_CONN_RMII,
	CPSW_PHY_CONN_RGMII,
	CPSW_PHY_CONN_RGMII_ID,
	CPSW_PHY_CONN_RGMII_RXID,
	CPSW_PHY_CONN_RGMII_TXID,
	CPSW_PHY_CONN_INVALID,
};

/**
 * @brief Translate a DT phy-connection-type string to cpsw_phy_conn_type
 *
 * Evaluated entirely at compile time using DT_ENUM_HAS_VALUE().
 *
 * @param node_id DT node identifier for the MAC port
 */
#define CPSW_PHY_CONN_TYPE(node_id)						\
	(DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rmii)      ?		\
		CPSW_PHY_CONN_RMII      :					\
	DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii)      ?		\
		CPSW_PHY_CONN_RGMII     :					\
	DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii_id)   ?		\
		CPSW_PHY_CONN_RGMII_ID  :					\
	DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii_rxid) ?		\
		CPSW_PHY_CONN_RGMII_RXID :					\
	DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii_txid) ?		\
		CPSW_PHY_CONN_RGMII_TXID :					\
	CPSW_PHY_CONN_INVALID)

/* Maximum / minimum Ethernet frame sizes */
#define CPSW_MAX_PACKET_SIZE		CONFIG_NET_BUF_DATA_SIZE
#define CPSW_MIN_PACKET_SIZE		64U

/* CPPI5 host descriptor definitions */

/* pkt_info0 fields */
#define CPPI5_INFO0_HDESC_TYPE_HOST	(0x1U << 30)
#define CPPI5_INFO0_HDESC_EPIB_PRESENT	BIT(29)
#define CPPI5_INFO0_HDESC_PKTLEN_MASK	GENMASK(21, 0)
#define CPPI5_INFO0_HDESC_PSFLGS_MASK	GENMASK(27, 24)

/* pkt_info1 fields */
#define CPPI5_INFO1_HDESC_FLOW_ID	0x3FFF

/* pkt_info2 fields */
#define CPPI5_INFO2_HDESC_PKTTYPE_MASK	GENMASK(29, 27)
#define CPPI5_INFO2_HDESC_RETQ_MASK	GENMASK(15, 0)
#define CPPI5_INFO2_HDESC_PKTTYPE_CPSW	0x7U

/* src_dst_tag fields */
#define CPPI5_TAG_SRCTAG_PORT_MASK	GENMASK(18, 16)
#define CPPI5_TAG_DSTTAG_MASK		GENMASK(15, 0)

/* Protocol-specific data size for CPSW TX descriptors */
#define CPPI5_PSD_SIZE_CPSW		16U

/* Total CPPI5 host descriptor size in bytes */
#define CPPI5_DESC_SIZE			128U

/* Ring element size: each ring entry holds one 8-byte descriptor pointer. */
#define RING_ELEM_SIZE			8U

/**
 * @brief CPPI5 host descriptor layout.
 *
 * 128-byte structure aligned to a 64-byte cache line.  The sw_data[]
 * region carries the original buffer pointer (bytes 0-7), EPIB (8-23),
 * protocol-specific data (24-39), and padding (40-95).
 */
struct cppi5_host_desc {
	uint32_t pkt_info0;
	uint32_t pkt_info1;
	uint32_t pkt_info2;
	uint32_t src_dst_tag;
	uint64_t next_desc;	/* Linking words (words 4-5) */
	uint64_t buf_ptr;	/* Buffer pointer (words 6-7) */
	uint32_t buf_len;	/* Buffer length (word 8) */
	uint32_t org_buf_len;	/* Original buffer length (word 9) */
	uint64_t org_buf_ptr;	/* Original buffer pointer (words 10-11) */
	uint8_t  sw_data[96];	/* [0-7]=org_buf [8-23]=EPIB [24-39]=PSD [40-95]=pad */
} __packed __aligned(64);

/* Descriptor pool parameters */
#define CPSW_TX_DESC_NUM		CONFIG_ETH_TI_AM62L_CPSW_TX_DESC_NUM
#define CPSW_RX_DESC_NUM		CONFIG_ETH_TI_AM62L_CPSW_RX_DESC_NUM
#define CPSW_RX_BUF_SIZE		CONFIG_NET_BUF_DATA_SIZE
#define CPSW_TX_RING_MEM_SIZE		(CPSW_TX_DESC_NUM * RING_ELEM_SIZE)
#define CPSW_RX_RING_MEM_SIZE		(CPSW_RX_DESC_NUM * RING_ELEM_SIZE)

/* PKTDMA state maintained by the CPSW driver */
struct cpsw_pktdma {
	/* Descriptor pools (64-byte aligned for cache operations) */
	struct cppi5_host_desc tx_descs[CPSW_TX_DESC_NUM] __aligned(64);
	struct cppi5_host_desc rx_descs[CPSW_RX_DESC_NUM] __aligned(64);

	/*
	 * Zero-filled pad buffer appended as an extra CPPI5 buffer descriptor
	 * when a TX packet is shorter than CPSW_MIN_PACKET_SIZE. Multiple
	 * in-flight descriptors may point to this buffer simultaneously since
	 * this will never be modified.
	 */
	uint8_t tx_pad_buf[CPSW_MIN_PACKET_SIZE] __aligned(64);

	/*
	 * RX fragment pointers for zero-copy receive. Each entry holds a
	 * net_buf whose data area is the live DMA receive buffer for the
	 * corresponding rx_descs[] slot.  NULL when the slot is unused.
	 */
	struct net_buf *rx_frags[CPSW_RX_DESC_NUM];

	/* Ring element backing memory: 8 bytes per entry */
	uint64_t tx_ring_mem[CPSW_TX_DESC_NUM] __aligned(64);
	uint64_t rx_ring_mem[CPSW_RX_DESC_NUM] __aligned(64);

	/* TX descriptor allocation index (wraps at CPSW_TX_DESC_NUM) */
	uint32_t tx_head;

	/*
	 * Zero-copy TX per-descriptor tracking
	 *
	 * tx_frags[i] – net_buf reference held for the fragment whose data
	 *               buffer backs descriptor slot i. NULL for the pad
	 *               descriptor (pointing to tx_pad_buf).
	 * tx_n_descs[i] – number of CPPI5 descriptors chained for the packet
	 *                 whose head descriptor is at slot i.
	 */
	struct net_buf *tx_frags[CPSW_TX_DESC_NUM];
	uint8_t         tx_n_descs[CPSW_TX_DESC_NUM];

	/*
	 * TX completion drain counter
	 *
	 * Each dma_reload() TX submission records the head descriptor slot
	 * index in tx_cmpl_head_idxs[] at position (tx_cmpl_wptr % N).
	 * The PKTDMA ISR pops completions from the ring in FIFO order and
	 * invokes the callback once per completion. Each callback invocation
	 * gives tx_compl_sem by one, which the TX completion thread consumes,
	 * and advances tx_cmpl_rptr to find the head_idx for that completion.
	 */
	uint32_t tx_cmpl_head_idxs[CPSW_TX_DESC_NUM];
	uint32_t tx_cmpl_wptr;   /* write pointer, advanced per dma_reload TX */
	uint32_t tx_cmpl_rptr;   /* read pointer, advanced per TX completion */

	/*
	 * RX completion drain counter
	 *
	 * RX descriptors are submitted to PKTDMA in slot order (0, 1, 2, ...)
	 * and completions are returned in FIFO order. rx_drain_idx identifies
	 * the rx_descs[] slot that just completed, and it is advanced by one for
	 * each callback invocation received from the PKTDMA ISR.
	 */
	uint32_t rx_drain_idx;

	/* PKTDMA initialization state */
	bool initialized;

	/*
	 * Interrupt-driven completion signalling.
	 * tx_free_sem:   counting semaphore, value = free TX descriptor slots.
	 *                Initialised to CPSW_TX_DESC_NUM (all slots free).
	 *                cpsw_tx() takes one slot before pushing to the ring;
	 *                the TX thread gives one slot back for each completion.
	 * tx_compl_sem:  given by TX ISR when the completion ring becomes
	 *                non-empty; taken by the TX completion thread.
	 * rx_avail_sem:  given by RX ISR when the completion ring becomes
	 *                non-empty; taken by the RX thread.
	 */
	struct k_sem tx_free_sem;
	struct k_sem tx_compl_sem;
	struct k_sem rx_avail_sem;
};

#define CPSW_TX_THREAD_STACK_SIZE	CONFIG_ETH_TI_AM62L_CPSW_TX_THREAD_STACK_SIZE
#define CPSW_TX_THREAD_PRIORITY		K_PRIO_PREEMPT(CONFIG_ETH_TI_AM62L_TX_THREAD_PRIO)

#define CPSW_RX_THREAD_STACK_SIZE	CONFIG_ETH_TI_AM62L_CPSW_RX_THREAD_STACK_SIZE
#define CPSW_RX_THREAD_PRIORITY		K_PRIO_PREEMPT(CONFIG_ETH_TI_AM62L_RX_THREAD_PRIO)

/* Maximum packets processed per poll cycle before yielding */
#define CPSW_TX_BUDGET			CONFIG_ETH_TI_AM62L_CPSW_TX_BUDGET
#define CPSW_RX_BUDGET			CONFIG_ETH_TI_AM62L_CPSW_RX_BUDGET

/* Timeout for acquiring a free transmit descriptor */
#define CPSW_TX_DESC_ACQUIRE_TIMEOUT_MS CONFIG_ETH_TI_AM62L_CPSW_TX_DESC_ACQUIRE_TIMEOUT

/*
 * CPSW MAC Port reset timeout of 1 millisecond is implemented as:
 * Poll every 100 microseconds and retry 10 times.
 */
#define CPSW_POLL_INTERVAL_US 100U
#define CPSW_MAC_RESET_MAX_RETRIES 10U

/* Maximum number of fragments for TX Scatter Gather */
#define CPSW_TX_MAX_SG_FRAGS		8U

/*
 * Wait period for RX Completion thread to drain packets after
 * all interfaces are stopped.
 */
#define CPSW_RX_DRAIN_PERIOD_MS 10U

/**
 * @brief CPSW Ethernet driver private data
 *
 * One instance is shared by both MAC port devices.  Per-port fields are
 * indexed by (port_num - 1).
 */
struct cpsw_priv {
	/* DEVICE_MMIO_RAM must be the first member */
	DEVICE_MMIO_RAM;

	/* CPSW SS virtual base address */
	uint32_t *base;

	/* Virtual base of mapped GMII SEL region */
	mm_reg_t gmii_sel_base;

	/* Virtual base of mapped MAC eFuse region */
	mm_reg_t mac_efuse_base;

	/* Primary (MAC port 1) device reference */
	const struct device *dev;

	/* Per-MAC-port network interfaces, indexed by (port_num - 1) */
	struct net_if *iface[CPSW_NUM_MAC_PORTS];

	/* Lock protecting TX DMA Channel shared by network interfaces */
	struct k_mutex tx_lock;

	/* Lock protecting shared hardware initialization in cpsw_start() */
	struct k_mutex start_lock;

	/* Per-MAC-port MAC addresses, indexed by (port_num - 1) */
	uint8_t mac_addr[CPSW_NUM_MAC_PORTS][6];

	/* Bitmask of enabled MAC ports: BIT(0) = port 1, BIT(1) = port 2 */
	uint32_t ports_started;

	/* PKTDMA ring memory and descriptor state */
	struct cpsw_pktdma dma;

	/* TX completion thread */
	struct k_thread tx_thread;

	/* TX completion thread ID */
	k_tid_t tx_tid;

	/* TX completion thread stack */
	K_KERNEL_STACK_MEMBER(tx_stack, CPSW_TX_THREAD_STACK_SIZE);

	/* RX completion thread */
	struct k_thread rx_thread;

	/* RX completion thread ID */
	k_tid_t rx_tid;

	/* RX completion thread stack */
	K_KERNEL_STACK_MEMBER(rx_stack, CPSW_RX_THREAD_STACK_SIZE);
};


/* MMIO Helpers */

/* Read a 32-bit CPSW register at byte offset @p offset from SS_BASE. */
static inline uint32_t cpsw_read(struct cpsw_priv *priv, uint32_t offset)
{
	__sync_synchronize();
	return priv->base[offset / 4];
}

/* Write a 32-bit value to a CPSW register at byte offset @p offset. */
static inline void cpsw_write(struct cpsw_priv *priv, uint32_t offset,
			      uint32_t value)
{
	priv->base[offset / 4] = value;
	__sync_synchronize();
}

#endif /* ETH_TI_AM62L_CPSW_H_ */
