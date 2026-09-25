/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Cadence GEM (Gigabit Ethernet MAC), the core shared by all
 * SoC integrations. Initialisation and the descriptor ring protocol follow
 * the Linux macb driver; the zero-copy fragment handling follows the Zephyr
 * dwc_mac driver.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_core, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/barrier.h>
#include <ethernet/eth_stats.h>

#include "eth_cdns_macb_priv.h"

/* Size of the pre-allocated packet fragments, the DMA receive buffer size */
#define RX_FRAG_SIZE CONFIG_NET_BUF_DATA_SIZE

/*
 * Grace period to wait for TX descriptor availability. Worst case estimate is
 * 1514 * 8 bits at 10 Mbit/s for a queued packet to be sent and freed, so 1 ms
 * is more than enough. Beyond that the packet is dropped.
 */
#define TX_AVAIL_WAIT K_MSEC(1)

/* Time to wait for the transmitter to halt before the TX ring is reset */
#define TX_HALT_TIMEOUT_US 14000

/* Interrupt status is re-read this many times at most per interrupt */
#define ISR_LOOP_MAX 8

/* Descriptor index iterator */
#define INC_WRAP(idx, size) ((idx) = ((idx) + 1) % (size))

/*
 * Multicast is hash filtered only when the stack tracks the L2 multicast
 * addresses, otherwise every multicast frame is accepted.
 */
#if defined(CONFIG_ETH_CDNS_MACB_MULTICAST_FILTER) && defined(NET_ETH_MCAST_FILTER_SUPPORTED)
#define CDNS_MACB_MCAST_FILTER 1
#else
#define CDNS_MACB_MCAST_FILTER 0
#endif

static inline uint32_t phys_hi32(uintptr_t addr)
{
	/* trickery to avoid compiler warnings on 32-bit build targets */
	if (sizeof(addr) > 4) {
		uint64_t hi = addr;

		return (uint32_t)(hi >> 32);
	}

	return 0;
}

static inline uint32_t phys_lo32(uintptr_t addr)
{
	return (uint32_t)addr;
}

/* Physical address of a descriptor inside the ring object */
static inline uintptr_t desc_phys(const struct device *dev, const struct cdns_macb_dma_desc *d)
{
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	return p->rings_phys + (POINTER_TO_UINT(d) - POINTER_TO_UINT(cfg->rings));
}

/*
 * Write the buffer address of a descriptor. The upper word is written first
 * so it is visible to the hardware when the low word, which for RX also holds
 * the used bit, hands the descriptor over.
 */
static inline void desc_set_addr(struct cdns_macb_dma_desc *d, uintptr_t addr, uint32_t flags)
{
#ifdef CONFIG_ETH_CDNS_MACB_DMA_64BIT
	d->addrh = phys_hi32(addr);
	barrier_dmem_fence_full();
#else
	__ASSERT(phys_hi32(addr) == 0U, "buffer above 4 GiB without 64-bit descriptors");
#endif
	d->addr = phys_lo32(addr) | flags;
}

/* for debug logs */
static __maybe_unused unsigned int net_pkt_get_nbfrags(struct net_pkt *pkt)
{
	unsigned int nbfrags = 0U;

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		nbfrags++;
	}

	return nbfrags;
}

static inline bool cdns_macb_rx_csum_enabled(const struct cdns_macb_priv *p)
{
	return IS_ENABLED(CONFIG_ETH_CDNS_MACB_RX_HW_CHECKSUM_EN) && p->pkt_buf_mode;
}

static inline bool cdns_macb_tx_csum_enabled(const struct cdns_macb_priv *p)
{
	return IS_ENABLED(CONFIG_ETH_CDNS_MACB_TX_HW_CHECKSUM_EN) && p->pkt_buf_mode;
}

/*
 * Interrupt registers of a queue
 */

static inline void cdns_macb_queue_idr(const struct device *dev, unsigned int q, uint32_t val)
{
	CDNS_MACB_REG_WRITE(q == 0U ? MACB_IDR : GEM_IDR_Q(q), val);
}

static inline uint32_t cdns_macb_queue_isr_read(const struct device *dev, unsigned int q)
{
	return CDNS_MACB_REG_READ(q == 0U ? MACB_ISR : GEM_ISR_Q(q));
}

static inline void cdns_macb_queue_isr_clear(const struct device *dev, unsigned int q, uint32_t val)
{
	struct cdns_macb_priv *p = dev->data;

	if (p->isr_clear_on_write) {
		CDNS_MACB_REG_WRITE(q == 0U ? MACB_ISR : GEM_ISR_Q(q), val);
	}
}

/*
 * Statistics
 */

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
#define STATS_INC(p, field) ((p)->stats.field++)
#else
#define STATS_INC(p, field) do {} while (false)
#endif

static struct net_stats_eth *cdns_macb_get_stats(const struct device *dev, struct net_if *iface)
{
	struct cdns_macb_priv *p = dev->data;

	ARG_UNUSED(iface);

	return COND_CODE_1(CONFIG_NET_STATISTICS_ETHERNET, (&p->stats), (NULL));
}

/*
 * MAC address and filtering
 */

static void cdns_macb_set_mac_addr(const struct device *dev, const uint8_t *addr)
{
	uint32_t bottom, top;

	bottom = sys_get_le32(&addr[0]);
	top = sys_get_le16(&addr[4]);

	CDNS_MACB_REG_WRITE(GEM_SAB(0), bottom);
	CDNS_MACB_REG_WRITE(GEM_SAT(0), top);
}

/* Fetch the address left in the specific address 1 registers by the firmware */
static void cdns_macb_get_hw_mac_addr(const struct device *dev, uint8_t *addr)
{
	uint32_t bottom, top;

	bottom = CDNS_MACB_REG_READ(GEM_SAB(0));
	top = CDNS_MACB_REG_READ(GEM_SAT(0));

	sys_put_le32(bottom, &addr[0]);
	sys_put_le16((uint16_t)top, &addr[4]);
}

