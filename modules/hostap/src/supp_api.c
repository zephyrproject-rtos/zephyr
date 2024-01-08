/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdarg.h>

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>

#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"

#include "supp_main.h"
#include "supp_api.h"
#include "wpa_cli_zephyr.h"
#include "supp_events.h"

extern struct k_sem wpa_supplicant_ready_sem;
extern struct wpa_global *global;

enum requested_ops {
	CONNECT = 0,
	DISCONNECT
};

enum status_thread_state {
	STATUS_THREAD_STOPPED = 0,
	STATUS_THREAD_RUNNING,
};

#define OP_STATUS_POLLING_INTERVAL 1

#define CONNECTION_SUCCESS 0
#define CONNECTION_FAILURE 1
#define CONNECTION_TERMINATED 2

#define DISCONNECT_TIMEOUT_MS 5000

K_MUTEX_DEFINE(wpa_supplicant_mutex);

extern struct k_work_q *get_workq(void);

struct wpa_supp_api_ctrl {
	const struct device *dev;
	enum requested_ops requested_op;
	enum status_thread_state status_thread_state;
	int connection_timeout; /* in seconds */
	struct k_work_sync sync;
	bool terminate;
};

static struct wpa_supp_api_ctrl wpas_api_ctrl;

static void supp_shell_connect_status(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(wpa_supp_status_work,
		supp_shell_connect_status);

#define wpa_cli_cmd_v(cmd, ...)	({					\
	bool status;							\
									\
	if (zephyr_wpa_cli_cmd_v(cmd, ##__VA_ARGS__) < 0) {		\
		wpa_printf(MSG_ERROR,					\
			   "Failed to execute wpa_cli command: %s",	\
			   cmd);					\
		status = false;						\
	} else {							\
		status = true;						\
	}								\
									\
	status;								\
})

static struct wpa_supplicant *get_wpa_s_handle(const struct device *dev)
{
	struct net_if *iface = net_if_lookup_by_dev(dev);
	char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	struct wpa_supplicant *wpa_s;
	int ret;

	if (!iface) {
		wpa_printf(MSG_ERROR, "Interface for device %s not found", dev->name);
		return NULL;
	}

	ret = net_if_get_name(iface, if_name, sizeof(if_name));
	if (!ret) {
		wpa_printf(MSG_ERROR, "Cannot get interface name (%d)", ret);
		return NULL;
	}

	wpa_s = zephyr_get_handle_by_ifname(if_name);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR, "Interface %s not found", if_name);
		return NULL;
	}

	return wpa_s;
}

static int wait_for_disconnect_complete(const struct device *dev)
{
	int ret = 0;
	int timeout = 0;
	struct wpa_supplicant *wpa_s = get_wpa_s_handle(dev);

	if (!wpa_s) {
		ret = -ENODEV;
		wpa_printf(MSG_ERROR, "Failed to get wpa_s handle");
		goto out;
	}

	while (wpa_s->wpa_state != WPA_DISCONNECTED) {
		if (timeout > DISCONNECT_TIMEOUT_MS) {
			ret = -ETIMEDOUT;
			wpa_printf(MSG_WARNING, "Failed to disconnect from network");
			break;
		}

		k_sleep(K_MSEC(10));
		timeout++;
	}
out:
	return ret;
}

static void supp_shell_connect_status(struct k_work *work)
{
	static int seconds_counter;
	int status = CONNECTION_SUCCESS;
	int conn_result = CONNECTION_FAILURE;
	struct wpa_supplicant *wpa_s;
	struct wpa_supp_api_ctrl *ctrl = &wpas_api_ctrl;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	if (ctrl->status_thread_state == STATUS_THREAD_RUNNING &&  ctrl->terminate) {
		status = CONNECTION_TERMINATED;
		goto out;
	}

	wpa_s = get_wpa_s_handle(ctrl->dev);
	if (!wpa_s) {
		status = CONNECTION_FAILURE;
		goto out;
	}

	if (ctrl->requested_op == CONNECT && wpa_s->wpa_state != WPA_COMPLETED) {
		if (ctrl->connection_timeout > 0 &&
		    seconds_counter++ > ctrl->connection_timeout) {
			if (!wpa_cli_cmd_v("disconnect")) {
				goto out;
			}

			conn_result = -ETIMEDOUT;
			supplicant_send_wifi_mgmt_event(wpa_s->ifname,
							NET_EVENT_WIFI_CMD_CONNECT_RESULT,
							(void *)&conn_result, sizeof(int));
			status = CONNECTION_FAILURE;
			goto out;
		}

		k_work_reschedule_for_queue(get_workq(), &wpa_supp_status_work,
					    K_SECONDS(OP_STATUS_POLLING_INTERVAL));
		ctrl->status_thread_state = STATUS_THREAD_RUNNING;
		k_mutex_unlock(&wpa_supplicant_mutex);
		return;
	}
out:
	seconds_counter = 0;

	ctrl->status_thread_state = STATUS_THREAD_STOPPED;
	k_mutex_unlock(&wpa_supplicant_mutex);
}

