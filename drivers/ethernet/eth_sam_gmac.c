/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Ethernet MAC (GMAC) driver.
 *
 * This is a zero-copy networking implementation of GMAC driver. To prepare
 * for the incoming frames the driver will reserve a defined amount of data net
 * buffers during the initialization phase. This driver has to be initialized
 * after the higher layer networking stack is.
 *
 * Limitations:
 * - one shot PHY setup, no support for PHY disconnect/reconnect
 * - no statistics collection
 * - no support for devices with DCache enabled due to missing non-cacheable
 *   RAM regions in Zephyr.
 */

#define SYS_LOG_DOMAIN "dev/eth_sam"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ETHERNET_LEVEL
#include <logging/sys_log.h>

#include <kernel.h>
#include <device.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <errno.h>
#include <stdbool.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <soc.h>
#include "phy_sam_gmac.h"
#include "eth_sam_gmac_priv.h"

/*
 * Verify Kconfig configuration
 */

#if CONFIG_NET_NBUF_DATA_COUNT <= CONFIG_ETH_SAM_GMAC_NBUF_DATA_COUNT
#error CONFIG_NET_NBUF_DATA_COUNT has to be larger than \
	CONFIG_ETH_SAM_GMAC_NBUF_DATA_COUNT
#endif

#if CONFIG_NET_NBUF_DATA_SIZE * CONFIG_ETH_SAM_GMAC_NBUF_DATA_COUNT \
	< GMAC_FRAME_SIZE_MAX
#error CONFIG_NET_NBUF_DATA_SIZE * CONFIG_ETH_SAM_GMAC_NBUF_DATA_COUNT is not \
	large enough to hold a full frame
#endif

#if CONFIG_NET_NBUF_DATA_SIZE * CONFIG_NET_NBUF_DATA_COUNT \
	< 2 * GMAC_FRAME_SIZE_MAX
#error CONFIG_NET_NBUF_DATA_SIZE * CONFIG_NET_NBUF_DATA_COUNT is not large\
	enough to hold two full frames
#endif

#if CONFIG_NET_NBUF_DATA_SIZE & 0x3F
#pragma message "CONFIG_NET_NBUF_DATA_SIZE should be a multiple of 64 bytes " \
	"due to the granularity of RX DMA"
#endif

/* RX descriptors list */
static struct gmac_desc rx_desc_que0[MAIN_QUEUE_RX_DESC_COUNT]
	__aligned(GMAC_DESC_ALIGNMENT);
static struct gmac_desc rx_desc_que12[PRIORITY_QUEUE_DESC_COUNT]
	__aligned(GMAC_DESC_ALIGNMENT);
/* TX descriptors list */
static struct gmac_desc tx_desc_que0[MAIN_QUEUE_TX_DESC_COUNT]
	__aligned(GMAC_DESC_ALIGNMENT);
static struct gmac_desc tx_desc_que12[PRIORITY_QUEUE_DESC_COUNT]
	__aligned(GMAC_DESC_ALIGNMENT);

/* RX buffer accounting list */
static struct net_buf *rx_buf_list_que0[MAIN_QUEUE_RX_DESC_COUNT];
/* TX frames accounting list */
static struct net_buf *tx_frame_list_que0[CONFIG_NET_NBUF_TX_COUNT + 1];

#define MODULO_INC(val, max) {val = (++val < max) ? val : 0; }

/*
 * Reset ring buffer
 */
static void ring_buf_reset(struct ring_buf *rb)
{
	rb->head = 0;
	rb->tail = 0;
}

/*
 * Get one 32 bit item from the ring buffer
 */
static uint32_t ring_buf_get(struct ring_buf *rb)
{
	uint32_t val;

	__ASSERT(rb->tail != rb->head,
		 "retrieving data from empty ring buffer");

	val = rb->buf[rb->tail];
	MODULO_INC(rb->tail, rb->len);

	return val;
}

/*
 * Put one 32 bit item into the ring buffer
 */
static void ring_buf_put(struct ring_buf *rb, uint32_t val)
{
	rb->buf[rb->head] = val;
	MODULO_INC(rb->head, rb->len);

	__ASSERT(rb->tail != rb->head,
		 "ring buffer overflow");
}

/*
 * Free pre-reserved RX buffers
 */
static void free_rx_bufs(struct ring_buf *rx_nbuf_list)
{
	struct net_buf *buf;

	for (int i = 0; i < rx_nbuf_list->len; i++) {
		buf = (struct net_buf *)rx_nbuf_list->buf;
		if (buf) {
			net_buf_unref(buf);
		}
	}
}

