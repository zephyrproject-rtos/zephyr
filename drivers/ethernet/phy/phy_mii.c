/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ethernet_phy

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_mii, CONFIG_PHY_LOG_LEVEL);

struct phy_mii_dev_config {
	uint8_t phy_addr;
	bool no_reset;
	bool fixed;
	int fixed_speed;
	const struct device * const mdio;
};

struct phy_mii_dev_data {
	const struct device *dev;
	phy_callback_t cb;
	void *cb_data;
	struct k_work_delayable monitor_work;
	struct phy_link_state state;
	struct k_sem sem;
	bool gigabit_supported;
};

/* Offset to align capabilities bits of 1000BASE-T Control and Status regs */
#define MII_1KSTSR_OFFSET 2

static int phy_mii_get_link_state(const struct device *dev,
				  struct phy_link_state *state);

static inline int reg_read(const struct device *dev, uint16_t reg_addr,
			   uint16_t *value)
{
	const struct phy_mii_dev_config *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg_addr, value);
}

static inline int reg_write(const struct device *dev, uint16_t reg_addr,
			    uint16_t value)
{
	const struct phy_mii_dev_config *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg_addr, value);
}

static bool is_gigabit_supported(const struct device *dev)
{
	uint16_t bmsr_reg;
	uint16_t estat_reg;

	if (reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if (bmsr_reg & MII_BMSR_EXTEND_STATUS) {
		if (reg_read(dev, MII_ESTAT, &estat_reg) < 0) {
			return -EIO;
		}

		if (estat_reg & (MII_ESTAT_1000BASE_T_HALF
				 | MII_ESTAT_1000BASE_T_FULL)) {
			return true;
		}
	}

	return false;
}

static int reset(const struct device *dev)
{
	uint32_t timeout = 12U;
	uint16_t value;

	/* Issue a soft reset */
	if (reg_write(dev, MII_BMCR, MII_BMCR_RESET) < 0) {
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

		if (reg_read(dev, MII_BMCR, &value) < 0) {
			return -EIO;
		}
	} while (value & MII_BMCR_RESET);

	return 0;
}

static int get_id(const struct device *dev, uint32_t *phy_id)
{
	uint16_t value;

	if (reg_read(dev, MII_PHYID1R, &value) < 0) {
		return -EIO;
	}

	*phy_id = (value & 0xFFFF) << 16;

	if (reg_read(dev, MII_PHYID2R, &value) < 0) {
		return -EIO;
	}

	*phy_id |= (value & 0xFFFF);

	return 0;
}

static int update_link_state(const struct device *dev)
{
	const struct phy_mii_dev_config *const cfg = dev->config;
	struct phy_mii_dev_data *const data = dev->data;
	bool link_up;

	uint16_t anar_reg = 0;
	uint16_t bmcr_reg = 0;
	uint16_t bmsr_reg = 0;
	uint16_t anlpar_reg = 0;
	uint16_t c1kt_reg = 0;
	uint16_t s1kt_reg = 0;
	uint32_t timeout = CONFIG_PHY_AUTONEG_TIMEOUT_MS / 100;

	if (reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	link_up = bmsr_reg & MII_BMSR_LINK_STATUS;

	/* If there is no change in link state don't proceed. */
	if (link_up == data->state.is_up) {
		return -EAGAIN;
	}

	data->state.is_up = link_up;

	/* If link is down, there is nothing more to be done */
	if (data->state.is_up == false) {
		return 0;
	}

	/**
	 * Perform auto-negotiation sequence.
	 */
	LOG_DBG("PHY (%d) Starting MII PHY auto-negotiate sequence",
		cfg->phy_addr);

	/* Read PHY default advertising parameters */
	if (reg_read(dev, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}

	/* Configure and start auto-negotiation process */
	if (reg_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	bmcr_reg |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	bmcr_reg &= ~MII_BMCR_ISOLATE;  /* Don't isolate the PHY */

	if (reg_write(dev, MII_BMCR, bmcr_reg) < 0) {
		return -EIO;
	}

	/* Wait for the auto-negotiation process to complete */
	do {
		if (timeout-- == 0U) {
			LOG_DBG("PHY (%d) auto-negotiate timedout",
				cfg->phy_addr);
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(100));

		if (reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
			return -EIO;
		}
	} while (!(bmsr_reg & MII_BMSR_AUTONEG_COMPLETE));

	LOG_DBG("PHY (%d) auto-negotiate sequence completed",
		cfg->phy_addr);

	/** Read peer device capability */
	if (reg_read(dev, MII_ANLPAR, &anlpar_reg) < 0) {
		return -EIO;
	}

	if (data->gigabit_supported) {
		if (reg_read(dev, MII_1KTCR, &c1kt_reg) < 0) {
			return -EIO;
		}
		if (reg_read(dev, MII_1KSTSR, &s1kt_reg) < 0) {
			return -EIO;
		}
		s1kt_reg = (uint16_t)(s1kt_reg >> MII_1KSTSR_OFFSET);
	}

	if (data->gigabit_supported &&
			((c1kt_reg & s1kt_reg) & MII_ADVERTISE_1000_FULL)) {
		data->state.speed = LINK_FULL_1000BASE_T;
	} else if (data->gigabit_supported &&
			((c1kt_reg & s1kt_reg) & MII_ADVERTISE_1000_HALF)) {
		data->state.speed = LINK_HALF_1000BASE_T;
	} else if ((anar_reg & anlpar_reg) & MII_ADVERTISE_100_FULL) {
		data->state.speed = LINK_FULL_100BASE_T;
	} else if ((anar_reg & anlpar_reg) & MII_ADVERTISE_100_HALF) {
		data->state.speed = LINK_HALF_100BASE_T;
	} else if ((anar_reg & anlpar_reg) & MII_ADVERTISE_10_FULL) {
		data->state.speed = LINK_FULL_10BASE_T;
	} else {
		data->state.speed = LINK_HALF_10BASE_T;
	}

	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex\n",
		cfg->phy_addr,
		PHY_LINK_IS_SPEED_1000M(data->state.speed) ? "1000" :
		(PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
		PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

	return 0;
}

static void invoke_link_cb(const struct device *dev)
{
	struct phy_mii_dev_data *const data = dev->data;
	struct phy_link_state state;

	if (data->cb == NULL) {
		return;
	}

	phy_mii_get_link_state(dev, &state);

	data->cb(data->dev, &state, data->cb_data);
}

static void monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_mii_dev_data *const data =
		CONTAINER_OF(dwork, struct phy_mii_dev_data, monitor_work);
	const struct device *dev = data->dev;
	int rc;

	k_sem_take(&data->sem, K_FOREVER);

	rc = update_link_state(dev);

	k_sem_give(&data->sem);

	/* If link state has changed and a callback is set, invoke callback */
	if (rc == 0) {
		invoke_link_cb(dev);
	}

	/* Submit delayed work */
	k_work_reschedule(&data->monitor_work,
			  K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_mii_read(const struct device *dev, uint16_t reg_addr,
			uint32_t *data)
{
	return reg_read(dev, reg_addr, (uint16_t *)data);
}

static int phy_mii_write(const struct device *dev, uint16_t reg_addr,
			 uint32_t data)
{
	return reg_write(dev, reg_addr, (uint16_t)data);
}

static int phy_mii_cfg_link(const struct device *dev,
			    enum phy_link_speed adv_speeds)
{
	struct phy_mii_dev_data *const data = dev->data;
	uint16_t anar_reg;
	uint16_t bmcr_reg;
	uint16_t c1kt_reg;

	if (reg_read(dev, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}

	if (reg_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	if (data->gigabit_supported) {
		if (reg_read(dev, MII_1KTCR, &c1kt_reg) < 0) {
			return -EIO;
		}
	}

	if (adv_speeds & LINK_FULL_10BASE_T) {
		anar_reg |= MII_ADVERTISE_10_FULL;
	} else {
		anar_reg &= ~MII_ADVERTISE_10_FULL;
	}

	if (adv_speeds & LINK_HALF_10BASE_T) {
		anar_reg |= MII_ADVERTISE_10_HALF;
	} else {
		anar_reg &= ~MII_ADVERTISE_10_HALF;
	}

	if (adv_speeds & LINK_FULL_100BASE_T) {
		anar_reg |= MII_ADVERTISE_100_FULL;
	} else {
		anar_reg &= ~MII_ADVERTISE_100_FULL;
	}

	if (adv_speeds & LINK_HALF_100BASE_T) {
		anar_reg |= MII_ADVERTISE_100_HALF;
	} else {
		anar_reg &= ~MII_ADVERTISE_100_HALF;
	}

	if (data->gigabit_supported) {
		if (adv_speeds & LINK_FULL_1000BASE_T)
			c1kt_reg |= MII_ADVERTISE_1000_FULL;
		else
			c1kt_reg &= ~MII_ADVERTISE_1000_FULL;

		if (adv_speeds & LINK_HALF_1000BASE_T)
			c1kt_reg |= MII_ADVERTISE_1000_HALF;
		else
			c1kt_reg &= ~MII_ADVERTISE_1000_HALF;

		if (reg_write(dev, MII_1KTCR, c1kt_reg) < 0) {
			return -EIO;
		}
	}

	bmcr_reg |= MII_BMCR_AUTONEG_ENABLE;

	if (reg_write(dev, MII_ANAR, anar_reg) < 0) {
		return -EIO;
	}

	if (reg_write(dev, MII_BMCR, bmcr_reg) < 0) {
		return -EIO;
	}

	return 0;
}

static int phy_mii_get_link_state(const struct device *dev,
				  struct phy_link_state *state)
{
	struct phy_mii_dev_data *const data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	k_sem_give(&data->sem);

	return 0;
}

static int phy_mii_link_cb_set(const struct device *dev, phy_callback_t cb,
			       void *user_data)
{
	struct phy_mii_dev_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	/**
	 * Immediately invoke the callback to notify the caller of the
	 * current link status.
	 */
	invoke_link_cb(dev);

	return 0;
}

static int phy_mii_initialize(const struct device *dev)
{
	const struct phy_mii_dev_config *const cfg = dev->config;
	struct phy_mii_dev_data *const data = dev->data;
	uint32_t phy_id;

	k_sem_init(&data->sem, 1, 1);

	data->dev = dev;
	data->cb = NULL;

	/**
	 * If this is a *fixed* link then we don't need to communicate
	 * with a PHY. We set the link parameters as configured
	 * and set link state to up.
	 */
	if (cfg->fixed) {
		const static int speed_to_phy_link_speed[] = {
			LINK_HALF_10BASE_T,
			LINK_FULL_10BASE_T,
			LINK_HALF_100BASE_T,
			LINK_FULL_100BASE_T,
			LINK_HALF_1000BASE_T,
			LINK_FULL_1000BASE_T,
		};

		data->state.speed = speed_to_phy_link_speed[cfg->fixed_speed];
		data->state.is_up = true;
	} else {
		data->state.is_up = false;

		mdio_bus_enable(cfg->mdio);

		if (cfg->no_reset == false) {
			reset(dev);
		}

		if (get_id(dev, &phy_id) == 0) {
			if (phy_id == 0xFFFFFF) {
				LOG_ERR("No PHY found at address %d",
					cfg->phy_addr);

				return -EINVAL;
			}

			LOG_INF("PHY (%d) ID %X\n", cfg->phy_addr, phy_id);
		}

		data->gigabit_supported = is_gigabit_supported(dev);

		/* Advertise all speeds */
		phy_mii_cfg_link(dev, LINK_HALF_10BASE_T |
				      LINK_FULL_10BASE_T |
				      LINK_HALF_100BASE_T |
				      LINK_FULL_100BASE_T |
				      LINK_HALF_1000BASE_T |
				      LINK_FULL_1000BASE_T);

		k_work_init_delayable(&data->monitor_work,
					monitor_work_handler);

		monitor_work_handler(&data->monitor_work.work);
	}

	return 0;
}

#define IS_FIXED_LINK(n)	DT_INST_NODE_HAS_PROP(n, fixed_link)

static const struct ethphy_driver_api phy_mii_driver_api = {
	.get_link = phy_mii_get_link_state,
	.cfg_link = phy_mii_cfg_link,
	.link_cb_set = phy_mii_link_cb_set,
	.read = phy_mii_read,
	.write = phy_mii_write,
};

#define PHY_MII_CONFIG(n)						 \
static const struct phy_mii_dev_config phy_mii_dev_config_##n = {	 \
	.phy_addr = DT_INST_PROP(n, address),				 \
	.fixed = IS_FIXED_LINK(n),					 \
	.fixed_speed = DT_INST_ENUM_IDX_OR(n, fixed_link, 0),		 \
	.mdio = UTIL_AND(UTIL_NOT(IS_FIXED_LINK(n)),			 \
			 DEVICE_DT_GET(DT_INST_PHANDLE(n, mdio)))	 \
};

#define PHY_MII_DEVICE(n)						\
	PHY_MII_CONFIG(n);						\
	static struct phy_mii_dev_data phy_mii_dev_data_##n;		\
	DEVICE_DT_INST_DEFINE(n,					\
			      &phy_mii_initialize,			\
			      NULL,					\
			      &phy_mii_dev_data_##n,			\
			      &phy_mii_dev_config_##n, POST_KERNEL,	\
			      CONFIG_PHY_INIT_PRIORITY,			\
			      &phy_mii_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PHY_MII_DEVICE)
