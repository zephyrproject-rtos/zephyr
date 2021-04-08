/** @file
 *  @brief Public APIs for Bluetooth CCP.
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CCP_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CCP_
#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include "tbs.h"

/* Call States */
#define BT_CCP_CALL_STATE_INCOMING                      0x00
#define BT_CCP_CALL_STATE_DIALING                       0x01
#define BT_CCP_CALL_STATE_ALERTING                      0x02
#define BT_CCP_CALL_STATE_ACTIVE                        0x03
#define BT_CCP_CALL_STATE_LOCALLY_HELD                  0x04
#define BT_CCP_CALL_STATE_REMOTELY_HELD                 0x05
#define BT_CCP_CALL_STATE_LOCALLY_AND_REMOTELY_HELD     0x06

/* Terminate Reason */
#define BT_CCP_REASON_BAD_REMOTE_URI                    0x00
#define BT_CCP_REASON_CALL_FAILED                       0x01
#define BT_CCP_REASON_REMOTE_ENDED_CALL                 0x02
#define BT_CCP_REASON_SERVER_ENDED_CALL                 0x03
#define BT_CCP_REASON_LINE_BUSY                         0x04
#define BT_CCP_REASON_NETWORK_CONGESTED                 0x05
#define BT_CCP_REASON_CLIENT_TERMINATED                 0x06
#define BT_CCP_REASON_UNSPECIFIED                       0x07

/* Application error codes */
#define BT_CCP_RESULT_CODE_SUCCESS                      0x00
#define BT_CCP_RESULT_CODE_OPCODE_NOT_SUPPORTED         0x01
#define BT_CCP_RESULT_CODE_OPERATION_NOT_POSSIBLE       0x02
#define BT_CCP_RESULT_CODE_INVALID_CALL_INDEX           0x03
#define BT_CCP_RESULT_CODE_STATE_MISMATCH               0x04
#define BT_CCP_RESULT_CODE_OUT_OF_RESOURCES             0x05
#define BT_CCP_RESULT_CODE_INVALID_URI                  0x06

#define BT_CCP_FEATURE_HOLD                             BIT(0)
#define BT_CCP_FEATURE_JOIN                             BIT(1)

#define BT_CCP_CALL_FLAG_SET_INCOMING(flag)            (flag &= ~BIT(0))
#define BT_CCP_CALL_FLAG_SET_OUTGOING(flag)            (flag |= BIT(0))

#define BT_CCP_SIGNAL_STRENGTH_NO_SERVICE               0
#define BT_CCP_SIGNAL_STRENGTH_MAX                      100
#define BT_CCP_SIGNAL_STRENGTH_UNKNOWN                  255

/**
 * @brief The GTBS index denotes whenever a callback is from a GTBS instance, or
 * whenever the client should perform on action on the GTBS instance of the
 * server, rather than any of the specific TBS instances.
 */
#define BT_CCP_GTBS_INDEX                               0xFF

struct bt_ccp_call_state_t {
	uint8_t index;
	uint8_t state;
	uint8_t flags;
} __packed;

struct bt_ccp_call_t {
	struct bt_ccp_call_state_t call_info;
	char remote_uri[CONFIG_BT_TBS_MAX_URI_LENGTH + 1];
} __packed;

/**
 * @brief Callback function for ccp_discover.
 *
 * @param conn          The connection that was used to discover CCP for a
 *                      device.
 * @param err           Error value. BT_CCP_RESULT_CODE_*,
 *                      GATT error og ERRNO value.
 * @param tbs_count     Number of TBS instances on peer device.
 * @param gtbs_found    Whether or not the server has a Generic TBS instance.
 */
typedef void (*bt_ccp_discover_cb_t)(
	struct bt_conn *conn, int err, uint8_t tbs_count, bool gtbs_found);

/**
 * @brief Callback function for writing values to peer device.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_CCP_RESULT_CODE_*,
 *                      GATT error og ERRNO value.
 * @param inst_index    The index of the TBS instance that was updated.
 */
typedef void (*bt_ccp_write_value_cb_t)(
	struct bt_conn *conn, int err, uint8_t inst_index);

/**
 * @brief Callback function for the CCP call control functions.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_CCP_RESULT_CODE_*,
 *                      GATT error og ERRNO value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index. For #bt_ccp_originate_call this will
 *                      always be 0, and does not reflect the actual call index.
 */
typedef void (*bt_ccp_cp_cb_t)(
	struct bt_conn *conn, int err, uint8_t inst_index, uint8_t call_index);

/**
 * @brief Callback function for functions that read a string value.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_CCP_RESULT_CODE_*,
 *                      GATT error og ERRNO value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param value         The Null-terminated string value. The value is not kept
 *                      by the profile, so must be copied to be saved.
 */
typedef void (*bt_ccp_read_string_cb_t)(
	struct bt_conn *conn, int err, uint8_t inst_index, const char *value);

/**
 * @brief Callback function for functions that read an integer value.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_CCP_RESULT_CODE_*,
 *                      GATT error og ERRNO value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param value         The integer value.
 */
typedef void (*bt_ccp_read_value_cb_t)(
	struct bt_conn *conn, int err, uint8_t inst_index, uint32_t value);

/**
 * @brief Callback function for ccp_read_termination_reason.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_CCP_RESULT_CODE_*,
 *                      GATT error og ERRNO value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index.
 * @param reason        The termination reason.
 */
typedef void (*bt_ccp_termination_reason_cb_t)(
	struct bt_conn *conn, int err, uint8_t inst_index,
	uint8_t call_index, uint8_t reason);

