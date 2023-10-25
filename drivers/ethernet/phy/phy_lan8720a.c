/*
 * Copyright 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_lan8720a

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_lan8720a, CONFIG_PHY_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/gpio.h>

#define LAN8720A_INTERRUPT_SOURCE_REG		0x1D
#define LAN8720A_INTERRUPT_MASK_REG		0x1E

#define LAN8720A_INTERRUPT_ENERGYON		BIT(7)
#define LAN8720A_INTERRUPT_AUTO_NEGOTIATE	BIT(6)
#define LAN8720A_INTERRUPT_LINK_DOWN		BIT(4)

/*
 * Per the IEEE 802.3u standard, clause 22 (22.2.4.1.1) the reset process will be completed
 * within 0.5s from the setting of the soft reset bit.
 */
#define LAN8720A_SOFT_RESET_TIMEOUT		K_MSEC(500)
/* The datasheet mentions a state-machine completion of approximately 1200ms */
#define LAN8720A_AUTO_NEGOTIATE_TIMEOUT		K_MSEC(1500)
#define LAN8720A_POLL_TIMEOUT			K_MSEC(50)

struct lan8720a_config {
	uint8_t addr;
	const struct device *mdio_dev;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	const struct gpio_dt_spec rst_gpio;
#endif
	const struct gpio_dt_spec irq_gpio;
};

struct lan8720a_data {
	const struct device *self;

	struct k_mutex mutex;
	struct phy_link_state state;

	struct k_work irq_work;
	struct gpio_callback irq_callback;

	phy_callback_t cb;
	void *cb_data;
};

static int phy_lan8720a_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct lan8720a_config *cfg = dev->config;

	return mdio_read(cfg->mdio_dev, cfg->addr, reg_addr, (uint16_t *)data);
}

static int phy_lan8720a_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct lan8720a_config *cfg = dev->config;

	return mdio_write(cfg->mdio_dev, cfg->addr, reg_addr, (uint16_t)data);
}

static int phy_lan8720a_soft_reset(const struct device *dev)
{
	struct lan8720a_data *data = dev->data;
	uint32_t bmcr;
	int ret;
	k_timepoint_t end;

	(void)k_mutex_lock(&data->mutex, K_FOREVER);

	ret = phy_lan8720a_read(dev, MII_BMCR, &bmcr);
	if (ret < 0) {
		LOG_ERR("Failed to read BMCR (%d)", ret);
		goto unlock;
	}

	/* Software MDIO reset */
	bmcr |= MII_BMCR_RESET;

	ret = phy_lan8720a_write(dev, MII_BMCR, bmcr);
	if (ret < 0) {
		LOG_ERR("Failed to write BMCR (%d)", ret);
		goto unlock;
	}

	end = sys_timepoint_calc(LAN8720A_SOFT_RESET_TIMEOUT);
	do {
		if (sys_timepoint_expired(end)) {
			ret = -ETIMEDOUT;
			goto unlock;
		}

		k_sleep(LAN8720A_POLL_TIMEOUT);

		ret = phy_lan8720a_read(dev, MII_BMCR, &bmcr);
		if (ret < 0) {
			LOG_ERR("Failed to read BMCR (%d)", ret);
			goto unlock;
		}
	} while ((bmcr & MII_BMCR_RESET) == MII_BMCR_RESET);

	/* (Re-)Set interrupt mask */
	ret = phy_lan8720a_write(dev, LAN8720A_INTERRUPT_MASK_REG, LAN8720A_INTERRUPT_LINK_DOWN |
				 LAN8720A_INTERRUPT_ENERGYON);
	if (ret < 0) {
		LOG_ERR("Failed to set interrupts (%d)", ret);
		goto unlock;
	}

unlock:
	(void)k_mutex_unlock(&data->mutex);

	return ret;
}

