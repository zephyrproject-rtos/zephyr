/*
 * Copyright 2023 NXP
 * Copyright 2023 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tja1103

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/net/mdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_tja1103, CONFIG_PHY_LOG_LEVEL);

/* PHYs out of reset check retry delay */
#define TJA1103_AWAIT_DELAY_POLL_US 15000U
/* Number of retries for PHYs out of reset check */
#define TJA1103_AWAIT_RETRY_COUNT   200U

/* TJA1103 PHY identifier */
#define TJA1103_ID 0x1BB013

/* MMD30 - Device status register */
#define TJA1103_DEVICE_CONTROL               (0x0040U)
#define TJA1103_DEVICE_CONTROL_GLOBAL_CFG_EN BIT(14)
#define TJA1103_DEVICE_CONTROL_SUPER_CFG_EN  BIT(13)
/* Shared - PHY control register */
#define TJA1103_PHY_CONTROL                  (0x8100U)
#define TJA1103_PHY_CONTROL_CFG_EN           BIT(14)
/* Shared - PHY status register */
#define TJA1103_PHY_STATUS                   (0x8102U)
#define TJA1103_PHY_STATUS_LINK_STAT         BIT(2)

/* Shared - PHY functional IRQ masked status register */
#define TJA1103_PHY_FUNC_IRQ_MSTATUS            (0x80A2)
#define TJA1103_PHY_FUNC_IRQ_LINK_EVENT         BIT(1)
#define TJA1103_PHY_FUNC_IRQ_LINK_AVAIL         BIT(2)
/* Shared -PHY functional IRQ source & enable registers */
#define TJA1103_PHY_FUNC_IRQ_ACK                (0x80A0)
#define TJA1103_PHY_FUNC_IRQ_EN                 (0x80A1)
#define TJA1103_PHY_FUNC_IRQ_LINK_EVENT_EN      BIT(1)
#define TJA1103_PHY_FUNC_IRQ_LINK_AVAIL_EN      BIT(2)
/* Always accessible reg for NMIs */
#define TJA1103_ALWAYS_ACCESSIBLE               (0x801F)
#define TJA1103_ALWAYS_ACCESSIBLE_FUSA_PASS_IRQ BIT(4)

#define MASTER_SLAVE_DEFAULT 0
#define MASTER_SLAVE_MASTER  1
#define MASTER_SLAVE_SLAVE   2
#define MASTER_SLAVE_AUTO    3

#define SWITCH_WAIT_RANGE(min, max) ((min) + (sys_rand16_get() % ((max) - (min))))

struct phy_tja1103_config {
	const struct device *mdio;
	struct gpio_dt_spec gpio_interrupt;
	uint8_t phy_addr;
	uint8_t master_slave;
};

struct phy_tja1103_data {
	const struct device *dev;
	struct phy_link_state state;
	struct k_sem sem;
	phy_callback_t cb;
	struct gpio_callback phy_tja1103_int_callback;
	void *cb_data;
	struct k_work_delayable phy_work;
	k_timepoint_t timeout;
};

static inline int phy_tja1103_c22_read(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct phy_tja1103_config *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg, val);
}

static inline int phy_tja1103_c22_write(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct phy_tja1103_config *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg, val);
}

static inline int phy_tja1103_c45_write(const struct device *dev, uint16_t devad, uint16_t reg,
					uint16_t val)
{
	const struct phy_tja1103_config *cfg = dev->config;

	return mdio_write_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
}

static inline int phy_tja1103_c45_read(const struct device *dev, uint16_t devad, uint16_t reg,
				       uint16_t *val)
{
	const struct phy_tja1103_config *cfg = dev->config;

	return mdio_read_c45(cfg->mdio, cfg->phy_addr, devad, reg, val);
}

