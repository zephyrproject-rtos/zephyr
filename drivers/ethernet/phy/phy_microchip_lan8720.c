/*
 * Copyright 2024 Muhammad Waleed Badar
 *
 * Inspiration from phy_microchip_ksz8081.c, which is:
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_lan8720

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>
#include <string.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(phy_mc_lan8720, CONFIG_PHY_LOG_LEVEL);

/* LAN8720 PHY identifier */
#define LAN8720_REV_MASK GENMASK(3, 0)
#define LAN8720_DEVID    0x7C0F0U

/* LAN8720 Mode control/status register */
#define LAN8720_MODE_CTRL_STAT_REG            0x11
/* LAN8720 Enable the energy detect power-down mode */
#define LAN8720_MODE_CTRL_STAT_EDPWRDOWN_MASK BIT(13)
/* LAN8720 Alternate interrupt mode */
#define LAN8720_MODE_CTRL_STAT_ALTINT_MASK    BIT(6)
/* LAN8720 Indicates whether energy is detected */
#define LAN8720_MODE_CTRL_STAT_ENERGYON_MASK  BIT(1)

/* LAN8720 Control/status indication register */
#define LAN8720_SPECIAL_CTRL_STAT_IND_REG            0x1B
/* LAN8720 HP Auto-MDIX control */
#define LAN8720_SPECIAL_CTRL_STAT_IND_AMDIXCTRL_MASK BIT(15)
/* LAN8720 Manual channel select */
#define LAN8720_SPECIAL_CTRL_STAT_IND_CH_SEL_MASK    BIT(13)

/* LAN8720 Interrupt source register */
#define LAN8720_INT_SRC_REG       0x1D
/* LAN8720 Interrupt mask register */
#define LAN8720_INT_MASK_REG      0x1E
/* LAN8720 ENERGY ON interrupt */
#define LAN8720_INT_ENERGYON_MASK BIT(7)
/* LAN8720 Auto-Negotiation complete interrupt */
#define LAN8720_INT_ANC_MASK      BIT(6)
/* LAN8720 Link down (link status negated) interrupt */
#define LAN8720_INT_LD_MASK       BIT(4)
/* LAN8720 Auto-Negotiation LP Acknowledge interrupt */
#define LAN8720_INT_LPA_MASK      BIT(3)

/* LAN8720 PHY special control/status register */
#define LAN8720_SPECIAL_CTRL_STATUS_REG 0x1F

struct mc_lan8720_config {
	uint8_t phy_addr;
	const struct device *mdio_dev;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	const struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct gpio_dt_spec interrupt_gpio;
#endif
};

struct mc_lan8720_data {
	const struct device *dev;
	phy_callback_t cb;
	void *cb_data;
	struct phy_link_state state;
	struct k_work_delayable phy_monitor_work;
	struct k_mutex mutex;
};

static int phy_mc_lan8720_get_link(const struct device *dev, struct phy_link_state *state);

static inline int phy_mc_lan8720_reg_read(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct mc_lan8720_config *const config = dev->config;

	return mdio_read(config->mdio_dev, config->phy_addr, reg, val);
}

static inline int phy_mc_lan8720_reg_write(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct mc_lan8720_config *const config = dev->config;

	return mdio_write(config->mdio_dev, config->phy_addr, reg, val);
}

static int phy_mc_lan8720_reset(const struct device *dev)
{
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct mc_lan8720_config *config = dev->config;

	if (config->reset_gpio.port) {
		/* Perform GPIO-based reset */
		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret == 0) {
			k_busy_wait(1000); /* 1ms reset pulse */
			ret = gpio_pin_set_dt(&config->reset_gpio, 1);
		}
	} else {
#endif
		/* Software reset via BMCR register */
		ret = phy_mc_lan8720_reg_write(dev, MII_BMCR, MII_BMCR_RESET);
		if (ret == 0) {
			k_busy_wait(500 * USEC_PER_MSEC); /* Wait for reset */
		}
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	}
#endif

	return ret;
}

