/*
 * Copyright (c) 2023 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_lan865x

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_lan865x, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>

#include <string.h>
#include <errno.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/drivers/ethernet/eth_lan865x.h>

#include "eth_lan865x_priv.h"

int eth_lan865x_mdio_c22_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			      uint16_t *data)
{
	struct lan865x_data *ctx = dev->data;

	return oa_tc6_mdio_read(ctx->tc6, prtad, regad, data);
}

int eth_lan865x_mdio_c22_write(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t data)
{
	struct lan865x_data *ctx = dev->data;

	return oa_tc6_mdio_write(ctx->tc6, prtad, regad, data);
}

int eth_lan865x_mdio_c45_read(const struct device *dev, uint8_t prtad, uint8_t devad,
			      uint16_t regad, uint16_t *data)
{
	struct lan865x_data *ctx = dev->data;

	return oa_tc6_mdio_read_c45(ctx->tc6, prtad, devad, regad, data);
}

int eth_lan865x_mdio_c45_write(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t regad, uint16_t data)
{
	struct lan865x_data *ctx = dev->data;

	return oa_tc6_mdio_write_c45(ctx->tc6, prtad, devad, regad, data);
}

static int lan865x_mac_rxtx_control(const struct device *dev, bool en)
{
	struct lan865x_data *ctx = dev->data;
	uint32_t ctl = 0;

	if (en) {
		ctl = LAN865x_MAC_NCR_TXEN | LAN865x_MAC_NCR_RXEN;
	}

	return oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_NCR, ctl);
}

static void lan865x_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct lan865x_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	int ret;

	ret = oa_tc6_enable_sync(tc6);
	if (ret) {
		LOG_ERR("Sync enable failed: %d\n", ret);
		return;
	}

	ret = oa_tc6_zero_align_receive_frame_enable(tc6);
	if (ret) {
		LOG_ERR("ZARFE enable failed: %d\n", ret);
		return;
	}

	ret = lan865x_mac_rxtx_control(dev, LAN865x_MAC_TXRX_ON);
	if (ret) {
		LOG_ERR("LAN865x TX and RX enable failed: %d\n", ret);
		return;
	}

	/*
	 * Interrupt line will remain as low, after completing the reset operation. On reception of
	 * the first data header interrupt line will become high
	 */
	tc6->int_flag = true;
	k_sem_give(&tc6->spi_sem);

	net_if_set_link_addr(iface, ctx->mac_address, sizeof(ctx->mac_address), NET_LINK_ETHERNET);

	if (tc6->iface == NULL) {
		tc6->iface = iface;
	}

	ethernet_init(iface);

	net_eth_carrier_on(iface);
}

static enum ethernet_hw_caps lan865x_port_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE | ETHERNET_PROMISC_MODE;
}

static void lan865x_write_macaddress(const struct device *dev);
static int lan865x_set_config(const struct device *dev, enum ethernet_config_type type,
			      const struct ethernet_config *config)
{
	const struct lan865x_config *cfg = dev->config;
	struct lan865x_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	struct phy_plca_cfg plca_cfg;

	if (type == ETHERNET_CONFIG_TYPE_PROMISC_MODE) {
		return oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_NCFGR, LAN865x_MAC_NCFGR_CAF);
	}

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		memcpy(ctx->mac_address, config->mac_address.addr, sizeof(ctx->mac_address));

		lan865x_write_macaddress(dev);

		return net_if_set_link_addr(tc6->iface, ctx->mac_address, sizeof(ctx->mac_address),
					    NET_LINK_ETHERNET);
	}

	if (type == ETHERNET_CONFIG_TYPE_T1S_PARAM) {
		if (config->t1s_param.type == ETHERNET_T1S_PARAM_TYPE_PLCA_CONFIG) {
			plca_cfg.enable = config->t1s_param.plca.enable;
			plca_cfg.node_id = config->t1s_param.plca.node_id;
			plca_cfg.node_count = config->t1s_param.plca.node_count;
			plca_cfg.burst_count = config->t1s_param.plca.burst_count;
			plca_cfg.burst_timer = config->t1s_param.plca.burst_timer;
			plca_cfg.to_timer = config->t1s_param.plca.to_timer;

			return phy_set_plca_cfg(cfg->phy, &plca_cfg);
		}
	}

	return -ENOTSUP;
}

static int lan865x_check_spi(const struct device *dev)
{
	struct lan865x_data *ctx = dev->data;
	uint32_t val;
	int ret;

	ret = oa_tc6_reg_read(ctx->tc6, LAN865x_DEVID, &val);
	if (ret < 0) {
		return -ENODEV;
	}

	ctx->silicon_rev = val & LAN865X_REV_MASK;
	if (ctx->silicon_rev != 1 && ctx->silicon_rev != 2) {
		return -ENODEV;
	}

	ctx->chip_id = (val >> 4) & 0xFFFF;
	if (ctx->chip_id != LAN8650_DEVID && ctx->chip_id != LAN8651_DEVID) {
		return -ENODEV;
	}

	return ret;
}

