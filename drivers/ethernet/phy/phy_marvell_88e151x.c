/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT marvell_88e151x

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_marvell_88e151x, CONFIG_PHY_LOG_LEVEL);

#include "phy_mii.h"

enum marvell_88e151x_phy_connection_type {
	MARVELL_88E151X_RGMII,
	MARVELL_88E151X_RGMII_TX_ID,
	MARVELL_88E151X_RGMII_RX_ID,
	MARVELL_88E151X_RGMII_ID,
};

struct marvell_88e151x_dev_config {
	uint8_t phy_addr;
	bool no_reset;
	enum phy_link_speed default_speeds;
	enum marvell_88e151x_phy_connection_type phy_connection_type;
	const struct device *const mdio;
};

struct marvell_88e151x_dev_data {
	const struct device *dev;
	phy_callback_t cb;
	void *cb_data;
	struct phy_link_state state;
	struct k_mutex mutex;
	struct k_work_delayable monitor_work;
	bool gigabit_supported;
	bool autoneg_in_progress;
	k_timepoint_t autoneg_timeout;
};

/* Offset to align capabilities bits of 1000BASE-T Control and Status regs */
#define MARVELL_88e151X_MII_1KSTSR_OFFSET 2

/* OUI + model; ignore revision nibble (88E151x family). */
#define MARVELL_88e151X_PHY_ID      0x01410DD0
#define MARVELL_88e151X_PHY_ID_MASK 0xFFFFFFF0

/* How often to poll auto-negotiation status while waiting for it to complete */
#define MARVELL_88e151X_MII_AUTONEG_POLL_INTERVAL_MS 100

static void marvell_88e151x_invoke_link_cb(const struct device *dev);

static int marvell_88e151x_check_autonegotiation_completion(const struct device *dev);

static inline int marvell_88e151x_reg_read(const struct device *dev, uint16_t reg_addr,
					   uint16_t *value)
{
	const struct marvell_88e151x_dev_config *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg_addr, value);
}

static inline int marvell_88e151x_reg_write(const struct device *dev, uint16_t reg_addr,
					    uint16_t value)
{
	const struct marvell_88e151x_dev_config *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg_addr, value);
}

