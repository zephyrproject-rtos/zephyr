/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file eth_mchp_gmac_u2005.c
 * @brief Eth GMAC driver for Microchip devices.
 *
 * This file provides the implementation of eth GMAC functions
 * for Microchip-based systems.
 */

#define LOG_MODULE_NAME eth_mchp_gmac_u2005
#define LOG_LEVEL       CONFIG_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include "eth.h"

/**
 * @brief Define compatible string for device tree.
 */
#define DT_DRV_COMPAT microchip_gmac_u2005_eth

#include <zephyr/types.h>

#define MCHP_OUI_B0 0x00
#define MCHP_OUI_B1 0x04
#define MCHP_OUI_B2 0xA3

#define GMAC_MTU NET_ETH_MTU

#define GMAC_FRAME_SIZE_MAX (GMAC_MTU + 18)

/** Cache alignment */
#define GMAC_DCACHE_ALIGNMENT 32

/** Memory alignment of the RX/TX Buffer Descriptor List */
#define GMAC_DESC_ALIGNMENT 4

/** Total number of queues supported by GMAC hardware module */
#define GMAC_QUEUE_NUM          DT_INST_PROP(0, num_queues)
#define GMAC_PRIORITY_QUEUE_NUM (GMAC_QUEUE_NUM - 1)

#if (GMAC_PRIORITY_QUEUE_NUM >= 1)
BUILD_ASSERT(ARRAY_SIZE(GMAC->GMAC_TBQBAPQ) + 1 == GMAC_QUEUE_NUM,
	     "GMAC_QUEUE_NUM doesn't match soc header");
#endif

/** Number of priority queues used */
#define GMAC_ACTIVE_QUEUE_NUM          (CONFIG_ETH_MCHP_QUEUES)
#define GMAC_ACTIVE_PRIORITY_QUEUE_NUM (GMAC_ACTIVE_QUEUE_NUM - 1)

/** RX descriptors count for main queue */
#define MAIN_QUEUE_RX_DESC_COUNT (CONFIG_ETH_MCHP_BUF_RX_COUNT + 1)

/** TX descriptors count for main queue */
#define MAIN_QUEUE_TX_DESC_COUNT (CONFIG_NET_BUF_TX_COUNT + 1)

#define PRIORITY_QUEUE1_RX_DESC_COUNT 1
#define PRIORITY_QUEUE1_TX_DESC_COUNT 1

/*
 * Receive buffer descriptor bit field definitions
 */

/** Buffer ownership, needs to be 0 for the GMAC to write data to the buffer */
#define GMAC_RXW0_OWNERSHIP (0x1u << 0)
/** Last descriptor in the receive buffer descriptor list */
#define GMAC_RXW0_WRAP      (0x1u << 1)
/** Address of beginning of buffer */
#define GMAC_RXW0_ADDR      (0x3FFFFFFFu << 2)

/** Receive frame length including FCS */
#define GMAC_RXW1_LEN               (0x1FFFu << 0)
/** FCS status */
#define GMAC_RXW1_FCS_STATUS        (0x1u << 13)
/** Start of frame */
#define GMAC_RXW1_SOF               (0x1u << 14)
/** End of frame */
#define GMAC_RXW1_EOF               (0x1u << 15)
/** Canonical Format Indicator */
#define GMAC_RXW1_CFI               (0x1u << 16)
/** VLAN priority (if VLAN detected) */
#define GMAC_RXW1_VLANPRIORITY      (0x7u << 17)
/** Priority tag detected */
#define GMAC_RXW1_PRIORITYDETECTED  (0x1u << 20)
/** VLAN tag detected */
#define GMAC_RXW1_VLANDETECTED      (0x1u << 21)
/** Type ID match */
#define GMAC_RXW1_TYPEIDMATCH       (0x3u << 22)
/** Type ID register match found */
#define GMAC_RXW1_TYPEIDFOUND       (0x1u << 24)
/** Specific Address Register match */
#define GMAC_RXW1_ADDRMATCH         (0x3u << 25)
/** Specific Address Register match found */
#define GMAC_RXW1_ADDRFOUND         (0x1u << 27)
/** Unicast hash match */
#define GMAC_RXW1_UNIHASHMATCH      (0x1u << 29)
/** Multicast hash match */
#define GMAC_RXW1_MULTIHASHMATCH    (0x1u << 30)
/** Global all ones broadcast address detected */
#define GMAC_RXW1_BROADCASTDETECTED (0x1u << 31)

/*
 * Transmit buffer descriptor bit field definitions
 */

/** Transmit buffer length */
#define GMAC_TXW1_LEN        (0x3FFFu << 0)
/** Last buffer in the current frame */
#define GMAC_TXW1_LASTBUFFER (0x1u << 15)
/** No CRC */
#define GMAC_TXW1_NOCRC      (0x1u << 16)
/** Transmit IP/TCP/UDP checksum generation offload errors */
#define GMAC_TXW1_CHKSUMERR  (0x7u << 20)
/** Late collision, transmit error detected */
#define GMAC_TXW1_LATECOLERR (0x1u << 26)
/** Transmit frame corruption due to AHB error */
#define GMAC_TXW1_TRANSERR   (0x1u << 27)
/** Retry limit exceeded, transmit error detected */
#define GMAC_TXW1_RETRYEXC   (0x1u << 29)
/** Last descriptor in Transmit Descriptor list */
#define GMAC_TXW1_WRAP       (0x1u << 30)
/** Buffer used, must be 0 for the GMAC to read data to the transmit buffer */
#define GMAC_TXW1_USED       (0x1u << 31)

/*
 * Interrupt Status/Enable/Disable/Mask register bit field definitions
 */

#define GMAC_INT_RX_ERR_BITS (GMAC_IER_RXUBR_Msk | GMAC_IER_ROVR_Msk)

#define GMAC_INT_TX_ERR_BITS (GMAC_IER_TUR_Msk | GMAC_IER_RLEX_Msk | GMAC_IER_TFC_Msk)

#define GMAC_INT_EN_FLAGS                                                                          \
	(GMAC_IER_RCOMP_Msk | GMAC_INT_RX_ERR_BITS | GMAC_IER_TCOMP_Msk | GMAC_INT_TX_ERR_BITS |   \
	 GMAC_IER_HRESP_Msk)

/** GMAC Priority Queues DMA flags */
#define GMAC_DMA_QUEUE_FLAGS (0)

/** List of GMAC queues */
enum queue_idx {
	GMAC_QUE_0, /** Main queue */
};

#if (DT_INST_PROP(0, max_frame_size) == 1518)
/* Maximum frame length is 1518 bytes */
#define GMAC_MAX_FRAME_SIZE 0
#elif (DT_INST_PROP(0, max_frame_size) == 1536)
/* Enable Max Frame Size of 1536 */
#define GMAC_MAX_FRAME_SIZE GMAC_NCFGR_MAXFS
#elif (DT_INST_PROP(0, max_frame_size) == 10240)
/* Jumbo Frame Enable */
#define GMAC_MAX_FRAME_SIZE GMAC_NCFGR_JFRAME
#else
#error "GMAC_MAX_FRAME_SIZE is invalid, fix it at device tree."
#endif

/** Receive/transmit buffer descriptor */
struct eth_mchp_gmac_desc {
	uint32_t w0;
	uint32_t w1;
};

/** Ring list of receive/transmit buffer descriptors */
struct eth_mchp_gmac_desc_list {
	struct eth_mchp_gmac_desc *buf;
	uint16_t len;
	uint16_t head;
	uint16_t tail;
};

/* Device run time data */
/**
 * @brief Queue related for the Eth device.
 *
 * This structure contains the run queue related for the Eth device.
 */
struct eth_mchp_gmac_queue {
	/* Rx descriptor list */
	struct eth_mchp_gmac_desc_list rx_desc_list;

