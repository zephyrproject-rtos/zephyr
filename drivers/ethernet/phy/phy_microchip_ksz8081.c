/*
 * Copyright 2023-2024 NXP
 *
 * Inspiration from phy_mii.c, which is:
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_ksz8081

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>
#include <string.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/gpio.h>

#define LOG_MODULE_NAME phy_mc_ksz8081
#define LOG_LEVEL CONFIG_PHY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "phy_mii.h"

#define PHY_MC_KSZ8081_OMSO_REG			0x16
#define PHY_MC_KSZ8081_OMSO_FACTORY_MODE_MASK	BIT(15)
#define PHY_MC_KSZ8081_OMSO_NAND_TREE_MASK	BIT(5)
#define PHY_MC_KSZ8081_OMSO_RMII_OVERRIDE_MASK	BIT(1)
#define PHY_MC_KSZ8081_OMSO_MII_OVERRIDE_MASK	BIT(0)

#define PHY_MC_KSZ8081_ICS_REG			0x1B
#define PHY_MC_KSZ8081_ICS_LINK_DOWN_IE_MASK	BIT(10)
#define PHY_MC_KSZ8081_ICS_LINK_UP_IE_MASK	BIT(8)

#define PHY_MC_KSZ8081_CTRL2_REG		0x1F
#define PHY_MC_KSZ8081_CTRL2_REF_CLK_SEL	BIT(7)

enum ksz8081_interface {
	KSZ8081_MII,
	KSZ8081_RMII,
	KSZ8081_RMII_25MHZ,
};

struct mc_ksz8081_config {
	uint8_t addr;
	const struct device *mdio_dev;
	enum ksz8081_interface phy_iface;
	enum phy_link_speed default_speeds;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	const struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct gpio_dt_spec interrupt_gpio;
#endif
};

/* arbitrarily defined internal driver flags */
#define KSZ8081_DO_AUTONEG_FLAG BIT(0)
#define KSZ8081_SILENCE_DEBUG_LOGS BIT(1)
#define KSZ8081_LINK_STATE_VALID BIT(2)

#define USING_INTERRUPT_GPIO							\
		UTIL_OR(DT_ALL_INST_HAS_PROP_STATUS_OKAY(int_gpios),		\
			UTIL_AND(DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios),	\
				 (config->interrupt_gpio.port != NULL)))

struct mc_ksz8081_data {
	const struct device *dev;
	struct phy_link_state state;
	phy_callback_t cb;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_callback gpio_callback;
#endif
	void *cb_data;
	struct k_mutex mutex;
	struct k_work_delayable phy_monitor_work;
	uint8_t flags;
};

static int phy_mc_ksz8081_read(const struct device *dev,
				uint16_t reg_addr, uint32_t *data)
{
	const struct mc_ksz8081_config *config = dev->config;
	struct mc_ksz8081_data *dev_data = dev->data;
	int ret;

	/* Make sure excessive bits 16-31 are reset */
	*data = 0U;

	ret = mdio_read(config->mdio_dev, config->addr, reg_addr, (uint16_t *)data);
	if (ret) {
		LOG_WRN("Failed to read from %s reg 0x%x", dev->name, reg_addr);
		return ret;
	}

	if (!(dev_data->flags & KSZ8081_SILENCE_DEBUG_LOGS)) {
		LOG_DBG("Read 0x%x from phy %d reg 0x%x", *data, config->addr, reg_addr);
	}

	return 0;
}

static int phy_mc_ksz8081_write(const struct device *dev,
				uint16_t reg_addr, uint32_t data)
{
	const struct mc_ksz8081_config *config = dev->config;
	struct mc_ksz8081_data *dev_data = dev->data;
	int ret;

	ret = mdio_write(config->mdio_dev, config->addr, reg_addr, (uint16_t)data);
	if (ret) {
		LOG_WRN("Failed to write to %s reg 0x%x", dev->name, reg_addr);
		return ret;
	}

	if (!(dev_data->flags & KSZ8081_SILENCE_DEBUG_LOGS)) {
		LOG_DBG("Wrote 0x%x to phy %d reg 0x%x", data, config->addr, reg_addr);
	}

	return 0;
}

#if !DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
#define phy_mc_ksz8081_clear_interrupt(data) 0
#else
static int phy_mc_ksz8081_clear_interrupt(struct mc_ksz8081_data *data)
{
	const struct device *dev = data->dev;
	const struct mc_ksz8081_config *config = dev->config;
	uint32_t ics;
	int ret;

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	/* Read/clear PHY interrupt status register */
	ret = phy_mc_ksz8081_read(dev, PHY_MC_KSZ8081_ICS_REG, &ics);
	if (ret < 0) {
		LOG_ERR("Error reading phy (%d) interrupt status register", config->addr);
	}

	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);
	return ret;
}

