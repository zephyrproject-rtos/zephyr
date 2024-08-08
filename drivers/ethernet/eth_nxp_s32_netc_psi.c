/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_netc_psi

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

#include <soc.h>
#include <Netc_Eth_Ip.h>
#include <Netc_Eth_Ip_Irq.h>
#include <Netc_EthSwt_Ip.h>

#include "eth.h"
#include "eth_nxp_s32_netc_priv.h"

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

static const struct device *nxp_s32_eth_get_phy(const struct device *dev)
{
	const struct nxp_s32_eth_config *cfg = dev->config;

	return cfg->phy_dev;
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

	return nxp_s32_eth_initialize_common(dev);
}

static void nxp_s32_eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nxp_s32_eth_data *ctx = dev->data;
	const struct nxp_s32_eth_config *cfg = dev->config;
	const struct nxp_s32_eth_msix *msix;

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
		if (mbox_is_ready_dt(&msix->mbox_spec)) {
			if (mbox_set_enabled_dt(&msix->mbox_spec, true)) {
				LOG_ERR("Failed to enable MRU channel %u",
					msix->mbox_spec.channel_id);
			}
		}
	}
}

static const struct ethernet_api nxp_s32_eth_api = {
	.iface_api.init = nxp_s32_eth_iface_init,
	.get_capabilities = nxp_s32_eth_get_capabilities,
	.get_phy = nxp_s32_eth_get_phy,
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
		.changeMACAllowed = true,					\
		.hashFilterUpdateAllowed = true,				\
		IF_ENABLED(CONFIG_NET_PROMISCUOUS_MODE,				\
			(.multicastPromiscuousChangeAllowed = true,))		\
	}

