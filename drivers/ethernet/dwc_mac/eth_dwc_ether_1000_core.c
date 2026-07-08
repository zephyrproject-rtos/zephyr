/*
 * Driver for Synopsys DesignWare MAC (10/100/1000 Universal version)
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwmac_core, CONFIG_ETHERNET_LOG_LEVEL);

#include <sys/types.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/device_mmio.h>
#include <ethernet/eth_stats.h>

#include "eth_dwmac_priv.h"
#include "../eth.h"

#define RX_FRAG_SIZE  CONFIG_NET_BUF_DATA_SIZE
#define TX_AVAIL_WAIT K_MSEC(1)

#define INC_WRAP(idx, size) ({ (idx) = ((idx) + 1) % (size); })
#define DEC_WRAP(idx, size) ({ (idx) = ((idx) + (size) - 1) % (size); })

#define TDES0_OWN BIT(31)
#define TDES0_IC  BIT(30)
#define TDES0_LS  BIT(29)
#define TDES0_FS  BIT(28)
#define TDES0_TCH BIT(20)
#define TDES0_ES  BIT(15)
#define TDES0_CIC GENMASK(23, 22)

#define TDES1_TBS1 GENMASK(12, 0)

#define RDES0_OWN BIT(31)
#define RDES0_FL  GENMASK(29, 16)
#define RDES0_ES  BIT(15)
#define RDES0_FS  BIT(9)
#define RDES0_LS  BIT(8)

#define RDES1_RBS1 GENMASK(12, 0)
#define RDES1_RCH  BIT(14)

#define RX_LEN_FROM_RDES0(rdes0) (FIELD_GET(RDES0_FL, (rdes0)))

#define TXDESC_PHYS_L(idx) phys_lo32(&p->tx_descs[idx])
#define RXDESC_PHYS_L(idx) phys_lo32(&p->rx_descs[idx])

static uint32_t phys_lo32(void *addr)
{
	return (uint32_t)(uintptr_t)addr;
}

static enum ethernet_hw_caps dwmac_caps(const struct device *dev __unused,
					struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#ifdef CONFIG_NET_PROMISCUOUS_MODE
	       | ETHERNET_PROMISC_MODE
#endif
#ifdef CONFIG_ETH_DWC_ETHER_RX_HW_CHECKSUM
	       | ETHERNET_HW_RX_CHKSUM_OFFLOAD
#endif
#ifdef CONFIG_ETH_DWC_ETHER_TX_HW_CHECKSUM
	       | ETHERNET_HW_TX_CHKSUM_OFFLOAD
#endif
		;
}

static __maybe_unused unsigned int net_pkt_get_nbfrags(struct net_pkt *pkt)
{
	unsigned int nbfrags = 0U;

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		nbfrags++;
	}

	return nbfrags;
}

#define TDES0_FLAGS_DEFAULT (TDES0_TCH | TDES0_IC | \
	(IS_ENABLED(CONFIG_ETH_DWC_ETHER_TX_HW_CHECKSUM) ? TDES0_CIC : 0U))

static int dwmac_send(const struct device *dev, struct net_pkt *pkt)
{
	struct dwmac_priv *p = dev->data;
	unsigned int d_idx;
	struct dwmac_dma_desc *d;
	uint32_t des0_flags;

	LOG_DBG("pkt len/frags=%zu/%u", net_pkt_get_len(pkt), net_pkt_get_nbfrags(pkt));

	d_idx = p->tx_desc_head;
	des0_flags = TDES0_FLAGS_DEFAULT;

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		if (k_sem_take(&p->free_tx_descs, TX_AVAIL_WAIT) != 0) {
			LOG_DBG("no more free tx descriptors");
			goto abort;
		}

		net_pkt_frag_ref(frag);
		sys_cache_data_flush_range(frag->data, frag->len);
		p->tx_frags[d_idx] = frag;

		d = &p->tx_descs[d_idx];
		d->des0 = des0_flags;
		d->des1 = FIELD_PREP(TDES1_TBS1, frag->len);
		d->des2 = phys_lo32(frag->data);

		if (!frag->frags) {
			d->des0 |= TDES0_LS;
		}

		des0_flags = TDES0_OWN | TDES0_FLAGS_DEFAULT;
		INC_WRAP(d_idx, NB_TX_DESCS);
	};

	barrier_dmem_fence_full();

	d = &p->tx_descs[p->tx_desc_head];
	d->des0 |= TDES0_OWN | TDES0_FS;

	barrier_dmem_fence_full();
	p->tx_desc_head = d_idx;
	REG_WRITE(DWMAC_DMATPDR, 0);

	return 0;

abort:
	while (d_idx != p->tx_desc_head) {
		DEC_WRAP(d_idx, NB_TX_DESCS);
		net_pkt_frag_unref(p->tx_frags[d_idx]);
		k_sem_give(&p->free_tx_descs);
	}

	return -ENOMEM;
}

static void dwmac_tx_release(const struct device *dev)
{
	struct dwmac_priv *p = dev->data;
	unsigned int d_idx;
	struct dwmac_dma_desc *d;
	struct net_buf *frag;
	uint32_t des0;

	for (d_idx = p->tx_desc_tail; d_idx != p->tx_desc_head; INC_WRAP(d_idx, NB_TX_DESCS)) {
		d = &p->tx_descs[d_idx];
		des0 = d->des0;

		if ((des0 & TDES0_OWN) != 0U) {
			break;
		}

		frag = p->tx_frags[d_idx];
		net_pkt_frag_unref(frag);

		if ((des0 & TDES0_LS) != 0U && (des0 & TDES0_ES) != 0U) {
			LOG_ERR("tx error (DES0 = 0x%08x)", des0);
			eth_stats_update_errors_tx(p->iface);
		}

		k_sem_give(&p->free_tx_descs);
	}

	p->tx_desc_tail = d_idx;
}

static void dwmac_receive(const struct device *dev)
{
	struct dwmac_priv *p = dev->data;
	struct dwmac_dma_desc *d;
	struct net_buf *frag;
	unsigned int d_idx;
	uint32_t des0;
	uint16_t bytes_so_far;

	for (d_idx = p->rx_desc_tail; d_idx != p->rx_desc_head;
	     INC_WRAP(d_idx, NB_RX_DESCS), k_sem_give(&p->free_rx_descs)) {
		d = &p->rx_descs[d_idx];
		des0 = d->des0;

		if ((des0 & RDES0_OWN) != 0U) {
			break;
		}

		if ((des0 & RDES0_FS) != 0U) {
			p->rx_bytes = 0;
			if (p->rx_pkt != NULL) {
				eth_stats_update_errors_rx(p->iface);
				net_pkt_unref(p->rx_pkt);
			}
			p->rx_pkt = net_pkt_rx_alloc_on_iface(p->iface, K_NO_WAIT);
			if (p->rx_pkt == NULL) {
				eth_stats_update_errors_rx(p->iface);
			}
		}

		if (p->rx_pkt == NULL) {
			continue;
		}

		frag = p->rx_frags[d_idx];
		p->rx_frags[d_idx] = NULL;

		sys_cache_data_invd_range(frag->data, frag->size);

		bytes_so_far = RX_LEN_FROM_RDES0(des0);

		if (bytes_so_far < p->rx_bytes) {
			eth_stats_update_errors_rx(p->iface);
			net_pkt_unref(p->rx_pkt);
			p->rx_pkt = NULL;
			net_buf_unref(frag);
			continue;
		}

		frag->len = bytes_so_far - p->rx_bytes;
		p->rx_bytes = bytes_so_far;
		net_pkt_frag_add(p->rx_pkt, frag);

		if ((des0 & RDES0_LS) != 0U) {
			if ((des0 & RDES0_ES) == 0U) {
				net_recv_data(p->iface, p->rx_pkt);
			} else {
				LOG_ERR("rx error (DES0 = 0x%08x)", des0);
				eth_stats_update_errors_rx(p->iface);
				net_pkt_unref(p->rx_pkt);
			}
			p->rx_pkt = NULL;
		}
	}

	p->rx_desc_tail = d_idx;
}

static void dwmac_rx_refill(const struct device *dev, unsigned int d_idx)
{
	struct dwmac_priv *p = dev->data;
	struct dwmac_dma_desc *d;
	struct net_buf *frag;

	d = &p->rx_descs[d_idx];
	__ASSERT(!(d->des0 & RDES0_OWN), "rx desc still owned");

	frag = p->rx_frags[d_idx];
	if (frag == NULL) {
		frag = net_pkt_get_reserve_rx_data(RX_FRAG_SIZE, K_FOREVER);
		if (frag == NULL) {
			k_sem_give(&p->free_rx_descs);
			return;
		}
		__ASSERT_NO_MSG(frag->size == RX_FRAG_SIZE);
		p->rx_frags[d_idx] = frag;
	}

	d->des1 = FIELD_PREP(RDES1_RBS1, frag->size) | RDES1_RCH;
	d->des2 = phys_lo32(frag->data);

	barrier_dmem_fence_full();

	d->des0 = RDES0_OWN;

	p->rx_desc_head = INC_WRAP(d_idx, NB_RX_DESCS);
	REG_WRITE(DWMAC_DMARPDR, 0);

	LOG_DBG("desc sem/head/tail=%d/%d/%d", k_sem_count_get(&p->free_rx_descs), p->rx_desc_head,
		p->rx_desc_tail);
}

static void dwmac_rx_refill_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = arg1;
	struct dwmac_priv *p = dev->data;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (true) {
		if (k_sem_take(&p->free_rx_descs, K_FOREVER) == 0) {
			dwmac_rx_refill(dev, p->rx_desc_head);
		}
	}
}

void dwmac_isr(const struct device *dev)
{
	uint32_t status;

	status = REG_READ(DWMAC_DMASR);
	REG_WRITE(DWMAC_DMASR, status);

	LOG_DBG("DMA status 0x%08x", status);

	if ((status & DWMAC_DMASR_AIS) != 0) {
		LOG_ERR("abnormal interrupt status (0x%08x)", status);
	}

	if ((status & DWMAC_DMASR_TI) != 0) {
		dwmac_tx_release(dev);
	}

	if ((status & DWMAC_DMASR_RI) != 0) {
		dwmac_receive(dev);
	}
}

static void dwmac_set_mac_addr(const struct device *dev, const uint8_t *addr)
{
	REG_WRITE(DWMAC_MACA0HR, (uint32_t)sys_get_le16(&addr[4]));
	REG_WRITE(DWMAC_MACA0LR, sys_get_le32(&addr[0]));
}

static int dwmac_set_config(const struct device *dev, struct net_if *iface __unused,
			    enum ethernet_config_type type, const struct ethernet_config *config)
{
	uint32_t reg __maybe_unused;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		dwmac_set_mac_addr(dev, config->mac_address.addr);
		break;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		reg = REG_READ(DWMAC_MACFFR);
		if (config->promisc_mode) {
			reg |= DWMAC_MACFFR_PM;
		} else {
			reg &= ~DWMAC_MACFFR_PM;
		}
		REG_WRITE(DWMAC_MACFFR, reg);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void phy_link_state_changed(const struct device *phy_dev __unused,
				   struct phy_link_state *state, void *user_data)
{
	const struct device *dev = user_data;
	struct dwmac_priv *p = dev->data;
	uint32_t reg;

	if (state->is_up) {
		reg = REG_READ(DWMAC_MACCR);

		reg &= ~(DWMAC_MACCR_PS | DWMAC_MACCR_FES | DWMAC_MACCR_DM);

		if (PHY_LINK_IS_SPEED_100M(state->speed)) {
			reg |= DWMAC_MACCR_FES | DWMAC_MACCR_PS;
		} else if (PHY_LINK_IS_SPEED_10M(state->speed)) {
			reg |= DWMAC_MACCR_PS;
		} else {
			/* nothing to do for other speeds */
		}

		if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
			reg |= DWMAC_MACCR_DM;
		}

		REG_WRITE(DWMAC_MACCR, reg);
	}

	net_eth_carrier_set(p->iface, state->is_up);
}

