/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dsa_netc, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/dsa_core.h>
#include <zephyr/net/dsa_tag_netc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/ethernet/nxp_imx_netc.h>
#include <zephyr/dt-bindings/ethernet/dsa_tag_proto.h>

#include "../eth.h"
#include "fsl_netc_switch.h"

#define DT_DRV_COMPAT nxp_netc_switch
#define PRV_DATA(ctx) ((struct dsa_netc_data *const)(ctx)->prv_data)

#define DEV_CFG(_dev)  ((const struct dsa_netc_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct dsa_netc_data *)(_dev)->data)

struct dsa_netc_port_config {
	const struct pinctrl_dev_config *pincfg;
	netc_hw_mii_mode_t phy_mode;
};

struct dsa_netc_config {
	DEVICE_MMIO_NAMED_ROM(base);
	DEVICE_MMIO_NAMED_ROM(pfconfig);
};

struct dsa_netc_data {
	DEVICE_MMIO_NAMED_RAM(base);
	DEVICE_MMIO_NAMED_RAM(pfconfig);
	swt_config_t swt_config;
	swt_handle_t swt_handle;
	netc_cmd_bd_t *cmd_bd;
#ifdef CONFIG_NET_L2_PTP
	uint8_t cpu_port_idx;
	struct k_fifo tx_ts_queue;
#endif
};