/*
 * Set MAC Address for frame filtering logic
 */
static void mac_addr_set(Gmac *gmac, uint8_t index,
				 uint8_t mac_addr[6])
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
	struct ring_buf *rx_nbuf_list = &queue->rx_nbuf_list;
	struct net_buf *rx_buf;
	uint8_t *rx_buf_addr;

	__ASSERT_NO_MSG(rx_nbuf_list->buf);

	rx_desc_list->tail = 0;
	rx_nbuf_list->tail = 0;

	for (int i = 0; i < rx_desc_list->len; i++) {
		rx_buf = net_nbuf_get_reserve_data(0, K_NO_WAIT);
		if (rx_buf == NULL) {
			free_rx_bufs(rx_nbuf_list);
			SYS_LOG_ERR("Failed to reserve data net buffers");
			return -ENOBUFS;
		}

		rx_nbuf_list->buf[i] = (uint32_t)rx_buf;

		rx_buf_addr = rx_buf->data;
		__ASSERT(!((uint32_t)rx_buf_addr & ~GMAC_RXW0_ADDR),
			 "Misaligned RX buffer address");
		__ASSERT(rx_buf->size == CONFIG_NET_NBUF_DATA_SIZE,
			 "Incorrect length of RX data buffer");
		/* Give ownership to GMAC and remove the wrap bit */
		rx_desc_list->buf[i].w0 = (uint32_t)rx_buf_addr & GMAC_RXW0_ADDR;
		rx_desc_list->buf[i].w1 = 0;
	}

	/* Set the wrap bit on the last descriptor */
	rx_desc_list->buf[rx_desc_list->len - 1].w0 |= GMAC_RXW0_WRAP;

	return 0;
}

/*
 * Initialize TX descriptor list
 */
static void tx_descriptors_init(Gmac *gmac, struct gmac_queue *queue)
{
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;

	tx_desc_list->head = 0;
	tx_desc_list->tail = 0;

	for (int i = 0; i < tx_desc_list->len; i++) {
		tx_desc_list->buf[i].w0 = 0;
		tx_desc_list->buf[i].w1 = GMAC_TXW1_USED;
	}

	/* Set the wrap bit on the last descriptor */
	tx_desc_list->buf[tx_desc_list->len - 1].w1 |= GMAC_TXW1_WRAP;

	/* Reset TX frame list */
	ring_buf_reset(&queue->tx_frames);
}

/*
 * Process successfully sent packets
 */
static void tx_completed(Gmac *gmac, struct gmac_queue *queue)
{
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;
	struct gmac_desc *tx_desc;
	struct net_buf *buf;

	__ASSERT(tx_desc_list->buf[tx_desc_list->tail].w1 & GMAC_TXW1_USED,
		 "first buffer of a frame is not marked as own by GMAC");

	while (tx_desc_list->tail != tx_desc_list->head) {

		tx_desc = &tx_desc_list->buf[tx_desc_list->tail];
		MODULO_INC(tx_desc_list->tail, tx_desc_list->len);

		if (tx_desc->w1 & GMAC_TXW1_LASTBUFFER) {
			/* Release net buffer to the buffer pool */
			buf = UINT_TO_POINTER(ring_buf_get(&queue->tx_frames));
			net_buf_unref(buf);
			SYS_LOG_DBG("Dropping buf %p", buf);

			break;
		}
	}
}

/*
 * Reset TX queue when errors are detected
 */
static void tx_error_handler(Gmac *gmac, struct gmac_queue *queue)
{
	queue->err_tx_flushed_count++;

	/* Stop transmission, clean transmit pipeline and control registers */
	gmac->GMAC_NCR &= ~GMAC_NCR_TXEN;

	tx_descriptors_init(gmac, queue);

	/* Restart transmission */
	gmac->GMAC_NCR |=  GMAC_NCR_TXEN;
}

/*
 * Clean RX queue, any received data stored still in the buffers is abandoned.
 */
static void rx_error_handler(Gmac *gmac, struct gmac_queue *queue)
{
	queue->err_rx_flushed_count++;

	/* Stop reception */
	gmac->GMAC_NCR &= ~GMAC_NCR_RXEN;

	queue->rx_desc_list.tail = 0;
	queue->rx_nbuf_list.tail = 0;

	for (int i = 0; i < queue->rx_desc_list.len; i++) {
		queue->rx_desc_list.buf[i].w1 = 0;
		queue->rx_desc_list.buf[i].w0 &= ~GMAC_RXW0_OWNERSHIP;
	}

	/* Set Receive Buffer Queue Pointer Register */
	gmac->GMAC_RBQB = (uint32_t)queue->rx_desc_list.buf;

	/* Restart reception */
	gmac->GMAC_NCR |=  GMAC_NCR_RXEN;
}

