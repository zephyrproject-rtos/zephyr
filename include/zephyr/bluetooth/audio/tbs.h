/** @file
 *  @brief Public APIs for Bluetooth Telephone Bearer Service.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TBS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TBS_H_

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/bluetooth/conn.h>

/* Call States */
#define BT_TBS_CALL_STATE_INCOMING                      0x00
#define BT_TBS_CALL_STATE_DIALING                       0x01
#define BT_TBS_CALL_STATE_ALERTING                      0x02
#define BT_TBS_CALL_STATE_ACTIVE                        0x03
#define BT_TBS_CALL_STATE_LOCALLY_HELD                  0x04
#define BT_TBS_CALL_STATE_REMOTELY_HELD                 0x05
#define BT_TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD     0x06

/* Terminate Reason */
#define BT_TBS_REASON_BAD_REMOTE_URI                    0x00
#define BT_TBS_REASON_CALL_FAILED                       0x01
#define BT_TBS_REASON_REMOTE_ENDED_CALL                 0x02
#define BT_TBS_REASON_SERVER_ENDED_CALL                 0x03
#define BT_TBS_REASON_LINE_BUSY                         0x04
#define BT_TBS_REASON_NETWORK_CONGESTED                 0x05
#define BT_TBS_REASON_CLIENT_TERMINATED                 0x06
#define BT_TBS_REASON_UNSPECIFIED                       0x07

/* Application error codes */
#define BT_TBS_RESULT_CODE_SUCCESS                      0x00
#define BT_TBS_RESULT_CODE_OPCODE_NOT_SUPPORTED         0x01
#define BT_TBS_RESULT_CODE_OPERATION_NOT_POSSIBLE       0x02
#define BT_TBS_RESULT_CODE_INVALID_CALL_INDEX           0x03
#define BT_TBS_RESULT_CODE_STATE_MISMATCH               0x04
#define BT_TBS_RESULT_CODE_OUT_OF_RESOURCES             0x05
#define BT_TBS_RESULT_CODE_INVALID_URI                  0x06

#define BT_TBS_FEATURE_HOLD                             BIT(0)
#define BT_TBS_FEATURE_JOIN                             BIT(1)

#define BT_TBS_CALL_FLAG_SET_INCOMING(flag)            (flag &= ~BIT(0))
#define BT_TBS_CALL_FLAG_SET_OUTGOING(flag)            (flag |= BIT(0))

#define BT_TBS_SIGNAL_STRENGTH_NO_SERVICE               0
#define BT_TBS_SIGNAL_STRENGTH_MAX                      100
#define BT_TBS_SIGNAL_STRENGTH_UNKNOWN                  255

/* Bearer Technology */
#define BT_TBS_TECHNOLOGY_3G                       0x01
#define BT_TBS_TECHNOLOGY_4G                       0x02
#define BT_TBS_TECHNOLOGY_LTE                      0x03
#define BT_TBS_TECHNOLOGY_WIFI                     0x04
#define BT_TBS_TECHNOLOGY_5G                       0x05
#define BT_TBS_TECHNOLOGY_GSM                      0x06
#define BT_TBS_TECHNOLOGY_CDMA                     0x07
#define BT_TBS_TECHNOLOGY_2G                       0x08
#define BT_TBS_TECHNOLOGY_WCDMA                    0x09
#define BT_TBS_TECHNOLOGY_IP                       0x0a

/**
 * @brief The GTBS index denotes whenever a callback is from a
 * Generic Telephone Bearer Service (GTBS) instance, or
 * whenever the client should perform on action on the GTBS instance of the
 * server, rather than any of the specific Telephone Bearer Service instances.
 */
#define BT_TBS_GTBS_INDEX                               0xFF

/** @brief Opaque Telephone Bearer Service instance. */
struct bt_tbs_instance;

/**
 * @brief Callback function for client originating a call.
 *
 * @param conn          The connection used.
 * @param call_index    The call index.
 * @param uri           The URI. The value may change, so should be
 *                      copied if persistence is wanted.
 *
 * @return true if the call request was accepted and remote party is alerted.
 */
