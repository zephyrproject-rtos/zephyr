/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/net_buf.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>

#include <am_rss_mgr.h>

LOG_MODULE_REGISTER(bt_hci_apollox_ipc_support);

int bt_hci_transport_teardown(const struct device *dev)
{
	ARG_UNUSED(dev);

	int ret;

	/* Disable the radio subsystem */
	ret = am_rss_mgr_rss_enable(false);

	return ret;
}

int bt_hci_transport_setup(const struct device *dev)
{
	ARG_UNUSED(dev);

	int ret = 0;

	/* Enable the radio subsystem */
	ret = am_rss_mgr_rss_enable(true);
	if (ret != 0) {
		return ret;
	}

	ret = am_rss_mgr_ipc_shm_config();

	return ret;
}

int bt_ipc_setup(const struct device *dev, const struct bt_hci_setup_params *params)
{
	ARG_UNUSED(params);

	int ret;
	struct net_buf *buf;

	buf = am_rss_mgr_opmode_config(AM_RSS_OPMODE_NP);
	if (!buf) {
		return -ENOBUFS;
	}

	ret = bt_hci_send(dev, buf);

	return ret;
}
