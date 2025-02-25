/*
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * Copyright (c) 2025 Immo Birnbaum
 *
 * Definitions and procedures moved here from the former proprietary
 * Xilinx GEM PHY driver (phy_xlnx_gem.c), adjusted to use the MDIO
 * framework provided by Zephyr. The TI DP83825 driver was used as a
 * template, which is (c) Bernhard Kraemer.
 *
 * Register IDs & procedures are based on the corresponding datasheet:
 * https://www.ti.com/lit/ds/symlink/dp83822i.pdf
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>

#define DT_DRV_COMPAT   ti_dp83822
#define LOG_MODULE_NAME phy_ti_dp83822

#define LOG_LEVEL CONFIG_PHY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define PHY_TI_CONTROL_REGISTER_1              0x0009
#define PHY_TI_PHY_STATUS_REGISTER             0x0010
#define PHY_TI_MII_INTERRUPT_STATUS_REGISTER_1 0x0012
#define PHY_TI_LED_CONTROL_REGISTER            0x0018
#define PHY_TI_PHY_CONTROL_REGISTER            0x0019

#define PHY_TI_CR1_ROBUST_AUTO_MDIX_BIT BIT(5)

#define PHY_TI_PHY_CONTROL_AUTO_MDIX_ENABLE_BIT     BIT(15)
#define PHY_TI_PHY_CONTROL_FORCE_MDIX_BIT           BIT(14)
#define PHY_TI_PHY_CONTROL_LED_CONFIG_LINK_ONLY_BIT BIT(5)

#define PHY_TI_LED_CONTROL_BLINK_RATE_SHIFT 9
#define PHY_TI_LED_CONTROL_BLINK_RATE_20HZ  0
#define PHY_TI_LED_CONTROL_BLINK_RATE_10HZ  1
#define PHY_TI_LED_CONTROL_BLINK_RATE_5HZ   2
#define PHY_TI_LED_CONTROL_BLINK_RATE_2HZ   3

#define PHY_TI_PHY_STATUS_LINK_BIT   BIT(0)
#define PHY_TI_PHY_STATUS_SPEED_BIT  BIT(1)
#define PHY_TI_PHY_STATUS_DUPLEX_BIT BIT(2)

struct ti_dp83822_config {
	const struct device *mdio_dev;
	uint8_t addr;
};

struct ti_dp83822_data {
	const struct device *dev;

	struct phy_link_state state;
	phy_callback_t cb;
	void *cb_data;

	struct k_mutex mutex;
	struct k_work_delayable phy_monitor_work;
};

static int phy_ti_dp83822_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;

	/* Make sure excessive bits 16-31 are reset */
	*data = 0U;

	return mdio_read(dev_conf->mdio_dev, dev_conf->addr, reg_addr, (uint16_t *)data);
}

static int phy_ti_dp83822_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;

	return mdio_write(dev_conf->mdio_dev, dev_conf->addr, reg_addr, (uint16_t)data);
}

static int phy_ti_dp83822_reset(const struct device *dev)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;

	int ret = 0;
	uint32_t phy_data;
	uint32_t poll_cnt = 0;

	ret = phy_ti_dp83822_read(dev, MII_BMCR, &phy_data);
	if (ret) {
		LOG_ERR("%s: reset PHY: read BCMR failed", dev->name);
		return ret;
	}

	phy_data |= MII_BMCR_RESET;
	ret = phy_ti_dp83822_write(dev, MII_BMCR, phy_data);
	if (ret) {
		LOG_ERR("%s: reset PHY: write BCMR failed", dev->name);
		return ret;
	}

	while (((phy_data & MII_BMCR_RESET) != 0) && (poll_cnt++ < 10)) {
		ret = phy_ti_dp83822_read(dev, MII_BMCR, &phy_data);
		if (ret) {
			LOG_ERR("%s: reset PHY: read BCMR (poll completion) failed", dev->name);
			return ret;
		}
	}
	if (poll_cnt == 10) {
		LOG_ERR("%s: reset PHY: reset completion timed out", dev->name);
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int phy_ti_dp83822_autonegotiate(const struct device *dev)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;

	int ret;
	uint32_t bmcr;

	ret = phy_ti_dp83822_read(dev, MII_BMCR, &bmcr);
	if (ret) {
		LOG_ERR("%s: trigger auto-neg: read BMCR failed", dev->name);
		return ret;
	}

	LOG_DBG("%s: triggering PHY link auto-negotiation", dev->name);
	bmcr |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	bmcr &= ~MII_BMCR_ISOLATE;

	ret = phy_ti_dp83822_write(dev, MII_BMCR, bmcr);
	if (ret) {
		LOG_ERR("%s: trigger auto-neg: write BMCR failed", dev->name);
		return ret;
	}

	return 0;
}

