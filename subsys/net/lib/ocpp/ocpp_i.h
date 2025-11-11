/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OCPP_I_
#define __OCPP_I_

#include "string.h"
#include <zephyr/logging/log.h>
#include <zephyr/net/ocpp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>
#include <zephyr/sys/slist.h>

#if __POSIX_VISIBLE < 200809
char    *strdup(const char *);
#endif

/* case-insensitive */
#define CISTR20	20
#define CISTR25	25
#define CISTR500 500

#define INVALID_CONN_ID ((uint8_t) -1)
#define INVALID_TXN_ID ((int) -1)

#define to_session(ptr) CONTAINER_OF(ptr, struct ocpp_session, node)

enum ocpp_pdu_msg {
	PDU_BOOTNOTIFICATION,
	PDU_AUTHORIZE,
	PDU_START_TRANSACTION,
	PDU_STOP_TRANSACTION,
	PDU_HEARTBEAT,
	PDU_METER_VALUES,
	PDU_CLEAR_CACHE,
	PDU_REMOTE_START_TRANSACTION,
	PDU_REMOTE_STOP_TRANSACTION,
	PDU_GET_CONFIGURATION,
	PDU_CHANGE_CONFIGURATION,
	PDU_CHANGE_AVAILABILITY,
	PDU_UNLOCK_CONNECTOR,
	PDU_RESET,

	PDU_MSG_END,

	/* internal msg */
	PDU_CS_ONLINE,
};

enum ocpp_key_type {
	KEY_TYPE_BOOL = 1,
	KEY_TYPE_INT = sizeof(int),
	KEY_TYPE_STR,
	KEY_TYPE_CSL,
};

enum boot_status {
	BOOT_ACCEPTED,
	BOOT_PENDING,
	BOOT_REJECTED
};

enum ocpp_key {
	/* core mandatory */
	CFG_ALLOW_OFFLINE_TX_FOR_UNKN_ID,
	CFG_AUTH_CACHE_ENABLED,
	CFG_AUTH_REMOTE_TX_REQ,
	CFG_BLINK_REPEAT,
	CFG_CLK_ALIGN_DATA_INTERVAL,
	CFG_CONN_TIMEOUT,
	CFG_GETCFG_MAX_KEY,
	CFG_HEARTBEAT_INTERVAL,
	CFG_LIGHT_INTENSITY,
	CFG_LOCAL_AUTH_OFFLINE,
	CFG_LOCAL_PREAUTH,
	CFG_MAX_ENERGYON_INVL_ID,
	CFG_MTR_VAL_ALGIN_DATA,
	CFG_MTR_VAL_ALGIN_DATA_MAXLEN,
	CFG_MTR_VAL_SAMPLED_DATA,
	CFG_MTR_VAL_SAMPLED_DATA_MAXLEN,
	CFG_MTR_VAL_SAMPLE_INTERVAL,
	CFG_MIN_STATUS_DURATION,
	CFG_NO_OF_CONNECTORS,
	CFG_REST_RETRIES,
	CFG_CONN_PHASE_ROT,
	CFG_CONN_PHASE_ROT_MAXLEN,
	CFG_STOP_TXN_ON_EVSIDE_DISCON,
	CFG_STOP_TXN_ON_INVL_ID,
	CFG_STOP_TXN_ALIGNED_DATA,
	CFG_STOP_TXN_ALIGNED_DATA_MAXLEN,
	CFG_SUPPORTED_FEATURE_PROFILE,
	CFG_SUPPORTED_FEATURE_PROFILE_MAXLEN,
	CFG_TXN_MSG_ATTEMPTS,
	CFG_TXN_MSG_RETRY_INTERVAL,
	CFG_UNLOCK_CONN_ON_EVSIDE_DISCON,
	CFG_WEBSOCK_PING_INTERVAL,

	/* optional */

	OCPP_CFG_END
};

enum ocpp_cp_state {
	CP_STATE_INIT,
	CP_STATE_BOOTNOTIF,
	CP_STATE_READY,
};

enum ocpp_wamp_rpc {
	OCPP_WAMP_RPC_REQ = '2',
	OCPP_WAMP_RPC_RESP = '3',
	OCPP_WAMP_RPC_ERR = '4'
};

typedef int (*ocpp_msg_fp_t)(char *buf, ...);

struct boot_notif {
	enum boot_status status;
	int interval;
	struct timeval date;
};

