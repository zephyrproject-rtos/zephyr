/*
 * Copyright 2024, PHYTEC America, LLC
 * Author: Florijan Plohl <florijan.plohl@norik.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_dp83867

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>
#include <string.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/gpio.h>

#define LOG_MODULE_NAME phy_ti_dp83867
#define LOG_LEVEL       CONFIG_PHY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#define PHY_TI_DP83867_PHYSTS                 0x11
#define PHY_TI_DP83867_PHYSTS_LINKSTATUS_MASK BIT(10)
#define PHY_TI_DP83867_PHYSTS_LINKDUPLEX_MASK BIT(13)
#define PHY_TI_DP83867_PHYSTS_LINKSPEED_MASK  (BIT(14) | BIT(15))
#define PHY_TI_DP83867_PHYSTS_LINKSPEED_SHIFT (14U)

#define PHY_TI_DP83867_PHYSTS_LINKSPEED_10M   (0U)
#define PHY_TI_DP83867_PHYSTS_LINKSPEED_100M  (1U)
#define PHY_TI_DP83867_PHYSTS_LINKSPEED_1000M (2U)

#define PHY_TI_DP83867_RESET_PULSE_WIDTH 1
#define PHY_TI_DP83867_POR_DELAY         200

#define PHY_TI_DP83867_MICR                    0x0012
#define PHY_TI_DP83867_ISR                     0x0013
#define PHY_TI_DP83867_LINK_STATUS_CHNG_INT_EN BIT(10)
#define PHY_TI_DP83867_CFG3                    0x001e
#define PHY_TI_DP83867_INT_EN                  BIT(7)

struct ti_dp83867_config {
	uint8_t addr;
	const struct device *mdio_dev;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	const struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct gpio_dt_spec interrupt_gpio;
#endif
};

struct ti_dp83867_data {
	const struct device *dev;
	struct phy_link_state state;
	phy_callback_t cb;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_callback gpio_callback;
#endif
	void *cb_data;
	struct k_mutex mutex;
	struct k_work_delayable phy_monitor_work;
};

static int phy_ti_dp83867_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct ti_dp83867_config *config = dev->config;
	int ret;

	/* Make sure excessive bits 16-31 are reset */
	*data = 0U;

	ret = mdio_read(config->mdio_dev, config->addr, reg_addr, (uint16_t *)data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int phy_ti_dp83867_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct ti_dp83867_config *config = dev->config;
	int ret;

	ret = mdio_write(config->mdio_dev, config->addr, reg_addr, (uint16_t)data);
	if (ret) {
		return ret;
	}

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static int phy_ti_dp83867_clear_interrupt(struct ti_dp83867_data *data)
{
	const struct device *dev = data->dev;
	const struct ti_dp83867_config *config = dev->config;
	uint32_t reg_val;
	int ret;

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	/* Read/clear PHY interrupt status register */
	ret = phy_ti_dp83867_read(dev, PHY_TI_DP83867_ISR, &reg_val);
	if (ret) {
		LOG_ERR("Error reading phy (%d) interrupt status register", config->addr);
	}

	/* Unlock mutex */
	(void)k_mutex_unlock(&data->mutex);

	return ret;
}

static void phy_ti_dp83867_interrupt_handler(const struct device *port, struct gpio_callback *cb,
					     gpio_port_pins_t pins)
{
	struct ti_dp83867_data *data = CONTAINER_OF(cb, struct ti_dp83867_data, gpio_callback);
	int ret;

	ret = k_work_reschedule(&data->phy_monitor_work, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to schedule phy_monitor_work from ISR");
	}
}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

static int phy_ti_dp83867_autonegotiate(const struct device *dev)
{
	const struct ti_dp83867_config *config = dev->config;
	int ret;
	uint32_t bmcr = 0;

	/* Read control register to write back with autonegotiation bit */
	ret = phy_ti_dp83867_read(dev, MII_BMCR, &bmcr);
	if (ret) {
		LOG_ERR("Error reading phy (%d) basic control register", config->addr);
		return ret;
	}

	/* (re)start autonegotiation */
	LOG_DBG("PHY (%d) is entering autonegotiation sequence", config->addr);
	bmcr |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;

	ret = phy_ti_dp83867_write(dev, MII_BMCR, bmcr);
	if (ret) {
		LOG_ERR("Error writing phy (%d) basic control register", config->addr);
		return ret;
	}

	return 0;
}

static int phy_ti_dp83867_get_link(const struct device *dev, struct phy_link_state *state)
{
	const struct ti_dp83867_config *config = dev->config;
	struct ti_dp83867_data *data = dev->data;
	int ret;
	uint32_t physr = 0;
	uint32_t duplex = 0;
	struct phy_link_state old_state = data->state;
	struct phy_link_state new_state = {};

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	/* Read PHY status register */
	ret = phy_ti_dp83867_read(dev, PHY_TI_DP83867_PHYSTS, &physr);
	if (ret) {
		LOG_ERR("Error reading phy (%d) specific status register", config->addr);
		(void)k_mutex_unlock(&data->mutex);
		return ret;
	}

	/* Unlock mutex */
	(void)k_mutex_unlock(&data->mutex);

	new_state.is_up = physr & PHY_TI_DP83867_PHYSTS_LINKSTATUS_MASK;

	if (new_state.is_up) {
		duplex = (physr & PHY_TI_DP83867_PHYSTS_LINKDUPLEX_MASK);
		switch ((physr & PHY_TI_DP83867_PHYSTS_LINKSPEED_MASK) >>
			PHY_TI_DP83867_PHYSTS_LINKSPEED_SHIFT) {
		case PHY_TI_DP83867_PHYSTS_LINKSPEED_1000M:
			if (duplex) {
				new_state.speed = LINK_FULL_1000BASE_T;
			} else {
				new_state.speed = LINK_HALF_1000BASE_T;
			}
			break;
		case PHY_TI_DP83867_PHYSTS_LINKSPEED_100M:
			if (duplex) {
				new_state.speed = LINK_FULL_100BASE_T;
			} else {
				new_state.speed = LINK_HALF_100BASE_T;
			}
			break;
		case PHY_TI_DP83867_PHYSTS_LINKSPEED_10M:
		default:
			if (duplex) {
				new_state.speed = LINK_FULL_10BASE_T;
			} else {
				new_state.speed = LINK_HALF_10BASE_T;
			}
			break;
		}
	}

	if (memcmp(&old_state, &new_state, sizeof(struct phy_link_state)) != 0) {
		LOG_INF("PHY %d is %s", config->addr, new_state.is_up ? "up" : "down");
		if (new_state.is_up) {
			LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", config->addr,
				(PHY_LINK_IS_SPEED_1000M(new_state.speed)
					 ? "1000"
					 : (PHY_LINK_IS_SPEED_100M(new_state.speed) ? "100"
										    : "10")),
				PHY_LINK_IS_FULL_DUPLEX(new_state.speed) ? "full" : "half");
		}
	}

	memcpy(state, &new_state, sizeof(struct phy_link_state));

	return ret;
}