/* Update NCFGR under the driver lock */
static void cdns_macb_ncfgr_update(const struct device *dev, uint32_t clear, uint32_t set)
{
	struct cdns_macb_priv *p = dev->data;
	uint32_t ncfgr;

	K_SPINLOCK(&p->lock) {
		ncfgr = CDNS_MACB_REG_READ(MACB_NCFGR);
		ncfgr &= ~clear;
		ncfgr |= set;
		CDNS_MACB_REG_WRITE(MACB_NCFGR, ncfgr);
	}
}

static void cdns_macb_set_hash(const struct device *dev, uint32_t bottom, uint32_t top)
{
	CDNS_MACB_REG_WRITE(GEM_HRB, bottom);
	CDNS_MACB_REG_WRITE(GEM_HRT, top);

	if ((bottom | top) != 0U) {
		cdns_macb_ncfgr_update(dev, 0U, MACB_NCFGR_MTI);
	} else {
		cdns_macb_ncfgr_update(dev, MACB_NCFGR_MTI, 0U);
	}
}

#if CDNS_MACB_MCAST_FILTER
/*
 * The 6-bit hash index is the exclusive or of every sixth bit of the
 * destination address, da[0] being the least significant bit of the first
 * byte on the wire.
 */
static unsigned int cdns_macb_hash_index(const uint8_t *addr)
{
	unsigned int index = 0U;

	for (unsigned int j = 0U; j < 6U; j++) {
		unsigned int bitval = 0U;

		for (unsigned int i = 0U; i < 8U; i++) {
			unsigned int bitnr = (i * 6U) + j;

			bitval ^= (addr[bitnr / 8U] >> (bitnr % 8U)) & 1U;
		}

		index |= bitval << j;
	}

	return index;
}

static void cdns_macb_mcast_hash_cb(struct net_if *iface, const struct net_eth_mcast_addr *mcast,
				    void *user_data)
{
	uint32_t *hash = user_data;
	unsigned int index = cdns_macb_hash_index(mcast->addr.addr);

	ARG_UNUSED(iface);

	hash[index / 32U] |= BIT(index % 32U);
}

static void cdns_macb_setup_multicast_filter(const struct device *dev)
{
	struct cdns_macb_priv *p = dev->data;
	uint32_t hash[2] = {0U, 0U};

	net_eth_mcast_addr_foreach(p->iface, cdns_macb_mcast_hash_cb, hash);

	cdns_macb_set_hash(dev, hash[0], hash[1]);
}
#endif /* CDNS_MACB_MCAST_FILTER */

/*
 * TX path
 */

static int cdns_macb_send(const struct device *dev, struct net_pkt *pkt)
{
	struct cdns_macb_priv *p = dev->data;
	struct cdns_macb_dma_desc *tx = ((const struct cdns_macb_config *)dev->config)->rings->tx;
	unsigned int nfrags = 0U;
	unsigned int remaining;
	unsigned int idx, stop_idx;
	uint32_t epoch, first_ctrl = 0U;
	k_spinlock_key_t key;

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		if (frag->len > 0U) {
			nfrags++;
		}
	}

	LOG_DBG("pkt len/frags=%zu/%u", net_pkt_get_len(pkt), nfrags);

	if (nfrags == 0U) {
		return -EINVAL;
	}

	/* One slot always stays free to hold the end of queue marker */
	if (nfrags > (CDNS_MACB_NB_TX_DESCS - 1U)) {
		LOG_ERR("packet needs %u descriptors, ring has %u", nfrags,
			CDNS_MACB_NB_TX_DESCS - 1U);
		return -EMSGSIZE;
	}

	epoch = p->tx_epoch;

	for (unsigned int taken = 0U; taken < nfrags; taken++) {
		if (k_sem_take(&p->free_tx_descs, TX_AVAIL_WAIT) != 0) {
			LOG_DBG("no more free tx descriptors");
			for (unsigned int i = 0U; i < taken; i++) {
				k_sem_give(&p->free_tx_descs);
			}
			return -ENOMEM;
		}
	}

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		if (frag->len > 0U) {
			sys_cache_data_flush_range(frag->data, frag->len);
		}
	}

	key = k_spin_lock(&p->lock);

	/*
	 * A TX ring reset re-credits the descriptor semaphore, so a sender that
	 * raced with it must not give its descriptors back.
	 */
	if ((p->tx_epoch != epoch) || p->tx_resetting) {
		k_spin_unlock(&p->lock, key);
		return -EIO;
	}

	idx = p->tx_head;
	stop_idx = (idx + nfrags) % CDNS_MACB_NB_TX_DESCS;

	/*
	 * Mark the descriptor after the frame as used so the hardware stops
	 * there, before any descriptor of the frame is handed over.
	 */
	tx[stop_idx].ctrl = MACB_TX_USED |
			    ((stop_idx == (CDNS_MACB_NB_TX_DESCS - 1U)) ? MACB_TX_WRAP : 0U);
	barrier_dmem_fence_full();

	/*
	 * Fill the descriptors. The first one keeps its used bit until every
	 * other descriptor of the frame is complete, so the hardware cannot
	 * start on a partially written frame.
	 */
	remaining = nfrags;
	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		struct cdns_macb_dma_desc *d;
		uint32_t ctrl;

		if (frag->len == 0U) {
			continue;
		}

		d = &tx[idx];
		remaining--;

		ctrl = FIELD_PREP(GEM_TX_FRMLEN, frag->len);
		if (remaining == 0U) {
			ctrl |= MACB_TX_LAST;
			p->tx_pkts[idx] = pkt;
		}
		if (idx == (CDNS_MACB_NB_TX_DESCS - 1U)) {
			ctrl |= MACB_TX_WRAP;
		}

		LOG_DBG("d[%u]: frag %p len %u", idx, (void *)frag->data, frag->len);

		desc_set_addr(d, POINTER_TO_UINT(frag->data), 0U);
		barrier_dmem_fence_full();

		if (idx == p->tx_head) {
			first_ctrl = ctrl;
		} else {
			d->ctrl = ctrl;
		}

		INC_WRAP(idx, CDNS_MACB_NB_TX_DESCS);
	}

	net_pkt_ref(pkt);

	/* Everything is in place, release the frame to the hardware */
	barrier_dmem_fence_full();
	tx[p->tx_head].ctrl = first_ctrl;
	p->tx_head = stop_idx;
	barrier_dmem_fence_full();

	CDNS_MACB_REG_WRITE(MACB_NCR, CDNS_MACB_REG_READ(MACB_NCR) | MACB_NCR_TSTART);

	k_spin_unlock(&p->lock, key);

	return 0;
}

