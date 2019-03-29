/*
 * @file
 * @brief State machine handling
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#include "wifimgr.h"

const char *wifimgr_cmd2str(int cmd)
{
	switch (cmd) {
	C2S(WIFIMGR_CMD_SET_STA_CONFIG)
	C2S(WIFIMGR_CMD_GET_STA_CONFIG)
	C2S(WIFIMGR_CMD_GET_STA_STATUS)
	C2S(WIFIMGR_CMD_GET_STA_CAPA)
	C2S(WIFIMGR_CMD_OPEN_STA)
	C2S(WIFIMGR_CMD_CLOSE_STA)
	C2S(WIFIMGR_CMD_STA_SCAN)
	C2S(WIFIMGR_CMD_RTT_REQ)
	C2S(WIFIMGR_CMD_CONNECT)
	C2S(WIFIMGR_CMD_DISCONNECT)
	C2S(WIFIMGR_CMD_GET_AP_CONFIG)
	C2S(WIFIMGR_CMD_SET_AP_CONFIG)
	C2S(WIFIMGR_CMD_GET_AP_STATUS)
	C2S(WIFIMGR_CMD_GET_AP_CAPA)
	C2S(WIFIMGR_CMD_OPEN_AP)
	C2S(WIFIMGR_CMD_CLOSE_AP)
	C2S(WIFIMGR_CMD_AP_SCAN)
	C2S(WIFIMGR_CMD_START_AP)
	C2S(WIFIMGR_CMD_STOP_AP)
	C2S(WIFIMGR_CMD_DEL_STA)
	C2S(WIFIMGR_CMD_SET_MAC_ACL)
	default:
		return "WIFIMGR_CMD_UNKNOWN";
	}
}

const char *wifimgr_evt2str(int evt)
{
	switch (evt) {
	C2S(WIFIMGR_EVT_SCAN_RESULT)
	C2S(WIFIMGR_EVT_SCAN_DONE)
	C2S(WIFIMGR_EVT_RTT_RESPONSE)
	C2S(WIFIMGR_EVT_RTT_DONE)
	C2S(WIFIMGR_EVT_CONNECT)
	C2S(WIFIMGR_EVT_DISCONNECT)
	C2S(WIFIMGR_EVT_NEW_STATION)
	default:
		return "WIFIMGR_EVT_UNKNOWN";
	}
}

const char *wifimgr_sts2str_cmd(void *handle, unsigned int cmd_id)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;

#ifdef CONFIG_WIFIMGR_STA
	if (is_sta_cmd(cmd_id) || is_sta_common_cmd(cmd_id) == true)
		return sta_sts2str(mgr->sta_sm.state);
#endif
#ifdef CONFIG_WIFIMGR_AP
	if (is_ap_cmd(cmd_id) == true || is_ap_common_cmd(cmd_id))
		return ap_sts2str(mgr->ap_sm.state);
#endif

	return NULL;
}

const char *wifimgr_sts2str_evt(void *handle, unsigned int evt_id)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;

#ifdef CONFIG_WIFIMGR_STA
	if (is_sta_evt(evt_id) == true)
		return sta_sts2str(mgr->sta_sm.state);
#endif
#ifdef CONFIG_WIFIMGR_AP
	if (is_ap_evt(evt_id) == true)
		return ap_sts2str(mgr->ap_sm.state);
#endif

	return NULL;
}

int wifimgr_sm_query_cmd(void *handle, unsigned int cmd_id)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret = 0;

#ifdef CONFIG_WIFIMGR_STA
	if (is_sta_cmd(cmd_id) == true)
		ret = sm_sta_query_cmd(&mgr->sta_sm, cmd_id);
#endif
#ifdef CONFIG_WIFIMGR_AP
	if (is_ap_cmd(cmd_id) == true)
		ret = sm_ap_query_cmd(&mgr->ap_sm, cmd_id);
#endif

	return ret;
}

void wifimgr_sm_cmd_step(void *handle, unsigned int cmd_id, char indication)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm;

#ifdef CONFIG_WIFIMGR_STA
	if (is_sta_cmd(cmd_id) == true) {
		sm = &mgr->sta_sm;
		if (!indication) {
			/* Step to next state and start timer on success */
			sm_sta_cmd_step(sm, cmd_id);
			sm_sta_timer_start(sm, cmd_id);
		} else {
			/* Remain current state on failure */
			wifimgr_err("failed to exec [%s]! remains %s\n",
				    wifimgr_cmd2str(cmd_id),
				    sta_sts2str(sm->state));
		}
	}
#endif
#ifdef CONFIG_WIFIMGR_AP
	if (is_ap_cmd(cmd_id) == true) {
		sm = &mgr->ap_sm;
		if (!indication) {
			/* Step to next state and start timer on success */
			sm_ap_cmd_step(sm, cmd_id);
			sm_ap_timer_start(sm, cmd_id);
		} else {
			/* Remain current state on failure */
			wifimgr_err("failed to exec [%s]! remains %s\n",
				    wifimgr_cmd2str(cmd_id),
				    ap_sts2str(sm->state));
		}
	}
#endif
}

void wifimgr_sm_evt_step(void *handle, unsigned int evt_id, char indication)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm = NULL;

#ifdef CONFIG_WIFIMGR_STA
	if (is_sta_evt(evt_id) == true) {
		sm = &mgr->sta_sm;
		/* Stop timer when receiving an event */
		sm_sta_timer_stop(sm, evt_id);
		if (!indication) {
			/* Step to next state on success */
			sm_sta_evt_step(sm, evt_id);
			/*softAP does not need step evt for now */
		} else {
			/*Roll back to previous state on failure */
			sm_sta_step_back(sm);
		}
	}
#endif
#ifdef CONFIG_WIFIMGR_AP
	if (is_ap_evt(evt_id) == true) {
		sm = &mgr->ap_sm;
		/* Stop timer when receiving an event */
		sm_ap_timer_stop(sm, evt_id);
		/* softAP does not need step evt/back for now */
	}
#endif

}

int wifimgr_sm_init(struct wifimgr_state_machine *sm, void *work_handler)
{
	int ret;

	sem_init(&sm->exclsem, 0, 1);
	wifimgr_init_work(&sm->dwork.work, work_handler);

	ret = wifimgr_timer_init(&sm->dwork, wifimgr_timeout, &sm->timerid);
	if (ret < 0)
		wifimgr_err("failed to init WiFi timer! %d\n", ret);

	return ret;
}

void wifimgr_sm_exit(struct wifimgr_state_machine *sm)
{
	if (sm->timerid)
		wifimgr_timer_release(sm->timerid);
}

#endif
