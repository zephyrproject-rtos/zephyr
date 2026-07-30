/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NXP NETC Virtual Station Interface (VSI) Ethernet driver.
 *
 * The PSI owner manages the physical function (PF) and enables the VSI.
 * This driver only uses VSI-specific registers, which are accessible only
 * after SR-IOV has been enabled by the PSI owner.
 */

#define DT_DRV_COMPAT nxp_imx_netc_vsi

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_imx_eth_vsi);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <ethernet/eth_stats.h>

#include "../eth.h"
#include "eth_nxp_imx_netc_priv.h"
#include "eth_nxp_imx_netc_msg.h"
#include "fsl_netc_msg.h"

/*Retry MAC-filter updates for a short time until the PSI grants access. */
#define NETC_VSI_MAC_SET_RETRIES  10
#define NETC_VSI_MAC_SET_RETRY_MS 200

static void netc_eth_vsi_thread(void *arg1, void *unused1, void *unused2);

static void netc_eth_vsi_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct netc_eth_data *data = dev->data;
	const struct netc_eth_config *cfg = dev->config;

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	LOG_INF("VSI (SI idx %d) MAC: %02x:%02x:%02x:%02x:%02x:%02x", getSiIdx(cfg->si_idx),
		data->mac_addr[0], data->mac_addr[1], data->mac_addr[2], data->mac_addr[3],
		data->mac_addr[4], data->mac_addr[5]);

	ethernet_init(iface);

	/* Stay carrier-off until the service thread brings the SI up; no SI access here. */
	net_if_carrier_off(iface);

	/* Publish the interface, then start the service thread that drives the SI. */
	data->iface = iface;

	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(data->rx_thread_stack), netc_eth_vsi_thread,
			(void *)dev, NULL, NULL, K_PRIO_COOP(CONFIG_ETH_NXP_IMX_RX_THREAD_PRIO), 0,
			K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "netc_eth_vsi");
}

static int netc_eth_vsi_set_config(const struct device *dev, struct net_if *iface __unused,
				   enum ethernet_config_type type,
				   const struct ethernet_config *config);
static int netc_eth_vsi_set_mac_filter(const struct device *dev,
				       const struct ethernet_filter *filter);

