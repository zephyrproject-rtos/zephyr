/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_igc_phy

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/ethernet/eth_intel_plat.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intel_igc_phy, CONFIG_PHY_LOG_LEVEL);

/* MULTI GBT AN Control Register */
#define ANEG_MGBT_AN_CTRL_REG		0x20
#define ADVERTISE_2P5G			BIT(7)

#define ANEG_MMD_DEV_NUM		7
#define PHY_REV_MASK			GENMASK(31, 4)
#define MII_INVALID_PHY_ID		UINT32_MAX
#define PHY_ID_SHIFT			16

struct phy_intel_igc_cfg {
	const struct device *const mdio;
	uint8_t phy_addr;
	bool multi_gb_supported;
};

struct phy_intel_igc_data {
	const struct device *dev;
	struct phy_link_state state;
	uint32_t phy_id;
};

static int phy_intel_igc_read(const struct device *dev, uint16_t reg_addr, uint32_t *value)
{
	const struct phy_intel_igc_cfg *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg_addr, (uint16_t *)value);
}

static int phy_intel_igc_write(const struct device *dev, uint16_t reg_addr, uint32_t value)
{
	const struct phy_intel_igc_cfg *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg_addr, (uint16_t)value);
}

static int phy_intel_igc_await_phy_status(const struct device *dev, uint16_t reg_addr,
					  uint16_t mask, uint32_t *value, uint16_t delay)
{
	uint32_t timeout;

	for (timeout = 0U; timeout < CONFIG_PHY_AUTONEG_TIMEOUT_MS; timeout += delay) {
		if (phy_intel_igc_read(dev, reg_addr, value) >= 0) {
			if ((*value & mask) == mask) {
				return 0;
			}
		}
		k_msleep(delay);
	}

	return -ETIMEDOUT;
}

static int phy_intel_igc_get_link_state(const struct device *dev,
					struct phy_link_state *state)
{
	const struct phy_intel_igc_cfg *const cfg = dev->config;
	uint32_t value = 0;
	int ret;

	ret = phy_intel_igc_await_phy_status(dev, MII_BMSR,
					     MII_BMSR_LINK_STATUS, &value, 10);
	if (ret < 0) {
		LOG_INF("Get PHY (%d) Status Timed Out", cfg->phy_addr);
		state->is_up = false;
	} else {
		state->is_up = (value & MII_BMSR_LINK_STATUS) ? true : false;
	}

	LOG_INF("PHY (%d) Link %s", cfg->phy_addr, state->is_up ? "Up" : "Down");

	return ret;
}