static struct hostapd_hw_modes *get_mode_by_band(struct wpa_supplicant *wpa_s, uint8_t band)
{
	enum hostapd_hw_mode hw_mode;
	bool is_6ghz = (band == WIFI_FREQ_BAND_6_GHZ) ? true : false;

	if (band == WIFI_FREQ_BAND_2_4_GHZ) {
		hw_mode = HOSTAPD_MODE_IEEE80211G;
	} else if ((band == WIFI_FREQ_BAND_5_GHZ) ||
		   (band == WIFI_FREQ_BAND_6_GHZ)) {
		hw_mode = HOSTAPD_MODE_IEEE80211A;
	} else {
		return NULL;
	}

	return get_mode(wpa_s->hw.modes, wpa_s->hw.num_modes, hw_mode, is_6ghz);
}

static int wpa_supp_supported_channels(struct wpa_supplicant *wpa_s, uint8_t band, char **chan_list)
{
	struct hostapd_hw_modes *mode = NULL;
	int i;
	int offset, retval;
	int size;
	char *_chan_list;

	mode = get_mode_by_band(wpa_s, band);
	if (!mode) {
		wpa_printf(MSG_ERROR, "Unsupported or invalid band: %d", band);
		return -EINVAL;
	}

	size = ((mode->num_channels) * CHAN_NUM_LEN) + 1;
	_chan_list = k_malloc(size);
	if (!_chan_list) {
		wpa_printf(MSG_ERROR, "Mem alloc failed for channel list");
		return -ENOMEM;
	}

	retval = 0;
	offset = 0;
	for (i = 0; i < mode->num_channels; i++) {
		retval = snprintf(_chan_list + offset, CHAN_NUM_LEN, " %d",
				  mode->channels[i].freq);
		offset += retval;
	}
	*chan_list = _chan_list;

	return 0;
}

static int wpa_supp_band_chan_compat(struct wpa_supplicant *wpa_s, uint8_t band, uint8_t channel)
{
	struct hostapd_hw_modes *mode = NULL;
	int i;

	mode = get_mode_by_band(wpa_s, band);
	if (!mode) {
		wpa_printf(MSG_ERROR, "Unsupported or invalid band: %d", band);
		return -EINVAL;
	}

	for (i = 0; i < mode->num_channels; i++) {
		if (mode->channels[i].freq == channel) {
			return mode->channels[i].freq;
		}
	}

	wpa_printf(MSG_ERROR, "Channel %d not supported for band %d", channel, band);

	return -EINVAL;
}

static inline void wpa_supp_restart_status_work(void)
{
	/* Terminate synchronously */
	wpas_api_ctrl.terminate = 1;
	k_work_flush_delayable(&wpa_supp_status_work, &wpas_api_ctrl.sync);
	wpas_api_ctrl.terminate = 0;

	/* Start afresh */
	k_work_reschedule_for_queue(get_workq(), &wpa_supp_status_work, K_MSEC(10));
}

static inline int chan_to_freq(int chan)
{
	/* We use global channel list here and also use the widest
	 * op_class for 5GHz channels as there is no user input
	 * for these (yet).
	 */
	int freq;

	freq = ieee80211_chan_to_freq(NULL, 81, chan);
	if (freq <= 0) {
		freq = ieee80211_chan_to_freq(NULL, 128, chan);
	}

	if (freq <= 0) {
		wpa_printf(MSG_ERROR, "Invalid channel %d", chan);
		return -1;
	}

	return freq;
}

