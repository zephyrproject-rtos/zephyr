/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyringt (c) 2018 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Ethernet MAC (GMAC) driver.
 *
 * This is a zero-copy networking implementation of an Ethernet driver. To
 * prepare for the incoming frames the driver will permanently reserve a defined
 * amount of RX data net buffers when the interface is brought up and thus
 * reduce the total amount of RX data net buffers available to the application.
 *
 * Limitations:
 * - one shot PHY setup, no support for PHY disconnect/reconnect
 * - no statistics collection
 */

#define LOG_MODULE_NAME eth_sam
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <kernel.h>
#include <device.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <errno.h>
#include <stdbool.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <drivers/i2c.h>
#include <soc.h>
#include "phy_sam_gmac.h"
#include "eth_sam_gmac_priv.h"

#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
#include <ptp_clock.h>
#include <net/gptp.h>
#endif

/*
 * Verify Kconfig configuration
 */
/* No need to verify things for unit tests */
#if !defined(CONFIG_NET_TEST)
#if CONFIG_NET_BUF_DATA_SIZE * CONFIG_ETH_SAM_GMAC_BUF_RX_COUNT \
	< GMAC_FRAME_SIZE_MAX
#error CONFIG_NET_BUF_DATA_SIZE * CONFIG_ETH_SAM_GMAC_BUF_RX_COUNT is \
	not large enough to hold a full frame
#endif

#if CONFIG_NET_BUF_DATA_SIZE * (CONFIG_NET_BUF_RX_COUNT - \
	CONFIG_ETH_SAM_GMAC_BUF_RX_COUNT) < GMAC_FRAME_SIZE_MAX
#error (CONFIG_NET_BUF_RX_COUNT - CONFIG_ETH_SAM_GMAC_BUF_RX_COUNT) * \
	CONFIG_NET_BUF_DATA_SIZE are not large enough to hold a full frame
#endif

#if CONFIG_NET_BUF_DATA_SIZE & 0x3F
#pragma message "CONFIG_NET_BUF_DATA_SIZE should be a multiple of 64 bytes " \
	"due to the granularity of RX DMA"
#endif

#if (CONFIG_ETH_SAM_GMAC_BUF_RX_COUNT + 1) * CONFIG_ETH_SAM_GMAC_QUEUES \
	> CONFIG_NET_BUF_RX_COUNT
#error Not enough RX buffers to allocate descriptors for each HW queue
#endif
#endif /* !CONFIG_NET_TEST */

/* RX descriptors list */
static struct gmac_desc rx_desc_que0[MAIN_QUEUE_RX_DESC_COUNT]
	__nocache __aligned(GMAC_DESC_ALIGNMENT);
static struct gmac_desc rx_desc_que1[PRIORITY_QUEUE1_RX_DESC_COUNT]
	__nocache __aligned(GMAC_DESC_ALIGNMENT);
static struct gmac_desc rx_desc_que2[PRIORITY_QUEUE2_RX_DESC_COUNT]
	__nocache __aligned(GMAC_DESC_ALIGNMENT);
/* TX descriptors list */
static struct gmac_desc tx_desc_que0[MAIN_QUEUE_TX_DESC_COUNT]
	__nocache __aligned(GMAC_DESC_ALIGNMENT);
static struct gmac_desc tx_desc_que1[PRIORITY_QUEUE1_TX_DESC_COUNT]
	__nocache __aligned(GMAC_DESC_ALIGNMENT);
static struct gmac_desc tx_desc_que2[PRIORITY_QUEUE2_TX_DESC_COUNT]
	__nocache __aligned(GMAC_DESC_ALIGNMENT);

/* RX buffer accounting list */
static struct net_buf *rx_frag_list_que0[MAIN_QUEUE_RX_DESC_COUNT];
#if GMAC_PRIORITY_QUEUE_NO >= 1
static struct net_buf *rx_frag_list_que1[PRIORITY_QUEUE1_RX_DESC_COUNT];
#endif
#if GMAC_PRIORITY_QUEUE_NO == 2
static struct net_buf *rx_frag_list_que2[PRIORITY_QUEUE2_RX_DESC_COUNT];
#endif
#if GMAC_MULTIPLE_TX_PACKETS == 1
/* TX buffer accounting list */
static struct net_buf *tx_frag_list_que0[MAIN_QUEUE_TX_DESC_COUNT];
#if GMAC_PRIORITY_QUEUE_NO >= 1
static struct net_buf *tx_frag_list_que1[PRIORITY_QUEUE1_TX_DESC_COUNT];
#endif
#if GMAC_PRIORITY_QUEUE_NO == 2
static struct net_buf *tx_frag_list_que2[PRIORITY_QUEUE2_TX_DESC_COUNT];
#endif
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
/* TX frames accounting list */
static struct net_pkt *tx_frame_list_que0[CONFIG_NET_PKT_TX_COUNT + 1];
#if GMAC_PRIORITY_QUEUE_NO >= 1
static struct net_pkt *tx_frame_list_que1[CONFIG_NET_PKT_TX_COUNT + 1];
#endif
#if GMAC_PRIORITY_QUEUE_NO == 2
static struct net_pkt *tx_frame_list_que2[CONFIG_NET_PKT_TX_COUNT + 1];
#endif
#endif
#endif

#define MODULO_INC(val, max) {val = (++val < max) ? val : 0; }

/*
 * Cache helpers
 */

static bool dcache_enabled;

static inline void dcache_invalidate(u32_t addr, u32_t size)
{
	if (!dcache_enabled) {
		return;
	}

	/* Make sure it is aligned to 32B */
	u32_t start_addr = addr & (u32_t)~(GMAC_DCACHE_ALIGNMENT - 1);
	u32_t size_full = size + addr - start_addr;

	SCB_InvalidateDCache_by_Addr((uint32_t *)start_addr, size_full);
}

static inline void dcache_clean(u32_t addr, u32_t size)
{
	if (!dcache_enabled) {
		return;
	}

	/* Make sure it is aligned to 32B */
	u32_t start_addr = addr & (u32_t)~(GMAC_DCACHE_ALIGNMENT - 1);
	u32_t size_full = size + addr - start_addr;

	SCB_CleanDCache_by_Addr((uint32_t *)start_addr, size_full);
}

#if GMAC_MULTIPLE_TX_PACKETS == 1
/*
 * Reset ring buffer
 */
static void ring_buf_reset(struct ring_buf *rb)
{
	rb->head = 0U;
	rb->tail = 0U;
}

/*
 * Get one 32 bit item from the ring buffer
 */
static u32_t ring_buf_get(struct ring_buf *rb)
{
	u32_t val;

	__ASSERT(rb->tail != rb->head,
		 "retrieving data from empty ring buffer");

	val = rb->buf[rb->tail];
	MODULO_INC(rb->tail, rb->len);

	return val;
}

/*
 * Put one 32 bit item into the ring buffer
 */
static void ring_buf_put(struct ring_buf *rb, u32_t val)
{
	rb->buf[rb->head] = val;
	MODULO_INC(rb->head, rb->len);

	__ASSERT(rb->tail != rb->head,
		 "ring buffer overflow");
}
#endif

/*
 * Free pre-reserved RX buffers
 */
static void free_rx_bufs(struct net_buf **rx_frag_list, u16_t len)
{
	for (int i = 0; i < len; i++) {
		if (rx_frag_list[i]) {
			net_buf_unref(rx_frag_list[i]);
			rx_frag_list[i] = NULL;
		}
	}
}

/*
 * Set MAC Address for frame filtering logic
 */
static void mac_addr_set(Gmac *gmac, u8_t index,
				 u8_t mac_addr[6])
{
	__ASSERT(index < 4, "index has to be in the range 0..3");

	gmac->GMAC_SA[index].GMAC_SAB =   (mac_addr[3] << 24)
					| (mac_addr[2] << 16)
					| (mac_addr[1] <<  8)
					| (mac_addr[0]);
	gmac->GMAC_SA[index].GMAC_SAT =   (mac_addr[5] <<  8)
					| (mac_addr[4]);
}

/*
 * Initialize RX descriptor list
 */
static int rx_descriptors_init(Gmac *gmac, struct gmac_queue *queue)
{
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct net_buf **rx_frag_list = queue->rx_frag_list;
	struct net_buf *rx_buf;
	u8_t *rx_buf_addr;

	__ASSERT_NO_MSG(rx_frag_list);

	rx_desc_list->tail = 0U;

	for (int i = 0; i < rx_desc_list->len; i++) {
		rx_buf = net_pkt_get_reserve_rx_data(K_NO_WAIT);
		if (rx_buf == NULL) {
			free_rx_bufs(rx_frag_list, rx_desc_list->len);
			LOG_ERR("Failed to reserve data net buffers");
			return -ENOBUFS;
		}

		rx_frag_list[i] = rx_buf;

		rx_buf_addr = rx_buf->data;
		__ASSERT(!((u32_t)rx_buf_addr & ~GMAC_RXW0_ADDR),
			 "Misaligned RX buffer address");
		__ASSERT(rx_buf->size == CONFIG_NET_BUF_DATA_SIZE,
			 "Incorrect length of RX data buffer");
		/* Give ownership to GMAC and remove the wrap bit */
		rx_desc_list->buf[i].w0 = (u32_t)rx_buf_addr & GMAC_RXW0_ADDR;
		rx_desc_list->buf[i].w1 = 0U;
	}

	/* Set the wrap bit on the last descriptor */
	rx_desc_list->buf[rx_desc_list->len - 1U].w0 |= GMAC_RXW0_WRAP;

	return 0;
}