static int netc_eth_vsi_set_config(const struct device *dev, struct net_if *iface __unused,
				   enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	struct netc_eth_data *data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		/* Only update the local MAC address; the PSI owner manages the HW filter. */
		net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				     NET_LINK_ETHERNET);
		break;
	case ETHERNET_CONFIG_TYPE_FILTER:
		return netc_eth_vsi_set_mac_filter(dev, &config->filter);
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		return netc_vsi_msg_set_mac_promisc(dev,
						NETC_MAC_FILTER_TYPE_UC | NETC_MAC_FILTER_TYPE_MC,
						config->promisc_mode);
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int netc_eth_vsi_si_bringup(const struct device *dev)
{
	const struct netc_eth_config *config = dev->config;
	struct netc_eth_data *data = dev->data;
	ep_handle_t *handle = &data->handle;
	netc_rx_bdr_config_t rx_bdr_config = {0};
	netc_tx_bdr_config_t tx_bdr_config = {0};
	netc_bdr_config_t bdr_config = {0};
	uint8_t ring;

	config->bdr_init(&bdr_config, &rx_bdr_config, &tx_bdr_config);

	/* Read the SI capabilities from the PF. */
	NETC_EnetcGetCapability(handle->hw.base, &handle->capability);

	/* Rx BD ring group configuration (SI-scoped). */
	NETC_SISetRxBDRGroup(handle->hw.si, 0, 1);
	NETC_SISetDefaultRxBDRGroup(handle->hw.si, kNETC_SiBDRGroupOne);

	/* Configure the Tx BD ring(s) and record their bases in the handle. */
	for (ring = 0; ring < handle->cfg.txRingUse; ring++) {
		if (NETC_SIConfigTxBDR(handle->hw.si, ring, &bdr_config.txBdrConfig[ring]) !=
		    kStatus_Success) {
			LOG_ERR("Failed to configure VSI Tx BD ring %u", ring);
			return -ENOBUFS;
		}
		handle->txBdRing[ring].bdBase = bdr_config.txBdrConfig[ring].bdArray;
		handle->txBdRing[ring].dirtyBase = bdr_config.txBdrConfig[ring].dirtyArray;
		handle->txBdRing[ring].len = bdr_config.txBdrConfig[ring].len;
		/* Resync SW indices with the just-reset HW ring (needed on re-bring-up). */
		handle->txBdRing[ring].producerIndex = 0;
		handle->txBdRing[ring].cleanIndex = 0;
	}

	netc_eth_tx_coalesce_init(data);

	/* Configure the Rx BD ring(s) and record their bases in the handle. */
	for (ring = 0; ring < handle->cfg.rxRingUse; ring++) {
		if (NETC_SIConfigRxBDR(handle->hw.si, ring, &bdr_config.rxBdrConfig[ring]) !=
		    kStatus_Success) {
			LOG_ERR("Failed to configure VSI Rx BD ring %u", ring);
			return -ENOBUFS;
		}
		handle->rxBdRing[ring].extendDesc = bdr_config.rxBdrConfig[ring].extendDescEn;
		handle->rxBdRing[ring].bdBase = bdr_config.rxBdrConfig[ring].bdArray;
		handle->rxBdRing[ring].len = bdr_config.rxBdrConfig[ring].len;
		handle->rxBdRing[ring].buffSize = bdr_config.rxBdrConfig[ring].buffSize;
		/* Resync the SW read index with the just-reset HW ring. */
		handle->rxBdRing[ring].index = 0;
	}

	/* Attach an Rx buffer to each descriptor. */
	for (ring = 0; ring < handle->cfg.rxRingUse; ring++) {
		const netc_rx_bdr_config_t *rc = &bdr_config.rxBdrConfig[ring];
		netc_rx_bd_t *rx_desc = &rc->bdArray[0];

		handle->rxBdRing[ring].buffArray = rc->buffAddrArray;
		for (uint16_t idx = 0; idx < rc->len; idx++) {
			(void)memset(rx_desc, 0, sizeof(netc_rx_bd_t));
			rx_desc->standard.addr = (uint64_t)MEMORY_ConvertMemoryMapAddress(
				(uintptr_t)handle->rxBdRing[ring].buffArray[idx],
				kMEMORY_Local2DMA);
			rx_desc++;
		}
	}

	/* Enable the Rx ring(s). */
	for (ring = 0; ring < handle->cfg.rxRingUse; ring++) {
		NETC_SIRxRingEnable(handle->hw.si, ring, true);
	}

	NETC_SIEnableVlanToIpv(handle->hw.si, false);

	/* Enable the SI; the PF-side enable is the PSI owner's. */
	NETC_SIEnable(handle->hw.si, true);

#ifdef CONFIG_ETH_NXP_IMX_MSGINTR
	netc_eth_msgintr_arm(dev);
#endif

    /* Enable PSI-to-VSI message interrupts for asynchronous link updates. */
	handle->hw.si->SIMSIVR = NETC_MSG_MSIX_ENTRY_IDX & ENETC_SI_SIMSIVR_VECTOR_MASK;
	EP_VsiEnableInterrupt(handle, kNETC_VsiMsgRxFlag, true);

	return 0;
}

/* ENETC multicast hash index (0..63). */
static uint8_t netc_eth_vsi_mac_hash_idx(const uint8_t *addr)
{
	uint64_t ether = 0;
	uint64_t fold;
	uint64_t mask = 0;
	uint8_t res = 0;

	for (int i = 0; i < 6; i++) {
		ether = (ether << 8) | addr[i];
	}
	fold = __builtin_bswap64(ether) >> 16;
	for (int i = 0; i < 8; i++) {
		mask |= BIT64(i * 6);
	}
	for (int i = 0; i < 6; i++) {
		res |= (uint8_t)(__builtin_popcountll(fold & (mask << i)) & 0x1U) << i;
	}

	return res;
}

/*
 * Add or remove a multicast destination MAC from the VSI hash filter. Called
 * by the Ethernet L2 multicast monitor on every group join/leave. Reference
 * counting per hash index keeps a bit set while any group still maps to it.
 */
