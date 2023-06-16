/*
 * Copyright (c) 2023 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OCPP_I_H
#define __OCPP_I_H

#include "strings.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/slist.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>

/* case-insensitive */
#define CISTR20	20
#define CISTR25	25
#define CISTR500 500

#define INVALID_CONN_ID ((uint8_t) -1)
#define INVALID_TXN_ID ((int) -1)

#define to_session(ptr) CONTAINER_OF(ptr, ocpp_session_t, node)

typedef enum {
	BootNotification,
	Authorize,
	StartTransaction,
	StopTransaction,
	Heartbeat,
	MeterValues,
	ClearCache,
	RemoteStartTransaction,
	RemoteStopTransaction,
	GetConfiguration,
	ChangeConfiguration,
	ChangeAvailability,
	UnlockConnector,
	Reset,

	PduMsgEnd,

	/* internal msg */
	CS_Online,
}ocpp_pdu_msg_t;

typedef enum {
	KEY_TYPE_BOOL = 1,
	KEY_TYPE_INT = sizeof(int),
	KEY_TYPE_STR,
	KEY_TYPE_CSL,
}ocpp_key_type_t;

typedef enum {
	BOOT_ACCEPTED,
	BOOT_PENDING,
	BOOT_REJECTED
}boot_status_t;

typedef enum {
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
}ocpp_key_t;

typedef enum {
	CP_STATE_INIT,
	CP_STATE_BOOTNOTIF,
	CP_STATE_READY,
}ocpp_cp_state_t;

typedef enum {
	OCPP_WAMP_RPC_REQ = '2',
	OCPP_WAMP_RPC_RESP = '3',
	OCPP_WAMP_RPC_ERR = '4'
}ocpp_wamp_rpc_t;

typedef int (*ocpp_msg_fp_t)(char *buf, ...);

typedef struct {
	ocpp_pdu_msg_t pdu;
	char *spdu;
}ocpp_msg_table_t;

typedef struct {
	boot_status_t status;
	int interval;
	struct timeval date;
}boot_notif_t;

typedef struct {
	char idtag[CISTR20];
	char p_idtag[CISTR20];
	auth_status_t auth_status;
	struct timeval exptime;
}ocpp_idtag_info_t;

typedef struct {
	struct k_mutex ws_sndlock; // to server
	struct k_poll_signal ws_rspsig; // wait for resp parsed
	int tcpsock;
	int wssock;
	struct k_thread tinfo;
	char *recv_buf;
	char *wsrecv_buf;

	ocpp_cs_info_t csi;
}ocpp_upstream_info_t;

typedef struct {
	struct k_mutex ilock; // internal to lib
	sys_slist_t slist; //session list
	ocpp_msg_fp_t *cfn;
	ocpp_msg_fp_t *pfn;
	bool is_cs_offline;
	struct k_timer hb_timer;
	struct k_timer mtr_timer;
	int hb_sec; //heartbeat interval
	struct k_msgq *msgq;
	struct k_thread tinfo;
	void *ui;
	bool is_cs_connected; //central system

	ocpp_user_notify_callback_t cb;
	void *user_data;
	ocpp_cp_state_t state;
}ocpp_info_t;

typedef struct {
	char idtag[CISTR20];
	bool is_active;
	uint8_t idcon;
	int idtxn;
	int resp_status;
	int uid;
	sys_snode_t node;
	ocpp_info_t *ctx;
}ocpp_session_t;

typedef union {
	int ival;
	char *str;
}ocpp_keyval_t;

typedef struct {
	char *msg;
	size_t msg_len;
	ocpp_info_t *ctx;
	struct k_mutex *sndlock;
	struct k_poll_signal *rspsig;
}ocpp_wamp_rpc_msg_t;

typedef struct {
	ocpp_pdu_msg_t msgtype;
	ocpp_io_value_t usr;
}internal_msg_t;
 
#endif /* __OCPP_I_H */