/*
 * Initialize TX descriptor list
 */
static void tx_descriptors_init(Gmac *gmac, struct gmac_queue *queue)
{
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;

	tx_desc_list->head = 0U;
	tx_desc_list->tail = 0U;

	for (int i = 0; i < tx_desc_list->len; i++) {
		tx_desc_list->buf[i].w0 = 0U;
		tx_desc_list->buf[i].w1 = GMAC_TXW1_USED;
	}

	/* Set the wrap bit on the last descriptor */
	tx_desc_list->buf[tx_desc_list->len - 1U].w1 |= GMAC_TXW1_WRAP;

#if GMAC_MULTIPLE_TX_PACKETS == 1
	/* Reset TX frame list */
	ring_buf_reset(&queue->tx_frag_list);
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	ring_buf_reset(&queue->tx_frames);
#endif
#endif
}

#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
static struct gptp_hdr *check_gptp_msg(struct net_if *iface,
				       struct net_pkt *pkt,
				       bool is_tx)
{
	u8_t *msg_start = net_pkt_data(pkt);
	struct ethernet_context *eth_ctx;
	struct gptp_hdr *gptp_hdr;
	int eth_hlen;

#if defined(CONFIG_NET_VLAN)
	eth_ctx = net_if_l2_data(iface);
	if (net_eth_is_vlan_enabled(eth_ctx, iface)) {
		struct net_eth_vlan_hdr *hdr_vlan;

		hdr_vlan = (struct net_eth_vlan_hdr *)msg_start;
		if (ntohs(hdr_vlan->type) != NET_ETH_PTYPE_PTP) {
			return NULL;
		}

		eth_hlen = sizeof(struct net_eth_vlan_hdr);
	} else
#else
	ARG_UNUSED(eth_ctx);
#endif
	{
		struct net_eth_hdr *hdr;

		hdr = (struct net_eth_hdr *)msg_start;
		if (ntohs(hdr->type) != NET_ETH_PTYPE_PTP) {
			return NULL;
		}

		eth_hlen = sizeof(struct net_eth_hdr);
	}

	/* In TX, the first net_buf contains the Ethernet header
	 * and the actual gPTP header is in the second net_buf.
	 * In RX, the Ethernet header + other headers are in the
	 * first net_buf.
	 */
	if (is_tx) {
		if (pkt->frags->frags == NULL) {
			return false;
		}

		gptp_hdr = (struct gptp_hdr *)pkt->frags->frags->data;
	} else {
		gptp_hdr = (struct gptp_hdr *)(pkt->frags->data + eth_hlen);
	}

	return gptp_hdr;
}

static bool need_timestamping(struct gptp_hdr *hdr)
{
	switch (hdr->message_type) {
	case GPTP_SYNC_MESSAGE:
	case GPTP_PATH_DELAY_RESP_MESSAGE:
		return true;
	default:
		return false;
	}
}

static void update_pkt_priority(struct gptp_hdr *hdr, struct net_pkt *pkt)
{
	if (GPTP_IS_EVENT_MSG(hdr->message_type)) {
		net_pkt_set_priority(pkt, NET_PRIORITY_CA);
	} else {
		net_pkt_set_priority(pkt, NET_PRIORITY_IC);
	}
}

static inline struct net_ptp_time get_ptp_event_rx_ts(Gmac *gmac)
{
	struct net_ptp_time ts;

	ts.second = ((u64_t)(gmac->GMAC_EFRSH & 0xffff) << 32)
			   | gmac->GMAC_EFRSL;
	ts.nanosecond = gmac->GMAC_EFRN;

	return ts;
}

static inline struct net_ptp_time get_ptp_peer_event_rx_ts(Gmac *gmac)
{
	struct net_ptp_time ts;

	ts.second = ((u64_t)(gmac->GMAC_PEFRSH & 0xffff) << 32)
		    | gmac->GMAC_PEFRSL;
	ts.nanosecond = gmac->GMAC_PEFRN;

	return ts;
}

static inline struct net_ptp_time get_ptp_event_tx_ts(Gmac *gmac)
{
	struct net_ptp_time ts;

	ts.second = ((u64_t)(gmac->GMAC_EFTSH & 0xffff) << 32)
			   | gmac->GMAC_EFTSL;
	ts.nanosecond = gmac->GMAC_EFTN;

	return ts;
}

static inline struct net_ptp_time get_ptp_peer_event_tx_ts(Gmac *gmac)
{
	struct net_ptp_time ts;

	ts.second = ((u64_t)(gmac->GMAC_PEFTSH & 0xffff) << 32)
		    | gmac->GMAC_PEFTSL;
	ts.nanosecond = gmac->GMAC_PEFTN;

	return ts;
}

static inline struct net_ptp_time get_current_ts(Gmac *gmac)
{
	struct net_ptp_time ts;

	ts.second = ((u64_t)(gmac->GMAC_TSH & 0xffff) << 32) | gmac->GMAC_TSL;
	ts.nanosecond = gmac->GMAC_TN;

	return ts;
}


static inline void timestamp_tx_pkt(Gmac *gmac, struct gptp_hdr *hdr,
				    struct net_pkt *pkt)
{
	struct net_ptp_time timestamp;

	if (hdr) {
		switch (hdr->message_type) {
		case GPTP_SYNC_MESSAGE:
			timestamp = get_ptp_event_tx_ts(gmac);
			break;
		default:
			timestamp = get_ptp_peer_event_tx_ts(gmac);
		}
	} else {
		timestamp = get_current_ts(gmac);
	}

	net_pkt_set_timestamp(pkt, &timestamp);
}

static inline void timestamp_rx_pkt(Gmac *gmac, struct gptp_hdr *hdr,
				    struct net_pkt *pkt)
{
	struct net_ptp_time timestamp;

	if (hdr) {
		switch (hdr->message_type) {
		case GPTP_SYNC_MESSAGE:
			timestamp = get_ptp_event_rx_ts(gmac);
			break;
		default:
			timestamp = get_ptp_peer_event_rx_ts(gmac);
		}
	} else {
		timestamp = get_current_ts(gmac);
	}

	net_pkt_set_timestamp(pkt, &timestamp);
}

#endif

static inline struct net_if *get_iface(struct eth_sam_dev_data *ctx,
				       u16_t vlan_tag)
{
#if defined(CONFIG_NET_VLAN)
	struct net_if *iface;

	iface = net_eth_get_vlan_iface(ctx->iface, vlan_tag);
	if (!iface) {
		return ctx->iface;
	}

	return iface;
#else
	ARG_UNUSED(vlan_tag);

	return ctx->iface;
#endif
}

/*
 * Process successfully sent packets
 */
static void tx_completed(Gmac *gmac, struct gmac_queue *queue)
{
#if GMAC_MULTIPLE_TX_PACKETS == 0
	k_sem_give(&queue->tx_sem);
#else
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;
	struct gmac_desc *tx_desc;
	struct net_buf *frag;
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	struct net_pkt *pkt;
	u16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct gptp_hdr *hdr;
	struct eth_sam_dev_data *dev_data =
		CONTAINER_OF(queue, struct eth_sam_dev_data,
			     queue_list[queue->que_idx]);
#endif

	__ASSERT(tx_desc_list->buf[tx_desc_list->tail].w1 & GMAC_TXW1_USED,
		 "first buffer of a frame is not marked as own by GMAC");

	while (tx_desc_list->tail != tx_desc_list->head) {

		tx_desc = &tx_desc_list->buf[tx_desc_list->tail];
		MODULO_INC(tx_desc_list->tail, tx_desc_list->len);
		k_sem_give(&queue->tx_desc_sem);

		/* Release net buffer to the buffer pool */
		frag = UINT_TO_POINTER(ring_buf_get(&queue->tx_frag_list));
		net_pkt_frag_unref(frag);
		LOG_DBG("Dropping frag %p", frag);

		if (tx_desc->w1 & GMAC_TXW1_LASTBUFFER) {
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
			/* Release net packet to the packet pool */
			pkt = UINT_TO_POINTER(ring_buf_get(&queue->tx_frames));
#if defined(CONFIG_NET_VLAN)
			struct net_eth_hdr *eth_hdr = NET_ETH_HDR(pkt);

			if (ntohs(eth_hdr->type) == NET_ETH_PTYPE_VLAN) {
				vlan_tag = net_pkt_vlan_tag(pkt);
			}
#endif
			hdr = check_gptp_msg(get_iface(dev_data, vlan_tag),
					     pkt, true);

			timestamp_tx_pkt(gmac, hdr, pkt);

			if (hdr && need_timestamping(hdr)) {
				net_if_add_tx_timestamp(pkt);
			}
			net_pkt_unref(pkt);
			LOG_DBG("Dropping pkt %p", pkt);
#endif
			break;
		}
	}
#endif
}

/*
 * Reset TX queue when errors are detected
 */