#define NETC_VSI_RX_MSG_BUF(node, prop, idx, n)							\
	BUILD_ASSERT((DT_PROP_BY_IDX(node, prop, idx) > NETC_ETH_IP_PSI_INDEX)			\
		&& (DT_PROP_BY_IDX(node, prop, idx) <= FEATURE_NETC_ETH_NUM_OF_VIRTUAL_CTRLS),	\
		"Invalid VSI index");								\
	static Netc_Eth_Ip_VsiToPsiMsgType							\
	_CONCAT3(nxp_s32_eth##n##_vsi, DT_PROP_BY_IDX(node, prop, idx), _rx_msg_buf)		\
		__aligned(FEATURE_NETC_ETH_VSI_MSG_ALIGNMENT)

#define NETC_VSI_RX_MSG_BUF_ARRAY(node, prop, idx, n)						\
	[DT_PROP_BY_IDX(node, prop, idx) - 1] =							\
		&_CONCAT3(nxp_s32_eth##n##_vsi, DT_PROP_BY_IDX(node, prop, idx), _rx_msg_buf)

#define NETC_SWITCH_PORT_CFG(_, n)						\
	{									\
		.ePort = &nxp_s32_eth##n##_switch_port_egress_cfg,		\
		.iPort = &nxp_s32_eth##n##_switch_port_ingress_cfg,		\
		.EthSwtPortMacLayerPortEnable = true,				\
		.EthSwtPortMacLayerSpeed = ETHTRCV_BAUD_RATE_1000MBIT,		\
		.EthSwtPortMacLayerDuplexMode = NETC_ETHSWT_PORT_FULL_DUPLEX,	\
		.EthSwtPortPhysicalLayerType = NETC_ETHSWT_RGMII_MODE,		\
		.EthSwtPortPruningEnable = true,				\
	}

#define PHY_NODE(n)	DT_INST_PHANDLE(n, phy_handle)
#define INIT_VSIS(n)	DT_INST_NODE_HAS_PROP(n, vsis)

#define NETC_PSI_INSTANCE_DEFINE(n)							\
void nxp_s32_eth_psi##n##_rx_event(uint8_t chan, const uint32 *buf, uint8_t buf_size)	\
{											\
	ARG_UNUSED(chan);								\
	ARG_UNUSED(buf);								\
	ARG_UNUSED(buf_size);								\
											\
	Netc_Eth_Ip_MSIX_Rx(NETC_SI_NXP_S32_HW_INSTANCE(n));				\
}											\
											\
static void nxp_s32_eth##n##_rx_callback(const uint8_t unused, const uint8_t ring)	\
{											\
	const struct device *dev = DEVICE_DT_INST_GET(n);				\
	const struct nxp_s32_eth_config *cfg = dev->config;				\
	struct nxp_s32_eth_data *ctx = dev->data;					\
											\
	ARG_UNUSED(unused);								\
											\
	if (ring == cfg->rx_ring_idx) {							\
		k_sem_give(&ctx->rx_sem);						\
	}										\
}											\
											\
static __nocache Netc_Eth_Ip_StateType nxp_s32_eth##n##_state;				\
static __nocache Netc_Eth_Ip_MACFilterHashTableEntryType				\
nxp_s32_eth##n##_mac_filter_hash_table[CONFIG_ETH_NXP_S32_MAC_FILTER_TABLE_SIZE];	\
											\
NETC_TX_RING(n, 0, NETC_MIN_RING_LEN, NETC_MIN_RING_BUF_SIZE);				\
NETC_TX_RING(n, TX_RING_IDX,								\
	CONFIG_ETH_NXP_S32_TX_RING_LEN, CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE);		\
NETC_RX_RING(n, RX_RING_IDX,								\
	CONFIG_ETH_NXP_S32_RX_RING_LEN, CONFIG_ETH_NXP_S32_RX_RING_BUF_SIZE);		\
											\
static const Netc_Eth_Ip_RxRingConfigType nxp_s32_eth##n##_rxring_cfg[1] = {		\
	{										\
		.RingDesc = nxp_s32_eth##n##_rxring0_desc,				\
		.Buffer = nxp_s32_eth##n##_rxring0_buf,					\
		.ringSize = CONFIG_ETH_NXP_S32_RX_RING_LEN,				\
		.maxRingSize = CONFIG_ETH_NXP_S32_RX_RING_LEN,				\
		.bufferLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
		.maxBuffLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
		.TimerThreshold = CONFIG_ETH_NXP_S32_RX_IRQ_TIMER_THRESHOLD,		\
		.PacketsThreshold = CONFIG_ETH_NXP_S32_RX_IRQ_PACKET_THRESHOLD,		\
		.Callback = nxp_s32_eth##n##_rx_callback,				\
	}										\
};											\
											\
static const Netc_Eth_Ip_TxRingConfigType nxp_s32_eth##n##_txring_cfg[2] = {		\
	{										\
		.RingDesc = nxp_s32_eth##n##_txring0_desc,				\
		.Buffer = nxp_s32_eth##n##_txring0_buf,					\
		.ringSize = NETC_MIN_RING_LEN,						\
		.maxRingSize = NETC_MIN_RING_LEN,					\
		.bufferLen = NETC_MIN_RING_BUF_SIZE,					\
		.maxBuffLen = NETC_MIN_RING_BUF_SIZE,					\
	},										\
	{										\
		.RingDesc = nxp_s32_eth##n##_txring1_desc,				\
		.Buffer = nxp_s32_eth##n##_txring1_buf,					\
		.ringSize = CONFIG_ETH_NXP_S32_TX_RING_LEN,				\
		.maxRingSize = CONFIG_ETH_NXP_S32_TX_RING_LEN,				\
		.bufferLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
		.maxBuffLen = CONFIG_ETH_NXP_S32_TX_RING_BUF_SIZE,			\
	}										\
};											\
											\