static int netc_eth_vsi_set_mac_filter(const struct device *dev,
				       const struct ethernet_filter *filter)
{
	struct netc_eth_data *data = dev->data;
	uint8_t idx;
	int ret = 0;

	if (filter->type != ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS) {
		return -ENOTSUP;
	}

	idx = netc_eth_vsi_mac_hash_idx(filter->mac_address.addr);

	k_mutex_lock(&data->msg_lock, K_FOREVER);

	if (filter->set) {
		if (data->mc_hash_refcnt[idx] == UINT8_MAX) {
			k_mutex_unlock(&data->msg_lock);
			return -ENOSPC;
		}
		data->mc_hash_refcnt[idx]++;
	} else if (data->mc_hash_refcnt[idx] > 0U) {
		data->mc_hash_refcnt[idx]--;
	}

	if (data->mc_hash_refcnt[idx] > 0U) {
		data->mc_hash |= BIT64(idx);
	} else {
		data->mc_hash &= ~BIT64(idx);
	}

	/* If the SI is not up yet, the table is programmed at bring-up. */
	if (data->si_ready) {
		ret = netc_vsi_msg_set_mac_hash(dev);
	}

	k_mutex_unlock(&data->msg_lock);

	return ret;
}

#if defined(CONFIG_NET_VLAN)
/* ENETC VLAN hash index (0..63): fold the two 6-bit halves of the VID. */
static uint8_t netc_eth_vsi_vlan_hash_idx(uint16_t tag)
{
	return (uint8_t)(((tag >> 6) & 0x3fU) ^ (tag & 0x3fU));
}

/*
 * Add or remove a VLAN from the VSI hash filter. Called by the Ethernet L2
 * on every VLAN enable/disable. Reference counting per hash index keeps a bit
 * set while any active VLAN still maps to it.
 */
static int netc_eth_vsi_vlan_setup(const struct device *dev, struct net_if *iface __unused,
				   uint16_t tag, bool enable)
{
	struct netc_eth_data *data = dev->data;
	uint8_t idx = netc_eth_vsi_vlan_hash_idx(tag);
	int ret = 0;

	k_mutex_lock(&data->msg_lock, K_FOREVER);

	if (enable) {
		if (data->vlan_hash_refcnt[idx] == UINT8_MAX) {
			k_mutex_unlock(&data->msg_lock);
			return -ENOSPC;
		}
		data->vlan_hash_refcnt[idx]++;
	} else if (data->vlan_hash_refcnt[idx] > 0U) {
		data->vlan_hash_refcnt[idx]--;
	}

	if (data->vlan_hash_refcnt[idx] > 0U) {
		data->vlan_hash |= BIT64(idx);
	} else {
		data->vlan_hash &= ~BIT64(idx);
	}

	/* If the SI is not up yet, the table is programmed at bring-up. */
	if (data->si_ready) {
		ret = netc_vsi_msg_set_vlan_hash(dev);
	}

	k_mutex_unlock(&data->msg_lock);

	return ret;
}
#endif /* CONFIG_NET_VLAN */

/*
 * Ask the PSI to program this VSI's primary MAC address, retrying briefly
 * while the SI comes up.
 */
static void netc_eth_vsi_program_mac(const struct device *dev)
{
	for (int attempt = 0; attempt < NETC_VSI_MAC_SET_RETRIES; attempt++) {
		if (netc_vsi_msg_set_primary_mac(dev) == 0) {
			LOG_DBG("VSI primary MAC filter programmed via PSI");
			return;
		}
		k_msleep(NETC_VSI_MAC_SET_RETRY_MS);
	}
	LOG_WRN("VSI MAC filter not accepted by PSI; unicast RX needs PSI-side steering");
}

/* Handle link-status messages from the PSI and update the carrier state. */
void netc_eth_vsi_msg_isr(const struct device *dev)
{
	struct netc_eth_data *data = dev->data;

	if (data->iface == NULL) {
		return;
	}

	switch (netc_vsi_msg_poll_link(dev)) {
	case NETC_MSG_LINK_UP:
		/* Link-up: wake the service thread to (re)program the PSI. */
		data->link_notify = true;
		k_sem_give(&data->rx_sem);
		break;
	case NETC_MSG_LINK_DOWN:
		/* Link-down: gate off Tx and drop the carrier. */
		data->si_ready = false;
		net_eth_carrier_off(data->iface);
		break;
	default:
		break;
	}
}