static void tx_error_handler(Gmac *gmac, struct gmac_queue *queue)
{
#if GMAC_MULTIPLE_TX_PACKETS == 1
	struct net_buf *frag;
	struct ring_buf *tx_frag_list = &queue->tx_frag_list;
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	struct net_pkt *pkt;
	struct ring_buf *tx_frames = &queue->tx_frames;
#endif
#endif

	queue->err_tx_flushed_count++;

	/* Stop transmission, clean transmit pipeline and control registers */
	gmac->GMAC_NCR &= ~GMAC_NCR_TXEN;

#if GMAC_MULTIPLE_TX_PACKETS == 1
	/* Free all frag resources in the TX path */
	while (tx_frag_list->tail != tx_frag_list->head) {
		/* Release net buffer to the buffer pool */
		frag = UINT_TO_POINTER(tx_frag_list->buf[tx_frag_list->tail]);
		net_pkt_frag_unref(frag);
		LOG_DBG("Dropping frag %p", frag);
		MODULO_INC(tx_frag_list->tail, tx_frag_list->len);
	}

#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	/* Free all pkt resources in the TX path */
	while (tx_frames->tail != tx_frames->head) {
		/* Release net packet to the packet pool */
		pkt = UINT_TO_POINTER(tx_frames->buf[tx_frames->tail]);
		net_pkt_unref(pkt);
		LOG_DBG("Dropping pkt %p", pkt);
		MODULO_INC(tx_frames->tail, tx_frames->len);
	}
#endif

	/* Reinitialize TX descriptor list */
	k_sem_reset(&queue->tx_desc_sem);
	for (int i = 0; i < queue->tx_desc_list.len - 1; i++) {
		k_sem_give(&queue->tx_desc_sem);
	}
#endif
	tx_descriptors_init(gmac, queue);

#if GMAC_MULTIPLE_TX_PACKETS == 0
	/* Reinitialize TX mutex */
	k_sem_give(&queue->tx_sem);
#endif

	/* Restart transmission */
	gmac->GMAC_NCR |=  GMAC_NCR_TXEN;
}

/*
 * Clean RX queue, any received data still stored in the buffers is abandoned.
 */
static void rx_error_handler(Gmac *gmac, struct gmac_queue *queue)
{
	queue->err_rx_flushed_count++;

	/* Stop reception */
	gmac->GMAC_NCR &= ~GMAC_NCR_RXEN;

	queue->rx_desc_list.tail = 0U;

	for (int i = 0; i < queue->rx_desc_list.len; i++) {
		queue->rx_desc_list.buf[i].w1 = 0U;
		queue->rx_desc_list.buf[i].w0 &= ~GMAC_RXW0_OWNERSHIP;
	}

	/* Set Receive Buffer Queue Pointer Register */
	if (queue->que_idx == 0) {
		gmac->GMAC_RBQB = (u32_t)queue->rx_desc_list.buf;
	} else {
		gmac->GMAC_RBQBAPQ[queue->que_idx - 1] =
			(u32_t)queue->rx_desc_list.buf;
	}

	/* Restart reception */
	gmac->GMAC_NCR |=  GMAC_NCR_RXEN;
}

/*
 * Set MCK to MDC clock divisor.
 *
 * According to 802.3 MDC should be less then 2.5 MHz.
 */
static int get_mck_clock_divisor(u32_t mck)
{
	u32_t mck_divisor;

	if (mck <= 20000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_8;
	} else if (mck <= 40000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_16;
	} else if (mck <= 80000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_32;
	} else if (mck <= 120000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_48;
	} else if (mck <= 160000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_64;
	} else if (mck <= 240000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_96;
	} else {
		LOG_ERR("No valid MDC clock");
		mck_divisor = -ENOTSUP;
	}

	return mck_divisor;
}

#if GMAC_PRIORITY_QUEUE_NO >= 1
static int eth_sam_gmac_setup_qav(Gmac *gmac, int queue_id, bool enable)
{
	/* Verify queue id */
	if (queue_id < 1 || queue_id > GMAC_PRIORITY_QUEUE_NO) {
		return -EINVAL;
	}

	if (queue_id == GMAC_QUE_2) {
		if (enable) {
			gmac->GMAC_CBSCR |= GMAC_CBSCR_QAE;
		} else {
			gmac->GMAC_CBSCR &= ~GMAC_CBSCR_QAE;
		}
	} else {
		if (enable) {
			gmac->GMAC_CBSCR |= GMAC_CBSCR_QBE;
		} else {
			gmac->GMAC_CBSCR &= ~GMAC_CBSCR_QBE;
		}
	}

	return 0;
}

static int eth_sam_gmac_get_qav_status(Gmac *gmac, int queue_id, bool *enabled)
{
	/* Verify queue id */
	if (queue_id < 1 || queue_id > GMAC_PRIORITY_QUEUE_NO) {
		return -EINVAL;
	}

	if (queue_id == GMAC_QUE_2) {
		*enabled = gmac->GMAC_CBSCR & GMAC_CBSCR_QAE;
	} else {
		*enabled = gmac->GMAC_CBSCR & GMAC_CBSCR_QBE;
	}

	return 0;
}

static int eth_sam_gmac_setup_qav_idle_slope(Gmac *gmac, int queue_id,
					     unsigned int idle_slope)
{
	u32_t cbscr_val;

	/* Verify queue id */
	if (queue_id < 1 || queue_id > GMAC_PRIORITY_QUEUE_NO) {
		return -EINVAL;
	}

	cbscr_val = gmac->GMAC_CBSISQA;

	if (queue_id == GMAC_QUE_2) {
		gmac->GMAC_CBSCR &= ~GMAC_CBSCR_QAE;
		gmac->GMAC_CBSISQA = idle_slope;
	} else {
		gmac->GMAC_CBSCR &= ~GMAC_CBSCR_QBE;
		gmac->GMAC_CBSISQB = idle_slope;
	}

	gmac->GMAC_CBSCR = cbscr_val;

	return 0;
}

static u32_t eth_sam_gmac_get_bandwidth(Gmac *gmac)
{
	u32_t bandwidth;

	/* See if we operate in 10Mbps or 100Mbps mode,
	 * Note: according to the manual, portTransmitRate is 0x07735940 for
	 * 1Gbps - therefore we cannot use the KB/MB macros - we have to
	 * multiply it by a round 1000 to get it right.
	 */
	if (gmac->GMAC_NCFGR & GMAC_NCFGR_SPD) {
		/* 100Mbps */
		bandwidth = (100 * 1000 * 1000) / 8;
	} else {
		/* 10Mbps */
		bandwidth = (10 * 1000 * 1000) / 8;
	}

	return bandwidth;
}

static int eth_sam_gmac_get_qav_idle_slope(Gmac *gmac, int queue_id,
					   unsigned int *idle_slope)
{
	/* Verify queue id */
	if (queue_id < 1 || queue_id > GMAC_PRIORITY_QUEUE_NO) {
		return -EINVAL;
	}

	if (queue_id == GMAC_QUE_2) {
		*idle_slope = gmac->GMAC_CBSISQA;
	} else {
		*idle_slope = gmac->GMAC_CBSISQB;
	}

	/* Convert to bps as expected by upper layer */
	*idle_slope *= 8U;

	return 0;
}

static int eth_sam_gmac_get_qav_delta_bandwidth(Gmac *gmac, int queue_id,
						unsigned int *delta_bandwidth)
{
	u32_t bandwidth;
	unsigned int idle_slope;
	int ret;

	ret = eth_sam_gmac_get_qav_idle_slope(gmac, queue_id, &idle_slope);
	if (ret) {
		return ret;
	}

	/* Calculate in Bps */
	idle_slope /= 8U;

	/* Get bandwidth and convert to bps */
	bandwidth = eth_sam_gmac_get_bandwidth(gmac);

	/* Calculate percentage - instead of multiplying idle_slope by 100,
	 * divide bandwidth - these numbers are so large that it should not
	 * influence the outcome and saves us from employing larger data types.
	 */
	*delta_bandwidth = idle_slope / (bandwidth / 100U);

	return 0;
}

static int eth_sam_gmac_setup_qav_delta_bandwidth(Gmac *gmac, int queue_id,
						  int queue_share)
{
	u32_t bandwidth;
	u32_t idle_slope;

	/* Verify queue id */
	if (queue_id < 1 || queue_id > GMAC_PRIORITY_QUEUE_NO) {
		return -EINVAL;
	}

	bandwidth = eth_sam_gmac_get_bandwidth(gmac);

	idle_slope = (bandwidth * queue_share) / 100U;

	return eth_sam_gmac_setup_qav_idle_slope(gmac, queue_id, idle_slope);
}
#endif

#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
static void gmac_setup_ptp_clock_divisors(Gmac *gmac)
{
	int mck_divs[] = {10, 5, 2};
	double min_cycles;
	double min_period;
	int div;
	int i;

	u8_t cns, acns, nit;

	min_cycles = SOC_ATMEL_SAM_MCK_FREQ_HZ;
	min_period = NSEC_PER_SEC;

	for (i = 0; i < ARRAY_SIZE(mck_divs); ++i) {
		div = mck_divs[i];
		while ((double)(min_cycles / div) == (int)(min_cycles / div) &&
		       (double)(min_period / div) == (int)(min_period / div)) {
			min_cycles /= div;
			min_period /= div;
		}
	}

	nit = min_cycles - 1;
	cns = 0U;
	acns = 0U;

	while ((cns + 2) * nit < min_period) {
		cns++;
	}

	acns = min_period - (nit * cns);

	gmac->GMAC_TI =
		GMAC_TI_CNS(cns) | GMAC_TI_ACNS(acns) | GMAC_TI_NIT(nit);
	gmac->GMAC_TISUBN = 0;
}
#endif