static int phy_tja1103_reg_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct phy_tja1103_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = phy_tja1103_c22_read(dev, reg_addr, (uint16_t *)data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_tja1103_reg_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct phy_tja1103_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = phy_tja1103_c22_write(dev, reg_addr, (uint16_t)data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_tja1103_id(const struct device *dev, uint32_t *phy_id)
{
	uint16_t val;

	if (phy_tja1103_c22_read(dev, MII_PHYID1R, &val) < 0) {
		return -EIO;
	}

	*phy_id = (val & UINT16_MAX) << 16;

	if (phy_tja1103_c22_read(dev, MII_PHYID2R, &val) < 0) {
		return -EIO;
	}

	*phy_id |= (val & UINT16_MAX);

	return 0;
}

static int phy_tja1103_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct phy_tja1103_data *const data = dev->data;

	state->speed = data->state.speed;
	state->is_up = data->state.is_up;

	return 0;
}

static int phy_tja1103_update_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct phy_tja1103_data *const data = dev->data;
	uint16_t val;
	int rc = 0;

	if (phy_tja1103_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_STATUS, &val) < 0) {
		return -EIO;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* TJA1103 Only supports 100BASE Full-duplex */
	state->speed = LINK_FULL_100BASE;
	state->is_up = (val & TJA1103_PHY_STATUS_LINK_STAT) != 0;

	k_sem_give(&data->sem);

	LOG_DBG("TJA1103 Link state %i", state->is_up);

	return rc;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void phy_tja1103_ack_irq(const struct device *dev)
{
	uint16_t irq;

	if (phy_tja1103_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_FUNC_IRQ_MSTATUS,
				 &irq) < 0) {
		return;
	}

	/* Handling Link related Functional IRQs */
	if (irq & (TJA1103_PHY_FUNC_IRQ_LINK_EVENT | TJA1103_PHY_FUNC_IRQ_LINK_AVAIL)) {
		/* Ack the assered link related interrupts */
		phy_tja1103_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_FUNC_IRQ_ACK,
				      irq);
	}
}

static void phy_tja1103_handle_irq(const struct device *port, struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(pins);
	ARG_UNUSED(port);

	struct phy_tja1103_data *const data =
		CONTAINER_OF(cb, struct phy_tja1103_data, phy_tja1103_int_callback);

	/* Trigger workqueue before leaving the ISR */
	k_work_reschedule(&data->phy_work, K_NO_WAIT);
}
#endif

static void phy_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_tja1103_data *const data =
		CONTAINER_OF(dwork, struct phy_tja1103_data, phy_work);
	const struct device *dev = data->dev;
	struct phy_link_state state = {};
	int rc;
	const struct phy_tja1103_config *const cfg = dev->config;

	rc = phy_tja1103_update_link_state(dev, &state);

	/* Update link state and trigger callback if changed */
	if (rc == 0 && state.is_up != data->state.is_up) {
		memcpy(&data->state, &state, sizeof(struct phy_link_state));
		if (data->cb) {
			data->cb(dev, &data->state, data->cb_data);
		}
	}

	if (cfg->master_slave == MASTER_SLAVE_AUTO && !data->state.is_up &&
	    sys_timepoint_expired(data->timeout)) {
		uint16_t val;

		data->timeout = sys_timepoint_calc(K_MSEC(SWITCH_WAIT_RANGE(1000, 3000)));

		phy_tja1103_c45_read(dev, MDIO_MMD_PMAPMD, MDIO_PMA_PMD_BT1_CTRL, &val);

		val ^= MDIO_PMA_PMD_BT1_CTRL_CFG_MST;

		phy_tja1103_c45_write(dev, MDIO_MMD_PMAPMD, MDIO_PMA_PMD_BT1_CTRL, val);
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (cfg->gpio_interrupt.port) {
		phy_tja1103_ack_irq(dev);

		if (cfg->master_slave == MASTER_SLAVE_AUTO && !data->state.is_up) {
			k_work_reschedule(&data->phy_work, sys_timepoint_timeout(data->timeout));
		}

		return;
	}
#endif

	/* Submit delayed work */
	k_work_reschedule(&data->phy_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static void phy_tja1103_cfg_irq_poll(const struct device *dev)
{
	struct phy_tja1103_data *const data = dev->data;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	int ret;
	const struct phy_tja1103_config *const cfg = dev->config;

	if (cfg->gpio_interrupt.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->gpio_interrupt)) {
			LOG_ERR("Interrupt GPIO device %s is not ready",
				cfg->gpio_interrupt.port->name);
			return;
		}

		ret = gpio_pin_configure_dt(&cfg->gpio_interrupt, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure interrupt GPIO, %d", ret);
			return;
		}

		gpio_init_callback(&(data->phy_tja1103_int_callback), phy_tja1103_handle_irq,
				   BIT(cfg->gpio_interrupt.pin));

		/* Add callback structure to global syslist */
		ret = gpio_add_callback(cfg->gpio_interrupt.port, &data->phy_tja1103_int_callback);
		if (ret < 0) {
			LOG_ERR("Failed to add INT callback, %d", ret);
			return;
		}

		ret = phy_tja1103_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_FUNC_IRQ_EN,
					    (TJA1103_PHY_FUNC_IRQ_LINK_EVENT_EN));
		if (ret < 0) {
			return;
		}

		ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_interrupt, GPIO_INT_EDGE_FALLING);
		if (ret < 0) {
			LOG_ERR("Failed to enable INT, %d", ret);
			return;
		}
	}
