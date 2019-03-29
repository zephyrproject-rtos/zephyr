/*
 * @file
 * @brief Station state machine handling
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

int sm_sta_timer_start(struct wifimgr_state_machine *sm, unsigned int cmd_id)
{
	int ret = 0;

	switch (cmd_id) {
	case WIFIMGR_CMD_STA_SCAN:
		ret = wifimgr_timer_start(sm->timerid, WIFIMGR_SCAN_TIMEOUT);
		break;
	case WIFIMGR_CMD_RTT_REQ:
		ret = wifimgr_timer_start(sm->timerid, WIFIMGR_RTT_TIMEOUT);
		break;
	case WIFIMGR_CMD_CONNECT:
	case WIFIMGR_CMD_DISCONNECT:
		ret = wifimgr_timer_start(sm->timerid, WIFIMGR_EVENT_TIMEOUT);
		break;
	default:
		break;
	}

	if (ret)
		wifimgr_err("failed to start STA timer! %d\n", ret);

	return ret;
}

int sm_sta_timer_stop(struct wifimgr_state_machine *sm, unsigned int evt_id)
{
	int ret = 0;

	switch (evt_id) {
	case WIFIMGR_EVT_SCAN_DONE:
	case WIFIMGR_EVT_RTT_DONE:
	case WIFIMGR_EVT_CONNECT:
		ret = wifimgr_timer_stop(sm->timerid);
		break;
	case WIFIMGR_EVT_DISCONNECT:
		if (sm->cur_cmd == WIFIMGR_CMD_DISCONNECT)
			ret = wifimgr_timer_stop(sm->timerid);
		break;
	default:
		break;
	}

	if (ret)
		wifimgr_err("failed to stop STA timer! %d\n", ret);

	return ret;
}

bool is_sta_common_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= 0)
		&& (cmd_id < WIFIMGR_CMD_OPEN_STA));
}

bool is_sta_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_OPEN_STA)
		&& (cmd_id < WIFIMGR_CMD_GET_AP_CONFIG));
}

bool is_sta_evt(unsigned int evt_id)
{
	return ((evt_id >= WIFIMGR_EVT_SCAN_RESULT)
		&& (evt_id <= WIFIMGR_EVT_DISCONNECT));
}

int sm_sta_query(struct wifimgr_state_machine *sm)
{
	return sm->state;
}

bool sm_sta_connected(struct wifimgr_state_machine *sm)
{
	return sm_sta_query(sm) == WIFI_STATE_STA_CONNECTED;
}

int sm_sta_query_cmd(struct wifimgr_state_machine *sm, unsigned int cmd_id)
{
	int ret = 0;

	switch (sm_sta_query(sm)) {
	case WIFI_STATE_STA_SCANNING:
	case WIFI_STATE_STA_RTTING:
	case WIFI_STATE_STA_CONNECTING:
	case WIFI_STATE_STA_DISCONNECTING:
		ret = -EBUSY;
		break;
	}

	return ret;
}

void sm_sta_step(struct wifimgr_state_machine *sm, unsigned int next_state)
{
	sm->old_state = sm->state;
	sm->state = next_state;
	wifimgr_info("(%s) -> (%s)\n", sta_sts2str(sm->old_state),
		     sta_sts2str(sm->state));
}

void sm_sta_step_back(struct wifimgr_state_machine *sm)
{
	wifimgr_info("(%s) -> (%s)\n", sta_sts2str(sm->state),
		     sta_sts2str(sm->old_state));

	sem_wait(&sm->exclsem);
	if (sm->state != sm->old_state)
		sm->state = sm->old_state;
	sem_post(&sm->exclsem);
}

void sm_sta_cmd_step(struct wifimgr_state_machine *sm, unsigned int cmd_id)
{
	sem_wait(&sm->exclsem);
	sm->old_state = sm->state;

	switch (sm->state) {
	case WIFI_STATE_STA_UNAVAIL:
		if (cmd_id == WIFIMGR_CMD_OPEN_STA)
			sm_sta_step(sm, WIFI_STATE_STA_READY);
		break;
	case WIFI_STATE_STA_READY:
		if (cmd_id == WIFIMGR_CMD_STA_SCAN)
			sm_sta_step(sm, WIFI_STATE_STA_SCANNING);
		else if (cmd_id == WIFIMGR_CMD_RTT_REQ)
			sm_sta_step(sm, WIFI_STATE_STA_RTTING);
		else if (cmd_id == WIFIMGR_CMD_CONNECT)
			sm_sta_step(sm, WIFI_STATE_STA_CONNECTING);
		else if (cmd_id == WIFIMGR_CMD_CLOSE_STA)
			sm_sta_step(sm, WIFI_STATE_STA_UNAVAIL);
		break;
	case WIFI_STATE_STA_CONNECTED:
		if (cmd_id == WIFIMGR_CMD_STA_SCAN)
			sm_sta_step(sm, WIFI_STATE_STA_SCANNING);
		else if (cmd_id == WIFIMGR_CMD_RTT_REQ)
			sm_sta_step(sm, WIFI_STATE_STA_RTTING);
		else if (cmd_id == WIFIMGR_CMD_DISCONNECT)
			sm_sta_step(sm, WIFI_STATE_STA_DISCONNECTING);
		else if (cmd_id == WIFIMGR_CMD_CLOSE_STA)
			sm_sta_step(sm, WIFI_STATE_STA_UNAVAIL);
		break;
	default:
		break;
	}

	sm->cur_cmd = cmd_id;
	sem_post(&sm->exclsem);
}

void sm_sta_evt_step(struct wifimgr_state_machine *sm, unsigned int evt_id)
{
	sem_wait(&sm->exclsem);

	switch (sm->state) {
	case WIFI_STATE_STA_SCANNING:
		if (evt_id == WIFIMGR_EVT_SCAN_DONE)
			sm_sta_step(sm, sm->old_state);
		break;
	case WIFI_STATE_STA_RTTING:
		if (evt_id == WIFIMGR_EVT_RTT_DONE)
			sm_sta_step(sm, sm->old_state);
		break;
	case WIFI_STATE_STA_CONNECTING:
		if (evt_id == WIFIMGR_EVT_CONNECT)
			sm_sta_step(sm, WIFI_STATE_STA_CONNECTED);
		break;
	case WIFI_STATE_STA_DISCONNECTING:
		if (evt_id == WIFIMGR_EVT_DISCONNECT)
			sm_sta_step(sm, WIFI_STATE_STA_READY);
		break;
	case WIFI_STATE_STA_CONNECTED:
		if (evt_id == WIFIMGR_EVT_DISCONNECT)
			sm_sta_step(sm, WIFI_STATE_STA_READY);
		break;
	default:
		break;
	}

	sem_post(&sm->exclsem);
}