static int gmac_init(Gmac *gmac, u32_t gmac_ncfgr_val)
{
	int mck_divisor;

	mck_divisor = get_mck_clock_divisor(SOC_ATMEL_SAM_MCK_FREQ_HZ);
	if (mck_divisor < 0) {
		return mck_divisor;
	}

	/* Set Network Control Register to its default value, clear stats. */
	gmac->GMAC_NCR = GMAC_NCR_CLRSTAT;

	/* Disable all interrupts */
	gmac->GMAC_IDR = UINT32_MAX;
	gmac->GMAC_IDRPQ[GMAC_QUE_1 - 1] = UINT32_MAX;
	gmac->GMAC_IDRPQ[GMAC_QUE_2 - 1] = UINT32_MAX;
	/* Clear all interrupts */
	(void)gmac->GMAC_ISR;
	(void)gmac->GMAC_ISRPQ[GMAC_QUE_1 - 1];
	(void)gmac->GMAC_ISRPQ[GMAC_QUE_2 - 1];
	/* Setup Hash Registers - enable reception of all multicast frames when
	 * GMAC_NCFGR_MTIHEN is set.
	 */
	gmac->GMAC_HRB = UINT32_MAX;
	gmac->GMAC_HRT = UINT32_MAX;
	/* Setup Network Configuration Register */
	gmac->GMAC_NCFGR = gmac_ncfgr_val | mck_divisor;

#ifdef CONFIG_ETH_SAM_GMAC_MII
	/* Setup MII Interface to the Physical Layer, RMII is the default */
	gmac->GMAC_UR = GMAC_UR_RMII; /* setting RMII to 1 selects MII mode */
#endif

#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	/* Initialize PTP Clock Registers */
	gmac_setup_ptp_clock_divisors(gmac);

	gmac->GMAC_TN = 0;
	gmac->GMAC_TSH = 0;
	gmac->GMAC_TSL = 0;
#endif

	/* Enable Qav if priority queues are used, and setup the default delta
	 * bandwidth according to IEEE802.1Qav (34.3.1)
	 */
#if GMAC_PRIORITY_QUEUE_NO == 1
	eth_sam_gmac_setup_qav_delta_bandwidth(gmac, 1, 75);
	eth_sam_gmac_setup_qav(gmac, 1, true);
#elif GMAC_PRIORITY_QUEUE_NO == 2
	/* For multiple priority queues, 802.1Qav suggests using 75% for the
	 * highest priority queue, and 0% for the lower priority queues.
	 * This is because the lower priority queues are supposed to be using
	 * the bandwidth available from the higher priority queues AND its own
	 * available bandwidth (see 802.1Q 34.3.1 for more details).
	 * This does not work like that in SAM GMAC - the lower priority queues
	 * are not using the bandwidth reserved for the higher priority queues
	 * at all. Thus we still set the default to a total of the recommended
	 * 75%, but split the bandwidth between them manually.
	 */
	eth_sam_gmac_setup_qav_delta_bandwidth(gmac, 1, 25);
	eth_sam_gmac_setup_qav_delta_bandwidth(gmac, 2, 50);
	eth_sam_gmac_setup_qav(gmac, 1, true);
	eth_sam_gmac_setup_qav(gmac, 2, true);
#endif

	return 0;
}

static void link_configure(Gmac *gmac, u32_t flags)
{
	u32_t val;

	gmac->GMAC_NCR &= ~(GMAC_NCR_RXEN | GMAC_NCR_TXEN);

	val = gmac->GMAC_NCFGR;

	val &= ~(GMAC_NCFGR_FD | GMAC_NCFGR_SPD);
	val |= flags & (GMAC_NCFGR_FD | GMAC_NCFGR_SPD);

	gmac->GMAC_NCFGR = val;

	gmac->GMAC_UR = 0;  /* Select RMII mode */
	gmac->GMAC_NCR |= (GMAC_NCR_RXEN | GMAC_NCR_TXEN);
}

static int nonpriority_queue_init(Gmac *gmac, struct gmac_queue *queue)
{
	int result;

	__ASSERT_NO_MSG(queue->rx_desc_list.len > 0);
	__ASSERT_NO_MSG(queue->tx_desc_list.len > 0);
	__ASSERT(!((u32_t)queue->rx_desc_list.buf & ~GMAC_RBQB_ADDR_Msk),
		 "RX descriptors have to be word aligned");
	__ASSERT(!((u32_t)queue->tx_desc_list.buf & ~GMAC_TBQB_ADDR_Msk),
		 "TX descriptors have to be word aligned");

	/* Setup descriptor lists */
	result = rx_descriptors_init(gmac, queue);
	if (result < 0) {
		return result;
	}

	tx_descriptors_init(gmac, queue);

#if GMAC_MULTIPLE_TX_PACKETS == 0
	/* Initialize TX semaphore. This semaphore is used to wait until the TX
	 * data has been sent.
	 */
	k_sem_init(&queue->tx_sem, 0, 1);
#else
	/* Initialize TX descriptors semaphore. The semaphore is required as the
	 * size of the TX descriptor list is limited while the number of TX data
	 * buffers is not.
	 */
	k_sem_init(&queue->tx_desc_sem, queue->tx_desc_list.len - 1,
		   queue->tx_desc_list.len - 1);
#endif

	/* Set Receive Buffer Queue Pointer Register */
	gmac->GMAC_RBQB = (u32_t)queue->rx_desc_list.buf;
	/* Set Transmit Buffer Queue Pointer Register */
	gmac->GMAC_TBQB = (u32_t)queue->tx_desc_list.buf;

	/* Configure GMAC DMA transfer */
	gmac->GMAC_DCFGR =
		  /* Receive Buffer Size (defined in multiples of 64 bytes) */
		  GMAC_DCFGR_DRBS(CONFIG_NET_BUF_DATA_SIZE >> 6)
		  /* 4 kB Receiver Packet Buffer Memory Size */
		| GMAC_DCFGR_RXBMS_FULL
		  /* 4 kB Transmitter Packet Buffer Memory Size */
		| GMAC_DCFGR_TXPBMS
		  /* Transmitter Checksum Generation Offload Enable */
		| GMAC_DCFGR_TXCOEN
		  /* Attempt to use INCR4 AHB bursts (Default) */
		| GMAC_DCFGR_FBLDO_INCR4;

	/* Setup RX/TX completion and error interrupts */
	gmac->GMAC_IER = GMAC_INT_EN_FLAGS;

	queue->err_rx_frames_dropped = 0U;
	queue->err_rx_flushed_count = 0U;
	queue->err_tx_flushed_count = 0U;

	LOG_INF("Queue %d activated", queue->que_idx);

	return 0;
}

static int priority_queue_init(Gmac *gmac, struct gmac_queue *queue)
{
	int result;
	int queue_index;

	__ASSERT_NO_MSG(queue->rx_desc_list.len > 0);
	__ASSERT_NO_MSG(queue->tx_desc_list.len > 0);
	__ASSERT(!((u32_t)queue->rx_desc_list.buf & ~GMAC_RBQB_ADDR_Msk),
		 "RX descriptors have to be word aligned");
	__ASSERT(!((u32_t)queue->tx_desc_list.buf & ~GMAC_TBQB_ADDR_Msk),
		 "TX descriptors have to be word aligned");

	/* Extract queue index for easier referencing */
	queue_index = queue->que_idx - 1;

	/* Setup descriptor lists */
	result = rx_descriptors_init(gmac, queue);
	if (result < 0) {
		return result;
	}

	tx_descriptors_init(gmac, queue);

#if GMAC_MULTIPLE_TX_PACKETS == 0
	k_sem_init(&queue->tx_sem, 0, 1);
#else
	k_sem_init(&queue->tx_desc_sem, queue->tx_desc_list.len - 1,
		   queue->tx_desc_list.len - 1);
#endif

	/* Setup RX buffer size for DMA */
	gmac->GMAC_RBSRPQ[queue_index] =
		GMAC_RBSRPQ_RBS(CONFIG_NET_BUF_DATA_SIZE >> 6);

	/* Set Receive Buffer Queue Pointer Register */
	gmac->GMAC_RBQBAPQ[queue_index] = (u32_t)queue->rx_desc_list.buf;
	/* Set Transmit Buffer Queue Pointer Register */
	gmac->GMAC_TBQBAPQ[queue_index] = (u32_t)queue->tx_desc_list.buf;

	/* Enable RX/TX completion and error interrupts */
	gmac->GMAC_IERPQ[queue_index] = GMAC_INTPQ_EN_FLAGS;

	queue->err_rx_frames_dropped = 0U;
	queue->err_rx_flushed_count = 0U;
	queue->err_tx_flushed_count = 0U;

	LOG_INF("Queue %d activated", queue->que_idx);

	return 0;
}

