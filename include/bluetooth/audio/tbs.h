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

#include <zephyr/types.h>
#include <bluetooth/conn.h>

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

#if defined(CONFIG_BT_DEBUG_TBS)
/** @brief Prints all calls of all services to the debug log */
void bt_tbs_dbg_print_calls(void);
#endif /* defined(CONFIG_BT_DEBUG_TBS) */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TBS_H_ */
