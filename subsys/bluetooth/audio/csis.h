/**
 * @file
 * @brief APIs for Bluetooth CSIS
 *
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_CSIS_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_CSIS_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include <bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_CSIS_MINIMUM_SET_SIZE                2
#define BT_CSIS_PSRI_SIZE                       6

/** Accept the request to read the SIRK as plaintext */
#define BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT        0x00
/** Accept the request to read the SIRK, but return encrypted SIRK */
#define BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC    0x01
/** Reject the request to read the SIRK */
#define BT_CSIS_READ_SIRK_REQ_RSP_REJECT        0x02
/** SIRK is available only via an OOB procedure */
#define BT_CSIS_READ_SIRK_REQ_RSP_OOB_ONLY      0x03

#define BT_CSIS_SET_SIRK_SIZE 16

#define BT_CSIS_ERROR_LOCK_DENIED               0x80
#define BT_CSIS_ERROR_LOCK_RELEASE_DENIED       0x81
#define BT_CSIS_ERROR_LOCK_INVAL_VALUE          0x82
#define BT_CSIS_ERROR_SIRK_ACCESS_REJECTED      0x83
#define BT_CSIS_ERROR_SIRK_OOB_ONLY             0x84
#define BT_CSIS_ERROR_LOCK_ALREADY_GRANTED      0x85

#define BT_CSIS_RELEASE_VALUE                   0x01
#define BT_CSIS_LOCK_VALUE                      0x02

#define BT_CSIS_SIRK_TYPE_ENCRYPTED             0x00
#define BT_CSIS_SIRK_TYPE_PLAIN                 0x01

/** @brief Opaque Coordinated Set Identification Service instance. */
struct bt_csis;

struct bt_csis_cb {
	/**
	 * @brief Callback whenever the lock changes on the server.
	 *
	 * @param conn    The connection to the client that changed the lock.
	 *                NULL if server changed it, either by calling
	 *                @ref csis_lock or by timeout.
	 * @param csis    Pointer to the Coordinated Set Identification Service.
	 * @param locked  Whether the lock was locked or released.
	 *
	 */
	void (*lock_changed)(struct bt_conn *conn, struct bt_csis *csis,
			     bool locked);

	/**
	 * @brief Request from a peer device to read the sirk.
	 *
	 * If this callback is not set, all clients will be allowed to read
	 * the SIRK unencrypted.
	 *
	 * @param conn The connection to the client that requested to read the
	 *             SIRK.
	 * @param csis Pointer to the Coordinated Set Identification Service.
	 *
	 * @return A BT_CSIS_READ_SIRK_REQ_RSP_* response code.
	 */
	uint8_t (*sirk_read_req)(struct bt_conn *conn, struct bt_csis *csis);
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
	uint8_t set_sirk[BT_CSIS_SET_SIRK_SIZE];

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
	struct bt_csis_cb *cb;
};

/**
 * @brief Get the service declaration attribute.
 *
 * The first service attribute can be included in any other GATT service.
 *
 * @param csis   Pointer to the Coordinated Set Identification Service.
 *
 * @return The first CSIS attribute instance.
 */
void *bt_csis_svc_decl_get(const struct bt_csis *csis);

/**
 * @brief Register the Coordinated Set Identification Service.
 *
 * This will register and enable the service and make it discoverable by
 * clients.
 *
 * This shall only be done as a server.
 *
 * @param param      Coordinated Set Identification Service register parameters.
 * @param[out] csis  Pointer to the registered Coordinated Set Identification
 *                   Service.
 *
 * @return 0 if success, errno on failure.
 */
int bt_csis_register(const struct bt_csis_register_param *param,
		     struct bt_csis **csis);

/**
 * @brief Print the sirk to the debug output
 *
 * @param csis   Pointer to the Coordinated Set Identification Service.
 */
void bt_csis_print_sirk(const struct bt_csis *csis);

/**
 * @brief Starts advertising the PRSI value.
 *
 * This cannot be used with other connectable advertising sets.
 *
 * @param csis          Pointer to the Coordinated Set Identification Service.
 * @param enable	If true start advertising, if false stop advertising
 *
 * @return int		0 if on success, ERRNO on error.
 */
int bt_csis_advertise(struct bt_csis *csis, bool enable);

/**
 * @brief Locks the sets on the server.
 *
 * @param csis    Pointer to the Coordinated Set Identification Service.
 * @param lock    If true lock the set, if false release the set.
 * @param force   This argument only have meaning when @p lock is false
 *                (release) and will force release the lock, regardless of who
 *                took the lock.
 *
 * @return 0 on success, GATT error on error.
 */
int bt_csis_lock(struct bt_csis *csis, bool lock, bool force);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_CSIS_H_ */
