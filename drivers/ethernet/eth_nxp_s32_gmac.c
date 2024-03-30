/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_gmac

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_eth, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <ethernet/eth_stats.h>
#include <soc.h>

#include <Gmac_Ip.h>
#include <Gmac_Ip_Hw_Access.h>
#include <Gmac_Ip_Irq.h>
#include <Clock_Ip.h>

#include "eth.h"

#define ETH_NXP_S32_BUF_TIMEOUT		K_MSEC(20)
#define ETH_NXP_S32_DMA_TX_TIMEOUT	K_MSEC(20)

#define ETH_NXP_S32_MAC_ADDR_LEN 6U

#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

struct eth_nxp_s32_config {
	uint8_t instance;
	uint8_t tx_ring_idx;
	uint8_t rx_ring_idx;
	uint32_t rx_irq;
	uint32_t tx_irq;
	void (*do_config)(void);
	const struct pinctrl_dev_config *pincfg;
	const struct device *phy_dev;

	const Gmac_CtrlConfigType ctrl_cfg;
	GMAC_Type *base;
};

struct eth_nxp_s32_data {
	struct net_if *iface;
	uint8_t mac_addr[ETH_NXP_S32_MAC_ADDR_LEN];
	uint8_t	if_suspended;
	struct k_mutex tx_mutex;
	struct k_sem rx_sem;
	struct k_sem tx_sem;
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_NXP_S32_RX_THREAD_STACK_SIZE);
};

static void eth_nxp_s32_rx_thread(void *arg1, void *unused1, void *unused2);

static inline struct net_if *get_iface(struct eth_nxp_s32_data *ctx, uint16_t vlan_tag)
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

static void convert_phy_to_mac_config(Gmac_Ip_ConfigType *gmac_cfg, enum phy_link_speed phy_speed)
{
	switch (phy_speed) {
	case LINK_HALF_10BASE_T:
		gmac_cfg->Speed = GMAC_SPEED_10M;
		gmac_cfg->Duplex = GMAC_HALF_DUPLEX;
		break;
	case LINK_FULL_10BASE_T:
		gmac_cfg->Speed = GMAC_SPEED_10M;
		gmac_cfg->Duplex = GMAC_FULL_DUPLEX;
		break;
	case LINK_HALF_100BASE_T:
		gmac_cfg->Speed = GMAC_SPEED_100M;
		gmac_cfg->Duplex = GMAC_HALF_DUPLEX;
		break;
	case LINK_FULL_100BASE_T:
		gmac_cfg->Speed = GMAC_SPEED_100M;
		gmac_cfg->Duplex = GMAC_FULL_DUPLEX;
		break;
	case LINK_HALF_1000BASE_T:
		gmac_cfg->Speed = GMAC_SPEED_1G;
		gmac_cfg->Duplex = GMAC_HALF_DUPLEX;
		break;
	case LINK_FULL_1000BASE_T:
		__fallthrough;
	default:
		gmac_cfg->Speed = GMAC_SPEED_1G;
		gmac_cfg->Duplex = GMAC_FULL_DUPLEX;
		break;
	}
}

static void phy_link_state_changed(const struct device *pdev,
				   struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (struct device *)user_data;
	const struct eth_nxp_s32_config *cfg = dev->config;
	struct eth_nxp_s32_data *ctx = dev->data;
	Gmac_Ip_ConfigType gmac_cfg;

	ARG_UNUSED(pdev);

	if (state->is_up) {
		/* Porting phy link config to mac */
		convert_phy_to_mac_config(&gmac_cfg, state->speed);
		/* Set MAC configuration */
		Gmac_Ip_SetSpeed(cfg->instance, gmac_cfg.Speed);

		cfg->base->MAC_CONFIGURATION |= GMAC_MAC_CONFIGURATION_DM(gmac_cfg.Duplex);

		/* net iface should be down even if PHY link state is up
		 * till the upper network layers have suspended the iface.
		 */
		if (ctx->if_suspended) {
			return;
		}

		LOG_DBG("Link up");
		net_eth_carrier_on(ctx->iface);
	} else {
		LOG_DBG("Link down");
		net_eth_carrier_off(ctx->iface);
	}
}