static int phy_mc_lan8720_id(const struct device *dev, uint32_t *phy_id)
{
	int ret;
	uint16_t value = 0;

	ret = phy_mc_lan8720_reg_read(dev, MII_PHYID1R, &value);
	if (ret) {
		return ret;
	}

	*phy_id = value << 16;

	ret = phy_mc_lan8720_reg_read(dev, MII_PHYID2R, &value);
	if (ret) {
		return ret;
	}

	*phy_id |= value | LAN8720_REV_MASK;

	return ret;
}

static int phy_mc_lan8720_autonegotiate(const struct device *dev)
{
	uint16_t bmcr = 0;
	uint16_t bmsr = 0;
	uint16_t timeout = CONFIG_PHY_AUTONEG_TIMEOUT_MS / 100;
	int ret;

	/* Read control register */
	ret = phy_mc_lan8720_reg_read(dev, MII_BMCR, &bmcr);
	if (ret) {
		return ret;
	}

	/* Enable and restart autonegotiation */
	bmcr |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	bmcr &= ~MII_BMCR_ISOLATE;

	ret = phy_mc_lan8720_reg_write(dev, MII_BMCR, bmcr);
	if (ret) {
		return ret;
	}

	/* Wait for autonegotiation complete */
	do {
		if (timeout-- == 0) {
			LOG_DBG("PHY autonegotiation timed out");
			return -ENETDOWN;
		}
		k_msleep(100);

		ret = phy_mc_lan8720_reg_read(dev, MII_BMSR, &bmsr);
		if (ret) {
			return ret;
		}

	} while (!(bmsr & MII_BMSR_AUTONEG_COMPLETE));

	LOG_DBG("PHY autonegotiation completed");
	return 0;
}

static int phy_mc_lan8720_amdix_cfg(const struct device *dev)
{
	uint16_t amdix = 0;
	int ret = 0;

	ret = phy_mc_lan8720_reg_read(dev, LAN8720_SPECIAL_CTRL_STAT_IND_REG, &amdix);
	if (ret) {
		return ret;
	}

	if (amdix & LAN8720_SPECIAL_CTRL_STAT_IND_CH_SEL_MASK) {
		amdix &= ~LAN8720_SPECIAL_CTRL_STAT_IND_AMDIXCTRL_MASK &
			 ~LAN8720_SPECIAL_CTRL_STAT_IND_CH_SEL_MASK;
	} else {
		amdix |= ~LAN8720_SPECIAL_CTRL_STAT_IND_AMDIXCTRL_MASK &
			 LAN8720_SPECIAL_CTRL_STAT_IND_CH_SEL_MASK;
	}

	LOG_DBG("PHY Auto-MDIX configuration: 0x%X", amdix);

	ret = phy_mc_lan8720_reg_write(dev, LAN8720_SPECIAL_CTRL_STAT_IND_REG, amdix);
	if (ret) {
		return ret;
	}

	return 0;
}