static int phy_ti_dp83822_static_cfg(const struct device *dev)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;

	int ret;
	uint32_t phy_data;
	uint32_t phy_id;

	/* Read PHY ID */
	ret = phy_ti_dp83822_read(dev, MII_PHYID1R, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read PHYID1R failed", dev->name);
		return ret;
	}
	phy_id = ((phy_data << 16) & 0xFFFF0000);
	ret = phy_ti_dp83822_read(dev, MII_PHYID2R, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read PHYID2R failed", dev->name);
		return ret;
	}
	phy_id |= (phy_data & 0x0000FFFF);

	if (phy_id == 0 || phy_id == 0xFFFFFFFF) {
		LOG_ERR("%s: configure PHY: no reply from PHY while reading PHY ID", dev->name);
		return -EIO;
	}

	LOG_DBG("%s: configure PHY: read PHY ID 0x%08X", dev->name, phy_id);

	/* Enable auto-negotiation */
	ret = phy_ti_dp83822_read(dev, MII_BMCR, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read BMCR failed", dev->name);
		return ret;
	}
	phy_data |= MII_BMCR_AUTONEG_ENABLE;
	ret = phy_ti_dp83822_write(dev, MII_BMCR, phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: write BMCR failed", dev->name);
		return ret;
	}

	/* Robust Auto MDIX */
	ret = phy_ti_dp83822_read(dev, PHY_TI_CONTROL_REGISTER_1, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read CR1 failed", dev->name);
		return ret;
	}
	phy_data |= PHY_TI_CR1_ROBUST_AUTO_MDIX_BIT;
	ret = phy_ti_dp83822_write(dev, PHY_TI_CONTROL_REGISTER_1, phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: write CR1 failed", dev->name);
		return ret;
	}

	ret = phy_ti_dp83822_read(dev, PHY_TI_PHY_CONTROL_REGISTER, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read PHYCR failed", dev->name);
		return ret;
	}
	/* Auto MDIX enable */
	phy_data |= PHY_TI_PHY_CONTROL_AUTO_MDIX_ENABLE_BIT;
	/* Link LED shall only indicate link up or down, no RX/TX activity */
	phy_data |= PHY_TI_PHY_CONTROL_LED_CONFIG_LINK_ONLY_BIT;
	/* Force MDIX disable */
	phy_data &= ~PHY_TI_PHY_CONTROL_FORCE_MDIX_BIT;
	ret = phy_ti_dp83822_write(dev, PHY_TI_PHY_CONTROL_REGISTER, phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: write PHYCR failed", dev->name);
		return ret;
	}

	/* Set blink rate to 5 Hz */
	phy_data = (PHY_TI_LED_CONTROL_BLINK_RATE_5HZ << PHY_TI_LED_CONTROL_BLINK_RATE_SHIFT);
	ret = phy_ti_dp83822_write(dev, PHY_TI_LED_CONTROL_REGISTER, phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: write LEDCR failed", dev->name);
	}

	return ret;
}