#if defined(CONFIG_SOC_SERIES_S32K3)
static int select_phy_interface(Gmac_Ip_MiiModeType mode)
{
	uint32_t regval;

	switch (mode) {
	case GMAC_MII_MODE:
		regval = DCM_GPR_DCMRWF1_EMAC_CONF_SEL(0U);
		break;
	case GMAC_RMII_MODE:
		regval = DCM_GPR_DCMRWF1_EMAC_CONF_SEL(2U);
		break;
#if (FEATURE_GMAC_RGMII_EN == 1U)
	case GMAC_RGMII_MODE:
		regval = DCM_GPR_DCMRWF1_EMAC_CONF_SEL(1U);
		break;
#endif
	default:
		return -EINVAL;
	}

	IP_DCM_GPR->DCMRWF1 = (IP_DCM_GPR->DCMRWF1 & ~DCM_GPR_DCMRWF1_EMAC_CONF_SEL_MASK) | regval;

	return 0;
}
#else
#error "SoC not supported"
#endif /* CONFIG_SOC_SERIES_S32K3 */

static int eth_nxp_s32_init(const struct device *dev)
{
	const struct eth_nxp_s32_config *cfg = dev->config;
	struct eth_nxp_s32_data *ctx = dev->data;
	Gmac_Ip_StatusType mac_status;
	Clock_Ip_StatusType clk_status;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	/*
	 * Currently, clock control shim driver does not support configuring clock
	 * muxes individually, so use the HAL directly.
	 */
	clk_status = Clock_Ip_Init(&Clock_Ip_aClockConfig[CONFIG_ETH_NXP_S32_CLOCK_CONFIG_IDX]);
	if (clk_status != CLOCK_IP_SUCCESS) {
		LOG_ERR("Failed to configure clocks (%d)", clk_status);
		return -EIO;
	}

	/*
	 * PHY mode selection must be done before the controller is reset,
	 * because the interface type is latched at controller's reset
	 */
	err = select_phy_interface(cfg->ctrl_cfg.Gmac_pCtrlConfig->MiiMode);
	if (err != 0) {
		LOG_ERR("Failed to select PHY interface (%d)", err);
		return -EIO;
	}

	mac_status = Gmac_Ip_Init(cfg->instance, &cfg->ctrl_cfg);
	if (mac_status != GMAC_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize GMAC%d (%d)", cfg->instance, mac_status);
		return -EIO;
	}

	k_mutex_init(&ctx->tx_mutex);
	k_sem_init(&ctx->rx_sem, 0, 1);
	k_sem_init(&ctx->tx_sem, 0, 1);

	k_thread_create(&ctx->rx_thread, ctx->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(ctx->rx_thread_stack),
			eth_nxp_s32_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_NXP_S32_RX_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&ctx->rx_thread, "eth_nxp_s32_rx");

	if (cfg->do_config != NULL) {
		cfg->do_config();
	}

	return 0;
}

static int eth_nxp_s32_start(const struct device *dev)
{
	const struct eth_nxp_s32_config *cfg = dev->config;
	struct eth_nxp_s32_data *ctx = dev->data;
	struct phy_link_state state;

	Gmac_Ip_EnableController(cfg->instance);

	irq_enable(cfg->rx_irq);
	irq_enable(cfg->tx_irq);

	/* If upper layers enable the net iface then mark it as
	 * not suspended so that PHY Link changes can have the impact
	 */
	ctx->if_suspended = false;

	if (cfg->phy_dev) {
		phy_get_link_state(cfg->phy_dev, &state);

		/* Enable net_iface only when Ethernet PHY link is up or else
		 * if net_iface is enabled when link is down and tx happens
		 * in this state then the used tx buffers will never be recovered back.
		 */
		if (state.is_up == true) {
			net_eth_carrier_on(ctx->iface);
		}
	} else {
		net_eth_carrier_on(ctx->iface);
	}

	LOG_DBG("GMAC%d started", cfg->instance);

	return 0;
}

static int eth_nxp_s32_stop(const struct device *dev)
{
	const struct eth_nxp_s32_config *cfg = dev->config;
	struct eth_nxp_s32_data *ctx = dev->data;
	Gmac_Ip_StatusType status;
	int err = 0;

	irq_disable(cfg->rx_irq);
	irq_disable(cfg->tx_irq);

	/* If upper layers disable the net iface then mark it as suspended
	 * in order to save it from the PHY link state changes
	 */
	ctx->if_suspended = true;

	net_eth_carrier_off(ctx->iface);

	status = Gmac_Ip_DisableController(cfg->instance);
	if (status != GMAC_STATUS_SUCCESS) {
		LOG_ERR("Failed to disable controller GMAC%d (%d)", cfg->instance, status);
		err = -EIO;
	}

	LOG_DBG("GMAC%d stopped", cfg->instance);

	return err;
}

