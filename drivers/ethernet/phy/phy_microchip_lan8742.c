/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 STMicroelectronics
 */

#define DT_DRV_COMPAT microchip_lan8742

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(microchip_lan8742, CONFIG_PHY_LOG_LEVEL);

#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/util_macro.h>

#include "phy_mii.h"

struct phy_lan8742_dev_config {
	uint8_t phy_addr;
	enum phy_link_speed default_speeds;
	const struct device *const mdio;
	struct gpio_dt_spec gpio_reset;
};

struct phy_lan8742_dev_data {
	const struct device *dev;
	phy_callback_t cb;
	void *cb_data;
	struct phy_link_state state;
	struct k_sem sem;
	struct k_work_delayable monitor_work;
	bool autoneg_in_progress;
	k_timepoint_t autoneg_timeout;
};

#define MII_INVALID_PHY_ID UINT32_MAX

/* How often to poll auto-negotiation status while waiting for it to complete */
#define MII_AUTONEG_POLL_INTERVAL_MS 100

static void phy_lan8742_invoke_link_cb(const struct device *dev);

static int phy_lan8742_reset(const struct device *dev)
{
	const struct phy_lan8742_dev_config *const cfg = dev->config;
	uint32_t timeout = 12U;
	uint16_t value;
	int ret;

	/* Issue an optional hard reset */
	if (cfg->gpio_reset.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("failed to initialize GPIO for reset");
			return ret;
		}
		gpio_pin_set_dt(&cfg->gpio_reset, 1);
		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(&cfg->gpio_reset, 0);
	}

	k_sleep(K_MSEC(25));

	/* Issue a soft reset */
	if (mdio_write(cfg->mdio, cfg->phy_addr, MII_BMCR, MII_BMCR_RESET) < 0) {
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

		if (mdio_read(cfg->mdio, cfg->phy_addr, MII_BMCR, &value) < 0) {
			return -EIO;
		}
	} while ((value & MII_BMCR_RESET) != 0U);

	return 0;
}

static int phy_lan8742_get_id(const struct device *dev, uint32_t *phy_id)
{
	const struct phy_lan8742_dev_config *const cfg = dev->config;
	uint16_t value;

	if (mdio_read(cfg->mdio, cfg->phy_addr, MII_PHYID1R, &value) < 0) {
		return -EIO;
	}

	*phy_id = value << 16;

	if (mdio_read(cfg->mdio, cfg->phy_addr, MII_PHYID2R, &value) < 0) {
		return -EIO;
	}

	*phy_id |= value;

	return 0;
}