/* Release the descriptors of the transmitted frames, called from the ISR */
static void cdns_macb_tx_release(const struct device *dev)
{
	struct cdns_macb_priv *p = dev->data;
	struct cdns_macb_dma_desc *tx = ((const struct cdns_macb_config *)dev->config)->rings->tx;
	unsigned int released = 0U;
	unsigned int idx;

	K_SPINLOCK(&p->lock) {
		idx = p->tx_tail;

		while (idx != p->tx_head) {
			struct net_pkt *pkt;
			uint32_t ctrl;

			barrier_dmem_fence_full();
			ctrl = tx[idx].ctrl;

			/*
			 * The hardware sets the used bit only in the first
			 * descriptor of a frame once the whole frame is sent.
			 */
			if ((ctrl & MACB_TX_USED) == 0U) {
				break;
			}

			if ((ctrl & MACB_TX_ERR_MASK) != 0U) {
				LOG_ERR("tx error (ctrl = 0x%08x)", ctrl);
				STATS_INC(p, errors.tx);
				if ((ctrl & MACB_TX_UNDERRUN) != 0U) {
					STATS_INC(p, error_details.tx_fifo_errors);
				}
				if ((ctrl & MACB_TX_ERROR) != 0U) {
					STATS_INC(p, error_details.tx_aborted_errors);
				}
				if ((ctrl & MACB_TX_LATE_COLL) != 0U) {
					STATS_INC(p, error_details.tx_window_errors);
				}
			}

			/* The packet is stored with the last descriptor of the frame */
			do {
				pkt = p->tx_pkts[idx];
				p->tx_pkts[idx] = NULL;
				INC_WRAP(idx, CDNS_MACB_NB_TX_DESCS);
				released++;
			} while (pkt == NULL);

			LOG_DBG("pkt len/frags=%zu/%u", net_pkt_get_len(pkt),
				net_pkt_get_nbfrags(pkt));
			net_pkt_unref(pkt);
		}

		p->tx_tail = idx;
	}

	for (unsigned int i = 0U; i < released; i++) {
		k_sem_give(&p->free_tx_descs);
	}
}

/*
 * The transmitter read the end of queue marker. If frames were queued since,
 * it needs a new start.
 */
static void cdns_macb_tx_restart(const struct device *dev)
{
	struct cdns_macb_priv *p = dev->data;
	struct cdns_macb_dma_desc *tx = ((const struct cdns_macb_config *)dev->config)->rings->tx;

	K_SPINLOCK(&p->lock) {
		if (p->tx_tail == p->tx_head) {
			K_SPINLOCK_BREAK;
		}

		if (CDNS_MACB_REG_READ(MACB_TBQP) == phys_lo32(desc_phys(dev, &tx[p->tx_head]))) {
			K_SPINLOCK_BREAK;
		}

		CDNS_MACB_REG_WRITE(MACB_NCR, CDNS_MACB_REG_READ(MACB_NCR) | MACB_NCR_TSTART);
	}
}

static void cdns_macb_init_tx_ring(const struct device *dev)
{
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;
	struct cdns_macb_dma_desc *tx = cfg->rings->tx;

	for (unsigned int i = 0U; i < CDNS_MACB_NB_TX_DESCS; i++) {
		desc_set_addr(&tx[i], 0U, 0U);
		tx[i].ctrl = MACB_TX_USED |
			     ((i == (CDNS_MACB_NB_TX_DESCS - 1U)) ? MACB_TX_WRAP : 0U);
		p->tx_pkts[i] = NULL;
	}

	p->tx_head = 0U;
	p->tx_tail = 0U;
	barrier_dmem_fence_full();
}

/*
 * A transmit error stops the transmitter. Halt it, drop everything that was
 * queued, and restart from an empty ring, the way the Linux driver does.
 */
static void cdns_macb_tx_error_work(struct k_work *work)
{
	struct cdns_macb_priv *p = CONTAINER_OF(work, struct cdns_macb_priv, tx_error_work);
	const struct device *dev = net_if_get_device(p->iface);
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_dma_desc *tx = cfg->rings->tx;
	bool halt_timeout = false;
	k_spinlock_key_t key;

	LOG_ERR("tx error, resetting the tx ring (TSR = 0x%08x)", CDNS_MACB_REG_READ(MACB_TSR));

	/* Stop new frames from being queued while the ring is reset */
	key = k_spin_lock(&p->lock);
	p->tx_resetting = true;
	CDNS_MACB_REG_WRITE(MACB_NCR, CDNS_MACB_REG_READ(MACB_NCR) | MACB_NCR_THALT);
	k_spin_unlock(&p->lock, key);

	/* The transmitter finishes the current frame before it halts */
	if (!WAIT_FOR((CDNS_MACB_REG_READ(MACB_TSR) & MACB_TSR_TGO) == 0U, TX_HALT_TIMEOUT_US,
		      k_busy_wait(250))) {
		LOG_ERR("halting the transmitter timed out");
		halt_timeout = true;
	}

	key = k_spin_lock(&p->lock);

	if (halt_timeout) {
		CDNS_MACB_REG_WRITE(MACB_NCR, CDNS_MACB_REG_READ(MACB_NCR) & ~MACB_NCR_TE);
	}

	/* Drop every queued frame, the ones that were sent are counted as well */
	for (unsigned int idx = p->tx_tail; idx != p->tx_head;
	     INC_WRAP(idx, CDNS_MACB_NB_TX_DESCS)) {
		struct net_pkt *pkt = p->tx_pkts[idx];

		if (pkt != NULL) {
			p->tx_pkts[idx] = NULL;
			net_pkt_unref(pkt);
			STATS_INC(p, errors.tx);
		}
	}

	cdns_macb_init_tx_ring(dev);

	/* The queue base may only be written while the transmitter is halted */
	CDNS_MACB_REG_WRITE(MACB_TBQP, phys_lo32(desc_phys(dev, tx)));
	CDNS_MACB_REG_WRITE(MACB_TSR, UINT32_MAX);

	if (halt_timeout) {
		CDNS_MACB_REG_WRITE(MACB_NCR, CDNS_MACB_REG_READ(MACB_NCR) | MACB_NCR_TE);
	}

	/* Restart senders with a fresh descriptor budget */
	p->tx_epoch++;
	p->tx_resetting = false;
	k_sem_reset(&p->free_tx_descs);

	CDNS_MACB_REG_WRITE(MACB_IER, MACB_TX_INT_FLAGS);

	k_spin_unlock(&p->lock, key);

	for (unsigned int i = 0U; i < (CDNS_MACB_NB_TX_DESCS - 1U); i++) {
		k_sem_give(&p->free_tx_descs);
	}
}