static const struct device *dwmac_get_phy(const struct device *dev, struct net_if *iface __unused)
{
	const struct dwmac_config *cfg = dev->config;

	return cfg->phy_dev;
}

static void dwmac_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dwmac_priv *p = dev->data;
	const struct dwmac_config *cfg = dev->config;

	p->iface = iface;

	ethernet_init(iface);
	k_sem_init(&p->free_tx_descs, NB_TX_DESCS - 1, NB_TX_DESCS - 1);
	k_sem_init(&p->free_rx_descs, NB_RX_DESCS - 1, NB_RX_DESCS - 1);

	net_if_set_link_addr(iface, p->mac_addr, sizeof(p->mac_addr), NET_LINK_ETHERNET);
	dwmac_set_mac_addr(dev, p->mac_addr);

	/* Pass all multicast frames */
	REG_WRITE(DWMAC_MACFFR, REG_READ(DWMAC_MACFFR) | DWMAC_MACFFR_PAM);

	net_if_carrier_off(iface);
	if (device_is_ready(cfg->phy_dev)) {
		phy_link_callback_set(cfg->phy_dev, phy_link_state_changed, (void *)dev);
	}

	k_thread_create(&p->rx_refill_thread, p->rx_refill_thread_stack,
			K_KERNEL_STACK_SIZEOF(p->rx_refill_thread_stack), dwmac_rx_refill_thread,
			(void *)dev, NULL, NULL, CONFIG_ETH_DWMAC_RX_REFILL_THREAD_PRIORITY,
			K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&p->rx_refill_thread, "dwmac_rx_refill");

	REG_WRITE(DWMAC_DMAIER,
		  DWMAC_DMAIER_NISE | DWMAC_DMAIER_RIE | DWMAC_DMAIER_TIE | DWMAC_DMAIER_AISE);

	REG_WRITE(DWMAC_DMAOMR, REG_READ(DWMAC_DMAOMR) | DWMAC_DMAOMR_SR | DWMAC_DMAOMR_ST);
	REG_WRITE(DWMAC_MACCR,
		  REG_READ(DWMAC_MACCR) | DWMAC_MACCR_CSTF | DWMAC_MACCR_TE | DWMAC_MACCR_RE);
}

