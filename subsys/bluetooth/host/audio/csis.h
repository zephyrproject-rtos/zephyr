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
#include "csip.h"

#define BT_CSIS_PSRI_SIZE			6

/** Accept the request to read the SIRK as plaintext */
#define BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT        0x00
/** Accept the request to read the SIRK, but return encrypted SIRK */
#define BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC    0x01
/** Reject the request to read the SIRK */
#define BT_CSIS_READ_SIRK_REQ_RSP_REJECT        0x02
/** SIRK is available only via an OOB procedure */
#define BT_CSIS_READ_SIRK_REQ_RSP_OOB_ONLY      0x03

/** @brief Opaque Coordinated Set Identification Service instance. */
struct bt_csis;

struct bt_csis_cb_t {
	/** @brief Callback whenever the lock changes on the server.
	 *
	 *  @param conn    The connection to the client that changed the lock.
	 *                 NULL if server changed it, either by calling
	 *                 @ref csis_lock or by timeout.
	 *  @param locked  Whether the lock was locked or released.
	 *
	 */
	void (*lock_changed)(struct bt_conn *conn, bool locked);

	/** @brief Request from a peer device to read the sirk.
	 *
	 * If this callback is not set, all clients will be allowed to read
	 * the SIRK unencrypted.
	 *
	 * @param conn The connection to the client that requested to read the
	 *             SIRK.
	 * @return A BT_CSIS_READ_SIRK_REQ_RSP_* response code.
	 */
	uint8_t (*sirk_read_req)(struct bt_conn *conn);
};

/** Register structure for Coordinated Set Identification Service */
struct bt_csis_register_param {
	/**
	 * @brief Size of the set.
	 *
	 * If set to 0, the set size characteric won't be initialized.
	 * Otherwise shall be set to minimum 2.
	 */
	uint8_t set_size;

	/**
	 * @brief The unique Set Identity Resolving Key (SIRK)
	 *
	 * This shall be unique between different sets, and shall be the same
	 * for each set member for each set.
	 */
	uint8_t set_sirk[BT_CSIP_SET_SIRK_SIZE];

	/**
	 * @brief Boolean to set whether the set is lockable by clients
	 *
	 * Setting this to false will disable the lock characteristic.
	 */
	bool lockable;

	/**
	 * @brief Rank of this device in this set.
	 *
	 * If the lockable parameter is set to true, this shall be > 0 and
	 * <= to the set_size. If the lockable parameter is set to false, this
	 * may be set to 0 to disable the rank characteristic.
	 */
	uint8_t rank;

	/** Pointer to the callback structure. */
	struct bt_csis_cb_t *cb;
};

/** @brief Get the service declaration attribute.
 *
 *  The first service attribute can be included in any other GATT service.
 *
 *  @return The first CSIS attribute instance.
 */
void *bt_csis_svc_decl_get(void);

/**
 * @brief Register the Coordinated Set Identification Service.
 *
 * This will register and enable the service and make it discoverable by
 * clients.
 *
 * This shall only be done as a server.
 *
 * @param param     Coordinated Set Identification Service register parameters.
 *
 * @return 0 if success, errno on failure.
 */
int bt_csis_register(const struct bt_csis_register_param *param);

/**
 * @brief Print the sirk to the debug output
 */
void bt_csis_print_sirk(void);

/**
 * @brief Starts advertising the PRSI value.
 *
 * This cannot be used with other connectable advertising sets.
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

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CSIS_ */
