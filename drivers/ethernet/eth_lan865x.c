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

#include "eth_lan865x_priv.h"

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

	net_if_set_link_addr(iface, ctx->mac_address, sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	if (ctx->iface == NULL) {
		ctx->iface = iface;
	}

	ethernet_init(iface);

	net_eth_carrier_on(iface);
	ctx->iface_initialized = true;
}

static enum ethernet_hw_caps lan865x_port_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE_T | ETHERNET_PROMISC_MODE;
}

static int lan865x_gpio_reset(const struct device *dev);
static void lan865x_write_macaddress(const struct device *dev);
static int lan865x_set_config(const struct device *dev, enum ethernet_config_type type,
			      const struct ethernet_config *config)
{
	const struct lan865x_config *cfg = dev->config;
	struct lan865x_data *ctx = dev->data;
	int ret = -ENOTSUP;

	if (type == ETHERNET_CONFIG_TYPE_PROMISC_MODE) {
		return oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_NCFGR,
				       LAN865x_MAC_NCFGR_CAF);
	}

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		memcpy(ctx->mac_address, config->mac_address.addr,
		       sizeof(ctx->mac_address));

		lan865x_write_macaddress(dev);

		return net_if_set_link_addr(ctx->iface, ctx->mac_address,
				     sizeof(ctx->mac_address),
				     NET_LINK_ETHERNET);
	}

	if (type == ETHERNET_CONFIG_TYPE_T1S_PARAM) {
		ret = lan865x_mac_rxtx_control(dev, LAN865x_MAC_TXRX_OFF);
		if (ret) {
			return ret;
		}

		if (config->t1s_param.type == ETHERNET_T1S_PARAM_TYPE_PLCA_CONFIG) {
			cfg->plca->enable = config->t1s_param.plca.enable;
			cfg->plca->node_id = config->t1s_param.plca.node_id;
			cfg->plca->node_count = config->t1s_param.plca.node_count;
			cfg->plca->burst_count = config->t1s_param.plca.burst_count;
			cfg->plca->burst_timer = config->t1s_param.plca.burst_timer;
			cfg->plca->to_timer = config->t1s_param.plca.to_timer;
		}

		/* Reset is required to re-program PLCA new configuration */
		lan865x_gpio_reset(dev);
	}

	return ret;
}

static int lan865x_wait_for_reset(const struct device *dev)
{
	struct lan865x_data *ctx = dev->data;
	uint8_t i;

	/* Wait for end of LAN865x reset */
	for (i = 0; !ctx->reset && i < LAN865X_RESET_TIMEOUT; i++) {
		k_msleep(1);
	}

	if (i == LAN865X_RESET_TIMEOUT) {
		LOG_ERR("LAN865x reset timeout reached!");
		return -ENODEV;
	}

	return 0;
}

