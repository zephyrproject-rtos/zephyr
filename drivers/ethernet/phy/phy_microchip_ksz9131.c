/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_ksz9131

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_mchp_ksz9131, CONFIG_PHY_LOG_LEVEL);

struct mchp_ksz9131_config {
	uint8_t phy_addr;
	const struct device *const mdio;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct gpio_dt_spec interrupt_gpio;
#endif
};

struct mchp_ksz9131_data {
	const struct device *dev;
	phy_callback_t cb;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_callback gpio_callback;
#endif
	void *cb_data;
	struct k_work_delayable monitor_work;
	struct phy_link_state state;
	struct k_sem sem;
	bool gigabit_supported;
};

#define PHY_ID_KSZ9131     0x00221640
#define PHY_ID_KSZ9131_MSK (~0xF)

#define PHY_KSZ9131_ICS_REG               0x1B
#define PHY_KSZ9131_ICS_LINK_DOWN_IE_MASK BIT(10)
#define PHY_KSZ9131_ICS_LINK_UP_IE_MASK   BIT(8)

static int ksz9131_read(const struct device *dev, uint16_t reg_addr, uint16_t *value)
{
	const struct mchp_ksz9131_config *const cfg = dev->config;
	int ret = mdio_read(cfg->mdio, cfg->phy_addr, reg_addr, value);

	if (ret < 0) {
		LOG_ERR("Error reading phy (%d) register (%d)", cfg->phy_addr, reg_addr);
	}

	return ret;
}

static int ksz9131_write(const struct device *dev, uint16_t reg_addr, uint16_t value)
{
	const struct mchp_ksz9131_config *const cfg = dev->config;
	int ret = mdio_write(cfg->mdio, cfg->phy_addr, reg_addr, value);

	if (ret < 0) {
		LOG_ERR("Error writing phy (%d) register (%d)", cfg->phy_addr, reg_addr);
	}

	return ret;
}

static int phy_mchp_ksz9131_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	/* Make sure excessive bits 16-31 are reset */
	*data = 0;

	return ksz9131_read(dev, reg_addr, (uint16_t *)data);
}

static int phy_mchp_ksz9131_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	return ksz9131_write(dev, reg_addr, (uint16_t)data);
}