typedef bool (*bt_tbs_originate_call_cb)(struct bt_conn *conn,
					 uint8_t call_index,
					 const char *uri);

/**
 * @brief Callback function for client terminating a call.
 *
 * The call may be either terminated by the client or the server.
 *
 * @param conn          The connection used.
 * @param call_index    The call index.
 * @param reason        The termination BT_TBS_REASON_* reason.
 */
typedef void (*bt_tbs_terminate_call_cb)(struct bt_conn *conn,
					 uint8_t call_index,
					 uint8_t reason);

/**
 * @brief Callback function for client joining calls.
 *
 * @param conn                  The connection used.
 * @param call_index_count      The number of call indexes to join.
 * @param call_indexes          The call indexes.
 */
typedef void (*bt_tbs_join_calls_cb)(struct bt_conn *conn,
				     uint8_t call_index_count,
				     const uint8_t *call_indexes);

/**
 * @brief Callback function for client request call state change
 *
 * @param conn          The connection used.
 * @param call_index    The call index.
 */
typedef void (*bt_tbs_call_change_cb)(struct bt_conn *conn,
				      uint8_t call_index);

/**
 * @brief Callback function for authorizing a client.
 *
 * Only used if BT_TBS_AUTHORIZATION is enabled.
 *
 * @param conn         The connection used.
 *
 * @return true if authorized, false otherwise
 */
typedef bool (*bt_tbs_authorize_cb)(struct bt_conn *conn);

struct bt_tbs_cb {
	bt_tbs_originate_call_cb      originate_call;
	bt_tbs_terminate_call_cb      terminate_call;
	bt_tbs_call_change_cb         hold_call;
	bt_tbs_call_change_cb         accept_call;
	bt_tbs_call_change_cb         retrieve_call;
	bt_tbs_join_calls_cb          join_calls;
	bt_tbs_authorize_cb           authorize;
};

/**
 * @brief Accept an alerting call.
 *
 * @param call_index    The index of the call that will be accepted.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_accept(uint8_t call_index);

/**
 * @brief Hold a call.
 *
 * @param call_index    The index of the call that will be held.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_hold(uint8_t call_index);

/**
 * @brief Retrieve a call.
 *
 * @param call_index    The index of the call that will be retrieved.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_retrieve(uint8_t call_index);

/**
 * @brief Terminate a call.
 *
 * @param call_index    The index of the call that will be terminated.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_terminate(uint8_t call_index);

/**
 * @brief Originate a call
 *
 * @param[in]  bearer_index  The index of the Telephone Bearer.
 * @param[in]  uri           The remote URI.
 * @param[out] call_index    Pointer to a value where the new call_index will be
 *                           stored.
 *
 * @return int          A call index on success (positive value),
 *                      errno value on fail.
 */
int bt_tbs_originate(uint8_t bearer_index, char *uri, uint8_t *call_index);

/**
 * @brief Join calls
 *
 * @param call_index_cnt   The number of call indexes to join
 * @param call_indexes     Array of call indexes to join.
 *
 * @return int             BT_TBS_RESULT_CODE_* if positive or 0,
 *                         errno value if negative.
 */
int bt_tbs_join(uint8_t call_index_cnt, uint8_t *call_indexes);

/**
 * @brief Notify the server that the remote party answered the call.
 *
 * @param call_index    The index of the call that was answered.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_remote_answer(uint8_t call_index);

/**
 * @brief Notify the server that the remote party held the call.
 *
 * @param call_index    The index of the call that was held.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_remote_hold(uint8_t call_index);

/**
 * @brief Notify the server that the remote party retrieved the call.
 *
 * @param call_index    The index of the call that was retrieved.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_remote_retrieve(uint8_t call_index);

/**
 * @brief Notify the server that the remote party terminated the call.
 *
 * @param call_index    The index of the call that was terminated.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_remote_terminate(uint8_t call_index);

/**
 * @brief Notify the server of an incoming call.
 *
 * @param bearer_index    The index of the Telephone Bearer.
 * @param to              The URI that is receiving the call.
 * @param from            The URI of the remote caller.
 * @param friendly_name   The  friendly name of the remote caller.
 *
 * @return int            New call index if positive or 0,
 *                        errno value if negative.
 */
