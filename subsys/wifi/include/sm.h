/*
 * @file
 * @brief State machine header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_SM_H_
#define _WIFI_SM_H_

#define WIFIMGR_SCAN_TIMEOUT	10
#define WIFIMGR_RTT_TIMEOUT	10
#define WIFIMGR_EVENT_TIMEOUT	10

struct wifimgr_delayed_work {
	wifimgr_workqueue wq;
	wifimgr_work work;
};

struct wifimgr_state_machine {
	sem_t exclsem;			/* exclusive access to the struct */
	timer_t timerid;		/* timer for event */
	struct wifimgr_delayed_work dwork;
	unsigned int state;
	unsigned int old_state;
	unsigned int cur_cmd;		/* record the current command */
};

int sm_sta_timer_start(struct wifimgr_state_machine *sm, unsigned int cmd_id);
int sm_sta_timer_stop(struct wifimgr_state_machine *sm, unsigned int evt_id);

bool is_sta_common_cmd(unsigned int cmd_id);
bool is_sta_cmd(unsigned int cmd_id);
bool is_sta_evt(unsigned int evt_id);
int sm_sta_query(struct wifimgr_state_machine *sm);
bool sm_sta_connected(struct wifimgr_state_machine *sm);
int sm_sta_query_cmd(struct wifimgr_state_machine *sm, unsigned int cmd_id);
void sm_sta_step(struct wifimgr_state_machine *sm, unsigned int next_state);
void sm_sta_step_back(struct wifimgr_state_machine *sm);
void sm_sta_cmd_step(struct wifimgr_state_machine *sm, unsigned int cmd_id);
void sm_sta_evt_step(struct wifimgr_state_machine *sm, unsigned int evt_id);

int sm_ap_timer_start(struct wifimgr_state_machine *sm, unsigned int cmd_id);
int sm_ap_timer_stop(struct wifimgr_state_machine *sm, unsigned int evt_id);
bool is_ap_common_cmd(unsigned int cmd_id);
bool is_ap_cmd(unsigned int cmd_id);
bool is_ap_evt(unsigned int evt_id);
int sm_ap_query(struct wifimgr_state_machine *sm);
bool sm_ap_started(struct wifimgr_state_machine *sm);
int sm_ap_query_cmd(struct wifimgr_state_machine *sm, unsigned int cmd_id);
void sm_ap_step(struct wifimgr_state_machine *sm, unsigned int next_state);
void sm_ap_cmd_step(struct wifimgr_state_machine *sm, unsigned int cmd_id);

const char *wifimgr_cmd2str(int cmd);
const char *wifimgr_evt2str(int evt);
const char *wifimgr_sts2str_cmd(void *handle, unsigned int cmd_id);
const char *wifimgr_sts2str_evt(void *handle, unsigned int evt_id);
int wifimgr_sm_query_cmd(void *handle, unsigned int cmd_id);
void wifimgr_sm_cmd_step(void *handle, unsigned int cmd_id, char indication);
void wifimgr_sm_evt_step(void *handle, unsigned int evt_id, char indication);
int wifimgr_sm_init(struct wifimgr_state_machine *sm, void *work_handler);
void wifimgr_sm_exit(struct wifimgr_state_machine *sm);

#endif
