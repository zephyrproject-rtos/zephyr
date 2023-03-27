/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_wifi_mgmt, CONFIG_NET_L2_WIFI_MGMT_LOG_LEVEL);

#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

static int wifi_connect(uint32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;

	if (off_api == NULL || off_api->connect == NULL) {
		return -ENOTSUP;
	}

	LOG_HEXDUMP_DBG(params->ssid, params->ssid_length, "ssid");
	LOG_HEXDUMP_DBG(params->psk, params->psk_length, "psk");
	if (params->sae_password) {
		LOG_HEXDUMP_DBG(params->sae_password, params->sae_password_length, "sae");
	}
	NET_DBG("ch %u sec %u", params->channel, params->security);

	if ((params->security > WIFI_SECURITY_TYPE_MAX) ||
	    (params->ssid_length > WIFI_SSID_MAX_LEN) ||
	    (params->ssid_length == 0U) ||
	    ((params->security == WIFI_SECURITY_TYPE_PSK ||
		  params->security == WIFI_SECURITY_TYPE_PSK_SHA256) &&
	     ((params->psk_length < 8) || (params->psk_length > 64) ||
	      (params->psk_length == 0U) || !params->psk)) ||
	    ((params->security == WIFI_SECURITY_TYPE_SAE) &&
	      ((params->psk_length == 0U) || !params->psk) &&
		  ((params->sae_password_length == 0U) || !params->sae_password)) ||
	    ((params->channel != WIFI_CHANNEL_ANY) &&
	     (params->channel > WIFI_CHANNEL_MAX)) ||
	    !params->ssid) {
		return -EINVAL;
	}

	return off_api->connect(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT, wifi_connect);

static void scan_result_cb(struct net_if *iface, int status,
			    struct wifi_scan_result *entry)
{
	if (!iface) {
		return;
	}

	if (!entry) {
		struct wifi_status scan_status = {
			.status = status,
		};

		net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_DONE,
						iface, &scan_status,
						sizeof(struct wifi_status));
		return;
	}

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_RESULT, iface,
					entry, sizeof(struct wifi_scan_result));
}

static int wifi_scan(uint32_t mgmt_request, struct net_if *iface,
		     void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;

	if (off_api == NULL || off_api->scan == NULL) {
		return -ENOTSUP;
	}

	return off_api->scan(dev, scan_result_cb);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN, wifi_scan);


static int wifi_disconnect(uint32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;

	if (off_api == NULL || off_api->disconnect == NULL) {
		return -ENOTSUP;
	}

	return off_api->disconnect(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT, wifi_disconnect);

void wifi_mgmt_raise_connect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_CONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

void wifi_mgmt_raise_disconnect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_DISCONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

static int wifi_ap_enable(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;

	if (off_api == NULL || off_api->ap_enable == NULL) {
		return -ENOTSUP;
	}

	return off_api->ap_enable(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_ENABLE, wifi_ap_enable);

static int wifi_ap_disable(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;

	if (off_api == NULL || off_api->ap_enable == NULL) {
		return -ENOTSUP;
	}

	return off_api->ap_disable(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_DISABLE, wifi_ap_disable);

static int wifi_iface_status(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct wifi_iface_status *status = data;

	if (off_api == NULL || off_api->iface_status == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*status)) {
		return -EINVAL;
	}

	return off_api->iface_status(dev, status);
}
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_IFACE_STATUS, wifi_iface_status);

void wifi_mgmt_raise_iface_status_event(struct net_if *iface,
		struct wifi_iface_status *iface_status)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_IFACE_STATUS,
					iface, iface_status,
					sizeof(struct wifi_iface_status));
}

#ifdef CONFIG_NET_STATISTICS_WIFI
static int wifi_iface_stats(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct net_stats_wifi *stats = data;

	if (off_api == NULL || off_api->get_stats == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*stats)) {
		return -EINVAL;
	}

	return off_api->get_stats(dev, stats);
}
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_WIFI, wifi_iface_stats);
#endif /* CONFIG_NET_STATISTICS_WIFI */

static int wifi_set_power_save(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct wifi_ps_params *ps_params = data;

	if (off_api == NULL || off_api->set_power_save == NULL) {
		return -ENOTSUP;
	}

	return off_api->set_power_save(dev, ps_params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_PS, wifi_set_power_save);

static int wifi_get_power_save_config(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct wifi_ps_config *ps_config = data;

	if (off_api == NULL || off_api->get_power_save_config == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*ps_config)) {
		return -EINVAL;
	}

	return off_api->get_power_save_config(dev, ps_config);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_PS_CONFIG, wifi_get_power_save_config);

static int wifi_set_power_save_mode(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct wifi_ps_mode_params *ps_mode_params = data;

	if (off_api == NULL || off_api->set_power_save_mode == NULL) {
		return -ENOTSUP;
	}

	return off_api->set_power_save_mode(dev, ps_mode_params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_PS_MODE, wifi_set_power_save_mode);

static int wifi_set_twt(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct wifi_twt_params *twt_params = data;

	if (off_api == NULL || off_api->set_twt == NULL) {
		return -ENOTSUP;
	}

	return off_api->set_twt(dev, twt_params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_TWT, wifi_set_twt);

void wifi_mgmt_raise_twt_event(struct net_if *iface, struct wifi_twt_params *twt_params)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_TWT,
					iface, twt_params,
					sizeof(struct wifi_twt_params));
}

static int wifi_reg_domain(uint32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
			(struct net_wifi_mgmt_offload *) dev->api;
	struct wifi_reg_domain *reg_domain = data;

	if (off_api == NULL || off_api->reg_domain == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*reg_domain)) {
		return -EINVAL;
	}

	return off_api->reg_domain(dev, reg_domain);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_REG_DOMAIN, wifi_reg_domain);

static int wifi_set_power_save_timeout(uint32_t mgmt_request, struct net_if *iface,
				       void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct wifi_ps_timeout_params *ps_timeout = data;

	if (off_api == NULL || off_api->set_power_save_timeout == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*ps_timeout)) {
		return -EINVAL;
	}

	return off_api->set_power_save_timeout(dev, ps_timeout);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_PS_TIMEOUT, wifi_set_power_save_timeout);

void wifi_mgmt_raise_twt_sleep_state(struct net_if *iface,
				     int twt_sleep_state)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_TWT_SLEEP_STATE,
					iface, INT_TO_POINTER(twt_sleep_state),
					sizeof(twt_sleep_state));
}

static int wifi_set_listen_interval(uint32_t mgmt_request, struct net_if *iface,
				    void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
		(struct net_wifi_mgmt_offload *) dev->api;
	struct wifi_listen_interval_params *params = data;

	if (off_api == NULL || off_api->set_listen_interval == NULL) {
		return -ENOTSUP;
	}

	return off_api->set_listen_interval(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_LISTEN_INTERVAL, wifi_set_listen_interval);