/*
 * Set MCK to MDC clock divisor.
 *
 * According to 802.3 MDC should be less then 2.5 MHz.
 */
static int get_mck_clock_divisor(uint32_t mck)
{
	uint32_t mck_divisor;

	if (mck <= 20000000) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_8;
	} else if (mck <= 40000000) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_16;
	} else if (mck <= 80000000) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_32;
	} else if (mck <= 120000000) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_48;
	} else if (mck <= 160000000) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_64;
	} else if (mck <= 240000000) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_96;
	} else {
		SYS_LOG_ERR("No valid MDC clock");
		mck_divisor = -ENOTSUP;
	}

	return mck_divisor;
}

static int gmac_init(Gmac *gmac, uint32_t gmac_ncfgr_val)
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

	return 0;
}

static void link_configure(Gmac *gmac, uint32_t flags)
{
	uint32_t val;

	gmac->GMAC_NCR &= ~(GMAC_NCR_RXEN | GMAC_NCR_TXEN);

	val = gmac->GMAC_NCFGR;

	val &= ~(GMAC_NCFGR_FD | GMAC_NCFGR_SPD);
	val |= flags & (GMAC_NCFGR_FD | GMAC_NCFGR_SPD);

	gmac->GMAC_NCFGR = val;

	gmac->GMAC_UR = 0;  /* Select RMII mode */
	gmac->GMAC_NCR |= (GMAC_NCR_RXEN | GMAC_NCR_TXEN);
}

static int queue_init(Gmac *gmac, struct gmac_queue *queue)
{
	int result;

	__ASSERT_NO_MSG(queue->rx_desc_list.len > 0);
	__ASSERT_NO_MSG(queue->tx_desc_list.len > 0);
	__ASSERT(!((uint32_t)queue->rx_desc_list.buf & ~GMAC_RBQB_ADDR_Msk),
		 "RX descriptors have to be word aligned");
	__ASSERT(!((uint32_t)queue->tx_desc_list.buf & ~GMAC_TBQB_ADDR_Msk),
		 "TX descriptors have to be word aligned");

	/* Setup descriptor lists */
	result = rx_descriptors_init(gmac, queue);
	if (result < 0) {
		return result;
	}

	tx_descriptors_init(gmac, queue);

	/* Set Receive Buffer Queue Pointer Register */
	gmac->GMAC_RBQB = (uint32_t)queue->rx_desc_list.buf;
	/* Set Transmit Buffer Queue Pointer Register */
	gmac->GMAC_TBQB = (uint32_t)queue->tx_desc_list.buf;

	/* Configure GMAC DMA transfer */
	gmac->GMAC_DCFGR =
		  /* Receive Buffer Size (defined in multiples of 64 bytes) */
		  GMAC_DCFGR_DRBS(CONFIG_NET_NBUF_DATA_SIZE >> 6)
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

	queue->err_rx_frames_dropped = 0;
	queue->err_rx_flushed_count = 0;
	queue->err_tx_flushed_count = 0;

	SYS_LOG_INF("Queue %d activated", queue->que_idx);

	return 0;
}

static int priority_queue_init_as_idle(Gmac *gmac, struct gmac_queue *queue)
{
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;

	__ASSERT(!((uint32_t)rx_desc_list->buf & ~GMAC_RBQB_ADDR_Msk),
		 "RX descriptors have to be word aligned");
	__ASSERT(!((uint32_t)tx_desc_list->buf & ~GMAC_TBQB_ADDR_Msk),
		 "TX descriptors have to be word aligned");
	__ASSERT((rx_desc_list->len == 1) && (tx_desc_list->len == 1),
		 "Priority queues are currently not supported, descriptor "
		 "list has to have a single entry");

	/* Setup RX descriptor lists */
	/* Take ownership from GMAC and set the wrap bit */
	rx_desc_list->buf[0].w0 = GMAC_RXW0_WRAP;
	rx_desc_list->buf[0].w1 = 0;
	/* Setup TX descriptor lists */
	tx_desc_list->buf[0].w0 = 0;
	/* Take ownership from GMAC and set the wrap bit */
	tx_desc_list->buf[0].w1 = GMAC_TXW1_USED | GMAC_TXW1_WRAP;

	/* Set Receive Buffer Queue Pointer Register */
	gmac->GMAC_RBQBAPQ[queue->que_idx - 1] = (uint32_t)rx_desc_list->buf;
	/* Set Transmit Buffer Queue Pointer Register */
	gmac->GMAC_TBQBAPQ[queue->que_idx - 1] = (uint32_t)tx_desc_list->buf;

	return 0;
}