/**
 * @brief Callback function for ccp_read_current_calls.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_CCP_RESULT_CODE_*,
 *                      GATT error og ERRNO value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_count    Number of calls read.
 * @param calls         Array of calls. The array is not kept by
 *                      the profile, so must be copied to be saved.
 */
typedef void (*bt_ccp_current_calls_cb_t)(
	struct bt_conn *conn, int err, uint8_t inst_index, uint8_t call_count,
	const struct bt_ccp_call_t *calls);

/**
 * @brief Callback function for ccp_read_call_state.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_CCP_RESULT_CODE_*,
 *                      GATT error og ERRNO value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_count    Number of call states read.
 * @param calls         Array of call states. The array is not kept by
 *                      the profile, so must be copied to be saved.
 */
typedef void (*bt_ccp_call_states_cb_t)(
	struct bt_conn *conn, int err, uint8_t inst_index, uint8_t call_count,
	const struct bt_ccp_call_state_t *call_states);

struct bt_ccp_cb_t {
	bt_ccp_discover_cb_t discover;
	bt_ccp_cp_cb_t originate_call;
	bt_ccp_cp_cb_t terminate_call;
	bt_ccp_cp_cb_t hold_call;
	bt_ccp_cp_cb_t accept_call;
	bt_ccp_cp_cb_t retrieve_call;
	bt_ccp_cp_cb_t join_calls;

	bt_ccp_read_string_cb_t bearer_provider_name;
	bt_ccp_read_string_cb_t bearer_uci;
	bt_ccp_read_value_cb_t technology;
	bt_ccp_read_string_cb_t uri_list;
	bt_ccp_read_value_cb_t signal_strength;
	bt_ccp_read_value_cb_t signal_interval;
	bt_ccp_current_calls_cb_t current_calls;
	bt_ccp_read_value_cb_t ccid;
	bt_ccp_read_value_cb_t status_flags;
	bt_ccp_read_string_cb_t call_uri;
	bt_ccp_call_states_cb_t call_state;
	bt_ccp_read_value_cb_t optional_opcodes;
	bt_ccp_termination_reason_cb_t termination_reason;
	bt_ccp_read_string_cb_t remote_uri;
	bt_ccp_read_string_cb_t friendly_name;
};

/**
 * @brief Discover TBS for a connection. This will start a GATT
 * discover and setup handles and subscriptions.
 *
 * @param conn          The connection to discover TBS for.
 * @param subscribe     Whether to subscribe to all handles.
 * @return int          0 on success, GATT error value on fail.
 */
int bt_ccp_discover(struct bt_conn *conn, bool subscribe);

/**
 * @brief Set the outgoing URI for a TBS instance on the peer device.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param uri           The Null-terminated uri string.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_set_outgoing_uri(struct bt_conn *conn, uint8_t inst_index,
			    const char *uri);

/**
 * @brief Set the signal strength reporting interval for a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param interval      The interval to write (0-255 seconds).
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_set_signal_strength_interval(struct bt_conn *conn,
					uint8_t inst_index, uint8_t interval);

/**
 * @brief Request to originate a call.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param uri           The URI of the callee.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_originate_call(struct bt_conn *conn, uint8_t inst_index,
			  const char *uri);

/**
 * @brief Request to terminate a call
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_index    The call index to terminate.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_terminate_call(struct bt_conn *conn, uint8_t inst_index,
			  uint8_t call_index);

/**
 * @brief Request to hold a call
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_index    The call index to place on hold.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_hold_call(struct bt_conn *conn, uint8_t inst_index,
		     uint8_t call_index);

/**
 * @brief Accept an incoming call
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_index    The call index to accept.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_accept_call(struct bt_conn *conn, uint8_t inst_index,
		       uint8_t call_index);

/**
 * @brief Retrieve call from (local) hold.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_index    The call index to retrieve.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_retrieve_call(struct bt_conn *conn, uint8_t inst_index,
			 uint8_t call_index);

/**
 * @brief Join multiple calls.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_indexes  Array of call indexes.
 * @param count         Number of call indexes in the call_indexes array.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_join_calls(struct bt_conn *conn, uint8_t inst_index,
		      const uint8_t *call_indexes, uint8_t count);

/**
 * @brief Read the bearer provider name of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_read_bearer_provider_name(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the UCI of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_read_bearer_uci(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the technology of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_read_technology(struct bt_conn *conn, uint8_t inst_index);


/**
 * @brief Read the URI schemes list of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return int          0 on success, ERRNO value on fail.
 */
int bt_ccp_read_uri_list(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the current signal strength of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_signal_strength(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the signal strength reporting interval of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_signal_interval(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the list of current calls of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_current_calls(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the content ID of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_ccid(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the feature and status value of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_status_flags(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the call target URI of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_call_uri(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the states of the current calls of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_call_state(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the remote URI of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_remote_uri(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the friendly name of a call for a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_friendly_name(struct bt_conn *conn, uint8_t inst_index);

/** @brief Read the supported opcode of a TBS instance.
 *
 *  @param conn          The connection to the TBS server.
 *  @param inst_index    The index of the TBS instance.
 *
 *  @return              int 0 on success, ERRNO value on fail.
 */
int bt_ccp_read_optional_opcodes(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Register the callbacks for CCP.
 *
 * @param cb Pointer to the callback structure.
 */
void bt_ccp_register_cb(struct bt_ccp_cb_t *cb);
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CCP_ */
