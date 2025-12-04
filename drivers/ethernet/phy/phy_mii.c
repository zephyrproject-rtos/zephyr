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
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_mii, CONFIG_PHY_LOG_LEVEL);

#include "phy_mii.h"

#define ANY_DYNAMIC_LINK UTIL_NOT(DT_ALL_INST_HAS_PROP_STATUS_OKAY(fixed_link))
#define ANY_FIXED_LINK   DT_ANY_INST_HAS_PROP_STATUS_OKAY(fixed_link)
#define ANY_RESET_GPIO   DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)

struct phy_mii_dev_config {
	uint8_t phy_addr;
	bool no_reset;
	bool fixed;
	int fixed_speed;
	enum phy_link_speed default_speeds;
	const struct device * const mdio;
#if ANY_RESET_GPIO
	const struct gpio_dt_spec reset_gpio;
	uint32_t reset_assert_duration_us;
	uint32_t reset_deassertion_timeout_ms;
#endif /* ANY_RESET_GPIO */
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
static int check_autonegotiation_completion(const struct device *dev);

static inline int phy_mii_reg_read(const struct device *dev, uint16_t reg_addr,
			   uint16_t *value)
{
	const struct phy_mii_dev_config *const cfg = dev->config;

	/* if there is no mdio (fixed-link) it is not supported to read */
	if (ANY_FIXED_LINK && cfg->fixed) {
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
	if (ANY_FIXED_LINK && cfg->fixed) {
		return -ENOTSUP;
	}

	if (cfg->mdio == NULL) {
		return -ENODEV;
	}

	return mdio_write(cfg->mdio, cfg->phy_addr, reg_addr, value);
}

static int read_gigabit_supported_flag(const struct device *dev, bool *supported)
{
	uint16_t bmsr_reg;
	uint16_t estat_reg;

	if (phy_mii_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if ((bmsr_reg & MII_BMSR_EXTEND_STATUS) != 0U) {
		if (phy_mii_reg_read(dev, MII_ESTAT, &estat_reg) < 0) {
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

static int reset(const struct device *dev)
{
	uint32_t timeout = 12U;
	uint16_t value;

#if ANY_RESET_GPIO
	const struct phy_mii_dev_config *const cfg = dev->config;
	int ret;

	if (gpio_is_ready_dt(&cfg->reset_gpio)) {
		/* Issue a hard reset */
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure RST pin (%d)", ret);
			return ret;
		}

		/* assertion time */
		k_busy_wait(cfg->reset_assert_duration_us);

		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Failed to de-assert RST pin (%d)", ret);
			return ret;
		}

		k_msleep(cfg->reset_deassertion_timeout_ms);

		return 0;
	}
#endif /* ANY_RESET_GPIO */

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
	} while ((value & MII_BMCR_RESET) != 0U);

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

	if (phy_mii_reg_read(dev, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	/* If auto-negotiation is not enabled, we only need to check the link speed */
	if (!IS_BIT_SET(bmcr_reg, MII_BMCR_AUTONEG_ENABLE_BIT)) {
		enum phy_link_speed new_speed = phy_mii_get_link_speed_bmcr_reg(dev, bmcr_reg);

		if ((data->state.speed != new_speed) || !data->state.is_up) {
			data->state.is_up = true;
			data->state.speed = new_speed;

			LOG_INF("PHY (%d) Link speed %s Mb, %s duplex",
				cfg->phy_addr,
				PHY_LINK_IS_SPEED_1000M(data->state.speed) ? "1000" :
				(PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
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

	return check_autonegotiation_completion(dev);
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

	if (phy_mii_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
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
		if (phy_mii_reg_read(dev, MII_BMSR, &bmsr_reg) < 0) {
			return -EIO;
		}

		if (!IS_BIT_SET(bmsr_reg, MII_BMSR_LINK_STATUS_BIT)) {
			return -EAGAIN;
		}
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
			((c1kt_reg & s1kt_reg & MII_ADVERTISE_1000_FULL) != 0U)) {
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

static int phy_mii_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds,
			    enum phy_cfg_link_flag flags)
{
	struct phy_mii_dev_data *const data = dev->data;
	const struct phy_mii_dev_config *const cfg = dev->config;
	int ret = 0;

	/* if there is no mdio (fixed-link) it is not supported to configure link */
	if (ANY_FIXED_LINK && cfg->fixed) {
		return -ENOTSUP;
	}

	if (cfg->mdio == NULL) {
		return -ENODEV;
	}

	k_sem_take(&data->sem, K_FOREVER);

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
					  K_MSEC(MII_AUTONEG_POLL_INTERVAL_MS));
		}
	}

	if (ret == -EALREADY) {
		LOG_DBG("PHY (%d) Link already configured", cfg->phy_addr);
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

	if (state->speed == 0) {
		/* If speed is 0, then link is also down, happens when autonegotiation is in
		 * progress
		 */
		state->is_up = false;
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
	int ret = 0;

	data->state.is_up = false;

	if (cfg->no_reset == false) {
		ret = reset(dev);
		if (ret < 0) {
			LOG_ERR("Failed to reset PHY (%d): %d", cfg->phy_addr, ret);
			return ret;
		}
	}

	if (get_id(dev, &phy_id) == 0) {
		if (phy_id == MII_INVALID_PHY_ID) {
			LOG_ERR("No PHY found at address %d", cfg->phy_addr);

			return -EINVAL;
		}

		LOG_INF("PHY (%d) ID %X", cfg->phy_addr, phy_id);
	}

	ret = read_gigabit_supported_flag(dev, &data->gigabit_supported);
	if (ret < 0) {
		LOG_ERR("Failed to read PHY capabilities: %d", ret);
		return ret;
	}

	k_work_init_delayable(&data->monitor_work, monitor_work_handler);

	/* Advertise default speeds */
	ret = phy_mii_cfg_link(dev, cfg->default_speeds, 0);
	if (ret == -EALREADY) {
		data->autoneg_in_progress = true;
		data->autoneg_timeout = sys_timepoint_calc(K_MSEC(CONFIG_PHY_AUTONEG_TIMEOUT_MS));
	}

	/* This will schedule the monitor work, if not already scheduled by phy_mii_cfg_link(). */
	k_work_schedule(&data->monitor_work, K_NO_WAIT);

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

#if ANY_RESET_GPIO
#define RESET_GPIO(n)							 \
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n,			 \
			 reset_gpios, {0}),				 \
	.reset_assert_duration_us = DT_INST_PROP_OR(n,			 \
			 reset_assert_duration_us, 0),			 \
	.reset_deassertion_timeout_ms = DT_INST_PROP_OR(n,		 \
			 reset_deassertion_timeout_ms, 0),
#else
#define RESET_GPIO(n)
#endif /* ANY_RESET_GPIO */

#define PHY_MII_CONFIG(n)						 \
BUILD_ASSERT(PHY_INST_GENERATE_DEFAULT_SPEEDS(n) != 0,			 \
	"At least one valid speed must be configured for this driver");	 \
									 \
static const struct phy_mii_dev_config phy_mii_dev_config_##n = {	 \
	.phy_addr = DT_INST_REG_ADDR(n),				 \
	.no_reset = DT_INST_PROP(n, no_reset),				 \
	.fixed = IS_FIXED_LINK(n),					 \
	.fixed_speed = DT_INST_ENUM_IDX_OR(n, fixed_link, 0),		 \
	.default_speeds = PHY_INST_GENERATE_DEFAULT_SPEEDS(n),		 \
	.mdio = UTIL_AND(UTIL_NOT(IS_FIXED_LINK(n)),			 \
			 DEVICE_DT_GET(DT_INST_BUS(n))),		 \
	RESET_GPIO(n)							 \
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