struct ocpp_idtag_info {
	char idtag[CISTR20];
	char p_idtag[CISTR20];
	enum ocpp_auth_status auth_status;
	char exptime[CISTR25];
};

struct ocpp_upstream_info {
	struct k_mutex ws_sndlock; /* to server */
	struct k_poll_signal ws_rspsig; /* wait for resp parsed */
	int tcpsock;
	int wssock;
	struct k_thread tinfo;
	char recv_buf[CONFIG_OCPP_RECV_BUFFER_SIZE];
	char wsrecv_buf[CONFIG_OCPP_RECV_BUFFER_SIZE * 2];

	struct ocpp_cs_info csi;
};

struct ocpp_info {
	struct k_mutex ilock; /* internal to lib */
	sys_slist_t slist; /* session list */
	ocpp_msg_fp_t *cfn;
	ocpp_msg_fp_t *pfn;
	bool is_cs_offline;
	struct k_timer hb_timer;
	struct k_timer mtr_timer;
	atomic_t mtr_timer_ref_cnt;
	int hb_sec; /* heartbeat interval */
	struct k_msgq *msgq;
	struct k_thread tinfo;
	struct ocpp_upstream_info ui;
	bool is_cs_connected; /* central system */

	ocpp_user_notify_callback_t cb;
	void *user_data;
	enum ocpp_cp_state state;
	/* only for pdu message from internal thread (ocpp_internal_handler) */
	char pdu_buf[512];
};

struct ocpp_session {
	struct k_mutex slock; /* session lock */
	char idtag[CISTR20];
	bool is_active;
	uint8_t idcon;
	int idtxn;
	int resp_status;
	int uid;
	sys_snode_t node;
	struct ocpp_info *ctx;
};

union ocpp_keyval {
	int ival;
	char *str;
};

struct ocpp_wamp_rpc_msg {
	char *msg;
	size_t msg_len;
	struct ocpp_info *ctx;
	struct k_mutex *sndlock;
	struct k_poll_signal *rspsig;
};

struct internal_msg {
	enum ocpp_pdu_msg msgtype;
	union ocpp_io_value usr;
};

void ocpp_parser_init(ocpp_msg_fp_t **cfn, ocpp_msg_fp_t **pfn);
int parse_rpc_msg(char *msg, int msglen, char *uid, int uidlen,
		  int *pdu, bool *is_rsp);
int ocpp_send_to_server(struct ocpp_wamp_rpc_msg *snd, k_timeout_t timeout);
int ocpp_receive_from_server(struct ocpp_wamp_rpc_msg *rcv, uint32_t *msg_type,
			     uint32_t timeout);

int ocpp_unlock_connector(struct ocpp_info *ctx,
			  struct internal_msg *msg, char *uid);
int ocpp_get_configuration(enum ocpp_key key, struct ocpp_info *ctx, char *uid);
int ocpp_boot_notification(ocpp_session_handle_t hndl,
			   struct ocpp_cp_info *cpi);
int ocpp_heartbeat(ocpp_session_handle_t hndl);
void ocpp_get_utc_now(char utc[CISTR25]);
bool ocpp_session_is_valid(struct ocpp_session *sh);
int ocpp_remote_start_transaction(struct ocpp_info *ctx,
				  struct internal_msg *msg,
				  char *uid);
int ocpp_remote_stop_transaction(struct ocpp_info *ctx,
				 struct internal_msg *msg,
				 int idtxn, char *uid);
int ocpp_change_configuration(char *skey, struct ocpp_info *ctx,
			      char *sval, char *uid);
int ocpp_meter_values(ocpp_session_handle_t hndl,
		      enum ocpp_meter_measurand mes,
		      char *sval);

union ocpp_keyval *ocpp_get_key_val(enum ocpp_key key);
enum ocpp_key ocpp_key_to_cfg(const char *skey);
enum ocpp_key_type ocpp_get_keyval_type(enum ocpp_key key);
char *ocpp_get_key_literal(enum ocpp_key key);
bool ocpp_is_key_rw(enum ocpp_key key);
int ocpp_set_cfg_val(enum ocpp_key key, union ocpp_keyval *val);
int ocpp_update_cfg_val(enum ocpp_key key, union ocpp_keyval *val);
int ocpp_find_pdu_from_literal(const char *msg);
const char *ocpp_get_pdu_literal(enum ocpp_pdu_msg pdu);
#endif /* __OCPP_I_ */