static void eth_nxp_s32_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_nxp_s32_config *cfg = dev->config;
	struct eth_nxp_s32_data *ctx = dev->data;

	/* For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (ctx->iface == NULL) {
		ctx->iface = iface;
	}

	ethernet_init(iface);

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);

	LOG_INF("GMAC%d MAC address %02x:%02x:%02x:%02x:%02x:%02x", cfg->instance,
		ctx->mac_addr[0], ctx->mac_addr[1], ctx->mac_addr[2],
		ctx->mac_addr[3], ctx->mac_addr[4], ctx->mac_addr[5]);

	/* Make sure that the net iface state is not suspended unless
	 * upper layers explicitly stop the iface
	 */
	ctx->if_suspended = false;

	/* No PHY available, link is always up and MAC speed/duplex settings are fixed */
	if (cfg->phy_dev == NULL) {
		net_if_carrier_on(iface);
		return;
	}

	/*
	 * GMAC controls the PHY. If PHY is configured either as fixed
	 * link or autoneg, the callback is executed at least once
	 * immediately after setting it.
	 */
	if (!device_is_ready(cfg->phy_dev)) {
		LOG_ERR("PHY device (%p) is not ready, cannot init iface",
			cfg->phy_dev);
		return;
	}

	phy_link_callback_set(cfg->phy_dev, &phy_link_state_changed, (void *)dev);
}

static int eth_nxp_s32_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_nxp_s32_data *ctx = dev->data;
	const struct eth_nxp_s32_config *cfg = dev->config;
	size_t pkt_len = net_pkt_get_len(pkt);
	int res = 0;
	Gmac_Ip_BufferType buf;
	Gmac_Ip_TxInfoType tx_info;
	Gmac_Ip_StatusType status;
	Gmac_Ip_TxOptionsType tx_options = {
		.NoInt = FALSE,
		.CrcPadIns = GMAC_CRC_AND_PAD_INSERTION,
		.ChecksumIns = GMAC_CHECKSUM_INSERTION_PROTO_PSEUDOH
	};

	__ASSERT(pkt, "Packet pointer is NULL");

	k_mutex_lock(&ctx->tx_mutex, K_FOREVER);
	k_sem_reset(&ctx->tx_sem);

	buf.Length = (uint16_t)pkt_len;
	buf.Data = NULL;
	status = Gmac_Ip_GetTxBuff(cfg->instance, cfg->tx_ring_idx, &buf, NULL);
	if (status != GMAC_STATUS_SUCCESS) {
		LOG_ERR("Failed to get tx buffer (%d)", status);
		res = -ENOBUFS;
		goto error;
	}

	res = net_pkt_read(pkt, buf.Data, pkt_len);
	if (res) {
		LOG_ERR("Failed to copy packet to tx buffer (%d)", res);
		res = -ENOBUFS;
		goto error;
	}

	buf.Length = (uint16_t)pkt_len;
	status = Gmac_Ip_SendFrame(cfg->instance, cfg->tx_ring_idx, &buf, &tx_options);
	if (status != GMAC_STATUS_SUCCESS) {
		LOG_ERR("Failed to tx frame (%d)", status);
		res = -EIO;
		goto error;
	}

	/* Wait for the transmission to complete */
	if (k_sem_take(&ctx->tx_sem, ETH_NXP_S32_DMA_TX_TIMEOUT) != 0) {
		LOG_ERR("Timeout transmitting frame");
		res = -EIO;
		goto error;
	}

	/* Restore the buffer address pointer and clear the descriptor after the status is read */
	status = Gmac_Ip_GetTransmitStatus(cfg->instance, cfg->tx_ring_idx, &buf, &tx_info);
	if (status != GMAC_STATUS_SUCCESS) {
		LOG_ERR("Failed to restore tx buffer: %s (%d) ",
			(status == GMAC_STATUS_BUSY ? "busy" : "buf not found"), status);
		res = -EIO;
	} else if (tx_info.ErrMask != 0U) {
		LOG_ERR("Tx frame has errors (error mask 0x%X)", tx_info.ErrMask);
		res = -EIO;
	}

error:
	k_mutex_unlock(&ctx->tx_mutex);

	if (res != 0) {
		eth_stats_update_errors_tx(ctx->iface);
	}
	return res;
}