static int phy_intel_igc_restart_aneg(const struct device *dev)
{
	uint32_t value = 0;
	int ret;

	ret = phy_intel_igc_read(dev, MII_BMCR, &value);
	if (ret < 0) {
		return ret;
	}

	/* Enable Autoneg and Autoneg-restart */
	value |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	ret = phy_intel_igc_write(dev, MII_BMCR, value);
	if (ret < 0) {
		return ret;
	}

	ret = phy_intel_igc_await_phy_status(dev, MII_BMSR,
					     MII_BMSR_AUTONEG_COMPLETE, &value, 100);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int phy_intel_igc_cfg_link(const struct device *dev, enum phy_link_speed speed)
{
	uint32_t mii_1ktcr_val = 0;
	uint32_t mii_anar_val = 0;
	int ret = 0;

	if ((phy_intel_igc_read(dev, MII_ANAR, &mii_anar_val) < 0) ||
	    (phy_intel_igc_read(dev, MII_1KTCR, &mii_1ktcr_val) < 0)) {
		return -EIO;
	}

	if (speed & LINK_HALF_10BASE_T) {
		mii_anar_val |= MII_ADVERTISE_10_HALF;
	} else {
		mii_anar_val &= ~MII_ADVERTISE_10_HALF;
	}
	if (speed & LINK_FULL_10BASE_T) {
		mii_anar_val |= MII_ADVERTISE_10_FULL;
	} else {
		mii_anar_val &= ~MII_ADVERTISE_10_FULL;
	}
	if (speed & LINK_HALF_100BASE_T) {
		mii_anar_val |= MII_ADVERTISE_100_HALF;
	} else {
		mii_anar_val &= ~MII_ADVERTISE_100_HALF;
	}
	if (speed & LINK_FULL_100BASE_T) {
		mii_anar_val |= MII_ADVERTISE_100_FULL;
	} else {
		mii_anar_val &= ~MII_ADVERTISE_100_FULL;
	}

	ret = phy_intel_igc_write(dev, MII_ANAR, mii_anar_val);
	if (ret < 0) {
		return ret;
	}

	if (speed & LINK_FULL_1000BASE_T) {
		mii_1ktcr_val |= MII_ADVERTISE_1000_FULL;
	} else {
		mii_1ktcr_val &= ~MII_ADVERTISE_1000_FULL;
	}

	ret = phy_intel_igc_write(dev, MII_1KTCR, mii_1ktcr_val);
	if (ret < 0) {
		return ret;
	}

	ret = phy_intel_igc_restart_aneg(dev);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int phy_intel_igc_aneg_10_100mbps(const struct device *dev)
{
	uint32_t value = 0;
	int ret = 0;

	ret = phy_intel_igc_read(dev, MII_ANAR, &value);
	if (ret < 0) {
		return ret;
	}

	/* Configure 10/100 Mbps advertise */
	value |= MII_ADVERTISE_10_HALF | MII_ADVERTISE_100_HALF |
		 MII_ADVERTISE_10_FULL | MII_ADVERTISE_100_FULL;

	/* Disable Flow control (Rx & Tx) */
	value &= ~(MII_ADVERTISE_PAUSE | MII_ADVERTISE_ASYM_PAUSE);

	ret = phy_intel_igc_write(dev, MII_ANAR, value);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int phy_intel_igc_aneg_1gbps(const struct device *dev)
{
	uint32_t value = 0;
	int ret = 0;

	ret = phy_intel_igc_read(dev, MII_1KTCR, &value);
	if (ret < 0) {
		return ret;
	}

	/* Configure 1000 Mbps advertise */
	ret = phy_intel_igc_read(dev, MII_1KTCR, &value);
	if (ret < 0) {
		return ret;
	}

	value &= ~MII_ADVERTISE_1000_HALF;
	value |= MII_ADVERTISE_1000_FULL;
	ret = phy_intel_igc_write(dev, MII_1KTCR, value);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int phy_intel_igc_aneg_multigb(const struct device *dev)
{
	const struct phy_intel_igc_cfg *cfg = dev->config;
	uint16_t value = 0;
	int ret = 0;

	/* Configure Multi Gigabit advertise */
	ret = mdio_read_c45(cfg->mdio, cfg->phy_addr, ANEG_MMD_DEV_NUM,
			    ANEG_MGBT_AN_CTRL_REG, &value);
	if (ret < 0) {
		return ret;
	}

	if (cfg->multi_gb_supported) {
		value |= ADVERTISE_2P5G;
	} else {
		value &= ~ADVERTISE_2P5G;
	}

	ret = mdio_write_c45(cfg->mdio, cfg->phy_addr, ANEG_MMD_DEV_NUM,
			    ANEG_MGBT_AN_CTRL_REG, value);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int phy_intel_igc_id_read(const struct device *dev)
{
	const struct phy_intel_igc_cfg *cfg = dev->config;
	struct phy_intel_igc_data *data = dev->data;
	uint32_t value = 0;
	int ret = 0;

	ret = phy_intel_igc_read(dev, MII_PHYID1R, &value);
	if (ret < 0) {
		return ret;
	}

	data->phy_id = value << PHY_ID_SHIFT;

	ret = phy_intel_igc_read(dev, MII_PHYID2R, &value);
	if (ret < 0) {
		return ret;
	}

	data->phy_id |= (value & PHY_REV_MASK);

	if (data->phy_id == MII_INVALID_PHY_ID) {
		LOG_ERR("Invalid PHY ID (%d) found at address (%d)",
			data->phy_id, cfg->phy_addr);
		return -EINVAL;
	}

	LOG_INF("PHY ID = 0x%x", data->phy_id);

	return 0;
}

static int phy_intel_igc_init(const struct device *dev)
{
	int ret;

	ret = phy_intel_igc_id_read(dev);
	if (ret < 0) {
		LOG_ERR("Failed to read PHY ID");
		return ret;
	}

	ret = phy_intel_igc_aneg_10_100mbps(dev);
	if (ret < 0) {
		LOG_ERR("Failed to config Autoneg 10/100 Mbps");
		return ret;
	}

	ret = phy_intel_igc_aneg_1gbps(dev);
	if (ret < 0) {
		LOG_ERR("Failed to config Autoneg 1Gbps");
		return ret;
	}

	ret = phy_intel_igc_aneg_multigb(dev);
	if (ret < 0) {
		LOG_ERR("Failed to config Autoneg Multi-Gigabit");
		return ret;
	}

	ret = phy_intel_igc_restart_aneg(dev);
	if (ret < 0) {
		LOG_ERR("Failed to restart Autonegotiation");
	}

	return ret;
}

static const struct ethphy_driver_api phy_api = {
	.get_link	= phy_intel_igc_get_link_state,
	.cfg_link	= phy_intel_igc_cfg_link,
	.read		= phy_intel_igc_read,
	.write		= phy_intel_igc_write,
};

#define INTEL_IGC_PHY_CONFIG(n) \
	static const struct phy_intel_igc_cfg phy_cfg_##n = { \
		.mdio		= DEVICE_DT_GET(DT_INST_PARENT(n)), \
		.phy_addr	= DT_INST_REG_ADDR(n), \
		.multi_gb_supported = DT_INST_PROP(n, multi_gigabit_support), \
};

#define INTEL_IGC_PHY_INIT(n) \
	INTEL_IGC_PHY_CONFIG(n); \
	static struct phy_intel_igc_data phy_data_##n; \
	\
	DEVICE_DT_INST_DEFINE(n, &phy_intel_igc_init, NULL, \
			      &phy_data_##n, &phy_cfg_##n, \
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, &phy_api);

DT_INST_FOREACH_STATUS_OKAY(INTEL_IGC_PHY_INIT)
