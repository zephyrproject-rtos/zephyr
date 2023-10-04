/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_eth_psi);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <ethernet/eth_stats.h>

#include <S32Z2.h>
#include <Netc_Eth_Ip.h>
#include <Netc_Eth_Ip_Irq.h>
#include <Netc_EthSwt_Ip.h>

#include "eth.h"
#include "eth_nxp_s32_netc_priv.h"

#define PSI_NODE	DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_s32_netc_psi)
#define PHY_NODE	DT_PHANDLE(PSI_NODE, phy_dev)
#define INIT_VSIS	DT_NODE_HAS_PROP(PSI_NODE, vsis)
#define TX_RING_IDX	1
#define RX_RING_IDX	0

static void nxp_s32_eth_configure_port(uint8_t port_idx, enum phy_link_speed speed)
{
	EthTrcv_BaudRateType baudrate;
	Netc_EthSwt_Ip_PortDuplexType duplex;
	Std_ReturnType status;

	(void)Netc_EthSwt_Ip_SetPortMode(NETC_SWITCH_IDX, port_idx, false);

	baudrate = PHY_TO_NETC_SPEED(speed);
	status = Netc_EthSwt_Ip_SetPortSpeed(NETC_SWITCH_IDX, port_idx, baudrate);
	if (status != E_OK) {
		LOG_ERR("Failed to set port %d speed: %d", port_idx, status);
		return;
	}

	duplex = PHY_TO_NETC_DUPLEX_MODE(speed);
	status = Netc_EthSwt_Ip_SetPortMacLayerDuplexMode(NETC_SWITCH_IDX, port_idx, duplex);
	if (status != E_OK) {
		LOG_ERR("Failed to set port %d duplex mode: %d", port_idx, status);
		return;
	}

	(void)Netc_EthSwt_Ip_SetPortMode(NETC_SWITCH_IDX, port_idx, true);
}

static void phy_link_state_changed(const struct device *pdev,
				   struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (struct device *)user_data;
	const struct nxp_s32_eth_config *cfg = dev->config;
	const struct nxp_s32_eth_data *ctx = dev->data;

	ARG_UNUSED(pdev);

	if (state->is_up) {
		LOG_DBG("Link up");
		nxp_s32_eth_configure_port(cfg->port_idx, state->speed);
		net_eth_carrier_on(ctx->iface);
	} else {
		LOG_DBG("Link down");
		net_eth_carrier_off(ctx->iface);
	}
}