static int phy_ti_dp83867_reset(const struct device *dev)
{
	const struct ti_dp83867_config *config = dev->config;
	struct ti_dp83867_data *data = dev->data;
	int ret;

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (config->reset_gpio.port) {
		ret = gpio_pin_set_dt(&config->reset_gpio, 1);
		if (ret >= 0) {
			/* Reset pulse (minimum specified width is T1=1us) */
			k_busy_wait(PHY_TI_DP83867_RESET_PULSE_WIDTH);

			ret = gpio_pin_set_dt(&config->reset_gpio, 0);
			if (ret >= 0) {
				goto done;
			}
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) */

	/* Reset PHY using register */
	ret = phy_ti_dp83867_write(dev, MII_BMCR, MII_BMCR_RESET);
	if (ret < 0) {
		LOG_ERR("Error writing phy (%d) basic control register", config->addr);
	}

done:
	/* POR release time (minimum specified is T4=195us) */
	k_busy_wait(PHY_TI_DP83867_POR_DELAY);

	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);
	LOG_DBG("PHY (%d) reset completed", config->addr);

	return ret;
}

static int phy_ti_dp83867_cfg_link(const struct device *dev, enum phy_link_speed speeds)
{
	const struct ti_dp83867_config *config = dev->config;
	struct ti_dp83867_data *data = dev->data;
	int ret;
	uint32_t anar, cfg1, val;

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		goto done;
	}

	/* We are going to reconfigure the phy, don't need to monitor until done */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->interrupt_gpio.port) {
		/* Enable interrupt output register */
		ret = phy_ti_dp83867_read(dev, PHY_TI_DP83867_CFG3, &val);
		if (ret) {
			LOG_ERR("Error reading phy (%d) CFG3 register", config->addr);
			goto done;
		}

		val |= PHY_TI_DP83867_INT_EN;
		ret = phy_ti_dp83867_write(dev, PHY_TI_DP83867_CFG3, val);
		if (ret) {
			LOG_ERR("Error writing phy (%d) CFG3 register", config->addr);
			goto done;
		}

		/* Enable link status change interrupt */
		ret = phy_ti_dp83867_read(dev, PHY_TI_DP83867_MICR, &val);
		if (ret) {
			LOG_ERR("Error reading phy (%d) MICR register", config->addr);
			goto done;
		}

		val |= PHY_TI_DP83867_LINK_STATUS_CHNG_INT_EN;
		ret = phy_ti_dp83867_write(dev, PHY_TI_DP83867_MICR, val);
		if (ret) {
			LOG_ERR("Error writing phy (%d) MICR register", config->addr);
			goto done;
		}

	} else {
		k_work_cancel_delayable(&data->phy_monitor_work);
	}