	/* Tx descriptor list */
	struct eth_mchp_gmac_desc_list tx_desc_list;

	/* Transmit Semaphore */
	struct k_sem tx_sem;

	/* Fragment list associated with a frame */
	struct net_buf **rx_frag_list;

	/* Number of RX frames dropped by the driver */
	volatile uint32_t err_rx_frames_dropped;

	/* Number of times receive queue was flushed */
	volatile uint32_t err_rx_flushed_count;

	/* Number of times transmit queue was flushed */
	volatile uint32_t err_tx_flushed_count;

	/* Queue Index */
	enum queue_idx que_idx;
};

/**
 * @brief Run time data structure for the Eth peripheral.
 *
 * This structure contains the run time parameters for the Eth
 * device, including statistics.
 */
typedef struct eth_mchp_dev_data {
	/* net iface structure has to be the first field */
	struct net_if *iface;

	/* MAC Address */
	uint8_t mac_addr[6];

	/* Link Status */
	bool link_up;

	/* queue list */
	struct eth_mchp_gmac_queue queue_list[GMAC_QUEUE_NUM];

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	/* eth frame statistics */
	struct net_stats_eth stats;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
} eth_mchp_dev_data_t;

/**
 * @brief Clock configuration structure for the ETH.
 *
 * This structure contains the clock configuration parameters for the ETH
 * device.
 */
typedef struct mchp_eth_clock {
	/* Clock driver */
	const struct device *clock_dev;

	/* Main APB clock subsystem. */
	clock_control_mchp_subsys_t mclk_apb_sys;

	/* Main AHB clock subsystem. */
	clock_control_mchp_subsys_t mclk_ahb_sys;

	/* Generic clock subsystem. */
	clock_control_mchp_subsys_t gclk_sys;

	/* Osc clock subsystem. */
	clock_control_mchp_subsys_t oscctrl_sys;
} mchp_eth_clock_t;

/* Device constant configuration parameters */

/**
 * @brief device configuration structure for the ETH device.
 *
 * This structure contains the Device constant configuration parameters
 * for the ETH device.
 */
typedef struct eth_mchp_dev_config {
	/* GMAC register */
	gmac_registers_t *const regs;

	/* Pin Control structure */
	const struct pinctrl_dev_config *pcfg;

	/* Configuration function pointer */
	void (*config_func)(void);

	/* Pointer to Phy device */
	const struct device *phy_dev;

	/* clock device configuration */
	mchp_eth_clock_t eth_clock;
} eth_mchp_dev_config_t;

#define ETH_MCHP_CLOCK_DEFN(n)                                                                     \
	.eth_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                 \
	.eth_clock.mclk_apb_sys = {.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, mclk_apb)), \
				   .id = DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_apb, id)},            \
	.eth_clock.mclk_ahb_sys = {.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, mclk_ahb)), \
				   .id = DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_ahb, id)},            \
	.eth_clock.gclk_sys = {.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, gclk)),         \
			       .id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, id)},                    \
	.eth_clock.oscctrl_sys = {.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, oscctrl)),   \
				  .id = DT_INST_CLOCKS_CELL_BY_NAME(n, oscctrl, id)}

#define ETH_MCHP_CONFIGURE_CLOCK(dev, subsys, data) clock_control_configure(dev, &subsys, &data)

#define ETH_MCHP_GET_CLOCK_FREQ(dev, subsys, rate) clock_control_get_rate(dev, &subsys, &rate)

#define ETH_MCHP_ENABLE_CLOCK(dev)                                                                 \
	clock_control_on(((const eth_mchp_dev_config_t *)(dev->config))->eth_clock.clock_dev,      \
			 &(((eth_mchp_dev_config_t *)(dev->config))->eth_clock.mclk_apb_sys));     \
	clock_control_on(((const eth_mchp_dev_config_t *)(dev->config))->eth_clock.clock_dev,      \
			 &(((eth_mchp_dev_config_t *)(dev->config))->eth_clock.mclk_ahb_sys));     \
	clock_control_on(((const eth_mchp_dev_config_t *)(dev->config))->eth_clock.clock_dev,      \
			 &(((eth_mchp_dev_config_t *)(dev->config))->eth_clock.gclk_sys));         \
	clock_control_on(((const eth_mchp_dev_config_t *)(dev->config))->eth_clock.clock_dev,      \
			 &(((eth_mchp_dev_config_t *)(dev->config))->eth_clock.oscctrl_sys))

/*
 * Verify Kconfig configuration
 */
#if CONFIG_NET_BUF_DATA_SIZE * CONFIG_ETH_MCHP_BUF_RX_COUNT < GMAC_FRAME_SIZE_MAX
#error CONFIG_NET_BUF_DATA_SIZE * CONFIG_ETH_SAM_GMAC_BUF_RX_COUNT is \
	not large enough to hold a full frame
#endif

#if CONFIG_NET_BUF_DATA_SIZE * (CONFIG_NET_BUF_RX_COUNT - CONFIG_ETH_MCHP_BUF_RX_COUNT) <          \
	GMAC_FRAME_SIZE_MAX
#error (CONFIG_NET_BUF_RX_COUNT - CONFIG_ETH_SAM_GMAC_BUF_RX_COUNT) * \
	CONFIG_NET_BUF_DATA_SIZE are not large enough to hold a full frame
#endif

#if CONFIG_NET_BUF_DATA_SIZE & 0x3F
#pragma message "CONFIG_NET_BUF_DATA_SIZE should be a multiple of 64 bytes "                       \
		"due to the granularity of RX DMA"
#endif

#if (CONFIG_ETH_MCHP_BUF_RX_COUNT + 1) * GMAC_ACTIVE_QUEUE_NUM > CONFIG_NET_BUF_RX_COUNT
#error Not enough RX buffers to allocate descriptors for each HW queue
#endif

BUILD_ASSERT(DT_INST_ENUM_IDX(0, phy_connection_type) <= 1, "Invalid PHY connection");

/** RX descriptors list */
static struct eth_mchp_gmac_desc rx_desc_que0[MAIN_QUEUE_RX_DESC_COUNT] __nocache
	__aligned(GMAC_DESC_ALIGNMENT);

/** TX descriptors list */
static struct eth_mchp_gmac_desc tx_desc_que0[MAIN_QUEUE_TX_DESC_COUNT] __nocache
	__aligned(GMAC_DESC_ALIGNMENT);

/** RX buffer accounting list */
static struct net_buf *rx_frag_list_que0[MAIN_QUEUE_RX_DESC_COUNT];

#define MODULO_INC(val, max)                                                                       \
	{                                                                                          \
		val = (++val < max) ? val : 0;                                                     \
	}

#ifdef __DCACHE_PRESENT
static bool dcache_enabled;

/**
 * @brief Get Data Cache Status.
 *
 * This function is used to find if the Data cache is enabled or not.
 *
 * @return true if enabled, else false.
 */
static inline void eth_mchp_dcache_is_enabled(void)
{
	dcache_enabled = (SCB->CCR & SCB_CCR_DC_Msk);
}

/**
 * @brief Invalidate the cache by address.
 *
 * This function is used to invalidate the data cache for a certain size
 * starting from addr. The function ensures cache coherency after
 * DMA write operation.
 *
 * @param addr memory address.
 * @param size size of the memory pointed to by the 'addr'.
 */
static inline void eth_mchp_dcache_invalidate(uint32_t addr, uint32_t size)
{
	if (!dcache_enabled) {
		return;
	}

	/* Make sure it is aligned to 32B */
	uint32_t start_addr = addr & (uint32_t)~(GMAC_DCACHE_ALIGNMENT - 1);
	uint32_t size_full = size + addr - start_addr;

	SCB_InvalidateDCache_by_Addr((uint32_t *)start_addr, size_full);
}