/* True once the PSI owner has enabled SR-IOV for our ENETC. */
static inline bool netc_eth_vsi_vf_ready(const struct netc_eth_config *config)
{
	static const uintptr_t pf_pci_base[] = ENETC_PCI_TYPE0_BASE_ADDRS;
	ENETC_PCI_TYPE0_Type *pf =
		(ENETC_PCI_TYPE0_Type *)pf_pci_base[1U + getSiInstance(config->si_idx)];
	const uint16_t vf_ready = ENETC_PCI_TYPE0_PCIE_CFC_SRIOV_CTL_VF_ENABLE_MASK |
				  ENETC_PCI_TYPE0_PCIE_CFC_SRIOV_CTL_VF_MSE_MASK;

	return (pf->PCIE_CFC_SRIOV_CTL & vf_ready) == vf_ready;
}

/* Wait until the SI is ready and can be accessed safely.  */
static void netc_eth_vsi_wait_ready(const struct device *dev)
{
	const struct netc_eth_config *config = dev->config;

	if (config->ready_clock_dev != NULL) {
		if (!device_is_ready(config->ready_clock_dev)) {
			LOG_DBG("VSI readiness-gate clock not ready; skipping clock gate");
		} else {
			while (clock_control_get_status(config->ready_clock_dev,
							config->ready_clock_subsys) !=
			       CLOCK_CONTROL_STATUS_ON) {
				k_msleep(CONFIG_ETH_NXP_IMX_NETC_VSI_READY_POLL_INTERVAL_MS);
			}
		}
	}

	while (!netc_eth_vsi_vf_ready(config)) {
		k_msleep(CONFIG_ETH_NXP_IMX_NETC_VSI_READY_POLL_INTERVAL_MS);
	}
}

/*
 * Send the VSI traffic settings to the PSI. This includes the main unicast
 * MAC address, and the multicast and VLAN RX hash filters.
 */
static void netc_eth_vsi_program_psi(const struct device *dev)
{
	struct netc_eth_data *data = dev->data;

	/* Set our MAC address in the PSI. */
	netc_eth_vsi_program_mac(dev);

	/*
	 * Mark the SI as ready and update the multicast/VLAN hash tables together,
	 * so no joins are missed during the change.
	 */
	k_mutex_lock(&data->msg_lock, K_FOREVER);
	data->si_ready = true;
	if (netc_vsi_msg_set_mac_hash(dev) != 0) {
		LOG_WRN("VSI multicast hash not programmed; multicast RX may be limited");
	}
#if defined(CONFIG_NET_VLAN)
	if (netc_vsi_msg_set_vlan_hash(dev) != 0) {
		LOG_WRN("VSI VLAN hash not programmed; VLAN RX may be limited");
	}
#endif
	k_mutex_unlock(&data->msg_lock);
}

/*
 * Full SI bring-up: wait for SR-IOV, bring the SI up, set up PSI steering,
 * subscribe to link status notifications, and enable the carrier.
 */
static void netc_eth_vsi_bring_up(const struct device *dev)
{
	struct netc_eth_data *data = dev->data;
	int ret;

	netc_eth_vsi_wait_ready(dev);
	do {
		ret = netc_eth_vsi_si_bringup(dev);
		if (ret != 0) {
			LOG_ERR("VSI SI bring-up failed (%d); retrying", ret);
			k_msleep(CONFIG_ETH_NXP_IMX_NETC_VSI_READY_POLL_INTERVAL_MS);
		}
	} while (ret != 0);

	netc_eth_vsi_program_psi(dev);

	/* Subscribe to link-status notifications once per SI */
	if (netc_vsi_msg_enable_link_notify(dev) == 0) {
		LOG_INF("VSI ready; carrier follows PSI link status");
	} else {
		LOG_WRN("VSI link-status notifications unavailable; forcing carrier up");
		net_eth_carrier_on(data->iface);
		LOG_INF("VSI ready");
	}
}