static int priority_queue_init_as_idle(Gmac *gmac, struct gmac_queue *queue)
{
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;

	__ASSERT(!((u32_t)rx_desc_list->buf & ~GMAC_RBQB_ADDR_Msk),
		 "RX descriptors have to be word aligned");
	__ASSERT(!((u32_t)tx_desc_list->buf & ~GMAC_TBQB_ADDR_Msk),
		 "TX descriptors have to be word aligned");
	__ASSERT((rx_desc_list->len == 1U) && (tx_desc_list->len == 1U),
		 "Priority queues are currently not supported, descriptor "
		 "list has to have a single entry");

	/* Setup RX descriptor lists */
	/* Take ownership from GMAC and set the wrap bit */
	rx_desc_list->buf[0].w0 = GMAC_RXW0_WRAP;
	rx_desc_list->buf[0].w1 = 0U;
	/* Setup TX descriptor lists */
	tx_desc_list->buf[0].w0 = 0U;
	/* Take ownership from GMAC and set the wrap bit */
	tx_desc_list->buf[0].w1 = GMAC_TXW1_USED | GMAC_TXW1_WRAP;

	/* Set Receive Buffer Queue Pointer Register */
	gmac->GMAC_RBQBAPQ[queue->que_idx - 1] = (u32_t)rx_desc_list->buf;
	/* Set Transmit Buffer Queue Pointer Register */
	gmac->GMAC_TBQBAPQ[queue->que_idx - 1] = (u32_t)tx_desc_list->buf;

	LOG_INF("Queue %d set to idle", queue->que_idx);

	return 0;
}

static int queue_init(Gmac *gmac, struct gmac_queue *queue)
{
	if (queue->que_idx == 0) {
		return nonpriority_queue_init(gmac, queue);
	} else if (queue->que_idx <= GMAC_PRIORITY_QUEUE_NO) {
		return priority_queue_init(gmac, queue);
	} else {
		return priority_queue_init_as_idle(gmac, queue);
	}
}

static struct net_pkt *frame_get(struct gmac_queue *queue)
{
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct gmac_desc *rx_desc;
	struct net_buf **rx_frag_list = queue->rx_frag_list;
	struct net_pkt *rx_frame;
	bool frame_is_complete;
	struct net_buf *frag;
	struct net_buf *new_frag;
	struct net_buf *last_frag = NULL;
	u8_t *frag_data;
	u32_t frag_len;
	u32_t frame_len = 0U;
	u16_t tail;
	u8_t wrap;

	/* Check if there exists a complete frame in RX descriptor list */
	tail = rx_desc_list->tail;
	rx_desc = &rx_desc_list->buf[tail];
	frame_is_complete = false;
	while ((rx_desc->w0 & GMAC_RXW0_OWNERSHIP)
		&& !frame_is_complete) {
		frame_is_complete = (bool)(rx_desc->w1
					   & GMAC_RXW1_EOF);
		MODULO_INC(tail, rx_desc_list->len);
		rx_desc = &rx_desc_list->buf[tail];
	}
	/* Frame which is not complete can be dropped by GMAC. Do not process
	 * it, even partially.
	 */
	if (!frame_is_complete) {
		return NULL;
	}

	rx_frame = net_pkt_rx_alloc(K_NO_WAIT);

	/* Process a frame */
	tail = rx_desc_list->tail;
	rx_desc = &rx_desc_list->buf[tail];
	frame_is_complete = false;

	/* TODO: Don't assume first RX fragment will have SOF (Start of frame)
	 * bit set. If SOF bit is missing recover gracefully by dropping
	 * invalid frame.
	 */
	__ASSERT(rx_desc->w1 & GMAC_RXW1_SOF,
		 "First RX fragment is missing SOF bit");

	/* TODO: We know already tail and head indexes of fragments containing
	 * complete frame. Loop over those indexes, don't search for them
	 * again.
	 */
	while ((rx_desc->w0 & GMAC_RXW0_OWNERSHIP)
	       && !frame_is_complete) {
		frag = rx_frag_list[tail];
		frag_data =
			(u8_t *)(rx_desc->w0 & GMAC_RXW0_ADDR);
		__ASSERT(frag->data == frag_data,
			 "RX descriptor and buffer list desynchronized");
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
			dcache_invalidate((u32_t)frag_data, frag->size);

			/* Get a new data net buffer from the buffer pool */
			new_frag = net_pkt_get_frag(rx_frame, K_NO_WAIT);
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
		__DMB();  /* data memory barrier */
		/* Update buffer descriptor address word */
		wrap = (tail == rx_desc_list->len-1U ? GMAC_RXW0_WRAP : 0);
		rx_desc->w0 = ((u32_t)frag->data & GMAC_RXW0_ADDR) | wrap;

		MODULO_INC(tail, rx_desc_list->len);
		rx_desc = &rx_desc_list->buf[tail];
	}

	rx_desc_list->tail = tail;
	LOG_DBG("Frame complete: rx=%p, tail=%d", rx_frame, tail);
	__ASSERT_NO_MSG(frame_is_complete);

	return rx_frame;
}

static void eth_rx(struct gmac_queue *queue)
{
	struct eth_sam_dev_data *dev_data =
		CONTAINER_OF(queue, struct eth_sam_dev_data,
			     queue_list[queue->que_idx]);
	u16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct net_pkt *rx_frame;
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	struct device *const dev = net_if_get_device(dev_data->iface);
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	Gmac *gmac = cfg->regs;
	struct gptp_hdr *hdr;
#endif

	/* More than one frame could have been received by GMAC, get all
	 * complete frames stored in the GMAC RX descriptor list.
	 */
	rx_frame = frame_get(queue);
	while (rx_frame) {
		LOG_DBG("ETH rx");

#if defined(CONFIG_NET_VLAN)
		/* FIXME: Instead of this, use the GMAC register to get
		 * the used VLAN tag.
		 */
		{
			struct net_eth_hdr *hdr = NET_ETH_HDR(rx_frame);

			if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
				struct net_eth_vlan_hdr *hdr_vlan =
					(struct net_eth_vlan_hdr *)
					NET_ETH_HDR(rx_frame);

				net_pkt_set_vlan_tci(rx_frame,
						    ntohs(hdr_vlan->vlan.tci));
				vlan_tag = net_pkt_vlan_tag(rx_frame);

#if CONFIG_NET_TC_RX_COUNT > 1
				{
					enum net_priority prio;

					prio = net_vlan2priority(
					      net_pkt_vlan_priority(rx_frame));
					net_pkt_set_priority(rx_frame, prio);
				}
#endif
			}
		}
#endif
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
		hdr = check_gptp_msg(get_iface(dev_data, vlan_tag), rx_frame,
				     false);

		timestamp_rx_pkt(gmac, hdr, rx_frame);

		if (hdr) {
			update_pkt_priority(hdr, rx_frame);
		}
#endif /* CONFIG_PTP_CLOCK_SAM_GMAC */

		if (net_recv_data(get_iface(dev_data, vlan_tag),
				  rx_frame) < 0) {
			eth_stats_update_errors_rx(get_iface(dev_data,
							     vlan_tag));
			net_pkt_unref(rx_frame);
		}

		rx_frame = frame_get(queue);
	}
}

#if !defined(CONFIG_ETH_SAM_GMAC_FORCE_QUEUE) && \
	((CONFIG_ETH_SAM_GMAC_QUEUES != NET_TC_TX_COUNT) || \
	((NET_TC_TX_COUNT != NET_TC_RX_COUNT) && defined(CONFIG_NET_VLAN)))
static int priority2queue(enum net_priority priority)
{
	static const u8_t queue_priority_map[] = {
#if CONFIG_ETH_SAM_GMAC_QUEUES == 1
		0, 0, 0, 0, 0, 0, 0, 0
#endif
#if CONFIG_ETH_SAM_GMAC_QUEUES == 2
		0, 0, 0, 0, 1, 1, 1, 1
#endif
#if CONFIG_ETH_SAM_GMAC_QUEUES == 3
		0, 0, 0, 0, 1, 1, 2, 2
#endif
	};

	return queue_priority_map[priority];
}
#endif

static int eth_tx(struct device *dev, struct net_pkt *pkt)
{
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);
	Gmac *gmac = cfg->regs;
	struct gmac_queue *queue;
	struct gmac_desc_list *tx_desc_list;
	struct gmac_desc *tx_desc;
	struct gmac_desc *tx_first_desc;
	struct net_buf *frag;
	u8_t *frag_data;
	u16_t frag_len;
	u32_t err_tx_flushed_count_at_entry;
#if GMAC_MULTIPLE_TX_PACKETS == 1
	unsigned int key;
#endif
	u8_t pkt_prio;
#if GMAC_MULTIPLE_TX_PACKETS == 0
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	u16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct gptp_hdr *hdr;
#if defined(CONFIG_NET_VLAN)
	struct net_eth_hdr *eth_hdr;
#endif
#endif
#endif

	__ASSERT(pkt, "buf pointer is NULL");
	__ASSERT(pkt->frags, "Frame data missing");

	LOG_DBG("ETH tx");

	/* Decide which queue should be used */
	pkt_prio = net_pkt_priority(pkt);

#if defined(CONFIG_ETH_SAM_GMAC_FORCE_QUEUE)
	/* Route eveything to the forced queue */
	queue = &dev_data->queue_list[CONFIG_ETH_SAM_GMAC_FORCED_QUEUE];
