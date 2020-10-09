/** @file
 *  @brief Public APIs for Bluetooth TBS.
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TBS_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TBS_
#include <zephyr/types.h>
#include <bluetooth/conn.h>

/**
 * @brief Callback function for client originating a call.
 *
 * @param conn          The connection used.
 * @param call_index    The call index.
 * @param uri           The URI. The value may change, so should be
 *                      copied if persistence is wanted.
 *
 *  return true if the call request was accepted and remote party is alerted.
 */
typedef bool (*bt_tbs_originate_call_cb_t)(
	struct bt_conn *conn, uint8_t call_index, const char *uri);

/**
 * @brief Callback function for terminating a call.
 *
 * The call may be either terminated by the client or the server.
 *
 * @param conn          The connection used.
 * @param call_index    The call index.
 * @param reason        The termination BT_CCP_REASON_* reason.
 */
typedef void (*bt_tbs_terminate_call_cb_t)(
	struct bt_conn *conn, uint8_t call_index, uint8_t reason);

/**
 * @brief Callback function for joining calls.
 *
 * @param conn                  The connection used.
 * @param call_index_count      The number of call indexes to join.
 * @param call_indexes          The call indexes.
 */
typedef void (*bt_tbs_join_calls_cb_t)(struct bt_conn *conn,
				       uint8_t call_index_count,
				       const uint8_t *call_indexes);

/**
 * @brief Callback function for client request call state change
 *
 * @param conn          The connection used.
 * @param call_index    The call index.
 */
typedef void (*bt_tbs_call_change_cb_t)(
	const struct bt_conn *conn, uint8_t call_index);

/** @brief Callback function for authorizing a client.
 *
 *  Only used if BT_TBS_AUTHORIZATION is enabled.
 *
 *  @param conn         The connection used.
 */
typedef bool (*bt_tbs_authorize_cb_t)(const struct bt_conn *conn);

struct bt_tbs_cb_t {
	bt_tbs_originate_call_cb_t      originate_call;
	bt_tbs_terminate_call_cb_t      terminate_call;
	bt_tbs_call_change_cb_t         hold_call;
	bt_tbs_call_change_cb_t         accept_call;
	bt_tbs_call_change_cb_t         retrieve_call;
	bt_tbs_join_calls_cb_t          join_calls;
	bt_tbs_authorize_cb_t           authorize;
};

/**
 * @brief Accept an alerting call.
 *
 * @param call_index    The index of the call that will be accepted.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_accept(uint8_t call_index);

/**
 * @brief Hold a call.
 *
 * @param call_index    The index of the call that will be held.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_hold(uint8_t call_index);

/**
 * @brief Retrieve a call.
 *
 * @param call_index    The index of the call that will be retrieved.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_retrieve(uint8_t call_index);

/**
 * @brief Terminate a call.
 *
 * @param call_index    The index of the call that will be terminated.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_terminate(uint8_t call_index);

/**
 * @brief Originate a call
 *
 * @param bearer_index  The index of the Telephone Bearer.
 * @param uri           The remote URI.
 * @param call_index    Pointer to a value where the new call_index will be
 *                      stored.
 * @return int          A call index on success (positive value),
 *                      ERRNO value on fail.
 */
int bt_tbs_originate(uint8_t bearer_index, char *uri, uint8_t *call_index);

/**
 * @brief Join calls
 *
 * @param call_index_cnt   The number of call indexes to join
 * @param call_indexes     Array of call indexes to join.
 * @return int             BT_CCP_RESULT_CODE_* if positive,
 *                         ERRNO value if negative.
 */
int bt_tbs_join(uint8_t call_index_cnt, uint8_t *call_indexes);

/**
 * @brief Notify the server that the remote party answered the call.
 *
 * @param call_index    The index of the call that was answered.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_remote_answer(uint8_t call_index);

/**
 * @brief Notify the server that the remote party held the call.
 *
 * @param call_index    The index of the call that was held.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_remote_hold(uint8_t call_index);

/**
 * @brief Notify the server that the remote party retrieved the call.
 *
 * @param call_index    The index of the call that was retrieved.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_remote_retrieve(uint8_t call_index);

/**
 * @brief Notify the server that the remote party terminated the call.
 *
 * @param call_index    The index of the call that was terminated.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_remote_terminate(uint8_t call_index);

/**
 * @brief Notify the server of an incoming call.
 *
 * @param bearer_index    The index of the Telephone Bearer.
 * @param to              The URI that is receiving the call.
 * @param from            The URI of the remote caller.
 * @param friendly_name   The  friendly name of the remote caller.
 * @return int            New call index if positive,
 *                        ERRNO value if negative.
 */
int bt_tbs_remote_incoming(uint8_t bearer_index, const char *to,
			   const char *from, const char *friendly_name);

/**
 * @brief Set a new bearer provider.
 *
 * @param bearer_index  The index of the Telephone Bearer or BT_CCP_GTBS_INDEX
 *                      for GTBS.
 * @param name          The new bearer provider name.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_set_bearer_provider_name(uint8_t bearer_index, const char *name);

/**
 * @brief Set a new bearer technology.
 *
 * @param bearer_index  The index of the Telephone Bearer or BT_CCP_GTBS_INDEX
 *                      for GTBS.
 * @param name          The new bearer technology.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_set_bearer_technology(uint8_t bearer_index, uint8_t new_technology);

/**
 * @brief Update the signal strength reported by the server.
 *
 * @param bearer_index  The index of the Telephone Bearer or BT_CCP_GTBS_INDEX
 *                      for GTBS.
 * @param name          The new signal strength.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_set_signal_strength(uint8_t bearer_index,
			       uint8_t new_signal_strength);

/**
 * @brief Sets the feature and status value.
 *
 * @param bearer_index  The index of the Telephone Bearer or BT_CCP_GTBS_INDEX
 *                      for GTBS.
 * @param status_flags  The new feature and status value.
 * @return int          BT_CCP_RESULT_CODE_* if positive,
 *                      ERRNO value if negative.
 */
int bt_tbs_set_status_flags(uint8_t bearer_index, uint16_t status_flags);

/** @brief Sets the URI scheme list of a bearer.
 *
 *  @param bearer_index  The index of the Telephone Bearer.
 *  @param uri_list      List of uri prefixes (e.g. {"skype", "tel"}).
 *
 *  @return BT_CCP_RESULT_CODE_* if positive, ERRNO value if negative.
 */
int bt_tbs_set_uri_scheme_list(uint8_t bearer_idx, const char **uri_list,
			       uint8_t uri_count);
/**
 * @brief Register the callbacks for TBS.
 *
 * @param cb Pointer to the callback structure.
 */
void bt_tbs_register_cb(struct bt_tbs_cb_t *cbs);

#if defined(CONFIG_BT_DEBUG_TBS)
/** @brief Prints all calls of all services to the debug log */
void bt_tbs_dbg_print_calls(void);
#endif /* defined(CONFIG_BT_DEBUG_TBS) */


#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_TBS_ */