static int phy_ti_dp83822_cfg_link(const struct device *dev, enum phy_link_speed speeds)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;
	struct ti_dp83822_data *dev_data = dev->data;

	int ret = 0;
	uint32_t anar = MII_ADVERTISE_SEL_IEEE_802_3;

	ret = k_mutex_lock(&dev_data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("%s: configure PHY link: mutex lock error", dev->name);
		return ret;
	}

	/* Cancel monitoring delayable work during link re-configuration */
	k_work_cancel_delayable(&dev_data->phy_monitor_work);

	/* Reset PHY */
	ret = phy_ti_dp83822_reset(dev);
	if (ret) {
		goto out;
	}

	/* Common configuration items */
	ret = phy_ti_dp83822_static_cfg(dev);
	if (ret) {
		goto out;
	}

	/* Configure Auto-Negotiation Advertisement Register (ANAR) */
	ret = phy_ti_dp83822_read(dev, MII_ANAR, &anar);
	if (ret) {
		LOG_ERR("%s: configure PHY link: read ANAR failed", dev->name);
		goto out;
	}

	/* Set link configuration(s) to be advertised in ANAR */
	if (speeds & LINK_FULL_100BASE_T) {
		anar |= MII_ADVERTISE_100_FULL;
	} else {
		anar &= ~MII_ADVERTISE_100_FULL;
	}

	if (speeds & LINK_HALF_100BASE_T) {
		anar |= MII_ADVERTISE_100_HALF;
	} else {
		anar &= ~MII_ADVERTISE_100_HALF;
	}

	if (speeds & LINK_FULL_10BASE_T) {
		anar |= MII_ADVERTISE_10_FULL;
	} else {
		anar &= ~MII_ADVERTISE_10_FULL;
	}

	if (speeds & LINK_HALF_10BASE_T) {
		anar |= MII_ADVERTISE_10_HALF;
	} else {
		anar &= ~MII_ADVERTISE_10_HALF;
	}

	/* Write assembled ANAR contents */
	ret = phy_ti_dp83822_write(dev, MII_ANAR, anar);
	if (ret) {
		LOG_ERR("%s: configure PHY link: write ANAR failed", dev->name);
		goto out;
	}

	/* Start auto-negotiation */
	ret = phy_ti_dp83822_autonegotiate(dev);
	if (ret) {
		LOG_ERR("%s: configure PHY link: auto-negotiation failed", dev->name);
	}

out:
	k_mutex_unlock(&dev_data->mutex);
	k_work_reschedule(&dev_data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	return ret;
}

static int phy_ti_dp83822_get_link(const struct device *dev, struct phy_link_state *state)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;
	struct ti_dp83822_data *dev_data = dev->data;

	int ret;
	uint32_t physts;

	struct phy_link_state old_state = dev_data->state;

	ret = k_mutex_lock(&dev_data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("%s: get PHY link state: mutex lock error", dev->name);
		return ret;
	}

	/* Get link state from PHYSTS */
	ret = phy_ti_dp83822_read(dev, PHY_TI_PHY_STATUS_REGISTER, &physts);
	if (ret) {
		LOG_ERR("%s: get PHY link state: read PHYSTS failed", dev->name);
		k_mutex_unlock(&dev_data->mutex);
		return ret;
	}

	/*
	 * Get link status from PHYSTS:
	 * [0] Link: 1 = up, 0 = down
	 * (Mirrored from BMSR)
	 */
	state->is_up = physts & PHY_TI_PHY_STATUS_LINK_BIT;
	if (!state->is_up) {
		k_mutex_unlock(&dev_data->mutex);
		goto result;
	}

	/*
	 * Get link speed and duplex from PHYSTS:
	 * [2] Duplex: 1 = full, 0 = half
	 * [1] Speed: 1 = 100 MBit/s, 0 = 10 MBit/s
	 */
	if (physts & PHY_TI_PHY_STATUS_DUPLEX_BIT) {
		if (physts & PHY_TI_PHY_STATUS_SPEED_BIT) {
			state->speed = LINK_FULL_100BASE_T;
		} else {
			state->speed = LINK_HALF_100BASE_T;
		}
	} else {
		if (physts & PHY_TI_PHY_STATUS_SPEED_BIT) {
			state->speed = LINK_FULL_10BASE_T;
		} else {
			state->speed = LINK_HALF_10BASE_T;
		}
	}

	k_mutex_unlock(&dev_data->mutex);