#elif CONFIG_ETH_SAM_GMAC_QUEUES == CONFIG_NET_TC_TX_COUNT
	/* Prefer to chose queue based on its traffic class */
	queue = &dev_data->queue_list[net_tx_priority2tc(pkt_prio)];
#else
	/* If that's not possible due to config - use builtin mapping */
	queue = &dev_data->queue_list[priority2queue(pkt_prio)];
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
		dcache_clean((u32_t)frag_data, frag->size);

#if GMAC_MULTIPLE_TX_PACKETS == 1
		k_sem_take(&queue->tx_desc_sem, K_FOREVER);

		/* The following section becomes critical and requires IRQ lock
		 * / unlock protection only due to the possibility of executing
		 * tx_error_handler() function.
		 */
		key = irq_lock();

		/* Check if tx_error_handler() function was executed */
		if (queue->err_tx_flushed_count !=
		    err_tx_flushed_count_at_entry) {
			irq_unlock(key);
			return -EIO;
		}
#endif

		tx_desc = &tx_desc_list->buf[tx_desc_list->head];

		/* Update buffer descriptor address word */
		tx_desc->w0 = (u32_t)frag_data;

		/* Update buffer descriptor status word (clear used bit except
		 * for the first frag).
		 */
		tx_desc->w1 = (frag_len & GMAC_TXW1_LEN)
			    | (!frag->frags ? GMAC_TXW1_LASTBUFFER : 0)
			    | (tx_desc_list->head == tx_desc_list->len - 1U
			       ? GMAC_TXW1_WRAP : 0)
			    | (tx_desc == tx_first_desc ? GMAC_TXW1_USED : 0);

		/* Update descriptor position */
		MODULO_INC(tx_desc_list->head, tx_desc_list->len);

#if GMAC_MULTIPLE_TX_PACKETS == 1
		__ASSERT(tx_desc_list->head != tx_desc_list->tail,
			 "tx_desc_list overflow");

		/* Account for a sent frag */
		ring_buf_put(&queue->tx_frag_list, POINTER_TO_UINT(frag));

		/* frag is internally queued, so it requires to hold a reference */
		net_pkt_frag_ref(frag);

		irq_unlock(key);
#endif

		/* Continue with the rest of fragments (only data) */
		frag = frag->frags;
	}

#if GMAC_MULTIPLE_TX_PACKETS == 1
	key = irq_lock();

	/* Check if tx_error_handler() function was executed */
	if (queue->err_tx_flushed_count != err_tx_flushed_count_at_entry) {
		irq_unlock(key);
		return -EIO;
	}
#endif

	/* Ensure the descriptor following the last one is marked as used */
	tx_desc_list->buf[tx_desc_list->head].w1 = GMAC_TXW1_USED;

	/* Guarantee that all the fragments have been written before removing
	 * the used bit to avoid race condition.
	 */
	__DMB();  /* data memory barrier */

	/* Remove the used bit of the first fragment to allow the controller
	 * to process it and the following fragments.
	 */
	tx_first_desc->w1 &= ~GMAC_TXW1_USED;

#if GMAC_MULTIPLE_TX_PACKETS == 1
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	/* Account for a sent frame */
	ring_buf_put(&queue->tx_frames, POINTER_TO_UINT(pkt));

	/* pkt is internally queued, so it requires to hold a reference */
	net_pkt_ref(pkt);
#endif

	irq_unlock(key);
#endif

	/* Guarantee that the first fragment got its bit removed before starting
	 * sending packets to avoid packets getting stuck.
	 */
	__DMB();  /* data memory barrier */

	/* Start transmission */
	gmac->GMAC_NCR |= GMAC_NCR_TSTART;

#if GMAC_MULTIPLE_TX_PACKETS == 0
	/* Wait until the packet is sent */
	k_sem_take(&queue->tx_sem, K_FOREVER);

	/* Check if transmit successful or not */
	if (queue->err_tx_flushed_count != err_tx_flushed_count_at_entry) {
		return -EIO;
	}
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
#if defined(CONFIG_NET_VLAN)
	eth_hdr = NET_ETH_HDR(pkt);
	if (ntohs(eth_hdr->type) == NET_ETH_PTYPE_VLAN) {
		vlan_tag = net_pkt_vlan_tag(pkt);
	}
#endif
	hdr = check_gptp_msg(get_iface(dev_data, vlan_tag), pkt, true);
	timestamp_tx_pkt(gmac, hdr, pkt);
	if (hdr && need_timestamping(hdr)) {
		net_if_add_tx_timestamp(pkt);
	}
#endif
#endif

	return 0;
}

static void queue0_isr(void *arg)
{
	struct device *const dev = (struct device *const)arg;
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);
	Gmac *gmac = cfg->regs;
	struct gmac_queue *queue;
	struct gmac_desc_list *rx_desc_list;
	struct gmac_desc_list *tx_desc_list;
	struct gmac_desc *tail_desc;
	u32_t isr;

	/* Interrupt Status Register is cleared on read */
	isr = gmac->GMAC_ISR;
	LOG_DBG("GMAC_ISR=0x%08x", isr);

	queue = &dev_data->queue_list[0];
	rx_desc_list = &queue->rx_desc_list;
	tx_desc_list = &queue->tx_desc_list;

	/* RX packet */
	if (isr & GMAC_INT_RX_ERR_BITS) {
		rx_error_handler(gmac, queue);
	} else if (isr & GMAC_ISR_RCOMP) {
		tail_desc = &rx_desc_list->buf[rx_desc_list->tail];
		LOG_DBG("rx.w1=0x%08x, tail=%d",
			tail_desc->w1,
			rx_desc_list->tail);
		eth_rx(queue);
	}

	/* TX packet */
	if (isr & GMAC_INT_TX_ERR_BITS) {
		tx_error_handler(gmac, queue);
	} else if (isr & GMAC_ISR_TCOMP) {
#if GMAC_MULTIPLE_TX_PACKETS == 1
		tail_desc = &tx_desc_list->buf[tx_desc_list->tail];
		LOG_DBG("tx.w1=0x%08x, tail=%d",
			tail_desc->w1,
			tx_desc_list->tail);
#endif

		tx_completed(gmac, queue);
	}

	if (isr & GMAC_IER_HRESP) {
		LOG_DBG("IER HRESP");
	}
}

#if GMAC_PRIORITY_QUEUE_NO >= 1
static inline void priority_queue_isr(void *arg, unsigned int queue_idx)
{
	struct device *const dev = (struct device *const)arg;
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);
	Gmac *gmac = cfg->regs;
	struct gmac_queue *queue;
	struct gmac_desc_list *rx_desc_list;
	struct gmac_desc_list *tx_desc_list;
	struct gmac_desc *tail_desc;
	u32_t isrpq;

	isrpq = gmac->GMAC_ISRPQ[queue_idx - 1];
	LOG_DBG("GMAC_ISRPQ%d=0x%08x", queue_idx - 1,  isrpq);

	queue = &dev_data->queue_list[queue_idx];
	rx_desc_list = &queue->rx_desc_list;
	tx_desc_list = &queue->tx_desc_list;

	/* RX packet */
	if (isrpq & GMAC_INTPQ_RX_ERR_BITS) {
		rx_error_handler(gmac, queue);
	} else if (isrpq & GMAC_ISRPQ_RCOMP) {
		tail_desc = &rx_desc_list->buf[rx_desc_list->tail];
		LOG_DBG("rx.w1=0x%08x, tail=%d",
			tail_desc->w1,
			rx_desc_list->tail);
		eth_rx(queue);
	}

	/* TX packet */
	if (isrpq & GMAC_INTPQ_TX_ERR_BITS) {
		tx_error_handler(gmac, queue);
	} else if (isrpq & GMAC_ISRPQ_TCOMP) {
#if GMAC_MULTIPLE_TX_PACKETS == 1
		tail_desc = &tx_desc_list->buf[tx_desc_list->tail];
		LOG_DBG("tx.w1=0x%08x, tail=%d",
			tail_desc->w1,
			tx_desc_list->tail);
#endif

		tx_completed(gmac, queue);
	}

	if (isrpq & GMAC_IERPQ_HRESP) {
		LOG_DBG("IERPQ%d HRESP", queue_idx - 1);
	}
}
#endif

#if GMAC_PRIORITY_QUEUE_NO >= 1
static void queue1_isr(void *arg)
{
	priority_queue_isr(arg, 1);
}
#endif

#if GMAC_PRIORITY_QUEUE_NO == 2
static void queue2_isr(void *arg)
{
	priority_queue_isr(arg, 2);
}
#endif

static int eth_initialize(struct device *dev)
{
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);

	cfg->config_func();

	/* Enable GMAC module's clock */
	soc_pmc_peripheral_enable(cfg->periph_id);

	/* Connect pins to the peripheral */
	soc_gpio_list_configure(cfg->pin_list, cfg->pin_list_size);

	return 0;
}

