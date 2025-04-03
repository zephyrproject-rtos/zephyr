/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/ethernet/eth_intel_plat.h>

#define DT_DRV_COMPAT intel_igc_phy

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intel_igc_phy, CONFIG_PHY_LOG_LEVEL);

#define ANEG_MMD_DEV_NUM		7
#define PHY_REV_MASK			GENMASK(31, 4)
#define MII_INVALID_PHY_ID		UINT32_MAX
#define PHY_ID_SHIFT			16
#define PHY_IGC_READ_DELAY		100

struct phy_intel_igc_cfg {
	const struct device *mdio;
	uint8_t phy_addr;
};

struct phy_intel_igc_data {
	const struct device *phy;
	struct phy_link_state state;
	struct k_work_delayable link_work;
	phy_callback_t cb;
	void *cb_data;
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
	for (uint32_t timeout = 0; timeout < CONFIG_PHY_AUTONEG_TIMEOUT_MS; timeout += delay) {
		if (phy_intel_igc_read(dev, reg_addr, value) >= 0 && (*value & mask) == mask) {
			return 0;
		}
		k_msleep(delay);
	}

	return -ETIMEDOUT;
}

static int phy_intel_igc_id_read(const struct device *dev)
{
	uint32_t phy_id, value = 0;
	int ret;

	ret = phy_intel_igc_read(dev, MII_PHYID1R, &value);
	if (ret < 0) {
		return ret;
	}

	phy_id = value << PHY_ID_SHIFT;

	ret = phy_intel_igc_read(dev, MII_PHYID2R, &value);
	if (ret < 0) {
		return ret;
	}

	phy_id |= (value & PHY_REV_MASK);

	if (phy_id == MII_INVALID_PHY_ID) {
		return -EINVAL;
	}

	LOG_INF("PHY ID = 0x%x", phy_id);

	return 0;
}

static int phy_intel_igc_aneg_10_100mbps(const struct device *dev)
{
	uint32_t value = 0;
	int ret;

	ret = phy_intel_igc_read(dev, MII_ANAR, &value);
	if (ret < 0) {
		return ret;
	}

	value |= MII_ADVERTISE_10_HALF | MII_ADVERTISE_100_HALF |
		 MII_ADVERTISE_10_FULL | MII_ADVERTISE_100_FULL;

	/* Disable Flow control (Rx & Tx) */
	value &= ~(MII_ADVERTISE_PAUSE | MII_ADVERTISE_ASYM_PAUSE);

	return phy_intel_igc_write(dev, MII_ANAR, value);
}

static int phy_intel_igc_aneg_1000mbps(const struct device *dev)
{
	uint32_t value = 0;
	int ret;

	ret = phy_intel_igc_read(dev, MII_1KTCR, &value);
	if (ret < 0) {
		return ret;
	}

	value &= ~MII_ADVERTISE_1000_HALF;
	value |= MII_ADVERTISE_1000_FULL;

	return phy_intel_igc_write(dev, MII_1KTCR, value);
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

	return phy_intel_igc_await_phy_status(dev, MII_BMSR, MII_BMSR_AUTONEG_COMPLETE,
					      &value, PHY_IGC_READ_DELAY);
}

static int phy_intel_igc_cfg_link(const struct device *dev, enum phy_link_speed speed)
{
	uint32_t mii_1ktcr_val = 0;
	uint32_t mii_anar_val = 0;
	int ret;

	ret  = phy_intel_igc_read(dev, MII_ANAR, &mii_anar_val);
	ret |= phy_intel_igc_read(dev, MII_1KTCR, &mii_1ktcr_val);
	if (ret < 0) {
		return ret;
	}

	if (speed & LINK_HALF_10BASE) {
		mii_anar_val |= MII_ADVERTISE_10_HALF;
	} else {
		mii_anar_val &= ~MII_ADVERTISE_10_HALF;
	}
	if (speed & LINK_FULL_10BASE) {
		mii_anar_val |= MII_ADVERTISE_10_FULL;
	} else {
		mii_anar_val &= ~MII_ADVERTISE_10_FULL;
	}

	if (speed & LINK_HALF_100BASE) {
		mii_anar_val |= MII_ADVERTISE_100_HALF;
	} else {
		mii_anar_val &= ~MII_ADVERTISE_100_HALF;
	}
	if (speed & LINK_FULL_100BASE) {
		mii_anar_val |= MII_ADVERTISE_100_FULL;
	} else {
		mii_anar_val &= ~MII_ADVERTISE_100_FULL;
	}

	ret = phy_intel_igc_write(dev, MII_ANAR, mii_anar_val);
	if (ret < 0) {
		return ret;
	}

	if (speed & LINK_FULL_1000BASE) {
		mii_1ktcr_val |= MII_ADVERTISE_1000_FULL;
	} else {
		mii_1ktcr_val &= ~MII_ADVERTISE_1000_FULL;
	}

	return phy_intel_igc_write(dev, MII_1KTCR, mii_1ktcr_val);
}