static int phy_mc_ksz8081_config_interrupt(const struct device *dev)
{
	struct mc_ksz8081_data *data = dev->data;
	uint32_t ics;
	int ret;

	/* Read Interrupt Control/Status register to write back */
	ret = phy_mc_ksz8081_read(dev, PHY_MC_KSZ8081_ICS_REG, &ics);
	if (ret < 0) {
		return ret;
	}
	ics |= PHY_MC_KSZ8081_ICS_LINK_UP_IE_MASK | PHY_MC_KSZ8081_ICS_LINK_DOWN_IE_MASK;

	/* Write settings to Interrupt Control/Status register */
	ret = phy_mc_ksz8081_write(dev, PHY_MC_KSZ8081_ICS_REG, ics);
	if (ret < 0) {
		return ret;
	}

	/* Clear interrupt */
	ret = phy_mc_ksz8081_clear_interrupt(data);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static void phy_mc_ksz8081_interrupt_handler(const struct device *port, struct gpio_callback *cb,
					     gpio_port_pins_t pins)
{
	struct mc_ksz8081_data *data = CONTAINER_OF(cb, struct mc_ksz8081_data, gpio_callback);
	int ret = k_work_reschedule(&data->phy_monitor_work, K_NO_WAIT);

	if (ret < 0) {
		LOG_ERR("Failed to schedule monitor_work from ISR");
	}
}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

static int phy_mc_ksz8081_autonegotiate(const struct device *dev)
{
	const struct mc_ksz8081_config *config = dev->config;
	struct mc_ksz8081_data *data = dev->data;
	int ret = 0, attempts = 0;
	uint32_t bmcr = 0;
	uint32_t bmsr = 0, last_bmsr = 0;
	uint16_t timeout = CONFIG_PHY_AUTONEG_TIMEOUT_MS / 100;

	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		goto done;
	}

	/* Read control register to write back with autonegotiation bit */
	ret = phy_mc_ksz8081_read(dev, MII_BMCR, &bmcr);
	if (ret) {
		goto done;
	}

	/* (re)start autonegotiation */
	LOG_INF("PHY (%d) is entering autonegotiation sequence", config->addr);
	bmcr |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	bmcr &= ~MII_BMCR_ISOLATE;

	ret = phy_mc_ksz8081_write(dev, MII_BMCR, bmcr);
	if (ret) {
		goto done;
	}

	data->flags |= KSZ8081_SILENCE_DEBUG_LOGS;
	do {
		if (timeout-- == 0) {
			LOG_ERR("PHY (%d) autonegotiation timed out", config->addr);
			/* The value -ETIMEDOUT can be returned by PHY read/write functions, so
			 * return -ENETDOWN instead to distinguish link timeout from PHY timeout.
			 */
			ret = -ENETDOWN;
			goto done;
		}
		k_msleep(100);

		ret = phy_mc_ksz8081_read(dev, MII_BMSR, &bmsr);
		if (ret) {
			goto done;
		}

		if (last_bmsr != bmsr) {
			LOG_DBG("phy %d autoneg BMSR: %x", config->addr, bmsr);
		}

		last_bmsr = bmsr;
		attempts++;
	} while (!(bmsr & MII_BMSR_AUTONEG_COMPLETE));
	data->flags &= ~KSZ8081_SILENCE_DEBUG_LOGS;

	LOG_DBG("PHY (%d) autonegotiation completed after %d checks", config->addr, attempts);

	data->flags &= ~KSZ8081_DO_AUTONEG_FLAG;

done:
	if (ret && ret != -ENETDOWN) {
		LOG_ERR("Failed to configure %s for autonegotiation", dev->name);
	}
	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);
	return ret;
}


static int phy_mc_ksz8081_get_link(const struct device *dev,
					struct phy_link_state *state)
{
	struct mc_ksz8081_data *data = dev->data;
	struct phy_link_state *link_state = &data->state;

	if ((data->flags & KSZ8081_LINK_STATE_VALID) != KSZ8081_LINK_STATE_VALID) {
		return -EIO;
	}

	state->speed = link_state->speed;
	state->is_up = link_state->is_up;

	return 0;
}