/**
 * @brief Clean the cache by address.
 *
 * This function is used to clean the data cache for a certain size
 * starting from addr.
 *
 * @param addr memory address.
 * @param size size of the memory pointed to by the 'addr'.
 */
static inline void eth_mchp_dcache_clean(uint32_t addr, uint32_t size)
{
	if (!dcache_enabled) {
		return;
	}

	/* Make sure it is aligned to 32B */
	uint32_t start_addr = addr & (uint32_t)~(GMAC_DCACHE_ALIGNMENT - 1);
	uint32_t size_full = size + addr - start_addr;

	SCB_CleanDCache_by_Addr((uint32_t *)start_addr, size_full);
}
#else
#define eth_mchp_dcache_is_enabled()
#define eth_mchp_dcache_invalidate(addr, size)
#define eth_mchp_dcache_clean(addr, size)
#endif

static void eth_mchp_rx(struct eth_mchp_gmac_queue *queue);

/**
 * @brief Free pre-reserved RX buffers.
 *
 * This function is used to free pre-reserved RX buffers.
 *
 * @param rx_frag_list Pointer to the fragment list.
 * @param len No of fragments in the list.
 */
static void hal_mchp_eth_free_rx_bufs(struct net_buf **rx_frag_list, uint16_t len)
{
	for (int i = 0; i < len; i++) {
		if (rx_frag_list[i]) {
			net_buf_unref(rx_frag_list[i]);
			rx_frag_list[i] = NULL;
		}
	}
}

/**
 * @brief Initialize RX descriptor list.
 *
 * This function is used to Initialize RX descriptor list
 *
 * @param gmac Pointer to gmac registers.
 * @param queue Pointer to gmac queue structure.
 *
 * @return 0 if successful, -ENOBUFS if no buffers available.
 */
static inline int hal_mchp_eth_rx_descriptors_init(gmac_registers_t *gmac,
						   struct eth_mchp_gmac_queue *queue)
{
	struct eth_mchp_gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct net_buf **rx_frag_list = queue->rx_frag_list;
	struct net_buf *rx_buf;
	uint8_t *rx_buf_addr;

	__ASSERT_NO_MSG(rx_frag_list);

	rx_desc_list->tail = 0U;

	for (int i = 0; i < rx_desc_list->len; i++) {
		rx_buf = net_pkt_get_reserve_rx_data(CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
		if (rx_buf == NULL) {
			hal_mchp_eth_free_rx_bufs(rx_frag_list, rx_desc_list->len);
			LOG_ERR("Failed to reserve data net buffers");
			return -ENOBUFS;
		}

		rx_frag_list[i] = rx_buf;

		rx_buf_addr = rx_buf->data;
		__ASSERT(!((uint32_t)rx_buf_addr & ~GMAC_RXW0_ADDR),
			 "Misaligned RX buffer address");
		__ASSERT(rx_buf->size == CONFIG_NET_BUF_DATA_SIZE,
			 "Incorrect length of RX data buffer");

		/* Give ownership to GMAC and remove the wrap bit */
		rx_desc_list->buf[i].w0 = (uint32_t)rx_buf_addr & GMAC_RXW0_ADDR;
		rx_desc_list->buf[i].w1 = 0U;
	}

	/* Set the wrap bit on the last descriptor */
	rx_desc_list->buf[rx_desc_list->len - 1U].w0 |= GMAC_RXW0_WRAP;

	return 0;
}

/**
 * @brief Initialize TX descriptor list.
 *
 * This function is used to Initialize TX descriptor list.
 *
 * @param gmac Pointer to gmac registers.
 * @param queue Pointer to gmac queue structure.
 *
 * @return 0 if successful, -ENOBUFS if no buffers available.
 */
static inline void hal_mchp_eth_tx_descriptors_init(gmac_registers_t *gmac,
						    struct eth_mchp_gmac_queue *queue)
{
	struct eth_mchp_gmac_desc_list *tx_desc_list = &queue->tx_desc_list;

	tx_desc_list->head = 0U;
	tx_desc_list->tail = 0U;

	for (int i = 0; i < tx_desc_list->len; i++) {
		tx_desc_list->buf[i].w0 = 0U;
		tx_desc_list->buf[i].w1 = GMAC_TXW1_USED;
	}

	/* Set the wrap bit on the last descriptor */
	tx_desc_list->buf[tx_desc_list->len - 1U].w1 |= GMAC_TXW1_WRAP;
}

/**
 * @brief Initialize Non-Priority queue.
 *
 * This function is used to Initialize Non-Priority queue.
 *
 * @param gmac Pointer to gmac registers.
 * @param queue Pointer to gmac queue structure.
 *
 * @return 0 if successful, -ENOBUFS if no buffers available.
 */
static inline int hal_mchp_eth_nonpriority_queue_init(gmac_registers_t *gmac,
						      struct eth_mchp_gmac_queue *queue)
{
	int result;

	__ASSERT_NO_MSG(queue->rx_desc_list.len > 0);
	__ASSERT_NO_MSG(queue->tx_desc_list.len > 0);
	__ASSERT(!((uint32_t)queue->rx_desc_list.buf & ~GMAC_RBQB_ADDR_Msk),
		 "RX descriptors have to be word aligned");
	__ASSERT(!((uint32_t)queue->tx_desc_list.buf & ~GMAC_TBQB_ADDR_Msk),
		 "TX descriptors have to be word aligned");

	/* Setup descriptor lists */
	result = hal_mchp_eth_rx_descriptors_init(gmac, queue);
	if (result < 0) {
		return result;
	}

	hal_mchp_eth_tx_descriptors_init(gmac, queue);

	/* Initialize TX semaphore. This semaphore is used to wait until the TX
	 * data has been sent.
	 */
	k_sem_init(&queue->tx_sem, 0, 1);

	/* Set Receive Buffer Queue Pointer Register */
	gmac->GMAC_RBQB = (uint32_t)queue->rx_desc_list.buf;

	/* Set Transmit Buffer Queue Pointer Register */
	gmac->GMAC_TBQB = (uint32_t)queue->tx_desc_list.buf;

	/* Configure GMAC DMA transfer */
	gmac->GMAC_DCFGR =

		/* Receive Buffer Size (defined in multiples of 64 bytes) */
		GMAC_DCFGR_DRBS(CONFIG_NET_BUF_DATA_SIZE >> 6) |
#if defined(GMAC_DCFGR_RXBMS)
		/* Use full receive buffer size on parts where this is selectable */
		GMAC_DCFGR_RXBMS(3) |
#endif
		/* Attempt to use INCR4 AHB bursts (Default) */
		GMAC_DCFGR_FBLDO_INCR4 |
		/* DMA Queue Flags */
		GMAC_DMA_QUEUE_FLAGS;

	/* Setup RX/TX completion and error interrupts */
	gmac->GMAC_IER = GMAC_INT_EN_FLAGS;

	queue->err_rx_frames_dropped = 0U;
	queue->err_rx_flushed_count = 0U;
	queue->err_tx_flushed_count = 0U;

	LOG_INF("Queue %d activated", queue->que_idx);