static int phy_mc_lan8720_update_link_state(const struct device *dev)
{
	const struct mc_lan8720_config *config = dev->config;
	struct mc_lan8720_data *data = dev->data;

	uint16_t bmsr = 0;
	uint16_t anar = 0;
	uint16_t anlpar = 0;
	uint16_t mutual_capabilities = 0;
	bool link_up = 0;
	int ret;

	ret = phy_mc_lan8720_reg_read(dev, MII_BMSR, &bmsr);
	if (ret) {
		return ret;
	}

	link_up = bmsr & MII_BMSR_LINK_STATUS;

	/* If there is no change in link state don't proceed. */
	if (link_up == data->state.is_up) {
		return -EAGAIN;
	}

	data->state.is_up = link_up;

	/* If link is down, change mdi to mdix */
	if (data->state.is_up == false) {
		LOG_INF("PHY (%d) is down", config->phy_addr);

		/* Configure Auto-MDIX */
		ret = phy_mc_lan8720_amdix_cfg(dev);
		if (ret) {
			LOG_DBG("PHY (%d) auto-mdix failed", config->phy_addr);
			return ret;
		}
		return 0;
	}

	LOG_DBG("Starting PHY (%d) auto-negotiate sequence", config->phy_addr);

	ret = phy_mc_lan8720_autonegotiate(dev);
	if (ret) {
		LOG_ERR("PHY (%d) auto-negotiation failed", config->phy_addr);
		return ret;
	}

	/* Read PHY default advertising parameters */
	ret = phy_mc_lan8720_reg_read(dev, MII_ANAR, &anar);
	if (ret) {
		return ret;
	}

	/* Read link partner advertising parameters */
	ret = phy_mc_lan8720_reg_read(dev, MII_ANLPAR, &anlpar);
	if (ret) {
		return ret;
	}

	/* Determine link speed and duplex */
	mutual_capabilities = anar & anlpar;

	if (mutual_capabilities & MII_ADVERTISE_100_FULL) {
		data->state.speed = LINK_FULL_100BASE_T;
	} else if (mutual_capabilities & MII_ADVERTISE_100_HALF) {
		data->state.speed = LINK_HALF_100BASE_T;
	} else if (mutual_capabilities & MII_ADVERTISE_10_FULL) {
		data->state.speed = LINK_FULL_10BASE_T;
	} else if (mutual_capabilities & MII_ADVERTISE_10_HALF) {
		data->state.speed = LINK_HALF_10BASE_T;
	}

	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", config->phy_addr,
		(PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
		PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

	return 0;
}

static void phy_mc_lan8720_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mc_lan8720_data *data =
		CONTAINER_OF(dwork, struct mc_lan8720_data, phy_monitor_work);
	const struct device *dev = data->dev;
	struct phy_link_state state = {};
	int rc;

	k_mutex_lock(&data->mutex, K_FOREVER);

	rc = phy_mc_lan8720_update_link_state(dev);

	k_mutex_unlock(&data->mutex);

	if (rc == 0) {
		phy_mc_lan8720_get_link(dev, &state);
	}

	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_mc_lan8720_read(const struct device *dev, uint16_t reg, uint32_t *val)
{
	return phy_mc_lan8720_reg_read(dev, reg, (uint16_t *)val);
}

static int phy_mc_lan8720_write(const struct device *dev, uint16_t reg, uint32_t val)
{
	return phy_mc_lan8720_reg_write(dev, reg, (uint16_t)val);
}

static int phy_mc_lan8720_cfg_link(const struct device *dev, enum phy_link_speed speeds)
{
	uint16_t anar = 0;
	uint16_t bmcr = 0;
	int ret;

	ret = phy_mc_lan8720_reg_read(dev, MII_ANAR, &anar);
	if (ret) {
		return ret;
	}

	ret = phy_mc_lan8720_reg_read(dev, MII_BMCR, &bmcr);
	if (ret) {
		return ret;
	}

	/* Configure speed capabilities */
	anar &= ~(MII_ADVERTISE_10_HALF | MII_ADVERTISE_10_FULL | MII_ADVERTISE_100_HALF |
		  MII_ADVERTISE_100_FULL);

	if (speeds & LINK_FULL_100BASE_T) {
		anar |= MII_ADVERTISE_100_FULL;
	}
	if (speeds & LINK_HALF_100BASE_T) {
		anar |= MII_ADVERTISE_100_HALF;
	}
	if (speeds & LINK_FULL_10BASE_T) {
		anar |= MII_ADVERTISE_10_FULL;
	}
	if (speeds & LINK_HALF_10BASE_T) {
		anar |= MII_ADVERTISE_10_HALF;
	}

	bmcr |= MII_BMCR_AUTONEG_ENABLE;

	ret = phy_mc_lan8720_reg_write(dev, MII_ANAR, anar);
	if (ret) {
		return ret;
	}

	ret = phy_mc_lan8720_reg_write(dev, MII_BMCR, bmcr);
	if (ret) {
		return ret;
	}

	return ret;
}

static int phy_mc_lan8720_get_link(const struct device *dev, struct phy_link_state *state)
{
	struct mc_lan8720_data *const data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int phy_mc_lan8720_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct mc_lan8720_data *data = dev->data;
	struct phy_link_state state = {};

	data->cb = cb;
	data->cb_data = user_data;

	phy_mc_lan8720_get_link(dev, &state);

	data->cb(dev, &state, data->cb_data);

	return 0;
}

static int phy_mc_lan8720_init(const struct device *dev)
{
	const struct mc_lan8720_config *config = dev->config;
	struct mc_lan8720_data *data = dev->data;
	uint32_t phy_id = 0;
	int ret;

	data->dev = dev;
	data->cb = NULL;
	data->state.is_up = false;

	ret = k_mutex_init(&data->mutex);
	if (ret) {
		return ret;
	}

	mdio_bus_enable(config->mdio_dev);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!config->interrupt_gpio.port) {
		return -ENODEV;
	}

	/* Prevent NAND TREE mode */
	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (!config->reset_gpio.port) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) */

	ret = phy_mc_lan8720_reset(dev);
	if (ret) {
		LOG_ERR("Failed to reset PHY (%d)", config->phy_addr);
		return ret;
	}

	ret = phy_mc_lan8720_id(dev, &phy_id);
	if (ret) {
		LOG_ERR("Failed to read PHY (%d) ID", config->phy_addr);
		return ret;
	}

	if (phy_id != (LAN8720_DEVID | LAN8720_REV_MASK)) {
		LOG_ERR("PHY (%d) unexpected PHY ID 0x%X", config->phy_addr, phy_id);
		return -EINVAL;
	}

	ret = phy_mc_lan8720_cfg_link(dev, LINK_HALF_10BASE_T | LINK_FULL_10BASE_T |
						   LINK_HALF_100BASE_T | LINK_FULL_100BASE_T);
	if (ret) {
		LOG_ERR("Failed to configure PHY (%u)", config->phy_addr);
		return ret;
	}

	k_work_init_delayable(&data->phy_monitor_work, phy_mc_lan8720_monitor_work_handler);

	phy_mc_lan8720_monitor_work_handler(&data->phy_monitor_work.work);

	return 0;
}