#else
	k_work_cancel_delayable(&data->phy_monitor_work);
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	/* Read ANAR register to write back */
	ret = phy_ti_dp83867_read(dev, MII_ANAR, &anar);
	if (ret) {
		LOG_ERR("Error reading phy (%d) advertising register", config->addr);
		goto done;
	}

	/* Read CFG1 register to write back */
	ret = phy_ti_dp83867_read(dev, MII_1KTCR, &cfg1);
	if (ret) {
		LOG_ERR("Error reading phy (%d) 1000Base-T control register", config->addr);
		goto done;
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

	/* Setup 1000Base-T control register */
	if (speeds & LINK_FULL_1000BASE_T) {
		cfg1 |= MII_ADVERTISE_1000_FULL;
	} else {
		cfg1 &= ~MII_ADVERTISE_1000_FULL;
	}

	/* Write capabilities to advertising register */
	ret = phy_ti_dp83867_write(dev, MII_ANAR, anar);
	if (ret) {
		LOG_ERR("Error writing phy (%d) advertising register", config->addr);
		goto done;
	}

	/* Write capabilities to 1000Base-T control register */
	ret = phy_ti_dp83867_write(dev, MII_1KTCR, cfg1);
	if (ret) {
		LOG_ERR("Error writing phy (%d) 1000Base-T control register", config->addr);
		goto done;
	}

	/* (re)do autonegotiation */
	ret = phy_ti_dp83867_autonegotiate(dev);
	if (ret && (ret != -ENETDOWN)) {
		LOG_ERR("Error in autonegotiation");
		goto done;
	}

done:
	/* Unlock mutex */
	k_mutex_unlock(&data->mutex);

	/* Start monitoring */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->interrupt_gpio.port) {
		return ret;
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	return ret;
}

static int phy_ti_dp83867_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct ti_dp83867_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	phy_ti_dp83867_get_link(dev, &data->state);

	data->cb(dev, &data->state, data->cb_data);

	return 0;
}

static void phy_ti_dp83867_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ti_dp83867_data *data =
		CONTAINER_OF(dwork, struct ti_dp83867_data, phy_monitor_work);
	const struct device *dev = data->dev;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct ti_dp83867_config *config = dev->config;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
	struct phy_link_state state = {};
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->interrupt_gpio.port) {
		ret = phy_ti_dp83867_clear_interrupt(data);
		if (ret) {
			return;
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	ret = phy_ti_dp83867_get_link(dev, &state);

	if (ret == 0 && memcmp(&state, &data->state, sizeof(struct phy_link_state)) != 0) {
		memcpy(&data->state, &state, sizeof(struct phy_link_state));
		if (data->cb) {
			data->cb(dev, &data->state, data->cb_data);
		}
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->interrupt_gpio.port) {
		return;
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_ti_dp83867_init(const struct device *dev)
{
	const struct ti_dp83867_config *config = dev->config;
	struct ti_dp83867_data *data = dev->data;
	int ret;

	data->dev = dev;

	ret = k_mutex_init(&data->mutex);
	if (ret) {
		return ret;
	}

	mdio_bus_enable(config->mdio_dev);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (config->reset_gpio.port) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) */

	/* Reset PHY */
	ret = phy_ti_dp83867_reset(dev);
	if (ret) {
		LOG_ERR("Failed to reset phy (%d)", config->addr);
		return ret;
	}

	k_work_init_delayable(&data->phy_monitor_work, phy_ti_dp83867_monitor_work_handler);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!config->interrupt_gpio.port) {
		goto skip_int_gpio;
	}

	/* Configure interrupt pin */
	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_callback, phy_ti_dp83867_interrupt_handler,
			   BIT(config->interrupt_gpio.pin));
	ret = gpio_add_callback_dt(&config->interrupt_gpio, &data->gpio_callback);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	return ret;

skip_int_gpio:
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
	phy_ti_dp83867_monitor_work_handler(&data->phy_monitor_work.work);

	return 0;
}

static DEVICE_API(ethphy, ti_dp83867_phy_api) = {
	.get_link = phy_ti_dp83867_get_link,
	.cfg_link = phy_ti_dp83867_cfg_link,
	.link_cb_set = phy_ti_dp83867_link_cb_set,
	.read = phy_ti_dp83867_read,
	.write = phy_ti_dp83867_write,
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

#define TI_DP83867_INIT(n)                                                                         \
	static const struct ti_dp83867_config ti_dp83867_##n##_config = {                          \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		RESET_GPIO(n) INTERRUPT_GPIO(n)};                                                  \
                                                                                                   \
	static struct ti_dp83867_data ti_dp83867_##n##_data;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &phy_ti_dp83867_init, NULL, &ti_dp83867_##n##_data,               \
			      &ti_dp83867_##n##_config, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,     \
			      &ti_dp83867_phy_api);

DT_INST_FOREACH_STATUS_OKAY(TI_DP83867_INIT)
