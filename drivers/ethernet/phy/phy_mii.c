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
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_mii, CONFIG_PHY_LOG_LEVEL);

#define ANY_DYNAMIC_LINK UTIL_NOT(DT_ALL_INST_HAS_PROP_STATUS_OKAY(fixed_link))
#define ANY_FIXED_LINK   DT_ANY_INST_HAS_PROP_STATUS_OKAY(fixed_link)

struct phy_mii_dev_config {
	uint8_t phy_addr;
	bool no_reset;
	bool fixed;
	int fixed_speed;
	enum phy_link_speed default_speeds;
	const struct device * const mdio;
};

struct phy_mii_dev_data {
	const struct device *dev;
	phy_callback_t cb;
	void *cb_data;
	struct phy_link_state state;
	struct k_sem sem;
#if ANY_DYNAMIC_LINK
	struct k_work_delayable monitor_work;
	bool gigabit_supported;
	bool restart_autoneg;
	bool autoneg_in_progress;
	k_timepoint_t autoneg_timeout;
#endif
};

/* Offset to align capabilities bits of 1000BASE-T Control and Status regs */
#define MII_1KSTSR_OFFSET 2

#define MII_INVALID_PHY_ID UINT32_MAX

/* How often to poll auto-negotiation status while waiting for it to complete */
#define MII_AUTONEG_POLL_INTERVAL_MS 100

static void invoke_link_cb(const struct device *dev);

#if ANY_DYNAMIC_LINK
static inline int phy_mii_reg_read(const struct device *dev, uint16_t reg_addr,
			   uint16_t *value)
{
	const struct phy_mii_dev_config *const cfg = dev->config;

	/* if there is no mdio (fixed-link) it is not supported to read */
	if (cfg->fixed) {
		return -ENOTSUP;
	}

	if (cfg->mdio == NULL) {
		return -ENODEV;
	}

	return mdio_read(cfg->mdio, cfg->phy_addr, reg_addr, value);
}

static inline int phy_mii_reg_write(const struct device *dev, uint16_t reg_addr,
			    uint16_t value)
{
	const struct phy_mii_dev_config *const cfg = dev->config;

	/* if there is no mdio (fixed-link) it is not supported to write */
	if (cfg->fixed) {
		return -ENOTSUP;
	}

	if (cfg->mdio == NULL) {
		return -ENODEV;
	}

	return mdio_write(cfg->mdio, cfg->phy_addr, reg_addr, value);
}