static int phy_lan8720a_auto_negotiate(const struct device *dev)
{
	const struct lan8720a_config *config = dev->config;
	int ret;
	uint32_t bmcr, bmsr;
	k_timepoint_t end;

	/* Read control register to write back with auto-negotiation bit */
	ret = phy_lan8720a_read(dev, MII_BMCR, &bmcr);
	if (ret < 0) {
		LOG_ERR("Failed to read BMCR (%d)", ret);
		return ret;
	}

	/* (re)start auto-negotiation */
	LOG_DBG("PHY (%d) is entering autonegotiation sequence", config->addr);
	bmcr |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	bmcr &= ~MII_BMCR_ISOLATE;

	ret = phy_lan8720a_write(dev, MII_BMCR, bmcr);
	if (ret < 0) {
		return ret;
	}

	end = sys_timepoint_calc(LAN8720A_AUTO_NEGOTIATE_TIMEOUT);
	do {
		if (sys_timepoint_expired(end)) {
			return -ETIMEDOUT;
		}

		k_sleep(LAN8720A_POLL_TIMEOUT);

		ret = phy_lan8720a_read(dev, MII_BMSR, &bmsr);
		if (ret < 0) {
			return ret;
		}
	} while ((bmsr & MII_BMSR_AUTONEG_COMPLETE) == 0U);

	return 0;
}

static int phy_lan8720a_get_link(const struct device *dev, struct phy_link_state *state)
{
	const struct lan8720a_config *config = dev->config;
	struct lan8720a_data *data = dev->data;
	int ret;
	uint32_t bmsr, anar, anlpar, mutual_capabilities;

	(void)k_mutex_lock(&data->mutex, K_FOREVER);

	/* Read link state */
	ret = phy_lan8720a_read(dev, MII_BMSR, &bmsr);
	if (ret < 0) {
		LOG_ERR("Failed to read BMSR (%d)", ret);
		goto unlock;
	}
	state->is_up = bmsr & MII_BMSR_LINK_STATUS;

	/* Read currently configured advertising options */
	ret = phy_lan8720a_read(dev, MII_ANAR, &anar);
	if (ret < 0) {
		LOG_ERR("Failed to read ANAR (%d)", ret);
		goto unlock;
	}

	/* Read link partner capability */
	ret = phy_lan8720a_read(dev, MII_ANLPAR, &anlpar);
	if (ret < 0) {
		LOG_ERR("Failed to read ANLPAR (%d)", ret);
		goto unlock;
	}

	mutual_capabilities = anar & anlpar;

	if (mutual_capabilities & MII_ADVERTISE_100_FULL) {
		state->speed = LINK_FULL_100BASE_T;
	} else if (mutual_capabilities & MII_ADVERTISE_100_HALF) {
		state->speed = LINK_HALF_100BASE_T;
	} else if (mutual_capabilities & MII_ADVERTISE_10_FULL) {
		state->speed = LINK_FULL_10BASE_T;
	} else if (mutual_capabilities & MII_ADVERTISE_10_HALF) {
		state->speed = LINK_HALF_10BASE_T;
	} else {
		LOG_ERR("No valid PHY %d capabilities", config->addr);
		ret = -EIO;
	}

	LOG_DBG("PHY (%d) Link speed %s Mb, %s duplex\n", config->addr,
		(PHY_LINK_IS_SPEED_100M(state->speed) ? "100" : "10"),
		PHY_LINK_IS_FULL_DUPLEX(state->speed) ? "full" : "half");

unlock:
	(void)k_mutex_unlock(&data->mutex);

	return ret;
}

static int phy_lan8720a_cfg_link(const struct device *dev, enum phy_link_speed speeds)
{
	const struct lan8720a_config *config = dev->config;
	struct lan8720a_data *data = dev->data;
	int ret;
	uint32_t anar;

	(void)k_mutex_lock(&data->mutex, K_FOREVER);

	/* Read ANAR register to write back */
	ret = phy_lan8720a_read(dev, MII_ANAR, &anar);
	if (ret < 0) {
		LOG_ERR("Failed to read ANAR (%d)", ret);
		goto unlock;
	}

	/* Setup advertising register */
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

	/* Write capabilities to advertising register */
	ret = phy_lan8720a_write(dev, MII_ANAR, anar);
	if (ret < 0) {
		LOG_ERR("Failed to write ANAR (%d)", ret);
		goto unlock;
	}

	/* (re)do auto-negotiation */
	ret = phy_lan8720a_auto_negotiate(dev);
	if (ret < 0) {
		LOG_ERR("Auto-negotiation error (%d)", ret);
		goto unlock;
	}

	/* Get link status */
	ret = phy_lan8720a_get_link(dev, &data->state);
	if (ret < 0) {
		LOG_ERR("Failed to get link status (%d)", ret);
		goto unlock;
	}

	/* Log the results of the configuration */
	LOG_INF("PHY %d is %s", config->addr, data->state.is_up ? "up" : "down");
	LOG_INF("PHY (%d) Link speed %s Mb, %s duplex\n", config->addr,
		(PHY_LINK_IS_SPEED_100M(data->state.speed) ? "100" : "10"),
		PHY_LINK_IS_FULL_DUPLEX(data->state.speed) ? "full" : "half");

unlock:
	(void)k_mutex_unlock(&data->mutex);

	return ret;
}

