/** @file
 *  @brief Internal APIs for Bluetooth TBS.
 */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/tbs.h>

#define BT_TBS_MAX_UCI_SIZE                        6
#define BT_TBS_MIN_URI_LEN                         3 /* a:b */
#define BT_TBS_FREE_CALL_INDEX                     0

/* Call Control Point Opcodes */
#define BT_TBS_CALL_OPCODE_ACCEPT                  0x00
#define BT_TBS_CALL_OPCODE_TERMINATE               0x01
#define BT_TBS_CALL_OPCODE_HOLD                    0x02
#define BT_TBS_CALL_OPCODE_RETRIEVE                0x03
#define BT_TBS_CALL_OPCODE_ORIGINATE               0x04
#define BT_TBS_CALL_OPCODE_JOIN                    0x05

/* Local Control Points - Used to do local control operations but still being
 * able to determine if it is a local or remote operation
 */
#define BT_TBS_LOCAL_OPCODE_ANSWER                 0x80
#define BT_TBS_LOCAL_OPCODE_HOLD                   0x81
#define BT_TBS_LOCAL_OPCODE_RETRIEVE               0x82
#define BT_TBS_LOCAL_OPCODE_TERMINATE              0x83
#define BT_TBS_LOCAL_OPCODE_INCOMING               0x84
#define BT_TBS_LOCAL_OPCODE_SERVER_TERMINATE       0x85

#define FIRST_PRINTABLE_ASCII_CHAR ' ' /* space */

const char *parse_string_value(const void *data, uint16_t length,
				      uint16_t max_len);

static inline const char *bt_tbs_state_str(uint8_t state)
{
	switch (state) {
	case BT_TBS_CALL_STATE_INCOMING:
		return "incoming";
	case BT_TBS_CALL_STATE_DIALING:
		return "dialing";
	case BT_TBS_CALL_STATE_ALERTING:
		return "alerting";
	case BT_TBS_CALL_STATE_ACTIVE:
		return "active";
	case BT_TBS_CALL_STATE_LOCALLY_HELD:
		return "locally held";
	case BT_TBS_CALL_STATE_REMOTELY_HELD:
		return "remote held";
	case BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD:
		return "locally and remotely held";
	default:
		return "unknown";
	}
}

static inline const char *bt_tbs_opcode_str(uint8_t opcode)
{
	switch (opcode) {
	case BT_TBS_CALL_OPCODE_ACCEPT:
		return "accept";
	case BT_TBS_CALL_OPCODE_TERMINATE:
		return "terminate";
	case BT_TBS_CALL_OPCODE_HOLD:
		return "hold";
	case BT_TBS_CALL_OPCODE_RETRIEVE:
		return "retrieve";
	case BT_TBS_CALL_OPCODE_ORIGINATE:
		return "originate";
	case BT_TBS_CALL_OPCODE_JOIN:
		return "join";
	case BT_TBS_LOCAL_OPCODE_ANSWER:
		return "remote answer";
	case BT_TBS_LOCAL_OPCODE_HOLD:
		return "remote hold";
	case BT_TBS_LOCAL_OPCODE_RETRIEVE:
		return "remote retrieve";
	case BT_TBS_LOCAL_OPCODE_TERMINATE:
		return "remote terminate";
	case BT_TBS_LOCAL_OPCODE_INCOMING:
		return "remote incoming";
	case BT_TBS_LOCAL_OPCODE_SERVER_TERMINATE:
		return "server terminate";
	default:
		return "unknown";
	}
}

static inline const char *bt_tbs_status_str(uint8_t status)
{
	switch (status) {
	case BT_TBS_RESULT_CODE_SUCCESS:
		return "success";
	case BT_TBS_RESULT_CODE_OPCODE_NOT_SUPPORTED:
		return "opcode not supported";
	case BT_TBS_RESULT_CODE_OPERATION_NOT_POSSIBLE:
		return "operation not possible";
	case BT_TBS_RESULT_CODE_INVALID_CALL_INDEX:
		return "invalid call index";
	case BT_TBS_RESULT_CODE_STATE_MISMATCH:
		return "state mismatch";
	case BT_TBS_RESULT_CODE_OUT_OF_RESOURCES:
		return "out of resources";
	case BT_TBS_RESULT_CODE_INVALID_URI:
		return "invalid URI";
	default:
		return "ATT err";
	}
}