#ifdef CONFIG_ETH_SAM_GMAC_MAC_I2C_EEPROM
static void get_mac_addr_from_i2c_eeprom(u8_t mac_addr[6])
{
	struct device *dev;
	u32_t iaddr = CONFIG_ETH_SAM_GMAC_MAC_I2C_INT_ADDRESS;

	dev = device_get_binding(CONFIG_ETH_SAM_GMAC_MAC_I2C_DEV_NAME);
	if (!dev) {
		LOG_ERR("I2C: Device not found");
		return;
	}

	i2c_write_read(dev, CONFIG_ETH_SAM_GMAC_MAC_I2C_SLAVE_ADDRESS,
		       &iaddr, CONFIG_ETH_SAM_GMAC_MAC_I2C_INT_ADDRESS_SIZE,
		       mac_addr, 6);
}
#endif

#if defined(CONFIG_ETH_SAM_GMAC_RANDOM_MAC)
static void generate_random_mac(u8_t mac_addr[6])
{
	u32_t entropy;

	entropy = sys_rand32_get();

	mac_addr[0] = ATMEL_OUI_B0 | 0x02; /* force LAA bit */
	mac_addr[1] = ATMEL_OUI_B1;
	mac_addr[2] = ATMEL_OUI_B2;

	mac_addr[3] = entropy >> 8;
	mac_addr[4] = entropy >> 16;
	mac_addr[5] = entropy >> 0;
}
#endif

static void generate_mac(u8_t mac_addr[6])
{
#if defined(CONFIG_ETH_SAM_GMAC_MAC_I2C_EEPROM)
	get_mac_addr_from_i2c_eeprom(mac_addr);
#elif defined(CONFIG_ETH_SAM_GMAC_RANDOM_MAC)
	generate_random_mac(mac_addr);
#endif
}

static void eth0_iface_init(struct net_if *iface)
{
	struct device *const dev = net_if_get_device(iface);
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	static bool init_done;
	u32_t gmac_ncfgr_val;
	u32_t link_status;
	int result;
	int i;

	/* For VLAN, this value is only used to get the correct L2 driver */
	dev_data->iface = iface;

	ethernet_init(iface);

	/* The rest of initialization should only be done once */
	if (init_done) {
		return;
	}

	/* Check the status of data caches */
	dcache_enabled = (SCB->CCR & SCB_CCR_DC_Msk);

	/* Initialize GMAC driver, maximum frame length is 1518 bytes */
	gmac_ncfgr_val =
		  GMAC_NCFGR_MTIHEN  /* Multicast Hash Enable */
		| GMAC_NCFGR_LFERD   /* Length Field Error Frame Discard */
		| GMAC_NCFGR_RFCS    /* Remove Frame Check Sequence */
		| GMAC_NCFGR_RXCOEN; /* Receive Checksum Offload Enable */
	result = gmac_init(cfg->regs, gmac_ncfgr_val);
	if (result < 0) {
		LOG_ERR("Unable to initialize ETH driver");
		return;
	}

	generate_mac(dev_data->mac_addr);

	LOG_INF("MAC: %02x:%02x:%02x:%02x:%02x:%02x",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

	/* Set MAC Address for frame filtering logic */
	mac_addr_set(cfg->regs, 0, dev_data->mac_addr);

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	/* Initialize GMAC queues */
	for (i = 0; i < GMAC_QUEUE_NO; i++) {
		result = queue_init(cfg->regs, &dev_data->queue_list[i]);
		if (result < 0) {
			LOG_ERR("Unable to initialize ETH queue%d", i);
			return;
		}
	}

#if GMAC_PRIORITY_QUEUE_NO >= 1
#if defined(CONFIG_ETH_SAM_GMAC_FORCE_QUEUE)
	for (i = 0; i < CONFIG_NET_TC_RX_COUNT; ++i) {
		cfg->regs->GMAC_ST1RPQ[i] =
			GMAC_ST1RPQ_DSTCM(i) |
			GMAC_ST1RPQ_QNB(CONFIG_ETH_SAM_GMAC_FORCED_QUEUE);
	}
#elif CONFIG_ETH_SAM_GMAC_QUEUES == NET_TC_RX_COUNT
	/* If TC configuration is compatible with HW configuration, setup the
	 * screening registers based on the DS/TC values.
	 * Map them 1:1 - TC 0 -> Queue 0, TC 1 -> Queue 1 etc.
	 */
	for (i = 0; i < CONFIG_NET_TC_RX_COUNT; ++i) {
		cfg->regs->GMAC_ST1RPQ[i] =
			GMAC_ST1RPQ_DSTCM(i) | GMAC_ST1RPQ_QNB(i);
	}
#elif defined(CONFIG_NET_VLAN)
	/* If VLAN is enabled, route packets according to VLAN priority */
	int j;

	i = 0;
	for (j = NET_PRIORITY_NC; j >= 0; --j) {
		if (priority2queue(j) == 0) {
			/* No point to set rules for the regular queue */
			continue;
		}

		if (i >= ARRAY_SIZE(cfg->regs->GMAC_ST2RPQ)) {
			/* No more screening registers available */
			break;
		}

		cfg->regs->GMAC_ST2RPQ[i++] =
			GMAC_ST2RPQ_QNB(priority2queue(j))
			| GMAC_ST2RPQ_VLANP(j)
			| GMAC_ST2RPQ_VLANE;
	}

#endif
#endif

	/* PHY initialize */
	result = phy_sam_gmac_init(&cfg->phy);
	if (result < 0) {
		LOG_ERR("ETH PHY Initialization Error");
		return;
	}
	/* PHY auto-negotiate link parameters */
	result = phy_sam_gmac_auto_negotiate(&cfg->phy, &link_status);
	if (result < 0) {
		LOG_ERR("ETH PHY auto-negotiate sequence failed");
		return;
	}

	/* Set up link parameters */
	link_configure(cfg->regs, link_status);

	init_done = true;
}

static enum ethernet_hw_caps eth_sam_gmac_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_HW_VLAN | ETHERNET_LINK_10BASE_T |
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
		ETHERNET_PTP |
#endif
		ETHERNET_PRIORITY_QUEUES |
#if GMAC_PRIORITY_QUEUE_NO >= 1
		ETHERNET_QAV |
#endif
		ETHERNET_LINK_100BASE_T;
}

#if GMAC_PRIORITY_QUEUE_NO >= 1
static int eth_sam_gmac_set_qav_param(struct device *dev,
				      enum ethernet_config_type type,
				      const struct ethernet_config *config)
{
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	Gmac *gmac = cfg->regs;
	enum ethernet_qav_param_type qav_param_type;
	unsigned int delta_bandwidth;
	unsigned int idle_slope;
	int queue_id;
	bool enable;

	/* Priority queue IDs start from 1 for SAM GMAC */
	queue_id = config->qav_param.queue_id + 1;

	qav_param_type = config->qav_param.type;

	switch (qav_param_type) {
	case ETHERNET_QAV_PARAM_TYPE_STATUS:
		enable = config->qav_param.enabled;
		return eth_sam_gmac_setup_qav(gmac, queue_id, enable);
	case ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH:
		delta_bandwidth = config->qav_param.delta_bandwidth;

		return eth_sam_gmac_setup_qav_delta_bandwidth(gmac, queue_id,
							      delta_bandwidth);
	case ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE:
		idle_slope = config->qav_param.idle_slope;

		/* The standard uses bps, SAM GMAC uses Bps - convert now */
		idle_slope /= 8U;

		return eth_sam_gmac_setup_qav_idle_slope(gmac, queue_id,
							 idle_slope);
	default:
		break;
	}

	return -ENOTSUP;
}
#endif

static int eth_sam_gmac_set_config(struct device *dev,
				   enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	switch (type) {
#if GMAC_PRIORITY_QUEUE_NO >= 1
	case ETHERNET_CONFIG_TYPE_QAV_PARAM:
		return eth_sam_gmac_set_qav_param(dev, type, config);
#endif
	default:
		break;
	}

	return -ENOTSUP;
}

#if GMAC_PRIORITY_QUEUE_NO >= 1
static int eth_sam_gmac_get_qav_param(struct device *dev,
				      enum ethernet_config_type type,
				      struct ethernet_config *config)
{
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	Gmac *gmac = cfg->regs;
	enum ethernet_qav_param_type qav_param_type;
	int queue_id;
	bool *enabled;
	unsigned int *idle_slope;
	unsigned int *delta_bandwidth;

	/* Priority queue IDs start from 1 for SAM GMAC */
	queue_id = config->qav_param.queue_id + 1;

	qav_param_type = config->qav_param.type;

	switch (qav_param_type) {
	case ETHERNET_QAV_PARAM_TYPE_STATUS:
		enabled = &config->qav_param.enabled;
		return eth_sam_gmac_get_qav_status(gmac, queue_id, enabled);
	case ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE:
		idle_slope = &config->qav_param.idle_slope;
		return eth_sam_gmac_get_qav_idle_slope(gmac, queue_id,
						       idle_slope);
	case ETHERNET_QAV_PARAM_TYPE_OPER_IDLE_SLOPE:
		idle_slope = &config->qav_param.oper_idle_slope;
		return eth_sam_gmac_get_qav_idle_slope(gmac, queue_id,
						       idle_slope);
	case ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH:
		delta_bandwidth = &config->qav_param.delta_bandwidth;
		return eth_sam_gmac_get_qav_delta_bandwidth(gmac, queue_id,
							    delta_bandwidth);
	case ETHERNET_QAV_PARAM_TYPE_TRAFFIC_CLASS:
#if CONFIG_ETH_SAM_GMAC_QUEUES == NET_TC_TX_COUNT
		config->qav_param.traffic_class = queue_id;
		return 0;
#else
		/* Invalid configuration - no direct TC to queue mapping */
		return -ENOTSUP;
#endif
	default:
		break;
	}