static DEVICE_API(ethphy, mc_lan8720_phy_api) = {
	.get_link = phy_mc_lan8720_get_link,
	.cfg_link = phy_mc_lan8720_cfg_link,
	.link_cb_set = phy_mc_lan8720_link_cb_set,
	.read = phy_mc_lan8720_read,
	.write = phy_mc_lan8720_write,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define RESET_GPIO(n) .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define RESET_GPIO(n)
#endif /* reset gpio */

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
#define INTERRUPT_GPIO(n) .interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),
#else
#define INTERRUPT_GPIO(n)
#endif /* interrupt gpio */

#define MICROCHIP_LAN8720_DEVICE(n)                                                                \
	static const struct mc_lan8720_config mc_lan8720_##n##_config = {                          \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.mdio_dev = DEVICE_DT_GET(DT_INST_BUS(n)),                                         \
		RESET_GPIO(n)                                                  \
		INTERRUPT_GPIO(n)                                                  \
	};                                                  \
                                                                                                   \
	static struct mc_lan8720_data mc_lan8720_##n##_data;                                       \
	DEVICE_DT_INST_DEFINE(n, &phy_mc_lan8720_init, NULL, &mc_lan8720_##n##_data,               \
			      &mc_lan8720_##n##_config, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,     \
			      &mc_lan8720_phy_api);

DT_INST_FOREACH_STATUS_OKAY(MICROCHIP_LAN8720_DEVICE)