int dwmac_probe(const struct device *dev)
{
	struct dwmac_priv *p = dev->data;
	int ret;
	k_timepoint_t timeout;
	unsigned int i;
	uint32_t reg_val;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = dwmac_bus_init(dev);
	if (ret < 0) {
		return ret;
	}

	REG_WRITE(DWMAC_DMABMR, REG_READ(DWMAC_DMABMR) | DWMAC_DMABMR_SR);

	timeout = sys_timepoint_calc(K_MSEC(100));
	while ((REG_READ(DWMAC_DMABMR) & DWMAC_DMABMR_SR) != 0U) {
		if (sys_timepoint_expired(timeout)) {
			LOG_ERR("unable to reset hardware");
			return -EIO;
		}
	}

	reg_val = REG_READ(DWMAC_MACVERR);
	LOG_INF("HW version %u.%u0", (reg_val >> 4) & 0xf, reg_val & 0xf);

	/* get configured hardware features */
	p->feature0 = REG_READ(DWMAC_HWFR);
	LOG_DBG("hw_feature: 0x%08x", p->feature0);

	ret = dwmac_platform_init(dev);
	if (ret < 0) {
		return ret;
	}

	memset(p->tx_descs, 0, NB_TX_DESCS * sizeof(struct dwmac_dma_desc));
	memset(p->rx_descs, 0, NB_RX_DESCS * sizeof(struct dwmac_dma_desc));

	for (i = 0; i < NB_TX_DESCS; i++) {
		p->tx_descs[i].des1 = TDES0_TCH;
		p->tx_descs[i].des3 = TXDESC_PHYS_L((i + 1) % NB_TX_DESCS);
	}

	for (i = 0; i < NB_RX_DESCS; i++) {
		p->rx_descs[i].des1 = RDES1_RCH;
		p->rx_descs[i].des3 = RXDESC_PHYS_L((i + 1) % NB_RX_DESCS);
	}

	if (IS_ENABLED(CONFIG_ETH_DWC_ETHER_1000_CORE_EDFE)) {
		REG_WRITE(DWMAC_DMABMR, REG_READ(DWMAC_DMABMR) | DWMAC_DMABMR_EDFE);
	}

	REG_WRITE(DWMAC_DMATDLAR, TXDESC_PHYS_L(0));
	REG_WRITE(DWMAC_DMARDLAR, RXDESC_PHYS_L(0));
	REG_WRITE(DWMAC_DMAOMR, DWMAC_DMAOMR_TSF | DWMAC_DMAOMR_RSF);

	if (IS_ENABLED(CONFIG_ETH_DWC_ETHER_RX_HW_CHECKSUM)) {
		REG_WRITE(DWMAC_MACCR, REG_READ(DWMAC_MACCR) | DWMAC_MACCR_IPCO);
	}

	return 0;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *dwmac_stats(const struct device *dev, struct net_if *iface __unused)
{
	struct dwmac_priv *p = dev->data;

	return &p->stats;
}
#endif

const struct ethernet_api dwmac_api = {
	.iface_api.init = dwmac_iface_init,
	.get_capabilities = dwmac_caps,
	.set_config = dwmac_set_config,
	.get_phy = dwmac_get_phy,
	.send = dwmac_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = dwmac_stats,
#endif
};