static int phy_mc_ksz8081_update_link(const struct device *dev)
{
	const struct mc_ksz8081_config *config = dev->config;
	struct mc_ksz8081_data *data = dev->data;
	struct phy_link_state *state = &data->state;
	int ret;
	uint32_t bmsr = 0;
	uint32_t anar = 0;
	uint32_t anlpar = 0;
	struct phy_link_state old_state = data->state;

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY %d mutex lock error", config->addr);
		return ret;
	}

	/* Read link state */
	ret = phy_mc_ksz8081_read(dev, MII_BMSR, &bmsr);
	if (ret) {
		goto done;
	}
	state->is_up = bmsr & MII_BMSR_LINK_STATUS;

	if (!state->is_up) {
		goto result;
	}

	/* Read currently configured advertising options */
	ret = phy_mc_ksz8081_read(dev, MII_ANAR, &anar);
	if (ret) {
		goto done;
	}

	/* Read link partner capability */
	ret = phy_mc_ksz8081_read(dev, MII_ANLPAR, &anlpar);
	if (ret) {
		goto done;
	}

	uint32_t mutual_capabilities = anar & anlpar;

	if (mutual_capabilities & MII_ADVERTISE_100_FULL) {
		state->speed = LINK_FULL_100BASE;
	} else if (mutual_capabilities & MII_ADVERTISE_100_HALF) {
		state->speed = LINK_HALF_100BASE;
	} else if (mutual_capabilities & MII_ADVERTISE_10_FULL) {
		state->speed = LINK_FULL_10BASE;
	} else if (mutual_capabilities & MII_ADVERTISE_10_HALF) {
		state->speed = LINK_HALF_10BASE;
	} else {
		ret = -EIO;
	}

result:
	if (memcmp(&old_state, state, sizeof(struct phy_link_state)) != 0) {
		LOG_DBG("PHY %d is %s", config->addr, state->is_up ? "up" : "down");
		if (state->is_up) {
			LOG_DBG("PHY (%d) Link speed %s Mb, %s duplex\n", config->addr,
				(PHY_LINK_IS_SPEED_100M(state->speed) ? "100" : "10"),
				PHY_LINK_IS_FULL_DUPLEX(state->speed) ? "full" : "half");
		}
	}

done:
	if (ret) {
		LOG_ERR("Failed to get %s state", dev->name);
	}
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int phy_mc_ksz8081_static_cfg(const struct device *dev)
{
	const struct mc_ksz8081_config *config = dev->config;
	uint32_t omso = 0;
	uint32_t ctrl2 = 0;
	int ret = 0;

	/* Force normal operation in the case of factory mode */
	ret = phy_mc_ksz8081_read(dev, PHY_MC_KSZ8081_OMSO_REG, (uint32_t *)&omso);
	if (ret) {
		return ret;
	}

	omso &= ~PHY_MC_KSZ8081_OMSO_FACTORY_MODE_MASK &
		~PHY_MC_KSZ8081_OMSO_NAND_TREE_MASK;
	if (config->phy_iface == KSZ8081_RMII || config->phy_iface == KSZ8081_RMII_25MHZ) {
		omso &= ~PHY_MC_KSZ8081_OMSO_MII_OVERRIDE_MASK;
		omso |= PHY_MC_KSZ8081_OMSO_RMII_OVERRIDE_MASK;
	}

	ret = phy_mc_ksz8081_write(dev, PHY_MC_KSZ8081_OMSO_REG, (uint32_t)omso);
	if (ret) {
		return ret;
	}

	/* Select correct reference clock mode depending on interface setup */
	ret = phy_mc_ksz8081_read(dev, PHY_MC_KSZ8081_CTRL2_REG, (uint32_t *)&ctrl2);
	if (ret) {
		return ret;
	}

	if (config->phy_iface == KSZ8081_RMII) {
		ctrl2 |= PHY_MC_KSZ8081_CTRL2_REF_CLK_SEL;
	} else {
		ctrl2 &= ~PHY_MC_KSZ8081_CTRL2_REF_CLK_SEL;
	}

	ret = phy_mc_ksz8081_write(dev, PHY_MC_KSZ8081_CTRL2_REG, (uint32_t)ctrl2);
	if (ret) {
		return ret;
	}

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
static int phy_mc_ksz8081_reset_gpio(const struct mc_ksz8081_config *config)
{
	int ret;

	if (!config->reset_gpio.port) {
		return -ENODEV;
	}

	/* Start reset */
	ret = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (ret) {
		return ret;
	}

	/* Wait for at least 500 us as specified by datasheet */
	k_busy_wait(1000);

	/* Reset over */
	ret = gpio_pin_set_dt(&config->reset_gpio, 1);

	/* After deasserting reset, must wait at least 100 us to use programming interface */
	k_busy_wait(200);

	return ret;
}
#else
static int phy_mc_ksz8081_reset_gpio(const struct mc_ksz8081_config *config)
{
	ARG_UNUSED(config);

	return -ENODEV;
}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) */

