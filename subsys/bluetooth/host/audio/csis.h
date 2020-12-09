/**
 * @file
 * @brief APIs for Bluetooth CSIS
 *
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CSIS_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CSIS_
#include <zephyr/types.h>
#include <stdbool.h>
#include <bluetooth/conn.h>

#define BT_CSIS_PSRI_SIZE			6

struct bt_csis_cb_t {
	/** @brief Callback whenever the lock changes on the server.
	 *
	 *  @param conn    The connection to the client that changed the lock.
	 *                 NULL if server changed it, either by calling
	 *                 @ref csis_lock or by timeout.
	 *  @param locked  Whether the lock was locked or released.
	 *
	 */
	void (*locked)(struct bt_conn *conn, bool locked);
};

/**
 * @brief Print the sirk to the debug output
 */
void bt_csis_print_sirk(void);

/**
 * @brief Starts advertising the PRSI value.
 *
 * @param enable	If true start advertising, if false stop advertising
 * @return int		0 if on success, ERRNO on error.
 */
int bt_csis_advertise(bool enable);

/** @brief Locks the sets on the server.
 *
 *  @param lock	   If true lock the set, if false release the set.
 *  @param force   This argument only have meaning when @p lock is false
 *                 (release) and will force release the lock, regardless of who
 *                 took the lock.
 *
 *  @return 0 on success, GATT error on error.
 */
int bt_csis_lock(bool lock, bool force);

/** @brief Registers callbacks for CSIS.
 *
 *  @param cb   Pointer to the callback structure.
 */
void bt_csis_register_cb(struct bt_csis_cb_t *cb);

/**
 * @brief Test function to dynamically set rank during runtime.
 *
 * @param rank The rank to set.
 */
int bt_csis_test_set_rank(uint8_t rank);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CSIS_ */