/*
 * RX path
 */

/*
 * The receiver stopped on a descriptor that was not yet refilled. Writing NCR
 * makes it re-read that descriptor; cores with the used bit read erratum need
 * the receiver toggled off and on. Called with the lock held.
 */
static void cdns_macb_rx_rearm(const struct device *dev)
{
	const struct cdns_macb_config *cfg = dev->config;
	uint32_t ncr;

	ncr = CDNS_MACB_REG_READ(MACB_NCR);
	if ((cfg->caps & CDNS_MACB_CAPS_NEEDS_RSTONUBR) != 0U) {
		CDNS_MACB_REG_WRITE(MACB_NCR, ncr & ~MACB_NCR_RE);
		barrier_dmem_fence_full();
	}
	CDNS_MACB_REG_WRITE(MACB_NCR, ncr | MACB_NCR_RE);
}

/* Hand a fragment to the hardware at the ring head, called with the lock held */
static void cdns_macb_rx_refill_desc(const struct device *dev, struct net_buf *frag)
{
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;
	unsigned int idx = p->rx_head;
	struct cdns_macb_dma_desc *d = &cfg->rings->rx[idx];

	__ASSERT((d->addr & MACB_RX_USED) != 0U, "rx desc still owned by hardware");
	__ASSERT(((idx + 1U) % CDNS_MACB_NB_RX_DESCS) != p->rx_tail, "rx ring overfilled");
	__ASSERT(p->rx_frags[idx] == NULL, "rx desc still has a fragment");
	__ASSERT((POINTER_TO_UINT(frag->data) & ~MACB_RX_ADDR) == 0U, "rx buffer misaligned");

	/* The buffer may hold dirty lines from a previous use */
	sys_cache_data_invd_range(frag->data, frag->size);

	p->rx_frags[idx] = frag;

	/*
	 * Writing the address word with the used bit clear hands the descriptor
	 * over, the control word must be cleared before that.
	 */
	d->ctrl = 0U;
	barrier_dmem_fence_full();
	desc_set_addr(d, POINTER_TO_UINT(frag->data),
		      (idx == (CDNS_MACB_NB_RX_DESCS - 1U)) ? MACB_RX_WRAP : 0U);
	barrier_dmem_fence_full();

	INC_WRAP(p->rx_head, CDNS_MACB_NB_RX_DESCS);

	/*
	 * Masked events are not recorded, so the used bit read interrupt is
	 * unmasked before the receiver is restarted: it may run into the next
	 * unfilled descriptor right away.
	 */
	if (p->rx_stalled) {
		p->rx_stalled = false;
		CDNS_MACB_REG_WRITE(MACB_IER, MACB_INT_RXUBR);
		cdns_macb_rx_rearm(dev);
	}

	LOG_DBG("desc sem/head/tail=%d/%u/%u %s", k_sem_count_get(&p->free_rx_descs), p->rx_head,
		p->rx_tail, k_is_in_isr() ? "ISR" : "thread");
}

/* Refill one descriptor with a fresh fragment, or leave it to the refill thread */
static void cdns_macb_rx_refill(const struct device *dev)
{
	struct cdns_macb_priv *p = dev->data;
	struct net_buf *frag;

	frag = net_pkt_get_reserve_rx_data(RX_FRAG_SIZE, K_FOREVER);
	if (frag == NULL) {
		k_sem_give(&p->free_rx_descs);
		return;
	}

	K_SPINLOCK(&p->lock) {
		cdns_macb_rx_refill_desc(dev, frag);
	}
}

static void cdns_macb_rx_refill_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = arg1;
	struct cdns_macb_priv *p = dev->data;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (true) {
		if (k_sem_take(&p->free_rx_descs, K_FOREVER) == 0) {
			cdns_macb_rx_refill(dev);
		}
	}
}

/* A descriptor was consumed: refill it now or hand it to the refill thread */
static void cdns_macb_rx_recycle(const struct device *dev, struct net_buf *frag)
{
	struct cdns_macb_priv *p = dev->data;

	if (frag != NULL) {
		/* The fragment was not used for a packet, reuse it right away */
		K_SPINLOCK(&p->lock) {
			cdns_macb_rx_refill_desc(dev, frag);
		}
	} else if (IS_ENABLED(CONFIG_ETH_CDNS_MACB_RX_REFILL_IRQ)) {
		cdns_macb_rx_refill(dev);
	} else {
		k_sem_give(&p->free_rx_descs);
	}
}