static inline enum wifi_frequency_bands wpas_band_to_zephyr(enum wpa_radio_work_band band)
{
	switch (band) {
	case BAND_2_4_GHZ:
		return WIFI_FREQ_BAND_2_4_GHZ;
	case BAND_5_GHZ:
		return WIFI_FREQ_BAND_5_GHZ;
	default:
		return WIFI_FREQ_BAND_UNKNOWN;
	}
}

static inline enum wifi_security_type wpas_key_mgmt_to_zephyr(int key_mgmt, int proto)
{
	switch (key_mgmt) {
	case WPA_KEY_MGMT_NONE:
		return WIFI_SECURITY_TYPE_NONE;
	case WPA_KEY_MGMT_PSK:
		if (proto == WPA_PROTO_RSN) {
			return WIFI_SECURITY_TYPE_PSK;
		} else {
			return WIFI_SECURITY_TYPE_WPA_PSK;
		}
	case WPA_KEY_MGMT_PSK_SHA256:
		return WIFI_SECURITY_TYPE_PSK_SHA256;
	case WPA_KEY_MGMT_SAE:
		return WIFI_SECURITY_TYPE_SAE;
	default:
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}

static int wpas_add_and_config_network(struct wpa_supplicant *wpa_s,
				       struct wifi_connect_req_params *params,
				       bool mode_ap)
{
	int ret;
	struct add_network_resp resp = {0};
	char *chan_list = NULL;
	struct wpa_supplicant *wpa_s;
	int ret = 0;

	if (!wpa_cli_cmd_v("remove_network all")) {
		goto out;
	}

	ret = z_wpa_ctrl_add_network(&resp);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to add network");
		goto out;
	}

	wpa_printf(MSG_DEBUG, "NET added: %d", resp.network_id);

	if (mode_ap) {
		if (!wpa_cli_cmd_v("set_network %d mode 2", resp.network_id)) {
			goto out;
		}
	}

	if (!wpa_cli_cmd_v("set_network %d ssid \"%s\"",
			   resp.network_id, params->ssid)) {
		goto out;
	}

	if (!wpa_cli_cmd_v("set_network %d scan_ssid 1", resp.network_id)) {
		goto out;
	}

	if (!wpa_cli_cmd_v("set_network %d key_mgmt NONE", resp.network_id)) {
		goto out;
	}

	if (!wpa_cli_cmd_v("set_network %d ieee80211w 0", resp.network_id)) {
		goto out;

	if (params->band != WIFI_FREQ_BAND_UNKNOWN) {
		ret = wpa_supp_supported_channels(wpa_s, params->band, &chan_list);
		if (ret < 0) {
			goto rem_net;
		}

		if (chan_list) {
			if (!wpa_cli_cmd_v("set_network %d freq_list%s", resp.network_id,
					   chan_list)) {
				k_free(chan_list);
				goto out;
			}

			k_free(chan_list);
		}
	}

	if (params->security != WIFI_SECURITY_TYPE_NONE) {
		if (params->security == WIFI_SECURITY_TYPE_SAE) {
			if (params->sae_password) {
				if (!wpa_cli_cmd_v("set_network %d sae_password \"%s\"",
						   resp.network_id, params->sae_password)) {
					goto out;
				}
			} else {
				if (!wpa_cli_cmd_v("set_network %d sae_password \"%s\"",
						   resp.network_id, params->psk)) {
					goto out;
				}
			}

			if (!wpa_cli_cmd_v("set_network %d key_mgmt SAE", resp.network_id)) {
				goto out;
			}
		} else if (params->security == WIFI_SECURITY_TYPE_PSK_SHA256) {
			if (!wpa_cli_cmd_v("set_network %d psk \"%s\"",
					   resp.network_id, params->psk)) {
				goto out;
			}

			if (!wpa_cli_cmd_v("set_network %d key_mgmt WPA-PSK-SHA256",
					   resp.network_id)) {
				goto out;
			}
		} else if (params->security == WIFI_SECURITY_TYPE_PSK ||
			   params->security == WIFI_SECURITY_TYPE_WPA_PSK) {
			if (!wpa_cli_cmd_v("set_network %d psk \"%s\"",
					   resp.network_id, params->psk)) {
				goto out;
			}

			if (!wpa_cli_cmd_v("set_network %d key_mgmt WPA-PSK",
					   resp.network_id)) {
				goto out;
			}

			if (params->security == WIFI_SECURITY_TYPE_WPA_PSK) {
				if (!wpa_cli_cmd_v("set_network %d proto WPA",
						   resp.network_id)) {
					goto out;
				}
			} else {
				if (!wpa_cli_cmd_v("set_network %d proto RSN",
						   resp.network_id)) {
					goto out;
				}
			}
		} else {
			ret = -1;
			wpa_printf(MSG_ERROR, "Unsupported security type: %d",
				params->security);
			goto rem_net;
		}

		if (params->mfp) {
			if (!wpa_cli_cmd_v("set_network %d ieee80211w %d",
					   resp.network_id, params->mfp)) {
				goto out;
			}
		}
	}

	if (params->channel != WIFI_CHANNEL_ANY) {
		int freq;

		if (params->band != WIFI_FREQ_BAND_UNKNOWN) {
			freq = wpa_supp_band_chan_compat(wpa_s, params->band, params->channel);
			if (freq < 0) {
				goto rem_net;
			}
		} else {
			freq = chan_to_freq(params->channel);
			if (freq < 0) {
				ret = -1;
				wpa_printf(MSG_ERROR, "Invalid channel %d",
					params->channel);
				goto rem_net;
			}
		}

		if (mode_ap) {
			if (!wpa_cli_cmd_v("set_network %d frequency %d",
					   resp.network_id, freq)) {
				goto out;
			}
		} else {
			if (!wpa_cli_cmd_v("set_network %d scan_freq %d",
					   resp.network_id, freq)) {
				goto out;
			}
		}
	}

	/* enable and select network */
	if (!wpa_cli_cmd_v("enable_network %d", resp.network_id)) {
		goto out;
	}

	return 0;

rem_net:
	if (!wpa_cli_cmd_v("remove_network %d", resp.network_id)) {
		goto out;
	}

out:
	return ret;
}

static int wpas_disconnect_network(const struct device *dev, int cur_mode)
{
	struct net_if *iface = net_if_lookup_by_dev(dev);
	struct wpa_supplicant *wpa_s;
	bool is_ap = false;
	int ret = 0;

	if (!iface) {
		ret = -ENOENT;
		wpa_printf(MSG_ERROR, "Interface for device %s not found", dev->name);
		return ret;
	}

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	if (!wpa_s->current_ssid || wpa_s->current_ssid->mode != cur_mode) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in %s mode", dev->name,
			   cur_mode == WPAS_MODE_INFRA ? "STA" : "AP");
		goto out;
	}

	is_ap = (cur_mode == WPAS_MODE_AP);

	wpas_api_ctrl.dev = dev;
	wpas_api_ctrl.requested_op = DISCONNECT;

	if (!wpa_cli_cmd_v("disconnect")) {
		goto out;
	}

