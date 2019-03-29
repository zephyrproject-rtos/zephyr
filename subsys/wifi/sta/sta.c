/*
 * @file
 * @brief Station mode handling
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include "wifimgr.h"
#include "dhcpc.h"

K_THREAD_STACK_DEFINE(wifimgr_sta_wq_stack, WIFIMGR_WORKQUEUE_STACK_SIZE);

static int wifimgr_sta_close(void *handle);

void wifimgr_sta_event_timeout(struct wifimgr_delayed_work *dwork)
{
	struct wifimgr_state_machine *sm =
	    container_of(dwork, struct wifimgr_state_machine, dwork);
	struct wifi_manager *mgr =
	    container_of(sm, struct wifi_manager, sta_sm);
	unsigned int expected_evt;

	/* Remove event receiver, notify the external caller */
	switch (sm->cur_cmd) {
	case WIFIMGR_CMD_STA_SCAN:
		expected_evt = WIFIMGR_EVT_SCAN_DONE;
		wifimgr_warn("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_SCAN_RESULT);
		evt_listener_remove_receiver(&mgr->lsnr, expected_evt);
		wifimgr_ctrl_evt_timeout(&mgr->sta_ctrl);
		break;
	case WIFIMGR_CMD_RTT_REQ:
		expected_evt = WIFIMGR_EVT_RTT_DONE;
		wifimgr_warn("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_RTT_RESPONSE);
		evt_listener_remove_receiver(&mgr->lsnr, expected_evt);
		wifimgr_ctrl_evt_timeout(&mgr->sta_ctrl);
		break;
	case WIFIMGR_CMD_CONNECT:
		expected_evt = WIFIMGR_EVT_CONNECT;
		wifimgr_warn("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		evt_listener_remove_receiver(&mgr->lsnr, expected_evt);
		wifimgr_ctrl_evt_timeout(&mgr->sta_ctrl);
		break;
	case WIFIMGR_CMD_DISCONNECT:
		expected_evt = WIFIMGR_EVT_DISCONNECT;
		wifimgr_warn("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		wifimgr_ctrl_evt_timeout(&mgr->sta_ctrl);
		break;
	default:
		break;
	}

	sm_sta_step_back(sm);
}

static int wifimgr_sta_set_config(void *handle)
{
	struct wifi_config *conf = (struct wifi_config *)handle;

	if (!memiszero(conf, sizeof(struct wifi_config)))
		wifimgr_config_clear(conf, WIFIMGR_SETTING_STA_PATH);
	else
		wifimgr_config_save(conf, WIFIMGR_SETTING_STA_PATH);

	return 0;
}

static int wifimgr_sta_get_config(void *handle)
{
	struct wifi_config *conf = (struct wifi_config *)handle;

	/* Load config form non-volatile memory */
	memset(conf, 0, sizeof(struct wifi_config));
	wifimgr_config_load(conf, WIFIMGR_SETTING_STA_PATH);

	return 0;
}

static int wifimgr_sta_get_capa(void *handle)
{
	/* Nothing TODO */

	return 0;
}

static int wifimgr_sta_get_status(void *handle)
{
	struct wifi_status *sts = (struct wifi_status *)handle;
	struct wifi_manager *mgr =
	    container_of(sts, struct wifi_manager, sta_sts);
	struct wifimgr_state_machine *sm = &mgr->sta_sm;

	sts->state = sm_sta_query(sm);
	if (sm_sta_connected(sm) == true)
		if (wifi_drv_get_station(mgr->sta_iface, &sts->u.sta.host_rssi))
			wifimgr_warn("failed to get Host RSSI!\n");

	return 0;
}

static int wifimgr_sta_disconnect_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	char reason_code = sta_evt->u.evt_status;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifi_status *sts = &mgr->sta_sts;
	struct net_if *iface = (struct net_if *)mgr->sta_iface;

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
	memset(sts->u.sta.host_bssid, 0, WIFI_MAC_ADDR_LEN);
	sts->u.sta.host_rssi = 0;

	/* Notify the external caller */
	wifimgr_ctrl_evt_disconnect(&mgr->sta_ctrl, &mgr->sta_ctrl.disc_chain,
				    reason_code);

	wifimgr_sta_led_off();

	if (iface)
		wifimgr_dhcp_stop(iface);

	return 0;
}

static int wifimgr_sta_disconnect(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_disconnect(mgr->sta_iface);
	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_DISCONNECT);
		wifimgr_err("failed to disconnect! %d\n", ret);
	}

	return ret;
}