/*
 * Single VSI service thread: manages the SI and handles Rx. It waits on
 * rx_sem and is woken by an Rx IRQ, a PSI link-up notification
 * (link_notify), or a timeout. On each wake-up it:
 *   1. Checks the SR-IOV VF-enable bit. If the VF is disabled, it drops the
 *      carrier and waits. If the VF is enabled again, it brings the SI up
 *      and performs the full setup.
 *   2. Re-sends the PSI steering after a link-up notification, because a
 *      PSI restart can lose our MAC and filter settings.
 *   3. Receives data from the Rx ring while the SI is up.
 */
static void netc_eth_vsi_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	const struct netc_eth_config *config = dev->config;
	struct netc_eth_data *data = dev->data;
	bool up = true;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	netc_eth_vsi_bring_up(dev);

	for (;;) {
		int work;
		bool ready;

		k_sem_take(&data->rx_sem,
			   K_MSEC(CONFIG_ETH_NXP_IMX_NETC_VSI_READY_POLL_INTERVAL_MS));
		ready = netc_eth_vsi_vf_ready(config);

		if (up && !ready) {
			/* SR-IOV torn down: the SI is gone until it returns. */
			k_mutex_lock(&data->msg_lock, K_FOREVER);
			data->si_ready = false;
			k_mutex_unlock(&data->msg_lock);
			net_eth_carrier_off(data->iface);
			LOG_INF("VSI down; awaiting SR-IOV re-enable");
			up = false;
		} else if (!up && ready) {
			/* SR-IOV back: the SI was reset, so redo the full bring-up. */
			netc_eth_vsi_bring_up(dev);
			up = true;
		}

		if (!up) {
			/* SI torn down (SR-IOV gone): nothing to drive until it returns. */
			continue;
		}

		if (data->link_notify) {
			/*
			 * Link-up notification: re-send the steering settings to restore si_ready.
			 */
			data->link_notify = false;
			netc_eth_vsi_program_psi(dev);
			net_eth_carrier_on(data->iface);
			LOG_INF("VSI link-up; PSI reprogrammed");
		}

		if (!data->si_ready) {
			/* Graceful link-down, no link-up yet: idle until it returns. */
			continue;
		}

		work = 0;
		while (netc_eth_rx(dev) != -ENOBUFS) {
			if (++work == CONFIG_ETH_NXP_IMX_RX_BUDGET) {
				/* more work to do, reschedule */
				work = 0;
				k_yield();
			}
		}
	}
}