static int phy_lan8720a_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct lan8720a_data *data = dev->data;
	int ret;

	data->cb = cb;
	data->cb_data = user_data;

	ret = phy_lan8720a_get_link(dev, &data->state);
	if (ret < 0) {
		return ret;
	}

	data->cb(dev, &data->state, data->cb_data);

	return 0;
}

static void phy_lan8720a_isr(const struct device *port, struct gpio_callback *cb,
			     gpio_port_pins_t pins)
{
	struct lan8720a_data *data = CONTAINER_OF(cb, struct lan8720a_data, irq_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->irq_work);
}

static void phy_lan8720a_work_handler(struct k_work *work)
{
	struct lan8720a_data *data = CONTAINER_OF(work, struct lan8720a_data, irq_work);
	struct phy_link_state new_state;
	int ret;
	uint32_t irq_value;

	ret = phy_lan8720a_read(data->self, LAN8720A_INTERRUPT_SOURCE_REG, &irq_value);
	if (ret < 0) {
		LOG_ERR("Failed to read interrupt register (%d)", ret);
		return;
	}

	ret = phy_lan8720a_get_link(data->self, &new_state);
	if (ret < 0) {
		return;
	}

	if (memcmp(&data->state, &new_state, sizeof(struct phy_link_state)) != 0) {
		memcpy(&data->state, &new_state, sizeof(struct phy_link_state));

		if (data->cb) {
			data->cb(data->self, &data->state, data->cb_data);
		}
	}
}

static int phy_lan8720a_init(const struct device *dev)
{
	const struct lan8720a_config *cfg = dev->config;
	struct lan8720a_data *data = dev->data;
	int ret;

	data->self = dev;

	(void)k_mutex_init(&data->mutex);
	k_work_init(&data->irq_work, phy_lan8720a_work_handler);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (cfg->rst_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->rst_gpio)) {
			LOG_ERR("RST pin not ready");
			return -ENODEV;
		}

		/* Start reset */
		ret = gpio_pin_configure_dt(&cfg->rst_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure RST pin (%d)", ret);
			return ret;
		}

		/* 100us assertion time */
		k_busy_wait(100);

		ret = gpio_pin_set_dt(&cfg->rst_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Failed to de-assert RST pin (%d)", ret);
			return ret;
		}
	}
#endif

	if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
		LOG_ERR("IRQ pin not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure INT pin (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure INT (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->irq_callback, phy_lan8720a_isr, cfg->irq_gpio.pin);
	ret = gpio_add_callback_dt(&cfg->irq_gpio, &data->irq_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add INT callback (%d)", ret);
		return ret;
	}

	mdio_bus_enable(cfg->mdio_dev);

	return 0;
}

static const struct ethphy_driver_api lan8720a_phy_api = {
	.soft_reset = phy_lan8720a_soft_reset,
	.get_link = phy_lan8720a_get_link,
	.cfg_link = phy_lan8720a_cfg_link,
	.link_cb_set = phy_lan8720a_link_cb_set,
	.read = phy_lan8720a_read,
	.write = phy_lan8720a_write,
};

#define LAN8720A_INIT(inst)								\
	static const struct lan8720a_config lan8720a_##inst##_config = {		\
		.addr = DT_INST_REG_ADDR(inst),						\
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),			\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, reset_gpios),			\
			(.rst_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),))	\
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),			\
	};										\
											\
	static struct lan8720a_data lan8720a_##inst##_data;				\
											\
	DEVICE_DT_INST_DEFINE(inst, phy_lan8720a_init, NULL,				\
			      &lan8720a_##inst##_data, &lan8720a_##inst##_config,	\
			      POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,			\
			      &lan8720a_phy_api);

DT_INST_FOREACH_STATUS_OKAY(LAN8720A_INIT)