	return -ENOTSUP;
}
#endif

static int eth_sam_gmac_get_config(struct device *dev,
				   enum ethernet_config_type type,
				   struct ethernet_config *config)
{
	switch (type) {
	case ETHERNET_CONFIG_TYPE_PRIORITY_QUEUES_NUM:
		config->priority_queues_num = GMAC_PRIORITY_QUEUE_NO;
		return 0;
#if GMAC_PRIORITY_QUEUE_NO >= 1
	case ETHERNET_CONFIG_TYPE_QAV_PARAM:
		return eth_sam_gmac_get_qav_param(dev, type, config);
#endif
	default:
		break;
	}

	return -ENOTSUP;
}

#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
static struct device *eth_sam_gmac_get_ptp_clock(struct device *dev)
{
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);

	return dev_data->ptp_clock;
}
#endif

static const struct ethernet_api eth_api = {
	.iface_api.init = eth0_iface_init,

	.get_capabilities = eth_sam_gmac_get_capabilities,
	.set_config = eth_sam_gmac_set_config,
	.get_config = eth_sam_gmac_get_config,
	.send = eth_tx,

#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
	.get_ptp_clock = eth_sam_gmac_get_ptp_clock,
#endif
};

static struct device DEVICE_NAME_GET(eth0_sam_gmac);

static void eth0_irq_config(void)
{
	IRQ_CONNECT(GMAC_IRQn, CONFIG_ETH_SAM_GMAC_IRQ_PRI, queue0_isr,
		    DEVICE_GET(eth0_sam_gmac), 0);
	irq_enable(GMAC_IRQn);

#if GMAC_PRIORITY_QUEUE_NO >= 1
	IRQ_CONNECT(GMAC_Q1_IRQn, CONFIG_ETH_SAM_GMAC_IRQ_PRI, queue1_isr,
		    DEVICE_GET(eth0_sam_gmac), 0);
	irq_enable(GMAC_Q1_IRQn);
#endif

#if GMAC_PRIORITY_QUEUE_NO == 2
	IRQ_CONNECT(GMAC_Q2_IRQn, CONFIG_ETH_SAM_GMAC_IRQ_PRI, queue2_isr,
		    DEVICE_GET(eth0_sam_gmac), 0);
	irq_enable(GMAC_Q2_IRQn);
#endif
}

static const struct soc_gpio_pin pins_eth0[] = PINS_GMAC0;

static const struct eth_sam_dev_cfg eth0_config = {
	.regs = GMAC,
	.periph_id = ID_GMAC,
	.pin_list = pins_eth0,
	.pin_list_size = ARRAY_SIZE(pins_eth0),
	.config_func = eth0_irq_config,
	.phy = {GMAC, CONFIG_ETH_SAM_GMAC_PHY_ADDR},
};

static struct eth_sam_dev_data eth0_data = {
#ifdef CONFIG_ETH_SAM_GMAC_MAC_MANUAL
	.mac_addr = {
		CONFIG_ETH_SAM_GMAC_MAC0,
		CONFIG_ETH_SAM_GMAC_MAC1,
		CONFIG_ETH_SAM_GMAC_MAC2,
		CONFIG_ETH_SAM_GMAC_MAC3,
		CONFIG_ETH_SAM_GMAC_MAC4,
		CONFIG_ETH_SAM_GMAC_MAC5,
	},
#endif
	.queue_list = {{
			.que_idx = GMAC_QUE_0,
			.rx_desc_list = {
				.buf = rx_desc_que0,
				.len = ARRAY_SIZE(rx_desc_que0),
			},
			.tx_desc_list = {
				.buf = tx_desc_que0,
				.len = ARRAY_SIZE(tx_desc_que0),
			},
			.rx_frag_list = rx_frag_list_que0,
#if GMAC_MULTIPLE_TX_PACKETS == 1
			.tx_frag_list = {
				.buf = (u32_t *)tx_frag_list_que0,
				.len = ARRAY_SIZE(tx_frag_list_que0),
			},
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
			.tx_frames = {
				.buf = (u32_t *)tx_frame_list_que0,
				.len = ARRAY_SIZE(tx_frame_list_que0),
			},
#endif
#endif
		}, {
			.que_idx = GMAC_QUE_1,
			.rx_desc_list = {
				.buf = rx_desc_que1,
				.len = ARRAY_SIZE(rx_desc_que1),
			},
			.tx_desc_list = {
				.buf = tx_desc_que1,
				.len = ARRAY_SIZE(tx_desc_que1),
			},
#if GMAC_PRIORITY_QUEUE_NO >= 1
			.rx_frag_list = rx_frag_list_que1,
#if GMAC_MULTIPLE_TX_PACKETS == 1
			.tx_frag_list = {
				.buf = (u32_t *)tx_frag_list_que1,
				.len = ARRAY_SIZE(tx_frag_list_que1),
			},
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
			.tx_frames = {
				.buf = (u32_t *)tx_frame_list_que1,
				.len = ARRAY_SIZE(tx_frame_list_que1),
			}
#endif
#endif
#endif
		}, {
			.que_idx = GMAC_QUE_2,
			.rx_desc_list = {
				.buf = rx_desc_que2,
				.len = ARRAY_SIZE(rx_desc_que2),
			},
			.tx_desc_list = {
				.buf = tx_desc_que2,
				.len = ARRAY_SIZE(tx_desc_que2),
			},
#if GMAC_PRIORITY_QUEUE_NO == 2
			.rx_frag_list = rx_frag_list_que2,
#if GMAC_MULTIPLE_TX_PACKETS == 1
			.tx_frag_list = {
				.buf = (u32_t *)tx_frag_list_que2,
				.len = ARRAY_SIZE(tx_frag_list_que2),
			},
#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
			.tx_frames = {
				.buf = (u32_t *)tx_frame_list_que2,
				.len = ARRAY_SIZE(tx_frame_list_que2),
			}
#endif
#endif
#endif
		}
	},
};

ETH_NET_DEVICE_INIT(eth0_sam_gmac, CONFIG_ETH_SAM_GMAC_NAME, eth_initialize,
		    &eth0_data, &eth0_config, CONFIG_ETH_INIT_PRIORITY,
		    &eth_api, GMAC_MTU);

#if defined(CONFIG_PTP_CLOCK_SAM_GMAC)
struct ptp_context {
	struct device *eth_dev;
};

static struct ptp_context ptp_gmac_0_context;

static int ptp_clock_sam_gmac_set(struct device *dev,
				  struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->driver_data;
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(ptp_context->eth_dev);
	Gmac *gmac = cfg->regs;

	gmac->GMAC_TSH = tm->_sec.high & 0xffff;
	gmac->GMAC_TSL = tm->_sec.low & 0xffffffff;
	gmac->GMAC_TN = tm->nanosecond & 0xffffffff;

	return 0;
}

static int ptp_clock_sam_gmac_get(struct device *dev,
				  struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->driver_data;
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(ptp_context->eth_dev);
	Gmac *gmac = cfg->regs;

	tm->second = ((u64_t)(gmac->GMAC_TSH & 0xffff) << 32) | gmac->GMAC_TSL;
	tm->nanosecond = gmac->GMAC_TN;

	return 0;
}

static int ptp_clock_sam_gmac_adjust(struct device *dev, int increment)
{
	struct ptp_context *ptp_context = dev->driver_data;
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(ptp_context->eth_dev);
	Gmac *gmac = cfg->regs;
	GMAC_TA_Type gmac_ta;

	if ((increment <= -NSEC_PER_SEC) || (increment >= NSEC_PER_SEC)) {
		return -EINVAL;
	}

	if (increment < 0) {
		gmac_ta.bit.ADJ = 1;
		gmac_ta.bit.ITDT = -increment;
	} else {
		gmac_ta.bit.ADJ = 0;
		gmac_ta.bit.ITDT = increment;
	}

	gmac->GMAC_TA = gmac_ta.reg;

	return 0;
}

static int ptp_clock_sam_gmac_rate_adjust(struct device *dev, float ratio)
{
	return -ENOTSUP;
}

static const struct ptp_clock_driver_api ptp_api = {
	.set = ptp_clock_sam_gmac_set,
	.get = ptp_clock_sam_gmac_get,
	.adjust = ptp_clock_sam_gmac_adjust,
	.rate_adjust = ptp_clock_sam_gmac_rate_adjust,
};

static int ptp_gmac_init(struct device *port)
{
	struct device *eth_dev = DEVICE_GET(eth0_sam_gmac);
	struct eth_sam_dev_data *dev_data = eth_dev->driver_data;
	struct ptp_context *ptp_context = port->driver_data;

	dev_data->ptp_clock = port;
	ptp_context->eth_dev = eth_dev;

	return 0;
}

DEVICE_AND_API_INIT(gmac_ptp_clock_0, PTP_CLOCK_NAME, ptp_gmac_init,
		    &ptp_gmac_0_context, NULL, POST_KERNEL,
		    CONFIG_APPLICATION_INIT_PRIORITY, &ptp_api);

#endif /* CONFIG_PTP_CLOCK_SAM_GMAC */
