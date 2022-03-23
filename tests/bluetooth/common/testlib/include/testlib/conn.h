/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_H_

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

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_H_ */
