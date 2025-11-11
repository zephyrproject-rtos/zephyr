/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_H_

#include <stdint.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>

/**
 * @brief Scan and connect using address
 *
 * Synchronous: Blocks until the connection procedure completes.
 * Thread-safe:
 *
 * This is a synchronous wrapper around @ref bt_conn_le_create with
 * default params. It will wait until the @ref bt_conn_cb.connected
 * event and return the HCI status of the connection creation.
 *
 * The reference created by @ref bt_conn_le_create is put in @p connp.
 *
 * @note The connection reference presists if the connection procedure
 * fails at a later point. @p connp is a reified reference: if it's not
 * NULL, then it's a valid reference.
 *
 * @remark Not disposing of the connection reference in the case of
 * connection failure is intentional. It's useful for comparing against
 * raw @ref bt_conn_cb.connected events.
 *
 * @note The reference variable @p connp is required be empty (i.e.
 * NULL) on entry.
 *
 * @param peer Peer address.
 * @param[out] conn Connection reference variable.
 *
 * @retval 0 Connection was enstablished.
 * @retval -errno @ref bt_conn_le_create error. No connection object
 * reference was created.
 * @retval BT_HCI_ERR HCI reason for connection failure. A connection
 * object reference was created and put in @p connp.
 *
 */
int bt_testlib_connect(const bt_addr_le_t *peer, struct bt_conn **connp);

/**
 * @brief Disconnect, wait and dispose of reference.
 *
 * The disconnect reason for a normal disconnect should be @ref
 * BT_HCI_ERR_REMOTE_USER_TERM_CONN.
 *
 * The following disconnect reasons are allowed:
 *
 *  - @ref BT_HCI_ERR_AUTH_FAIL
 *  - @ref BT_HCI_ERR_REMOTE_USER_TERM_CONN
 *  - @ref BT_HCI_ERR_REMOTE_LOW_RESOURCES
 *  - @ref BT_HCI_ERR_REMOTE_POWER_OFF
 *  - @ref BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
 *  - @ref BT_HCI_ERR_PAIRING_NOT_SUPPORTED
 *  - @ref BT_HCI_ERR_UNACCEPT_CONN_PARAM
 */
int bt_testlib_disconnect(struct bt_conn **connp, uint8_t reason);

/**
 * @brief Wait for connected state
 *
 * Thread-safe.
 *
 * @attention This function does not look at the history of a connection
 * object. If it's already disconnected after a connection, then this
 * function will wait forever. Don't use this function if you cannot
 * guarantee that any disconnection event comes after this function is
 * called. This is only truly safe in a simulated environment.
 */
int bt_testlib_wait_connected(struct bt_conn *conn);

/**
 * @brief Wait for disconnected state
 *
 * Thread-safe.
 */
int bt_testlib_wait_disconnected(struct bt_conn *conn);

/**
 * @brief Dispose of a reified connection reference
 *
 * Thread-safe.
 *
 * Atomically swaps NULL into the reference variable @p connp, then
 * moves the reference into @ref bt_conn_unref.
 */
void bt_testlib_conn_unref(struct bt_conn **connp);

/** @brief Obtain a reference to a connection object by its
 *  index
 *
 *  This is an inverse of @ref bt_conn_index during the lifetime
 *  of the original  @ref bt_conn reference.
 *
 *  This function can be used instead of @ref bt_conn_foreach to
 *  loop over all connections.
 *
 *  @note The ranges of conn_index overlap for different
 *  connection types. They all range from 0 up to their
 *  respective capacities, tabulated below.
 *
 *    @p conn_type     | Capacity
 *   ------------------|------------------------
 *    BT_CONN_TYPE_LE  | CONFIG_BT_MAX_CONN
 *    BT_CONN_TYPE_SCO | CONFIG_BT_MAX_SCO_CONN
 *    BT_CONN_TYPE_ISO | CONFIG_BT_ISO_MAX_CHAN
 *
 * Internal details that cannot be relied on:
 *  - The memory addresses returned point into an array.
 *  - The same index and type always yields the same memory
 *    address.
 *
 * Guarantees:
 *  - Looping over the whole range is equivalent to @ref
 *    bt_conn_foreach.
 *
 *  @retval NULL if the reference is dead
 *  @retval conn a reference to the found connection object
 */
struct bt_conn *bt_testlib_conn_unindex(enum bt_conn_type conn_type, uint8_t conn_index);

/**
 * @brief Wait until there is a free connection slot
 *
 * Thread-safe.
 *
 * Returns when there already is a free connection slot or a
 * connection slot is recycled.
 *
 * @note The free connection slots may have been taken by the
 * time this function returns. Call this function in a loop if
 * needed.
 */
void bt_testlib_conn_wait_free(void);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_H_ */