static inline const char *bt_tbs_technology_str(uint8_t status)
{
	switch (status) {
	case BT_TBS_TECHNOLOGY_3G:
		return "3G";
	case BT_TBS_TECHNOLOGY_4G:
		return "4G";
	case BT_TBS_TECHNOLOGY_LTE:
		return "LTE";
	case BT_TBS_TECHNOLOGY_WIFI:
		return "WIFI";
	case BT_TBS_TECHNOLOGY_5G:
		return "5G";
	case BT_TBS_TECHNOLOGY_GSM:
		return "GSM";
	case BT_TBS_TECHNOLOGY_CDMA:
		return "CDMA";
	case BT_TBS_TECHNOLOGY_2G:
		return "2G";
	case BT_TBS_TECHNOLOGY_WCDMA:
		return "WCDMA";
	case BT_TBS_TECHNOLOGY_IP:
		return "IP";
	default:
		return "unknown technology";
	}
}

static inline const char *bt_tbs_term_reason_str(uint8_t reason)
{
	switch (reason) {
	case BT_TBS_REASON_BAD_REMOTE_URI:
		return "bad remote URI";
	case BT_TBS_REASON_CALL_FAILED:
		return "call failed";
	case BT_TBS_REASON_REMOTE_ENDED_CALL:
		return "remote ended call";
	case BT_TBS_REASON_SERVER_ENDED_CALL:
		return "server ended call";
	case BT_TBS_REASON_LINE_BUSY:
		return "line busy";
	case BT_TBS_REASON_NETWORK_CONGESTED:
		return "network congested";
	case BT_TBS_REASON_CLIENT_TERMINATED:
		return "client terminated";
	case BT_TBS_REASON_UNSPECIFIED:
		return "unspecified";
	default:
		return "unknown reason";
	}
}

/**
 * @brief Checks if a string contains a colon (':') followed by a printable
 * character. Minimal uri is "a:b".
 *
 * @param uri The uri "scheme:id"
 * @return true If the above is true
 * @return false If the above is not true
 */
static inline bool bt_tbs_valid_uri(const char *uri)
{
	size_t len;

	if (!uri) {
		return false;
	}

	len = strlen(uri);
	if (len > CONFIG_BT_TBS_MAX_URI_LENGTH ||
	    len < BT_TBS_MIN_URI_LEN) {
		return false;
	} else if (uri[0] < FIRST_PRINTABLE_ASCII_CHAR) {
		/* Invalid first char */
		return false;
	}

	for (int i = 1; i < len; i++) {
		if (uri[i] == ':' && uri[i + 1] >= FIRST_PRINTABLE_ASCII_CHAR) {
			return true;
		}
	}

	return false;
}

/* TODO: The bt_tbs_call could use the bt_tbs_call_state struct for the first
 * 3 fields
 */
struct bt_tbs_call {
	uint8_t index;
	uint8_t state;
	uint8_t flags;
	char remote_uri[CONFIG_BT_TBS_MAX_URI_LENGTH + 1];
} __packed;

struct bt_tbs_call_state {
	uint8_t index;
	uint8_t state;
	uint8_t flags;
} __packed;

struct bt_tbs_call_cp_acc {
	uint8_t opcode;
	uint8_t call_index;
} __packed;

struct bt_tbs_call_cp_term {
	uint8_t opcode;
	uint8_t call_index;
} __packed;

struct bt_tbs_call_cp_hold {
	uint8_t opcode;
	uint8_t call_index;
} __packed;

struct bt_tbs_call_cp_retrieve {
	uint8_t opcode;
	uint8_t call_index;
} __packed;

struct bt_tbs_call_cp_originate {
	uint8_t opcode;
	uint8_t uri[0];
} __packed;

struct bt_tbs_call_cp_join {
	uint8_t opcode;
	uint8_t call_indexes[0];
} __packed;

