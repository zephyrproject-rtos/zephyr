/*
 * Copyright (c) 2021 IP-Logix Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT phy_mii

#include <errno.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/mdio.h>
#include <net/phy.h>
#include <net/mii.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(phy_mii, CONFIG_PHY_LOG_LEVEL);

struct phy_mii_dev_config {
	uint8_t phy_addr;
	bool no_reset;
	bool fixed;
	int fixed_speed;
	const struct device * const mdio;
};

struct phy_mii_dev_data {
	phy_callback_t cb;
	void *cb_data;
	struct k_work_delayable monitor_work;
	struct phy_link_state state;
	struct k_sem sem;
	const struct device *me;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_DATA(dev) ((struct phy_mii_dev_data *const)(dev)->data)
#define DEV_CFG(dev) \
	((const struct phy_mii_dev_config *const)(dev)->config)

static inline int reg_read(const struct device *dev, uint16_t reg_addr,
				uint16_t *value)
{
	const struct phy_mii_dev_config *const cfg = DEV_CFG(dev);

	return mdio_read(cfg->mdio, cfg->phy_addr, reg_addr, value);
}

static inline int reg_write(const struct device *dev, uint16_t reg_addr,
				uint16_t value)
{
	const struct phy_mii_dev_config *const cfg = DEV_CFG(dev);

	return mdio_write(cfg->mdio, cfg->phy_addr, reg_addr, value);
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
	const struct phy_mii_dev_config *const cfg = DEV_CFG(dev);
	struct phy_mii_dev_data *const data = DEV_DATA(dev);
	bool link_up;

	uint16_t anar_reg = 0;
	uint16_t bmcr_reg = 0;
	uint16_t bmsr_reg = 0;
	uint16_t anlpar_reg = 0;
	uint32_t timeout = CONFIG_PHY_AUTONEG_TIMEOUT_MS / 100;

	if (reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	link_up = bmsr_reg & MII_BMSR_LINK_STATUS;

	/* If there is no change in link state don't proceed. */
	if (link_up == data->state.is_up) {
		return -EAGAIN;
	}

	/* TODO -- invoke link state change callback */

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

	/* Determine best link speed */
	if ((anar_reg & anlpar_reg) & MII_ADVERTISE_100_FULL) {
		data->state.speed = LINK_FULL_100BASE_T;
	} else if ((anar_reg & anlpar_reg) & MII_ADVERTISE_100_HALF) {
		data->state.speed = LINK_HALF_100BASE_T;
	} else if ((anar_reg & anlpar_reg) & MII_ADVERTISE_10_FULL) {
		data->state.speed = LINK_FULL_10BASE_T;
	} else {
		data->state.speed = LINK_HALF_10BASE_T;
	}

	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex",
		cfg->phy_addr,
		PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10",
		PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

	return 0;
}

static void invoke_link_cb(const struct device *dev)
{
	struct phy_mii_dev_data *const data = DEV_DATA(dev);
	struct phy_link_state state;

	if (data->cb == NULL) {
		return;
	}

	memcpy(&state, &data->state, sizeof(struct phy_link_state));
	data->cb(data->me, &state, data->cb_data);
}

static void monitor_work_handler(struct k_work *work)
{
	struct phy_mii_dev_data *const data =
		CONTAINER_OF(work, struct phy_mii_dev_data, monitor_work);
	const struct device *dev = data->me;
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
	uint16_t anar_reg;
	uint16_t bmcr_reg;

	if (reg_read(dev, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}

	if (reg_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	if (adv_speeds & LINK_FULL_10BASE_T)
		anar_reg |= MII_ADVERTISE_10_FULL;
	else
		anar_reg &= ~MII_ADVERTISE_10_FULL;

	if (adv_speeds & LINK_HALF_10BASE_T)
		anar_reg |= MII_ADVERTISE_10_HALF;
	else
		anar_reg &= ~MII_ADVERTISE_10_HALF;

	if (adv_speeds & LINK_FULL_100BASE_T)
		anar_reg |= MII_ADVERTISE_100_FULL;
	else
		anar_reg &= ~MII_ADVERTISE_100_FULL;

	if (adv_speeds & LINK_HALF_100BASE_T)
		anar_reg |= MII_ADVERTISE_100_HALF;
	else
		anar_reg &= ~MII_ADVERTISE_100_HALF;

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
	struct phy_mii_dev_data *const data = DEV_DATA(dev);

	k_sem_take(&data->sem, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	k_sem_give(&data->sem);

	return 0;
}

static int phy_mii_link_cb_set(const struct device *dev, phy_callback_t cb,
				void *user_data)
{
	struct phy_mii_dev_data *const data = DEV_DATA(dev);

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
	const struct phy_mii_dev_config *const cfg = DEV_CFG(dev);
	struct phy_mii_dev_data *const data = DEV_DATA(dev);
	uint32_t phy_id;

	k_sem_init(&data->sem, 1, 1);

	data->me = dev;
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

			LOG_INF("PHY (%d) ID %X", cfg->phy_addr, phy_id);
		}

		/* Advertise all speeds */
		phy_mii_cfg_link(dev, -1);

		k_work_init_delayable(&data->monitor_work,
					monitor_work_handler);
		k_work_reschedule(&data->monitor_work,
				  K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
	}

	return 0;
}

#define IS_FIXED_LINK(n)	DT_NODE_HAS_PROP(DT_DRV_INST(n), fixed_link)

static const struct ethphy_driver_api phy_mii_driver_api = {
	.get_link = phy_mii_get_link_state,
	.cfg_link = phy_mii_cfg_link,
	.link_cb_set = phy_mii_link_cb_set,
	.read = phy_mii_read,
	.write = phy_mii_write,
};

#define PHY_MII_CONFIG(n)						\
static const struct phy_mii_dev_config phy_mii_dev_config_##n = {	\
	.phy_addr = DT_PROP(DT_DRV_INST(n), address),			\
	.fixed = IS_FIXED_LINK(n),					\
	.fixed_speed = DT_ENUM_IDX_OR(DT_DRV_INST(n), fixed_link, 0),	\
	.mdio = UTIL_AND(UTIL_NOT(IS_FIXED_LINK(n)),			\
			DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), mdio)))\
};

#define PHY_MII_DEVICE(n)						\
	PHY_MII_CONFIG(n);						\
	static struct phy_mii_dev_data phy_mii_dev_data_##n;		\
	DEVICE_DT_INST_DEFINE(n,					\
			&phy_mii_initialize,				\
			NULL,						\
			&phy_mii_dev_data_##n,				\
			&phy_mii_dev_config_##n, POST_KERNEL,		\
			CONFIG_PHY_INIT_PRIORITY,			\
			&phy_mii_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PHY_MII_DEVICE)