static struct net_pkt *eth_nxp_s32_get_pkt(const struct device *dev,
					Gmac_Ip_BufferType *buf,
					Gmac_Ip_RxInfoType *rx_info,
					uint16_t *vlan_tag)
{
	struct eth_nxp_s32_data *ctx = dev->data;
	struct net_pkt *pkt = NULL;
	int res = 0;
#if defined(CONFIG_NET_VLAN)
	struct net_eth_hdr *hdr;
	struct net_eth_vlan_hdr *hdr_vlan;
#if CONFIG_NET_TC_RX_COUNT > 1
	enum net_priority prio;
#endif /* CONFIG_NET_TC_RX_COUNT > 1 */
#endif /* CONFIG_NET_VLAN */

	/* Using root iface, it will be updated in net_recv_data() */
	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, rx_info->PktLen,
					   AF_UNSPEC, 0, ETH_NXP_S32_BUF_TIMEOUT);
	if (!pkt) {
		LOG_ERR("Failed to allocate rx buffer of length %u", rx_info->PktLen);
		goto exit;
	}

	res = net_pkt_write(pkt, buf->Data, rx_info->PktLen);
	if (res) {
		LOG_ERR("Failed to write rx frame into pkt buffer (%d)", res);
		net_pkt_unref(pkt);
		pkt = NULL;
		goto exit;
	}

#if defined(CONFIG_NET_VLAN)
	hdr = NET_ETH_HDR(pkt);
	if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
		hdr_vlan = (struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);
		net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
		*vlan_tag = net_pkt_vlan_tag(pkt);

#if CONFIG_NET_TC_RX_COUNT > 1
		prio = net_vlan2priority(net_pkt_vlan_priority(pkt));
		net_pkt_set_priority(pkt, prio);
#endif /* CONFIG_NET_TC_RX_COUNT > 1 */
	}
#endif /* CONFIG_NET_VLAN */

exit:
	if (!pkt) {
		eth_stats_update_errors_rx(get_iface(ctx, *vlan_tag));
	}

	return pkt;
}

static void eth_nxp_s32_rx(const struct device *dev)
{
	struct eth_nxp_s32_data *ctx = dev->data;
	const struct eth_nxp_s32_config *cfg = dev->config;
	uint16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct net_pkt *pkt;
	int res = 0;
	Gmac_Ip_RxInfoType rx_info = {0};
	Gmac_Ip_BufferType buf;
	Gmac_Ip_StatusType status;

	status = Gmac_Ip_ReadFrame(cfg->instance, cfg->rx_ring_idx, &buf, &rx_info);
	if (rx_info.ErrMask != 0U) {
		Gmac_Ip_ProvideRxBuff(cfg->instance, cfg->rx_ring_idx, &buf);
		LOG_ERR("Rx frame has errors (error mask 0x%X)", rx_info.ErrMask);
	} else if (status == GMAC_STATUS_SUCCESS) {
		pkt = eth_nxp_s32_get_pkt(dev, &buf, &rx_info, &vlan_tag);
		Gmac_Ip_ProvideRxBuff(cfg->instance, cfg->rx_ring_idx, &buf);
		if (pkt != NULL) {
			res = net_recv_data(get_iface(ctx, vlan_tag), pkt);
			if (res < 0) {
				eth_stats_update_errors_rx(get_iface(ctx, vlan_tag));
				net_pkt_unref(pkt);
				LOG_ERR("Failed to enqueue frame into rx queue (%d)", res);
			}
		}
	}
}

static void eth_nxp_s32_rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct eth_nxp_s32_data *ctx = dev->data;
	const struct eth_nxp_s32_config *cfg = dev->config;
	int res;
	int work;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	__ASSERT_NO_MSG(arg1 != NULL);
	__ASSERT_NO_MSG(ctx != NULL);

	while (1) {
		res = k_sem_take(&ctx->rx_sem, K_FOREVER);
		__ASSERT_NO_MSG(res == 0);

		work = 0;
		while (Gmac_Ip_IsFrameAvailable(cfg->instance, cfg->rx_ring_idx)) {
			eth_nxp_s32_rx(dev);
			if (++work == CONFIG_ETH_NXP_S32_RX_BUDGET) {
				/* More work to do, reschedule */
				work = 0;
				k_yield();
			}
		}

		/* All work done, re-enable rx interrupt and exit polling */
		irq_enable(cfg->rx_irq);

		/* In case a frame arrived after last eth_nxp_s32_rx() and before irq_enable() */
		if (Gmac_Ip_IsFrameAvailable(cfg->instance, cfg->rx_ring_idx)) {
			eth_nxp_s32_rx(dev);
		}
	}
}