static bool is_gigabit_supported(const struct device *dev)
{
	uint16_t bmsr_reg;
	uint16_t estat_reg;

	if (phy_mii_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if (bmsr_reg & MII_BMSR_EXTEND_STATUS) {
		if (phy_mii_reg_read(dev, MII_ESTAT, &estat_reg) < 0) {
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
	if (phy_mii_reg_write(dev, MII_BMCR, MII_BMCR_RESET) < 0) {
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

		if (phy_mii_reg_read(dev, MII_BMCR, &value) < 0) {
			return -EIO;
		}
	} while (value & MII_BMCR_RESET);

	return 0;
}

static int get_id(const struct device *dev, uint32_t *phy_id)
{
	uint16_t value;

	if (phy_mii_reg_read(dev, MII_PHYID1R, &value) < 0) {
		return -EIO;
	}

	*phy_id = value << 16;

	if (phy_mii_reg_read(dev, MII_PHYID2R, &value) < 0) {
		return -EIO;
	}

	*phy_id |= value;

	return 0;
}

static int update_link_state(const struct device *dev)
{
	const struct phy_mii_dev_config *const cfg = dev->config;
	struct phy_mii_dev_data *const data = dev->data;
	bool link_up;

	uint16_t bmcr_reg = 0;
	uint16_t bmsr_reg = 0;

	if (phy_mii_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	link_up = bmsr_reg & MII_BMSR_LINK_STATUS;

	/* If there is no change in link state don't proceed. */
	if ((link_up == data->state.is_up) && !data->restart_autoneg) {
		return -EAGAIN;
	}

	data->state.is_up = link_up;

	if (!data->state.is_up) {
		data->state.speed = 0;
		LOG_INF("PHY (%d) is down", cfg->phy_addr);
		/* If not restarting auto-negotiation, just return */
		if (!data->restart_autoneg) {
			return 0;
		}
	}

	/**
	 * Perform auto-negotiation sequence.
	 */
	LOG_DBG("PHY (%d) Starting MII PHY auto-negotiate sequence",
		cfg->phy_addr);

	/* Configure and start auto-negotiation process */
	if (phy_mii_reg_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	bmcr_reg |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	bmcr_reg &= ~MII_BMCR_ISOLATE;  /* Don't isolate the PHY */

	if (phy_mii_reg_write(dev, MII_BMCR, bmcr_reg) < 0) {
		return -EIO;
	}

	data->restart_autoneg = false;

	/* We have to wait for the auto-negotiation process to complete */
	data->autoneg_timeout = sys_timepoint_calc(K_MSEC(CONFIG_PHY_AUTONEG_TIMEOUT_MS));
	return -EINPROGRESS;
}

static int check_autonegotiation_completion(const struct device *dev)
{
	const struct phy_mii_dev_config *const cfg = dev->config;
	struct phy_mii_dev_data *const data = dev->data;

	uint16_t anar_reg = 0;
	uint16_t bmsr_reg = 0;
	uint16_t anlpar_reg = 0;
	uint16_t c1kt_reg = 0;
	uint16_t s1kt_reg = 0;

	if (data->restart_autoneg) {
		return -EAGAIN;
	}

	/* On some PHY chips, the BMSR bits are latched, so the first read may
	 * show incorrect status. A second read ensures correct values.
	 */
	if (phy_mii_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	/* Second read, clears the latched bits and gives the correct status */
	if (phy_mii_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if (!(bmsr_reg & MII_BMSR_AUTONEG_COMPLETE)) {
		if (sys_timepoint_expired(data->autoneg_timeout)) {
			LOG_DBG("PHY (%d) auto-negotiate timedout", cfg->phy_addr);
			return -ETIMEDOUT;
		}
		return -EINPROGRESS;
	}

	LOG_DBG("PHY (%d) auto-negotiate sequence completed",
		cfg->phy_addr);

	/* Read PHY default advertising parameters */
	if (phy_mii_reg_read(dev, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}

	/** Read peer device capability */
	if (phy_mii_reg_read(dev, MII_ANLPAR, &anlpar_reg) < 0) {
		return -EIO;
	}

	if (data->gigabit_supported) {
		if (phy_mii_reg_read(dev, MII_1KTCR, &c1kt_reg) < 0) {
			return -EIO;
		}
		if (phy_mii_reg_read(dev, MII_1KSTSR, &s1kt_reg) < 0) {
			return -EIO;
		}
		s1kt_reg = (uint16_t)(s1kt_reg >> MII_1KSTSR_OFFSET);
	}

	if (data->gigabit_supported &&
			((c1kt_reg & s1kt_reg) & MII_ADVERTISE_1000_FULL)) {
		data->state.speed = LINK_FULL_1000BASE;
	} else if (data->gigabit_supported &&
			((c1kt_reg & s1kt_reg) & MII_ADVERTISE_1000_HALF)) {
		data->state.speed = LINK_HALF_1000BASE;
	} else if ((anar_reg & anlpar_reg) & MII_ADVERTISE_100_FULL) {
		data->state.speed = LINK_FULL_100BASE;
	} else if ((anar_reg & anlpar_reg) & MII_ADVERTISE_100_HALF) {
		data->state.speed = LINK_HALF_100BASE;
	} else if ((anar_reg & anlpar_reg) & MII_ADVERTISE_10_FULL) {
		data->state.speed = LINK_FULL_10BASE;
	} else {
		data->state.speed = LINK_HALF_10BASE;
	}

	data->state.is_up = (bmsr_reg & MII_BMSR_LINK_STATUS) != 0U;

	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex",
		cfg->phy_addr,
		PHY_LINK_IS_SPEED_1000M(data->state.speed) ? "1000" :
		(PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
		PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

	return 0;
}

static void monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_mii_dev_data *const data =
		CONTAINER_OF(dwork, struct phy_mii_dev_data, monitor_work);
	const struct device *dev = data->dev;
	int rc;

	if (k_sem_take(&data->sem, K_NO_WAIT) == 0) {
		if (data->autoneg_in_progress) {
			rc = check_autonegotiation_completion(dev);
		} else {
			/* If autonegotiation is not in progress, just update the link state */
			rc = update_link_state(dev);
		}

		data->autoneg_in_progress = (rc == -EINPROGRESS);

		k_sem_give(&data->sem);

		/* If link state has changed and a callback is set, invoke callback */
		if (rc == 0) {
			invoke_link_cb(dev);
		}
	}

	k_work_reschedule(&data->monitor_work, data->autoneg_in_progress
						       ? K_MSEC(MII_AUTONEG_POLL_INTERVAL_MS)
						       : K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_mii_read(const struct device *dev, uint16_t reg_addr,
			uint32_t *data)
{
	return phy_mii_reg_read(dev, reg_addr, (uint16_t *)data);
}

static int phy_mii_write(const struct device *dev, uint16_t reg_addr,
			 uint32_t data)
{
	return phy_mii_reg_write(dev, reg_addr, (uint16_t)data);
}

static int phy_mii_cfg_link(const struct device *dev,
			    enum phy_link_speed adv_speeds)
{
	struct phy_mii_dev_data *const data = dev->data;
	const struct phy_mii_dev_config *const cfg = dev->config;
	uint16_t anar_reg;
	uint16_t anar_reg_old;
	uint16_t bmcr_reg;
	uint16_t c1kt_reg;
	uint16_t c1kt_reg_old;
	int ret = 0;

	/* if there is no mdio (fixed-link) it is not supported to configure link */
	if (cfg->fixed) {
		return -ENOTSUP;
	}

	if (cfg->mdio == NULL) {
		return -ENODEV;
	}

	k_sem_take(&data->sem, K_FOREVER);

	if (phy_mii_reg_read(dev, MII_ANAR, &anar_reg) < 0) {
		ret = -EIO;
		goto cfg_link_end;
	}
	anar_reg_old = anar_reg;

	if (phy_mii_reg_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		ret = -EIO;
		goto cfg_link_end;
	}

	if (data->gigabit_supported) {
		if (phy_mii_reg_read(dev, MII_1KTCR, &c1kt_reg) < 0) {
			ret = -EIO;
			goto cfg_link_end;
		}
	}

	if (adv_speeds & LINK_FULL_10BASE) {
		anar_reg |= MII_ADVERTISE_10_FULL;
	} else {
		anar_reg &= ~MII_ADVERTISE_10_FULL;
	}

	if (adv_speeds & LINK_HALF_10BASE) {
		anar_reg |= MII_ADVERTISE_10_HALF;
	} else {
		anar_reg &= ~MII_ADVERTISE_10_HALF;
	}

	if (adv_speeds & LINK_FULL_100BASE) {
		anar_reg |= MII_ADVERTISE_100_FULL;
	} else {
		anar_reg &= ~MII_ADVERTISE_100_FULL;
	}

	if (adv_speeds & LINK_HALF_100BASE) {
		anar_reg |= MII_ADVERTISE_100_HALF;
	} else {
		anar_reg &= ~MII_ADVERTISE_100_HALF;
	}

	if (data->gigabit_supported) {
		c1kt_reg_old = c1kt_reg;

		if (adv_speeds & LINK_FULL_1000BASE) {
			c1kt_reg |= MII_ADVERTISE_1000_FULL;
		} else {
			c1kt_reg &= ~MII_ADVERTISE_1000_FULL;
		}

		if (adv_speeds & LINK_HALF_1000BASE) {
			c1kt_reg |= MII_ADVERTISE_1000_HALF;
		} else {
			c1kt_reg &= ~MII_ADVERTISE_1000_HALF;
		}

		if (c1kt_reg != c1kt_reg_old) {
			if (phy_mii_reg_write(dev, MII_1KTCR, c1kt_reg) < 0) {
				ret = -EIO;
				goto cfg_link_end;
			}

			data->restart_autoneg = true;
		}
	}

	if (anar_reg != anar_reg_old) {
		if (phy_mii_reg_write(dev, MII_ANAR, anar_reg) < 0) {
			ret = -EIO;
			goto cfg_link_end;
		}

		data->restart_autoneg = true;
	}

	if ((bmcr_reg & MII_BMCR_AUTONEG_ENABLE) == 0U) {
		if (phy_mii_reg_write(dev, MII_BMCR, bmcr_reg | MII_BMCR_AUTONEG_ENABLE) < 0) {
			ret = -EIO;
			goto cfg_link_end;
		}
	}

	if (data->restart_autoneg && data->state.is_up) {
		k_work_reschedule(&data->monitor_work, K_NO_WAIT);
	}

cfg_link_end:
	k_sem_give(&data->sem);

	return ret;
}
#endif /* ANY_DYNAMIC_LINK */

static int phy_mii_get_link_state(const struct device *dev,
				  struct phy_link_state *state)
{
	struct phy_mii_dev_data *const data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	if (data->state.speed == 0) {
		/* If speed is 0, then link is also down, happens when autonegotiation is in
		 * progress
		 */
		data->state.is_up = false;
	}

	k_sem_give(&data->sem);

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

	data->cb(dev, &state, data->cb_data);
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

#if ANY_FIXED_LINK
static int phy_mii_initialize_fixed_link(const struct device *dev)
{
	const struct phy_mii_dev_config *const cfg = dev->config;
	struct phy_mii_dev_data *const data = dev->data;

	/**
	 * If this is a *fixed* link then we don't need to communicate
	 * with a PHY. We set the link parameters as configured
	 * and set link state to up.
	 */

	const static int speed_to_phy_link_speed[] = {
		LINK_HALF_10BASE,
		LINK_FULL_10BASE,
		LINK_HALF_100BASE,
		LINK_FULL_100BASE,
		LINK_HALF_1000BASE,
		LINK_FULL_1000BASE,
	};

	data->state.speed = speed_to_phy_link_speed[cfg->fixed_speed];
	data->state.is_up = true;

	return 0;
}
#endif /* ANY_FIXED_LINK */

#if ANY_DYNAMIC_LINK
static int phy_mii_initialize_dynamic_link(const struct device *dev)
{
	const struct phy_mii_dev_config *const cfg = dev->config;
	struct phy_mii_dev_data *const data = dev->data;
	uint32_t phy_id;

	data->state.is_up = false;

	mdio_bus_enable(cfg->mdio);

	if (cfg->no_reset == false) {
		reset(dev);
	}

	if (get_id(dev, &phy_id) == 0) {
		if (phy_id == MII_INVALID_PHY_ID) {
			LOG_ERR("No PHY found at address %d", cfg->phy_addr);

			return -EINVAL;
		}

		LOG_INF("PHY (%d) ID %X", cfg->phy_addr, phy_id);
	}

	data->gigabit_supported = is_gigabit_supported(dev);

	k_work_init_delayable(&data->monitor_work, monitor_work_handler);

	/* Advertise default speeds */
	phy_mii_cfg_link(dev, cfg->default_speeds);

	monitor_work_handler(&data->monitor_work.work);

	return 0;
}
#endif /* ANY_DYNAMIC_LINK */

#define IS_FIXED_LINK(n)	DT_INST_NODE_HAS_PROP(n, fixed_link)

static DEVICE_API(ethphy, phy_mii_driver_api) = {
	.get_link = phy_mii_get_link_state,
	.link_cb_set = phy_mii_link_cb_set,
#if ANY_DYNAMIC_LINK
	.cfg_link = phy_mii_cfg_link,
	.read = phy_mii_read,
	.write = phy_mii_write,
#endif
};

#define PHY_MII_GENERATE_DEFAULT_SPEEDS(n)							\
((DT_INST_ENUM_HAS_VALUE(n, default_speeds, 10base_half_duplex) ? LINK_HALF_10BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 10base_full_duplex) ? LINK_FULL_10BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 100base_half_duplex) ? LINK_HALF_100BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 100base_full_duplex) ? LINK_FULL_100BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 1000base_half_duplex) ? LINK_HALF_1000BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 1000base_full_duplex) ? LINK_FULL_1000BASE : 0))

#define PHY_MII_CONFIG(n)						 \
BUILD_ASSERT(PHY_MII_GENERATE_DEFAULT_SPEEDS(n) != 0,			 \
	"At least one valid speed must be configured for this driver");	 \
									 \
static const struct phy_mii_dev_config phy_mii_dev_config_##n = {	 \
	.phy_addr = DT_INST_REG_ADDR(n),				 \
	.no_reset = DT_INST_PROP(n, no_reset),				 \
	.fixed = IS_FIXED_LINK(n),					 \
	.fixed_speed = DT_INST_ENUM_IDX_OR(n, fixed_link, 0),		 \
	.default_speeds = PHY_MII_GENERATE_DEFAULT_SPEEDS(n),		 \
	.mdio = UTIL_AND(UTIL_NOT(IS_FIXED_LINK(n)),			 \
			 DEVICE_DT_GET(DT_INST_BUS(n)))			 \
};

#define PHY_MII_DATA(n)							 \
static struct phy_mii_dev_data phy_mii_dev_data_##n = {			 \
	.dev = DEVICE_DT_INST_GET(n),					 \
	.cb = NULL,							 \
	.sem = Z_SEM_INITIALIZER(phy_mii_dev_data_##n.sem, 1, 1),	 \
};

#define PHY_MII_INIT(n)							 \
	COND_CODE_1(IS_FIXED_LINK(n), (&phy_mii_initialize_fixed_link),	 \
		   (&phy_mii_initialize_dynamic_link))

#define PHY_MII_DEVICE(n)						\
	PHY_MII_CONFIG(n);						\
	PHY_MII_DATA(n);						\
	DEVICE_DT_INST_DEFINE(n,					\
			      PHY_MII_INIT(n),				\
			      NULL,					\
			      &phy_mii_dev_data_##n,			\
			      &phy_mii_dev_config_##n, POST_KERNEL,	\
			      CONFIG_PHY_INIT_PRIORITY,			\
			      &phy_mii_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PHY_MII_DEVICE)