union bt_tbs_call_cp_t {
	uint8_t opcode;
	struct bt_tbs_call_cp_acc accept;
	struct bt_tbs_call_cp_term terminate;
	struct bt_tbs_call_cp_hold hold;
	struct bt_tbs_call_cp_retrieve retrieve;
	struct bt_tbs_call_cp_originate originate;
	struct bt_tbs_call_cp_join join;
} __packed;

struct bt_tbs_call_cp_notify {
	uint8_t opcode;
	uint8_t call_index;
	uint8_t status;
} __packed;

struct bt_tbs_call_state_notify {
	uint8_t call_index;
	uint8_t state;
} __packed;

struct bt_tbs_terminate_reason {
	uint8_t call_index;
	uint8_t reason;
} __packed;

struct bt_tbs_current_call_item {
	uint8_t length;
	uint8_t call_index;
	uint8_t call_state;
	uint8_t uri[CONFIG_BT_TBS_MAX_URI_LENGTH];
} __packed;

struct bt_tbs_in_uri {
	uint8_t call_index;
	char uri[CONFIG_BT_TBS_MAX_URI_LENGTH + 1];
} __packed;

#if defined(CONFIG_BT_TBS_CLIENT)

/* Features which may require long string reads */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) || \
	defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) || \
	defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) || \
	defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) || \
	defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) || \
	defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) || \
	defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
#define BT_TBS_CLIENT_INST_READ_BUF_SIZE (BT_ATT_MAX_ATTRIBUTE_LEN)
#else
/* Need only be the size of call state reads which is the largest of the
 * remaining characteristic values
 */
#define BT_TBS_CLIENT_INST_READ_BUF_SIZE \
		MIN(BT_ATT_MAX_ATTRIBUTE_LEN, \
			(CONFIG_BT_TBS_CLIENT_MAX_CALLS \
			* sizeof(struct bt_tbs_client_call_state)))
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

struct bt_tbs_instance {
	struct bt_tbs_client_call_state calls[CONFIG_BT_TBS_CLIENT_MAX_CALLS];

	uint16_t start_handle;
	uint16_t end_handle;
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
	uint16_t bearer_uci_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
	uint16_t uri_list_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */
#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) \
|| defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL)
	uint16_t signal_interval_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */
/* || defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL) */
#if defined(CONFIG_BT_TBS_CLIENT_CCID)
	uint16_t ccid_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */
#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
	uint16_t optional_opcodes_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */
	uint16_t termination_reason_handle;

	bool busy;
	uint8_t subscribe_cnt;
#if defined(CONFIG_BT_TBS_CLIENT_CCID)
	uint8_t ccid;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
	struct bt_gatt_subscribe_params name_sub_params;
	struct bt_gatt_discover_params name_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
	struct bt_gatt_subscribe_params technology_sub_params;
	struct bt_gatt_discover_params technology_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
	struct bt_gatt_subscribe_params signal_strength_sub_params;
	struct bt_gatt_discover_params signal_strength_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
	struct bt_gatt_subscribe_params current_calls_sub_params;
	struct bt_gatt_discover_params current_calls_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */
#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
	struct bt_gatt_subscribe_params status_flags_sub_params;
	struct bt_gatt_discover_params status_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
	struct bt_gatt_subscribe_params in_target_uri_sub_params;
	struct bt_gatt_discover_params in_target_uri_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */
#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
	struct bt_gatt_subscribe_params call_cp_sub_params;
	struct bt_gatt_discover_params call_cp_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */
#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
	struct bt_gatt_subscribe_params friendly_name_sub_params;
	struct bt_gatt_discover_params friendly_name_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
	struct bt_gatt_subscribe_params incoming_call_sub_params;
	struct bt_gatt_discover_params incoming_call_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */
	struct bt_gatt_subscribe_params call_state_sub_params;
	struct bt_gatt_discover_params call_state_sub_disc_params;
	struct bt_gatt_subscribe_params termination_sub_params;
	struct bt_gatt_discover_params termination_sub_disc_params;
	struct bt_gatt_read_params read_params;
	uint8_t read_buf[BT_TBS_CLIENT_INST_READ_BUF_SIZE];
	struct net_buf_simple net_buf;
};
#endif /* CONFIG_BT_TBS_CLIENT */