static int eth_nxp_s32_set_config(const struct device *dev,
				  enum ethernet_config_type type,
				  const struct ethernet_config *config)
{
	struct eth_nxp_s32_data *ctx = dev->data;
	const struct eth_nxp_s32_config *cfg = dev->config;
	int res = 0;
	uint32_t regval;

	ARG_UNUSED(cfg);
	ARG_UNUSED(regval);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		/* Set new Ethernet MAC address and register it with the upper layer */
		memcpy(ctx->mac_addr, config->mac_address.addr, sizeof(ctx->mac_addr));
		Gmac_Ip_SetMacAddr(cfg->instance, (const uint8_t *)ctx->mac_addr);
		net_if_set_link_addr(ctx->iface, ctx->mac_addr, sizeof(ctx->mac_addr),
				     NET_LINK_ETHERNET);
		LOG_INF("MAC set to: %02x:%02x:%02x:%02x:%02x:%02x",
			ctx->mac_addr[0], ctx->mac_addr[1], ctx->mac_addr[2],
			ctx->mac_addr[3], ctx->mac_addr[4], ctx->mac_addr[5]);
		break;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		regval = cfg->base->MAC_PACKET_FILTER;
		if (config->promisc_mode && !(regval & GMAC_MAC_PACKET_FILTER_PR_MASK)) {
			cfg->base->MAC_PACKET_FILTER |= GMAC_MAC_PACKET_FILTER_PR_MASK;
		} else if (!config->promisc_mode && (regval & GMAC_MAC_PACKET_FILTER_PR_MASK)) {
			cfg->base->MAC_PACKET_FILTER &= ~GMAC_MAC_PACKET_FILTER_PR_MASK;
		} else {
			res = -EALREADY;
		}
		break;
#endif
#if defined(CONFIG_ETH_NXP_S32_MULTICAST_FILTER)
	case ETHERNET_HW_FILTERING:
		if (config->filter.set) {
			Gmac_Ip_AddDstAddrToHashFilter(cfg->instance,
						       config->filter.mac_address.addr);
		} else {
			Gmac_Ip_RemoveDstAddrFromHashFilter(cfg->instance,
							    config->filter.mac_address.addr);
		}
		break;
#endif
	default:
		res = -ENOTSUP;
		break;
	}

	return res;
}

static enum ethernet_hw_caps eth_nxp_s32_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (ETHERNET_LINK_10BASE_T
		| ETHERNET_LINK_100BASE_T
#if (FEATURE_GMAC_RGMII_EN == 1U)
		| ETHERNET_LINK_1000BASE_T
#endif
		| ETHERNET_DUPLEX_SET
		| ETHERNET_HW_TX_CHKSUM_OFFLOAD
		| ETHERNET_HW_RX_CHKSUM_OFFLOAD
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
#if defined(CONFIG_ETH_NXP_S32_MULTICAST_FILTER)
		| ETHERNET_HW_FILTERING
#endif
	);
}

static void eth_nxp_s32_tx_irq(const struct device *dev)
{
	const struct eth_nxp_s32_config *cfg = dev->config;

	GMAC_TxIRQHandler(cfg->instance, cfg->tx_ring_idx);
}

static void eth_nxp_s32_rx_irq(const struct device *dev)
{
	const struct eth_nxp_s32_config *cfg = dev->config;

	GMAC_RxIRQHandler(cfg->instance, cfg->rx_ring_idx);
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_nxp_s32_iface_init,
	.get_capabilities = eth_nxp_s32_get_capabilities,
	.start = eth_nxp_s32_start,
	.stop = eth_nxp_s32_stop,
	.send = eth_nxp_s32_tx,
	.set_config = eth_nxp_s32_set_config,
};


BUILD_ASSERT(((CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE * CONFIG_ETH_NXP_S32_RX_RING_LEN)
		% FEATURE_GMAC_MTL_RX_FIFO_BLOCK_SIZE) == 0,
		"CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE * CONFIG_ETH_NXP_S32_RX_RING_LEN "
		"must be multiple of RX FIFO block size");
BUILD_ASSERT(((CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE * CONFIG_ETH_NXP_S32_TX_RING_LEN)
		% FEATURE_GMAC_MTL_TX_FIFO_BLOCK_SIZE) == 0,
		"CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE * CONFIG_ETH_NXP_S32_TX_RING_LEN "
		"must be multiple of TX FIFO block size");