result:
	if (memcmp(&old_state, state, sizeof(struct phy_link_state)) != 0) {
		LOG_DBG("%s: PHY link is %s", dev->name, state->is_up ? "up" : "down");
		if (state->is_up) {
			LOG_DBG("%s: PHY configured for %s MBit/s %s", dev->name,
				PHY_LINK_IS_SPEED_100M(state->speed) ? "100" : "10",
				PHY_LINK_IS_FULL_DUPLEX(state->speed) ? "FDX" : "HDX");
		}
	}

	return ret;
}

static int phy_ti_dp83822_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;
	struct ti_dp83822_data *dev_data = dev->data;

	int ret = 0;

	ret = k_mutex_lock(&dev_data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("%s: set link state callback: mutex lock error", dev->name);
		return ret;
	}

	dev_data->cb = cb;
	dev_data->cb_data = user_data;

	k_mutex_unlock(&dev_data->mutex);

	/* Initial state propagation to the newly registered callback function */
	phy_ti_dp83822_get_link(dev, &dev_data->state);
	dev_data->cb(dev, &dev_data->state, dev_data->cb_data);

	return 0;
}

static void phy_ti_dp83822_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ti_dp83822_data *dev_data =
		CONTAINER_OF(dwork, struct ti_dp83822_data, phy_monitor_work);
	const struct device *dev = dev_data->dev;

	struct phy_link_state state = {};
	int ret = phy_ti_dp83822_get_link(dev, &state);

	if (ret == 0 && memcmp(&state, &dev_data->state, sizeof(struct phy_link_state)) != 0) {
		memcpy(&dev_data->state, &state, sizeof(struct phy_link_state));
		if (dev_data->cb) {
			dev_data->cb(dev, &dev_data->state, dev_data->cb_data);
		}
	}

	k_work_reschedule(&dev_data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_ti_dp83822_init(const struct device *dev)
{
	const struct ti_dp83822_config *const dev_conf = dev->config;
	struct ti_dp83822_data *dev_data = dev->data;

	int ret;

	dev_data->dev = dev;

	ret = k_mutex_init(&dev_data->mutex);
	if (ret) {
		LOG_ERR("%s: init PHY: initialize mutex failed", dev->name);
		return ret;
	}

	mdio_bus_enable(dev_conf->mdio_dev);

	k_work_init_delayable(&dev_data->phy_monitor_work, phy_ti_dp83822_monitor_work_handler);
	k_work_reschedule(&dev_data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	LOG_DBG("%s: init PHY: completed", dev->name);

	return 0;
}

static DEVICE_API(ethphy, ti_dp83822_phy_api) = {
	.get_link = phy_ti_dp83822_get_link,
	.cfg_link = phy_ti_dp83822_cfg_link,
	.link_cb_set = phy_ti_dp83822_link_cb_set,
	.read = phy_ti_dp83822_read,
	.write = phy_ti_dp83822_write,
};

#define PHY_TI_DP83822_DEV_CONFIG(n)                                                               \
	static const struct ti_dp83822_config ti_dp83822_##n##_config = {                          \
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};

#define PHY_TI_DP83822_DEV_DATA(n) static struct ti_dp83822_data ti_dp83822_##n##_data;

#define PHY_TI_DP83822_DEV_INIT(n)                                                                 \
	DEVICE_DT_INST_DEFINE(n, phy_ti_dp83822_init, NULL, &ti_dp83822_##n##_data,                \
			      &ti_dp83822_##n##_config, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,     \
			      &ti_dp83822_phy_api);

#define PHY_TI_DP83822_INITIALIZE(n)                                                               \
	PHY_TI_DP83822_DEV_CONFIG(n);                                                              \
	PHY_TI_DP83822_DEV_DATA(n);                                                                \
	PHY_TI_DP83822_DEV_INIT(n);

DT_INST_FOREACH_STATUS_OKAY(PHY_TI_DP83822_INITIALIZE)