static int wifimgr_sta_connect_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	struct wifi_drv_connect_evt *conn = &sta_evt->u.conn;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifi_status *sts = &mgr->sta_sts;
	struct net_if *iface = (struct net_if *)mgr->sta_iface;

	if (!conn->status) {
		/* Register disconnect event here due to AP deauth */
		evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_DISCONNECT,
					  true, wifimgr_sta_disconnect_event,
					  &mgr->sta_evt);

		cmd_processor_add_sender(&mgr->prcs,
					 WIFIMGR_CMD_DISCONNECT,
					 WIFIMGR_CMD_TYPE_EXCHANGE,
					 wifimgr_sta_disconnect, mgr);

		if (!is_zero_ether_addr(conn->bssid))
			memcpy(sts->u.sta.host_bssid, conn->bssid,
			       WIFI_MAC_ADDR_LEN);

		wifimgr_sta_led_on();

		if (iface)
			wifimgr_dhcp_start(iface);

	}

	/* Notify the external caller */
	wifimgr_ctrl_evt_connect(&mgr->sta_ctrl, &mgr->sta_ctrl.conn_chain,
				 conn->status);

	return conn->status;
}

static int wifimgr_sta_connect(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifi_config *conf = &mgr->sta_conf;
	char *ssid = NULL;
	char *bssid = NULL;
	char *psk = NULL;
	char psk_len = 0;
	char wpa_psk[WIFIMGR_PSK_LEN];
	int ret;

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT,
					true, wifimgr_sta_connect_event,
					&mgr->sta_evt);
	if (ret)
		return ret;

	if (!memiszero(conf, sizeof(struct wifi_config))) {
		wifimgr_info("No STA config found!\n");
		return -EINVAL;
	}
	if (strlen(conf->ssid))
		ssid = conf->ssid;
	if (!is_zero_ether_addr(conf->bssid))
		bssid = conf->bssid;
	if (strlen(conf->passphrase)) {
		ret = pbkdf2_sha1(conf->passphrase, ssid, WIFIMGR_PSK_ITER,
				  wpa_psk, WIFIMGR_PSK_LEN);
		if (ret) {
			wifimgr_err("failed to calculate PSK! %d\n", ret);
			return ret;
		}
		psk = wpa_psk;
		psk_len = WIFIMGR_PSK_LEN;
	}

	ret = wifi_drv_connect(mgr->sta_iface, ssid, bssid, psk, psk_len,
			       conf->channel);
	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT);
		wifimgr_err("failed to connect! %d\n", ret);
	}

	wifimgr_info("Connecting to %s\n", conf->ssid);
	return ret;
}