BUILD_ASSERT((CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE % FEATURE_GMAC_DATA_BUS_WIDTH_BYTES) == 0,
		"CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE must be multiple of the data bus width");
BUILD_ASSERT((CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE % FEATURE_GMAC_DATA_BUS_WIDTH_BYTES) == 0,
		"CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE must be multiple of the data bus width");

#define ETH_NXP_S32_FIXED_LINK_NODE(n)							\
	DT_INST_CHILD(n, fixed_link)

#define ETH_NXP_S32_IS_FIXED_LINK(n)							\
	DT_NODE_EXISTS(ETH_NXP_S32_FIXED_LINK_NODE(n))

#define ETH_NXP_S32_FIXED_LINK_SPEED(n)							\
	DT_PROP(ETH_NXP_S32_FIXED_LINK_NODE(n), speed)

#define ETH_NXP_S32_FIXED_LINK_FULL_DUPLEX(n)						\
	DT_PROP(ETH_NXP_S32_FIXED_LINK_NODE(n), full_duplex)

#define ETH_NXP_S32_MAC_SPEED(n)							\
	COND_CODE_1(ETH_NXP_S32_IS_FIXED_LINK(n),					\
		(_CONCAT(_CONCAT(GMAC_SPEED_, ETH_NXP_S32_FIXED_LINK_SPEED(n)), M)),	\
		(GMAC_SPEED_100M))

#define ETH_NXP_S32_MAC_DUPLEX(n)							\
	COND_CODE_1(ETH_NXP_S32_IS_FIXED_LINK(n),					\
		(COND_CODE_1(ETH_NXP_S32_FIXED_LINK_FULL_DUPLEX(n),			\
			(GMAC_FULL_DUPLEX), (GMAC_HALF_DUPLEX))),			\
		(GMAC_FULL_DUPLEX))

#define ETH_NXP_S32_MAC_MII(n)								\
	_CONCAT(_CONCAT(GMAC_, DT_INST_STRING_UPPER_TOKEN(n, phy_connection_type)), _MODE)

#define ETH_NXP_S32_IRQ_INIT(n, name)							\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, name, irq),					\
		DT_INST_IRQ_BY_NAME(n, name, priority),					\
		eth_nxp_s32_##name##_irq,						\
		DEVICE_DT_INST_GET(n),							\
		COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, flags),				\
			(DT_INST_IRQ_BY_NAME(n, name, flags)), (0)));

#define ETH_NXP_S32_INIT_CONFIG(n)							\
	static void eth_nxp_s32_init_config_##n(void)					\
	{										\
		const struct device *dev = DEVICE_DT_INST_GET(n);			\
		struct eth_nxp_s32_data *ctx = dev->data;				\
		const struct eth_nxp_s32_config *cfg = dev->config;			\
											\
		ETH_NXP_S32_IRQ_INIT(n, tx);						\
		ETH_NXP_S32_IRQ_INIT(n, rx);						\
											\
		COND_CODE_1(DT_INST_PROP(n, zephyr_random_mac_address), (		\
			gen_random_mac(ctx->mac_addr, FREESCALE_OUI_B0,			\
					FREESCALE_OUI_B1, FREESCALE_OUI_B2);		\
			Gmac_Ip_SetMacAddr(cfg->instance, ctx->mac_addr);		\
		), (									\
			Gmac_Ip_GetMacAddr(cfg->instance, ctx->mac_addr);		\
		))									\
	}

#define ETH_NXP_S32_RX_CALLBACK(n)							\
	static void eth_nxp_s32_rx_callback_##n(uint8_t inst, uint8_t chan)		\
	{										\
		const struct device *dev = DEVICE_DT_INST_GET(n);			\
		struct eth_nxp_s32_data *ctx = dev->data;				\
		const struct eth_nxp_s32_config *cfg = dev->config;			\
											\
		ARG_UNUSED(inst);							\
		ARG_UNUSED(chan);							\
											\
		/* Rx irq will be re-enabled from Rx thread */				\
		irq_disable(cfg->rx_irq);						\
		k_sem_give(&ctx->rx_sem);						\
	}

#define ETH_NXP_S32_TX_CALLBACK(n)							\
	static void eth_nxp_s32_tx_callback_##n(uint8_t inst, uint8_t chan)		\
	{										\
		const struct device *dev = DEVICE_DT_INST_GET(n);			\
		struct eth_nxp_s32_data *ctx = dev->data;				\
											\
		ARG_UNUSED(inst);							\
		ARG_UNUSED(chan);							\
											\
		k_sem_give(&ctx->tx_sem);						\
	}