static int phy_mc_ksz8081_reset(const struct device *dev)
{
	const struct mc_ksz8081_config *config = dev->config;
	struct mc_ksz8081_data *data = dev->data;
	int ret;

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	ret = phy_mc_ksz8081_reset_gpio(config);
	if (ret != -ENODEV) { /* On -ENODEV, attempt command-based reset */
		goto done;
	}

	ret = phy_mc_ksz8081_write(dev, MII_BMCR, MII_BMCR_RESET);
	if (ret) {
		goto done;
	}

	/* According to IEEE 802.3, Section 2, Subsection 22.2.4.1.1,
	 * a PHY reset may take up to 0.5 s.
	 */
	k_busy_wait(500 * USEC_PER_MSEC);

	/* After each reset we will apply the static cfg from DT */
	ret = phy_mc_ksz8081_static_cfg(dev);
	if (ret) {
		goto done;
	}
done:
	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);
	return ret;
}

static int phy_mc_ksz8081_cfg_link(const struct device *dev, enum phy_link_speed speeds,
				   enum phy_cfg_link_flag flags)
{
	__maybe_unused const struct mc_ksz8081_config *config = dev->config;
	struct mc_ksz8081_data *data = dev->data;
	int ret;

	if (flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED) {
		LOG_ERR("Disabling auto-negotiation is not supported by this driver");
		return -ENOTSUP;
	}

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	/* DT configurations */
	ret = phy_mc_ksz8081_static_cfg(dev);
	if (ret) {
		goto done;
	}

	ret = phy_mii_set_anar_reg(dev, speeds);
	if (ret == -EALREADY) {
		ret = 0;
	}
	if (ret < 0) {
		goto done;
	}

	data->flags |= KSZ8081_DO_AUTONEG_FLAG;
done:
	if (ret) {
		LOG_ERR("Failed to configure %s", dev->name);
	}
	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);

	if (USING_INTERRUPT_GPIO) {
		return ret;
	}

	/* Start monitoring */
	k_work_reschedule(&data->phy_monitor_work,
				K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	return ret;
}

static int phy_mc_ksz8081_link_cb_set(const struct device *dev,
					phy_callback_t cb, void *user_data)
{
	struct mc_ksz8081_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	phy_mc_ksz8081_get_link(dev, &data->state);

	data->cb(dev, &data->state, data->cb_data);

	return 0;
}