#endif

	data->timeout = sys_timepoint_calc(K_MSEC(SWITCH_WAIT_RANGE(1000, 3000)));

	k_work_init_delayable(&data->phy_work, phy_work_handler);

	phy_work_handler(&data->phy_work.work);
}

static int phy_tja1103_init(const struct device *dev)
{
	const struct phy_tja1103_config *const cfg = dev->config;
	struct phy_tja1103_data *const data = dev->data;
	uint32_t phy_id = 0;
	uint16_t val;
	int ret;

	data->dev = dev;
	data->cb = NULL;
	data->state.is_up = false;
	data->state.speed = LINK_FULL_100BASE;

	ret = WAIT_FOR(!phy_tja1103_id(dev, &phy_id) && phy_id == TJA1103_ID,
		       TJA1103_AWAIT_RETRY_COUNT * TJA1103_AWAIT_DELAY_POLL_US,
		       k_sleep(K_USEC(TJA1103_AWAIT_DELAY_POLL_US)));
	if (ret == 0) {
		LOG_ERR("Unable to obtain PHY ID for device 0x%x", cfg->phy_addr);
		return -ENODEV;
	}

	/* enable config registers */
	ret = phy_tja1103_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_DEVICE_CONTROL,
				    TJA1103_DEVICE_CONTROL_GLOBAL_CFG_EN |
					    TJA1103_DEVICE_CONTROL_SUPER_CFG_EN);
	if (ret < 0) {
		return ret;
	}

	ret = phy_tja1103_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_CONTROL,
				    TJA1103_PHY_CONTROL_CFG_EN);
	if (ret < 0) {
		return ret;
	}

	ret = phy_tja1103_c45_read(dev, MDIO_MMD_PMAPMD, MDIO_PMA_PMD_BT1_CTRL, &val);
	if (ret < 0) {
		return ret;
	}

	/* Change master/slave mode if need */
	if (cfg->master_slave == MASTER_SLAVE_MASTER) {
		val |= MDIO_PMA_PMD_BT1_CTRL_CFG_MST;
	} else if (cfg->master_slave == MASTER_SLAVE_SLAVE) {
		val &= ~MDIO_PMA_PMD_BT1_CTRL_CFG_MST;
	}

	ret = phy_tja1103_c45_write(dev, MDIO_MMD_PMAPMD, MDIO_PMA_PMD_BT1_CTRL, val);
	if (ret < 0) {
		return ret;
	}

	/* Check always accessible register for handling NMIs */
	ret = phy_tja1103_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_ALWAYS_ACCESSIBLE, &val);
	if (ret < 0) {
		return ret;
	}

	/* Ack Fusa Pass Interrupt if Startup Self Test Passed successfully */
	if (val & TJA1103_ALWAYS_ACCESSIBLE_FUSA_PASS_IRQ) {
		ret = phy_tja1103_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1,
					    TJA1103_ALWAYS_ACCESSIBLE,
					    TJA1103_ALWAYS_ACCESSIBLE_FUSA_PASS_IRQ);
	}

	/* Configure interrupt or poll mode for reporting link changes */
	phy_tja1103_cfg_irq_poll(dev);

	return ret;
}

static int phy_tja1103_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	int rc = 0;
	struct phy_tja1103_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	data->cb(dev, &data->state, data->cb_data);

	return rc;
}

static DEVICE_API(ethphy, phy_tja1103_api) = {
	.get_link = phy_tja1103_get_link_state,
	.link_cb_set = phy_tja1103_link_cb_set,
	.read = phy_tja1103_reg_read,
	.write = phy_tja1103_reg_write,
};

#define TJA1103_INITIALIZE(n)                                                                      \
	static const struct phy_tja1103_config phy_tja1103_config_##n = {                          \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n)),                                             \
		.gpio_interrupt = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),                     \
		.master_slave = DT_INST_ENUM_IDX(n, master_slave),                                 \
	};                                                                                         \
	static struct phy_tja1103_data phy_tja1103_data_##n = {                                    \
		.sem = Z_SEM_INITIALIZER(phy_tja1103_data_##n.sem, 1, 1),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &phy_tja1103_init, NULL, &phy_tja1103_data_##n,                   \
			      &phy_tja1103_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,      \
			      &phy_tja1103_api);

DT_INST_FOREACH_STATUS_OKAY(TJA1103_INITIALIZE)
