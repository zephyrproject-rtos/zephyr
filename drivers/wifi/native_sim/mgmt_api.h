/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/wifi_mgmt.h>

int wifi_scan(const struct device *dev, struct wifi_scan_params *params,
	      scan_result_cb_t cb);
int wifi_connect(const struct device *dev, struct wifi_connect_req_params *params);
int wifi_disconnect(const struct device *dev);
#ifdef CONFIG_NET_STATISTICS_WIFI
int wifi_get_stats(const struct device *dev, struct net_stats_wifi *stats);
#endif
int wifi_set_power_save(const struct device *dev,
			struct wifi_ps_params *params);
int wifi_set_twt(const struct device *dev, struct wifi_twt_params *twt_params);
int wifi_reg_domain(const struct device *dev,
		    struct wifi_reg_domain *reg_domain);
int wifi_get_power_save_config(const struct device *dev,
			       struct wifi_ps_config *ps_config);
int wifi_iface_status(const struct device *dev, struct wifi_iface_status *status);
int wifi_filter(const struct device *dev, struct wifi_filter_info *filter);
int wifi_mode(const struct device *dev, struct wifi_mode_info *mode);
int wifi_channel(const struct device *dev, struct wifi_channel_info *channel);