int bt_tbs_remote_incoming(uint8_t bearer_index, const char *to,
			   const char *from, const char *friendly_name);

/**
 * @brief Set a new bearer provider.
 *
 * @param bearer_index  The index of the Telephone Bearer or BT_TBS_GTBS_INDEX
 *                      for GTBS.
 * @param name          The new bearer provider name.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_set_bearer_provider_name(uint8_t bearer_index, const char *name);

/**
 * @brief Set a new bearer technology.
 *
 * @param bearer_index   The index of the Telephone Bearer or BT_TBS_GTBS_INDEX
 *                       for GTBS.
 * @param new_technology The new bearer technology.
 *
 * @return int           BT_TBS_RESULT_CODE_* if positive or 0,
 *                       errno value if negative.
 */
int bt_tbs_set_bearer_technology(uint8_t bearer_index, uint8_t new_technology);

/**
 * @brief Update the signal strength reported by the server.
 *
 * @param bearer_index        The index of the Telephone Bearer or
 *                            BT_TBS_GTBS_INDEX for GTBS.
 * @param new_signal_strength The new signal strength.
 *
 * @return int                BT_TBS_RESULT_CODE_* if positive or 0,
 *                            errno value if negative.
 */
int bt_tbs_set_signal_strength(uint8_t bearer_index,
			       uint8_t new_signal_strength);

/**
 * @brief Sets the feature and status value.
 *
 * @param bearer_index  The index of the Telephone Bearer or BT_TBS_GTBS_INDEX
 *                      for GTBS.
 * @param status_flags  The new feature and status value.
 *
 * @return int          BT_TBS_RESULT_CODE_* if positive or 0,
 *                      errno value if negative.
 */
int bt_tbs_set_status_flags(uint8_t bearer_index, uint16_t status_flags);

/** @brief Sets the URI scheme list of a bearer.
 *
 *  @param bearer_index  The index of the Telephone Bearer.
 *  @param uri_list      List of URI prefixes (e.g. {"skype", "tel"}).
 *  @param uri_count     Number of URI prefixies in @p uri_list.
 *
 *  @return BT_TBS_RESULT_CODE_* if positive or 0, errno value if negative.
 */
int bt_tbs_set_uri_scheme_list(uint8_t bearer_index, const char **uri_list,
			       uint8_t uri_count);
/**
 * @brief Register the callbacks for TBS.
 *
 * @param cbs Pointer to the callback structure.
 */
void bt_tbs_register_cb(struct bt_tbs_cb *cbs);

/** @brief Prints all calls of all services to the debug log */
void bt_tbs_dbg_print_calls(void);

struct bt_tbs_client_call_state {
	uint8_t index;
	uint8_t state;
	uint8_t flags;
} __packed;

struct bt_tbs_client_call {
	struct bt_tbs_client_call_state call_info;
	char *remote_uri;
};

/**
 * @brief Callback function for ccp_discover.
 *
 * @param conn          The connection that was used to discover CCP for a
 *                      device.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param tbs_count     Number of TBS instances on peer device.
 * @param gtbs_found    Whether or not the server has a Generic TBS instance.
 */
typedef void (*bt_tbs_client_discover_cb)(struct bt_conn *conn, int err,
					  uint8_t tbs_count, bool gtbs_found);

/**
 * @brief Callback function for writing values to peer device.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 */
typedef void (*bt_tbs_client_write_value_cb)(struct bt_conn *conn, int err,
					     uint8_t inst_index);

/**
 * @brief Callback function for the CCP call control functions.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index. For #bt_tbs_client_originate_call this will
 *                      always be 0, and does not reflect the actual call index.
 */
typedef void (*bt_tbs_client_cp_cb)(struct bt_conn *conn, int err,
				    uint8_t inst_index, uint8_t call_index);

/**
 * @brief Callback function for functions that read a string value.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param value         The Null-terminated string value. The value is not kept
 *                      by the client, so must be copied to be saved.
 */
