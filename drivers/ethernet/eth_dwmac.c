/*
 * Driver for Synopsys DesignWare MAC
 *
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define LOG_MODULE_NAME dwmac_core
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/barrier.h>
#include <ethernet/eth_stats.h>

#include "eth_dwmac_priv.h"
#include "eth.h"


/*
 * This driver references network data fragments with a zero-copy approach.
 * Even though the hardware can store received packets with an arbitrary
 * offset in memory, the gap bytes in the first word will be overwritten,
 * and subsequent fragments have to be buswidth-aligned anyway.
 * This means CONFIG_NET_BUF_VARIABLE_DATA_SIZE requires special care due
 * to its refcount byte placement, so we take the easy way out for now.
 */
#ifdef CONFIG_NET_BUF_VARIABLE_DATA_SIZE
#error "CONFIG_NET_BUF_VARIABLE_DATA_SIZE=y is not supported"
#endif

/* size of pre-allocated packet fragments */
#define RX_FRAG_SIZE CONFIG_NET_BUF_DATA_SIZE

/*
 * Grace period to wait for TX descriptor/fragment availability.
 * Worst case estimate is 1514*8 bits at 10 mbps for an existing packet
 * to be sent and freed, therefore 1ms is far more than enough.
 * Beyond that we'll drop the packet.
 */
#define TX_AVAIL_WAIT K_MSEC(1)

/* descriptor index iterators */
#define INC_WRAP(idx, size) ({ idx = (idx + 1) % size; })
#define DEC_WRAP(idx, size) ({ idx = (idx + size - 1) % size; })

/*
 * Descriptor physical location .
 * MMU is special here as we have a separate uncached mapping that is
 * different from the normal RAM virt_to_phys mapping.
 */
#ifdef CONFIG_MMU
#define TXDESC_PHYS_H(idx) hi32(p->tx_descs_phys + (idx) * sizeof(struct dwmac_dma_desc))
#define TXDESC_PHYS_L(idx) lo32(p->tx_descs_phys + (idx) * sizeof(struct dwmac_dma_desc))
#define RXDESC_PHYS_H(idx) hi32(p->rx_descs_phys + (idx) * sizeof(struct dwmac_dma_desc))
#define RXDESC_PHYS_L(idx) lo32(p->rx_descs_phys + (idx) * sizeof(struct dwmac_dma_desc))
#else
#define TXDESC_PHYS_H(idx) phys_hi32(&p->tx_descs[idx])
#define TXDESC_PHYS_L(idx) phys_lo32(&p->tx_descs[idx])
#define RXDESC_PHYS_H(idx) phys_hi32(&p->rx_descs[idx])
#define RXDESC_PHYS_L(idx) phys_lo32(&p->rx_descs[idx])
#endif

static inline uint32_t hi32(uintptr_t val)
{
	/* trickery to avoid compiler warnings on 32-bit build targets */
	if (sizeof(uintptr_t) > 4) {
		uint64_t hi = val;

		return hi >> 32;
	}
	return 0;
}

static inline uint32_t lo32(uintptr_t val)
{
	/* just a typecast return to be symmetric with hi32() */
	return val;
}

static inline uint32_t phys_hi32(void *addr)
{
	/* the default 1:1 mapping is assumed */
	return hi32((uintptr_t)addr);
}

static inline uint32_t phys_lo32(void *addr)
{
	/* the default 1:1 mapping is assumed */
	return lo32((uintptr_t)addr);
}

static enum ethernet_hw_caps dwmac_caps(const struct device *dev)
{
	struct dwmac_priv *p = dev->data;
	enum ethernet_hw_caps caps = 0;

	if (p->feature0 & MAC_HW_FEATURE0_GMIISEL) {
		caps |= ETHERNET_LINK_1000BASE_T;
	}

	if (p->feature0 & MAC_HW_FEATURE0_MIISEL) {
		caps |= ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T;
	}