#define _ETH_NXP_S32_RING(n, name, len, buf_size)					\
	static Gmac_Ip_BufferDescriptorType eth_nxp_s32_##name##ring_desc_##n[len]	\
		__nocache __aligned(FEATURE_GMAC_BUFFDESCR_ALIGNMENT_BYTES);		\
	static uint8_t eth_nxp_s32_##name##ring_buf_##n[len * buf_size]			\
		__nocache __aligned(FEATURE_GMAC_BUFF_ALIGNMENT_BYTES)

#define ETH_NXP_S32_RX_RING(n)								\
	_ETH_NXP_S32_RING(n, rx,							\
			  CONFIG_ETH_NXP_S32_RX_RING_LEN,				\
			  CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE)

#define ETH_NXP_S32_TX_RING(n)								\
	_ETH_NXP_S32_RING(n, tx,							\
			  CONFIG_ETH_NXP_S32_TX_RING_LEN,				\
			  CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE)

#define ETH_NXP_S32_MAC_TXTIMESHAPER_CONFIG(n)						\
	static const Gmac_Ip_TxTimeAwareShaper eth_nxp_s32_mac_txtimeshaper_config_##n = {\
		.GateControlList = NULL,						\
	}

#define ETH_NXP_S32_MAC_RXRING_CONFIG(n)						\
	static const Gmac_Ip_RxRingConfigType eth_nxp_s32_mac_rxring_config_##n = {	\
		.RingDesc = eth_nxp_s32_rxring_desc_##n,				\
		.Callback = eth_nxp_s32_rx_callback_##n,				\
		.Buffer = eth_nxp_s32_rxring_buf_##n,					\
		.Interrupts = (uint32_t)GMAC_CH_INTERRUPT_RI,				\
		.BufferLen = CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE,			\
		.RingSize = CONFIG_ETH_NXP_S32_RX_RING_LEN,				\
		.PriorityMask = 0U,							\
		.DmaBurstLength = 32U,							\
	}

#define ETH_NXP_S32_MAC_TXRING_CONFIG(n)						\
	static const Gmac_Ip_TxRingConfigType eth_nxp_s32_mac_txring_config_##n = {	\
		.Weight = 0U,								\
		.IdleSlopeCredit = 0U,							\
		.SendSlopeCredit = 0U,							\
		.HiCredit = 0U,								\
		.LoCredit = 0,								\
		.RingDesc = eth_nxp_s32_txring_desc_##n,				\
		.Callback = eth_nxp_s32_tx_callback_##n,				\
		.Buffer = eth_nxp_s32_txring_buf_##n,					\
		.Interrupts = (uint32_t)GMAC_CH_INTERRUPT_TI,				\
		.BufferLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
		.RingSize = CONFIG_ETH_NXP_S32_TX_RING_LEN,				\
		.PriorityMask = 0U,							\
		.DmaBurstLength = 32U,							\
		.QueueOpMode = GMAC_OP_MODE_DCB_GEN,					\
	}

#define ETH_NXP_S32_MAC_PKT_FILTER(n)							\
	((uint32_t)(0U									\
	COND_CODE_1(CONFIG_ETH_NXP_S32_MULTICAST_FILTER,				\
		(|GMAC_PKT_FILTER_HASH_MULTICAST),					\
		(|GMAC_PKT_FILTER_PASS_ALL_MULTICAST))					\
	))

#define ETH_NXP_S32_MAC_CONF(n)								\
	((uint32_t)(GMAC_MAC_CONFIG_CRC_STRIPPING					\
		| GMAC_MAC_CONFIG_AUTO_PAD						\
		| GMAC_MAC_CONFIG_CHECKSUM_OFFLOAD					\
	IF_ENABLED(CONFIG_ETH_NXP_S32_LOOPBACK,						\
		(|GMAC_MAC_CONFIG_LOOPBACK))						\
	))

#define ETH_NXP_S32_MAC_CONFIG(n)							\
	static const Gmac_Ip_ConfigType eth_nxp_s32_mac_config_##n = {			\
		.RxRingCount = 1U,							\
		.TxRingCount = 1U,							\
		.Interrupts = 0U,							\
		.Callback = NULL,							\
		.TxSchedAlgo = GMAC_SCHED_ALGO_SP,					\
		.MiiMode = ETH_NXP_S32_MAC_MII(n),					\
		.Speed = ETH_NXP_S32_MAC_SPEED(n),					\
		.Duplex = ETH_NXP_S32_MAC_DUPLEX(n),					\
		.MacConfig = ETH_NXP_S32_MAC_CONF(n),					\
		.MacPktFilterConfig = ETH_NXP_S32_MAC_PKT_FILTER(n),			\
		.EnableCtrl = false,							\
	}