/* Boot-time init: software handle setup only (no MMIO), then spawn the threads. */
static int netc_eth_vsi_init(const struct device *dev)
{
	const struct netc_eth_config *config = dev->config;
	struct netc_eth_data *data = dev->data;
	ep_handle_t *handle = &data->handle;

	/* No-op without an MMU, but kept for parity with the PSI driver. */
	DEVICE_MMIO_NAMED_MAP(dev, port, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	DEVICE_MMIO_NAMED_MAP(dev, vfconfig, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	(void)memset(handle, 0, sizeof(*handle));

	NETC_SocGetBaseResource(&handle->hw, (netc_hw_si_idx_t)config->si_idx);

	/* Fill the handle config the driver needs to drive the SI datapath. */
	handle->cfg.si = (netc_hw_si_idx_t)config->si_idx;
	handle->cfg.rxRingUse = 1;
	handle->cfg.txRingUse = 1;
	handle->cfg.rxBdrGroupNum = 0;
	handle->cfg.ringPerBdrGroup = 1;
	/* Buffers/BD rings are non-cacheable; skip HAL cache ops (DSB still runs). */
	handle->cfg.rxCacheMaintain = false;
	handle->cfg.txCacheMaintain = false;
	handle->cfg.rxZeroCopy = false;
	handle->cfg.entryNum = config->msix_entry_num;
	handle->cfg.userData = data;
	handle->cfg.reclaimCallback = NULL;

	if (net_eth_mac_load(&config->mac_config, data->mac_addr) == -ENODATA) {
		LOG_WRN("No MAC address in the device tree; set local-mac-address or "
			"zephyr,random-mac-address");
	}

	data->si_ready = false;
	k_mutex_init(&data->msg_lock);
	k_sem_init(&data->tx_sem, 0, 1);
	k_sem_init(&data->rx_sem, 0, 1);

	return 0;
}

static int netc_eth_vsi_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct netc_eth_data *data = dev->data;
	const struct netc_eth_config *config = dev->config;

	/*
	 * Do not transmit unless the SI is up. If the VF is disabled, wake the
	 * service thread so it can handle the carrier-down state immediately.
	 */
	if (!data->si_ready) {
		return -ENETDOWN;
	}
	if (!netc_eth_vsi_vf_ready(config)) {
		k_sem_give(&data->rx_sem);
		return -ENETDOWN;
	}

	return netc_eth_tx(dev, pkt);
}

static enum ethernet_hw_caps netc_eth_vsi_get_capabilities(const struct device *dev,
							   struct net_if *iface)
{
	/* The VSI programs a multicast hash filter over the PSI mailbox. */
	return netc_eth_get_capabilities(dev, iface) | ETHERNET_HW_FILTERING;
}

static const struct ethernet_api netc_eth_vsi_api = {
	.iface_api.init = netc_eth_vsi_iface_init,
	.get_capabilities = netc_eth_vsi_get_capabilities,
	.set_config = netc_eth_vsi_set_config,
#if defined(CONFIG_NET_VLAN)
	.vlan_setup = netc_eth_vsi_vlan_setup,
#endif
	.send = netc_eth_vsi_tx,
};

#define NETC_VSI_INSTANCE_DEFINE(n)                                                                \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static uint8_t eth_vsi##n##_tx_buff[CONFIG_ETH_NXP_IMX_TX_RING_LEN]                \
						   [CONFIG_ETH_NXP_IMX_TX_RING_BUF_SIZE],         \
		NETC_BUFF_ALIGN);                                                                  \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static uint8_t eth_vsi##n##_msg_buff[NETC_MSG_VSI_BUF_SIZE], NETC_BUFF_ALIGN);     \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static netc_tx_bd_t eth_vsi##n##_txbd_array[CONFIG_ETH_NXP_IMX_TX_RING_NUM]        \
							   [CONFIG_ETH_NXP_IMX_TX_RING_LEN],       \
		NETC_BD_ALIGN);                                                                    \
	static netc_tx_frame_info_t eth_vsi##n##_txdirty_array[CONFIG_ETH_NXP_IMX_TX_RING_NUM]     \
							      [CONFIG_ETH_NXP_IMX_TX_RING_LEN];    \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static rx_buffer_t eth_vsi##n##_rx_buff[CONFIG_ETH_NXP_IMX_RX_RING_NUM]            \
						       [CONFIG_ETH_NXP_IMX_RX_RING_LEN],           \
		NETC_BUFF_ALIGN);                                                                  \
	static uint64_t eth_vsi##n##_rx_buff_addr_array[CONFIG_ETH_NXP_IMX_RX_RING_NUM]            \
						       [CONFIG_ETH_NXP_IMX_RX_RING_LEN];           \
	AT_NONCACHEABLE_SECTION(                                                                   \
		static uint8_t eth_vsi##n##_rx_frame[NETC_RX_RING_BUF_SIZE_ALIGN]);                \
	AT_NONCACHEABLE_SECTION_ALIGN(                                                             \
		static netc_rx_bd_t eth_vsi##n##_rxbd_array[CONFIG_ETH_NXP_IMX_RX_RING_NUM]        \
							   [CONFIG_ETH_NXP_IMX_RX_RING_LEN],       \
		NETC_BD_ALIGN);                                                                    \
	static void netc_eth_vsi##n##_bdr_init(netc_bdr_config_t *bdr_config,                      \
					       netc_rx_bdr_config_t *rx_bdr_config,                \
					       netc_tx_bdr_config_t *tx_bdr_config)                \
	{                                                                                          \
		for (uint8_t ring = 0U; ring < CONFIG_ETH_NXP_IMX_RX_RING_NUM; ring++) {           \
			for (uint8_t bd = 0U; bd < CONFIG_ETH_NXP_IMX_RX_RING_LEN; bd++) {         \
				eth_vsi##n##_rx_buff_addr_array[ring][bd] =                        \
					(uint64_t)(uintptr_t)&eth_vsi##n##_rx_buff[ring][bd];      \
			}                                                                          \
		}                                                                                  \
		bdr_config->rxBdrConfig = rx_bdr_config;                                           \
		bdr_config->txBdrConfig = tx_bdr_config;                                           \
		bdr_config->rxBdrConfig[0].bdArray = &eth_vsi##n##_rxbd_array[0][0];               \
		bdr_config->rxBdrConfig[0].len = CONFIG_ETH_NXP_IMX_RX_RING_LEN;                   \
		bdr_config->rxBdrConfig[0].buffAddrArray = &eth_vsi##n##_rx_buff_addr_array[0][0]; \
		bdr_config->rxBdrConfig[0].buffSize = NETC_RX_RING_BUF_SIZE_ALIGN;                 \
		bdr_config->rxBdrConfig[0].msixEntryIdx = NETC_RX_MSIX_ENTRY_IDX;                  \
		bdr_config->rxBdrConfig[0].extendDescEn = false;                                   \
		bdr_config->rxBdrConfig[0].enThresIntr = true;                                     \
		bdr_config->rxBdrConfig[0].enCoalIntr = true;                                      \
		bdr_config->rxBdrConfig[0].intrThreshold = 1;                                      \
		bdr_config->txBdrConfig[0].bdArray = &eth_vsi##n##_txbd_array[0][0];               \
		bdr_config->txBdrConfig[0].len = CONFIG_ETH_NXP_IMX_TX_RING_LEN;                   \
		bdr_config->txBdrConfig[0].dirtyArray = &eth_vsi##n##_txdirty_array[0][0];         \
		bdr_config->txBdrConfig[0].msixEntryIdx = NETC_TX_MSIX_ENTRY_IDX;                  \
		bdr_config->txBdrConfig[0].enIntr = false;                                         \
		bdr_config->txBdrConfig[0].enThresIntr = true;                                     \
	}                                                                                          \
	static struct netc_eth_data netc_eth_vsi##n##_data = {                                     \
		.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),                            \
		.tx_buff = eth_vsi##n##_tx_buff,                                                   \
		.msg_buff = eth_vsi##n##_msg_buff,                                                 \
		.rx_frame = eth_vsi##n##_rx_frame,                                                 \
	};                                                                                         \
	static const struct netc_eth_config netc_eth_vsi##n##_config = {                           \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(port, DT_DRV_INST(n)),                          \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(vfconfig, DT_DRV_INST(n)),                      \
		.mac_config = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                                  \
		.bdr_init = netc_eth_vsi##n##_bdr_init,                                            \
		.si_idx = (DT_INST_PROP(n, mac_index) << 8) | DT_INST_PROP(n, si_index),           \
		.is_vsi = true,                                                                    \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, clocks),                                       \
			(.ready_clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                 \
			.ready_clock_subsys =                                                      \
				(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),))            \
		.msix_entry_num = NETC_MSIX_VSI_EVENTS_COUNT,                                     \
		IF_ENABLED(CONFIG_ETH_NXP_IMX_NETC_MSI_GIC,                                        \
			(.msi_device_id = DT_INST_PROP_OR(n, msi_device_id, 0),                    \
			.msi_dev = (COND_CODE_1(DT_NODE_HAS_PROP(DT_INST_PARENT(n), msi_parent),   \
			       (DEVICE_DT_GET(DT_PHANDLE(DT_INST_PARENT(n), msi_parent))), NULL)), \
			))                                                                         \
		IF_DISABLED(CONFIG_ETH_NXP_IMX_NETC_MSI_GIC,                                       \
			(.tx_intr_msg_data = NETC_TX_INTR_MSG_DATA_START + n,                      \
			.rx_intr_msg_data = NETC_RX_INTR_MSG_DATA_START + n,                       \
			.msg_intr_msg_data = NETC_MSG_INTR_MSG_DATA_START + n,))                   \
	};                                                                                         \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, netc_eth_vsi_init, NULL, &netc_eth_vsi##n##_data,         \
				      &netc_eth_vsi##n##_config,                                   \
				      CONFIG_ETH_INIT_PRIORITY,                                    \
				      &netc_eth_vsi_api, NET_ETH_MTU);
DT_INST_FOREACH_STATUS_OKAY(NETC_VSI_INSTANCE_DEFINE)