	caps |= ETHERNET_PROMISC_MODE;

	return caps;
}

/* for debug logs */
static inline int net_pkt_get_nbfrags(struct net_pkt *pkt)
{
	struct net_buf *frag;
	int nbfrags = 0;

	for (frag = pkt->buffer; frag; frag = frag->frags) {
		nbfrags++;
	}
	return nbfrags;
}

static int dwmac_send(const struct device *dev, struct net_pkt *pkt)
{
	struct dwmac_priv *p = dev->data;
	struct net_buf *frag, *pinned;
	unsigned int pkt_len = net_pkt_get_len(pkt);
	unsigned int d_idx;
	struct dwmac_dma_desc *d;
	uint32_t des2_flags, des3_flags;

	LOG_DBG("pkt len/frags=%d/%d", pkt_len, net_pkt_get_nbfrags(pkt));

	/* initial flag values */
	des2_flags = 0;
	des3_flags = TDES3_FD | TDES3_OWN;

	/* map packet fragments */
	d_idx = p->tx_desc_head;
	frag = pkt->buffer;
	do {
		LOG_DBG("desc sem/head/tail=%d/%d/%d",
			k_sem_count_get(&p->free_tx_descs),
			p->tx_desc_head, p->tx_desc_tail);

		/* reserve a free descriptor for this fragment */
		if (k_sem_take(&p->free_tx_descs, TX_AVAIL_WAIT) != 0) {
			LOG_DBG("no more free tx descriptors");
			goto abort;
		}

		/* pin this fragment */
		pinned = net_buf_clone(frag, TX_AVAIL_WAIT);
		if (!pinned) {
			LOG_DBG("net_buf_clone() returned NULL");
			k_sem_give(&p->free_tx_descs);
			goto abort;
		}
		sys_cache_data_flush_range(pinned->data, pinned->len);
		p->tx_frags[d_idx] = pinned;
		LOG_DBG("d[%d]: frag %p pinned %p len %d", d_idx,
			frag->data, pinned->data, pinned->len);

		/* if no more fragments after this one: */
		if (!frag->frags) {
			/* set those flags on the last descriptor */
			des2_flags |= TDES2_IOC;
			des3_flags |= TDES3_LD;
		}

		/* fill the descriptor */
		d = &p->tx_descs[d_idx];
		d->des0 = phys_lo32(pinned->data);
		d->des1 = phys_hi32(pinned->data);
		d->des2 = pinned->len | des2_flags;
		d->des3 = pkt_len | des3_flags;

		/* clear the FD flag on subsequent descriptors */
		des3_flags &= ~TDES3_FD;

		INC_WRAP(d_idx, NB_TX_DESCS);
		frag = frag->frags;
	} while (frag);

	/* make sure all the above made it to memory */
	barrier_dmem_fence_full();

	/* update the descriptor index head */
	p->tx_desc_head = d_idx;

	/* lastly notify the hardware */
	REG_WRITE(DMA_CHn_TXDESC_TAIL_PTR(0), TXDESC_PHYS_L(d_idx));

	return 0;

abort:
	while (d_idx != p->tx_desc_head) {
		/* release already pinned fragments */
		DEC_WRAP(d_idx, NB_TX_DESCS);
		frag = p->tx_frags[d_idx];
		net_pkt_frag_unref(frag);
		k_sem_give(&p->free_tx_descs);
	}
	return -ENOMEM;
}