static int marvell_88e151x_read_gigabit_supported_flag(const struct device *dev, bool *supported)
{
	uint16_t bmsr_reg;
	uint16_t estat_reg;

	if (marvell_88e151x_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if ((bmsr_reg & MII_BMSR_EXTEND_STATUS) != 0U) {
		if (marvell_88e151x_reg_read(dev, MII_ESTAT, &estat_reg) < 0) {
			return -EIO;
		}

		if ((estat_reg & (MII_ESTAT_1000BASE_T_HALF | MII_ESTAT_1000BASE_T_FULL)) != 0U) {
			*supported = true;
			return 0;
		}
	}

	*supported = false;
	return 0;
}

static void marvell_88e151x_phy_setup(const struct device *dev)
{
#define MII_MARVELL_PHY_PAGE        22
#define MIIM_88E151x_PHY_MSCR       21
#define MIIM_88E151x_RGMII_TX_DELAY BIT(4)
#define MIIM_88E151x_RGMII_RX_DELAY BIT(5)

	uint16_t value = 0;
	const struct marvell_88e151x_dev_config *const cfg = dev->config;

	marvell_88e151x_reg_write(dev, MII_MARVELL_PHY_PAGE, 2);
	marvell_88e151x_reg_read(dev, MIIM_88E151x_PHY_MSCR, &value);
	/* RGMII TX/RX internal delay settings */
	if (cfg->phy_connection_type == MARVELL_88E151X_RGMII) {
		value &= ~MIIM_88E151x_RGMII_TX_DELAY;
		value &= ~MIIM_88E151x_RGMII_RX_DELAY;
	} else if (cfg->phy_connection_type == MARVELL_88E151X_RGMII_TX_ID) {
		value |= MIIM_88E151x_RGMII_TX_DELAY;
		value &= ~MIIM_88E151x_RGMII_RX_DELAY;
	} else if (cfg->phy_connection_type == MARVELL_88E151X_RGMII_RX_ID) {
		value &= ~MIIM_88E151x_RGMII_TX_DELAY;
		value |= MIIM_88E151x_RGMII_RX_DELAY;
	} else if (cfg->phy_connection_type == MARVELL_88E151X_RGMII_ID) {
		value |= MIIM_88E151x_RGMII_TX_DELAY;
		value |= MIIM_88E151x_RGMII_RX_DELAY;
	}
	marvell_88e151x_reg_write(dev, MIIM_88E151x_PHY_MSCR, value);
	marvell_88e151x_reg_write(dev, MII_MARVELL_PHY_PAGE, 0);
}

static int marvell_88e151x_reset(const struct device *dev)
{
	uint32_t timeout = 12U;
	uint16_t value;

	marvell_88e151x_phy_setup(dev);

	/* Issue a soft reset */
	if (marvell_88e151x_reg_write(dev, MII_BMCR, MII_BMCR_RESET) < 0) {
		return -EIO;
	}

	/* Wait up to 0.6s for the reset sequence to finish. According to
	 * IEEE 802.3, Section 2, Subsection 22.2.4.1.1 a PHY reset may take
	 * up to 0.5 s.
	 */
	do {
		if (timeout-- == 0U) {
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(50));

		if (marvell_88e151x_reg_read(dev, MII_BMCR, &value) < 0) {
			return -EIO;
		}
	} while ((value & MII_BMCR_RESET) != 0U);

	return 0;
}

static int marvell_88e151x_get_id(const struct device *dev, uint32_t *phy_id)
{
	uint16_t value;

	if (marvell_88e151x_reg_read(dev, MII_PHYID1R, &value) < 0) {
		return -EIO;
	}

	*phy_id = value << 16;

	if (marvell_88e151x_reg_read(dev, MII_PHYID2R, &value) < 0) {
		return -EIO;
	}

	*phy_id |= value;

	return 0;
}

static int marvell_88e151x_update_link_state(const struct device *dev)
{
	const struct marvell_88e151x_dev_config *const cfg = dev->config;
	struct marvell_88e151x_dev_data *const data = dev->data;
	bool link_up;

	uint16_t bmcr_reg = 0;
	uint16_t bmsr_reg = 0;

	if (marvell_88e151x_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	link_up = IS_BIT_SET(bmsr_reg, MII_BMSR_LINK_STATUS_BIT);
	/* If link is down, we can stop here. */
	if (!link_up) {
		data->state.speed = 0;
		if (link_up != data->state.is_up) {
			data->state.is_up = false;
			LOG_INF("PHY (%d) is down", cfg->phy_addr);
			return 0;
		}
		return -EAGAIN;
	}

	if (marvell_88e151x_reg_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	/* If auto-negotiation is not enabled, we only need to check the link speed */
	if (!IS_BIT_SET(bmcr_reg, MII_BMCR_AUTONEG_ENABLE_BIT)) {
		enum phy_link_speed new_speed = phy_mii_get_link_speed_bmcr_reg(dev, bmcr_reg);

		if (data->state.speed != new_speed || !data->state.is_up) {
			data->state.is_up = true;
			data->state.speed = new_speed;

			LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", cfg->phy_addr,
				PHY_LINK_IS_SPEED_1000M(data->state.speed)
					? "1000"
					: (PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100"
										     : "10"),
				PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

			return 0;
		}
		return -EAGAIN;
	}

	/* If auto-negotiation is enabled and the link was already up last time we checked,
	 * we can return immediately, as the link state has not changed.
	 * If the link was down, we will start the auto-negotiation sequence.
	 */
	if (data->state.is_up) {
		return -EAGAIN;
	}

	data->state.is_up = true;

	LOG_DBG("PHY (%d) Starting MII PHY auto-negotiate sequence", cfg->phy_addr);

	data->autoneg_timeout = sys_timepoint_calc(K_MSEC(CONFIG_PHY_AUTONEG_TIMEOUT_MS));

	return marvell_88e151x_check_autonegotiation_completion(dev);
}

static int marvell_88e151x_check_autonegotiation_completion(const struct device *dev)
{
	const struct marvell_88e151x_dev_config *const cfg = dev->config;
	struct marvell_88e151x_dev_data *const data = dev->data;

	uint16_t anar_reg = 0;
	uint16_t bmsr_reg = 0;
	uint16_t anlpar_reg = 0;
	uint16_t c1kt_reg = 0;
	uint16_t s1kt_reg = 0;

	if (marvell_88e151x_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if (!IS_BIT_SET(bmsr_reg, MII_BMSR_AUTONEG_COMPLETE_BIT)) {
		if (sys_timepoint_expired(data->autoneg_timeout)) {
			LOG_DBG("PHY (%d) auto-negotiate timeout", cfg->phy_addr);
			return -ETIMEDOUT;
		}
		return -EINPROGRESS;
	}

	/* Link status bit is latched low, so read it again to get current status */
	if (unlikely(!IS_BIT_SET(bmsr_reg, MII_BMSR_LINK_STATUS_BIT))) {
		/* Second read, clears the latched bits and gives the correct status */
		if (marvell_88e151x_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
			return -EIO;
		}

		if (!IS_BIT_SET(bmsr_reg, MII_BMSR_LINK_STATUS_BIT)) {
			return -EAGAIN;
		}
	}

	LOG_DBG("PHY (%d) auto-negotiate sequence completed", cfg->phy_addr);

	/* Read PHY default advertising parameters */
	if (marvell_88e151x_reg_read(dev, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}

	/** Read peer device capability */
	if (marvell_88e151x_reg_read(dev, MII_ANLPAR, &anlpar_reg) < 0) {
		return -EIO;
	}

	if (data->gigabit_supported) {
		if (marvell_88e151x_reg_read(dev, MII_1KTCR, &c1kt_reg) < 0) {
			return -EIO;
		}
		if (marvell_88e151x_reg_read(dev, MII_1KSTSR, &s1kt_reg) < 0) {
			return -EIO;
		}
		s1kt_reg = (uint16_t)(s1kt_reg >> MARVELL_88e151X_MII_1KSTSR_OFFSET);
	}

	if (data->gigabit_supported && ((c1kt_reg & s1kt_reg & MII_ADVERTISE_1000_FULL) != 0U)) {
		data->state.speed = LINK_FULL_1000BASE;
	} else if (data->gigabit_supported &&
		   ((c1kt_reg & s1kt_reg & MII_ADVERTISE_1000_HALF) != 0U)) {
		data->state.speed = LINK_HALF_1000BASE;
	} else if ((anar_reg & anlpar_reg & MII_ADVERTISE_100_FULL) != 0U) {
		data->state.speed = LINK_FULL_100BASE;
	} else if ((anar_reg & anlpar_reg & MII_ADVERTISE_100_HALF) != 0U) {
		data->state.speed = LINK_HALF_100BASE;
	} else if ((anar_reg & anlpar_reg & MII_ADVERTISE_10_FULL) != 0U) {
		data->state.speed = LINK_FULL_10BASE;
	} else {
		data->state.speed = LINK_HALF_10BASE;
	}

	data->state.is_up = true;

	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", cfg->phy_addr,
		PHY_LINK_IS_SPEED_1000M(data->state.speed)
			? "1000"
			: (PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
		PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

	return 0;
}

static void marvell_88e151x_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct marvell_88e151x_dev_data *const data =
		CONTAINER_OF(dwork, struct marvell_88e151x_dev_data, monitor_work);
	const struct device *dev = data->dev;
	int rc;

	if (k_mutex_lock(&data->mutex, K_NO_WAIT) == 0) {
		if (data->autoneg_in_progress) {
			rc = marvell_88e151x_check_autonegotiation_completion(dev);
		} else {
			/* If autonegotiation is not in progress, just update the link state */
			rc = marvell_88e151x_update_link_state(dev);
		}

		data->autoneg_in_progress = (rc == -EINPROGRESS);

		/* If link state has changed and a callback is set, invoke callback */
		if (rc == 0) {
			marvell_88e151x_invoke_link_cb(dev);
		}

		k_mutex_unlock(&data->mutex);
	}

	k_work_reschedule(&data->monitor_work,
			  data->autoneg_in_progress
				  ? K_MSEC(MARVELL_88e151X_MII_AUTONEG_POLL_INTERVAL_MS)
				  : K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int marvell_88e151x_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	return marvell_88e151x_reg_read(dev, reg_addr, (uint16_t *)data);
}

static int marvell_88e151x_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	return marvell_88e151x_reg_write(dev, reg_addr, (uint16_t)data);
}

static int marvell_88e151x_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds,
				    enum phy_cfg_link_flag flags)
{
	struct marvell_88e151x_dev_data *const data = dev->data;
	const struct marvell_88e151x_dev_config *const cfg = dev->config;
	int ret = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if ((flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED) != 0U) {
		/* If auto-negotiation is disabled, only one speed can be selected.
		 * If gigabit is not supported, this speed must not be 1000M.
		 */
		if (!data->gigabit_supported && PHY_LINK_IS_SPEED_1000M(adv_speeds)) {
			LOG_ERR("PHY (%d) Gigabit not supported, can't configure link",
				cfg->phy_addr);
			ret = -ENOTSUP;
			goto cfg_link_end;
		}

		ret = phy_mii_set_bmcr_reg_autoneg_disabled(dev, adv_speeds);
		if (ret >= 0) {
			data->autoneg_in_progress = false;
			k_work_reschedule(&data->monitor_work, K_NO_WAIT);
		}
	} else {
		ret = phy_mii_cfg_link_autoneg(dev, adv_speeds, data->gigabit_supported);
		if (ret >= 0) {
			LOG_DBG("PHY (%d) Starting MII PHY auto-negotiate sequence", cfg->phy_addr);
			data->autoneg_in_progress = true;
			data->autoneg_timeout =
				sys_timepoint_calc(K_MSEC(CONFIG_PHY_AUTONEG_TIMEOUT_MS));
			k_work_reschedule(&data->monitor_work,
					  K_MSEC(MARVELL_88e151X_MII_AUTONEG_POLL_INTERVAL_MS));
		}
	}

	if (ret == -EALREADY) {
		LOG_DBG("PHY (%d) Link already configured", cfg->phy_addr);
	}

cfg_link_end:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int marvell_88e151x_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct marvell_88e151x_dev_data *const data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	if (state->speed == 0) {
		/* If speed is 0, then link is also down, happens when autonegotiation is in
		 * progress
		 */
		state->is_up = false;
	}

	k_mutex_unlock(&data->mutex);

	return 0;
}

static void marvell_88e151x_invoke_link_cb(const struct device *dev)
{
	struct marvell_88e151x_dev_data *const data = dev->data;
	struct phy_link_state state;

	if (data->cb == NULL) {
		return;
	}

	marvell_88e151x_get_link_state(dev, &state);

	data->cb(dev, &state, data->cb_data);
}

static int marvell_88e151x_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct marvell_88e151x_dev_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	/**
	 * Immediately invoke the callback to notify the caller of the
	 * current link status.
	 */
	marvell_88e151x_invoke_link_cb(dev);

	return 0;
}

static int marvell_88e151x_init(const struct device *dev)
{
	const struct marvell_88e151x_dev_config *const cfg = dev->config;
	struct marvell_88e151x_dev_data *const data = dev->data;
	uint32_t phy_id;
	int ret = 0;

	data->dev = dev;

	k_mutex_init(&data->mutex);

	if (!cfg->no_reset) {
		ret = marvell_88e151x_reset(dev);
		if (ret < 0) {
			LOG_ERR("Failed to reset PHY (%d): %d", cfg->phy_addr, ret);
			return ret;
		}
	}

	if (marvell_88e151x_get_id(dev, &phy_id) == 0) {
		if ((phy_id & MARVELL_88e151X_PHY_ID_MASK) !=
		    (MARVELL_88e151X_PHY_ID & MARVELL_88e151X_PHY_ID_MASK)) {
			LOG_ERR("PHY found at address %d is %x while %x is expected", cfg->phy_addr,
				phy_id, MARVELL_88e151X_PHY_ID);

			return -EINVAL;
		}

		LOG_INF("PHY (%d) ID %X", cfg->phy_addr, phy_id);
	}

	ret = marvell_88e151x_read_gigabit_supported_flag(dev, &data->gigabit_supported);
	if (ret < 0) {
		LOG_ERR("Failed to read PHY capabilities: %d", ret);
		return ret;
	}

	k_work_init_delayable(&data->monitor_work, marvell_88e151x_monitor_work_handler);

	/* Advertise default speeds */
	ret = marvell_88e151x_cfg_link(dev, cfg->default_speeds, 0);
	if (ret == -EALREADY) {
		data->autoneg_in_progress = true;
		data->autoneg_timeout = sys_timepoint_calc(K_MSEC(CONFIG_PHY_AUTONEG_TIMEOUT_MS));
	}

	/* This will schedule the monitor work, if not already scheduled by
	 * marvell_88e151x_cfg_link().
	 */
	k_work_schedule(&data->monitor_work, K_NO_WAIT);

	return 0;
}

static DEVICE_API(ethphy, marvell_88e151x_driver_api) = {
	.get_link = marvell_88e151x_get_link_state,
	.link_cb_set = marvell_88e151x_link_cb_set,
	.cfg_link = marvell_88e151x_cfg_link,
	.read = marvell_88e151x_read,
	.write = marvell_88e151x_write,
};

#define MARVELL_88e151X_CONFIG(n)                                                                  \
	BUILD_ASSERT(PHY_INST_GENERATE_DEFAULT_SPEEDS(n) != 0,                                     \
		     "At least one valid speed must be configured for this driver");               \
                                                                                                   \
	static const struct marvell_88e151x_dev_config marvell_88e151x_dev_config_##n = {          \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.no_reset = DT_INST_PROP(n, no_reset),                                             \
		.default_speeds = PHY_INST_GENERATE_DEFAULT_SPEEDS(n),                             \
		.phy_connection_type = DT_INST_ENUM_IDX(n, phy_connection_type),                   \
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n))};

#define MARVELL_88e151X_DATA(n) static struct marvell_88e151x_dev_data marvell_88e151x_dev_data_##n;

#define MARVELL_88e151X_DEVICE(n)                                                                  \
	MARVELL_88e151X_CONFIG(n);                                                                 \
	MARVELL_88e151X_DATA(n);                                                                   \
	DEVICE_DT_INST_DEFINE(n, &marvell_88e151x_init, NULL, &marvell_88e151x_dev_data_##n,       \
			      &marvell_88e151x_dev_config_##n, POST_KERNEL,                        \
			      CONFIG_PHY_INIT_PRIORITY, &marvell_88e151x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MARVELL_88e151X_DEVICE)