static void cdns_macb_receive(const struct device *dev)
{
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;
	struct cdns_macb_dma_desc *rx = cfg->rings->rx;

	/*
	 * A consumed descriptor keeps its used bit until it is refilled, so
	 * the filled part of the ring ends at the refill head, not at the
	 * first used bit.
	 */
	while (p->rx_tail != p->rx_head) {
		unsigned int idx = p->rx_tail;
		struct cdns_macb_dma_desc *d = &rx[idx];
		struct net_buf *frag;
		uint32_t ctrl;

		/* stop here if hardware still owns it */
		if ((d->addr & MACB_RX_USED) == 0U) {
			break;
		}

		/* The control word is written before the used bit is set */
		barrier_dmem_fence_full();
		ctrl = d->ctrl;

		frag = p->rx_frags[idx];
		p->rx_frags[idx] = NULL;
		__ASSERT(frag != NULL, "rx desc without a fragment");

		INC_WRAP(p->rx_tail, CDNS_MACB_NB_RX_DESCS);

		LOG_DBG("desc[%u] ctrl = 0x%08x", idx, ctrl);

		if ((ctrl & MACB_RX_SOF) != 0U) {
			if (p->rx_pkt != NULL) {
				LOG_DBG("dropping partial frame");
				STATS_INC(p, errors.rx);
				net_pkt_unref(p->rx_pkt);
			}

			p->rx_pkt = net_pkt_rx_alloc_on_iface(p->iface, K_NO_WAIT);
			if (p->rx_pkt == NULL) {
				STATS_INC(p, errors.rx);
				STATS_INC(p, error_details.rx_buf_alloc_failed);
			}
			p->rx_nfrags = 0U;
		}

		if (p->rx_pkt == NULL) {
			/* No packet in progress, the fragment goes straight back */
			cdns_macb_rx_recycle(dev, frag);
			continue;
		}

		sys_cache_data_invd_range(frag->data, frag->size);

		/* Only the last buffer of a frame is partially filled */
		frag->len = RX_FRAG_SIZE;
		net_pkt_frag_add(p->rx_pkt, frag);
		p->rx_nfrags++;

		if ((ctrl & MACB_RX_EOF) != 0U) {
			uint32_t total = FIELD_GET(MACB_RX_FRMLEN, ctrl);
			uint32_t before = (p->rx_nfrags - 1U) * RX_FRAG_SIZE;

			if ((total <= before) || ((total - before) > RX_FRAG_SIZE)) {
				LOG_ERR("bad frame length %u in %u fragments", total, p->rx_nfrags);
				STATS_INC(p, errors.rx);
				STATS_INC(p, error_details.rx_length_errors);
				net_pkt_unref(p->rx_pkt);
			} else {
				frag->len = total - before;

				LOG_DBG("pkt len/frags=%zu/%u", net_pkt_get_len(p->rx_pkt),
					net_pkt_get_nbfrags(p->rx_pkt));

				if (net_recv_data(p->iface, p->rx_pkt) < 0) {
					STATS_INC(p, errors.rx);
					net_pkt_unref(p->rx_pkt);
				}
			}
			p->rx_pkt = NULL;
		}

		cdns_macb_rx_recycle(dev, NULL);
	}
}

/*
 * The receiver read a descriptor that was not refilled yet. Unless a refill
 * already happened, mask the interrupt until one does, which then re-arms the
 * receiver.
 */
static void cdns_macb_rx_used_bit_read(const struct device *dev)
{
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	STATS_INC(p, error_details.rx_no_buffer_count);

	K_SPINLOCK(&p->lock) {
		/*
		 * The receiver stopped at the oldest descriptor that was not
		 * refilled, which is the one the next refill fills. If it was
		 * refilled meanwhile, the receiver only needs to look again.
		 */
		if ((cfg->rings->rx[p->rx_head].addr & MACB_RX_USED) == 0U) {
			cdns_macb_rx_rearm(dev);
		} else {
			p->rx_stalled = true;
			CDNS_MACB_REG_WRITE(MACB_IDR, MACB_INT_RXUBR);
		}
	}
}

/*
 * Interrupt
 */

void cdns_macb_isr(const struct device *dev)
{
	struct cdns_macb_priv *p = dev->data;

	for (unsigned int n = 0U; n < ISR_LOOP_MAX; n++) {
		uint32_t status = CDNS_MACB_REG_READ(MACB_ISR);

		if (status == 0U) {
			break;
		}

		LOG_DBG("ISR = 0x%08x", status);
		cdns_macb_queue_isr_clear(dev, 0U, status);

		/*
		 * A frame cut short by the ring running dry leaves used
		 * descriptors behind without a receive complete event, so the
		 * ring is also drained on a used bit read.
		 */
		if ((status & (MACB_INT_RCOMP | MACB_INT_RXUBR)) != 0U) {
			cdns_macb_receive(dev);
		}

		if ((status & MACB_INT_RXUBR) != 0U) {
			cdns_macb_rx_used_bit_read(dev);
		}

		if ((status & MACB_INT_ROVR) != 0U) {
			STATS_INC(p, errors.rx);
			STATS_INC(p, error_details.rx_over_errors);
		}

		if ((status & MACB_INT_TCOMP) != 0U) {
			cdns_macb_tx_release(dev);
		}

		if ((status & MACB_INT_TXUBR) != 0U) {
			cdns_macb_tx_restart(dev);
		}

		if ((status & MACB_TX_ERR_FLAGS) != 0U) {
			CDNS_MACB_REG_WRITE(MACB_IDR, MACB_TX_INT_FLAGS);
			k_work_submit(&p->tx_error_work);
		}

		if ((status & MACB_INT_HRESP) != 0U) {
			LOG_ERR("DMA bus error: HRESP not OK");
			STATS_INC(p, errors.rx);
			STATS_INC(p, error_details.rx_dma_failed);
		}
	}
}

/*
 * Ethernet API
 */

static enum ethernet_hw_caps cdns_macb_caps(const struct device *dev, struct net_if *iface)
{
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;
	enum ethernet_hw_caps caps = ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;

	ARG_UNUSED(iface);

	if ((cfg->caps & CDNS_MACB_CAPS_GIGABIT_MODE_AVAILABLE) != 0U) {
		caps |= ETHERNET_LINK_1000BASE;
	}

	caps |= ETHERNET_PROMISC_MODE;

	if (CDNS_MACB_MCAST_FILTER) {
		caps |= ETHERNET_HW_FILTERING;
	}

	if (IS_ENABLED(CONFIG_NET_VLAN)) {
		caps |= ETHERNET_HW_VLAN;
	}

	if (cdns_macb_rx_csum_enabled(p)) {
		caps |= ETHERNET_HW_RX_CHKSUM_OFFLOAD;
	}