static void dwmac_tx_release(struct dwmac_priv *p)
{
	unsigned int d_idx;
	struct dwmac_dma_desc *d;
	struct net_buf *frag;
	uint32_t des3_val;

	for (d_idx = p->tx_desc_tail;
	     d_idx != p->tx_desc_head;
	     INC_WRAP(d_idx, NB_TX_DESCS), k_sem_give(&p->free_tx_descs)) {

		LOG_DBG("desc sem/tail/head=%d/%d/%d",
			k_sem_count_get(&p->free_tx_descs),
			p->tx_desc_tail, p->tx_desc_head);

		d = &p->tx_descs[d_idx];
		des3_val = d->des3;
		LOG_DBG("TDES3[%d] = 0x%08x", d_idx, des3_val);

		/* stop here if hardware still owns it */
		if (des3_val & TDES3_OWN) {
			break;
		}

		/* release corresponding fragments */
		frag = p->tx_frags[d_idx];
		LOG_DBG("unref frag %p", frag->data);
		net_pkt_frag_unref(frag);

		/* last packet descriptor: */
		if (des3_val & TDES3_LD) {
			/* log any errors */
			if (des3_val & TDES3_ES) {
				LOG_ERR("tx error (DES3 = 0x%08x)", des3_val);
				eth_stats_update_errors_tx(p->iface);
			}
		}
	}
	p->tx_desc_tail = d_idx;
}

static void dwmac_receive(struct dwmac_priv *p)
{
	struct dwmac_dma_desc *d;
	struct net_buf *frag;
	unsigned int d_idx, bytes_so_far;
	uint32_t des3_val;

	for (d_idx = p->rx_desc_tail;
	     d_idx != p->rx_desc_head;
	     INC_WRAP(d_idx, NB_RX_DESCS), k_sem_give(&p->free_rx_descs)) {

		LOG_DBG("desc sem/tail/head=%d/%d/%d",
			k_sem_count_get(&p->free_rx_descs),
			d_idx, p->rx_desc_head);

		d = &p->rx_descs[d_idx];
		des3_val = d->des3;
		LOG_DBG("RDES3[%d] = 0x%08x", d_idx, des3_val);

		/* stop here if hardware still owns it */
		if (des3_val & RDES3_OWN) {
			break;
		}

		/* we ignore those for now */
		if (des3_val & RDES3_CTXT) {
			continue;
		}

		/* a packet's first descriptor: */
		if (des3_val & RDES3_FD) {
			p->rx_bytes = 0;
			if (p->rx_pkt) {
				LOG_ERR("d[%d] first desc but pkt exists", d_idx);
				eth_stats_update_errors_rx(p->iface);
				net_pkt_unref(p->rx_pkt);
			}
			p->rx_pkt = net_pkt_rx_alloc_on_iface(p->iface, K_NO_WAIT);
			if (!p->rx_pkt) {
				LOG_ERR("net_pkt_rx_alloc_on_iface() failed");
				eth_stats_update_errors_rx(p->iface);
			}
		}

		if (!p->rx_pkt) {
			LOG_ERR("no rx_pkt: skipping desc %d", d_idx);
			continue;
		}

		/* retrieve current fragment */
		frag = p->rx_frags[d_idx];
		p->rx_frags[d_idx] = NULL;
		bytes_so_far = FIELD_GET(RDES3_PL, des3_val);
		frag->len = bytes_so_far - p->rx_bytes;
		p->rx_bytes = bytes_so_far;
		net_pkt_frag_add(p->rx_pkt, frag);

		/* last descriptor: */
		if (des3_val & RDES3_LD) {
			/* submit packet if no errors */
			if (!(des3_val & RDES3_ES)) {
				LOG_DBG("pkt len/frags=%zd/%d",
					net_pkt_get_len(p->rx_pkt),
					net_pkt_get_nbfrags(p->rx_pkt));
				net_recv_data(p->iface, p->rx_pkt);
			} else {
				LOG_ERR("rx error (DES3 = 0x%08x)", des3_val);
				eth_stats_update_errors_rx(p->iface);
				net_pkt_unref(p->rx_pkt);
			}
			p->rx_pkt = NULL;
		}
	}
	p->rx_desc_tail = d_idx;
}