static int read_gigabit_supported_flag(const struct device *dev, bool *supported)
{
	uint16_t bmsr_reg;
	uint16_t estat_reg;

	if (ksz9131_read(dev, MII_BMSR, &bmsr_reg) < 0) {
		return -EIO;
	}

	if ((bmsr_reg & MII_BMSR_EXTEND_STATUS) != 0U) {
		if (ksz9131_read(dev, MII_ESTAT, &estat_reg) < 0) {
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

static int phy_mchp_ksz9131_reset(const struct device *dev)
{
	struct mchp_ksz9131_data *data = dev->data;
	int ret;

	k_sem_take(&data->sem, K_FOREVER);

	ret = ksz9131_write(dev, MII_BMCR, MII_BMCR_RESET);
	if (ret >= 0) {
		/* According to IEEE 802.3, Section 2, Subsection 22.2.4.1.1,
		 * a PHY reset may take up to 0.5 s.
		 */
		k_busy_wait(500 * USEC_PER_MSEC);
	}

	k_sem_give(&data->sem);

	return ret;
}

static int phy_check_ksz9131_id(const struct device *dev)
{
	const struct mchp_ksz9131_config *const cfg = dev->config;
	uint32_t phy_id;
	uint16_t value;
	int ret;

	(void)cfg;

	ret = ksz9131_read(dev, MII_PHYID1R, &value);
	if (ret < 0) {
		return ret;
	}
	phy_id = value << 16;

	ret = ksz9131_read(dev, MII_PHYID2R, &value);
	if (ret < 0) {
		return ret;
	}
	phy_id |= value;

	if ((phy_id & PHY_ID_KSZ9131_MSK) != PHY_ID_KSZ9131) {
		LOG_ERR("PHY (%d) ID 0x%X not as expected", cfg->phy_addr, phy_id);
		return -EINVAL;
	}

	LOG_INF("PHY (%d) ID 0x%X", cfg->phy_addr, phy_id);

	return ret;
}

static int phy_mchp_ksz9131_update_anar(const struct device *dev, enum phy_link_speed adv_speeds)
{
	uint16_t anar;
	int ret;

	ret = ksz9131_read(dev, MII_ANAR, &anar);
	if (ret < 0) {
		return ret;
	}

	if (adv_speeds & LINK_FULL_10BASE) {
		anar |= MII_ADVERTISE_10_FULL;
	} else {
		anar &= ~MII_ADVERTISE_10_FULL;
	}

	if (adv_speeds & LINK_HALF_10BASE) {
		anar |= MII_ADVERTISE_10_HALF;
	} else {
		anar &= ~MII_ADVERTISE_10_HALF;
	}

	if (adv_speeds & LINK_FULL_100BASE) {
		anar |= MII_ADVERTISE_100_FULL;
	} else {
		anar &= ~MII_ADVERTISE_100_FULL;
	}

	if (adv_speeds & LINK_HALF_100BASE) {
		anar |= MII_ADVERTISE_100_HALF;
	} else {
		anar &= ~MII_ADVERTISE_100_HALF;
	}

	return ksz9131_write(dev, MII_ANAR, anar);
}

static int phy_mchp_ksz9131_link_status(const struct device *dev, bool *link_up)
{
	uint16_t bmsr;
	int ret;

	/* Read link state. Read BMSR twice to avoid using the incorrect
	 * status due to the type of bit "Link Status" is RO/LL.
	 */
	ret = ksz9131_read(dev, MII_BMSR, &bmsr);
	if (ret < 0) {
		return ret;
	}
	ret = ksz9131_read(dev, MII_BMSR, &bmsr);
	if (ret < 0) {
		return ret;
	}

	*link_up = bmsr & MII_BMSR_LINK_STATUS;

	return ret;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static int phy_mchp_ksz9131_clear_interrupt(struct mchp_ksz9131_data *data)
{
	const struct device *dev = data->dev;
	const struct mchp_ksz9131_config *cfg = dev->config;
	uint16_t reg_val;
	int ret;

	k_sem_take(&data->sem, K_FOREVER);

	/* Read/clear PHY interrupt status register */
	ret = ksz9131_read(dev, PHY_KSZ9131_ICS_REG, &reg_val);
	if (ret < 0) {
		LOG_ERR("Error reading phy (%d) interrupt status register", cfg->phy_addr);
	}

	k_sem_give(&data->sem);

	return ret;
}

static int phy_mchp_ksz9131_config_interrupt(const struct device *dev)
{
	struct mchp_ksz9131_data *data = dev->data;
	uint16_t reg_val;
	int ret;

	/* Read Interrupt Control/Status register to write back */
	ret = ksz9131_read(dev, PHY_KSZ9131_ICS_REG, &reg_val);
	if (ret < 0) {
		return ret;
	}
	reg_val |= PHY_KSZ9131_ICS_LINK_UP_IE_MASK | PHY_KSZ9131_ICS_LINK_DOWN_IE_MASK;

	/* Write settings to Interrupt Control/Status register */
	ret = ksz9131_write(dev, PHY_KSZ9131_ICS_REG, reg_val);
	if (ret < 0) {
		return ret;
	}

	/* Clear interrupt */
	ret = phy_mchp_ksz9131_clear_interrupt(data);

	return ret;
}

static void phy_mchp_ksz9131_interrupt_handler(const struct device *port, struct gpio_callback *cb,
					       gpio_port_pins_t pins)
{
	struct mchp_ksz9131_data *data = CONTAINER_OF(cb, struct mchp_ksz9131_data, gpio_callback);
	int ret = k_work_reschedule(&data->monitor_work, K_NO_WAIT);

	if (ret < 0) {
		LOG_ERR("Failed to schedule monitor_work from ISR");
	}
}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

static int phy_mchp_ksz9131_autonegotiate(const struct device *dev)
{
	const struct mchp_ksz9131_config *const cfg = dev->config;
	struct mchp_ksz9131_data *const data = dev->data;
	bool link_up;

	uint16_t bmcr = 0;
	uint16_t bmsr = 0;
	uint32_t timeout = CONFIG_PHY_AUTONEG_TIMEOUT_MS / 100;
	int ret;

	ret = phy_mchp_ksz9131_link_status(dev, &link_up);
	if (ret < 0) {
		return ret;
	}
	data->state.is_up = link_up;

	LOG_DBG("PHY (%d) Starting MII PHY auto-negotiate sequence", cfg->phy_addr);

	/* Configure and start auto-negotiation process */
	ret = ksz9131_read(dev, MII_BMCR, &bmcr);
	if (ret < 0) {
		return ret;
	}
	bmcr |= MII_BMCR_AUTONEG_ENABLE | MII_BMCR_AUTONEG_RESTART;
	bmcr &= ~MII_BMCR_ISOLATE; /* Don't isolate the PHY */
	ret = ksz9131_write(dev, MII_BMCR, bmcr);
	if (ret < 0) {
		return ret;
	}

	/* Wait for the auto-negotiation process to complete */
	do {
		if (timeout-- == 0U) {
			LOG_DBG("PHY (%d) auto-negotiate timedout", cfg->phy_addr);
			ret = -ETIMEDOUT;
			break;
		}

		k_sleep(K_MSEC(100));

		ret = ksz9131_read(dev, MII_BMSR, &bmsr);
		if (ret < 0) {
			break;
		}

		if (bmsr & MII_BMSR_AUTONEG_COMPLETE) {
			LOG_DBG("PHY (%d) auto-negotiate sequence completed", cfg->phy_addr);
			break;
		}
	} while (!(bmsr & MII_BMSR_AUTONEG_COMPLETE));

	return ret;
}

static int phy_mchp_ksz9131_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds,
				     enum phy_cfg_link_flag flags)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct mchp_ksz9131_config *const cfg = dev->config;
#endif
	struct mchp_ksz9131_data *const data = dev->data;
	uint16_t c1kt;
	int ret;

	if (flags & PHY_FLAG_AUTO_NEGOTIATION_DISABLED) {
		LOG_ERR("Disabling auto-negotiation is not supported by this driver");
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* We are going to reconfigure the phy, don't need to monitor until done */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!cfg->interrupt_gpio.port) {
		k_work_cancel_delayable(&data->monitor_work);
	}
#else
	k_work_cancel_delayable(&data->monitor_work);
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	ret = phy_mchp_ksz9131_update_anar(dev, adv_speeds);
	if (ret < 0) {
		goto done;
	}

	if (data->gigabit_supported) {
		ret = ksz9131_read(dev, MII_1KTCR, &c1kt);
		if (ret < 0) {
			goto done;
		}

		if (adv_speeds & LINK_FULL_1000BASE) {
			c1kt |= MII_ADVERTISE_1000_FULL;
		} else {
			c1kt &= ~MII_ADVERTISE_1000_FULL;
		}

		if (adv_speeds & LINK_HALF_1000BASE) {
			c1kt |= MII_ADVERTISE_1000_HALF;
		} else {
			c1kt &= ~MII_ADVERTISE_1000_HALF;
		}

		ret = ksz9131_write(dev, MII_1KTCR, c1kt);
		if (ret < 0) {
			goto done;
		}
	}

	ret = phy_mchp_ksz9131_autonegotiate(dev);
done:
	k_sem_give(&data->sem);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (cfg->interrupt_gpio.port) {
		return ret;
	}
#endif

	/* Start monitoring */
	k_work_reschedule(&data->monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	return ret;
}

static bool phy_mchp_ksz9131_gigabit(const struct device *dev, enum phy_link_speed *speed, int *ret)
{
	uint16_t mutual_capabilities = 0;
	uint16_t mscr = 0;
	uint16_t mssr = 0;

	/* Read AUTO-NEGOTIATION MASTER SLAVE CONTROL REGISTER */
	*ret = ksz9131_read(dev, MII_1KTCR, &mscr);
	if (*ret < 0) {
		return true;
	}

	/* Read AUTO-NEGOTIATION MASTER SLAVE STATUS REGISTER */
	*ret = ksz9131_read(dev, MII_1KSTSR, &mssr);
	if (*ret < 0) {
		return true;
	}

	mutual_capabilities = mscr & (mssr >> 2);
	if (mutual_capabilities & MII_ADVERTISE_1000_FULL) {
		*speed = LINK_FULL_1000BASE;
		return true;
	} else if (mutual_capabilities & MII_ADVERTISE_1000_HALF) {
		*speed = LINK_HALF_1000BASE;
		return true;
	}

	return false;
}

static int phy_mchp_ksz9131_get_link(const struct device *dev, struct phy_link_state *state)
{
	const struct mchp_ksz9131_config *config = dev->config;
	struct mchp_ksz9131_data *const data = dev->data;
	struct phy_link_state old_state = data->state;
	uint16_t mutual_capabilities = 0;
	uint16_t anlpar = 0;
	uint16_t anar = 0;
	int ret = 0;

	(void)config; /* avoid warnings due to config is only used in LOG_DBG() */

	k_sem_take(&data->sem, K_FOREVER);

	ret = phy_mchp_ksz9131_link_status(dev, &state->is_up);
	if (ret < 0 || !state->is_up) {
		goto done;
	}

	if (data->gigabit_supported && phy_mchp_ksz9131_gigabit(dev, &state->speed, &ret)) {
		goto done;
	}

	/* Read currently configured advertising options */
	ret = ksz9131_read(dev, MII_ANAR, &anar);
	if (ret < 0) {
		goto done;
	}

	/* Read link partner capability */
	ret = ksz9131_read(dev, MII_ANLPAR, &anlpar);
	if (ret < 0) {
		goto done;
	}

	mutual_capabilities = anar & anlpar;
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
done:

	if (old_state.speed != state->speed || old_state.is_up != state->is_up) {
		LOG_DBG("PHY %d is %s", config->phy_addr, state->is_up ? "up" : "down");
		if (state->is_up) {
			LOG_DBG("PHY (%d) Link speed %s Mb, %s duplex\n", config->phy_addr,
				(PHY_LINK_IS_SPEED_1000M(state->speed)
					? "1000"
					: (PHY_LINK_IS_SPEED_100M(state->speed) ? "100" : "10")),
				PHY_LINK_IS_FULL_DUPLEX(state->speed) ? "full" : "half");
		}
	}

	k_sem_give(&data->sem);

	return ret;
}

static int phy_mchp_ksz9131_link_cb_set(const struct device *dev, phy_callback_t cb,
					void *user_data)
{
	struct mchp_ksz9131_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	phy_mchp_ksz9131_get_link(dev, &data->state);
	if (data->cb) {
		data->cb(dev, &data->state, data->cb_data);
	}

	return 0;
}

static void phy_mchp_ksz9131_monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mchp_ksz9131_data *const data =
		CONTAINER_OF(dwork, struct mchp_ksz9131_data, monitor_work);
	const struct device *dev = data->dev;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct mchp_ksz9131_config *cfg = dev->config;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */
	struct phy_link_state state = {};
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (cfg->interrupt_gpio.port) {
		ret = phy_mchp_ksz9131_clear_interrupt(data);
		if (ret < 0) {
			return;
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	ret = phy_mchp_ksz9131_get_link(dev, &state);
	if (ret == 0 && (state.speed != data->state.speed || state.is_up != data->state.is_up)) {
		memcpy(&data->state, &state, sizeof(struct phy_link_state));
		if (data->cb) {
			data->cb(dev, &data->state, data->cb_data);
		}
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (cfg->interrupt_gpio.port) {
		return;
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	/* Submit delayed work */
	k_work_reschedule(&data->monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_mchp_ksz9131_init(const struct device *dev)
{
	const struct mchp_ksz9131_config *const cfg = dev->config;
	struct mchp_ksz9131_data *const data = dev->data;
	int ret;

	k_sem_init(&data->sem, 1, 1);

	data->dev = dev;
	data->cb = NULL;

	mdio_bus_enable(cfg->mdio);

	ret = phy_mchp_ksz9131_reset(dev);
	if (ret < 0) {
		return ret;
	}

	ret = phy_check_ksz9131_id(dev);
	if (ret < 0) {
		return ret;
	}

	ret = read_gigabit_supported_flag(dev, &data->gigabit_supported);
	if (ret < 0) {
		LOG_ERR("Failed to read PHY capabilities: %d", ret);
		return ret;
	}

	k_work_init_delayable(&data->monitor_work, phy_mchp_ksz9131_monitor_work_handler);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (!cfg->interrupt_gpio.port) {
		phy_mchp_ksz9131_monitor_work_handler(&data->monitor_work.work);
		goto done;
	}

	/* Configure interrupt pin */
	ret = gpio_pin_configure_dt(&cfg->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		goto done;
	}

	gpio_init_callback(&data->gpio_callback, phy_mchp_ksz9131_interrupt_handler,
			   BIT(cfg->interrupt_gpio.pin));

	ret = gpio_add_callback_dt(&cfg->interrupt_gpio, &data->gpio_callback);
	if (ret < 0) {
		goto done;
	}

	ret = phy_mchp_ksz9131_config_interrupt(dev);
	if (ret < 0) {
		goto done;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
done:
#else
	phy_mchp_ksz9131_monitor_work_handler(&data->monitor_work.work);
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios) */

	if (ret < 0) {
		LOG_ERR("PHY (%d) init failed", cfg->phy_addr);
	}

	return ret;
}

static DEVICE_API(ethphy, mchp_ksz9131_phy_api) = {
	.get_link = phy_mchp_ksz9131_get_link,
	.cfg_link = phy_mchp_ksz9131_cfg_link,
	.link_cb_set = phy_mchp_ksz9131_link_cb_set,
	.read = phy_mchp_ksz9131_read,
	.write = phy_mchp_ksz9131_write,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
#define INTERRUPT_GPIO(n) .interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),
#else
#define INTERRUPT_GPIO(n)
#endif /* interrupt gpio */

#define MICROCHIP_KSZ9131_INIT(n)						\
	static const struct mchp_ksz9131_config mchp_ksz9131_##n##_config = {	\
		.phy_addr = DT_INST_REG_ADDR(n),				\
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n)),				\
		INTERRUPT_GPIO(n)						\
	};									\
										\
	static struct mchp_ksz9131_data mchp_ksz9131_##n##_data;		\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      &phy_mchp_ksz9131_init,				\
			      NULL,						\
			      &mchp_ksz9131_##n##_data,				\
			      &mchp_ksz9131_##n##_config,			\
			      POST_KERNEL,					\
			      CONFIG_PHY_INIT_PRIORITY,				\
			      &mchp_ksz9131_phy_api);

DT_INST_FOREACH_STATUS_OKAY(MICROCHIP_KSZ9131_INIT)
