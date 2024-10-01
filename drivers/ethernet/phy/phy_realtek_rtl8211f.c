/*
 * Copyright 2023-2024 NXP
 *
 * Inspiration from phy_mii.c, which is:
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rtl8211f

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/drivers/mdio.h>
#include <string.h>
#include <zephyr/sys/util_macro.h>
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) || DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
#include <zephyr/drivers/gpio.h>
#endif

#define LOG_MODULE_NAME phy_rt_rtl8211f
#define LOG_LEVEL CONFIG_PHY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define REALTEK_OUI_MSB (0x1CU)

#define PHY_RT_RTL8211F_PHYSR_REG (0x1A)

#define PHY_RT_RTL8211F_PHYSR_LINKSTATUS_MASK BIT(2)
#define PHY_RT_RTL8211F_PHYSR_LINKDUPLEX_MASK BIT(3)
#define PHY_RT_RTL8211F_PHYSR_LINKSPEED_MASK  (BIT(4) | BIT(5))
#define PHY_RT_RTL8211F_PHYSR_LINKSPEED_SHIFT (4U)
#define PHY_RT_RTL8211F_PHYSR_LINKSPEED_10M   (0U)
#define PHY_RT_RTL8211F_PHYSR_LINKSPEED_100M  (1U)
#define PHY_RT_RTL8211F_PHYSR_LINKSPEED_1000M (2U)

#define PHY_RT_RTL8211F_PAGSR_REG (0x1F)

#define PHY_RT_RTL8211F_PAGE_MIICR_ADDR   (0xD08)
#define PHY_RT_RTL8211F_MIICR1_REG        (0x11)
#define PHY_RT_RTL8211F_MIICR2_REG        (0x15)
#define PHY_RT_RTL8211F_MIICR1_TXDLY_MASK BIT(8)
#define PHY_RT_RTL8211F_MIICR2_RXDLY_MASK BIT(3)

#define PHY_RT_RTL8211F_PAGE_INTR_PIN_ADDR (0xD40)
#define PHY_RT_RTL8211F_INTR_PIN_REG       (0x16)
#define PHY_RT_RTL8211F_INTR_PIN_MASK      BIT(5)

#define PHY_RT_RTL8211F_PAGE_INTR_ADDR              (0xA42U)
#define PHY_RT_RTL8211F_INER_REG                    (0x12U)
#define PHY_RT_RTL8211F_INER_LINKSTATUS_CHANGE_MASK BIT(4)
#define PHY_RT_RTL8211F_INSR_REG                    (0x1DU)

#define PHY_RT_RTL8211F_RESET_HOLD_TIME_MS 10

struct rt_rtl8211f_config {
	uint8_t addr;
	const struct device *mdio_dev;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	const struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct gpio_dt_spec interrupt_gpio;
#endif
};

struct rt_rtl8211f_data {
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

static int phy_rt_rtl8211f_read(const struct device *dev,
				uint16_t reg_addr, uint32_t *data)
{
	const struct rt_rtl8211f_config *config = dev->config;
	int ret;

	/* Make sure excessive bits 16-31 are reset */
	*data = 0U;

	/* Read the PHY register */
	ret = mdio_read(config->mdio_dev, config->addr, reg_addr, (uint16_t *)data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int phy_rt_rtl8211f_write(const struct device *dev,
				uint16_t reg_addr, uint32_t data)
{
	const struct rt_rtl8211f_config *config = dev->config;
	int ret;

	ret = mdio_write(config->mdio_dev, config->addr, reg_addr, (uint16_t)data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int phy_rt_rtl8211f_reset(const struct device *dev)
{
	const struct rt_rtl8211f_config *config = dev->config;
	uint32_t reg_val;
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (config->reset_gpio.port) {
		/* Start reset */
		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret) {
			return ret;
		}

		/* Hold reset for the minimum time specified by datasheet */
		k_busy_wait(USEC_PER_MSEC * PHY_RT_RTL8211F_RESET_HOLD_TIME_MS);

		/* Reset over */
		ret = gpio_pin_set_dt(&config->reset_gpio, 1);
		if (ret) {
			return ret;
		}

		/* Wait another 30 ms (circuits settling time) before accessing registers */
		k_busy_wait(USEC_PER_MSEC * 30);

		goto finalize_reset;
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) */

	/* Reset PHY using register */
	ret = phy_rt_rtl8211f_write(dev, MII_BMCR, MII_BMCR_RESET);
	if (ret) {
		LOG_ERR("Error writing phy (%d) basic control register", config->addr);
		return ret;
	}

	/* Wait for the minimum reset time specified by datasheet */
	k_busy_wait(USEC_PER_MSEC * PHY_RT_RTL8211F_RESET_HOLD_TIME_MS);

	/* Wait for the reset to be cleared */
	do {
		ret = phy_rt_rtl8211f_read(dev, MII_BMCR, &reg_val);
		if (ret) {
			LOG_ERR("Error reading phy (%d) basic control register", config->addr);
			return ret;
		}
	} while (reg_val & MII_BMCR_RESET);

	goto finalize_reset;

finalize_reset:
	/* Wait until correct data can be read from registers */
	do {
		ret = phy_rt_rtl8211f_read(dev, MII_PHYID1R, &reg_val);
		if (ret) {
			LOG_ERR("Error reading phy (%d) identifier register 1", config->addr);
			return ret;
		}
	} while (reg_val != REALTEK_OUI_MSB);

	return 0;
}

static int phy_rt_rtl8211f_restart_autonegotiation(const struct device *dev)
{
	const struct rt_rtl8211f_config *config = dev->config;
	uint32_t bmcr = 0;
	int ret;

	/* Read control register to write back with autonegotiation bit */
	ret = phy_rt_rtl8211f_read(dev, MII_BMCR, &bmcr);
	if (ret) {
		LOG_ERR("Error reading phy (%d) basic control register", config->addr);
		return ret;
	}

	/* (re)start autonegotiation */
	LOG_DBG("PHY (%d) is entering autonegotiation sequence", config->addr);
	bmcr |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;

	ret = phy_rt_rtl8211f_write(dev, MII_BMCR, bmcr);
	if (ret) {
		LOG_ERR("Error writing phy (%d) basic control register", config->addr);
		return ret;
	}

	return 0;
}

static int phy_rt_rtl8211f_get_link(const struct device *dev,
					struct phy_link_state *state)
{
	const struct rt_rtl8211f_config *config = dev->config;
	struct rt_rtl8211f_data *data = dev->data;
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

	/* Read PHY specific status register */
	ret = phy_rt_rtl8211f_read(dev, PHY_RT_RTL8211F_PHYSR_REG, &physr);
	if (ret) {
		LOG_ERR("Error reading phy (%d) specific status register", config->addr);
		(void)k_mutex_unlock(&data->mutex);
		return ret;
	}

	/* Unlock mutex */
	(void)k_mutex_unlock(&data->mutex);

	new_state.is_up = physr & PHY_RT_RTL8211F_PHYSR_LINKSTATUS_MASK;

	if (!new_state.is_up) {
		goto result;
	}

	duplex = (physr & PHY_RT_RTL8211F_PHYSR_LINKDUPLEX_MASK);
	switch ((physr & PHY_RT_RTL8211F_PHYSR_LINKSPEED_MASK)
				>> PHY_RT_RTL8211F_PHYSR_LINKSPEED_SHIFT) {
		case PHY_RT_RTL8211F_PHYSR_LINKSPEED_100M:
			if (duplex) {
				new_state.speed = LINK_FULL_100BASE_T;
			} else {
				new_state.speed = LINK_HALF_100BASE_T;
			}
			break;
		case PHY_RT_RTL8211F_PHYSR_LINKSPEED_1000M:
			if (duplex) {
				new_state.speed = LINK_FULL_1000BASE_T;
			} else {
				new_state.speed = LINK_HALF_1000BASE_T;
			}
			break;
		case PHY_RT_RTL8211F_PHYSR_LINKSPEED_10M:
		default:
			if (duplex) {
				new_state.speed = LINK_FULL_10BASE_T;
			} else {
				new_state.speed = LINK_HALF_10BASE_T;
			}
			break;
	}
result:
	if (memcmp(&old_state, &new_state, sizeof(struct phy_link_state)) != 0) {
		LOG_INF("PHY %d is %s", config->addr, new_state.is_up ? "up" : "down");
		if (new_state.is_up) {
			LOG_INF("PHY (%d) Link speed %s Mb, %s duplex", config->addr,
				(PHY_LINK_IS_SPEED_1000M(new_state.speed) ? "1000" :
				(PHY_LINK_IS_SPEED_100M(new_state.speed) ? "100" : "10")),
				PHY_LINK_IS_FULL_DUPLEX(new_state.speed) ? "full" : "half");
		}
	}

	memcpy(state, &new_state, sizeof(struct phy_link_state));

	return ret;
}

static int phy_rt_rtl8211f_cfg_link(const struct device *dev,
					enum phy_link_speed speeds)
{
	const struct rt_rtl8211f_config *config = dev->config;
	struct rt_rtl8211f_data *data = dev->data;
	uint32_t anar;
	uint32_t gbcr;
	int ret;

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	/* We are going to reconfigure the phy, don't need to monitor until done */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!config->interrupt_gpio.port) {
		k_work_cancel_delayable(&data->phy_monitor_work);
	}
#else
	k_work_cancel_delayable(&data->phy_monitor_work);
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	/* Read ANAR register to write back */
	ret = phy_rt_rtl8211f_read(dev, MII_ANAR, &anar);
	if (ret) {
		LOG_ERR("Error reading phy (%d) advertising register", config->addr);
		goto done;
	}

	/* Read GBCR register to write back */
	ret = phy_rt_rtl8211f_read(dev, MII_1KTCR, &gbcr);
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
		gbcr |= MII_ADVERTISE_1000_FULL;
	} else {
		gbcr &= ~MII_ADVERTISE_1000_FULL;
	}

	/* Write capabilities to advertising register */
	ret = phy_rt_rtl8211f_write(dev, MII_ANAR, anar);
	if (ret) {
		LOG_ERR("Error writing phy (%d) advertising register", config->addr);
		goto done;
	}

	/* Write capabilities to 1000Base-T control register */
	ret = phy_rt_rtl8211f_write(dev, MII_1KTCR, gbcr);
	if (ret) {
		LOG_ERR("Error writing phy (%d) 1000Base-T control register", config->addr);
		goto done;
	}

	/* (Re)start autonegotiation */
	ret = phy_rt_rtl8211f_restart_autonegotiation(dev);
	if (ret) {
		LOG_ERR("Error restarting autonegotiation");
		goto done;
	}
done:
	/* Unlock mutex */
	(void)k_mutex_unlock(&data->mutex);

	/* Start monitoring */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!config->interrupt_gpio.port) {
		k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
	}