typedef void (*bt_tbs_client_read_string_cb)(struct bt_conn *conn, int err,
					     uint8_t inst_index,
					     const char *value);

/**
 * @brief Callback function for functions that read an integer value.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param value         The integer value.
 */
typedef void (*bt_tbs_client_read_value_cb)(struct bt_conn *conn, int err,
					    uint8_t inst_index, uint32_t value);

/**
 * @brief Callback function for ccp_read_termination_reason.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index.
 * @param reason        The termination reason.
 */
typedef void (*bt_tbs_client_termination_reason_cb)(struct bt_conn *conn,
						    int err, uint8_t inst_index,
						    uint8_t call_index,
						    uint8_t reason);

/**
 * @brief Callback function for ccp_read_current_calls.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_count    Number of calls read.
 * @param calls         Array of calls. The array is not kept by
 *                      the client, so must be copied to be saved.
 */
typedef void (*bt_tbs_client_current_calls_cb)(struct bt_conn *conn, int err,
					       uint8_t inst_index,
					       uint8_t call_count,
					       const struct bt_tbs_client_call *calls);

/**
 * @brief Callback function for ccp_read_call_state.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_count    Number of call states read.
 * @param call_states   Array of call states. The array is not kept by
 *                      the client, so must be copied to be saved.
 */
typedef void (*bt_tbs_client_call_states_cb)(struct bt_conn *conn, int err,
					     uint8_t inst_index,
					     uint8_t call_count,
					     const struct bt_tbs_client_call_state *call_states);

struct bt_tbs_client_cb {
	bt_tbs_client_discover_cb            discover;
#if defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL)
	bt_tbs_client_cp_cb                  originate_call;
#endif /* defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL)
	bt_tbs_client_cp_cb                  terminate_call;
#endif /* defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL)
	bt_tbs_client_cp_cb                  hold_call;
#endif /* defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL)
	bt_tbs_client_cp_cb                  accept_call;
#endif /* defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL)
	bt_tbs_client_cp_cb                  retrieve_call;
#endif /* defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS)
	bt_tbs_client_cp_cb                  join_calls;
#endif /* defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
	bt_tbs_client_read_string_cb         bearer_provider_name;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
	bt_tbs_client_read_string_cb         bearer_uci;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
	bt_tbs_client_read_value_cb          technology;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
	bt_tbs_client_read_string_cb         uri_list;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
	bt_tbs_client_read_value_cb          signal_strength;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */
#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL)
	bt_tbs_client_read_value_cb          signal_interval;
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
	bt_tbs_client_current_calls_cb       current_calls;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */
#if defined(CONFIG_BT_TBS_CLIENT_CCID)
	bt_tbs_client_read_value_cb          ccid;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
	bt_tbs_client_read_string_cb         call_uri;
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */
#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
	bt_tbs_client_read_value_cb          status_flags;
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */
	bt_tbs_client_call_states_cb         call_state;
#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
	bt_tbs_client_read_value_cb          optional_opcodes;
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */
	bt_tbs_client_termination_reason_cb  termination_reason;
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
	bt_tbs_client_read_string_cb         remote_uri;
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
	bt_tbs_client_read_string_cb         friendly_name;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */
};

/**
 * @brief Discover TBS for a connection. This will start a GATT
 * discover and setup handles and subscriptions.
 *
 * @param conn          The connection to discover TBS for.
 *
 * @return int          0 on success, GATT error value on fail.
 */
int bt_tbs_client_discover(struct bt_conn *conn);

/**
 * @brief Set the outgoing URI for a TBS instance on the peer device.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param uri           The Null-terminated URI string.
 *
 * @return int          0 on success, errno value on fail.
 */
int bt_tbs_client_set_outgoing_uri(struct bt_conn *conn, uint8_t inst_index,
				   const char *uri);

/**
 * @brief Set the signal strength reporting interval for a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param interval      The interval to write (0-255 seconds).
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL} must be set
 * for this function to be effective.
 */
int bt_tbs_client_set_signal_strength_interval(struct bt_conn *conn,
					       uint8_t inst_index,
					       uint8_t interval);

