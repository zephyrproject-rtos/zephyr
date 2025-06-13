/*
 * Copyright 2023 NXP
 * Copyright 2023 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tja11xx

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
LOG_MODULE_REGISTER(phy_tja11xx, CONFIG_PHY_LOG_LEVEL);

/* Extended control register */
#define TJA11XX_EXTENDED_CONTROL           0x0017U
/* Configuration register 1 */
#define TJA11XX_CONFIGURATION_1            0x0018U

struct phy_tja11xx_config {
	const struct device *mdio;
	uint8_t phy_addr;
};

struct phy_tja11xx_data {
	const struct device *dev;
	struct phy_link_state state;
	struct k_sem sem;
	phy_callback_t cb;
	void *cb_data;

	struct k_work_delayable monitor_work;
};

static inline int phy_tja11xx_c22_read(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct phy_tja11xx_config *const cfg = dev->config;

	return mdio_read(cfg->mdio, cfg->phy_addr, reg, val);
}

static inline int phy_tja11xx_c22_write(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct phy_tja11xx_config *const cfg = dev->config;

	return mdio_write(cfg->mdio, cfg->phy_addr, reg, val);
}

static int phy_tja11xx_reg_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	const struct phy_tja11xx_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = phy_tja11xx_c22_read(dev, reg_addr, (uint16_t *)data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int phy_tja11xx_reg_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	const struct phy_tja11xx_config *cfg = dev->config;
	int ret;

	mdio_bus_enable(cfg->mdio);

	ret = phy_tja11xx_c22_write(dev, reg_addr, (uint16_t)data);

	mdio_bus_disable(cfg->mdio);

	return ret;
}

static int update_link_state(const struct device *dev)
{
	struct phy_tja11xx_data *const data = dev->data;
	bool link_up;
	uint16_t val;

	if (phy_tja11xx_c22_read(dev, MII_BMSR, &val) < 0) {
		return -EIO;
	}

	link_up = (val & MII_BMSR_LINK_STATUS) != 0;

	/* Let workqueue re-schedule and re-check if the
	 * link status is unchanged this time
	 */
	if (data->state.is_up == link_up) {
		return -EAGAIN;
	}

	data->state.is_up = link_up;

	return 0;
}

static int phy_tja11xx_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct phy_tja11xx_data *const data = dev->data;
	int rc = 0;

	k_sem_take(&data->sem, K_FOREVER);

	memcpy(state, &data->state, sizeof(struct phy_link_state));

	k_sem_give(&data->sem);

	return rc;
}

static void invoke_link_cb(const struct device *dev)
{
	struct phy_tja11xx_data *const data = dev->data;
	struct phy_link_state state;

	if (data->cb == NULL) {
		return;
	}

	/* Send callback only on link state change */
	if (phy_tja11xx_get_link_state(dev, &state) != 0) {
		return;
	}

	data->cb(dev, &state, data->cb_data);
}

static void monitor_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_tja11xx_data *const data =
		CONTAINER_OF(dwork, struct phy_tja11xx_data, monitor_work);
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

static void phy_tja11xx_cfg_irq_poll(const struct device *dev)
{
	struct phy_tja11xx_data *const data = dev->data;

	k_work_init_delayable(&data->monitor_work, monitor_work_handler);

	monitor_work_handler(&data->monitor_work.work);
}

static int phy_tja11xx_init(const struct device *dev)
{
	struct phy_tja11xx_data *const data = dev->data;
	int ret;

	data->dev = dev;
	data->cb = NULL;
	data->state.is_up = false;
	data->state.speed = LINK_FULL_100BASE;

	ret = phy_tja11xx_reg_write(dev, TJA11XX_EXTENDED_CONTROL, 0x1804);
	if (ret < 0) {
		return ret;
	}

	ret = phy_tja11xx_reg_write(dev, MII_BMCR, 0x2100);
	if (ret < 0) {
		return ret;
	}

	ret = phy_tja11xx_reg_write(dev, TJA11XX_CONFIGURATION_1, 0x8A00);
	if (ret < 0) {
		return ret;
	}

	ret = phy_tja11xx_reg_write(dev, TJA11XX_EXTENDED_CONTROL, 0x9804);
	if (ret < 0) {
		return ret;
	}
	phy_tja11xx_cfg_irq_poll(dev);

	return ret;
}

static int phy_tja11xx_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct phy_tja11xx_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;

	/* Invoke the callback to notify the caller of the current
	 * link status.
	 */
	invoke_link_cb(dev);

	return 0;
}

static const struct ethphy_driver_api phy_tja11xx_api = {
	.get_link = phy_tja11xx_get_link_state,
	.link_cb_set = phy_tja11xx_link_cb_set,
	.read = phy_tja11xx_reg_read,
	.write = phy_tja11xx_reg_write,
};

#define TJA11xx_INITIALIZE(n)                                                                      \
	static const struct phy_tja11xx_config phy_tja11xx_config_##n = {                          \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n))                                              \
	};                                                                                         \
	static struct phy_tja11xx_data phy_tja11xx_data_##n = {                                    \
		.sem = Z_SEM_INITIALIZER(phy_tja11xx_data_##n.sem, 1, 1),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &phy_tja11xx_init, NULL, &phy_tja11xx_data_##n,                   \
				&phy_tja11xx_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,    \
				&phy_tja11xx_api);

DT_INST_FOREACH_STATUS_OKAY(TJA11xx_INITIALIZE)