static const Netc_Eth_Ip_GeneralSIConfigType						\
nxp_s32_eth##n##_psi_cfg[FEATURE_NETC_ETH_NUMBER_OF_CTRLS] = {				\
	[NETC_SI_NXP_S32_HW_INSTANCE(n)] = {						\
		.siId = NETC_SI_NXP_S32_HW_INSTANCE(n),					\
		.enableSi = true,							\
		.NumberOfRxBDR = 1,							\
		.NumberOfTxBDR = 2,							\
		.SIVlanControl = (NETC_F3_PSICFGR0_SIVC_CVLAN_BIT			\
				| NETC_F3_PSICFGR0_SIVC_SVLAN_BIT),			\
		.changeMACAllowed = true,						\
		.hashFilterUpdateAllowed = true,					\
		IF_ENABLED(CONFIG_NET_PROMISCUOUS_MODE,					\
			(.multicastPromiscuousChangeAllowed = true,))			\
	},										\
	COND_CODE_1(INIT_VSIS(n),							\
		(DT_INST_FOREACH_PROP_ELEM_SEP(n, vsis, NETC_VSI_GENERAL_CFG, (,))),	\
		(EMPTY))								\
};											\
											\
COND_CODE_1(INIT_VSIS(n),								\
	(DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, vsis, NETC_VSI_RX_MSG_BUF, (;), n)),	\
	(EMPTY));									\
											\
static const Netc_Eth_Ip_EnetcGeneralConfigType nxp_s32_eth##n##_enetc_general_cfg = {	\
	.numberOfConfiguredSis = FEATURE_NETC_ETH_NUMBER_OF_CTRLS,			\
	.stationInterfaceGeneralConfig = &nxp_s32_eth##n##_psi_cfg,			\
	IF_ENABLED(CONFIG_NET_PROMISCUOUS_MODE,						\
		(.maskMACPromiscuousMulticastEnable = (uint16_t)true,			\
		.maskMACPromiscuousUnicastEnable = (uint16_t)true,))			\
	.RxVsiMsgCmdToPsi = {								\
		COND_CODE_1(INIT_VSIS(n),						\
			(DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, vsis,			\
				NETC_VSI_RX_MSG_BUF_ARRAY, (,), n)),			\
			(EMPTY))							\
	},										\
	.maskMACVLANPromiscuousEnable =	(uint16)0x3U,					\
	.maskVLANAllowUntaggedEnable =	(uint32)0x30000U,				\
};											\
											\
static const Netc_Eth_Ip_StationInterfaceConfigType nxp_s32_eth##n##_si_cfg = {		\
	.NumberOfRxBDR = 1,								\
	.NumberOfTxBDR = 2,								\
	.txMruMailboxAddr = NULL,							\
	.rxMruMailboxAddr = (uint32 *)MRU_MBOX_ADDR(DT_DRV_INST(n), rx),		\
	.siMsgMruMailboxAddr = COND_CODE_1(INIT_VSIS(n),				\
		((uint32 *)MRU_MBOX_ADDR(DT_DRV_INST(n), vsi_msg)), (NULL)),		\
	.EnableSIMsgInterrupt = true,							\
	.RxInterrupts = (uint32_t)true,							\
	.TxInterrupts = (uint32_t)false,						\
	.MACFilterTableMaxNumOfEntries = CONFIG_ETH_NXP_S32_MAC_FILTER_TABLE_SIZE,	\
};											\
											\
static uint8_t nxp_s32_eth##n##_switch_vlandr2dei_cfg[NETC_ETHSWT_IP_NUMBER_OF_DR];	\
static Netc_EthSwt_Ip_PortIngressType nxp_s32_eth##n##_switch_port_ingress_cfg;		\
static Netc_EthSwt_Ip_PortEgressType nxp_s32_eth##n##_switch_port_egress_cfg = {	\
	.vlanDrToDei = &nxp_s32_eth##n##_switch_vlandr2dei_cfg,				\
};											\
static Netc_EthSwt_Ip_PortType								\
nxp_s32_eth##n##_switch_ports_cfg[NETC_ETHSWT_IP_NUMBER_OF_PORTS] = {			\
	LISTIFY(NETC_ETHSWT_IP_NUMBER_OF_PORTS, NETC_SWITCH_PORT_CFG, (,), n)		\
};											\
											\
