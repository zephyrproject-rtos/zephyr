/*
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * Copyright (c) 2025 Immo Birnbaum
 *
 * Definitions and procedures moved here from the former proprietary
 * Xilinx GEM PHY driver (phy_xlnx_gem.c), adjusted to use the MDIO
 * framework provided by Zephyr. The TI DP83825 driver was used as a
 * template, which is (c) Bernhard Kraemer.
 *
 * Register IDs & procedures are based on the corresponding datasheets:
 * https://www.marvell.com/content/dam/marvell/en/public-collateral/transceivers/marvell-phys-transceivers-alaska-88e1111-datasheet.pdf
 * https://www.marvell.com/content/dam/marvell/en/public-collateral/phys-transceivers/marvell-phys-transceivers-alaska-88e151x-datasheet.pdf
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT marvell_88e1xxx

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>

#define DT_DRV_COMPAT   marvell_88e1xxx
#define LOG_MODULE_NAME phy_marvell_88e1xxx

#define LOG_LEVEL CONFIG_PHY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Marvell PHY ID: bits [3..0] = revision -> discard during ID check */
#define PHY_MRVL_PHY_ID_MODEL_MASK    0xFFFFFFF0
#define PHY_MRVL_PHY_ID_MODEL_88E151X 0x01410DD0

#define PHY_MRVL_BASE_REGISTERS_PAGE         0
#define PHY_MRVL_COPPER_CONTROL_REGISTER     MII_BMCR
#define PHY_MRVL_COPPER_AUTONEG_ADV_REGISTER MII_ANAR
#define PHY_MRVL_1000BASET_CONTROL_REGISTER  0x09
#define PHY_MRVL_COPPER_CONTROL_1_REGISTER   0x10
#define PHY_MRVL_COPPER_STATUS_1_REGISTER    0x11
#define PHY_MRVL_COPPER_PAGE_SWITCH_REGISTER 0x16

#define PHY_MRVL_GENERAL_CONTROL_1_PAGE     0x12
#define PHY_MRVL_GENERAL_CONTROL_1_REGISTER 0x14

#define PHY_MRVL_COPPER_CONTROL_RESET_BIT          BIT(15)
#define PHY_MRVL_COPPER_CONTROL_AUTONEG_ENABLE_BIT BIT(12)

#define PHY_MRVL_GENERAL_CONTROL_1_RESET_BIT BIT(15)

#define PHY_MRVL_ADVERTISE_1000_FULL BIT(9)
#define PHY_MRVL_ADVERTISE_1000_HALF BIT(8)

#define PHY_MRVL_MDIX_CONFIG_MASK           0x0003
#define PHY_MRVL_MDIX_CONFIG_SHIFT          5
#define PHY_MRVL_MDIX_AUTO_CROSSOVER_ENABLE 0x0003
#define PHY_MRVL_MODE_CONFIG_MASK           0x0007
#define PHY_MRVL_MODE_CONFIG_SHIFT          0

#define PHY_MRVL_LINK_SPEED_SHIFT   14
#define PHY_MRVL_LINK_SPEED_MASK    0x3
#define PHY_MRVL_LINK_SPEED_10MBIT  0
#define PHY_MRVL_LINK_SPEED_100MBIT BIT(0)
#define PHY_MRVL_LINK_SPEED_1GBIT   BIT(1)
#define PHY_MRVL_LINK_DUPLEX_FDX    BIT(13)
#define PHY_MRVL_LINK_STATUS        BIT(10)

struct marvell_alaska_config {
	const struct device *mdio_dev;
	uint8_t addr;
};

struct marvell_alaska_data {
	const struct device *dev;

	struct phy_link_state state;
	phy_callback_t cb;
	void *cb_data;

	struct k_mutex mutex;
	struct k_work_delayable phy_monitor_work;

	uint32_t phy_id;
};

static int phy_marvell_alaska_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct marvell_alaska_config *const dev_conf = dev->config;
	*data = 0U; /* Clear unused bits [31..16] */

	return mdio_read(dev_conf->mdio_dev, dev_conf->addr, reg_addr, (uint16_t *)data);
}