static int phy_lan8742_update_link_state(const struct device *dev)
{
	const struct phy_lan8742_dev_config *const cfg = dev->config;
	struct phy_lan8742_dev_data *const data = dev->data;
	uint16_t bmcr_reg = 0;
	uint16_t bmsr_reg = 0;
	bool link_up;

	if (mdio_read(cfg->mdio, cfg->phy_addr, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	link_up = (bmsr_reg & MII_BMSR_LINK_STATUS) != 0U;

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

	if (mdio_read(cfg->mdio, cfg->phy_addr, MII_BMCR, &bmcr_reg) < 0) {
		return -EIO;
	}

	/* If auto-negotiation is not enabled, we only need to check the link speed */
	if ((bmcr_reg & MII_BMCR_AUTONEG_ENABLE) == 0U) {
		enum phy_link_speed new_speed = phy_mii_get_link_speed_bmcr_reg(dev, bmcr_reg);

		if (data->state.speed != new_speed || !data->state.is_up) {
			data->state.is_up = true;
			data->state.speed = new_speed;

			LOG_INF("PHY (%d) Link speed %s Mb, %s duplex",
				cfg->phy_addr,
				PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10",
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
	return -EINPROGRESS;
}

static int phy_lan8742_check_autoneg_completion(const struct device *dev)
{
	const struct phy_lan8742_dev_config *const cfg = dev->config;
	struct phy_lan8742_dev_data *const data = dev->data;
	uint16_t anlpar_reg = 0;
	uint16_t anar_reg = 0;
	uint16_t bmsr_reg = 0;

	/* On some PHY chips, the BMSR bits are latched, so the first read may
	 * show incorrect status. A second read ensures correct values.
	 */
	if (mdio_read(cfg->mdio, cfg->phy_addr, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	/* Second read, clears the latched bits and gives the correct status */
	if (mdio_read(cfg->mdio, cfg->phy_addr, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if ((bmsr_reg & MII_BMSR_AUTONEG_COMPLETE) == 0U) {
		if (sys_timepoint_expired(data->autoneg_timeout)) {
			LOG_DBG("PHY (%d) auto-negotiate timeout", cfg->phy_addr);
			return -ETIMEDOUT;
		}
		return -EINPROGRESS;
	}

	LOG_DBG("PHY (%d) auto-negotiate sequence completed", cfg->phy_addr);

	/* Read PHY default advertising parameters */
	if (mdio_read(cfg->mdio, cfg->phy_addr, MII_ANAR, &anar_reg) < 0) {
		return -EIO;
	}

	/** Read peer device capability */
	if (mdio_read(cfg->mdio, cfg->phy_addr, MII_ANLPAR, &anlpar_reg) < 0) {
		return -EIO;
	}

	if ((anar_reg & anlpar_reg & MII_ADVERTISE_100_FULL) != 0U) {
		data->state.speed = LINK_FULL_100BASE;
	} else if ((anar_reg & anlpar_reg & MII_ADVERTISE_100_HALF) != 0U) {
		data->state.speed = LINK_HALF_100BASE;
	} else if ((anar_reg & anlpar_reg & MII_ADVERTISE_10_FULL) != 0U) {
		data->state.speed = LINK_FULL_10BASE;
	} else {
		data->state.speed = LINK_HALF_10BASE;
	}

	data->state.is_up = (bmsr_reg & MII_BMSR_LINK_STATUS) != 0U;

	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", cfg->phy_addr,
		PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10",
		PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

	return 0;
}

static void phy_lan8742_monitor_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_lan8742_dev_data *const data =
		CONTAINER_OF(dwork, struct phy_lan8742_dev_data, monitor_work);
	const struct device *dev = data->dev;
	int rc;

	if (k_sem_take(&data->sem, K_NO_WAIT) == 0) {
		if (data->autoneg_in_progress) {
			rc = phy_lan8742_check_autoneg_completion(dev);
		} else {
			/* If autonegotiation is not in progress, just update the link state */
			rc = phy_lan8742_update_link_state(dev);
		}

		data->autoneg_in_progress = (rc == -EINPROGRESS);

		k_sem_give(&data->sem);

		/* If link state has changed and a callback is set, invoke callback */
		if (rc == 0) {
			phy_lan8742_invoke_link_cb(dev);
		}
	}

	k_work_reschedule(&data->monitor_work, data->autoneg_in_progress
						? K_MSEC(MII_AUTONEG_POLL_INTERVAL_MS)
						: K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_lan8742_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct phy_lan8742_dev_config *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg_addr, (uint16_t *)data);
}

static int phy_lan8742_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct phy_lan8742_dev_config *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg_addr, (uint16_t)data);
}

static int phy_lan8742_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds,
				enum phy_cfg_link_flag flags)
{
	const struct phy_lan8742_dev_config *const cfg = dev->config;
	struct phy_lan8742_dev_data *const data = dev->data;
	int ret = 0;

	if (!cfg->mdio) {
		return -ENODEV;
	}

	k_sem_take(&data->sem, K_FOREVER);

	if ((flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED) != 0U) {
		/* If auto-negotiation is disabled, only one speed can be selected.
		 * If gigabit is not supported, this speed must not be 1000M.
		 */

		ret = phy_mii_set_bmcr_reg_autoneg_disabled(dev, adv_speeds);
		if (ret >= 0) {
			data->autoneg_in_progress = false;
			k_work_reschedule(&data->monitor_work, K_NO_WAIT);
		}
	} else {
		ret = phy_mii_cfg_link_autoneg(dev, adv_speeds, false);
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

	k_sem_give(&data->sem);

	return ret;
}

static int phy_lan8742_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct phy_lan8742_dev_data *const data = dev->data;

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

static void phy_lan8742_invoke_link_cb(const struct device *dev)
{
	struct phy_lan8742_dev_data *const data = dev->data;
	struct phy_link_state state;

	if (!data->cb) {
		return;
	}

	phy_lan8742_get_link_state(dev, &state);

	data->cb(dev, &state, data->cb_data);
}

static int phy_lan8742_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct phy_lan8742_dev_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	/**
	 * Immediately invoke the callback to notify the caller of the
	 * current link status.
	 */
	phy_lan8742_invoke_link_cb(dev);

	return 0;
}

static DEVICE_API(ethphy, phy_lan8742_driver_api) = {
	.get_link = phy_lan8742_get_link_state,
	.link_cb_set = phy_lan8742_link_cb_set,
	.cfg_link = phy_lan8742_cfg_link,
	.read = phy_lan8742_read,
	.write = phy_lan8742_write,
};

static int phy_lan8742_init(const struct device *dev)
{
	const struct phy_lan8742_dev_config *const cfg = dev->config;
	struct phy_lan8742_dev_data *const data = dev->data;
	uint32_t phy_id;
	int ret = 0;

	data->state.is_up = false;

	if (!device_is_ready(cfg->mdio)) {
		return -ENODEV;
	}

	mdio_bus_enable(cfg->mdio);

	ret = phy_lan8742_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset PHY (%d): %d", cfg->phy_addr, ret);
		return ret;
	}

	if (phy_lan8742_get_id(dev, &phy_id) == 0) {
		if (phy_id == MII_INVALID_PHY_ID) {
			LOG_ERR("No PHY found at address %d", cfg->phy_addr);

			return -EINVAL;
		}

		LOG_INF("PHY (%d) ID 0x%X", cfg->phy_addr, phy_id);
	}

	k_work_init_delayable(&data->monitor_work, phy_lan8742_monitor_work);

	/* Advertise default speeds */
	ret = phy_lan8742_cfg_link(dev, cfg->default_speeds, 0);
	if (ret < 0) {
		LOG_ERR("Failed to configure link (%d)", ret);
		return ret;
	}

	/* Schedule the monitor work, if not already scheduled by phy_lan8742_cfg_link(). */
	k_work_schedule(&data->monitor_work, K_NO_WAIT);

	return 0;
}

#define PHY_LAN8742_CONFIG(n)                                                                     \
                                                                                                  \
	static const struct phy_lan8742_dev_config phy_lan8742_dev_config_##n = {                 \
		.phy_addr = DT_INST_REG_ADDR(n),                                                  \
		.default_speeds = PHY_INST_GENERATE_DEFAULT_SPEEDS(n),                            \
		.mdio = DEVICE_DT_GET(DT_INST_PARENT(n)),                                         \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0})			  \
	}

#define PHY_LAN8742_DATA(n)                                                                       \
	static struct phy_lan8742_dev_data phy_lan8742_dev_data_##n = {                           \
		.dev = DEVICE_DT_INST_GET(n),                                                     \
		.sem = Z_SEM_INITIALIZER(phy_lan8742_dev_data_##n.sem, 1, 1),                     \
	}

#define PHY_LAN8742_DEVICE(n)                                                                     \
	PHY_LAN8742_CONFIG(n);                                                                    \
	PHY_LAN8742_DATA(n);                                                                      \
	DEVICE_DT_INST_DEFINE(n, &phy_lan8742_init, NULL, &phy_lan8742_dev_data_##n,              \
			      &phy_lan8742_dev_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY, \
			      &phy_lan8742_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PHY_LAN8742_DEVICE)
