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

#include "phy_mii.h"

#define PHY_REV_MASK       GENMASK(31, 4)
#define MII_INVALID_PHY_ID UINT32_MAX
#define PHY_ID_SHIFT       16
#define PHY_IGC_READ_DELAY 100

struct phy_intel_igc_cfg {
	const struct device *mdio;
	enum phy_link_speed default_speeds;
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

static int phy_intel_igc_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds,
				  enum phy_cfg_link_flag flags)
{
	if ((flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED) != 0) {
		LOG_ERR("Disabling auto-negotiation is not supported by this driver");
		return -ENOTSUP;
	}

	return phy_mii_cfg_link_autoneg(dev, adv_speeds, true);
}

static int phy_intel_igc_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	uint32_t anlpar_val = 0;
	uint32_t anar_val = 0;
	uint32_t c1kt_val = 0;
	uint32_t s1kt_val = 0;
	uint32_t bmsr_val = 0;
	int ret;

	ret = phy_intel_igc_read(dev, MII_BMSR, &bmsr_val);
	if (ret < 0) {
		return ret;
	}

	if ((bmsr_val & MII_BMSR_LINK_STATUS) == 0U) {
		state->is_up = false;
		return 0;
	}
	state->is_up = true;

	ret = phy_intel_igc_read(dev, MII_ANAR, &anar_val);
	if (ret < 0) {
		return ret;
	}

	ret = phy_intel_igc_read(dev, MII_ANLPAR, &anlpar_val);
	if (ret < 0) {
		return ret;
	}

	ret = phy_intel_igc_read(dev, MII_1KTCR, &c1kt_val);
	if (ret < 0) {
		return ret;
	}

	ret = phy_intel_igc_read(dev, MII_1KSTSR, &s1kt_val);
	if (ret < 0) {
		return ret;
	}

	s1kt_val = (s1kt_val >> 2) & 0xFFFF;
	if ((c1kt_val & s1kt_val & MII_ADVERTISE_1000_FULL) != 0U) {
		state->speed = LINK_FULL_1000BASE;
	} else if ((anar_val & anlpar_val & MII_ADVERTISE_100_FULL) != 0U) {
		state->speed = LINK_FULL_100BASE;
	} else if ((anar_val & anlpar_val & MII_ADVERTISE_100_HALF) != 0U) {
		state->speed = LINK_HALF_100BASE;
	} else if ((anar_val & anlpar_val & MII_ADVERTISE_10_FULL) != 0U) {
		state->speed = LINK_FULL_10BASE;
	} else {
		state->speed = LINK_HALF_10BASE;
	}

	return 0;
}

static void invoke_link_cb(const struct device *dev)
{
	struct phy_intel_igc_data *const data = dev->data;

	if (data->cb == NULL) {
		return;
	}

	data->cb(data->phy, &data->state, data->cb_data);
}

static int phy_intel_igc_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
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
	struct phy_intel_igc_data *data = CONTAINER_OF(dwork, struct phy_intel_igc_data, link_work);
	const struct device *dev = data->phy;
	struct phy_link_state new_state = {0};

	if (phy_intel_igc_get_link_state(dev, &new_state) < 0) {
		LOG_DBG("Failed to read PHY status registers");
	}

	if (new_state.is_up != data->state.is_up || new_state.speed != data->state.speed) {
		data->state.is_up = new_state.is_up;
		data->state.speed = new_state.speed;

		invoke_link_cb(dev);

		if (data->state.is_up) {
			LOG_INF("PHY Link is Up with Speed %s Mbps & %s duplex",
				PHY_LINK_IS_SPEED_1000M(new_state.speed)
					? "1000"
					: (PHY_LINK_IS_SPEED_100M(new_state.speed) ? "100" : "10"),
				PHY_LINK_IS_FULL_DUPLEX(new_state.speed) ? "Full" : "Half");
		} else {
			LOG_INF("PHY Link is Down");
		}
	}
}

void eth_intel_igc_misc_phy_link_isr(const struct device *dev)
{
	struct phy_intel_igc_data *data = dev->data;

	k_work_reschedule(&data->link_work, K_NO_WAIT);
}

static int phy_intel_igc_init(const struct device *dev)
{
	const struct phy_intel_igc_cfg *const cfg = dev->config;
	int ret;

	ret = phy_intel_igc_id_read(dev);
	if (ret < 0) {
		LOG_ERR("Failed to read PHY ID");
		return ret;
	}

	return phy_intel_igc_cfg_link(dev, cfg->default_speeds, 0);
}

static DEVICE_API(ethphy, phy_api) = {
	.get_link = phy_intel_igc_get_link_state,
	.cfg_link = phy_intel_igc_cfg_link,
	.link_cb_set = phy_intel_igc_link_cb_set,
	.read = phy_intel_igc_read,
	.write = phy_intel_igc_write,
};

#define IGC_PHY_GENERATE_DEFAULT_SPEEDS(n)                                                         \
	((DT_INST_ENUM_HAS_VALUE(n, default_speeds, 10base_half_duplex) ? LINK_HALF_10BASE : 0) |  \
	 (DT_INST_ENUM_HAS_VALUE(n, default_speeds, 10base_full_duplex) ? LINK_FULL_10BASE : 0) |  \
	 (DT_INST_ENUM_HAS_VALUE(n, default_speeds, 100base_half_duplex) ? LINK_HALF_100BASE       \
									 : 0) |                    \
	 (DT_INST_ENUM_HAS_VALUE(n, default_speeds, 100base_full_duplex) ? LINK_FULL_100BASE       \
									 : 0) |                    \
	 (DT_INST_ENUM_HAS_VALUE(n, default_speeds, 1000base_full_duplex) ? LINK_FULL_1000BASE     \
									  : 0))

#define INTEL_IGC_PHY_CONFIG(n)                                                                    \
	BUILD_ASSERT(IGC_PHY_GENERATE_DEFAULT_SPEEDS(n) != 0,                                      \
		     "At least one valid speed must be configured for this driver");               \
	static const struct phy_intel_igc_cfg phy_cfg_##n = {                                      \
		.mdio = DEVICE_DT_GET(DT_INST_PARENT(n)),                                          \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.default_speeds = IGC_PHY_GENERATE_DEFAULT_SPEEDS(n)};

#define INTEL_IGC_PHY_INIT(n)                                                                      \
	INTEL_IGC_PHY_CONFIG(n);                                                                   \
	static struct phy_intel_igc_data phy_data_##n = {                                          \
		.link_work = Z_WORK_DELAYABLE_INITIALIZER(update_link_state),                      \
		.phy = DEVICE_DT_INST_GET(n),                                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &phy_intel_igc_init, NULL, &phy_data_##n, &phy_cfg_##n,           \
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, &phy_api);

DT_INST_FOREACH_STATUS_OKAY(INTEL_IGC_PHY_INIT)