out:
	k_mutex_unlock(&wpa_supplicant_mutex);

	if (ret) {
		wpa_printf(MSG_ERROR, "Disconnect failed: %s", strerror(-ret));
		return ret;
	}

	wpa_supp_restart_status_work();

	ret = wait_for_disconnect_complete(dev);
#ifdef CONFIG_AP
	if (is_ap) {
		supplicant_send_wifi_mgmt_ap_status(wpa_s,
			NET_EVENT_WIFI_CMD_AP_DISABLE_RESULT,
			ret == 0 ? WIFI_STATUS_AP_SUCCESS : WIFI_STATUS_AP_FAIL);
	} else {
#else
	{
#endif /* CONFIG_AP */
		wifi_mgmt_raise_disconnect_complete_event(iface, ret);
	}

	return ret;
}

/* Public API */
int supplicant_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct wpa_supplicant *wpa_s;
	int ret = 0;

	if (!net_if_is_admin_up(net_if_lookup_by_dev(dev))) {
		wpa_printf(MSG_ERROR,
			   "Interface %s is down, dropping connect",
			   dev->name);
		return -1;
	}

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Device %s not found", dev->name);
		goto out;
	}

	/* Allow connect in STA mode only even if we are connected already */
	if  (wpa_s->current_ssid && wpa_s->current_ssid->mode != WPAS_MODE_INFRA) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in STA mode", dev->name);
		goto out;
	}

	ret = wpas_add_and_config_network(wpa_s, params, false);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to add and configure network for STA mode: %d", ret);
		goto out;
	}

	wpas_api_ctrl.dev = dev;
	wpas_api_ctrl.requested_op = CONNECT;
	wpas_api_ctrl.connection_timeout = params->timeout;