#else
	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	return ret;
}

static int phy_rt_rtl8211f_link_cb_set(const struct device *dev,
					phy_callback_t cb, void *user_data)
{
	struct rt_rtl8211f_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	phy_rt_rtl8211f_get_link(dev, &data->state);

	data->cb(dev, &data->state, data->cb_data);

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static int phy_rt_rtl8211f_clear_interrupt(struct rt_rtl8211f_data *data)
{
	const struct device *dev = data->dev;
	const struct rt_rtl8211f_config *config = dev->config;
	uint32_t reg_val;
	int ret;

	/* Lock mutex */
	ret = k_mutex_lock(&data->mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("PHY mutex lock error");
		return ret;
	}

	/* Read/clear PHY interrupt status register */
	ret = phy_rt_rtl8211f_read(dev, PHY_RT_RTL8211F_INSR_REG, &reg_val);
	if (ret) {
		LOG_ERR("Error reading phy (%d) interrupt status register", config->addr);
	}

	/* Unlock mutex */
	(void)k_mutex_unlock(&data->mutex);

	return ret;
}

static void phy_rt_rtl8211f_interrupt_handler(const struct device *port,
						struct gpio_callback *cb,
						gpio_port_pins_t pins)
{
	struct rt_rtl8211f_data *data = CONTAINER_OF(cb, struct rt_rtl8211f_data, gpio_callback);
	int ret;

	ret = k_work_reschedule(&data->phy_monitor_work, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to schedule phy_monitor_work from ISR");
	}
}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

static void phy_rt_rtl8211f_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct rt_rtl8211f_data *data =
		CONTAINER_OF(dwork, struct rt_rtl8211f_data, phy_monitor_work);
	const struct device *dev = data->dev;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct rt_rtl8211f_config *config = dev->config;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
	struct phy_link_state state = {};
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (config->interrupt_gpio.port) {
		ret = phy_rt_rtl8211f_clear_interrupt(data);
		if (ret) {
			return;
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	ret = phy_rt_rtl8211f_get_link(dev, &state);

	if (ret == 0 && memcmp(&state, &data->state, sizeof(struct phy_link_state)) != 0) {
		memcpy(&data->state, &state, sizeof(struct phy_link_state));
		if (data->cb) {
			data->cb(dev, &data->state, data->cb_data);
		}
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!config->interrupt_gpio.port) {
		k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
	}
#else
	k_work_reschedule(&data->phy_monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
}

static int phy_rt_rtl8211f_init(const struct device *dev)
{
	const struct rt_rtl8211f_config *config = dev->config;
	struct rt_rtl8211f_data *data = dev->data;
	uint32_t reg_val;
	int ret;

	data->dev = dev;

	ret = k_mutex_init(&data->mutex);
	if (ret) {
		return ret;
	}

	mdio_bus_enable(config->mdio_dev);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	/* Configure reset pin */
	if (config->reset_gpio.port) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) */

	/* Reset PHY */
	ret = phy_rt_rtl8211f_reset(dev);
	if (ret) {
		LOG_ERR("Failed to reset phy (%d)", config->addr);
		return ret;
	}

	/* Set RGMII Tx/Rx Delay. */
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_PAGSR_REG,
					PHY_RT_RTL8211F_PAGE_MIICR_ADDR);
	if (ret) {
		LOG_ERR("Error writing phy (%d) page select register", config->addr);
		return ret;
	}

	ret = phy_rt_rtl8211f_read(dev, PHY_RT_RTL8211F_MIICR1_REG, &reg_val);
	if (ret) {
		LOG_ERR("Error reading phy (%d) mii control register1", config->addr);
		return ret;
	}

	reg_val |= PHY_RT_RTL8211F_MIICR1_TXDLY_MASK;
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_MIICR1_REG, reg_val);
	if (ret) {
		LOG_ERR("Error writing phy (%d) mii control register1", config->addr);
		return ret;
	}

	ret = phy_rt_rtl8211f_read(dev, PHY_RT_RTL8211F_MIICR2_REG, &reg_val);
	if (ret) {
		LOG_ERR("Error reading phy (%d) mii control register2", config->addr);
		return ret;
	}

	reg_val |= PHY_RT_RTL8211F_MIICR2_RXDLY_MASK;
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_MIICR2_REG, reg_val);
	if (ret) {
		LOG_ERR("Error writing phy (%d) mii control register2", config->addr);
		return ret;
	}

	/* Restore to default page 0 */
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_PAGSR_REG, 0);
	if (ret) {
		LOG_ERR("Error writing phy (%d) page select register", config->addr);
		return ret;
	}

	k_work_init_delayable(&data->phy_monitor_work, phy_rt_rtl8211f_monitor_work_handler);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!config->interrupt_gpio.port) {
		phy_rt_rtl8211f_monitor_work_handler(&data->phy_monitor_work.work);
		goto skip_int_gpio;
	}

	/* Set INTB/PMEB pin to interrupt mode */
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_PAGSR_REG,
					PHY_RT_RTL8211F_PAGE_INTR_PIN_ADDR);
	if (ret) {
		LOG_ERR("Error writing phy (%d) page select register", config->addr);
		return ret;
	}
	ret = phy_rt_rtl8211f_read(dev, PHY_RT_RTL8211F_INTR_PIN_REG, &reg_val);
	if (!ret) {
		reg_val &= ~PHY_RT_RTL8211F_INTR_PIN_MASK;
		ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_INTR_PIN_REG, reg_val);
		if (ret) {
			LOG_ERR("Error writing phy (%d) interrupt pin setting register",
				config->addr);
			return ret;
		}
	} else {
		LOG_ERR("Error reading phy (%d) interrupt pin setting register", config->addr);
		return ret;
	}
	/* Restore to default page 0 */
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_PAGSR_REG, 0);
	if (ret) {
		LOG_ERR("Error writing phy (%d) page select register", config->addr);
		return ret;
	}

	/* Clear interrupt */
	ret = phy_rt_rtl8211f_clear_interrupt(data);
	if (ret) {
		return ret;
	}

	/* Configure interrupt pin */
	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	gpio_init_callback(&data->gpio_callback, phy_rt_rtl8211f_interrupt_handler,
				BIT(config->interrupt_gpio.pin));
	ret = gpio_add_callback_dt(&config->interrupt_gpio, &data->gpio_callback);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		return ret;
	}

	/* Enable PHY interrupt. */
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_PAGSR_REG,
					PHY_RT_RTL8211F_PAGE_INTR_ADDR);
	if (ret) {
		LOG_ERR("Error writing phy (%d) page select register", config->addr);
		return ret;
	}
	ret = phy_rt_rtl8211f_read(dev, PHY_RT_RTL8211F_INER_REG, &reg_val);
	if (ret) {
		LOG_ERR("Error reading phy (%d) interrupt enable register", config->addr);
		return ret;
	}
	reg_val |= PHY_RT_RTL8211F_INER_LINKSTATUS_CHANGE_MASK;
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_INER_REG, reg_val);
	if (ret) {
		LOG_ERR("Error writing phy (%d) interrupt enable register", config->addr);
		return ret;
	}
	/* Restore to default page 0 */
	ret = phy_rt_rtl8211f_write(dev, PHY_RT_RTL8211F_PAGSR_REG, 0);
	if (ret) {
		LOG_ERR("Error writing phy (%d) page select register", config->addr);
		return ret;
	}
