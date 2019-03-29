/*
 * @file
 * @brief WiFi manager APIs for the external caller
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#include "ctrl_iface.h"

int wifi_register_connection_notifier(wifi_notifier_fn_t notifier_call)
{
	return wifimgr_register_connection_notifier(notifier_call);
}

int wifi_unregister_connection_notifier(wifi_notifier_fn_t notifier_call)
{
	return wifimgr_unregister_connection_notifier(notifier_call);
}

int wifi_register_disconnection_notifier(wifi_notifier_fn_t notifier_call)
{
	return wifimgr_register_disconnection_notifier(notifier_call);
}

int wifi_unregister_disconnection_notifier(wifi_notifier_fn_t notifier_call)
{
	return wifimgr_unregister_disconnection_notifier(notifier_call);
}

int wifi_register_new_station_notifier(wifi_notifier_fn_t notifier_call)
{
	return wifimgr_register_new_station_notifier(notifier_call);
}

int wifi_unregister_new_station_notifier(wifi_notifier_fn_t notifier_call)
{
	return wifimgr_unregister_new_station_notifier(notifier_call);
}

int wifi_register_station_leave_notifier(wifi_notifier_fn_t notifier_call)
{
	return wifimgr_register_station_leave_notifier(notifier_call);
}

int wifi_unregister_station_leave_notifier(wifi_notifier_fn_t notifier_call)
{
	return wifimgr_unregister_station_leave_notifier(notifier_call);
}

#ifdef CONFIG_WIFIMGR_STA
int wifi_sta_set_conf(struct wifi_config *conf)
{
	return wifimgr_ctrl_iface_set_conf(WIFIMGR_IFACE_NAME_STA, conf);
}

int wifi_sta_clear_conf(void)
{
	struct wifi_config conf;

	memset(&conf, 0, sizeof(conf));

	return wifimgr_ctrl_iface_set_conf(WIFIMGR_IFACE_NAME_STA, &conf);
}

int wifi_sta_get_conf(struct wifi_config *conf)
{
	return wifimgr_ctrl_iface_get_conf(WIFIMGR_IFACE_NAME_STA, conf);
}

int wifi_sta_get_capa(union wifi_drv_capa *capa)
{
	return wifimgr_ctrl_iface_get_capa(WIFIMGR_IFACE_NAME_STA, capa);
}

int wifi_sta_get_status(struct wifi_status *sts)
{
	return wifimgr_ctrl_iface_get_status(WIFIMGR_IFACE_NAME_STA, sts);
}

int wifi_sta_open(void)
{
	return wifimgr_ctrl_iface_open(WIFIMGR_IFACE_NAME_STA);
}

int wifi_sta_close(void)
{
	return wifimgr_ctrl_iface_close(WIFIMGR_IFACE_NAME_STA);
}

int wifi_sta_scan(struct wifi_scan_params *params, scan_res_cb_t cb)
{
	int ret;

	ret = wifimgr_ctrl_iface_scan(WIFIMGR_IFACE_NAME_STA, params, cb);
	if (ret)
		return ret;

	ret = wifimgr_ctrl_iface_wait_event(WIFIMGR_IFACE_NAME_STA);
	return ret;
}

int wifi_sta_rtt_request(struct wifi_rtt_request *req, rtt_resp_cb_t cb)
{
	int ret;

	ret = wifimgr_ctrl_iface_rtt_request(req, cb);
	if (ret)
		return ret;

	ret = wifimgr_ctrl_iface_wait_event(WIFIMGR_IFACE_NAME_STA);
	return ret;
}

int wifi_sta_connect(void)
{
	int ret;

	ret = wifimgr_ctrl_iface_connect();
	if (ret)
		return ret;

	ret = wifimgr_ctrl_iface_wait_event(WIFIMGR_IFACE_NAME_STA);
	return ret;
}

int wifi_sta_disconnect(void)
{
	int ret;

	ret = wifimgr_ctrl_iface_disconnect();
	if (ret)
		return ret;

	ret = wifimgr_ctrl_iface_wait_event(WIFIMGR_IFACE_NAME_STA);
	return ret;
}
#endif

#ifdef CONFIG_WIFIMGR_AP
int wifi_ap_set_conf(struct wifi_config *conf)
{
	return wifimgr_ctrl_iface_set_conf(WIFIMGR_IFACE_NAME_AP, conf);
}

int wifi_ap_clear_conf(void)
{
	struct wifi_config conf;

	memset(&conf, 0, sizeof(conf));

	return wifimgr_ctrl_iface_set_conf(WIFIMGR_IFACE_NAME_AP, &conf);
}

int wifi_ap_get_conf(struct wifi_config *conf)
{
	return wifimgr_ctrl_iface_get_conf(WIFIMGR_IFACE_NAME_AP, conf);
}

int wifi_ap_get_capa(union wifi_drv_capa *capa)
{
	return wifimgr_ctrl_iface_get_capa(WIFIMGR_IFACE_NAME_AP, capa);
}

int wifi_ap_get_status(struct wifi_status *sts)
{
	return wifimgr_ctrl_iface_get_status(WIFIMGR_IFACE_NAME_AP, sts);
}

int wifi_ap_open(void)
{
	return wifimgr_ctrl_iface_open(WIFIMGR_IFACE_NAME_AP);
}

int wifi_ap_close(void)
{
	return wifimgr_ctrl_iface_close(WIFIMGR_IFACE_NAME_AP);
}

int wifi_ap_start_ap(void)
{
	return wifimgr_ctrl_iface_start_ap();
}

int wifi_ap_stop_ap(void)
{
	return wifimgr_ctrl_iface_stop_ap();
}

int wifi_ap_del_station(char *mac)
{
	int ret;

	ret = wifimgr_ctrl_iface_del_station(mac);
	if (ret)
		return ret;

	ret = wifimgr_ctrl_iface_wait_event(WIFIMGR_IFACE_NAME_AP);
	return ret;
}

int wifi_ap_set_mac_acl(char subcmd, char *mac)
{
	return wifimgr_ctrl_iface_set_mac_acl(subcmd, mac);
}
#endif

#endif