static struct net_buf *frame_get(struct gmac_queue *queue)
{
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct gmac_desc *rx_desc;
	struct ring_buf *rx_nbuf_list = &queue->rx_nbuf_list;
	struct net_buf *rx_frame;
	bool frame_is_complete;
	struct net_buf *prev_frag;
	struct net_buf *frag;
	struct net_buf *new_frag;
	uint8_t *frag_data;
	uint32_t frag_len;
	uint32_t frame_len = 0;
	uint16_t tail;

	/* Check if there exists a complete frame in RX descriptor list */
	tail = rx_desc_list->tail;
	rx_desc = &rx_desc_list->buf[tail];
	frame_is_complete = false;
	while ((rx_desc->w0 & GMAC_RXW0_OWNERSHIP) && !frame_is_complete) {
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

	rx_frame = net_nbuf_get_reserve_rx(0, K_NO_WAIT);

	/* Process a frame */
	prev_frag = rx_frame;
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
	while ((rx_desc->w0 & GMAC_RXW0_OWNERSHIP) && !frame_is_complete) {
		frag = (struct net_buf *)rx_nbuf_list->buf[tail];
		frag_data = (uint8_t *)(rx_desc->w0 & GMAC_RXW0_ADDR);
		__ASSERT(frag->data == frag_data,
			 "RX descriptor and buffer list desynchronized");
		frame_is_complete = (bool)(rx_desc->w1 & GMAC_RXW1_EOF);
		if (frame_is_complete) {
			frag_len = (rx_desc->w1 & GMAC_TXW1_LEN) - frame_len;
		} else {
			frag_len = CONFIG_NET_NBUF_DATA_SIZE;
		}

		frame_len += frag_len;

		/* Link frame fragments only if RX net buffer is valid */
		if (rx_frame != NULL) {
			/* Assure cache coherency after DMA write operation */
			DCACHE_INVALIDATE(frag_data, frag_len);

			/* Get a new data net buffer from the buffer pool */
			new_frag = net_nbuf_get_reserve_data(0, K_NO_WAIT);
			if (new_frag == NULL) {
				queue->err_rx_frames_dropped++;
				net_buf_unref(rx_frame);
				rx_frame = NULL;
			} else {
				net_buf_add(frag, frag_len);
				net_buf_frag_insert(prev_frag, frag);
				prev_frag = frag;
				frag = new_frag;
				rx_nbuf_list->buf[tail] = (uint32_t)frag;
			}
		}

		/* Update buffer descriptor status word */
		rx_desc->w1 = 0;
		/* Guarantee that status word is written before the address
		 * word to avoid race condition.
		 */
		__DMB();  /* data memory barrier */
		/* Update buffer descriptor address word */
		rx_desc->w0 =
			  ((uint32_t)frag->data & GMAC_RXW0_ADDR)
			| (tail == rx_desc_list->len-1 ? GMAC_RXW0_WRAP : 0);

		MODULO_INC(tail, rx_desc_list->len);
		rx_desc = &rx_desc_list->buf[tail];
	}

	rx_desc_list->tail = tail;
	SYS_LOG_DBG("Frame complete: rx=%p, tail=%d", rx_frame, tail);
	__ASSERT_NO_MSG(frame_is_complete);

	return rx_frame;
}

static void eth_rx(struct gmac_queue *queue)
{
	struct eth_sam_dev_data *dev_data =
		CONTAINER_OF(queue, struct eth_sam_dev_data, queue_list);
	struct net_buf *rx_frame;

	/* More than one frame could have been received by GMAC, get all
	 * complete frames stored in the GMAC RX descriptor list.
	 */
	rx_frame = frame_get(queue);
	while (rx_frame) {
		SYS_LOG_DBG("ETH rx");

		net_recv_data(dev_data->iface, rx_frame);

		rx_frame = frame_get(queue);
	}
}

static int eth_tx(struct net_if *iface, struct net_buf *buf)
{
	struct device *const dev = net_if_get_device(iface);
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);
	Gmac *gmac = cfg->regs;
	struct gmac_queue *queue = &dev_data->queue_list[0];
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;
	struct gmac_desc *tx_desc;
	struct net_buf *frag;
	uint8_t *frag_data;
	uint16_t frag_len;