	if (cdns_macb_tx_csum_enabled(p)) {
		caps |= ETHERNET_HW_TX_CHKSUM_OFFLOAD;
	}

	return caps;
}

static int cdns_macb_set_config(const struct device *dev, struct net_if *iface,
				enum ethernet_config_type type,
				const struct ethernet_config *config)
{
	struct cdns_macb_priv *p = dev->data;
	int ret = 0;

	ARG_UNUSED(iface);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(p->mac_addr, config->mac_address.addr, sizeof(p->mac_addr));
		cdns_macb_set_mac_addr(dev, p->mac_addr);
		break;

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE: {
		/*
		 * The checksum offload discards frames with bad checksums,
		 * which promiscuous mode is expected to deliver.
		 */
		uint32_t rxcoen = cdns_macb_rx_csum_enabled(p) ? GEM_NCFGR_RXCOEN : 0U;
		bool is_promisc = (CDNS_MACB_REG_READ(MACB_NCFGR) & MACB_NCFGR_CAF) != 0U;

		if (config->promisc_mode == is_promisc) {
			ret = -EALREADY;
		} else if (config->promisc_mode) {
			cdns_macb_ncfgr_update(dev, rxcoen, MACB_NCFGR_CAF);
		} else {
			cdns_macb_ncfgr_update(dev, MACB_NCFGR_CAF, rxcoen);
		}
		break;
	}
#endif
#if CDNS_MACB_MCAST_FILTER
	case ETHERNET_CONFIG_TYPE_FILTER:
		cdns_macb_setup_multicast_filter(dev);
		break;
#endif
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int cdns_macb_get_config(const struct device *dev, struct net_if *iface,
				enum ethernet_config_type type, struct ethernet_config *config)
{
	struct cdns_macb_priv *p = dev->data;
	const enum ethernet_checksum_support supported =
		ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER | ETHERNET_CHECKSUM_SUPPORT_IPV6_HEADER |
		ETHERNET_CHECKSUM_SUPPORT_TCP | ETHERNET_CHECKSUM_SUPPORT_UDP;

	ARG_UNUSED(iface);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT:
		config->chksum_support = cdns_macb_rx_csum_enabled(p)
						 ? supported
						 : ETHERNET_CHECKSUM_SUPPORT_NONE;
		return 0;
	case ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT:
		config->chksum_support = cdns_macb_tx_csum_enabled(p)
						 ? supported
						 : ETHERNET_CHECKSUM_SUPPORT_NONE;
		return 0;
	default:
		return -ENOTSUP;
	}
}

__weak void cdns_macb_platform_link_speed_changed(const struct device *dev,
						  enum phy_link_speed speed)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(speed);
}

static void cdns_macb_phy_link_state_changed(const struct device *phy_dev,
					     struct phy_link_state *state, void *user_data)
{
	const struct device *dev = user_data;
	struct cdns_macb_priv *p = dev->data;

	ARG_UNUSED(phy_dev);

	if (state->is_up) {
		uint32_t set = 0U;

		if (PHY_LINK_IS_SPEED_100M(state->speed)) {
			set |= MACB_NCFGR_SPD;
		} else if (PHY_LINK_IS_SPEED_1000M(state->speed)) {
			set |= GEM_NCFGR_GBE;
		} else if (!PHY_LINK_IS_SPEED_10M(state->speed)) {
			LOG_ERR("unsupported link speed %d", state->speed);
		}

		if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
			set |= MACB_NCFGR_FD;
		}

		cdns_macb_ncfgr_update(dev, MACB_NCFGR_SPD | GEM_NCFGR_GBE | MACB_NCFGR_FD, set);
		cdns_macb_platform_link_speed_changed(dev, state->speed);
	}

	net_eth_carrier_set(p->iface, state->is_up);
}

static const struct device *cdns_macb_get_phy(const struct device *dev, struct net_if *iface)
{
	const struct cdns_macb_config *cfg = dev->config;

	ARG_UNUSED(iface);

	return cfg->phy_dev;
}

static void cdns_macb_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;

	__ASSERT(p->iface == NULL, "interface already initialized?");
	p->iface = iface;

	ethernet_init(iface);

	net_if_set_link_addr(iface, p->mac_addr, sizeof(p->mac_addr), NET_LINK_ETHERNET);

	/*
	 * Without a multicast filter every multicast frame is accepted, the
	 * filter itself is set up by the stack through set_config().
	 */
	if (!CDNS_MACB_MCAST_FILTER) {
		cdns_macb_set_hash(dev, UINT32_MAX, UINT32_MAX);
	}

	/* Do not start the interface until the PHY link is up */
	net_if_carrier_off(iface);
	phy_link_callback_set(cfg->phy_dev, cdns_macb_phy_link_state_changed, (void *)dev);

	/*
	 * Semaphores are used to represent number of available descriptors.
	 * The total is one less than ring size in order to always have at
	 * least one unfilled slot: it holds the end of queue marker on TX,
	 * and on RX it keeps the refill head from looping back onto the tail.
	 * The RX ring is filled before the receiver is enabled; the hardware
	 * stops on the first descriptor it finds unfilled.
	 */
	k_sem_init(&p->free_tx_descs, CDNS_MACB_NB_TX_DESCS - 1U, CDNS_MACB_NB_TX_DESCS - 1U);
	k_sem_init(&p->free_rx_descs, 0U, CDNS_MACB_NB_RX_DESCS - 1U);
	k_work_init(&p->tx_error_work, cdns_macb_tx_error_work);

	for (unsigned int i = 0U; i < (CDNS_MACB_NB_RX_DESCS - 1U); i++) {
		struct net_buf *frag = net_pkt_get_reserve_rx_data(RX_FRAG_SIZE, K_NO_WAIT);

		if (frag == NULL) {
			/* The refill thread completes the ring as buffers get freed */
			LOG_WRN("RX data pool too small to fill the ring, %u of %u filled", i,
				CDNS_MACB_NB_RX_DESCS - 1U);
			for (; i < (CDNS_MACB_NB_RX_DESCS - 1U); i++) {
				k_sem_give(&p->free_rx_descs);
			}
			break;
		}

		K_SPINLOCK(&p->lock) {
			cdns_macb_rx_refill_desc(dev, frag);
		}
	}

	/* set up RX buffer refill thread */
	k_thread_create(&p->rx_refill_thread, p->rx_refill_thread_stack,
			K_KERNEL_STACK_SIZEOF(p->rx_refill_thread_stack),
			cdns_macb_rx_refill_thread, (void *)dev, NULL, NULL,
			CONFIG_ETH_CDNS_MACB_RX_REFILL_THREAD_PRIORITY, K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&p->rx_refill_thread, "cdns_macb_rx_refill");

	/* start up TX/RX */
	K_SPINLOCK(&p->lock) {
		CDNS_MACB_REG_WRITE(MACB_NCR,
				    CDNS_MACB_REG_READ(MACB_NCR) | MACB_NCR_RE | MACB_NCR_TE);
	}

	/* unmask IRQs */
	CDNS_MACB_REG_WRITE(MACB_IER, MACB_RX_INT_FLAGS | MACB_TX_INT_FLAGS | MACB_INT_HRESP);

	LOG_DBG("done");
}

