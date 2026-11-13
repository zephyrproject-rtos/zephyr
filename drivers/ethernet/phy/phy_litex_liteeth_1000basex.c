/*
 * Copyright 2025 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_liteeth_1000basex_phy

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>

#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_litex_1000basex, CONFIG_PHY_LOG_LEVEL);

struct phy_litex_1000basex_config {
	mem_addr_t pcs_status_addr;
	mem_addr_t ev_pending_addr;
	mem_addr_t ev_enable_addr;
};

struct phy_litex_1000basex_data {
	phy_callback_t cb;
	void *cb_data;
};

#define TX_CONFIG_REG_SGMII_OFFSET       16

#define TX_CONFIG_REG_SGMII_SPEED_HALF_1000 BIT(11)
#define TX_CONFIG_REG_SGMII_SPEED_FULL_1000 BIT(12) | TX_CONFIG_REG_SGMII_SPEED_HALF_1000
#define TX_CONFIG_REG_SGMII_SPEED_HALF_100  BIT(10)
#define TX_CONFIG_REG_SGMII_SPEED_FULL_100  BIT(12) | TX_CONFIG_REG_SGMII_SPEED_HALF_100
#define TX_CONFIG_REG_SGMII_SPEED_HALF_10   0
#define TX_CONFIG_REG_SGMII_SPEED_FULL_10   BIT(12) | TX_CONFIG_REG_SGMII_SPEED_HALF_10

#define TX_CONFIG_REG_IS_UP(status)    IS_BIT_SET(status, 0)
#define TX_CONFIG_REG_IS_SGMII(status) IS_BIT_SET(status, 1)

static int phy_litex_1000basex_get_link_state(const struct device *dev,
					      struct phy_link_state *state)
{
	const struct phy_litex_1000basex_config *const cfg = dev->config;
	uint32_t status;

	status = litex_read32(cfg->pcs_status_addr);

	state->is_up = TX_CONFIG_REG_IS_UP(status);

	state->speed = 0;

	if (!state->is_up) {
		return 0;
	}

	if (!TX_CONFIG_REG_IS_SGMII(status)) {
		/* Current link is 1000BASE-X and not SGMII */
		state->speed = LINK_FULL_1000BASE;
		return 0;
	}

	switch ((status >> TX_CONFIG_REG_SGMII_OFFSET) & (BIT(10) | BIT(11) | BIT(12))) {
	case TX_CONFIG_REG_SGMII_SPEED_HALF_10:
		state->speed = LINK_HALF_10BASE;
		break;

	case TX_CONFIG_REG_SGMII_SPEED_FULL_10:
		state->speed = LINK_FULL_10BASE;
		break;

	case TX_CONFIG_REG_SGMII_SPEED_HALF_100:
		state->speed = LINK_HALF_100BASE;
		break;

	case TX_CONFIG_REG_SGMII_SPEED_FULL_100:
		state->speed = LINK_FULL_100BASE;
		break;

	case TX_CONFIG_REG_SGMII_SPEED_HALF_1000:
		state->speed = LINK_HALF_1000BASE;
		break;

	case TX_CONFIG_REG_SGMII_SPEED_FULL_1000:
		state->speed = LINK_FULL_1000BASE;
		break;

	default:
		break;
	}

	return 0;
}

static void phy_litex_1000basex_invoke_link_cb(const struct device *dev)
{
	struct phy_litex_1000basex_data *const data = dev->data;
	struct phy_link_state state;

	phy_litex_1000basex_get_link_state(dev, &state);

	if (state.is_up) {
		LOG_INF("%s: Link speed %s Mb, %s duplex %s",
			dev->name,
			PHY_LINK_IS_SPEED_1000M(state.speed) ? "1000" :
			(PHY_LINK_IS_SPEED_100M(state.speed) ? "100" :
			(PHY_LINK_IS_SPEED_10M(state.speed) ? "10" : "0")),
			PHY_LINK_IS_FULL_DUPLEX(state.speed) ? "full" : "half");
	} else {
		LOG_INF("%s: Link down", dev->name);
	}

	if (data->cb == NULL) {
		return;
	}

	data->cb(dev, &state, data->cb_data);
}

static int phy_litex_1000basex_link_cb_set(const struct device *dev, phy_callback_t cb,
					   void *user_data)
{
	const struct phy_litex_1000basex_config *config = dev->config;
	struct phy_litex_1000basex_data *const data = dev->data;

	litex_write8(0, config->ev_enable_addr);
	litex_write8(BIT(0), config->ev_pending_addr);

	data->cb = cb;
	data->cb_data = user_data;

	/**
	 * Immediately invoke the callback to notify the caller of the
	 * current link status.
	 */
	phy_litex_1000basex_invoke_link_cb(dev);

	litex_write8(1, config->ev_enable_addr);

	return 0;
}

static void phy_litex_1000basex_irq_handler(const struct device *dev)
{
	const struct phy_litex_1000basex_config *config = dev->config;

	if (!IS_BIT_SET(litex_read8(config->ev_pending_addr), 0)) {
		return;
	}

	litex_write8(BIT(0), config->ev_pending_addr);

	phy_litex_1000basex_invoke_link_cb(dev);
}

static DEVICE_API(ethphy, phy_litex_1000basex_driver_api) = {
	.get_link = phy_litex_1000basex_get_link_state,
	.link_cb_set = phy_litex_1000basex_link_cb_set,
};

#define PHY_LITEX_1000BASEX_DEVICE(n)                                                              \
                                                                                                   \
	static int phy_litex_1000basex_init##n(const struct device *dev)                           \
	{                                                                                          \
		const struct phy_litex_1000basex_config *config = dev->config;                     \
                                                                                                   \
		litex_write8(0, config->ev_enable_addr);                                           \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                             \
			    phy_litex_1000basex_irq_handler, DEVICE_DT_INST_GET(n), 0);            \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct phy_litex_1000basex_config phy_litex_1000basex_config_##n = {          \
		.pcs_status_addr = DT_INST_REG_ADDR_BY_NAME(n, pcs_status),                        \
		.ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_pending),                        \
		.ev_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_enable),                          \
	};                                                                                         \
	static struct phy_litex_1000basex_data phy_litex_1000basex_data_##n;                       \
	DEVICE_DT_INST_DEFINE(n, &phy_litex_1000basex_init##n, NULL,                               \
			      &phy_litex_1000basex_data_##n, &phy_litex_1000basex_config_##n,      \
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,                               \
			      &phy_litex_1000basex_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PHY_LITEX_1000BASEX_DEVICE)
