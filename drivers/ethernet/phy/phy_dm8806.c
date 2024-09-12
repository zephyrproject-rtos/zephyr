/*
 * Copyright (c) 2024 Robert Slawinski <robert.slawinski1@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT davicom_dm8806_phy

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_dm8806_phy, CONFIG_ETHERNET_LOG_LEVEL);

#include <stdio.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/gpio.h>

#include "phy_dm8806_priv.h"

struct phy_dm8806_config {
	const struct device *mdio;
	uint8_t phy_addr;
	uint8_t switch_addr;
	struct gpio_dt_spec gpio_rst;
	struct gpio_dt_spec gpio_int;
	bool mii;
};

struct phy_dm8806_data {
	const struct device *dev;
	struct phy_link_state state;
	struct k_sem sem;
	struct k_work_delayable monitor_work;
	phy_callback_t cb;
	void *cb_data;
};

static int phy_dm8806_init(const struct device *dev)
{
	int ret;
	uint16_t val;
	const struct phy_dm8806_config *cfg = dev->config;

	ret = mdio_read(cfg->mdio, PHY_ADDRESS_18H, PORT5_MAC_CONTROL, &val);
	if (ret) {
		LOG_ERR("Failed to read PORT5_MAC_CONTROL: %i", ret);
		return ret;
	}

	/* Activate default working mode*/
	val |= (P5_50M_INT_CLK_SOURCE | P5_50M_CLK_OUT_ENABLE | P5_EN_FORCE);
	val &= (P5_SPEED_100M | P5_FULL_DUPLEX | P5_FORCE_LINK_ON);

	ret = mdio_write(cfg->mdio, PHY_ADDRESS_18H, PORT5_MAC_CONTROL, val);
	if (ret) {
		LOG_ERR("Failed to write PORT5_MAC_CONTROL, %i", ret);
		return ret;
	}

	ret = mdio_read(cfg->mdio, PHY_ADDRESS_18H, IRQ_LED_CONTROL, &val);
	if (ret) {
		LOG_ERR("Failed to read IRQ_LED_CONTROL, %i", ret);
		return ret;
	}

	/* Activate LED blinking mode indicator mode 0. */
	val &= LED_MODE_0;
	ret = mdio_write(cfg->mdio, PHY_ADDRESS_18H, IRQ_LED_CONTROL, val);
	if (ret) {
		LOG_ERR("Failed to write IRQ_LED_CONTROL, %i", ret);
		return ret;
	}
	return 0;
}

static int phy_dm8806_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	int ret;
	uint16_t status;
	uint16_t data;
	const struct phy_dm8806_config *cfg = dev->config;

	/* Read data from Switch Per-Port Register. */
	ret = mdio_read(cfg->mdio, cfg->switch_addr, PORTX_SWITCH_STATUS, &data);
	if (ret) {
		LOG_ERR("Failes to read data drom DM8806 Switch Per-Port Registers area");
		return ret;
	}
	/* Extract speed and duplex status from Switch Per-Port Register: Per Port
	 * Status Data Register
	 */
	status = data;
	status >>= SPEED_AND_DUPLEX_OFFSET;
	switch (status & SPEED_AND_DUPLEX_MASK) {
	case SPEED_10MBPS_HALF_DUPLEX:
		state->speed = LINK_HALF_10BASE_T;
		break;
	case SPEED_10MBPS_FULL_DUPLEX:
		state->speed = LINK_FULL_10BASE_T;
		break;
	case SPEED_100MBPS_HALF_DUPLEX:
		state->speed = LINK_HALF_100BASE_T;
		break;
	case SPEED_100MBPS_FULL_DUPLEX:
		state->speed = LINK_FULL_100BASE_T;
		break;
	}
	/* Extract link status from Switch Per-Port Register: Per Port Status Data
	 * Register
	 */
	status = data;
	if (status & LINK_STATUS_MASK) {
		state->is_up = true;
	} else {
		state->is_up = false;
	}
	return ret;
}

