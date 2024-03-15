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
	struct k_sem offload_sem;
	phy_callback_t cb;
	struct gpio_callback phy_tja1103_int_callback;
	void *cb_data;

	K_KERNEL_STACK_MEMBER(irq_thread_stack, CONFIG_PHY_TJA1103_IRQ_THREAD_STACK_SIZE);
	struct k_thread irq_thread;

	struct k_work_delayable monitor_work;
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

static int update_link_state(const struct device *dev)
{
	struct phy_tja1103_data *const data = dev->data;
	bool link_up;
	uint16_t val;

	if (phy_tja1103_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_STATUS, &val) < 0) {
		return -EIO;
	}

	link_up = (val & TJA1103_PHY_STATUS_LINK_STAT) != 0;

	/* Let workqueue re-schedule and re-check if the
	 * link status is unchanged this time
	 */
	if (data->state.is_up == link_up) {
		return -EAGAIN;
	}

	data->state.is_up = link_up;

	return 0;
}

static int phy_tja1103_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct phy_tja1103_data *const data = dev->data;
	const struct phy_tja1103_config *const cfg = dev->config;
	int rc = 0;

	k_sem_take(&data->sem, K_FOREVER);

	/* If Interrupt is configured then the workqueue will not
	 * update the link state periodically so do it explicitly
	 */
	if (cfg->gpio_interrupt.port != NULL) {
		rc = update_link_state(dev);
	}

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	k_sem_give(&data->sem);

	return rc;
}

static void invoke_link_cb(const struct device *dev)
{
	struct phy_tja1103_data *const data = dev->data;
	struct phy_link_state state;

	if (data->cb == NULL) {
		return;
	}

	/* Send callback only on link state change */
	if (phy_tja1103_get_link_state(dev, &state) != 0) {
		return;
	}

	data->cb(dev, &state, data->cb_data);
}

static void monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_tja1103_data *const data =
		CONTAINER_OF(dwork, struct phy_tja1103_data, monitor_work);
	const struct device *dev = data->dev;
	int rc;

	k_sem_take(&data->sem, K_FOREVER);

	rc = update_link_state(dev);

	k_sem_give(&data->sem);

	/* If link state has changed and a callback is set, invoke callback */
	if (rc == 0) {
		invoke_link_cb(dev);
	}

	/* Submit delayed work */
	k_work_reschedule(&data->monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static void phy_tja1103_irq_offload_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct phy_tja1103_data *const data = dev->data;
	uint16_t irq;

	for (;;) {
		/* await trigger from ISR */
		k_sem_take(&data->offload_sem, K_FOREVER);

		if (phy_tja1103_c45_read(dev, MDIO_MMD_VENDOR_SPECIFIC1,
					 TJA1103_PHY_FUNC_IRQ_MSTATUS, &irq) < 0) {
			return;
		}

		/* Handling Link related Functional IRQs */
		if (irq & (TJA1103_PHY_FUNC_IRQ_LINK_EVENT | TJA1103_PHY_FUNC_IRQ_LINK_AVAIL)) {
			/* Send callback to MAC on link status changed */
			invoke_link_cb(dev);

			/* Ack the assered link related interrupts */
			phy_tja1103_c45_write(dev, MDIO_MMD_VENDOR_SPECIFIC1,
					      TJA1103_PHY_FUNC_IRQ_ACK, irq);
		}
	}
}

static void phy_tja1103_handle_irq(const struct device *port, struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(pins);
	ARG_UNUSED(port);

	struct phy_tja1103_data *const data =
		CONTAINER_OF(cb, struct phy_tja1103_data, phy_tja1103_int_callback);

	/* Trigger BH before leaving the ISR */
	k_sem_give(&data->offload_sem);
}

static void phy_tja1103_cfg_irq_poll(const struct device *dev)
{
	struct phy_tja1103_data *const data = dev->data;
	const struct phy_tja1103_config *const cfg = dev->config;
	int ret;

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

		ret = phy_tja1103_c45_write(
			dev, MDIO_MMD_VENDOR_SPECIFIC1, TJA1103_PHY_FUNC_IRQ_EN,
			(TJA1103_PHY_FUNC_IRQ_LINK_EVENT_EN | TJA1103_PHY_FUNC_IRQ_LINK_AVAIL_EN));
		if (ret < 0) {
			return;
		}

		ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_interrupt, GPIO_INT_EDGE_FALLING);
		if (ret < 0) {
			LOG_ERR("Failed to enable INT, %d", ret);
			return;
		}

		/* PHY initialized, IRQ configured, now initialize the BH handler */
		k_thread_create(&data->irq_thread, data->irq_thread_stack,
				CONFIG_PHY_TJA1103_IRQ_THREAD_STACK_SIZE,
				phy_tja1103_irq_offload_thread, (void *)dev, NULL, NULL,
				CONFIG_PHY_TJA1103_IRQ_THREAD_PRIO, K_ESSENTIAL, K_NO_WAIT);
		k_thread_name_set(&data->irq_thread, "phy_tja1103_irq_offload");

	} else {
		k_work_init_delayable(&data->monitor_work, monitor_work_handler);

		monitor_work_handler(&data->monitor_work.work);
	}
}

static int phy_tja1103_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds)
{
	ARG_UNUSED(dev);

	if (adv_speeds & LINK_FULL_100BASE_T) {
		return 0;
	}

	return -ENOTSUP;
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
	data->state.speed = LINK_FULL_100BASE_T;

	ret = WAIT_FOR(!phy_tja1103_id(dev, &phy_id) && phy_id == TJA1103_ID,
		       TJA1103_AWAIT_RETRY_COUNT * TJA1103_AWAIT_DELAY_POLL_US,
		       k_sleep(K_USEC(TJA1103_AWAIT_DELAY_POLL_US)));
	if (ret < 0) {
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
	if (cfg->master_slave == 1) {
		val |= MDIO_PMA_PMD_BT1_CTRL_CFG_MST;
	} else if (cfg->master_slave == 2) {
		val &= ~MDIO_PMA_PMD_BT1_CTRL_CFG_MST;
	}

	ret = phy_tja1103_c45_write(dev, MDIO_MMD_PMAPMD, MDIO_PMA_PMD_BT1_CTRL, val);
	if (ret < 0) {
		return ret;
	}

	/* Check always accesible register for handling NMIs */
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
	struct phy_tja1103_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	/* Invoke the callback to notify the caller of the current
	 * link status.
	 */
	invoke_link_cb(dev);

	return 0;
}

static const struct ethphy_driver_api phy_tja1103_api = {
	.get_link = phy_tja1103_get_link_state,
	.cfg_link = phy_tja1103_cfg_link,
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
		.offload_sem = Z_SEM_INITIALIZER(phy_tja1103_data_##n.offload_sem, 0, 1),          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &phy_tja1103_init, NULL, &phy_tja1103_data_##n,                   \
			      &phy_tja1103_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,      \
			      &phy_tja1103_api);

DT_INST_FOREACH_STATUS_OKAY(TJA1103_INITIALIZE)