	return 0;
}

/**
 * @brief Sets the receive buffer queue pointer in the appropriate register.
 *
 * This function is used to set the receive buffer queue pointer in
 * the appropriate register.
 *
 * @param gmac Pointer to gmac registers.
 * @param queue Pointer to gmac queue structure.
 */
static inline void hal_mchp_eth_set_receive_buf_queue_pointer(gmac_registers_t *gmac,
							      struct eth_mchp_gmac_queue *queue)
{
	gmac->GMAC_RBQB = (uint32_t)queue->rx_desc_list.buf;
}

/**
 * @brief Sets the receive buffer queue pointer in the appropriate register.
 *
 * This function is used to set the receive buffer queue pointer in
 * the appropriate register.
 *
 * @param gmac Pointer to gmac registers.
 * @param queue Pointer to gmac queue structure.
 *
 * @return 0 if successful, -ENOBUFS if no buffers available.
 */
static inline int hal_mchp_eth_queue_init(gmac_registers_t *gmac, struct eth_mchp_gmac_queue *queue)
{
	return hal_mchp_eth_nonpriority_queue_init(gmac, queue);
}

/**
 * @brief Set MAC Address for frame filtering logic.
 *
 * This function is used to set MAC Address for frame filtering logic.
 *
 * @param gmac Pointer to gmac registers.
 * @param index Index.
 * @param mac_addr MAC Address.
 */
static void hal_mchp_eth_mac_addr_set(gmac_registers_t *gmac, uint8_t index, uint8_t mac_addr[6])
{
	uint32_t bottom_addr = 0;
	uint32_t top_addr = 0;

	__ASSERT(index < 4, "index has to be in the range 0..3");

	bottom_addr =
		(mac_addr[3] << 24) | (mac_addr[2] << 16) | (mac_addr[1] << 8) | (mac_addr[0]);
	gmac->SA[index].GMAC_SAB = GMAC_SAB_ADDR(bottom_addr);

	top_addr = (mac_addr[5] << 8) | (mac_addr[4]);
	gmac->SA[index].GMAC_SAT = GMAC_SAT_ADDR(top_addr);
}

/**
 * @brief Process successfully sent packets.
 *
 * This function is used to process successfully sent packets.
 *
 * @param gmac Pointer to gmac registers.
 * @param queue Pointer to gmac queue structure.
 */
static inline void hal_mchp_eth_tx_completed(gmac_registers_t *gmac,
					     struct eth_mchp_gmac_queue *queue)
{
	k_sem_give(&queue->tx_sem);
}

/**
 * @brief Reset TX queue when errors are detected.
 *
 * This function is used to reset TX queue when errors are detected.
 *
 * @param hal Pointer to HAL structure.
 * @param queue Pointer to gmac queue structure.
 */
static inline void hal_mchp_eth_tx_error_handler(gmac_registers_t *gmac,
						 struct eth_mchp_gmac_queue *queue)
{
	queue->err_tx_flushed_count++;

	/* Stop transmission, clean transmit pipeline and control registers */
	gmac->GMAC_NCR &= ~GMAC_NCR_TXEN_Msk;

	hal_mchp_eth_tx_descriptors_init(gmac, queue);

	/* Reinitialize TX mutex */
	k_sem_give(&queue->tx_sem);

	/* Restart transmission */
	gmac->GMAC_NCR |= GMAC_NCR_TXEN_Msk;
}

/**
 * @brief Clean RX queue, any received data still stored in the buffers is abandoned.
 *
 * This function is used to clean RX queue, any received data still stored
 * in the buffers is abandoned.
 *
 * @param gmac Pointer to gmac registers.
 * @param queue Pointer to gmac queue structure.
 */
static inline void hal_mchp_eth_rx_error_handler(gmac_registers_t *gmac,
						 struct eth_mchp_gmac_queue *queue)
{
	queue->err_rx_flushed_count++;

	/* Stop reception */
	gmac->GMAC_NCR &= ~GMAC_NCR_RXEN_Msk;

	queue->rx_desc_list.tail = 0U;

	/* clean RX queue */
	for (int i = 0; i < queue->rx_desc_list.len; i++) {
		queue->rx_desc_list.buf[i].w1 = 0U;
		queue->rx_desc_list.buf[i].w0 &= ~GMAC_RXW0_OWNERSHIP;
	}

	hal_mchp_eth_set_receive_buf_queue_pointer(gmac, queue);

