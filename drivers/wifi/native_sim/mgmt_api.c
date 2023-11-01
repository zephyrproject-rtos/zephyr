/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi.h>

#include "mgmt_api.h"
#include "internal.h"

LOG_MODULE_DECLARE(wifi_native_sim, CONFIG_WIFI_LOG_LEVEL);

int wifi_scan(const struct device *dev, struct wifi_scan_params *params,
	      scan_result_cb_t cb)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	if (ctx->scan_in_progress) {
		LOG_DBG("Scan already in progress for %s", ctx->name);
		ret = -EBUSY;
		goto out;
	}

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* Do the scan */

	/* TODO: add code here */

	ctx->scan_in_progress = true;

	ret = 0;
out:
	return ret;
}

int wifi_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* Initiate the connect */

	/* TODO: add code here */

	ret = 0;

	return ret;
}

int wifi_disconnect(const struct device *dev)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* Initiate the disconnect */

	/* TODO: add code here */

	ret = 0;

	return ret;
}

int wifi_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* TODO: add code here */

	ret = 0;

	return ret;
}

int wifi_ap_disable(const struct device *dev)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* TODO: add code here */

	ret = 0;

	return ret;
}

int wifi_iface_status(const struct device *dev, struct wifi_iface_status *status)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* TODO: add code here */

	ret = 0;

	return ret;
}

int wifi_filter(const struct device *dev, struct wifi_filter_info *filter)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* TODO: add code here */

	ret = 0;

	return ret;
}

int wifi_mode(const struct device *dev, struct wifi_mode_info *mode)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* TODO: add code here */

	ret = 0;

	return ret;
}

int wifi_channel(const struct device *dev, struct wifi_channel_info *channel)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* TODO: add code here */

	ret = 0;

	return ret;
}

#ifdef CONFIG_NET_STATISTICS_WIFI
int wifi_get_stats(const struct device *dev, struct net_stats_wifi *stats)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* Get the stats from the device */

	/* TODO: add code here, skipping this for now */
	ret = -ENOTSUP;
	goto out;

	stats->pkts.tx = 0;
	stats->pkts.rx = 0;
	stats->errors.tx = 0;
	stats->errors.rx = 0;
	stats->bytes.received = 0;
	stats->bytes.sent = 0;
	stats->sta_mgmt.beacons_rx = 0;
	stats->sta_mgmt.beacons_miss = 0;
	stats->broadcast.rx = 0;
	stats->broadcast.tx = 0;
	stats->multicast.rx = 0;
	stats->multicast.tx = 0;

	ret = 0;
out:
	return ret;
}
#endif /* CONFIG_NET_STATISTICS_WIFI */

int wifi_set_power_save(const struct device *dev,
			struct wifi_ps_params *params)
{
	struct wifi_context *ctx = dev->data;

	ARG_UNUSED(dev);
	ARG_UNUSED(params);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	return 0;
}

int wifi_set_twt(const struct device *dev,
		 struct wifi_twt_params *twt_params)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* Set target wait time */

	/* TODO: add code here, skipping this for now */
	ret = -ENOTSUP;
	goto out;

	ret = 0;
out:
	return ret;
}

int wifi_reg_domain(const struct device *dev,
		    struct wifi_reg_domain *reg_domain)
{
	struct wifi_context *ctx = dev->data;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	/* Manage registration domain */

	/* TODO: add code here, skipping this for now */
	ret = -ENOTSUP;
	goto out;

	ret = 0;
out:
	return ret;
}

int wifi_get_power_save_config(const struct device *dev,
			       struct wifi_ps_config *ps_config)
{
	struct wifi_context *ctx = dev->data;

	ARG_UNUSED(dev);
	ARG_UNUSED(ps_config);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	return 0;
}