out:
	k_mutex_unlock(&wpa_supplicant_mutex);

	if (!ret) {
		wpa_supp_restart_status_work();
	}

	return ret;
}

int supplicant_disconnect(const struct device *dev)
{
	return wpas_disconnect_network(dev, WPAS_MODE_INFRA);
}

int supplicant_status(const struct device *dev, struct wifi_iface_status *status)
{
	struct net_if *iface = net_if_lookup_by_dev(dev);
	struct wpa_supplicant *wpa_s;
	int ret = -1;
	struct wpa_signal_info *si = NULL;
	struct wpa_conn_info *conn_info = NULL;

	if (!iface) {
		ret = -ENOENT;
		wpa_printf(MSG_ERROR, "Interface for device %s not found", dev->name);
		return ret;
	}

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR, "Device %s not found", dev->name);
		goto out;
	}

	si = os_zalloc(sizeof(struct wpa_signal_info));
	if (!si) {
		wpa_printf(MSG_ERROR, "Failed to allocate memory for signal info");
		goto out;
	}

	status->state = wpa_s->wpa_state; /* 1-1 Mapping */

	if (wpa_s->wpa_state >= WPA_ASSOCIATED) {
		struct wpa_ssid *ssid = wpa_s->current_ssid;
		u8 channel;
		struct signal_poll_resp signal_poll;
		u8 *_ssid = ssid->ssid;
		size_t ssid_len = ssid->ssid_len;
		struct status_resp cli_status;

		if (!ssid) {
			wpa_printf(MSG_ERROR, "Failed to get current ssid");
			goto out;
		}

		os_memcpy(status->bssid, wpa_s->bssid, WIFI_MAC_ADDR_LEN);
		status->band = wpas_band_to_zephyr(wpas_freq_to_band(wpa_s->assoc_freq));
		status->security = wpas_key_mgmt_to_zephyr(wpa_s->key_mgmt, wpa_s->wpa_proto);
		status->mfp = ssid->ieee80211w; /* Same mapping */
		ieee80211_freq_to_chan(wpa_s->assoc_freq, &channel);
		status->channel = channel;

		if (ssid_len == 0) {
			int _res = z_wpa_ctrl_status(&cli_status);

			if (_res < 0) {
				ssid_len = 0;
			} else {
				ssid_len = cli_status.ssid_len;
			}

			_ssid = cli_status.ssid;
		}

		os_memcpy(status->ssid, _ssid, ssid_len);
		status->ssid_len = ssid_len;
		status->iface_mode = ssid->mode;

		if (wpa_s->connection_set == 1) {
			status->link_mode = wpa_s->connection_he ? WIFI_6 :
					wpa_s->connection_vht ? WIFI_5 :
					wpa_s->connection_ht ? WIFI_4 :
					wpa_s->connection_g ? WIFI_3 :
					wpa_s->connection_a ? WIFI_2 :
					wpa_s->connection_b ? WIFI_1 :
					WIFI_0;
		} else {
			status->link_mode = WIFI_LINK_MODE_UNKNOWN;
		}

		status->rssi = -WPA_INVALID_NOISE;
		if (status->iface_mode == WIFI_MODE_INFRA) {
			ret = z_wpa_ctrl_signal_poll(&signal_poll);
			if (!ret) {
				status->rssi = signal_poll.rssi;
			} else {
				wpa_printf(MSG_WARNING, "%s:Failed to read RSSI", __func__);
			}
		}

		conn_info = os_zalloc(sizeof(struct wpa_conn_info));
		if (!conn_info) {
			wpa_printf(MSG_ERROR, "%s:Failed to allocate memory\n",
				__func__);
			ret = -ENOMEM;
			goto out;
		}

		ret = wpa_drv_get_conn_info(wpa_s, conn_info);
		if (!ret) {
			status->beacon_interval = conn_info->beacon_interval;
			status->dtim_period = conn_info->dtim_period;
			status->twt_capable = conn_info->twt_capable;
		} else {
			wpa_printf(MSG_WARNING, "%s: Failed to get connection info\n",
				__func__);

			status->beacon_interval = 0;
			status->dtim_period = 0;
			status->twt_capable = false;
			ret = 0;
		}

		os_free(conn_info);
	} else {
		ret = 0;
	}