static int phy_marvell_alaska_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct marvell_alaska_config *const dev_conf = dev->config;

	return mdio_write(dev_conf->mdio_dev, dev_conf->addr, reg_addr, (uint16_t)data);
}

static int phy_marvell_alaska_reset(const struct device *dev)
{
	int ret = 0;
	uint32_t phy_data;
	uint32_t poll_cnt = 0;

	/*
	 * Page 0, register address 0 = Copper control register,
	 * bit [15] = PHY Software reset. Register 0/0 access is R/M/W.
	 * Comp.  datasheet chapter 2.6 and table 64 "Copper Control
	 * Register". Triggering a PHY Software reset affects pages
	 * 0, 2, 3, 5, 7.
	 */

	ret = phy_marvell_alaska_read(dev, PHY_MRVL_COPPER_CONTROL_REGISTER, &phy_data);
	if (ret) {
		LOG_ERR("%s: reset PHY: read Copper Control Register failed", dev->name);
		return ret;
	}

	phy_data |= PHY_MRVL_COPPER_CONTROL_RESET_BIT;
	ret = phy_marvell_alaska_write(dev, PHY_MRVL_COPPER_CONTROL_REGISTER, phy_data);
	if (ret) {
		LOG_ERR("%s: reset PHY: write Copper Control Register failed", dev->name);
		return ret;
	}

	while (((phy_data & PHY_MRVL_COPPER_CONTROL_RESET_BIT) != 0) && (poll_cnt++ < 10)) {
		ret = phy_marvell_alaska_read(dev, PHY_MRVL_COPPER_CONTROL_REGISTER, &phy_data);
		if (ret) {
			LOG_ERR("%s: reset PHY: read Copper Control Register "
				"(poll completion) failed",
				dev->name);
			return ret;
		}
	}
	if (poll_cnt == 10) {
		LOG_ERR("%s: reset PHY: reset completion timed out", dev->name);
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int phy_marvell_alaska_autonegotiate(const struct device *dev)
{
	int ret = 0;
	uint32_t phy_data;

	/*
	 * Trigger a PHY reset, affecting pages 0, 2, 3, 5, 7.
	 * Afterwards, set the auto-negotiation enable bit [12] in the
	 * Copper Control Register.
	 */
	ret = phy_marvell_alaska_reset(dev);
	if (ret != 0) {
		LOG_ERR("%s: start auto-neg: reset call within "
			"%s failed", dev->name, __func__);

		return ret;
	}

	ret = phy_marvell_alaska_read(dev, PHY_MRVL_COPPER_CONTROL_REGISTER, &phy_data);
	if (ret) {
		LOG_ERR("%s: start auto-neg: read Copper Control Register failed", dev->name);
		return ret;
	}

	phy_data |= PHY_MRVL_COPPER_CONTROL_AUTONEG_ENABLE_BIT;

	ret = phy_marvell_alaska_write(dev, PHY_MRVL_COPPER_CONTROL_REGISTER, phy_data);
	if (ret) {
		LOG_ERR("%s: start auto-neg: write Copper Control Register failed", dev->name);
		return ret;
	}

	return 0;
}

static int phy_marvell_alaska_static_cfg(const struct device *dev)
{
	struct marvell_alaska_data *dev_data = dev->data;

	int ret;
	uint32_t phy_data;
	uint32_t phy_id;
	uint32_t poll_cnt = 0;

	/* Read & store PHY ID */
	ret = phy_marvell_alaska_read(dev, MII_PHYID1R, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read PHYID1R failed", dev->name);
		return ret;
	}
	phy_id = ((phy_data << 16) & 0xFFFF0000);
	ret = phy_marvell_alaska_read(dev, MII_PHYID2R, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read PHYID2R failed", dev->name);
		return ret;
	}
	phy_id |= (phy_data & 0x0000FFFF);

	LOG_DBG("%s: configure PHY: read PHY ID 0x%08X", dev->name, phy_id);

	if (phy_id == 0 || phy_id == 0xFFFFFFFF) {
		LOG_ERR("%s: configure PHY: no reply from PHY while reading PHY ID", dev->name);
		return -EIO;
	}

	dev_data->phy_id = phy_id;

	/*
	 * Page 0, register address 0 = Copper control register,
	 * bit [12] = auto-negotiation enable bit is to be cleared
	 * for now, afterwards, trigger a PHY Software reset.
	 * Register 0/0 access is R/M/W. Comp. datasheet chapter 2.6
	 * and table 64 "Copper Control Register".
	 */
	ret = phy_marvell_alaska_read(dev, PHY_MRVL_COPPER_CONTROL_REGISTER, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read Copper Control Register failed", dev->name);
		return ret;
	}
	phy_data &= ~PHY_MRVL_COPPER_CONTROL_AUTONEG_ENABLE_BIT;
	ret = phy_marvell_alaska_write(dev, PHY_MRVL_COPPER_CONTROL_REGISTER, phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: write Copper Control Register failed", dev->name);
		return ret;
	}

	ret = phy_marvell_alaska_reset(dev);
	if (ret) {
		LOG_ERR("%s: configure PHY: reset PHY failed (1)", dev->name);
		return ret;
	}

	if ((dev_data->phy_id & PHY_MRVL_PHY_ID_MODEL_MASK) == PHY_MRVL_PHY_ID_MODEL_88E151X) {
		/*
		 * 88E151x only: configure the system interface and media type
		 * (i.e. "RGMII to Copper", 0x0). On the 88E1111, this setting
		 * is configured using I/O pins on the device.
		 * Page 18, register address 20 = General Control Register 1,
		 * bits [2..0] = mode configuration
		 * Comp. datasheet table 129 "General Control Register 1"
		 * NOTICE: a change of this value requires a subsequent software
		 * reset command via the same register's bit [15].
		 */
		ret = phy_marvell_alaska_write(dev, PHY_MRVL_COPPER_PAGE_SWITCH_REGISTER,
					       PHY_MRVL_GENERAL_CONTROL_1_PAGE);
		if (ret) {
			LOG_ERR("%s: configure PHY: write Page Switch Register failed", dev->name);
			return ret;
		}

		ret = phy_marvell_alaska_read(dev, PHY_MRVL_GENERAL_CONTROL_1_REGISTER, &phy_data);
		if (ret) {
			LOG_ERR("%s: configure PHY: read General Control Register 1 failed",
				dev->name);
			return ret;
		}
		phy_data &= ~(PHY_MRVL_MODE_CONFIG_MASK << PHY_MRVL_MODE_CONFIG_SHIFT);
		ret = phy_marvell_alaska_write(dev, PHY_MRVL_GENERAL_CONTROL_1_REGISTER, phy_data);
		if (ret) {
			LOG_ERR("%s: configure PHY: write General Control Register 1 failed",
				dev->name);
			return ret;
		}

		/*
		 * [15] Mode Software Reset bit, affecting pages 6 and 18
		 * Reset is performed immediately, bit [15] is self-clearing.
		 * This reset bit mirrors that in the Copper Control Register
		 * without the need for a prior register page switch.
		 */
		phy_data |= PHY_MRVL_GENERAL_CONTROL_1_RESET_BIT;
		ret = phy_marvell_alaska_write(dev, PHY_MRVL_GENERAL_CONTROL_1_REGISTER, phy_data);
		if (ret) {
			LOG_ERR("%s: configure PHY: write General Control Register 1 failed",
				dev->name);
			return ret;
		}

		/* Bit [15] reverts to 0 once the reset is complete. */
		while (((phy_data & PHY_MRVL_GENERAL_CONTROL_1_RESET_BIT) != 0) &&
		       (poll_cnt++ < 10)) {
			ret = phy_marvell_alaska_read(dev, PHY_MRVL_GENERAL_CONTROL_1_REGISTER,
						      &phy_data);
		}
		if (poll_cnt == 10) {
			LOG_ERR("%s: configure PHY: software-reset PHY completion timed out",
				dev->name);
			return -ETIMEDOUT;
		}

		/* Revert to register page 0 */
		ret = phy_marvell_alaska_write(dev, PHY_MRVL_COPPER_PAGE_SWITCH_REGISTER,
					       PHY_MRVL_BASE_REGISTERS_PAGE);
		if (ret) {
			LOG_ERR("%s: configure PHY: write Page Switch Register failed", dev->name);
			return ret;
		}
	}

	/*
	 * Configure MDIX
	 * 88E151x: Page 0, register address 16 = Copper specific control register 1,
	 * 88E1111: Page any, register address 16 = PHY specific control register,
	 * bits [6..5] = MDIO crossover mode. Comp. datasheet table 76.
	 * NOTICE: a change of this value requires a subsequent software
	 * reset command via Copper Control Register's bit [15].
	 */

	/* [6..5] 11 = Enable auto cross over detection */
	ret = phy_marvell_alaska_read(dev, PHY_MRVL_COPPER_CONTROL_1_REGISTER, &phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: read Copper spec. Control Register 1 failed",
			dev->name);
		return ret;
	}

	phy_data &= ~(PHY_MRVL_MDIX_CONFIG_MASK << PHY_MRVL_MDIX_CONFIG_SHIFT);
	phy_data |= (PHY_MRVL_MDIX_AUTO_CROSSOVER_ENABLE << PHY_MRVL_MDIX_CONFIG_SHIFT);
	ret = phy_marvell_alaska_write(dev, PHY_MRVL_COPPER_CONTROL_1_REGISTER, phy_data);
	if (ret) {
		LOG_ERR("%s: configure PHY: write Copper spec. Control Register 1 failed",
			dev->name);
		return ret;
	}

	/* Trigger a PHY Reset, affecting pages 0, 2, 3, 5, 7. */
	ret = phy_marvell_alaska_reset(dev);
	if (ret) {
		LOG_ERR("%s: configure PHY: reset PHY failed (2)", dev->name);
		return ret;
	}

	return 0;
}

static int phy_marvell_alaska_cfg_link(const struct device *dev, enum phy_link_speed speeds)
{
	struct marvell_alaska_data *dev_data = dev->data;

	int ret = 0;
	uint32_t phy_data;
	uint32_t phy_data_gbit;

	ret = k_mutex_lock(&dev_data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("%s: configure PHY link: mutex lock error", dev->name);
		return ret;
	}

	/* Cancel monitoring delayable work during link re-configuration */
	k_work_cancel_delayable(&dev_data->phy_monitor_work);

	/* Reset PHY */
	ret = phy_marvell_alaska_reset(dev);
	if (ret) {
		goto out;
	}

	/* Common configuration items */
	ret = phy_marvell_alaska_static_cfg(dev);
	if (ret) {
		goto out;
	}

	/*
	 * Advertise the link speed from the device configuration & perform
	 * auto-negotiation. This process involves:
	 *
	 * Page 0, register address 4 =
	 *     Copper Auto-Negotiation Advertisement Register,
	 * Page 0, register address 0 =
	 *     Copper Control Register, bit [15] = Reset -> apply all changes
	 *     made regarding advertisement,
	 * Page 0, register address 9 =
	 *     1000BASE-T Control Register (if link speed = 1GBit/s),
	 * Page 0, register address 1 =
	 *     Copper Status Register, bit [5] = Copper Auto-Negotiation
	 *     Complete.
	 *
	 * Comp. datasheet tables 68 & 73.
	 */

	phy_data = MII_ADVERTISE_SEL_IEEE_802_3;

	/*
	 * Read 1000BASE-T Control Register, mask out GBit bits,
	 * modify & write this register. 10/100 advertisement data
	 * in the ANAR register is assembled from scratch.
	 */
	ret = phy_marvell_alaska_read(dev, PHY_MRVL_1000BASET_CONTROL_REGISTER, &phy_data_gbit);
	if (ret) {
		goto out;
	}
	phy_data_gbit &= ~PHY_MRVL_ADVERTISE_1000_FULL;
	phy_data_gbit &= ~PHY_MRVL_ADVERTISE_1000_HALF;

	if (speeds & LINK_FULL_1000BASE) {
		phy_data_gbit |= PHY_MRVL_ADVERTISE_1000_FULL;
	}
	if (speeds & LINK_HALF_1000BASE) {
		phy_data_gbit |= PHY_MRVL_ADVERTISE_1000_HALF;
	}
	if (speeds & LINK_FULL_100BASE) {
		phy_data |= MII_ADVERTISE_100_FULL;
	}
	if (speeds & LINK_HALF_100BASE) {
		phy_data |= MII_ADVERTISE_100_HALF;
	}
	if (speeds & LINK_FULL_10BASE) {
		phy_data |= MII_ADVERTISE_10_FULL;
	}
	if (speeds & LINK_HALF_10BASE) {
		phy_data |= MII_ADVERTISE_10_HALF;
	}

	LOG_DBG("%s: configure PHY link: 1000CTRL 0x%08X ANAR 0x%08X", dev->name, phy_data_gbit,
		phy_data);

	ret = phy_marvell_alaska_write(dev, PHY_MRVL_1000BASET_CONTROL_REGISTER, phy_data_gbit);
	if (ret) {
		goto out;
	}
	ret = phy_marvell_alaska_write(dev, PHY_MRVL_COPPER_AUTONEG_ADV_REGISTER, phy_data);
	if (ret) {
		goto out;
	}

	/* Start auto-negotiation */
	ret = phy_marvell_alaska_autonegotiate(dev);
	if (ret) {
		LOG_ERR("%s: configure PHY link: auto-negotiation failed", dev->name);
	}

out:
	k_mutex_unlock(&dev_data->mutex);
	k_work_reschedule(&dev_data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	return ret;
}

static int phy_marvell_alaska_get_link(const struct device *dev, struct phy_link_state *state)
{
	struct marvell_alaska_data *dev_data = dev->data;

	int ret;
	uint32_t phy_data;
	uint32_t speed;
	bool fdx;
	bool link_up;

	struct phy_link_state old_state = dev_data->state;

	ret = k_mutex_lock(&dev_data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("%s: get PHY link state: mutex lock error", dev->name);
		return ret;
	}

	ret = phy_marvell_alaska_read(dev, PHY_MRVL_COPPER_STATUS_1_REGISTER, &phy_data);
	if (ret) {
		LOG_ERR("%s: get PHY link state: read Copper Specific Status Register 1 failed",
			dev->name);
		k_mutex_unlock(&dev_data->mutex);
		return ret;
	}

	/*
	 * Copper Specific Status Register 1 (88E15xx) /
	 * PHY Specific Status Register - Copper (88E1111):
	 *
	 * Link speed: [15 .. 14]
	 *   00b = 10 MBit/s
	 *   01b = 100 MBit/s
	 *   10b = 1 GBit/s
	 * Duplex: [13]
	 *   0b = half duplex
	 *   1b = full duplex
	 * Link State: [10]
	 *   0b = link down
	 *   1b = link up
	 */
	speed = (phy_data >> PHY_MRVL_LINK_SPEED_SHIFT) & PHY_MRVL_LINK_SPEED_MASK;
	fdx = (phy_data & PHY_MRVL_LINK_DUPLEX_FDX) ? true : false;
	link_up = (phy_data & PHY_MRVL_LINK_STATUS) ? true : false;

	if (speed == PHY_MRVL_LINK_SPEED_10MBIT) {
		if (fdx) {
			state->speed = LINK_FULL_10BASE;
		} else {
			state->speed = LINK_HALF_10BASE;
		}
	} else if (speed == PHY_MRVL_LINK_SPEED_100MBIT) {
		if (fdx) {
			state->speed = LINK_FULL_100BASE;
		} else {
			state->speed = LINK_HALF_100BASE;
		}
	} else if (speed == PHY_MRVL_LINK_SPEED_1GBIT) {
		if (fdx) {
			state->speed = LINK_FULL_1000BASE;
		} else {
			state->speed = LINK_HALF_1000BASE;
		}
	}
	state->is_up = link_up;

	k_mutex_unlock(&dev_data->mutex);

	if (memcmp(&old_state, state, sizeof(struct phy_link_state)) != 0) {
		LOG_DBG("%s: PHY link is %s", dev->name, state->is_up ? "up" : "down");
		if (state->is_up) {
			LOG_DBG("%s: PHY configured for %s MBit/s %s", dev->name,
				PHY_LINK_IS_SPEED_1000M(state->speed)  ? "1000"
				: PHY_LINK_IS_SPEED_100M(state->speed) ? "100"
								       : "10",
				PHY_LINK_IS_FULL_DUPLEX(state->speed) ? "FDX" : "HDX");
		}
	}

	return ret;
}

static int phy_marvell_alaska_link_cb_set(const struct device *dev, phy_callback_t cb,
					  void *user_data)
{
	struct marvell_alaska_data *dev_data = dev->data;

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
	phy_marvell_alaska_get_link(dev, &dev_data->state);
	dev_data->cb(dev, &dev_data->state, dev_data->cb_data);

	return 0;
}

static void phy_marvell_alaska_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct marvell_alaska_data *dev_data =
		CONTAINER_OF(dwork, struct marvell_alaska_data, phy_monitor_work);
	const struct device *dev = dev_data->dev;

	struct phy_link_state state = {};
	int ret = phy_marvell_alaska_get_link(dev, &state);

	if (ret == 0 && memcmp(&state, &dev_data->state, sizeof(struct phy_link_state)) != 0) {
		memcpy(&dev_data->state, &state, sizeof(struct phy_link_state));
		if (dev_data->cb) {
			dev_data->cb(dev, &dev_data->state, dev_data->cb_data);
		}
	}

	k_work_reschedule(&dev_data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_marvell_alaska_init(const struct device *dev)
{
	const struct marvell_alaska_config *const dev_conf = dev->config;
	struct marvell_alaska_data *dev_data = dev->data;

	int ret;

	dev_data->dev = dev;

	ret = k_mutex_init(&dev_data->mutex);
	if (ret) {
		LOG_ERR("%s: init PHY: initialize mutex failed", dev->name);
		return ret;
	}

	mdio_bus_enable(dev_conf->mdio_dev);

	k_work_init_delayable(&dev_data->phy_monitor_work, phy_marvell_alaska_monitor_work_handler);
	k_work_reschedule(&dev_data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	LOG_DBG("%s: init PHY: completed", dev->name);

	return 0;
}

static DEVICE_API(ethphy, marvell_alaska_phy_api) = {
	.get_link = phy_marvell_alaska_get_link,
	.cfg_link = phy_marvell_alaska_cfg_link,
	.link_cb_set = phy_marvell_alaska_link_cb_set,
	.read = phy_marvell_alaska_read,
	.write = phy_marvell_alaska_write,
};

#define PHY_MARVELL_ALASKA_DEV_CONFIG(n)                                                           \
	static const struct marvell_alaska_config marvell_alaska_##n##_config = {                  \
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};

#define PHY_MARVELL_ALASKA_DEV_DATA(n)                                                             \
	static struct marvell_alaska_data marvell_alaska_##n##_data = {                            \
		.phy_id = 0,                                                                       \
	};

#define PHY_MARVELL_ALASKA_DEV_INIT(n)                                                             \
	DEVICE_DT_INST_DEFINE(n, phy_marvell_alaska_init, NULL, &marvell_alaska_##n##_data,        \
			      &marvell_alaska_##n##_config, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, \
			      &marvell_alaska_phy_api);

#define PHY_MARVELL_ALASKA_INITIALIZE(n)                                                           \
	PHY_MARVELL_ALASKA_DEV_CONFIG(n);                                                          \
	PHY_MARVELL_ALASKA_DEV_DATA(n);                                                            \
	PHY_MARVELL_ALASKA_DEV_INIT(n);

DT_INST_FOREACH_STATUS_OKAY(PHY_MARVELL_ALASKA_INITIALIZE)