	__ASSERT(buf, "buf pointer is NULL");
	__ASSERT(buf->frags, "Frame data missing");

	SYS_LOG_DBG("ETH tx");

	/* First fragment is special - it contains link layer (Ethernet
	 * in our case) header.
	 */
	frag_data = net_nbuf_ll(buf);
	frag_len = net_nbuf_ll_reserve(buf) + buf->frags->len;
	/* TODO: make following workaround compiler version dependent when
	 * the bug is fixed.
	 */
#if defined(__GNUC__)
	/* NOTE: the following cast is a workaround for arm-none-eabi-gcc bug
	 * observed with the 5.4.1 20160919 release. When compiling with
	 * optimization enabled '-Os' the while(frag) conditional loop is
	 * optimized to while(1) breaking the logic.
	 */
	frag = ((volatile struct net_buf *)buf)->frags;
#else
	frag = buf->frags;
#endif

	while (frag) {
		/* Assure cache coherency before DMA read operation */
		DCACHE_CLEAN(frag_data, frag_len);

		tx_desc = &tx_desc_list->buf[tx_desc_list->head];

		/* Update buffer descriptor address word */
		tx_desc->w0 = (uint32_t)frag_data;
		/* Guarantee that address word is written before the status
		 * word to avoid race condition.
		 */
		__DMB();  /* data memory barrier */
		/* Update buffer descriptor status word (clear used bit) */
		tx_desc->w1 =
			  (frag_len & GMAC_TXW1_LEN)
			| (!frag->frags ? GMAC_TXW1_LASTBUFFER : 0)
			| (tx_desc_list->head == tx_desc_list->len - 1
			   ? GMAC_TXW1_WRAP : 0);

		/* Update descriptor position */
		MODULO_INC(tx_desc_list->head, tx_desc_list->len);

		__ASSERT(tx_desc_list->head != tx_desc_list->tail,
			 "tx_desc_list overflow");

		/* Continue with the rest of fragments (only data) */
		frag = frag->frags;
		frag_data = frag->data;
		frag_len = frag->len;
	}

	/* Ensure the descriptor following the last one is marked as used */
	tx_desc = &tx_desc_list->buf[tx_desc_list->head];
	tx_desc->w1 |= GMAC_TXW1_USED;

	/* Account for a sent frame */
	ring_buf_put(&queue->tx_frames, POINTER_TO_UINT(buf));

	/* Start transmission */
	gmac->GMAC_NCR |= GMAC_NCR_TSTART;

	return 0;
}

static void queue0_isr(void *arg)
{
	struct device *const dev = (struct device *const)arg;
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);
	Gmac *gmac = cfg->regs;
	struct gmac_queue *queue = &dev_data->queue_list[0];
	uint32_t isr;

	/* Interrupt Status Register is cleared on read */
	isr = gmac->GMAC_ISR;
	SYS_LOG_DBG("GMAC_ISR=0x%08x", isr);

	/* RX packet */
	if (isr & GMAC_INT_RX_ERR_BITS) {
		rx_error_handler(gmac, queue);
	} else if (isr & GMAC_ISR_RCOMP) {
		SYS_LOG_DBG("rx.w1=0x%08x, tail=%d",
			    queue->rx_desc_list.buf[queue->rx_desc_list.tail].w1,
			    queue->rx_desc_list.tail);
		eth_rx(queue);
	}

	/* TX packet */
	if (isr & GMAC_INT_TX_ERR_BITS) {
		tx_error_handler(gmac, queue);
	} else if (isr & GMAC_ISR_TCOMP) {
		SYS_LOG_DBG("tx.w1=0x%08x, tail=%d",
			    queue->tx_desc_list.buf[queue->tx_desc_list.tail].w1,
			    queue->tx_desc_list.tail);
		tx_completed(gmac, queue);
	}

	if (isr & GMAC_IER_HRESP) {
		SYS_LOG_DBG("HRESP");
	}
}