/*
 * Probe
 */

static uint32_t cdns_macb_dbw(uint32_t dcfg1)
{
	switch (FIELD_GET(GEM_DCFG1_DBWDEF, dcfg1)) {
	case 4:
		return FIELD_PREP(GEM_NCFGR_DBW, GEM_DBW128);
	case 2:
		return FIELD_PREP(GEM_NCFGR_DBW, GEM_DBW64);
	case 1:
	default:
		return FIELD_PREP(GEM_NCFGR_DBW, GEM_DBW32);
	}
}

/* The USRIO register selects the PHY interface on cores that have it */
static void cdns_macb_init_usrio(const struct device *dev)
{
	const struct cdns_macb_config *cfg = dev->config;
	uint32_t usrio = 0U;

	if ((cfg->caps & CDNS_MACB_CAPS_USRIO_HAS_MII) != 0U) {
		bool default_mii = (cfg->caps & CDNS_MACB_CAPS_USRIO_DEFAULT_IS_MII_GMII) != 0U;

		if (cfg->phy_iface == CDNS_MACB_PHY_IFACE_RGMII) {
			usrio = MACB_USRIO_MII;
		} else if ((cfg->phy_iface == CDNS_MACB_PHY_IFACE_RMII) && default_mii) {
			usrio = MACB_USRIO_MII;
		} else if (!default_mii) {
			usrio = MACB_USRIO_MII;
		}
	}

	if ((cfg->caps & CDNS_MACB_CAPS_USRIO_HAS_CLKEN) != 0U) {
		usrio |= MACB_USRIO_CLKEN;
	}

	CDNS_MACB_REG_WRITE(GEM_USRIO, usrio);
}

static void cdns_macb_init_rings(const struct device *dev)
{
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;
	struct cdns_macb_rings *rings = cfg->rings;

	cdns_macb_init_tx_ring(dev);

	/* The RX descriptors stay software owned until they are filled */
	for (unsigned int i = 0U; i < CDNS_MACB_NB_RX_DESCS; i++) {
		rings->rx[i].ctrl = 0U;
		desc_set_addr(&rings->rx[i], 0U,
			      MACB_RX_USED |
				      ((i == (CDNS_MACB_NB_RX_DESCS - 1U)) ? MACB_RX_WRAP : 0U));
		p->rx_frags[i] = NULL;
	}
	p->rx_head = 0U;
	p->rx_tail = 0U;

	/* Unused queues point at descriptors that never become available */
	rings->tx_tieoff.ctrl = MACB_TX_USED | MACB_TX_WRAP;
	desc_set_addr(&rings->tx_tieoff, 0U, 0U);
	rings->rx_tieoff.ctrl = 0U;
	desc_set_addr(&rings->rx_tieoff, 0U, MACB_RX_USED | MACB_RX_WRAP);

	barrier_dmem_fence_full();

	if (IS_ENABLED(CONFIG_ETH_CDNS_MACB_DMA_64BIT)) {
		CDNS_MACB_REG_WRITE(MACB_TBQPH, phys_hi32(p->rings_phys));
		CDNS_MACB_REG_WRITE(MACB_RBQPH, phys_hi32(p->rings_phys));
	}

	CDNS_MACB_REG_WRITE(MACB_TBQP, phys_lo32(desc_phys(dev, rings->tx)));
	CDNS_MACB_REG_WRITE(MACB_RBQP, phys_lo32(desc_phys(dev, rings->rx)));

	for (unsigned int q = 1U; q < 8U; q++) {
		if ((p->queue_mask & BIT(q)) == 0U) {
			continue;
		}

		CDNS_MACB_REG_WRITE(GEM_TBQP_Q(q), phys_lo32(desc_phys(dev, &rings->tx_tieoff)));
		CDNS_MACB_REG_WRITE(GEM_RBQP_Q(q), phys_lo32(desc_phys(dev, &rings->rx_tieoff)));
		CDNS_MACB_REG_WRITE(GEM_RBQS_Q(q), RX_FRAG_SIZE / CDNS_MACB_RX_BUFFER_MULTIPLE);
	}
}

