/*
 * SPDX-FileCopyrightText: Copyright 2026 DZG Metering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_ksz8863_phy

#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_microchip_ksz8863, CONFIG_PHY_LOG_LEVEL);

#include "../dsa/dsa_ksz8863.h"

struct phy_ksz8863_config {
	uint8_t port;
};

struct phy_ksz8863_data {
	const struct device *dev;
	phy_callback_t cb;
	void *cb_data;
	struct phy_link_state state;
	struct k_work_delayable monitor_work;
};

static int phy_ksz8863_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	const struct phy_ksz8863_config *cfg = dev->config;
	const struct device *switch_dev = DEVICE_DT_GET(DT_INST(0, microchip_ksz8863));
	bool link_up;
	int ret;

	ret = dsa_ksz8863_port_link_status(switch_dev, cfg->port, &link_up);
	if (ret != 0) {
		return ret;
	}

	state->is_up = link_up;
	state->speed = 0;

	return 0;
}

static void phy_ksz8863_invoke_cb(struct phy_ksz8863_data *data)
{
	if (data->cb != NULL) {
		data->cb(data->dev, &data->state, data->cb_data);
	}
}

static void phy_ksz8863_monitor_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct phy_ksz8863_data *data = CONTAINER_OF(dwork, struct phy_ksz8863_data, monitor_work);
	struct phy_link_state state;
	int ret;

	ret = phy_ksz8863_get_link_state(data->dev, &state);
	if (ret == 0 && state.is_up != data->state.is_up) {
		data->state = state;
		phy_ksz8863_invoke_cb(data);
	}

	k_work_reschedule(&data->monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
}

static int phy_ksz8863_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct phy_ksz8863_data *data = dev->data;
	int ret;

	data->cb = cb;
	data->cb_data = user_data;

	if (cb == NULL) {
		k_work_cancel_delayable(&data->monitor_work);
		return 0;
	}

	ret = phy_ksz8863_get_link_state(dev, &data->state);
	if (ret != 0) {
		return ret;
	}

	phy_ksz8863_invoke_cb(data);
	k_work_reschedule(&data->monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

	return 0;
}

static int phy_ksz8863_init(const struct device *dev)
{
	struct phy_ksz8863_data *data = dev->data;

	data->dev = dev;
	data->state.is_up = false;
	data->state.speed = 0;
	k_work_init_delayable(&data->monitor_work, phy_ksz8863_monitor_work);

	return 0;
}

static DEVICE_API(ethphy, phy_ksz8863_driver_api) = {
	.get_link = phy_ksz8863_get_link_state,
	.link_cb_set = phy_ksz8863_link_cb_set,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(microchip_ksz8863) == 1,
	     "KSZ8863 PHY currently supports exactly one KSZ8863 switch instance");

#define PHY_KSZ8863_DEVICE(n)                                                                      \
	static const struct phy_ksz8863_config phy_ksz8863_config_##n = {                          \
		.port = DT_INST_PROP(n, port),                                                     \
	};                                                                                         \
	static struct phy_ksz8863_data phy_ksz8863_data_##n;                                       \
	DEVICE_DT_INST_DEFINE(n, phy_ksz8863_init, NULL, &phy_ksz8863_data_##n,                    \
			      &phy_ksz8863_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,      \
			      &phy_ksz8863_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PHY_KSZ8863_DEVICE)