static void lan865x_write_macaddress(const struct device *dev)
{
	struct lan865x_data *ctx = dev->data;
	uint8_t *mac = &ctx->mac_address[0];
	uint32_t val;

	/* SPEC_ADD2_BOTTOM */
	val = (mac[3] << 24) | (mac[2] << 16) | (mac[1] << 8) | mac[0];
	oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_SAB2, val);

	/* SPEC_ADD2_TOP */
	val = (mac[5] << 8) | mac[4];
	oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_SAT2, val);

	/*
	 * SPEC_ADD1_BOTTOM - setting unique lower MAC address, back off time is
	 * generated out of it.
	 */
	val = (mac[5] << 24) | (mac[4] << 16) | (mac[3] << 8) | mac[2];
	oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_SAB1, val);
}

static int lan865x_set_specific_multicast_addr(const struct device *dev)
{
	struct lan865x_data *ctx = dev->data;
	uint32_t mac_h_hash = 0xffffffff;
	uint32_t mac_l_hash = 0xffffffff;
	int ret;

	/* Enable hash for all multicast addresses */
	ret = oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_HRT, mac_h_hash);
	if (ret) {
		return ret;
	}

	ret = oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_HRB, mac_l_hash);
	if (ret) {
		return ret;
	}

	return oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_NCFGR, LAN865x_MAC_NCFGR_MTIHEN);
}

static int lan865x_default_config(const struct device *dev)
{
	struct lan865x_data *ctx = dev->data;
	int ret;

	/* Enable protected control RW */
	oa_tc6_set_protected_ctrl(ctx->tc6, true);

	ret = oa_tc6_reg_write(ctx->tc6, LAN865X_FIXUP_REG, LAN865X_FIXUP_VALUE);
	if (ret) {
		return ret;
	}

	lan865x_write_macaddress(dev);
	lan865x_set_specific_multicast_addr(dev);

	return 0;
}

static int lan865x_init(const struct device *dev)
{
	struct lan865x_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	int ret;

	ret = oa_tc6_init(tc6);
	if (ret < 0) {
		LOG_ERR("OA TC6 init failed %d", ret);
		return ret;
	}

	/* Check SPI communication after reset */
	ret = lan865x_check_spi(dev);
	if (ret < 0) {
		LOG_ERR("SPI communication is not working, %d", ret);
		return ret;
	}

	return lan865x_default_config(dev);
}

static int lan865x_port_send(const struct device *dev, struct net_pkt *pkt)
{
	struct lan865x_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;

	return oa_tc6_send_chunks(tc6, pkt);
}

const struct device *lan865x_get_phy(const struct device *dev)
{
	const struct lan865x_config *cfg = dev->config;

	return cfg->phy;
}

static const struct ethernet_api lan865x_api_func = {
	.iface_api.init = lan865x_iface_init,
	.get_capabilities = lan865x_port_get_capabilities,
	.set_config = lan865x_set_config,
	.send = lan865x_port_send,
	.get_phy = lan865x_get_phy,
};

#define LAN865X_DEFINE(inst)                                                                       \
	static const struct lan865x_config lan865x_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),                                \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                               \
		.reset = GPIO_DT_SPEC_INST_GET(inst, rst_gpios),                                   \
		.phy = DEVICE_DT_GET(                                                              \
			DT_CHILD(DT_INST_CHILD(inst, lan865x_mdio), ethernet_phy_##inst))};        \
	BUILD_ASSERT(DT_INST_PROP(inst, spi_max_frequency) <= LAN865X_SPI_MAX_FREQUENCY,           \
		     "SPI frequency exceeds supported maximum");                                   \
                                                                                                   \
	struct oa_tc6 oa_tc6_##inst = {                                                            \
		.cps = 64,                                                                         \
		.protected = 0,                                                                    \
		.spi = &lan865x_config_##inst.spi,                                                 \
		.timeout = CONFIG_OA_TC6_TIMEOUT,                                                  \
		.interrupt = &lan865x_config_##inst.interrupt,                                     \
		.reset = &lan865x_config_##inst.reset,                                             \
		.tx_rx_sem = Z_SEM_INITIALIZER((oa_tc6_##inst).tx_rx_sem, 1, 1),                   \
		.int_sem = Z_SEM_INITIALIZER((oa_tc6_##inst).int_sem, 0, 1)};                      \
	static struct lan865x_data lan865x_data_##inst = {                                         \
		.mac_address = DT_INST_PROP_OR(inst, local_mac_address, {0}),                      \
		.tc6 = &oa_tc6_##inst};                                                            \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, lan865x_init, NULL, &lan865x_data_##inst,              \
				      &lan865x_config_##inst, CONFIG_ETH_LAN865X_INIT_PRIORITY,    \
				      &lan865x_api_func, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(LAN865X_DEFINE);