/* Configure ETHx_EXT_RX_CLK @ 125 MHz as source of ETH_x_RGMII_RX_CLK */
static int nxp_s32_eth_configure_cgm(uint8_t port_idx)
{
	uint32_t tout = 0xFFFFFFFF;

	if (port_idx == 0) {
		IP_MC_CGM_1->MUX_7_CSC = (IP_MC_CGM_1->MUX_7_CSC & ~MC_CGM_MUX_7_CSC_SELCTL_MASK)
					| MC_CGM_MUX_7_CSC_SELCTL(NETC_ETH_0_RX_CLK_IDX);
		IP_MC_CGM_1->MUX_7_CSC = (IP_MC_CGM_1->MUX_7_CSC & ~MC_CGM_MUX_7_CSC_CLK_SW_MASK)
					| MC_CGM_MUX_7_CSC_CLK_SW(1);

		while (((IP_MC_CGM_1->MUX_7_CSS & MC_CGM_MUX_7_CSS_CLK_SW_MASK) == 0)
				&& (tout > 0)) {
			tout--;
		}
		while (((IP_MC_CGM_1->MUX_7_CSS & MC_CGM_MUX_7_CSS_SWIP_MASK) != 0)
				&& (tout > 0)) {
			tout--;
		}
		while (((IP_MC_CGM_1->MUX_7_CSS & MC_CGM_MUX_7_CSS_SWTRG_MASK)
				>> MC_CGM_MUX_7_CSS_SWTRG_SHIFT != 1) && (tout > 0)) {
			tout--;
		}

		__ASSERT_NO_MSG(((IP_MC_CGM_1->MUX_7_CSS & MC_CGM_MUX_7_CSS_SELSTAT_MASK)
				 >> MC_CGM_MUX_7_CSS_SELSTAT_SHIFT) == NETC_ETH_0_RX_CLK_IDX);

	} else if (port_idx == 1) {
		IP_MC_CGM_1->MUX_9_CSC = (IP_MC_CGM_1->MUX_9_CSC & ~MC_CGM_MUX_9_CSC_SELCTL_MASK)
					| MC_CGM_MUX_9_CSC_SELCTL(NETC_ETH_1_RX_CLK_IDX);
		IP_MC_CGM_1->MUX_9_CSC = (IP_MC_CGM_1->MUX_9_CSC & ~MC_CGM_MUX_9_CSC_CLK_SW_MASK)
					| MC_CGM_MUX_9_CSC_CLK_SW(1);

		while (((IP_MC_CGM_1->MUX_9_CSS & MC_CGM_MUX_9_CSS_CLK_SW_MASK) == 0)
				&& (tout > 0)) {
			tout--;
		}
		while (((IP_MC_CGM_1->MUX_9_CSS & MC_CGM_MUX_9_CSS_SWIP_MASK) != 0)
				&& (tout > 0)) {
			tout--;
		}
		while (((IP_MC_CGM_1->MUX_9_CSS & MC_CGM_MUX_9_CSS_SWTRG_MASK)
				>> MC_CGM_MUX_9_CSS_SWTRG_SHIFT != 1) && (tout > 0)) {
			tout--;
		}

		__ASSERT_NO_MSG(((IP_MC_CGM_1->MUX_9_CSS & MC_CGM_MUX_9_CSS_SELSTAT_MASK)
				 >> MC_CGM_MUX_9_CSS_SELSTAT_SHIFT) == NETC_ETH_1_RX_CLK_IDX);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int nxp_s32_eth_initialize(const struct device *dev)
{
	const struct nxp_s32_eth_config *cfg = dev->config;
	Std_ReturnType swt_status;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	err = nxp_s32_eth_configure_cgm(cfg->port_idx);
	if (err != 0) {
		LOG_ERR("Failed to configure NETC Switch CGM");
		return -EIO;
	}

	/* NETC Switch driver must be initialized before PSI */
	swt_status = Netc_EthSwt_Ip_Init(NETC_SWITCH_IDX, &cfg->switch_cfg);
	if (swt_status != E_OK) {
		LOG_ERR("Failed to initialize NETC Switch %d (%d)",
			NETC_SWITCH_IDX, swt_status);
		return -EIO;
	}

	return nxp_s32_eth_initialize_common(dev);
}

static void nxp_s32_eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nxp_s32_eth_data *ctx = dev->data;
	const struct nxp_s32_eth_config *cfg = dev->config;
	const struct nxp_s32_eth_msix *msix;
#if defined(CONFIG_NET_IPV6)
	static struct net_if_mcast_monitor mon;

	net_if_mcast_mon_register(&mon, iface, nxp_s32_eth_mcast_cb);
#endif /* CONFIG_NET_IPV6 */

	/*
	 * For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (ctx->iface == NULL) {
		ctx->iface = iface;
	}

	Netc_Eth_Ip_SetMacAddr(cfg->si_idx, (const uint8_t *)ctx->mac_addr);
	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);

	LOG_INF("SI%d MAC: %02x:%02x:%02x:%02x:%02x:%02x", cfg->si_idx,
		ctx->mac_addr[0], ctx->mac_addr[1], ctx->mac_addr[2],
		ctx->mac_addr[3], ctx->mac_addr[4], ctx->mac_addr[5]);

	ethernet_init(iface);

	/*
	 * PSI controls the PHY. If PHY is configured either as fixed
	 * link or autoneg, the callback is executed at least once
	 * immediately after setting it.
	 */
	if (!device_is_ready(cfg->phy_dev)) {
		LOG_ERR("PHY device (%p) is not ready, cannot init iface",
			cfg->phy_dev);
		return;
	}
	phy_link_callback_set(cfg->phy_dev, &phy_link_state_changed, (void *)dev);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	for (int i = 0; i < NETC_MSIX_EVENTS_COUNT; i++) {
		msix = &cfg->msix[i];
		if (msix->mbox_channel.dev != NULL) {
			if (mbox_set_enabled(&msix->mbox_channel, true)) {
				LOG_ERR("Failed to enable MRU channel %u", msix->mbox_channel.id);
			}
		}
	}
}

static void nxp_s32_eth0_rx_callback(const uint8_t unused, const uint8_t ring)
{
	const struct device *dev = DEVICE_DT_GET(PSI_NODE);
	const struct nxp_s32_eth_config *cfg = dev->config;
	struct nxp_s32_eth_data *ctx = dev->data;

	ARG_UNUSED(unused);

	if (ring == cfg->rx_ring_idx) {
		k_sem_give(&ctx->rx_sem);
	}
}

static const struct ethernet_api nxp_s32_eth_api = {
	.iface_api.init = nxp_s32_eth_iface_init,
	.get_capabilities = nxp_s32_eth_get_capabilities,
	.set_config = nxp_s32_eth_set_config,
	.send = nxp_s32_eth_tx
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(nxp_s32_netc_psi) == 1, "Only one PSI enabled supported");

#define NETC_VSI_GENERAL_CFG(node, prop, idx)					\
	[DT_PROP_BY_IDX(node, prop, idx)] = {					\
		.siId = DT_PROP_BY_IDX(node, prop, idx),			\
		.enableSi = true,						\
		.NumberOfRxBDR = 1,						\
		.NumberOfTxBDR = 1,						\
		.SIVlanControl = (NETC_F3_PSICFGR0_SIVC_CVLAN_BIT		\
				| NETC_F3_PSICFGR0_SIVC_SVLAN_BIT),		\
	}

#define NETC_VSI_RX_MSG_BUF(node, prop, idx)							\
	BUILD_ASSERT((DT_PROP_BY_IDX(node, prop, idx) > NETC_ETH_IP_PSI_INDEX)			\
		&& (DT_PROP_BY_IDX(node, prop, idx) <= FEATURE_NETC_ETH_NUM_OF_VIRTUAL_CTRLS),	\
		"Invalid VSI index");								\
	static Netc_Eth_Ip_VsiToPsiMsgType							\
	_CONCAT3(nxp_s32_eth0_vsi, DT_PROP_BY_IDX(node, prop, idx), _rx_msg_buf)		\
		__aligned(FEATURE_NETC_ETH_VSI_MSG_ALIGNMENT)

#define NETC_VSI_RX_MSG_BUF_ARRAY(node, prop, idx)						\
	[DT_PROP_BY_IDX(node, prop, idx) - 1] =							\
		&_CONCAT3(nxp_s32_eth0_vsi, DT_PROP_BY_IDX(node, prop, idx), _rx_msg_buf)

#define NETC_SWITCH_PORT_CFG(_, __)						\
	{									\
		.ePort = &nxp_s32_eth0_switch_port_egress_cfg,			\
		.iPort = &nxp_s32_eth0_switch_port_ingress_cfg,			\
		.EthSwtPortMacLayerPortEnable = true,				\
		.EthSwtPortMacLayerSpeed = ETHTRCV_BAUD_RATE_1000MBIT,		\
		.EthSwtPortMacLayerDuplexMode = NETC_ETHSWT_PORT_FULL_DUPLEX,	\
		.EthSwtPortPhysicalLayerType = NETC_ETHSWT_RGMII_MODE,		\
		.EthSwtPortPruningEnable = true,				\
	}

static Netc_Eth_Ip_StateType nxp_s32_eth0_state;

static Netc_Eth_Ip_MACFilterHashTableEntryType
nxp_s32_eth0_mac_filter_hash_table[CONFIG_ETH_NXP_S32_MAC_FILTER_TABLE_SIZE];

NETC_TX_RING(0, 0, NETC_MIN_RING_LEN, NETC_MIN_RING_BUF_SIZE);
NETC_TX_RING(0, TX_RING_IDX,
	     CONFIG_ETH_NXP_S32_TX_RING_LEN, CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE);
NETC_RX_RING(0, RX_RING_IDX,
	     CONFIG_ETH_NXP_S32_RX_RING_LEN, CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE);

static const Netc_Eth_Ip_RxRingConfigType nxp_s32_eth0_rxring_cfg[1] = {
	{
		.RingDesc = nxp_s32_eth0_rxring0_desc,
		.Buffer = nxp_s32_eth0_rxring0_buf,
		.ringSize = CONFIG_ETH_NXP_S32_RX_RING_LEN,
		.maxRingSize = CONFIG_ETH_NXP_S32_RX_RING_LEN,
		.bufferLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,
		.maxBuffLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,
		.TimerThreshold = CONFIG_ETH_NXP_S32_RX_IRQ_TIMER_THRESHOLD,
		.PacketsThreshold = CONFIG_ETH_NXP_S32_RX_IRQ_PACKET_THRESHOLD,
		.Callback = nxp_s32_eth0_rx_callback,
	}
};

static const Netc_Eth_Ip_TxRingConfigType nxp_s32_eth0_txring_cfg[2] = {
	{
		.RingDesc = nxp_s32_eth0_txring0_desc,
		.Buffer = nxp_s32_eth0_txring0_buf,
		.ringSize = NETC_MIN_RING_LEN,
		.maxRingSize = NETC_MIN_RING_LEN,
		.bufferLen = NETC_MIN_RING_BUF_SIZE,
		.maxBuffLen = NETC_MIN_RING_BUF_SIZE,
	},
	{
		.RingDesc = nxp_s32_eth0_txring1_desc,
		.Buffer = nxp_s32_eth0_txring1_buf,
		.ringSize = CONFIG_ETH_NXP_S32_TX_RING_LEN,
		.maxRingSize = CONFIG_ETH_NXP_S32_TX_RING_LEN,
		.bufferLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,
		.maxBuffLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,
	}
};

static const Netc_Eth_Ip_GeneralSIConfigType
nxp_s32_eth0_psi_cfg[FEATURE_NETC_ETH_NUMBER_OF_CTRLS] = {
	[NETC_ETH_IP_PSI_INDEX] = {
		.siId = NETC_ETH_IP_PSI_INDEX,
		.enableSi = true,
		.NumberOfRxBDR = 1,
		.NumberOfTxBDR = 2,
		.SIVlanControl = (NETC_F3_PSICFGR0_SIVC_CVLAN_BIT
				| NETC_F3_PSICFGR0_SIVC_SVLAN_BIT),
	},
	COND_CODE_1(INIT_VSIS,
		(DT_FOREACH_PROP_ELEM_SEP(PSI_NODE, vsis, NETC_VSI_GENERAL_CFG, (,))),
		(EMPTY))
};

COND_CODE_1(INIT_VSIS,
	(DT_FOREACH_PROP_ELEM_SEP(PSI_NODE, vsis, NETC_VSI_RX_MSG_BUF, (;))),
	(EMPTY));

static const Netc_Eth_Ip_EnetcGeneralConfigType nxp_s32_eth0_enetc_general_cfg = {
	.numberOfConfiguredSis = FEATURE_NETC_ETH_NUMBER_OF_CTRLS,
	.stationInterfaceGeneralConfig = &nxp_s32_eth0_psi_cfg,
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	.maskMACPromiscuousMulticastEnable = (uint16_t)true,
	.maskMACPromiscuousUnicastEnable = (uint16_t)true,
#endif
	.RxVsiMsgCmdToPsi = {
		COND_CODE_1(INIT_VSIS,
			(DT_FOREACH_PROP_ELEM_SEP(PSI_NODE, vsis,
				NETC_VSI_RX_MSG_BUF_ARRAY, (,))),
			(EMPTY))
	},
};

static const Netc_Eth_Ip_StationInterfaceConfigType nxp_s32_eth0_si_cfg = {
	.NumberOfRxBDR = 1,
	.NumberOfTxBDR = 2,
	.txMruMailboxAddr = NULL,
	.rxMruMailboxAddr = (uint32_t *)MRU_MBOX_ADDR(PSI_NODE, rx),
	.siMsgMruMailboxAddr = COND_CODE_1(INIT_VSIS,
		((uint32_t *)MRU_MBOX_ADDR(PSI_NODE, vsi_msg)), (NULL)),
	.RxInterrupts = (uint32_t)true,
	.TxInterrupts = (uint32_t)false,
	.MACFilterTableMaxNumOfEntries = CONFIG_ETH_NXP_S32_MAC_FILTER_TABLE_SIZE,
};

static uint8_t nxp_s32_eth0_switch_vlandr2dei_cfg[NETC_ETHSWT_NUMBER_OF_DR];
static Netc_EthSwt_Ip_PortIngressType nxp_s32_eth0_switch_port_ingress_cfg;
static Netc_EthSwt_Ip_PortEgressType nxp_s32_eth0_switch_port_egress_cfg = {
	.vlanDrToDei = &nxp_s32_eth0_switch_vlandr2dei_cfg,
};
static Netc_EthSwt_Ip_PortType nxp_s32_eth0_switch_ports_cfg[NETC_ETHSWT_NUMBER_OF_PORTS] = {
	LISTIFY(NETC_ETHSWT_NUMBER_OF_PORTS, NETC_SWITCH_PORT_CFG, (,))
};

PINCTRL_DT_DEFINE(PSI_NODE);

NETC_GENERATE_MAC_ADDRESS(PSI_NODE, 0)

static const struct nxp_s32_eth_config nxp_s32_eth0_config = {
	.netc_cfg = {
		.SiType = NETC_ETH_IP_PHYSICAL_SI,
		.siConfig = &nxp_s32_eth0_si_cfg,
		.generalConfig = &nxp_s32_eth0_enetc_general_cfg,
		.stateStructure = &nxp_s32_eth0_state,
		.paCtrlRxRingConfig = &nxp_s32_eth0_rxring_cfg,
		.paCtrlTxRingConfig = &nxp_s32_eth0_txring_cfg,
	},
	.switch_cfg = {
		.port = &nxp_s32_eth0_switch_ports_cfg,
		.EthSwtArlTableEntryTimeout = NETC_SWITCH_PORT_AGING,
		.netcClockFrequency = DT_PROP(PSI_NODE, clock_frequency),
		.MacLearningOption = ETHSWT_MACLEARNINGOPTION_HWDISABLED,
		.MacForwardingOption = ETHSWT_NO_FDB_LOOKUP_FLOOD_FRAME,
		.Timer1588ClkSrc = ETHSWT_REFERENCE_CLOCK_DISABLED,
	},
	.si_idx = NETC_ETH_IP_PSI_INDEX,
	.port_idx = NETC_SWITCH_PORT_IDX,
	.tx_ring_idx = TX_RING_IDX,
	.rx_ring_idx = RX_RING_IDX,
	.msix = {
		NETC_MSIX(PSI_NODE, rx, Netc_Eth_Ip_0_MSIX_RxEvent),
		COND_CODE_1(INIT_VSIS,
			(NETC_MSIX(PSI_NODE, vsi_msg, Netc_Eth_Ip_MSIX_SIMsgEvent)),
			(EMPTY))
	},
	.mac_filter_hash_table = &nxp_s32_eth0_mac_filter_hash_table[0],
	.generate_mac = nxp_s32_eth0_generate_mac,
	.phy_dev = DEVICE_DT_GET(PHY_NODE),
	.pincfg = PINCTRL_DT_DEV_CONFIG_GET(PSI_NODE),
};

static struct nxp_s32_eth_data nxp_s32_eth0_data = {
	.mac_addr = DT_PROP_OR(PSI_NODE, local_mac_address, {0}),
};

ETH_NET_DEVICE_DT_DEFINE(PSI_NODE,
			nxp_s32_eth_initialize,
			NULL,
			&nxp_s32_eth0_data,
			&nxp_s32_eth0_config,
			CONFIG_ETH_NXP_S32_PSI_INIT_PRIORITY,
			&nxp_s32_eth_api,
			NET_ETH_MTU);