	/* Restart reception */
	gmac->GMAC_NCR |= GMAC_NCR_RXEN_Msk;
}

/**
 * @brief Set MCK to MDC clock divisor.
 *
 * This function is used to set MCK to MDC clock divisor. Also, according to
 * 802.3 MDC should be less then 2.5 MHz
 *
 * @param mck valid frequency.
 *
 * @return divisor if successful, -ENOTSUP if not supprted.
 */
static inline int hal_mchp_eth_get_mck_clock_divisor(uint32_t mck)
{
	uint32_t mck_divisor;

	if (mck <= 20000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK8;
	} else if (mck <= 40000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK16;
	} else if (mck <= 80000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK32;
	} else if (mck <= 120000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK48;
	} else if (mck <= 160000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK64;
	} else if (mck <= 240000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK96;
	} else {
		LOG_ERR("No valid MDC clock");
		mck_divisor = -ENOTSUP;
	}

	LOG_INF("mck %d mck_divisor = 0x%x", mck, mck_divisor);

	return mck_divisor;
}

/**
 * @brief Initialize the registers, interrupts.
 *
 * This function is used to Initialize and configure the registers,
 * and interrupts.
 *
 * @param gmac Pointer to gmac registers.
 * @param gmac_ncfgr_val value to be set in the register.
 * @param clk_freq_hz clock frequency.
 *
 * @return divisor if successful, -ENOTSUP if not supprted.
 */
static inline int hal_mchp_eth_gmac_init(gmac_registers_t *gmac, uint32_t gmac_ncfgr_val,
					 uint32_t clk_freq_hz)
{
	int mck_divisor;

	mck_divisor = hal_mchp_eth_get_mck_clock_divisor(clk_freq_hz);
	if (mck_divisor < 0) {
		return mck_divisor;
	}

	/* Set Network Control Register to its default value, clear stats. */
	gmac->GMAC_NCR = GMAC_NCR_CLRSTAT_Msk | GMAC_NCR_MPE_Msk;

	/* Disable all interrupts */
	gmac->GMAC_IDR = UINT32_MAX;
	/* Clear all interrupts */
	(void)gmac->GMAC_ISR;

	/* Setup Hash Registers - enable reception of all
	 * multicast frames when GMAC_NCFGR_MTIHEN is set.
	 */
	gmac->GMAC_HRB = UINT32_MAX;
	gmac->GMAC_HRT = UINT32_MAX;

	/* Setup Network Configuration Register */
	gmac->GMAC_NCFGR = gmac_ncfgr_val | mck_divisor;

	/* Default (RMII) is defined at atmel,gmac-common.yaml file */
	switch (DT_INST_ENUM_IDX(0, phy_connection_type)) {
	case 0: /* mii */
		gmac->GMAC_UR = 0x1;
		break;
	case 1: /* rmii */
		gmac->GMAC_UR = 0x0;
		break;
	default:
		/* Build assert at top of file should catch this case */
		LOG_ERR("The phy connection type is invalid");

		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Initialize the registers, interrupts.
 *
 * This function is used to Initialize and configure the registers,
 * and interrupts.
 *
 * @param gmac Pointer to gmac registers.
 * @param full_duplex if full duplex or not.
 * @param speed_100M if speed is 100M or not.
 *
 * @return divisor if successful, -ENOTSUP if not supprted.
 */
static inline void hal_mchp_eth_link_configure(gmac_registers_t *gmac, bool full_duplex,
					       bool speed_100M)
{
	uint32_t val;

	val = gmac->GMAC_NCFGR;

	val &= ~(GMAC_NCFGR_FD_Msk | GMAC_NCFGR_SPD_Msk);
	val |= (full_duplex) ? GMAC_NCFGR_FD_Msk : 0;
	val |= (speed_100M) ? GMAC_NCFGR_SPD_Msk : 0;

	gmac->GMAC_NCFGR = val;

	gmac->GMAC_NCR |= (GMAC_NCR_RXEN_Msk | GMAC_NCR_TXEN_Msk);
}

/**
 * @brief Set the register to start the transmission.
 *
 * This function is used to set the register to start the transmission.
 *
 * @param gmac Pointer to gmac registers.
 */
static inline void hal_mchp_eth_tx(gmac_registers_t *gmac)
{
	/* Start transmission */
	gmac->GMAC_NCR |= GMAC_NCR_TSTART_Msk;
}

/**
 * @brief ISR to handle the interrupt.
 *
 * This ISR is used to handle interrupt related to reception of frame, or
 * completion of transmission of a frame.
 *
 * @param gmac Pointer to gmac registers.
 * @param queue Pointer to gmac queue structure.
 */
static inline void hal_mchp_eth_queue0_isr(gmac_registers_t *gmac,
					   struct eth_mchp_gmac_queue *queue)
{
	struct eth_mchp_gmac_desc_list *rx_desc_list;
	struct eth_mchp_gmac_desc_list *tx_desc_list;
	struct eth_mchp_gmac_desc *tail_desc;
	uint32_t isr;

	/* Interrupt Status Register is cleared on read */
	isr = gmac->GMAC_ISR;
	LOG_DBG("GMAC_ISR=0x%08x", isr);

	rx_desc_list = &queue->rx_desc_list;
	tx_desc_list = &queue->tx_desc_list;

	/* Check if packet received */
	if (isr & GMAC_INT_RX_ERR_BITS) {
		hal_mchp_eth_rx_error_handler(gmac, queue);
	} else if (isr & GMAC_ISR_RCOMP_Msk) {
		tail_desc = &rx_desc_list->buf[rx_desc_list->tail];
		LOG_DBG("rx.w1=0x%08x, tail=%d", tail_desc->w1, rx_desc_list->tail);
		eth_mchp_rx(queue);
	}

	/* Check if TX packet completion */
	if (isr & GMAC_INT_TX_ERR_BITS) {
		hal_mchp_eth_tx_error_handler(gmac, queue);
	} else if (isr & GMAC_ISR_TCOMP_Msk) {
		hal_mchp_eth_tx_completed(gmac, queue);
	}

	if (isr & GMAC_IER_HRESP_Msk) {
		LOG_DBG("IER HRESP");
	}
}

#if DT_INST_NODE_HAS_PROP(0, mac_eeprom)
/**
 * @brief Read MAC address from EEPROM
 *
 * This function is used to read MAC address from EEPROM.
 *
 * @param mac_addr buffer for the read mac address.
 */
static void hal_mchp_eth_get_mac_addr_from_i2c_eeprom(uint8_t mac_addr[6])
{
	uint32_t iaddr = CONFIG_ETH_MCHP_MAC_I2C_INT_ADDRESS;
	int ret;
	const struct i2c_dt_spec i2c = I2C_DT_SPEC_GET(DT_INST_PHANDLE(0, mac_eeprom));

	/* Check if the I2C bus is ready */
	if (!device_is_ready(i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return;
	}

	/* Read the MAc address */
	ret = i2c_write_read_dt(&i2c, &iaddr, CONFIG_ETH_MCHP_MAC_I2C_INT_ADDRESS_SIZE, mac_addr,
				6);
	if (ret != 0) {
		LOG_ERR("I2C: failed to read MAC addr");
		return;
	}
}
#endif

/**
 * @brief Set MAC address
 *
 * This function is used to set the MAC address either by reading it from EEPROM,
 * or generating it.
 *
 * @param gmac Pointer to gmac registers.
 * @param mac_addr buffer for the read mac address.
 */
static inline void hal_mchp_eth_generate_set_mac(gmac_registers_t *gmac, uint8_t mac_addr[6])
{

#if DT_INST_NODE_HAS_PROP(0, mac_eeprom)
	hal_mchp_eth_get_mac_addr_from_i2c_eeprom(mac_addr);
#elif DT_INST_PROP(0, zephyr_random_mac_address)
	gen_random_mac(mac_addr, MCHP_OUI_B0, MCHP_OUI_B1, MCHP_OUI_B2);
#endif

	/* Set MAC Address for frame filtering logic */
	hal_mchp_eth_mac_addr_set(gmac, 0, mac_addr);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
/**
 * @brief Get Eth MAC stats
 *
 * This function is used to get the statistics related to Eth MAC device.
 *
 * @param gmac Pointer to gmac registers.
 * @param eth_stats Pointer to structure in which to fill in the stats data.
 */
static inline void hal_mchp_eth_get_stats(gmac_registers_t *gmac, struct net_stats_eth *eth_stats)
{
	uint32_t ae = gmac->GMAC_AE;    /* Alignment Error */
	uint32_t ofr = gmac->GMAC_OFR;  /* Over Length Frames */
	uint32_t fcs = gmac->GMAC_FCSE; /* FCS Error */
	uint32_t scf = gmac->GMAC_SCF;  /* Single Collision */
	uint32_t mcf = gmac->GMAC_MCF;  /* Multiple Collision */
	uint32_t ecf = gmac->GMAC_EC;   /* Excess Collision */

	/*
	 * No ned to update the bytes rx/tx; bcast rx/tx; pkt rx/tx
	 * as it is being updated in the net subsystem via the
	 * eth_stats_update_* APIs in eth_stats.h
	 */

	eth_stats->collisions += scf + mcf + ecf;

	eth_stats->csum.rx_csum_offload_errors += gmac->GMAC_FCSE;
	eth_stats->csum.rx_csum_offload_good = 0;

	eth_stats->error_details.rx_align_errors += ae;
	eth_stats->error_details.rx_long_length_errors += ofr;
	eth_stats->error_details.rx_crc_errors += fcs;
	eth_stats->error_details.rx_length_errors += ofr;

	eth_stats->errors.rx += gmac->GMAC_UCE + gmac->GMAC_TCE + gmac->GMAC_IHCE + gmac->GMAC_ROE +
				gmac->GMAC_RRE + ae + gmac->GMAC_RSE + gmac->GMAC_LFFE + fcs +
				gmac->GMAC_JR + ofr + gmac->GMAC_UFR;

	eth_stats->errors.tx += scf + mcf + ecf + gmac->GMAC_TUR + gmac->GMAC_CSE;

	eth_stats->multicast.rx += gmac->GMAC_MFR;
	eth_stats->multicast.tx += gmac->GMAC_MFT;

	/* eth_stats->flow_control */

	/* eth_stats->hw_timestamp.rx_hwtstamp_cleared */

	eth_stats->tx_dropped = 0;
	eth_stats->tx_restart_queue = 0;
	eth_stats->tx_timeout_count = 0;
	eth_stats->unknown_protocol = 0;

#ifdef CONFIG_NET_STATISTICS_ETHERNET_VENDOR
	eth_stats->vendor.key = NULL;
	eth_stats->vendor.value = 0;
#endif /* CONFIG_NET_STATISTICS_ETHERNET_VENDOR */
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

/**
 * @brief Get the net interface handle associated with the Eth MAC driver
 *
 * This function is used to get the net interface handle associated with
 * the Eth MAC to pass the received packet to it.
 *
 * @param ctx data context of the Eth MAC driver.
 *
 * @return net_if* Pointer to the net interface structure
 */
static inline struct net_if *eth_mchp_get_iface(eth_mchp_dev_data_t *ctx)
{
	return ctx->iface;
}

/**
 * @brief Get the frame from the Rx Queue
 *
 * This function is used to get the frame received from the network from
 * the queue.
 *
 * @param queue Pointer to the queue onwhich the fram has been received.
 *
 * @return net_pkt* Pointer to the buffer containing the complete frame.
 */
static struct net_pkt *eth_mchp_frame_get(struct eth_mchp_gmac_queue *queue)
{
	struct eth_mchp_gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct eth_mchp_gmac_desc *rx_desc;
	struct net_buf **rx_frag_list = queue->rx_frag_list;
	struct net_pkt *rx_frame;
	bool frame_is_complete;
	struct net_buf *frag;
	struct net_buf *new_frag;
	struct net_buf *last_frag = NULL;
	uint8_t *frag_data;
	uint32_t frag_len;
	uint32_t frame_len = 0U;
	uint16_t tail;
	uint8_t wrap;

	/* Check if there exists a complete frame in RX descriptor list */
	tail = rx_desc_list->tail;
	rx_desc = &rx_desc_list->buf[tail];
	frame_is_complete = false;
	while ((rx_desc->w0 & GMAC_RXW0_OWNERSHIP) && !frame_is_complete) {

		if (!(uint8_t *)(rx_desc->w0 & GMAC_RXW0_ADDR)) {
			rx_desc->w0 &= ~GMAC_RXW0_OWNERSHIP;
			return NULL;
		}
		frame_is_complete = (bool)(rx_desc->w1 & GMAC_RXW1_EOF);
		MODULO_INC(tail, rx_desc_list->len);
		rx_desc = &rx_desc_list->buf[tail];
	}
	/* Frame which is not complete can be dropped by GMAC. Do not process
	 * it, even partially.
	 */
	if (!frame_is_complete) {
		return NULL;
	}

	/* Process a frame */
	tail = rx_desc_list->tail;
	rx_desc = &rx_desc_list->buf[tail];
	frame_is_complete = false;

	if (rx_desc->w1 & GMAC_RXW1_SOF) {
		rx_frame = net_pkt_rx_alloc(K_NO_WAIT);
	} else {
		/* TODO: Don't assume first RX fragment will have SOF (Start of frame)
		 * bit set. If SOF bit is missing recover gracefully by dropping
		 * invalid frame.
		 */
		return NULL;
	}

	while ((rx_desc->w0 & GMAC_RXW0_OWNERSHIP) && !frame_is_complete) {
		frag = rx_frag_list[tail];
		frag_data = (uint8_t *)(rx_desc->w0 & GMAC_RXW0_ADDR);

		__ASSERT(frag->data == frag_data, "RX descriptor and buffer list desynchronized");

		frame_is_complete = (bool)(rx_desc->w1 & GMAC_RXW1_EOF);
		if (frame_is_complete) {
			frag_len = (rx_desc->w1 & GMAC_RXW1_LEN) - frame_len;
		} else {
			frag_len = CONFIG_NET_BUF_DATA_SIZE;
		}

		frame_len += frag_len;

		/* Link frame fragments only if RX net buffer is valid */
		if (rx_frame != NULL) {
			/* Assure cache coherency after DMA write operation */
			eth_mchp_dcache_invalidate((uint32_t)frag_data, frag->size);

			/* Get a new data net buffer from the buffer pool */
			new_frag = net_pkt_get_frag(rx_frame, CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
			if (new_frag == NULL) {
				queue->err_rx_frames_dropped++;
				net_pkt_unref(rx_frame);
				rx_frame = NULL;
			} else {
				net_buf_add(frag, frag_len);
				if (!last_frag) {
					net_pkt_frag_insert(rx_frame, frag);
				} else {
					net_buf_frag_insert(last_frag, frag);
				}
				last_frag = frag;
				frag = new_frag;
				rx_frag_list[tail] = frag;
			}
		}

		/* Update buffer descriptor status word */
		rx_desc->w1 = 0U;

		/* Guarantee that status word is written before the address
		 * word to avoid race condition.
		 */
		barrier_dmem_fence_full();

		/* Update buffer descriptor address word */
		wrap = (tail == rx_desc_list->len - 1U ? GMAC_RXW0_WRAP : 0);
		rx_desc->w0 = ((uint32_t)frag->data & GMAC_RXW0_ADDR) | wrap;

		MODULO_INC(tail, rx_desc_list->len);
		rx_desc = &rx_desc_list->buf[tail];
	}

	rx_desc_list->tail = tail;
	LOG_DBG("Frame complete: rx=%p, tail=%d", rx_frame, tail);
	__ASSERT_NO_MSG(frame_is_complete);

	return rx_frame;
}

/**
 * @brief Get the frame from the Rx Queue and pass it onto the net interface.
 *
 * This function is used to get the frame received from the network from
 * the queue, and pass it on to the net interface layer.
 *
 * @param queue Pointer to the queue onwhich the fram has been received.
 */
static void eth_mchp_rx(struct eth_mchp_gmac_queue *queue)
{
	eth_mchp_dev_data_t *dev_data =
		CONTAINER_OF(queue, eth_mchp_dev_data_t, queue_list[queue->que_idx]);
	struct net_pkt *rx_frame;

	/* More than one frame could have been received by GMAC, get all
	 * complete frames stored in the GMAC RX descriptor list.
	 */
	rx_frame = eth_mchp_frame_get(queue);
	if (!rx_frame) {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		dev_data->stats.error_details.rx_buf_alloc_failed++;
#endif
	}

	while (rx_frame) {
		LOG_DBG("ETH rx");

		if (net_recv_data(eth_mchp_get_iface(dev_data), rx_frame) < 0) {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
			dev_data->stats.error_details.rx_frame_errors++;
#endif
			net_pkt_unref(rx_frame);
		}

		rx_frame = eth_mchp_frame_get(queue);
	}
}

/**
 * @brief Start the Eth MAC device.
 *
 * This function is used to start the Eth Interface.
 *
 * @param dev Pointer to the Eth MAC device structure.
 *
 * @return 0 on starting the Eth MAC device.
 */
static int eth_mchp_start(const struct device *dev)
{
	eth_mchp_dev_data_t *const dev_data = dev->data;

	/* Do not start the interface until PHY link is up */
	if (!(dev_data->link_up)) {
		net_if_carrier_off(dev_data->iface);
	} else {
		net_eth_carrier_on(dev_data->iface);
	}

	return 0;
}

/**
 * @brief Stop the Eth MAC device.
 *
 * This function is used to stop the Eth Interface.
 *
 * @param dev Pointer to the Eth MAC device structure.
 *
 * @return 0 on stopping the Eth MAC device.
 */
static int eth_mchp_stop(const struct device *dev)
{
	eth_mchp_dev_data_t *const dev_data = dev->data;

	net_eth_carrier_off(dev_data->iface);
	return 0;
}

/**
 * @brief Find the Tx Queue based on the priority of the packet
 *
 * This function is used to find the Tx Queue based on the priority of
 * the packet.
 *
 * @param priority packet piority.
 *
 * @return Queue id of the queue to which this packet should enqueued.
 */
#if !defined(CONFIG_ETH_SAM_GMAC_FORCE_QUEUE) &&                                                   \
	((GMAC_ACTIVE_QUEUE_NUM != NET_TC_TX_COUNT) ||                                             \
	 ((NET_TC_TX_COUNT != NET_TC_RX_COUNT) && defined(CONFIG_NET_VLAN)))
static int eth_mchp_priority2queue(enum net_priority priority)
{
	static const uint8_t queue_priority_map[] = {
#if GMAC_ACTIVE_QUEUE_NUM == 1
		0, 0, 0, 0, 0,
		0, 0, 0
#endif
	};

	return queue_priority_map[priority];
}
#endif

/**
 * @brief Transmit the frame to network
 *
 * This function is used to queeu the frame coming in from the net interface
 * to th etx queue to eventually transmit the frame to the network.
 * the packet.
 *
 * @param dev Pointer to the Eth MAC device structure.
 * @param dev Pointer to the net packet which was received from the net interface.
 *
 * @return 0 if the frame was successfully transmitted, -EIO in error case.
 */
static int eth_mchp_send(const struct device *dev, struct net_pkt *pkt)
{
	const eth_mchp_dev_config_t *const cfg = dev->config;
	eth_mchp_dev_data_t *const dev_data = dev->data;
	gmac_registers_t *const hal = cfg->regs;
	struct eth_mchp_gmac_queue *queue;
	struct eth_mchp_gmac_desc_list *tx_desc_list;
	struct eth_mchp_gmac_desc *tx_desc;
	struct eth_mchp_gmac_desc *tx_first_desc;
	struct net_buf *frag;
	uint8_t *frag_data;
	uint16_t frag_len;
	uint32_t err_tx_flushed_count_at_entry;
	uint8_t pkt_prio;
	uint32_t pkt_len = 0;

	pkt_len = net_pkt_get_len(pkt);

	if (pkt == NULL) {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		dev_data->stats.errors.tx++;
#endif
		return -EIO;
	}

	if (pkt->frags == NULL) {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		dev_data->stats.errors.tx++;
#endif
		return -EIO;
	}

	LOG_DBG("ETH tx");

	/* Decide which queue should be used */
	pkt_prio = net_pkt_priority(pkt);

#if defined(CONFIG_ETH_SAM_GMAC_FORCE_QUEUE)
	/* Route eveything to the forced queue */
	queue = &dev_data->queue_list[CONFIG_ETH_SAM_GMAC_FORCED_QUEUE];

#elif GMAC_ACTIVE_QUEUE_NUM == CONFIG_NET_TC_TX_COUNT
	/* Prefer to chose queue based on its traffic class */
	queue = &dev_data->queue_list[net_tx_priority2tc(pkt_prio)];

#else
	/* If that's not possible due to config - use builtin mapping */
	queue = &dev_data->queue_list[eth_mchp_priority2queue(pkt_prio)];
#endif

	tx_desc_list = &queue->tx_desc_list;
	err_tx_flushed_count_at_entry = queue->err_tx_flushed_count;

	frag = pkt->frags;

	/* Keep reference to the descriptor */
	tx_first_desc = &tx_desc_list->buf[tx_desc_list->head];

	while (frag) {

		frag_data = frag->data;
		frag_len = frag->len;

		/* Assure cache coherency before DMA read operation */
		eth_mchp_dcache_clean((uint32_t)frag_data, frag->size);

		tx_desc = &tx_desc_list->buf[tx_desc_list->head];

		/* Update buffer descriptor address word */
		tx_desc->w0 = (uint32_t)frag_data;

		/* Update buffer descriptor status word (clear used bit except
		 * for the first frag).
		 */
		tx_desc->w1 = (frag_len & GMAC_TXW1_LEN) |
			      (!frag->frags ? GMAC_TXW1_LASTBUFFER : 0) |
			      (tx_desc_list->head == tx_desc_list->len - 1U ? GMAC_TXW1_WRAP : 0) |
			      (tx_desc == tx_first_desc ? GMAC_TXW1_USED : 0);

		/* Update descriptor position */
		MODULO_INC(tx_desc_list->head, tx_desc_list->len);

		/* Continue with the rest of fragments (only data) */
		frag = frag->frags;
	}

	/* Ensure the descriptor following the last one is marked as used */
	tx_desc_list->buf[tx_desc_list->head].w1 = GMAC_TXW1_USED;

	/* Guarantee that all the fragments have been written before removing
	 * the used bit to avoid race condition.
	 */
	barrier_dmem_fence_full();

	/* Remove the used bit of the first fragment to allow the controller
	 * to process it and the following fragments.
	 */
	tx_first_desc->w1 &= ~GMAC_TXW1_USED;

	/* Guarantee that the first fragment got its bit removed before starting
	 * sending packets to avoid packets getting stuck.
	 */
	barrier_dmem_fence_full();

	/* Start transmission */
	hal_mchp_eth_tx(hal);

	/* Wait until the packet is sent */
	k_sem_take(&queue->tx_sem, K_FOREVER);

	/* Check if transmit successful or not */
	if (queue->err_tx_flushed_count != err_tx_flushed_count_at_entry) {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		dev_data->stats.errors.tx++;
#endif
		return -EIO;
	}
	return 0;
}

/**
 * @brief ISR when either packet is received or transmission is completed
 *
 * This is an ISR which is called when either packet is received on Queue 0
 * or transmission of the previous frame has completed.
 *
 * @param dev Pointer to the Eth MAC device structure.
 */
static void eth_mchp_queue0_isr(const struct device *dev)
{
	const eth_mchp_dev_config_t *const cfg = dev->config;
	eth_mchp_dev_data_t *const dev_data = dev->data;
	gmac_registers_t *const hal = cfg->regs;
	struct eth_mchp_gmac_queue *queue = &dev_data->queue_list[0];

	hal_mchp_eth_queue0_isr(hal, queue);
}

/**
 * @brief Eth MAC Device Initialization
 *
 * This function is used to initialize the Eth MAC devices etting the
 * pin control state, and ensuring the Eth MAC device is ready.
 *
 * @param dev Pointer to the Eth MAC device structure.
 *
 * @return 0 on success, negative error code on failure.
 */
static int eth_mchp_initialize(const struct device *dev)
{
	const eth_mchp_dev_config_t *const cfg = dev->config;
	int retval;

	cfg->config_func();

	/* Connect pins to the peripheral */
	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	return retval;
}

/**
 * @brief Eth MAC Interface link state change callback
 *
 * This function is called when there is a change in link state of the Eth Phy
 * to let the Eth MAC driver know about it. This callback is registered with
 * the Eth Phy during initialization.
 *
 * @param pdev Pointer to the Eth Phy device structure.
 * @param state Pointer to the link state structure.
 * @param user_data Pointer to the Eth MAC device structure.
 */
static void eth_mchp_phy_link_state_changed(const struct device *pdev, struct phy_link_state *state,
					    void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	eth_mchp_dev_data_t *const dev_data = dev->data;
	const eth_mchp_dev_config_t *const cfg = dev->config;
	gmac_registers_t *const hal = cfg->regs;
	bool is_up;

	is_up = state->is_up;

	if (is_up && !dev_data->link_up) {
		LOG_INF("Link up");

		/* Announce link up status */
		dev_data->link_up = true;
		net_eth_carrier_on(dev_data->iface);

		/* Set up link */
		hal_mchp_eth_link_configure(hal, PHY_LINK_IS_FULL_DUPLEX(state->speed),
					    PHY_LINK_IS_SPEED_100M(state->speed));
	} else if (!is_up && dev_data->link_up) {
		LOG_INF("Link down");

		/* Announce link down status */
		dev_data->link_up = false;
		net_eth_carrier_off(dev_data->iface);
	}
}

/**
 * @brief Getting the Phy Device handle
 *
 * This function is called for getting the phy device handle associated with
 * this Eth MAc interface.
 *
 * @param dev Pointer to the Eth MAC device structure.
 *
 * @return Pointer to the Phy Device structure.
 */
static const struct device *eth_mchp_get_phy(const struct device *dev)
{
	const eth_mchp_dev_config_t *const cfg = dev->config;

	return cfg->phy_dev;
}

/**
 * @brief Initializing the Eth Interface
 *
 * This function is used to initialize the ethernet interface by configure
 * and enabling the clocks, setting the appropriate registers, initializing
 * rx and tx queues associated with the interface, and setting and regsitering
 * the MAC address with the upper layer.
 *
 * @param iface Pointer to the net interface structure.
 */
static void eth_mchp_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	eth_mchp_dev_data_t *const dev_data = dev->data;
	const eth_mchp_dev_config_t *const cfg = dev->config;
	gmac_registers_t *const hal = cfg->regs;
	static bool init_done;
	uint32_t gmac_ncfgr_val;
	int i;
	uint32_t clk_freq_hz = 0;
	int result;

	if (dev_data->iface == NULL) {
		dev_data->iface = iface;
	}

	ethernet_init(iface);

	/* The rest of initialization should only be done once */
	if (init_done) {
		return;
	}

	/* Check the status of data caches */
	eth_mchp_dcache_is_enabled();

	/* Initialize GMAC driver */
	gmac_ncfgr_val = GMAC_NCFGR_MTIHEN_Msk   /* Multicast Hash Enable */
			 | GMAC_NCFGR_LFERD_Msk  /* Length Field Error Frame Discard */
			 | GMAC_NCFGR_RFCS_Msk   /* Remove Frame Check Sequence */
			 | GMAC_NCFGR_RXCOEN_Msk /* Receive Checksum Offload Enable */
			 | GMAC_MAX_FRAME_SIZE;

	/* Get Clock frequency */
	result = ETH_MCHP_GET_CLOCK_FREQ(
		cfg->eth_clock.clock_dev,
		(((eth_mchp_dev_config_t *)(dev->config))->eth_clock.mclk_apb_sys), clk_freq_hz);
	if (result < 0) {
		LOG_ERR("ETH_MCHP_GET_CLOCK_FREQ Failed");
	}

	result = hal_mchp_eth_gmac_init(hal, gmac_ncfgr_val, clk_freq_hz);
	if (result < 0) {
		LOG_ERR("Unable to initialize ETH driver");
		return;
	}

	/* Set the MAC address */
	hal_mchp_eth_generate_set_mac(hal, dev_data->mac_addr);

	LOG_INF("MAC: %02x:%02x:%02x:%02x:%02x:%02x", dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3], dev_data->mac_addr[4],
		dev_data->mac_addr[5]);

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr, sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	/* Initialize GMAC queues */
	for (i = GMAC_QUE_0; i < GMAC_QUEUE_NUM; i++) {
		result = hal_mchp_eth_queue_init(hal, &dev_data->queue_list[i]);
		if (result < 0) {
			LOG_ERR("Unable to initialize ETH queue%d", i);
			return;
		}
	}

	if (device_is_ready(cfg->phy_dev)) {
		phy_link_callback_set(cfg->phy_dev, &eth_mchp_phy_link_state_changed, (void *)dev);

	} else {
		LOG_ERR("PHY device not ready");
	}

	init_done = true;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
/**
 * @brief Get the statistics related to Ethernet interface.
 *
 * This function is used to get the statistics related to Ethernet interface.
 *
 * @param dev Pointer to the Eth MAC device structure.
 *
 * @return net_stats_eth* Pointer to the ethernet statistics structure.
 */
static struct net_stats_eth *eth_mchp_get_stats(const struct device *dev)
{
	eth_mchp_dev_data_t *dev_data = dev->data;
	const eth_mchp_dev_config_t *const cfg = dev->config;
	gmac_registers_t *const hal = cfg->regs;

	hal_mchp_eth_get_stats(hal, &dev_data->stats);

	return &dev_data->stats;
}
#endif

/**
 * @brief Get the Eth MAC device capabilities.
 *
 * This function is used to get the Eth MAC device capabilities.
 *
 * @param dev Pointer to the Eth MAC device structure.
 *
 * @return set of device capabilities.
 */
static enum ethernet_hw_caps eth_mchp_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T |
#if defined(CONFIG_NET_VLAN)
	       ETHERNET_HW_VLAN |
#endif
	       ETHERNET_PRIORITY_QUEUES |
#if GMAC_ACTIVE_PRIORITY_QUEUE_NUM >= 1
	       ETHERNET_QAV |
#endif
	       ETHERNET_LINK_100BASE_T;
}

/**
 * @brief Set the hardware specific configuration for the Eternet interface.
 *
 * This function is used to set the hardware specific configuration for
 * the Eternet interface.
 *
 * @param dev Pointer to the Eth MAC device structure.
 * @param type specific param being set like the MAC address.
 * @param config Pointer to the Eth configuration structure to get the
 * corresponding config to be set.
 *
 * @return 0 if successful, -ENOTSUP if type not supported.
 */
static int eth_mchp_set_config(const struct device *dev, enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	int result = 0;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS: {
		eth_mchp_dev_data_t *const dev_data = dev->data;
		const eth_mchp_dev_config_t *const cfg = dev->config;
		gmac_registers_t *const hal = cfg->regs;

		memcpy(dev_data->mac_addr, config->mac_address.addr, sizeof(dev_data->mac_addr));

		/* Set MAC Address for frame filtering logic */
		hal_mchp_eth_mac_addr_set(hal, 0, dev_data->mac_addr);

		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name,
			dev_data->mac_addr[0], dev_data->mac_addr[1], dev_data->mac_addr[2],
			dev_data->mac_addr[3], dev_data->mac_addr[4], dev_data->mac_addr[5]);

		/* Register Ethernet MAC Address with the upper layer */
		net_if_set_link_addr(dev_data->iface, dev_data->mac_addr,
				     sizeof(dev_data->mac_addr), NET_LINK_ETHERNET);
		break;
	}
	default:
		result = -ENOTSUP;
		break;
	}

	return result;
}

/**
 * @brief Get the hardware specific configuration for the Eternet interface.
 *
 * This function is used to get the hardware specific configuration for
 * the Eternet interface.
 *
 * @param dev Pointer to the Eth MAC device structure.
 * @param type specific param being get like the number of queues.
 * @param config Pointer to the Eth configuration structure to get the
 * corresponding config to be set.
 *
 * @return 0 if successful, -ENOTSUP if type not supported.
 */
static int eth_mchp_get_config(const struct device *dev, enum ethernet_config_type type,
			       struct ethernet_config *config)
{
	switch (type) {
	case ETHERNET_CONFIG_TYPE_PRIORITY_QUEUES_NUM:
		config->priority_queues_num = GMAC_ACTIVE_PRIORITY_QUEUE_NUM;
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

/**
 * @brief Eth MAC device API structure for Microchip Eth MAC driver.
 *
 * This structure defines the function pointers for Eth MAC operations,
 * including sending/ receiving frames and optional statistics functions.
 */
static const struct ethernet_api eth_api = {
	/* interface initialization function */
	.iface_api.init = eth_mchp_iface_init,

	/* eth interface start function */
	.start = eth_mchp_start,

	/* eth interface stop function */
	.stop = eth_mchp_stop,

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	/* get interface stats function */
	.get_stats = eth_mchp_get_stats,
#endif

	/* get device capabilities function */
	.get_capabilities = eth_mchp_get_capabilities,

	/* eth hw config set function */
	.set_config = eth_mchp_set_config,

	/* eth hw config get function */
	.get_config = eth_mchp_get_config,

	/* phy interface get function */
	.get_phy = eth_mchp_get_phy,

	/* eth frame transmit function */
	.send = eth_mchp_send,
};

/**
 * @brief Macro to configure and enable Eth MAC IRQ
 */
static void eth0_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, gmac, irq), DT_INST_IRQ_BY_NAME(0, gmac, priority),
		    eth_mchp_queue0_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, gmac, irq));
}

PINCTRL_DT_INST_DEFINE(0);

static const eth_mchp_dev_config_t eth0_config = {
	.regs = (gmac_registers_t *)DT_INST_REG_ADDR(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.config_func = eth0_irq_config,
	ETH_MCHP_CLOCK_DEFN(0),
	.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle)),
};

static eth_mchp_dev_data_t eth0_data = {
#if NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))
	.mac_addr = DT_INST_PROP(0, local_mac_address),
#endif
	.queue_list = {{
		.que_idx = GMAC_QUE_0,
		/* clang-format off */
		.rx_desc_list = {
				.buf = rx_desc_que0,
				.len = ARRAY_SIZE(rx_desc_que0),
			},
		.tx_desc_list = {
				.buf = tx_desc_que0,
				.len = ARRAY_SIZE(tx_desc_que0),
			},
		/* clang-format on */
		.rx_frag_list = rx_frag_list_que0,
	}},
};

/**
 * @brief Macro to initialize Eth MAC device
 *
 * @param n Instance number
 */
ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_mchp_initialize, NULL, &eth0_data, &eth0_config,
			      CONFIG_ETH_INIT_PRIORITY, &eth_api, GMAC_MTU);