static void phy_mc_ksz8081_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mc_ksz8081_data *data =
		CONTAINER_OF(dwork, struct mc_ksz8081_data, phy_monitor_work);
	const struct device *dev = data->dev;
	const struct mc_ksz8081_config *config = dev->config;
	struct phy_link_state state = data->state;
	bool turn_on_logs = false;
	int rc = 0;

	if (USING_INTERRUPT_GPIO) {
		rc = phy_mc_ksz8081_clear_interrupt(data);
		if (rc < 0) {
			return;
		}
	}

	if (!data->state.is_up) {
		/* some overrides might need set on cold reset way late for some reason */
		phy_mc_ksz8081_static_cfg(dev);
	}

	/* (re)do autonegotiation if needed */
	if (data->flags & KSZ8081_DO_AUTONEG_FLAG) {
		rc = phy_mc_ksz8081_autonegotiate(dev);
	}
	if (rc && (rc != -ENETDOWN)) {
		LOG_ERR("Error in %s autonegotiation", dev->name);
		data->flags &= ~KSZ8081_SILENCE_DEBUG_LOGS; /* get logs next time */
		turn_on_logs = true;
	}

	data->flags &= ~KSZ8081_LINK_STATE_VALID;
	rc = phy_mc_ksz8081_update_link(dev);
	if (rc == 0) {
		data->flags |= KSZ8081_LINK_STATE_VALID;
	}
	if (!turn_on_logs) {
		turn_on_logs = (rc != 0);
	}
	if (rc == 0 && memcmp(&state, &data->state, sizeof(struct phy_link_state)) != 0) {
		if (data->cb) {
			data->cb(dev, &data->state, data->cb_data);
		}
		LOG_INF("PHY %d is %s", config->addr, data->state.is_up ? "up" : "down");
		if (data->state.is_up) {
			LOG_INF("PHY (%d) Link speed %s Mb, %s duplex\n", config->addr,
				(PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
				PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");
		}
	}

	if (turn_on_logs) {
		/* something wrong, if it happens again, we'll get logs next time */
		data->flags &= ~KSZ8081_SILENCE_DEBUG_LOGS;
	} else {
		/* everything is fine, don't need to spam annoying register logs */
		data->flags |= KSZ8081_SILENCE_DEBUG_LOGS;
	}

	if (USING_INTERRUPT_GPIO) {
		return;
	}

	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static int ksz8081_init_int_gpios(const struct device *dev)
{
	const struct mc_ksz8081_config *config = dev->config;
	struct mc_ksz8081_data *data = dev->data;
	int ret;

	if (config->interrupt_gpio.port == NULL) {
		return 0;
	}

	/* Configure interrupt pin */
	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		goto done;
	}

	gpio_init_callback(&data->gpio_callback, phy_mc_ksz8081_interrupt_handler,
			   BIT(config->interrupt_gpio.pin));

	ret = gpio_add_callback_dt(&config->interrupt_gpio, &data->gpio_callback);
	if (ret < 0) {
		goto done;
	}

	ret = phy_mc_ksz8081_config_interrupt(dev);
	if (ret < 0) {
		goto done;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
done:
	if (ret < 0) {
		LOG_ERR("PHY (%d) config interrupt failed", config->addr);
	}

	return ret;
}
#else
#define ksz8081_init_int_gpios(dev) 0
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
static int ksz8081_init_reset_gpios(const struct device *dev)
{
	const struct mc_ksz8081_config *config = dev->config;

	if (config->reset_gpio.port == NULL) {
		return 0;
	}

	return gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
}
#else
#define ksz8081_init_reset_gpios(dev) 0
#endif

static int phy_mc_ksz8081_init(const struct device *dev)
{
	const struct mc_ksz8081_config *config = dev->config;
	struct mc_ksz8081_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_busy_wait(100000);

	ret = k_mutex_init(&data->mutex);
	if (ret) {
		return ret;
	}

	mdio_bus_enable(config->mdio_dev);
	k_busy_wait(100000);

	ret = ksz8081_init_reset_gpios(dev);
	if (ret) {
		return ret;
	}
	k_busy_wait(100000);

	/* Reset PHY */
	ret = phy_mc_ksz8081_reset(dev);
	if (ret) {
		return ret;
	}

	ret = ksz8081_init_int_gpios(dev);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(100000);
	k_work_init_delayable(&data->phy_monitor_work,
				phy_mc_ksz8081_monitor_work_handler);

	/* Advertise default speeds */
	k_busy_wait(100000);
	phy_mc_ksz8081_cfg_link(dev, config->default_speeds, 0);

	return 0;
}

static DEVICE_API(ethphy, mc_ksz8081_phy_api) = {
	.get_link = phy_mc_ksz8081_get_link,
	.cfg_link = phy_mc_ksz8081_cfg_link,
	.link_cb_set = phy_mc_ksz8081_link_cb_set,
	.read = phy_mc_ksz8081_read,
	.write = phy_mc_ksz8081_write,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define RESET_GPIO(n) \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define RESET_GPIO(n)
#endif /* reset gpio */

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
#define INTERRUPT_GPIO(n) \
		.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),
#else
#define INTERRUPT_GPIO(n)
#endif /* interrupt gpio */

#define MICROCHIP_KSZ8081_INIT(n)						\
	static const struct mc_ksz8081_config mc_ksz8081_##n##_config = {	\
		.addr = DT_INST_REG_ADDR(n),					\
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),			\
		.phy_iface = DT_INST_ENUM_IDX(n, microchip_interface_type),	\
		.default_speeds = PHY_INST_GENERATE_DEFAULT_SPEEDS(n),		\
		RESET_GPIO(n)							\
		INTERRUPT_GPIO(n)						\
	};									\
										\
	static struct mc_ksz8081_data mc_ksz8081_##n##_data;			\
										\
	DEVICE_DT_INST_DEFINE(n, &phy_mc_ksz8081_init, NULL,			\
			&mc_ksz8081_##n##_data, &mc_ksz8081_##n##_config,	\
			POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,			\
			&mc_ksz8081_phy_api);

DT_INST_FOREACH_STATUS_OKAY(MICROCHIP_KSZ8081_INIT)