out:
	os_free(si);
	k_mutex_unlock(&wpa_supplicant_mutex);
	return ret;
}

/* Below APIs are not natively supported by WPA supplicant, so,
 * these are just wrappers around driver offload APIs. But it is
 * transparent to the user.
 *
 * In the future these might be implemented natively by the WPA
 * supplicant.
 */

static const struct wifi_mgmt_ops *const get_wifi_mgmt_api(const struct device *dev)
{
	struct net_wifi_mgmt_offload *api = (struct net_wifi_mgmt_offload *)dev->api;

	return api ? api->wifi_mgmt_api : NULL;
}

int supplicant_scan(const struct device *dev, struct wifi_scan_params *params,
		    scan_result_cb_t cb)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->scan) {
		wpa_printf(MSG_ERROR, "Scan not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->scan(dev, params, cb);
}

#ifdef CONFIG_NET_STATISTICS_WIFI
int supplicant_get_stats(const struct device *dev, struct net_stats_wifi *stats)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->get_stats) {
		wpa_printf(MSG_ERROR, "Get stats not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->get_stats(dev, stats);
}
#endif /* CONFIG_NET_STATISTICS_WIFI */

int supplicant_set_power_save(const struct device *dev, struct wifi_ps_params *params)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->set_power_save) {
		wpa_printf(MSG_ERROR, "Set power save not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->set_power_save(dev, params);
}

int supplicant_set_twt(const struct device *dev, struct wifi_twt_params *params)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->set_twt) {
		wpa_printf(MSG_ERROR, "Set TWT not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->set_twt(dev, params);
}

int supplicant_get_power_save_config(const struct device *dev,
				     struct wifi_ps_config *config)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->get_power_save_config) {
		wpa_printf(MSG_ERROR, "Get power save config not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->get_power_save_config(dev, config);
}

int supplicant_reg_domain(const struct device *dev,
			  struct wifi_reg_domain *reg_domain)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->reg_domain) {
		wpa_printf(MSG_ERROR, "Regulatory domain not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->reg_domain(dev, reg_domain);
}

int supplicant_mode(const struct device *dev, struct wifi_mode_info *mode)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->mode) {
		wpa_printf(MSG_ERROR, "Setting mode not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->mode(dev, mode);
}

int supplicant_filter(const struct device *dev, struct wifi_filter_info *filter)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->filter) {
		wpa_printf(MSG_ERROR, "Setting filter not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->filter(dev, filter);
}

int supplicant_channel(const struct device *dev, struct wifi_channel_info *channel)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (!wifi_mgmt_api || !wifi_mgmt_api->channel) {
		wpa_printf(MSG_ERROR, "Setting channel not supported");
		return -ENOTSUP;
	}

	return wifi_mgmt_api->channel(dev, channel);
}

#ifdef CONFIG_AP
int supplicant_ap_enable(const struct device *dev,
			 struct wifi_connect_req_params *params)
{
	struct wpa_supplicant *wpa_s;
	int ret;

	if (!net_if_is_admin_up(net_if_lookup_by_dev(dev))) {
		wpa_printf(MSG_ERROR,
			   "Interface %s is down, dropping connect",
			   dev->name);
		return -1;
	}

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	if (wpa_s->wpa_state != WPA_DISCONNECTED) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in disconnected state", dev->name);
		goto out;
	}

	/* No need to check for existing network to join for SoftAP*/
	wpa_s->conf->ap_scan = 2;

	ret = wpas_add_and_config_network(wpa_s, params, true);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to add and configure network for AP mode: %d", ret);
		goto out;
	}

out:
	k_mutex_unlock(&wpa_supplicant_mutex);

	return ret;
}

int supplicant_ap_disable(const struct device *dev)
{
	struct wpa_supplicant *wpa_s;
	int ret = -1;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	ret = wpas_disconnect_network(dev, WPAS_MODE_AP);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to disconnect from network");
		goto out;
	}

	/* Restore ap_scan to default value */
	wpa_s->conf->ap_scan = 1;

out:
	k_mutex_unlock(&wpa_supplicant_mutex);
	return ret;
}
#endif /* CONFIG_AP */
