/*
 * @file
 * @brief Control interface header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_CTRL_IFACE_H_
#define _WIFIMGR_CTRL_IFACE_H_

#include <net/wifi_api.h>

#include "notifier.h"

#define WIFIMGR_IFACE_NAME_STA	"sta"
#define WIFIMGR_IFACE_NAME_AP	"ap"

enum wifimgr_cmd {
	/*STA Common command */
	WIFIMGR_CMD_SET_STA_CONFIG,
	WIFIMGR_CMD_GET_STA_CONFIG,
	WIFIMGR_CMD_GET_STA_CAPA,
	WIFIMGR_CMD_GET_STA_STATUS,
	/*STA command */
	WIFIMGR_CMD_OPEN_STA,
	WIFIMGR_CMD_CLOSE_STA,
	WIFIMGR_CMD_STA_SCAN,
	WIFIMGR_CMD_RTT_REQ,
	WIFIMGR_CMD_CONNECT,
	WIFIMGR_CMD_DISCONNECT,
	/*AP Common command */
	WIFIMGR_CMD_GET_AP_CONFIG,
	WIFIMGR_CMD_SET_AP_CONFIG,
	WIFIMGR_CMD_GET_AP_CAPA,
	WIFIMGR_CMD_GET_AP_STATUS,
	/*AP command */
	WIFIMGR_CMD_OPEN_AP,
	WIFIMGR_CMD_CLOSE_AP,
	WIFIMGR_CMD_AP_SCAN,
	WIFIMGR_CMD_START_AP,
	WIFIMGR_CMD_STOP_AP,
	WIFIMGR_CMD_DEL_STA,
	WIFIMGR_CMD_SET_MAC_ACL,

	WIFIMGR_CMD_MAX,
};

struct wifimgr_set_mac_acl {
	char subcmd;
	char mac[WIFI_MAC_ADDR_LEN];
};

struct wifimgr_ctrl_iface {
	sem_t syncsem;		/* synchronization for async command */
	mqd_t mq;
	bool wait_event;
	char evt_status;
	scan_res_cb_t scan_res_cb;
	rtt_resp_cb_t rtt_resp_cb;
	struct wifimgr_notifier_chain conn_chain;
	struct wifimgr_notifier_chain disc_chain;
	struct wifimgr_notifier_chain new_sta_chain;
	struct wifimgr_notifier_chain sta_leave_chain;
};

int wifimgr_register_connection_notifier(wifi_notifier_fn_t notifier_call);
int wifimgr_unregister_connection_notifier(wifi_notifier_fn_t notifier_call);
int wifimgr_register_disconnection_notifier(wifi_notifier_fn_t notifier_call);
int wifimgr_unregister_disconnection_notifier(wifi_notifier_fn_t notifier_call);
int wifimgr_register_new_station_notifier(wifi_notifier_fn_t notifier_call);
int wifimgr_unregister_new_station_notifier(wifi_notifier_fn_t notifier_call);
int wifimgr_register_station_leave_notifier(wifi_notifier_fn_t notifier_call);
int wifimgr_unregister_station_leave_notifier(wifi_notifier_fn_t notifier_call);

void wifimgr_ctrl_evt_scan_result(void *handle, struct wifi_scan_result *res);
void wifimgr_ctrl_evt_scan_done(void *handle, char status);
void wifimgr_ctrl_evt_rtt_response(void *handle,
				   struct wifi_rtt_response *resp);
void wifimgr_ctrl_evt_rtt_done(void *handle, char status);
void wifimgr_ctrl_evt_connect(void *handle,
			      struct wifimgr_notifier_chain *chain,
			      char status);
void wifimgr_ctrl_evt_disconnect(void *handle,
				 struct wifimgr_notifier_chain *chain,
				 char reason_code);
void wifimgr_ctrl_evt_new_station(void *handle,
				  struct wifimgr_notifier_chain *chain,
				  char status, char *mac);

void wifimgr_ctrl_evt_timeout(void *handle);

int wifimgr_ctrl_iface_get_conf(char *iface_name, struct wifi_config *conf);
int wifimgr_ctrl_iface_set_conf(char *iface_name,
				struct wifi_config *user_conf);
int wifimgr_ctrl_iface_get_capa(char *iface_name, union wifi_drv_capa *capa);
int wifimgr_ctrl_iface_get_status(char *iface_name, struct wifi_status *sts);
int wifimgr_ctrl_iface_open(char *iface_name);
int wifimgr_ctrl_iface_close(char *iface_name);
int wifimgr_ctrl_iface_scan(char *iface_name,
			    struct wifi_scan_params *params, scan_res_cb_t cb);
int wifimgr_ctrl_iface_rtt_request(struct wifi_rtt_request *req,
				   rtt_resp_cb_t cb);
int wifimgr_ctrl_iface_connect(void);
int wifimgr_ctrl_iface_disconnect(void);
int wifimgr_ctrl_iface_start_ap(void);
int wifimgr_ctrl_iface_stop_ap(void);
int wifimgr_ctrl_iface_del_station(char *mac);
int wifimgr_ctrl_iface_set_mac_acl(char subcmd, char *mac);

int wifimgr_ctrl_iface_wait_event(char *iface_name);
int wifimgr_ctrl_iface_wakeup(struct wifimgr_ctrl_iface *ctrl);

int wifimgr_ctrl_iface_init(char *iface_name, struct wifimgr_ctrl_iface *ctrl);
int wifimgr_ctrl_iface_destroy(char *iface_name,
			       struct wifimgr_ctrl_iface *ctrl);

#endif
