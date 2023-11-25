/*
 * Copyright 2023 NXP
 * Copyright 2023 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tja1103

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/net/mdio.h>
#include <zephyr/drivers/mdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_tja1103, CONFIG_PHY_LOG_LEVEL);

/* PHYs out of reset check retry delay */
#define TJA1103_AWAIT_DELAY_POLL_US 15000U
/* Number of retries for PHYs out of reset check */
#define TJA1103_AWAIT_RETRY_COUNT   200U

/* TJA1103 PHY identifier */
#define TJA1103_ID 0x1BB013

/* MMD30 - Device status register */
#define TJA1103_DEVICE_CONTROL               (0x0040U)
#define TJA1103_DEVICE_CONTROL_GLOBAL_CFG_EN BIT(14)
#define TJA1103_DEVICE_CONTROL_SUPER_CFG_EN  BIT(13)
/* Shared - PHY control register */
#define TJA1103_PHY_CONTROL                  (0x8100U)
#define TJA1103_PHY_CONTROL_CFG_EN           BIT(14)
/* Shared - PHY status register */
#define TJA1103_PHY_STATUS                   (0x8102U)
#define TJA1103_PHY_STATUS_LINK_STAT         BIT(2)

struct phy_tja1103_config {
	const struct device *mdio;
	uint8_t phy_addr;
	uint8_t master_slave;
};

struct phy_tja1103_data {
	struct phy_link_state state;
	struct k_sem sem;
};

static inline int phy_tja1103_c22_read(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct phy_tja1103_config *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg, val);
}

static inline int phy_tja1103_c22_write(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct phy_tja1103_config *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg, val);
}

static inline int phy_tja1103_c45_write(const struct device *dev, uint16_t devad, uint16_t reg,
					uint16_t val)
{
	const struct phy_tja1103_config *cfg = dev->config;

	return mdio_write_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
}

static inline int phy_tja1103_c45_read(const struct device *dev, uint16_t devad, uint16_t reg,
				       uint16_t *val)
{
	const struct phy_tja1103_config *cfg = dev->config;

	return mdio_read_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
}

static int phy_tja1103_reg_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct phy_tja1103_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = phy_tja1103_c22_read(dev, reg_addr, (uint16_t *)data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_tja1103_reg_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct phy_tja1103_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = phy_tja1103_c22_write(dev, reg_addr, (uint16_t)data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_tja1103_id(const struct device *dev, uint32_t *phy_id)
{
	uint16_t val;

	if (phy_tja1103_c22_read(dev, MII_PHYID1R, &val) < 0) {
		return -EIO;
	}

	*phy_id = (val & UINT16_MAX) << 16;

	if (phy_tja1103_c22_read(dev, MII_PHYID2R, &val) < 0) {
		return -EIO;
	}

	*phy_id |= (val & UINT16_MAX);

	return 0;
}

static int phy_tja1103_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct phy_tja1103_data *const data = dev->data;
	uint16_t val;

	k_sem_take(&data->sem, K_FOREVER);

	if (phy_tja1103_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_STATUS, &val) >= 0) {

		data->state.is_up = (val & TJA1103_PHY_STATUS_LINK_STAT) != 0;

		memcpy(state, &data->state, sizeof(struct phy_link_state));
	}

	k_sem_give(&data->sem);

	return 0;
}

static int phy_tja1103_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds)
{
	ARG_UNUSED(dev);

	if (adv_speeds & LINK_FULL_100BASE_T) {
		return 0;
	}

	return -ENOTSUP;
}

static int phy_tja1103_init(const struct device *dev)
{
	const struct phy_tja1103_config *const cfg = dev->config;
	struct phy_tja1103_data *const data = dev->data;
	uint32_t phy_id = 0;
	uint16_t val;
	int ret;

	data->state.is_up = false;
	data->state.speed = LINK_FULL_100BASE_T;

	ret = WAIT_FOR(!phy_tja1103_id(dev, &phy_id) && phy_id == TJA1103_ID,
			TJA1103_AWAIT_RETRY_COUNT * TJA1103_AWAIT_DELAY_POLL_US,
			k_sleep(K_USEC(TJA1103_AWAIT_DELAY_POLL_US)));
	if (ret < 0) {
		LOG_ERR("Unable to obtain PHY ID for device 0x%x", cfg->phy_addr);
		return -ENODEV;
	}

	/* enable config registers */
	ret = phy_tja1103_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_DEVICE_CONTROL,
				    TJA1103_DEVICE_CONTROL_GLOBAL_CFG_EN |
					    TJA1103_DEVICE_CONTROL_SUPER_CFG_EN);
	if (ret < 0) {
		return ret;
	}

	ret = phy_tja1103_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_CONTROL,
				    TJA1103_PHY_CONTROL_CFG_EN);
	if (ret < 0) {
		return ret;
	}

	ret = phy_tja1103_c45_read(dev, MDIO_MMD_PMAPMD, MDIO_PMA_PMD_BT1_CTRL, &val);
	if (ret < 0) {
		return ret;
	}

	/* Change master/slave mode if need */
	if (cfg->master_slave == 1) {
		val |= MDIO_PMA_PMD_BT1_CTRL_CFG_MST;
	} else if (cfg->master_slave == 2) {
		val &= ~MDIO_PMA_PMD_BT1_CTRL_CFG_MST;
	}

	ret = phy_tja1103_c45_write(dev, MDIO_MMD_PMAPMD, MDIO_PMA_PMD_BT1_CTRL, val);
	if (ret < 0) {
		return ret;
	}

	/* Wait for settings to go in affect before checking link */
	k_sleep(K_MSEC(400));

	phy_tja1103_get_link_state(dev, &data->state);
	return ret;
}

static int phy_tja1103_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
}

static const struct ethphy_driver_api phy_tja1103_api = {
	.get_link = phy_tja1103_get_link_state,
	.cfg_link = phy_tja1103_cfg_link,
	.link_cb_set = phy_tja1103_link_cb_set,
	.read = phy_tja1103_reg_read,
	.write = phy_tja1103_reg_write,
};

#define TJA1103_INITIALIZE(n)                                                                      \
	static const struct phy_tja1103_config phy_tja1103_config_##n = {                          \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n)),                                             \
		.master_slave = DT_INST_ENUM_IDX(n, master_slave),                                 \
	};                                                                                         \
	static struct phy_tja1103_data phy_tja1103_data_##n = {                                    \
		.sem = Z_SEM_INITIALIZER(phy_tja1103_data_##n.sem, 1, 1),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &phy_tja1103_init, NULL, &phy_tja1103_data_##n,                   \
			      &phy_tja1103_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,      \
			      &phy_tja1103_api);

DT_INST_FOREACH_STATUS_OKAY(TJA1103_INITIALIZE)