static int wifimgr_sta_scan_result_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	struct wifi_drv_scan_result_evt *scan_res = &sta_evt->u.scan_res;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifi_scan_result *sta_scan_res = &mgr->sta_scan_res;
	struct wifi_config *conf = &mgr->sta_conf;
	struct wifi_status *sts = &mgr->sta_sts;

	/* Drop the invalid result */
	if (is_zero_ether_addr(scan_res->bssid))
		return 0;

	memcpy(sta_scan_res->bssid, scan_res->bssid, WIFI_MAC_ADDR_LEN);

	if (strlen(scan_res->ssid))
		strcpy(sta_scan_res->ssid, scan_res->ssid);

	sta_scan_res->band = scan_res->band;
	sta_scan_res->channel = scan_res->channel;
	sta_scan_res->rssi = scan_res->rssi;
	sta_scan_res->security = scan_res->security;
	sta_scan_res->rtt_supported = scan_res->rtt_supported;

	/* Find specified AP */
	if (!strcmp(scan_res->ssid, conf->ssid)) {
		/* Choose the first match when BSSID is not specified */
		if (is_zero_ether_addr(conf->bssid))
			sts->u.sta.host_found = 1;
		else if (!memcmp
			 (scan_res->bssid, conf->bssid, WIFI_MAC_ADDR_LEN))
			sts->u.sta.host_found = 1;
	}

	/* Notify the external caller */
	wifimgr_ctrl_evt_scan_result(&mgr->sta_ctrl, sta_scan_res);

	return 0;
}

static int wifimgr_sta_scan_done_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	char status = sta_evt->u.evt_status;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);

	evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_RESULT);

	/* Notify the external caller */
	wifimgr_ctrl_evt_scan_done(&mgr->sta_ctrl, status);

	return status;
}

static int wifimgr_sta_scan(void *handle)
{
	struct wifi_scan_params *params = (struct wifi_scan_params *)handle;
	struct wifi_manager *mgr =
	    container_of(params, struct wifi_manager, sta_scan_params);
	int ret;

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_RESULT,
					false, wifimgr_sta_scan_result_event,
					&mgr->sta_evt);
	if (ret)
		return ret;

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_DONE,
					true, wifimgr_sta_scan_done_event,
					&mgr->sta_evt);
	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_SCAN_RESULT);
		return ret;
	}

	mgr->sta_sts.u.sta.host_found = 0;

	ret = wifi_drv_scan(mgr->sta_iface, params->band, params->channel);
	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_SCAN_RESULT);
		evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_DONE);
		wifimgr_err("failed to trigger scan! %d\n", ret);
	}

	wifimgr_info("trgger scan!\n");
	return ret;
}

static int wifimgr_sta_rtt_response_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	struct wifi_drv_rtt_response_evt *rtt_resp = &sta_evt->u.rtt_resp;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifi_rtt_response *sta_rtt_resp = &mgr->sta_rtt_resp;

	if (!is_zero_ether_addr(rtt_resp->bssid))
		memcpy(sta_rtt_resp->bssid, rtt_resp->bssid, WIFI_MAC_ADDR_LEN);

	sta_rtt_resp->range = rtt_resp->range;

	/* Notify the external caller */
	wifimgr_ctrl_evt_rtt_response(&mgr->sta_ctrl, sta_rtt_resp);

	return 0;
}

static int wifimgr_sta_rtt_done_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	char status = sta_evt->u.evt_status;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);

	evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_RTT_RESPONSE);

	/* Notify the external caller */
	wifimgr_ctrl_evt_rtt_done(&mgr->sta_ctrl, status);

	return status;
}

static int wifimgr_sta_rtt_request(void *handle)
{
	struct wifi_rtt_request *rtt_req = (struct wifi_rtt_request *)handle;
	struct wifi_manager *mgr =
	    container_of(rtt_req, struct wifi_manager, sta_rtt_req);
	int ret;

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_RTT_RESPONSE,
					false, wifimgr_sta_rtt_response_event,
					&mgr->sta_evt);
	if (ret)
		return ret;

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_RTT_DONE,
					true, wifimgr_sta_rtt_done_event,
					&mgr->sta_evt);
	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_RTT_RESPONSE);
		return ret;
	}

	ret = wifi_drv_rtt(mgr->sta_iface, rtt_req->peers, rtt_req->nr_peers);
	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_RTT_RESPONSE);
		evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_RTT_DONE);
		wifimgr_err("failed to trigger RTT! %d\n", ret);
	}

	wifimgr_info("request RTT range!\n");
	return ret;
}