static int phy_dm8806_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds)
{
	uint8_t ret;
	uint16_t data;
	uint16_t req_speed;
	const struct phy_dm8806_config *cfg = dev->config;

	req_speed = adv_speeds;
	switch (req_speed) {
	case LINK_HALF_10BASE_T:
		req_speed = MODE_10_BASET_HALF_DUPLEX;
		break;

	case LINK_FULL_10BASE_T:
		req_speed = MODE_10_BASET_FULL_DUPLEX;
		break;

	case LINK_HALF_100BASE_T:
		req_speed = MODE_100_BASET_HALF_DUPLEX;
		break;

	case LINK_FULL_100BASE_T:
		req_speed = MODE_100_BASET_FULL_DUPLEX;
		break;
	}

	/* Power down */
	ret = mdio_read(cfg->mdio, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, &data);
	if (ret) {
		LOG_ERR("Failes to read data drom DM8806");
		return ret;
	}
	data |= POWER_DOWN;
	ret = mdio_write(cfg->mdio, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}

	/* Turn off the auto-negotiation process. */
	ret = mdio_read(cfg->mdio, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, &data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}
	data &= ~(AUTO_NEGOTIATION);
	ret = mdio_write(cfg->mdio, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}

	/* Change the link speed. */
	ret = mdio_read(cfg->mdio, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, &data);
	if (ret) {
		LOG_ERR("Failed to read data from DM8806");
		return ret;
	}
	data &= ~(LINK_SPEED | DUPLEX_MODE);
	data |= req_speed;
	ret = mdio_write(cfg->mdio, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}

	/* Power up ethernet port*/
	ret = mdio_read(cfg->mdio, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, &data);
	if (ret) {
		LOG_ERR("Failes to read data drom DM8806");
		return ret;
	}
	data &= ~(POWER_DOWN);
	ret = mdio_write(cfg->mdio, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}
	return -ENOTSUP;
}

static int phy_dm8806_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}

static int phy_dm8806_reg_read(const struct device *dev, uint16_t phy_addr, uint16_t reg_addr,
			       uint32_t *data)
{
	int res;
	const struct phy_dm8806_config *cfg = dev->config;

	res = mdio_read(cfg->mdio, phy_addr, reg_addr, (uint16_t *)data);
	if (res) {
		LOG_ERR("Failed to read data from DM8806");
		return res;
	}
	return res;
}

static int phy_dm8806_reg_write(const struct device *dev, uint16_t phy_addr, uint16_t reg_addr,
				uint32_t data)
{
	int res;
	const struct phy_dm8806_config *cfg = dev->config;

	res = mdio_write(cfg->mdio, phy_addr, reg_addr, data);
	if (res) {
		LOG_ERR("Failed to write data to DM8806");
		return res;
	}
	return res;
}

static const struct ethphy_driver_api phy_dm8806_api = {
	.get_link = phy_dm8806_get_link_state,
	.cfg_link = phy_dm8806_cfg_link,
	.link_cb_set = phy_dm8806_link_cb_set,
	.read = phy_dm8806_reg_read,
	.write = phy_dm8806_reg_write,
};

#define DM8806_PHY_DEFINE_CONFIG(n)                                                                \
	static const struct phy_dm8806_config phy_dm8806_config_##n = {                            \
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n)),                                             \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.switch_addr = DT_PROP(DT_NODELABEL(dm8806_phy##n), reg_switch),                   \
		.gpio_int = GPIO_DT_SPEC_INST_GET(n, interrupt_gpio),                              \
		.gpio_rst = GPIO_DT_SPEC_INST_GET(n, reset_gpio),                                  \
	}

#define DM8806_PHY_INITIALIZE(n)                                                                   \
	DM8806_PHY_DEFINE_CONFIG(n);                                                               \
	static struct phy_dm8806_data phy_dm8806_data_##n = {                                      \
		.sem = Z_SEM_INITIALIZER(phy_dm8806_data_##n.sem, 1, 1),                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &phy_dm8806_init, NULL, &phy_dm8806_data_##n,                     \
			      &phy_dm8806_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,       \
			      &phy_dm8806_api);

DT_INST_FOREACH_STATUS_OKAY(DM8806_PHY_INITIALIZE)