#define ETH_NXP_S32_MAC_ADDR(n)								\
	BUILD_ASSERT(DT_INST_PROP(n, zephyr_random_mac_address) ||			\
		NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(n)),				\
		"eth_nxp_s32_gmac requires either a fixed or random MAC address");	\
	static const uint8_t eth_nxp_s32_mac_addr_##n[ETH_NXP_S32_MAC_ADDR_LEN] =	\
		DT_INST_PROP_OR(n, local_mac_address, {0U})

#define ETH_NXP_S32_MAC_STATE(n) Gmac_Ip_StateType eth_nxp_s32_mac_state_##n

#define ETH_NXP_S32_CTRL_CONFIG(n)							\
	{										\
		.Gmac_pCtrlState = &eth_nxp_s32_mac_state_##n,				\
		.Gmac_pCtrlConfig = &eth_nxp_s32_mac_config_##n,			\
		.Gmac_paCtrlRxRingConfig = &eth_nxp_s32_mac_rxring_config_##n,		\
		.Gmac_paCtrlTxRingConfig = &eth_nxp_s32_mac_txring_config_##n,		\
		.Gmac_pau8CtrlPhysAddr = &eth_nxp_s32_mac_addr_##n[0],			\
		.Gmac_pCtrlTxTimeAwareShaper = &eth_nxp_s32_mac_txtimeshaper_config_##n,\
	}

#define ETH_NXP_S32_HW_INSTANCE_CHECK(i, n)						\
	((DT_INST_REG_ADDR(n) == IP_GMAC_##i##_BASE) ? i : 0)

#define ETH_NXP_S32_HW_INSTANCE(n)							\
	LISTIFY(__DEBRACKET FEATURE_GMAC_NUM_INSTANCES,					\
		ETH_NXP_S32_HW_INSTANCE_CHECK, (|), n)

#define ETH_NXP_S32_PHY_DEV(n)								\
	COND_CODE_1(ETH_NXP_S32_IS_FIXED_LINK(n), NULL,					\
		(COND_CODE_1(DT_INST_NODE_HAS_PROP(n, phy_handle),			\
			(DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle))), NULL)))

#define ETH_NXP_S32_DEVICE(n)								\
	ETH_NXP_S32_TX_CALLBACK(n)							\
	ETH_NXP_S32_RX_CALLBACK(n)							\
	ETH_NXP_S32_INIT_CONFIG(n)							\
	ETH_NXP_S32_RX_RING(n);								\
	ETH_NXP_S32_TX_RING(n);								\
	ETH_NXP_S32_MAC_STATE(n);							\
	ETH_NXP_S32_MAC_TXTIMESHAPER_CONFIG(n);						\
	ETH_NXP_S32_MAC_RXRING_CONFIG(n);						\
	ETH_NXP_S32_MAC_TXRING_CONFIG(n);						\
	ETH_NXP_S32_MAC_CONFIG(n);							\
	ETH_NXP_S32_MAC_ADDR(n);							\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static const struct eth_nxp_s32_config eth_nxp_s32_config_##n = {		\
		.instance = ETH_NXP_S32_HW_INSTANCE(n),					\
		.base = (GMAC_Type *)DT_INST_REG_ADDR(n),				\
		.ctrl_cfg = ETH_NXP_S32_CTRL_CONFIG(n),					\
		.do_config = eth_nxp_s32_init_config_##n,				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.phy_dev = ETH_NXP_S32_PHY_DEV(n),					\
		.rx_irq = DT_INST_IRQ_BY_NAME(n, rx, irq),				\
		.tx_irq = DT_INST_IRQ_BY_NAME(n, tx, irq),				\
		.tx_ring_idx = 0U,							\
		.rx_ring_idx = 0U,							\
	};										\
											\
	static struct eth_nxp_s32_data eth_nxp_s32_data_##n;				\
											\
	ETH_NET_DEVICE_DT_INST_DEFINE(n,						\
				eth_nxp_s32_init,					\
				NULL,							\
				&eth_nxp_s32_data_##n,					\
				&eth_nxp_s32_config_##n,				\
				CONFIG_ETH_INIT_PRIORITY,				\
				&eth_api,						\
				NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_NXP_S32_DEVICE)