static int eth_initialize(struct device *dev)
{
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);
	const struct eth_sam_dev_cfg *const cfg = DEV_CFG(dev);
	uint32_t link_status;
	uint32_t gmac_ncfgr_val;
	int result;

	cfg->config_func();

	/* Enable GMAC module's clock */
	soc_pmc_peripheral_enable(cfg->periph_id);

	/* Initialize GMAC driver, maximum frame length is 1518 bytes */
	gmac_ncfgr_val =
		  GMAC_NCFGR_MTIHEN  /* Multicast Hash Enable */
		| GMAC_NCFGR_LFERD   /* Length Field Error Frame Discard */
		| GMAC_NCFGR_RFCS    /* Remove Frame Check Sequence */
		| GMAC_NCFGR_RXCOEN; /* Receive Checksum Offload Enable */
	result = gmac_init(cfg->regs, gmac_ncfgr_val);
	if (result < 0) {
		SYS_LOG_ERR("Unable to initialize ETH driver");
		return result;
	}

	/* Initialize GMAC queues */
	/* Note: Queues 1 and 2 are not used, configured to stay idle */
	priority_queue_init_as_idle(cfg->regs, &dev_data->queue_list[2]);
	priority_queue_init_as_idle(cfg->regs, &dev_data->queue_list[1]);
	result = queue_init(cfg->regs, &dev_data->queue_list[0]);
	if (result < 0) {
		SYS_LOG_ERR("Unable to initialize ETH queue");
		return result;
	}
	/* Set MAC Address for frame filtering logic */
	mac_addr_set(cfg->regs, 0, dev_data->mac_addr);

	/* Connect pins to the peripheral */
	soc_gpio_list_configure(cfg->pin_list, cfg->pin_list_size);

	/* PHY initialize */
	result = phy_sam_gmac_init(&cfg->phy);
	if (result < 0) {
		SYS_LOG_ERR("ETH PHY Initialization Error");
		return result;
	}
	/* PHY auto-negotiate link parameters */
	result = phy_sam_gmac_auto_negotiate(&cfg->phy, &link_status);
	if (result < 0) {
		SYS_LOG_ERR("ETH PHY auto-negotiate sequence failed");
		return result;
	}

	/* Set up link parameters */
	link_configure(cfg->regs, link_status);

	return 0;
}

static void eth0_iface_init(struct net_if *iface)
{
	struct device *const dev = net_if_get_device(iface);
	struct eth_sam_dev_data *const dev_data = DEV_DATA(dev);

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	dev_data->iface = iface;
}

static struct net_if_api eth0_api = {
	.init	= eth0_iface_init,
	.send	= eth_tx,
};

static struct device DEVICE_NAME_GET(eth0_sam_gmac);

static void eth0_irq_config(void)
{
	IRQ_CONNECT(GMAC_IRQn, CONFIG_ETH_SAM_GMAC_IRQ_PRI, queue0_isr,
		    DEVICE_GET(eth0_sam_gmac), 0);
	irq_enable(GMAC_IRQn);
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
	.mac_addr = {
		CONFIG_ETH_SAM_GMAC_MAC0,
		CONFIG_ETH_SAM_GMAC_MAC1,
		CONFIG_ETH_SAM_GMAC_MAC2,
		CONFIG_ETH_SAM_GMAC_MAC3,
		CONFIG_ETH_SAM_GMAC_MAC4,
		CONFIG_ETH_SAM_GMAC_MAC5,
	},
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
			.rx_nbuf_list = {
				.buf = (uint32_t *)rx_buf_list_que0,
				.len = ARRAY_SIZE(rx_buf_list_que0),
			},
			.tx_frames = {
				.buf = (uint32_t *)tx_frame_list_que0,
				.len = ARRAY_SIZE(tx_frame_list_que0),
			},
		}, {
			.que_idx = GMAC_QUE_1,
			.rx_desc_list = {
				.buf = rx_desc_que12,
				.len = ARRAY_SIZE(rx_desc_que12),
			},
			.tx_desc_list = {
				.buf = tx_desc_que12,
				.len = ARRAY_SIZE(tx_desc_que12),
			},
		}, {
			.que_idx = GMAC_QUE_2,
			.rx_desc_list = {
				.buf = rx_desc_que12,
				.len = ARRAY_SIZE(rx_desc_que12),
			},
			.tx_desc_list = {
				.buf = tx_desc_que12,
				.len = ARRAY_SIZE(tx_desc_que12),
			},
		}
	},
};

NET_DEVICE_INIT(eth0_sam_gmac, CONFIG_ETH_SAM_GMAC_NAME, eth_initialize,
		&eth0_data, &eth0_config, CONFIG_ETH_INIT_PRIORITY, &eth0_api,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2), GMAC_MTU);