int cdns_macb_probe(const struct device *dev)
{
	const struct cdns_macb_config *cfg = dev->config;
	struct cdns_macb_priv *p = dev->data;
	uint32_t mid, dcfg1, dcfg2, dcfg6;
	uint32_t ncfgr, dmacfg;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	mid = CDNS_MACB_REG_READ(MACB_MID);
	if (FIELD_GET(MACB_MID_IDNUM, mid) < MACB_MID_IDNUM_GEM) {
		LOG_ERR("MACB core (MID 0x%08x) is not supported, only GEM", mid);
		return -ENOTSUP;
	}
	LOG_DBG("GEM id 0x%03x revision 0x%04x",
		(uint32_t)FIELD_GET(MACB_MID_IDNUM, mid), (uint16_t)FIELD_GET(MACB_MID_REV, mid));

	/* Design configuration */
	dcfg1 = CDNS_MACB_REG_READ(GEM_DCFG1);
	dcfg2 = CDNS_MACB_REG_READ(GEM_DCFG2);
	dcfg6 = CDNS_MACB_REG_READ(GEM_DCFG6);
	LOG_DBG("DCFG1/2/6 = 0x%08x 0x%08x 0x%08x", dcfg1, dcfg2, dcfg6);

	p->isr_clear_on_write = (dcfg1 & GEM_DCFG1_IRQCOR) == 0U;
	p->pkt_buf_mode = (dcfg2 & (GEM_DCFG2_RX_PKT_BUFF | GEM_DCFG2_TX_PKT_BUFF)) ==
			  (GEM_DCFG2_RX_PKT_BUFF | GEM_DCFG2_TX_PKT_BUFF);
	p->queue_mask = 1U | (dcfg6 & GEM_DCFG6_QUEUE_MASK);

	if (IS_ENABLED(CONFIG_ETH_CDNS_MACB_DMA_64BIT) && ((dcfg6 & GEM_DCFG6_DAW64) == 0U)) {
		LOG_ERR("CONFIG_ETH_CDNS_MACB_DMA_64BIT set, but the core has no 64-bit DMA");
		return -ENOTSUP;
	}

	if ((cfg->caps & CDNS_MACB_CAPS_BD_RD_PREFETCH) != 0U) {
		uint32_t dcfg10 = CDNS_MACB_REG_READ(GEM_DCFG10);
		uint32_t rdbuff = MAX(FIELD_GET(GEM_DCFG10_RXBD_RDBUFF, dcfg10),
				      FIELD_GET(GEM_DCFG10_TXBD_RDBUFF, dcfg10));

		if ((rdbuff > 0U) && ((2U << (rdbuff - 1U)) > CDNS_MACB_DESC_PREFETCH_PAD)) {
			LOG_WRN("descriptor prefetch of %u exceeds the ring padding",
				2U << (rdbuff - 1U));
		}
	}

	/* Reset: disable RX/TX, clear statistics and status, mask every queue */
	CDNS_MACB_REG_WRITE(MACB_NCR, MACB_NCR_CLRSTAT | MACB_NCR_MPE);
	CDNS_MACB_REG_WRITE(MACB_TSR, UINT32_MAX);
	CDNS_MACB_REG_WRITE(MACB_RSR, UINT32_MAX);
	for (unsigned int q = 0U; q < 8U; q++) {
		if ((p->queue_mask & BIT(q)) != 0U) {
			cdns_macb_queue_idr(dev, q, UINT32_MAX);
			(void)cdns_macb_queue_isr_read(dev, q);
			cdns_macb_queue_isr_clear(dev, q, UINT32_MAX);
		}
	}

	/*
	 * Network configuration: slowest MDC, bus width from the design
	 * configuration, strip the FCS, accept 1536 byte frames.
	 */
	ncfgr = FIELD_PREP(GEM_NCFGR_CLK, GEM_CLK_DIV224) | cdns_macb_dbw(dcfg1) |
		MACB_NCFGR_DRFCS | MACB_NCFGR_BIG;
	if (cdns_macb_rx_csum_enabled(p)) {
		ncfgr |= GEM_NCFGR_RXCOEN;
	}
	if (cfg->phy_iface == CDNS_MACB_PHY_IFACE_SGMII) {
		ncfgr |= GEM_NCFGR_SGMIIEN | GEM_NCFGR_PCSSEL;
	}
	CDNS_MACB_REG_WRITE(MACB_NCFGR, ncfgr);

	if ((dcfg1 & GEM_DCFG1_USERIO) != 0U) {
		cdns_macb_init_usrio(dev);
	}

	/* DMA: full packet buffers, receive buffer size of one fragment */
	dmacfg = FIELD_PREP(GEM_DMACFG_FBLDO, cfg->dma_burst_length) |
		 FIELD_PREP(GEM_DMACFG_RXBMS, 3U) | GEM_DMACFG_TXPBMS |
		 FIELD_PREP(GEM_DMACFG_RXBS, RX_FRAG_SIZE / CDNS_MACB_RX_BUFFER_MULTIPLE);
	if (cdns_macb_tx_csum_enabled(p)) {
		dmacfg |= GEM_DMACFG_TXCOEN;
	}
	if (IS_ENABLED(CONFIG_ETH_CDNS_MACB_DMA_64BIT)) {
		dmacfg |= GEM_DMACFG_ADDR64;
	}
	CDNS_MACB_REG_WRITE(GEM_DMACFG, dmacfg);

	ret = cdns_macb_platform_init(dev);
	if (ret != 0) {
		return ret;
	}

	ret = net_eth_mac_load(&cfg->mac_cfg, p->mac_addr);
	if (ret == -ENODATA) {
		/*
		 * Fall back to the address the firmware left behind. Without
		 * one the interface waits for a MAC address to be set through
		 * ETHERNET_CONFIG_TYPE_MAC_ADDRESS.
		 */
		cdns_macb_get_hw_mac_addr(dev, p->mac_addr);
		if (net_eth_is_addr_unspecified((struct net_eth_addr *)p->mac_addr)) {
			LOG_WRN("no MAC address configured");
		}
	} else if (ret < 0) {
		LOG_ERR("failed to load the MAC address (%d)", ret);
		return ret;
	}

	cdns_macb_set_mac_addr(dev, p->mac_addr);
	for (unsigned int n = 1U; n < GEM_SA_COUNT; n++) {
		CDNS_MACB_REG_WRITE(GEM_SAB(n), 0U);
		CDNS_MACB_REG_WRITE(GEM_SAT(n), 0U);
	}
	CDNS_MACB_REG_WRITE(GEM_HRB, 0U);
	CDNS_MACB_REG_WRITE(GEM_HRT, 0U);

	cdns_macb_init_rings(dev);

	return 0;
}

const struct ethernet_api cdns_macb_api = {
	.iface_api.init = cdns_macb_iface_init,
	.get_capabilities = cdns_macb_caps,
	.set_config = cdns_macb_set_config,
	.get_config = cdns_macb_get_config,
	.get_phy = cdns_macb_get_phy,
	.send = cdns_macb_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = cdns_macb_get_stats,
#endif
};