static void dwmac_rx_refill_thread(void *arg1, void *unused1, void *unused2)
{
	struct dwmac_priv *p = arg1;
	struct dwmac_dma_desc *d;
	struct net_buf *frag;
	unsigned int d_idx;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	d_idx = p->rx_desc_head;
	for (;;) {
		LOG_DBG("desc sem/head/tail=%d/%d/%d",
			k_sem_count_get(&p->free_rx_descs),
			p->rx_desc_head, p->rx_desc_tail);

		/* wait for an empty descriptor */
		if (k_sem_take(&p->free_rx_descs, K_FOREVER) != 0) {
			LOG_ERR("can't get free RX desc to refill");
			break;
		}

		d = &p->rx_descs[d_idx];

		__ASSERT(!(d->des3 & RDES3_OWN),
			 "desc[%d]=0x%x: still hw owned! (sem/head/tail=%d/%d/%d)",
			 d_idx, d->des3, k_sem_count_get(&p->free_rx_descs),
			 p->rx_desc_head, p->rx_desc_tail);

		frag = p->rx_frags[d_idx];

		/* get a new fragment if the previous one was consumed */
		if (!frag) {
			frag = net_pkt_get_reserve_rx_data(RX_FRAG_SIZE, K_FOREVER);
			if (!frag) {
				LOG_ERR("net_pkt_get_reserve_rx_data() returned NULL");
				k_sem_give(&p->free_rx_descs);
				break;
			}
			LOG_DBG("new frag[%d] at %p", d_idx, frag->data);
			__ASSERT(frag->size == RX_FRAG_SIZE, "");
			sys_cache_data_invd_range(frag->data, frag->size);
			p->rx_frags[d_idx] = frag;
		} else {
			LOG_DBG("reusing frag[%d] at %p", d_idx, frag->data);
		}

		/* all is good: initialize the descriptor */
		d->des0 = phys_lo32(frag->data);
		d->des1 = phys_hi32(frag->data);
		d->des2 = 0;
		d->des3 = RDES3_BUF1V | RDES3_IOC | RDES3_OWN;

		/* commit the above to memory */
		barrier_dmem_fence_full();

		/* advance to the next descriptor */
		p->rx_desc_head = INC_WRAP(d_idx, NB_RX_DESCS);

		/* lastly notify the hardware */
		REG_WRITE(DMA_CHn_RXDESC_TAIL_PTR(0), RXDESC_PHYS_L(d_idx));
	}
}

static void dwmac_dma_irq(struct dwmac_priv *p, unsigned int ch)
{
	uint32_t status;

	status = REG_READ(DMA_CHn_STATUS(ch));
	LOG_DBG("DMA_CHn_STATUS(%d) = 0x%08x", ch, status);
	REG_WRITE(DMA_CHn_STATUS(ch), status);

	__ASSERT(ch == 0, "only one DMA channel is currently supported");

	if (status & DMA_CHn_STATUS_AIS) {
		LOG_ERR("Abnormal Interrupt Status received (0x%x)", status);
	}

	if (status & DMA_CHn_STATUS_TI) {
		dwmac_tx_release(p);
	}

	if (status & DMA_CHn_STATUS_RI) {
		dwmac_receive(p);
	}
}

static void dwmac_mac_irq(struct dwmac_priv *p)
{
	uint32_t status;

	status = REG_READ(MAC_IRQ_STATUS);
	LOG_DBG("MAC_IRQ_STATUS = 0x%08x", status);
	__ASSERT(false, "unimplemented");
}

static void dwmac_mtl_irq(struct dwmac_priv *p)
{
	uint32_t status;

	status = REG_READ(MTL_IRQ_STATUS);
	LOG_DBG("MTL_IRQ_STATUS = 0x%08x", status);
	__ASSERT(false, "unimplemented");
}