static int phy_intel_igc_get_speed(const struct device *dev, struct phy_link_state *state)
{
	uint32_t anlpar_val = 0;
	uint32_t anar_val = 0;
	uint32_t c1kt_val = 0;
	uint32_t s1kt_val = 0;
	int ret;

	ret  = phy_intel_igc_read(dev, MII_ANAR, &anar_val);
	ret |= phy_intel_igc_read(dev, MII_ANLPAR, &anlpar_val);
	ret |= phy_intel_igc_read(dev, MII_1KTCR, &c1kt_val);
	ret |= phy_intel_igc_read(dev, MII_1KSTSR, &s1kt_val);
	if (ret < 0) {
		return ret;
	}

	s1kt_val = (uint16_t)((s1kt_val >> 2) & 0xFFFF);
	if (((c1kt_val & s1kt_val) & MII_ADVERTISE_1000_FULL)) {
		state->speed = LINK_FULL_1000BASE;
	} else if (((c1kt_val & s1kt_val) & MII_ADVERTISE_1000_HALF)) {
		state->speed = LINK_HALF_1000BASE;
	} else if ((anar_val & anlpar_val) & MII_ADVERTISE_100_FULL) {
		state->speed = LINK_FULL_100BASE;
	} else if ((anar_val & anlpar_val) & MII_ADVERTISE_100_HALF) {
		state->speed = LINK_HALF_100BASE;
	} else if ((anar_val & anlpar_val) & MII_ADVERTISE_10_FULL) {
		state->speed = LINK_FULL_10BASE;
	} else {
		state->speed = LINK_HALF_10BASE;
	}

	return ret;
}

static int phy_intel_igc_get_link_state(const struct device *dev,
					struct phy_link_state *state)
{
	uint32_t value = 0;
	int ret;

	ret = phy_intel_igc_await_phy_status(dev, MII_BMSR, MII_BMSR_LINK_STATUS,
					     &value, PHY_IGC_READ_DELAY);
	if (ret < 0) {
		return ret;
	}

	state->is_up = !!(value & MII_BMSR_LINK_STATUS);

	return ret;
}

static void invoke_link_cb(const struct device *dev)
{
	struct phy_intel_igc_data *const data = dev->data;

	if (!data->cb) {
		LOG_INF("Callback not yet registered");
		return;
	}

	data->cb(data->phy, &data->state, data->cb_data);
}

static int phy_intel_igc_link_cb_set(const struct device *dev,
				     phy_callback_t cb, void *user_data)
{
	struct phy_intel_igc_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	invoke_link_cb(dev);

	return 0;
}

static void update_link_state(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_intel_igc_data *data = CONTAINER_OF(dwork, struct phy_intel_igc_data,
						       link_work);
	const struct device *dev = data->phy;
	struct phy_link_state new_state = {0};

	if (phy_intel_igc_get_link_state(dev, &new_state) < 0) {
		LOG_ERR("PHY Link is Down");
	}

	if (phy_intel_igc_get_speed(dev, &new_state) < 0) {
		LOG_ERR("Failed to get PHY link Speed");
	}

	if (new_state.is_up != data->state.is_up || new_state.speed != data->state.speed) {
		data->state.is_up = new_state.is_up;
		data->state.speed = new_state.speed;

		invoke_link_cb(dev);

		if (data->state.is_up) {
			LOG_INF("PHY Link is Up with Speed %s Mbps & %s duplex",
				PHY_LINK_IS_SPEED_1000M(new_state.speed) ? "1000" :
				(PHY_LINK_IS_SPEED_100M(new_state.speed) ? "100" : "10"),
				PHY_LINK_IS_FULL_DUPLEX(new_state.speed) ? "Full" : "Half");
		}
	}
}

void eth_intel_igc_misc_phy_link_isr(const struct device *dev)
{
	struct phy_intel_igc_data *data = dev->data;

	k_work_schedule(&data->link_work, K_MSEC(0));
}

static int phy_intel_igc_init(const struct device *dev)
{
	struct phy_intel_igc_data *data = dev->data;
	int ret;

	data->phy = dev;
	ret = phy_intel_igc_id_read(dev);
	if (ret < 0) {
		LOG_ERR("Failed to read PHY ID");
		return ret;
	}

	ret = phy_intel_igc_aneg_10_100mbps(dev);
	if (ret < 0) {
		LOG_ERR("Autoneg init failed for 10/100 Mbps");
		return ret;
	}

	ret = phy_intel_igc_aneg_1000mbps(dev);
	if (ret < 0) {
		LOG_ERR("Autoneg init failed for 1000 Mbps");
		return ret;
	}

	ret = phy_intel_igc_restart_aneg(dev);
	if (ret < 0) {
		LOG_ERR("Failed to restart Autoneg");
	}

	k_work_init_delayable(&data->link_work, update_link_state);

	phy_intel_igc_get_link_state(dev, &data->state);

	return ret;
}

static DEVICE_API(ethphy, phy_api) = {
	.get_link	= phy_intel_igc_get_link_state,
	.cfg_link	= phy_intel_igc_cfg_link,
	.link_cb_set	= phy_intel_igc_link_cb_set,
	.read		= phy_intel_igc_read,
	.write		= phy_intel_igc_write,
};

#define INTEL_IGC_PHY_CONFIG(n)                                                          \
	static const struct phy_intel_igc_cfg phy_cfg_##n = {                            \
		.mdio		= DEVICE_DT_GET(DT_INST_PARENT(n)),                      \
		.phy_addr	= DT_INST_REG_ADDR(n),                                   \
	}                                                                                \

#define INTEL_IGC_PHY_INIT(n)                                                            \
	INTEL_IGC_PHY_CONFIG(n);                                                         \
	static struct phy_intel_igc_data phy_data_##n;                                   \
	DEVICE_DT_INST_DEFINE(n, &phy_intel_igc_init, NULL, &phy_data_##n, &phy_cfg_##n, \
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, &phy_api);          \

DT_INST_FOREACH_STATUS_OKAY(INTEL_IGC_PHY_INIT)