static const Netc_EthSwt_Ip_ConfigType nxp_s32_eth##n##_switch_cfg = {			\
	.port = &nxp_s32_eth##n##_switch_ports_cfg,					\
	.EthSwtArlTableEntryTimeout = NETC_SWITCH_PORT_AGING,				\
	.netcClockFrequency = DT_INST_PROP(n, clock_frequency),				\
	.MacLearningOption = ETHSWT_MACLEARNINGOPTION_HWDISABLED,			\
	.MacForwardingOption = ETHSWT_NO_FDB_LOOKUP_FLOOD_FRAME,			\
	.Timer1588ClkSrc = ETHSWT_REFERENCE_CLOCK_DISABLED,				\
};											\
											\
PINCTRL_DT_INST_DEFINE(n);								\
											\
NETC_GENERATE_MAC_ADDRESS(n)								\
											\
static const struct nxp_s32_eth_config nxp_s32_eth##n##_config = {			\
	.netc_cfg = {									\
		.SiType = NETC_ETH_IP_PHYSICAL_SI,					\
		.siConfig = &nxp_s32_eth##n##_si_cfg,					\
		.generalConfig = &nxp_s32_eth##n##_enetc_general_cfg,			\
		.stateStructure = &nxp_s32_eth##n##_state,				\
		.paCtrlRxRingConfig = &nxp_s32_eth##n##_rxring_cfg,			\
		.paCtrlTxRingConfig = &nxp_s32_eth##n##_txring_cfg,			\
	},										\
	.si_idx = NETC_SI_NXP_S32_HW_INSTANCE(n),					\
	.port_idx = NETC_SWITCH_PORT_IDX,						\
	.tx_ring_idx = TX_RING_IDX,							\
	.rx_ring_idx = RX_RING_IDX,							\
	.msix = {									\
		NETC_MSIX(DT_DRV_INST(n), rx, nxp_s32_eth_psi##n##_rx_event),		\
		COND_CODE_1(INIT_VSIS(n),						\
			(NETC_MSIX(DT_DRV_INST(n), vsi_msg, Netc_Eth_Ip_MSIX_SIMsgEvent)),\
			(EMPTY))							\
	},										\
	.mac_filter_hash_table = &nxp_s32_eth##n##_mac_filter_hash_table[0],		\
	.generate_mac = nxp_s32_eth##n##_generate_mac,					\
	.phy_dev = DEVICE_DT_GET(PHY_NODE(n)),						\
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
};											\
											\
static struct nxp_s32_eth_data nxp_s32_eth##n##_data = {				\
	.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),				\
};											\
											\
ETH_NET_DEVICE_DT_INST_DEFINE(n,							\
			nxp_s32_eth_initialize,						\
			NULL,								\
			&nxp_s32_eth##n##_data,						\
			&nxp_s32_eth##n##_config,					\
			CONFIG_ETH_INIT_PRIORITY,					\
			&nxp_s32_eth_api,						\
			NET_ETH_MTU);							\

DT_INST_FOREACH_STATUS_OKAY(NETC_PSI_INSTANCE_DEFINE)

static int nxp_s32_eth_switch_init(void)
{
	Std_ReturnType swt_status;

	swt_status = Netc_EthSwt_Ip_Init(NETC_SWITCH_IDX, &nxp_s32_eth0_switch_cfg);
	if (swt_status != E_OK) {
		LOG_ERR("Failed to initialize NETC Switch %d (%d)",
			NETC_SWITCH_IDX, swt_status);
		return -EIO;
	}

	return 0;
}

/*
 * NETC Switch driver must be initialized before any other NETC component.
 * This is because Netc_EthSwt_Ip_Init() will not only initialize the Switch,
 * but also perform global initialization, enable the PCIe functions for MDIO
 * and ENETC, and initialize MDIO with a fixed configuration.
 */
SYS_INIT(nxp_s32_eth_switch_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