void dwmac_isr(const struct device *ddev)
{
	struct dwmac_priv *p = ddev->data;
	uint32_t irq_status;
	unsigned int ch;

	irq_status = REG_READ(DMA_IRQ_STATUS);
	LOG_DBG("DMA_IRQ_STATUS = 0x%08x", irq_status);

	while (irq_status & 0xff) {
		ch = find_lsb_set(irq_status & 0xff) - 1;
		irq_status &= ~BIT(ch);
		dwmac_dma_irq(p, ch);
	}

	if (irq_status & DMA_IRQ_STATUS_MTLIS) {
		dwmac_mtl_irq(p);
	}

	if (irq_status & DMA_IRQ_STATUS_MACIS) {
		dwmac_mac_irq(p);
	}
}

static void dwmac_set_mac_addr(struct dwmac_priv *p, uint8_t *addr, int n)
{
	uint32_t reg_val;

	reg_val = (addr[5] << 8) | addr[4];
	REG_WRITE(MAC_ADDRESS_HIGH(n), reg_val | MAC_ADDRESS_HIGH_AE);
	reg_val = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	REG_WRITE(MAC_ADDRESS_LOW(n), reg_val);
}

static int dwmac_set_config(const struct device *dev,
			    enum ethernet_config_type type,
			    const struct ethernet_config *config)
{
	struct dwmac_priv *p = dev->data;
	uint32_t reg_val;
	int ret = 0;

	(void) reg_val; /* silence the "unused variable" warning */

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(p->mac_addr, config->mac_address.addr, sizeof(p->mac_addr));
		dwmac_set_mac_addr(p, p->mac_addr, 0);
		net_if_set_link_addr(p->iface, p->mac_addr,
				     sizeof(p->mac_addr), NET_LINK_ETHERNET);
		break;

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		reg_val = REG_READ(MAC_PKT_FILTER);
		if (config->promisc_mode &&
		    !(reg_val & MAC_PKT_FILTER_PR)) {
			REG_WRITE(MAC_PKT_FILTER,
				  reg_val | MAC_PKT_FILTER_PR);
		} else if (!config->promisc_mode &&
			   (reg_val & MAC_PKT_FILTER_PR)) {
			REG_WRITE(MAC_PKT_FILTER,
				  reg_val & ~MAC_PKT_FILTER_PR);
		} else {
			ret = -EALREADY;
		}
		break;
#endif

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static void dwmac_iface_init(struct net_if *iface)
{
	struct dwmac_priv *p = net_if_get_device(iface)->data;
	uint32_t reg_val;

	__ASSERT(!p->iface, "interface already initialized?");
	p->iface = iface;

	ethernet_init(iface);

	net_if_set_link_addr(iface, p->mac_addr, sizeof(p->mac_addr),
			     NET_LINK_ETHERNET);
	dwmac_set_mac_addr(p, p->mac_addr, 0);

	/*
	 * Semaphores are used to represent number of available descriptors.
	 * The total is one less than ring size in order to always have
	 * at least one inactive slot for the hardware tail pointer to
	 * stop at and to prevent our head indexes from looping back
	 * onto our tail indexes.
	 */
	k_sem_init(&p->free_tx_descs, NB_TX_DESCS - 1, NB_TX_DESCS - 1);
	k_sem_init(&p->free_rx_descs, NB_RX_DESCS - 1, NB_RX_DESCS - 1);

	/* set up RX buffer refill thread */
	k_thread_create(&p->rx_refill_thread, p->rx_refill_thread_stack,
			K_KERNEL_STACK_SIZEOF(p->rx_refill_thread_stack),
			dwmac_rx_refill_thread, p, NULL, NULL,
			0, K_PRIO_PREEMPT(0), K_NO_WAIT);
	k_thread_name_set(&p->rx_refill_thread, "dwmac_rx_refill");

	/* start up TX/RX */
	reg_val = REG_READ(DMA_CHn_TX_CTRL(0));
	REG_WRITE(DMA_CHn_TX_CTRL(0), reg_val | DMA_CHn_TX_CTRL_St);
	reg_val = REG_READ(DMA_CHn_RX_CTRL(0));
	REG_WRITE(DMA_CHn_RX_CTRL(0), reg_val | DMA_CHn_RX_CTRL_SR);
	reg_val = REG_READ(MAC_CONF);
	reg_val |= MAC_CONF_CST | MAC_CONF_TE | MAC_CONF_RE;
	REG_WRITE(MAC_CONF, reg_val);

	/* unmask IRQs */
	REG_WRITE(DMA_CHn_IRQ_ENABLE(0),
		  DMA_CHn_IRQ_ENABLE_TIE |
		  DMA_CHn_IRQ_ENABLE_RIE |
		  DMA_CHn_IRQ_ENABLE_NIE |
		  DMA_CHn_IRQ_ENABLE_FBEE |
		  DMA_CHn_IRQ_ENABLE_CDEE |
		  DMA_CHn_IRQ_ENABLE_AIE);

	LOG_DBG("done");
}

int dwmac_probe(const struct device *dev)
{
	struct dwmac_priv *p = dev->data;
	int ret;
	uint32_t reg_val;
	k_timepoint_t timeout;

	ret = dwmac_bus_init(p);
	if (ret != 0) {
		return ret;
	}

	reg_val = REG_READ(MAC_VERSION);
	LOG_INF("HW version %u.%u0", (reg_val >> 4) & 0xf, reg_val & 0xf);
	__ASSERT(FIELD_GET(MAC_VERSION_SNPSVER, reg_val) >= 0x40,
		 "This driver expects DWC-ETHERNET version >= 4.00");

	/* resets all of the MAC internal registers and logic */
	REG_WRITE(DMA_MODE, DMA_MODE_SWR);
	timeout = sys_timepoint_calc(K_MSEC(100));
	while (REG_READ(DMA_MODE) & DMA_MODE_SWR) {
		if (sys_timepoint_expired(timeout)) {
			LOG_ERR("unable to reset hardware");
			return -EIO;
		}
	}

	/* get configured hardware features */
	p->feature0 = REG_READ(MAC_HW_FEATURE0);
	p->feature1 = REG_READ(MAC_HW_FEATURE1);
	p->feature2 = REG_READ(MAC_HW_FEATURE2);
	p->feature3 = REG_READ(MAC_HW_FEATURE3);
	LOG_DBG("hw_feature: 0x%08x 0x%08x 0x%08x 0x%08x",
		p->feature0, p->feature1, p->feature2, p->feature3);

	dwmac_platform_init(p);

	memset(p->tx_descs, 0, NB_TX_DESCS * sizeof(struct dwmac_dma_desc));
	memset(p->rx_descs, 0, NB_RX_DESCS * sizeof(struct dwmac_dma_desc));

	/* set up DMA */
	REG_WRITE(DMA_CHn_TX_CTRL(0), 0);
	REG_WRITE(DMA_CHn_RX_CTRL(0),
		  FIELD_PREP(DMA_CHn_RX_CTRL_PBL, 32) |
		  FIELD_PREP(DMA_CHn_RX_CTRL_RBSZ, RX_FRAG_SIZE));
	REG_WRITE(DMA_CHn_TXDESC_LIST_HADDR(0), TXDESC_PHYS_H(0));
	REG_WRITE(DMA_CHn_TXDESC_LIST_ADDR(0), TXDESC_PHYS_L(0));
	REG_WRITE(DMA_CHn_RXDESC_LIST_HADDR(0), RXDESC_PHYS_H(0));
	REG_WRITE(DMA_CHn_RXDESC_LIST_ADDR(0), RXDESC_PHYS_L(0));
	REG_WRITE(DMA_CHn_TXDESC_RING_LENGTH(0), NB_TX_DESCS - 1);
	REG_WRITE(DMA_CHn_RXDESC_RING_LENGTH(0), NB_RX_DESCS - 1);

	return 0;
}

const struct ethernet_api dwmac_api = {
	.iface_api.init		= dwmac_iface_init,
	.get_capabilities	= dwmac_caps,
	.set_config		= dwmac_set_config,
	.send			= dwmac_send,
};