/**
 * @brief Request to originate a call.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param uri           The URI of the callee.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL} must be set
 * for this function to be effective.
 */
int bt_tbs_client_originate_call(struct bt_conn *conn, uint8_t inst_index,
				 const char *uri);

/**
 * @brief Request to terminate a call
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_index    The call index to terminate.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_TERMINATE_CALL} must be set
 * for this function to be effective.
 */
int bt_tbs_client_terminate_call(struct bt_conn *conn, uint8_t inst_index,
				 uint8_t call_index);

/**
 * @brief Request to hold a call
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_index    The call index to place on hold.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_HOLD_CALL} must be set
 * for this function to be effective.
 */
int bt_tbs_client_hold_call(struct bt_conn *conn, uint8_t inst_index,
			    uint8_t call_index);

/**
 * @brief Accept an incoming call
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_index    The call index to accept.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_ACCEPT_CALL} must be set
 * for this function to be effective.
 */
int bt_tbs_client_accept_call(struct bt_conn *conn, uint8_t inst_index,
			      uint8_t call_index);

/**
 * @brief Retrieve call from (local) hold.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_index    The call index to retrieve.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL} must be set
 * for this function to be effective.
 */
int bt_tbs_client_retrieve_call(struct bt_conn *conn, uint8_t inst_index,
				uint8_t call_index);

/**
 * @brief Join multiple calls.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 * @param call_indexes  Array of call indexes.
 * @param count         Number of call indexes in the call_indexes array.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_JOIN_CALLS} must be set
 * for this function to be effective.
 */
int bt_tbs_client_join_calls(struct bt_conn *conn, uint8_t inst_index,
			     const uint8_t *call_indexes, uint8_t count);

/**
 * @brief Read the bearer provider name of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_bearer_provider_name(struct bt_conn *conn,
					    uint8_t inst_index);

/**
 * @brief Read the UCI of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_BEARER_UCI} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_bearer_uci(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the technology of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_technology(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the URI schemes list of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return int          0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_uri_list(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the current signal strength of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_signal_strength(struct bt_conn *conn,
				       uint8_t inst_index);

/**
 * @brief Read the signal strength reporting interval of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_signal_interval(struct bt_conn *conn,
				       uint8_t inst_index);

/**
 * @brief Read the list of current calls of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_current_calls(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the content ID of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_CCID} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_ccid(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the call target URI of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_INCOMING_URI} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_call_uri(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the feature and status value of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_STATUS_FLAGS} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_status_flags(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the states of the current calls of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 */
int bt_tbs_client_read_call_state(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the remote URI of a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_INCOMING_CALL} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_remote_uri(struct bt_conn *conn, uint8_t inst_index);

/**
 * @brief Read the friendly name of a call for a TBS instance.
 *
 * @param conn          The connection to the TBS server.
 * @param inst_index    The index of the TBS instance.
 *
 * @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_friendly_name(struct bt_conn *conn, uint8_t inst_index);

/** @brief Read the supported opcode of a TBS instance.
 *
 *  @param conn          The connection to the TBS server.
 *  @param inst_index    The index of the TBS instance.
 *
 *  @return              int 0 on success, errno value on fail.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES} must be set
 * for this function to be effective.
 */
int bt_tbs_client_read_optional_opcodes(struct bt_conn *conn,
					uint8_t inst_index);

/**
 * @brief Register the callbacks for CCP.
 *
 * @param cbs Pointer to the callback structure.
 */
void bt_tbs_client_register_cb(const struct bt_tbs_client_cb *cbs);

/**
 * @brief Look up Telephone Bearer Service instance by CCID
 *
 * @param conn  The connection to the TBS server.
 * @param ccid  The CCID to lookup a service instance for.
 *
 * @return Pointer to a Telephone Bearer Service instance if found else NULL.
 *
 * @note @kconfig{CONFIG_BT_TBS_CLIENT_CCID} must be set
 * for this function to be effective.
 */
struct bt_tbs_instance *bt_tbs_client_get_by_ccid(const struct bt_conn *conn,
						  uint8_t ccid);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TBS_H_ */