static int lan865x_gpio_reset(const struct device *dev)
{
	const struct lan865x_config *cfg = dev->config;
	struct lan865x_data *ctx = dev->data;

	ctx->reset = false;
	ctx->tc6->protected = false;

	/* Perform (GPIO based) HW reset */
	/* assert RESET_N low for 10 µs (5 µs min) */
	gpio_pin_set_dt(&cfg->reset, 1);
	k_busy_wait(10U);
	/* deassert - end of reset indicated by IRQ_N low  */
	gpio_pin_set_dt(&cfg->reset, 0);

	return lan865x_wait_for_reset(dev);
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

/* Implementation of pseudo code from AN1760 */
static uint8_t lan865x_read_indirect_reg(const struct device *dev, uint8_t addr,
					 uint8_t mask)
{
	struct lan865x_data *ctx = dev->data;
	uint32_t val;

	oa_tc6_reg_write(ctx->tc6, 0x000400D8, addr);
	oa_tc6_reg_write(ctx->tc6, 0x000400DA, 0x02);

	oa_tc6_reg_read(ctx->tc6, 0x000400D9, &val);

	return (uint8_t) val & mask;
}

/*
 * Values in the below table for LAN865x rev. B0 and B1 (with place
 * holders for cfgparamX.
 */
static oa_mem_map_t lan865x_conf[] = {
	{ .mms = 0x1, .address = 0x00, .value = 0x0000 },
	{ .mms = 0x4, .address = 0xD0, .value = 0x3F31 },
	{ .mms = 0x4, .address = 0xE0, .value = 0xC000 },
	{ .mms = 0x4, .address = 0x84, .value = 0x0000 }, /* cfgparam1 */
	{ .mms = 0x4, .address = 0x8A, .value = 0x0000 }, /* cfgparam2 */
	{ .mms = 0x4, .address = 0xE9, .value = 0x9E50 },
	{ .mms = 0x4, .address = 0xF5, .value = 0x1CF8 },
	{ .mms = 0x4, .address = 0xF4, .value = 0xC020 },
	{ .mms = 0x4, .address = 0xF8, .value = 0xB900 },
	{ .mms = 0x4, .address = 0xF9, .value = 0x4E53 },
	{ .mms = 0x4, .address = 0x91, .value = 0x9660 },
	{ .mms = 0x4, .address = 0x77, .value = 0x0028 },
	{ .mms = 0x4, .address = 0x43, .value = 0x00FF },
	{ .mms = 0x4, .address = 0x44, .value = 0xFFFF },
	{ .mms = 0x4, .address = 0x45, .value = 0x0000 },
	{ .mms = 0x4, .address = 0x53, .value = 0x00FF },
	{ .mms = 0x4, .address = 0x54, .value = 0xFFFF },
	{ .mms = 0x4, .address = 0x55, .value = 0x0000 },
	{ .mms = 0x4, .address = 0x40, .value = 0x0002 },
	{ .mms = 0x4, .address = 0x50, .value = 0x0002 },
	{ .mms = 0x4, .address = 0xAD, .value = 0x0000 }, /* cfgparam3 */
	{ .mms = 0x4, .address = 0xAE, .value = 0x0000 }, /* cfgparam4 */
	{ .mms = 0x4, .address = 0xAF, .value = 0x0000 }, /* cfgparam5 */
	{ .mms = 0x4, .address = 0xB0, .value = 0x0103 },
	{ .mms = 0x4, .address = 0xB1, .value = 0x0910 },
	{ .mms = 0x4, .address = 0xB2, .value = 0x1D26 },
	{ .mms = 0x4, .address = 0xB3, .value = 0x002A },
	{ .mms = 0x4, .address = 0xB4, .value = 0x0103 },
	{ .mms = 0x4, .address = 0xB5, .value = 0x070D },
	{ .mms = 0x4, .address = 0xB6, .value = 0x1720 },
	{ .mms = 0x4, .address = 0xB7, .value = 0x0027 },
	{ .mms = 0x4, .address = 0xB8, .value = 0x0509 },
	{ .mms = 0x4, .address = 0xB9, .value = 0x0E13 },
	{ .mms = 0x4, .address = 0xBA, .value = 0x1C25 },
	{ .mms = 0x4, .address = 0xBB, .value = 0x002B },
	{ .mms = 0x4, .address = 0x0C, .value = 0x0100 },
	{ .mms = 0x4, .address = 0x81, .value = 0x00E0 },
};

/* Based on AN1760 DS60001760G pseudo code */
static int lan865x_init_chip(const struct device *dev, uint8_t silicon_rev)
{
	uint16_t cfgparam1, cfgparam2, cfgparam3, cfgparam4, cfgparam5;
	uint8_t i, size = ARRAY_SIZE(lan865x_conf);
	struct lan865x_data *ctx = dev->data;
	int8_t offset1 = 0, offset2 = 0;
	uint8_t value1, value2;

	/* Enable protected control RW */
	oa_tc6_set_protected_ctrl(ctx->tc6, true);

	value1 = lan865x_read_indirect_reg(dev, 0x04, 0x1F);
	if ((value1 & 0x10) != 0) { /* Convert uint8_t to int8_t */
		offset1 = (int8_t)((uint8_t)value1 - 0x20);
	} else {
		offset1 = (int8_t)value1;
	}

	value2 = lan865x_read_indirect_reg(dev, 0x08, 0x1F);
	if ((value2 & 0x10) != 0) { /* Convert uint8_t to int8_t */
		offset2 = (int8_t)((uint8_t)value2 - 0x20);
	} else {
		offset2 = (int8_t)value2;
	}

	cfgparam1 = (uint16_t) (((9 + offset1) & 0x3F) << 10) |
		(uint16_t) (((14 + offset1) & 0x3F) << 4) | 0x03;
	cfgparam2 = (uint16_t) (((40 + offset2) & 0x3F) << 10);
	cfgparam3 = (uint16_t) (((5 + offset1) & 0x3F) << 8) |
		(uint16_t) ((9 + offset1) & 0x3F);
	cfgparam4 = (uint16_t) (((9 + offset1) & 0x3F) << 8) |
		(uint16_t) ((14 + offset1) & 0x3F);
	cfgparam5 = (uint16_t) (((17 + offset1) & 0x3F) << 8) |
		(uint16_t) ((22 + offset1) & 0x3F);

	lan865x_update_dev_cfg_array(lan865x_conf, size,
				     MMS_REG(0x4, 0x84), cfgparam1);
	lan865x_update_dev_cfg_array(lan865x_conf, size,
				     MMS_REG(0x4, 0x8A), cfgparam2);
	lan865x_update_dev_cfg_array(lan865x_conf, size,
				     MMS_REG(0x4, 0xAD), cfgparam3);
	lan865x_update_dev_cfg_array(lan865x_conf, size,
				     MMS_REG(0x4, 0xAE), cfgparam4);
	lan865x_update_dev_cfg_array(lan865x_conf, size,
				     MMS_REG(0x4, 0xAF), cfgparam5);

	if (silicon_rev == 1) {
		/* For silicon rev 1 (B0): (bit [3..0] from 0x0A0084 */
		lan865x_update_dev_cfg_array(lan865x_conf, size,
					     MMS_REG(0x4, 0xD0), 0x5F21);
	}

	/* Write LAN865x config with correct order */
	for (i = 0; i < size; i++) {
		oa_tc6_reg_write(ctx->tc6, MMS_REG(lan865x_conf[i].mms,
						   lan865x_conf[i].address),
				 (uint32_t)lan865x_conf[i].value);
	}

	return 0;
}
/* Implementation of pseudo code from AN1760 - END */

static int lan865x_config_plca(const struct device *dev, uint8_t node_id,
			       uint8_t node_cnt, uint8_t burst_cnt, uint8_t burst_timer)
{
	struct lan865x_data *ctx = dev->data;
	uint32_t val;

	/* Collision Detection */
	oa_tc6_reg_write(ctx->tc6, 0x00040087, 0x0083u); /* COL_DET_CTRL0 */

	/* T1S Phy Node ID and Max Node Count */
	if (node_id == 0) {
		val = (uint32_t)node_cnt << 8;
	} else {
		val = (uint32_t)node_id;
	}
	oa_tc6_reg_write(ctx->tc6, 0x0004CA02, val); /* PLCA_CONTROL_1_REGISTER */

	/* PLCA Burst Count and Burst Timer */
	val = ((uint32_t)burst_cnt << 8) | burst_timer;
	oa_tc6_reg_write(ctx->tc6, 0x0004CA05, val); /* PLCA_BURST_MODE_REGISTER */

	/* Enable PLCA */
	oa_tc6_reg_write(ctx->tc6, 0x0004CA01, BIT(15)); /* PLCA_CONTROL_0_REGISTER */

	return 0;
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

	return oa_tc6_reg_write(ctx->tc6, LAN865x_MAC_NCFGR,
				LAN865x_MAC_NCFGR_MTIHEN);
}

static int lan865x_default_config(const struct device *dev, uint8_t silicon_rev)
{
	const struct lan865x_config *cfg = dev->config;
	int ret;

	lan865x_write_macaddress(dev);
	lan865x_set_specific_multicast_addr(dev);

	ret = lan865x_init_chip(dev, silicon_rev);
	if (ret < 0) {
		return ret;
	}

	if (cfg->plca->enable) {
		ret = lan865x_config_plca(dev, cfg->plca->node_id,
					  cfg->plca->node_count,
					  cfg->plca->burst_count,
					  cfg->plca->burst_timer);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

static void lan865x_int_callback(const struct device *dev,
				  struct gpio_callback *cb,
				  uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct lan865x_data *ctx =
		CONTAINER_OF(cb, struct lan865x_data, gpio_int_callback);

	k_sem_give(&ctx->int_sem);
}

static void lan865x_read_chunks(const struct device *dev)
{
	const struct lan865x_config *cfg = dev->config;
	struct lan865x_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_rx_alloc(K_MSEC(cfg->timeout));
	if (!pkt) {
		LOG_ERR("OA RX: Could not allocate packet!");
		return;
	}

	k_sem_take(&ctx->tx_rx_sem, K_FOREVER);
	ret = oa_tc6_read_chunks(tc6, pkt);
	if (ret < 0) {
		eth_stats_update_errors_rx(ctx->iface);
		net_pkt_unref(pkt);
		k_sem_give(&ctx->tx_rx_sem);
		return;
	}

	/* Feed buffer frame to IP stack */
	ret = net_recv_data(ctx->iface, pkt);
	if (ret < 0) {
		LOG_ERR("OA RX: Could not process packet (%d)!", ret);
		net_pkt_unref(pkt);
	}
	k_sem_give(&ctx->tx_rx_sem);
}

static void lan865x_int_thread(const struct device *dev)
{
	struct lan865x_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	uint32_t sts, val, ftr;
	int ret;

	while (true) {
		k_sem_take(&ctx->int_sem, K_FOREVER);
		if (!ctx->reset) {
			oa_tc6_reg_read(tc6, OA_STATUS0, &sts);
			if (sts & OA_STATUS0_RESETC) {
				oa_tc6_reg_write(tc6, OA_STATUS0, sts);
				lan865x_default_config(dev, ctx->silicon_rev);
				oa_tc6_reg_read(tc6, OA_CONFIG0, &val);
				val |= OA_CONFIG0_SYNC | OA_CONFIG0_RFA_ZARFE;
				oa_tc6_reg_write(tc6, OA_CONFIG0, val);
				lan865x_mac_rxtx_control(dev, LAN865x_MAC_TXRX_ON);

				ctx->reset = true;
				/*
				 * According to OA T1S standard - it is mandatory to
				 * read chunk of data to get the IRQ_N negated (deasserted).
				 */
				oa_tc6_read_status(tc6, &ftr);
				continue;
			}
		}

		/*
		 * The IRQ_N is asserted when RCA becomes > 0. As described in
		 * OPEN Alliance 10BASE-T1x standard it is deasserted when first
		 * data header is received by LAN865x.
		 *
		 * Hence, it is mandatory to ALWAYS read at least one data chunk!
		 */
		do {
			lan865x_read_chunks(dev);
		} while (tc6->rca > 0);

		ret = oa_tc6_check_status(tc6);
		if (ret == -EIO) {
			lan865x_gpio_reset(dev);
		}
	}
}

static int lan865x_init(const struct device *dev)
{
	const struct lan865x_config *cfg = dev->config;
	struct lan865x_data *ctx = dev->data;
	int ret;

	__ASSERT(cfg->spi.config.frequency <= LAN865X_SPI_MAX_FREQUENCY,
		 "SPI frequency exceeds supported maximum\n");

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus %s not ready", cfg->spi.bus->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->interrupt)) {
		LOG_ERR("Interrupt GPIO device %s is not ready",
			cfg->interrupt.port->name);
		return -ENODEV;
	}

	/* Check SPI communication after reset */
	ret = lan865x_check_spi(dev);
	if (ret < 0) {
		LOG_ERR("SPI communication not working, %d", ret);
		return ret;
	}

	/*
	 * Configure interrupt service routine for LAN865x IRQ
	 */
	ret = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO, %d", ret);
		return ret;
	}

	gpio_init_callback(&(ctx->gpio_int_callback), lan865x_int_callback,
			   BIT(cfg->interrupt.pin));

	ret = gpio_add_callback(cfg->interrupt.port, &ctx->gpio_int_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add INT callback, %d", ret);
		return ret;
	}

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);

	/* Start interruption-poll thread */
	ctx->tid_int =
		k_thread_create(&ctx->thread, ctx->thread_stack,
				CONFIG_ETH_LAN865X_IRQ_THREAD_STACK_SIZE,
				(k_thread_entry_t)lan865x_int_thread,
				(void *)dev, NULL, NULL,
				K_PRIO_COOP(CONFIG_ETH_LAN865X_IRQ_THREAD_PRIO),
				0, K_NO_WAIT);
	k_thread_name_set(ctx->tid_int, "lan865x_interrupt");

	/* Perform HW reset - 'rst-gpios' required property set in DT */
	if (!gpio_is_ready_dt(&cfg->reset)) {
		LOG_ERR("Reset GPIO device %s is not ready",
			cfg->reset.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure reset GPIO, %d", ret);
		return ret;
	}

	return lan865x_gpio_reset(dev);
}

static int lan865x_port_send(const struct device *dev, struct net_pkt *pkt)
{
	struct lan865x_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	int ret;

	k_sem_take(&ctx->tx_rx_sem, K_FOREVER);
	ret = oa_tc6_send_chunks(tc6, pkt);

	/* Check if rca > 0 during half-duplex TX transmission */
	if (tc6->rca > 0) {
		k_sem_give(&ctx->int_sem);
	}

	k_sem_give(&ctx->tx_rx_sem);
	if (ret < 0) {
		LOG_ERR("TX transmission error, %d", ret);
		eth_stats_update_errors_tx(net_pkt_iface(pkt));
		return ret;
	}

	return 0;
}

static const struct ethernet_api lan865x_api_func = {
	.iface_api.init = lan865x_iface_init,
	.get_capabilities = lan865x_port_get_capabilities,
	.set_config = lan865x_set_config,
	.send = lan865x_port_send,
};

#define LAN865X_DEFINE(inst)                                                                       \
	static struct lan865x_config_plca lan865x_config_plca_##inst = {                           \
		.node_id = DT_INST_PROP(inst, plca_node_id),                                       \
		.node_count = DT_INST_PROP(inst, plca_node_count),                                 \
		.burst_count = DT_INST_PROP(inst, plca_burst_count),                               \
		.burst_timer = DT_INST_PROP(inst, plca_burst_timer),                               \
		.to_timer = DT_INST_PROP(inst, plca_to_timer),                                     \
		.enable = DT_INST_PROP(inst, plca_enable),                                         \
	};                                                                                         \
												   \
	static const struct lan865x_config lan865x_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0),                             \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                               \
		.reset = GPIO_DT_SPEC_INST_GET(inst, rst_gpios),                                   \
		.timeout = CONFIG_ETH_LAN865X_TIMEOUT,                                             \
		.plca = &lan865x_config_plca_##inst,                                               \
	};                                                                                         \
												   \
	struct oa_tc6 oa_tc6_##inst = {                                                            \
		.cps = 64,                                                                         \
		.protected = 0,                                                                    \
		.spi = &lan865x_config_##inst.spi                                                  \
	};                                                                                         \
	static struct lan865x_data lan865x_data_##inst = {                                         \
		.mac_address = DT_INST_PROP(inst, local_mac_address),                              \
		.tx_rx_sem =                                                                       \
			Z_SEM_INITIALIZER((lan865x_data_##inst).tx_rx_sem, 1, 1),                  \
		.int_sem = Z_SEM_INITIALIZER((lan865x_data_##inst).int_sem, 0, 1),                 \
		.tc6 = &oa_tc6_##inst                                                              \
	};                                                                                         \
												   \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, lan865x_init, NULL, &lan865x_data_##inst,              \
				      &lan865x_config_##inst, CONFIG_ETH_INIT_PRIORITY,            \
				      &lan865x_api_func, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(LAN865X_DEFINE);