static int dsa_netc_port_init(const struct device *dev)
{
#ifdef CONFIG_NET_L2_PTP
	struct net_if *iface = net_if_lookup_by_dev(dev);
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
#endif
	const struct dsa_port_config *cfg = dev->config;
	struct dsa_netc_port_config *prv_cfg = cfg->prv_config;
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(dsa_switch_ctx);
	swt_config_t *swt_config = &prv->swt_config;
	int ret;

	if (prv_cfg->pincfg != NULL) {
		ret = pinctrl_apply_state(prv_cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
	}

	/* Get default config when beginning to init the first port */
	if (dsa_switch_ctx->init_ports == 1) {
		SWT_GetDefaultConfig(swt_config);
		swt_config->bridgeCfg.dVFCfg.portMembership = 0x0;
	}

	/* miiSpeed and miiDuplex will get set correctly when link is up */
	swt_config->ports[cfg->port_idx].ethMac.miiMode = prv_cfg->phy_mode;

	swt_config->bridgeCfg.dVFCfg.portMembership |= (1 << cfg->port_idx);
	swt_config->ports[cfg->port_idx].bridgeCfg.enMacStationMove = true;

#ifdef CONFIG_NET_L2_PTP
	/* Enable ingress port filter on user ports */
	if (eth_ctx->dsa_port == DSA_CPU_PORT) {
		prv->cpu_port_idx = cfg->port_idx;
		swt_config->ports[cfg->port_idx].commonCfg.ipfCfg.enIPFTable = false;
	} else {
		swt_config->ports[cfg->port_idx].commonCfg.ipfCfg.enIPFTable = true;
	}
#endif

	return 0;
}

static void dsa_netc_port_generate_random_mac(uint8_t *mac_addr)
{
	gen_random_mac(mac_addr, FREESCALE_OUI_B0, FREESCALE_OUI_B1, FREESCALE_OUI_B2);
}

static int dsa_netc_switch_setup(const struct dsa_switch_context *dsa_switch_ctx)
{
	struct dsa_netc_data *prv = PRV_DATA(dsa_switch_ctx);
	swt_config_t *swt_config = &prv->swt_config;
#ifdef CONFIG_NET_L2_PTP
	uint32_t entry_id = 0;
#endif
	status_t result;

	swt_config->cmdRingUse = 1U;
	swt_config->cmdBdrCfg[0].bdBase = prv->cmd_bd;
	swt_config->cmdBdrCfg[0].bdLength = 8U;

	result = SWT_Init(&prv->swt_handle, &prv->swt_config);
	if (result != kStatus_Success) {
		return -EIO;
	}

#ifdef CONFIG_NET_L2_PTP
	/*
	 * For gPTP, switch should work as time-aware bridge.
	 * Trap gPTP frames to cpu port to perform gPTP protocol.
	 */
	netc_tb_ipf_config_t ipf_entry_cfg = {
		.keye.etherType = htons(NET_ETH_PTYPE_PTP),
		.keye.etherTypeMask = 0xffff,
		.keye.srcPort = 0,
		.keye.srcPortMask = 0x0,
		.cfge.fltfa = kNETC_IPFRedirectToMgmtPort,
		.cfge.hr = kNETC_SoftwareDefHR0,
		.cfge.timecape = 1,
		.cfge.rrt = 1,
	};

	result = SWT_RxIPFAddTableEntry(&prv->swt_handle, &ipf_entry_cfg, &entry_id);
	if ((result != kStatus_Success) || (entry_id == 0xFFFFFFFF)) {
		return -EIO;
	}

	k_fifo_init(&prv->tx_ts_queue);
#endif
	return 0;
}

static void dsa_netc_port_phylink_change(const struct device *phydev, struct phy_link_state *state,
					 void *user_data)
{
	const struct device *dev = (struct device *)user_data;
	struct net_if *iface = net_if_lookup_by_dev(dev);
	const struct dsa_port_config *cfg = dev->config;
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(dsa_switch_ctx);
	status_t result;

	if (state->is_up) {
		LOG_INF("DSA user port %d Link up", cfg->port_idx);
		result = SWT_SetEthPortMII(&prv->swt_handle, cfg->port_idx,
					   PHY_TO_NETC_SPEED(state->speed),
					   PHY_TO_NETC_DUPLEX_MODE(state->speed));
		if (result != kStatus_Success) {
			LOG_ERR("DSA user port %d failed to set MAC up", cfg->port_idx);
		}
		net_eth_carrier_on(iface);
	} else {
		LOG_INF("DSA user port %d Link down", cfg->port_idx);
		net_eth_carrier_off(iface);
	}
}

#ifdef CONFIG_NET_L2_PTP
static int dsa_netc_port_txtstamp(const struct device *dev, struct net_pkt *pkt)
{
	struct dsa_switch_context *dsa_switch_ctx = dev->data;
	struct dsa_netc_data *prv = PRV_DATA(dsa_switch_ctx);
	static uint8_t id = 1;

	/* Utilize control block for timestamp request ID. */
	if (id == 16) {
		id = 0;
	}

	pkt->cb.cb[0] = id;
	id++;

	k_fifo_put(&prv->tx_ts_queue, pkt);
	net_pkt_ref(pkt);

	return 0;
}

static void dsa_netc_twostep_timestamp_handler(const struct dsa_switch_context *ctx,
	uint8_t ts_req_id, uint64_t ts)
{
	struct dsa_netc_data *prv = PRV_DATA(ctx);
	struct net_pkt *pkt = k_fifo_get(&prv->tx_ts_queue, K_NO_WAIT);

	while (pkt != NULL) {

		/* Find the matched timestamp */
		if (pkt->cb.cb[0] == ts_req_id) {
			pkt->timestamp.nanosecond = ts % NSEC_PER_SEC;
			pkt->timestamp.second = ts / NSEC_PER_SEC;
			net_if_call_timestamp_cb(pkt);
			net_pkt_unref(pkt);
			return;
		/* Enqueue back */
		} else {
			k_fifo_put(&prv->tx_ts_queue, pkt);
		}

		/* Try next */
		pkt = k_fifo_get(&prv->tx_ts_queue, K_NO_WAIT);
	}
}
#endif

static struct dsa_tag_netc_data dsa_netc_tag_data = {
#ifdef CONFIG_NET_L2_PTP
	.twostep_timestamp_handler = dsa_netc_twostep_timestamp_handler,
#endif
};

static int dsa_netc_connect_tag_protocol(struct dsa_switch_context *dsa_switch_ctx,
					 int tag_proto)
{
	if (tag_proto == DSA_TAG_PROTO_NETC) {
		dsa_switch_ctx->tagger_data = (void *)(&dsa_netc_tag_data);
		return 0;
	}

	return -EIO;
}

static int dsa_netc_switch_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	DEVICE_MMIO_NAMED_MAP(dev, pfconfig, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	return 0;
}

static struct dsa_api dsa_netc_api = {
	.port_init = dsa_netc_port_init,
	.port_generate_random_mac = dsa_netc_port_generate_random_mac,
	.switch_setup = dsa_netc_switch_setup,
	.port_phylink_change = dsa_netc_port_phylink_change,
#ifdef CONFIG_NET_L2_PTP
	.port_txtstamp = dsa_netc_port_txtstamp,
#endif
	.connect_tag_protocol = dsa_netc_connect_tag_protocol,
};

#define DSA_NETC_PORT_INST_INIT(port, n)                                                    \
	COND_CODE_1(DT_NUM_PINCTRL_STATES(port),                                            \
			(PINCTRL_DT_DEFINE(port);), (EMPTY))                                \
	struct dsa_netc_port_config dsa_netc_##n##_##port##_config = {                      \
		.pincfg = COND_CODE_1(DT_NUM_PINCTRL_STATES(port),                          \
				(PINCTRL_DT_DEV_CONFIG_GET(port)), NULL),                   \
		.phy_mode = NETC_PHY_MODE(port),                                            \
	};                                                                                  \
	struct dsa_port_config dsa_##n##_##port##_config = {                                \
		.use_random_mac_addr = DT_NODE_HAS_PROP(port, zephyr_random_mac_address),   \
		.mac_addr = DT_PROP_OR(port, local_mac_address, {0}),                       \
		.port_idx = DT_REG_ADDR(port),                                              \
		.phy_dev = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, phy_handle)),             \
		.phy_mode = DT_PROP_OR(port, phy_connection_type, ""),                      \
		.tag_proto = DT_PROP_OR(port, dsa_tag_protocol, DSA_TAG_PROTO_NOTAG),       \
		.ethernet_connection = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, ethernet)),   \
		IF_ENABLED(CONFIG_PTP_CLOCK_NXP_NETC,				            \
			(.ptp_clock = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, ptp_clock)),)) \
		.prv_config = &dsa_netc_##n##_##port##_config,                              \
	};                                                                                  \
	DSA_PORT_INST_INIT(port, n, &dsa_##n##_##port##_config)

#define DSA_NETC_DEVICE(n)                                                                  \
	AT_NONCACHEABLE_SECTION_ALIGN(static netc_cmd_bd_t dsa_netc_##n##_cmd_bd[8],        \
				      NETC_BD_ALIGN);                                       \
	static const struct dsa_netc_config netc_switch##n##_config = {                     \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(base, DT_DRV_INST(n)),                   \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(pfconfig, DT_DRV_INST(n)),               \
	};                                                                                  \
	static struct dsa_netc_data dsa_netc_data_##n = {                                   \
		.cmd_bd = dsa_netc_##n##_cmd_bd,                                            \
	};                                                                                  \
	DEVICE_DT_INST_DEFINE(n,                                                            \
			      dsa_netc_switch_init,                                         \
			      NULL,                                                         \
			      &dsa_netc_data_##n,                                           \
			      &netc_switch##n##_config,                                     \
			      POST_KERNEL,                                                  \
			      CONFIG_ETH_INIT_PRIORITY,                                     \
			      NULL);		                                            \
	DSA_SWITCH_INST_INIT(n, &dsa_netc_api, &dsa_netc_data_##n, DSA_NETC_PORT_INST_INIT); \

DT_INST_FOREACH_STATUS_OKAY(DSA_NETC_DEVICE);
