/*
 * @file
 * @brief SoftAP state machine handling
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

int sm_ap_timer_start(struct wifimgr_state_machine *sm, unsigned int cmd_id)
{
	int ret = 0;

	switch (cmd_id) {
	case WIFIMGR_CMD_DEL_STA:
		ret = wifimgr_timer_start(sm->timerid, WIFIMGR_EVENT_TIMEOUT);
		break;
	default:
		break;
	}

	if (ret)
		wifimgr_err("failed to start AP timer! %d\n", ret);

	return ret;
}

int sm_ap_timer_stop(struct wifimgr_state_machine *sm, unsigned int evt_id)
{
	int ret = 0;

	switch (evt_id) {
	case WIFIMGR_EVT_NEW_STATION:
		if (sm->cur_cmd == WIFIMGR_CMD_DEL_STA)
			ret = wifimgr_timer_stop(sm->timerid);
		break;
	default:
		break;
	}

	if (ret)
		wifimgr_err("failed to stop AP timer! %d\n", ret);

	return ret;
}

bool is_ap_common_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_GET_AP_CONFIG)
		&& (cmd_id < WIFIMGR_CMD_OPEN_AP));
}

bool is_ap_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_OPEN_AP)
		&& (cmd_id < WIFIMGR_CMD_MAX));
}

bool is_ap_evt(unsigned int evt_id)
{
	return ((evt_id >= WIFIMGR_EVT_NEW_STATION)
		&& (evt_id <= WIFIMGR_EVT_MAX));
}

int sm_ap_query(struct wifimgr_state_machine *sm)
{
	return sm->state;
}

bool sm_ap_started(struct wifimgr_state_machine *sm)
{
	return sm_ap_query(sm) == WIFI_STATE_AP_STARTED;
}

int sm_ap_query_cmd(struct wifimgr_state_machine *sm, unsigned int cmd_id)
{
	return 0;
}

void sm_ap_step(struct wifimgr_state_machine *sm, unsigned int next_state)
{
	sm->old_state = sm->state;
	sm->state = next_state;
	wifimgr_info("(%s) -> (%s)\n", ap_sts2str(sm->old_state),
		     ap_sts2str(sm->state));
}

void sm_ap_cmd_step(struct wifimgr_state_machine *sm, unsigned int cmd_id)
{
	sem_wait(&sm->exclsem);
	sm->old_state = sm->state;

	switch (sm->state) {
	case WIFI_STATE_AP_UNAVAIL:
		if (cmd_id == WIFIMGR_CMD_OPEN_AP)
			sm_ap_step(sm, WIFI_STATE_AP_READY);
		break;
	case WIFI_STATE_AP_READY:
		if (cmd_id == WIFIMGR_CMD_START_AP)
			sm_ap_step(sm, WIFI_STATE_AP_STARTED);
		else if (cmd_id == WIFIMGR_CMD_CLOSE_AP)
			sm_ap_step(sm, WIFI_STATE_AP_UNAVAIL);
		break;
	case WIFI_STATE_AP_STARTED:
		if (cmd_id == WIFIMGR_CMD_STOP_AP)
			sm_ap_step(sm, WIFI_STATE_AP_READY);
		else if (cmd_id == WIFIMGR_CMD_CLOSE_AP)
			sm_ap_step(sm, WIFI_STATE_AP_UNAVAIL);
		break;
	default:
		break;
	}

	sm->cur_cmd = cmd_id;
	sem_post(&sm->exclsem);
}