static int wifimgr_sta_open(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_open(mgr->sta_iface);
	if (ret) {
		wifimgr_err("failed to open STA!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_STA);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_STA,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_sta_close, mgr);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_STA_SCAN,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_sta_scan, &mgr->sta_scan_params);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_RTT_REQ,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_sta_rtt_request, &mgr->sta_rtt_req);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_CONNECT,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_sta_connect, mgr);

	wifimgr_info("open STA!\n");

	return ret;
}

static int wifimgr_sta_close(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_close(mgr->sta_iface);
	if (ret) {
		wifimgr_err("failed to close STA!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_STA);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_CONNECT);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_RTT_REQ);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_STA_SCAN);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_STA,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_sta_open, mgr);

	wifimgr_info("close STA!\n");
	return ret;
}

static int wifimgr_sta_drv_init(struct wifi_manager *mgr)
{
	char *devname = WIFIMGR_DEV_NAME_STA;
	struct net_if *iface = NULL;
	int ret;

	/* Initialize driver interface */
	iface = wifi_drv_init(devname);
	if (!iface) {
		wifimgr_err("failed to init WiFi STA driver!\n");
		return -ENODEV;
	}
	mgr->sta_iface = iface;

	/* Get MAC address */
	ret = wifi_drv_get_mac(iface, mgr->sta_sts.own_mac);
	if (ret)
		wifimgr_warn("failed to get Own MAC!\n");

	/* Check driver capability */
	ret = wifi_drv_get_capa(iface, &mgr->sta_capa);
	if (ret)
		wifimgr_warn("failed to get driver capability!");

	wifimgr_info("interface %s(" MACSTR ") initialized!\n", devname,
		     MAC2STR(mgr->sta_sts.own_mac));
	return 0;
}

int wifimgr_sta_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct cmd_processor *prcs = &mgr->prcs;
	int ret;

	/* Register default STA commands */
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_SET_STA_CONFIG,
				 WIFIMGR_CMD_TYPE_SET,
				 wifimgr_sta_set_config, &mgr->sta_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_STA_CONFIG,
				 WIFIMGR_CMD_TYPE_GET,
				 wifimgr_sta_get_config, &mgr->sta_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_STA_CAPA,
				 WIFIMGR_CMD_TYPE_GET,
				 wifimgr_sta_get_capa, &mgr->sta_capa);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_STA_STATUS,
				 WIFIMGR_CMD_TYPE_GET,
				 wifimgr_sta_get_status, &mgr->sta_sts);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_OPEN_STA,
				 WIFIMGR_CMD_TYPE_EXCHANGE,
				 wifimgr_sta_open, mgr);

	/* Initialize STA config */
	ret = wifimgr_settings_init(&mgr->sta_conf, WIFIMGR_SETTING_STA_PATH);
	if (ret)
		wifimgr_warn("failed to init WiFi STA config!\n");

	/* Initialize STA driver */
	ret = wifimgr_sta_drv_init(mgr);
	if (ret) {
		wifimgr_err("failed to init WiFi STA driver!\n");
		return ret;
	}

	/* Initialize STA state machine */
	ret = wifimgr_sm_init(&mgr->sta_sm, wifimgr_sta_event_timeout);
	if (ret)
		wifimgr_err("failed to init WiFi STA state machine!\n");
	wifimgr_create_workqueue(&mgr->sta_sm.dwork.wq, wifimgr_sta_wq_stack);

	/* Initialize STA global control */
	wifimgr_ctrl_iface_init(WIFIMGR_IFACE_NAME_STA, &mgr->sta_ctrl);

	return ret;
}

void wifimgr_sta_exit(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;

	/* Deinitialize STA global control */
	wifimgr_ctrl_iface_destroy(WIFIMGR_IFACE_NAME_STA, &mgr->sta_ctrl);

	/* Deinitialize STA state machine */
	wifimgr_sm_exit(&mgr->sta_sm);

	/* Deinitialize STA config */
	wifimgr_config_exit(WIFIMGR_SETTING_STA_PATH);
}