skip_int_gpio:
#else
	phy_rt_rtl8211f_monitor_work_handler(&data->phy_monitor_work.work);
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	return 0;
}

static const struct ethphy_driver_api rt_rtl8211f_phy_api = {
	.get_link = phy_rt_rtl8211f_get_link,
	.cfg_link = phy_rt_rtl8211f_cfg_link,
	.link_cb_set = phy_rt_rtl8211f_link_cb_set,
	.read = phy_rt_rtl8211f_read,
	.write = phy_rt_rtl8211f_write,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define RESET_GPIO(n) \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define RESET_GPIO(n)
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) */

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
#define INTERRUPT_GPIO(n) \
		.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),
#else
#define INTERRUPT_GPIO(n)
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

#define REALTEK_RTL8211F_INIT(n)						\
	static const struct rt_rtl8211f_config rt_rtl8211f_##n##_config = {	\
		.addr = DT_INST_REG_ADDR(n),					\
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),			\
		RESET_GPIO(n)							\
		INTERRUPT_GPIO(n)						\
	};									\
										\
	static struct rt_rtl8211f_data rt_rtl8211f_##n##_data;			\
										\
	DEVICE_DT_INST_DEFINE(n, &phy_rt_rtl8211f_init, NULL,			\
			&rt_rtl8211f_##n##_data, &rt_rtl8211f_##n##_config,	\
			POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,			\
			&rt_rtl8211f_phy_api);

DT_INST_FOREACH_STATUS_OKAY(REALTEK_RTL8211F_INIT)
