/** @file
 *  @brief APIs for Bluetooth Coordinated Set Identification Profile (CSIP)
 * client.
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include "csis.h"

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CSIP_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CSIP_

#define BT_CSIP_SET_SIRK_SIZE 16

#define BT_CSIP_ERROR_LOCK_DENIED               0x80
#define BT_CSIP_ERROR_LOCK_RELEASE_DENIED       0x81
#define BT_CSIP_ERROR_LOCK_INVAL_VALUE          0x82
#define BT_CSIP_ERROR_SIRK_ACCESS_REJECTED      0x83
#define BT_CSIP_ERROR_SIRK_OOB_ONLY             0x84
#define BT_CSIP_ERROR_LOCK_ALREADY_GRANTED      0x85

#define BT_CSIP_RELEASE_VALUE                   0x01
#define BT_CSIP_LOCK_VALUE                      0x02

struct bt_csip_set_t {
	uint8_t set_sirk[BT_CSIP_SET_SIRK_SIZE];
	uint8_t set_size;
};

struct bt_csip_set_member_t {
	bt_addr_le_t addr;
};

typedef void (*bt_csip_discover_cb_t)(struct bt_conn *conn, int err,
				      uint8_t set_count);

/**
 * @brief Initialise the CSIP instance for a connection. This will do a
 * discovery on the device and prepare the instance for following commands.
 *
 * @param conn The connection to the device that has a CSIS instance
 * @param subscribe Bool to control whether or not to subscribe to
 *                  characteristics if possible
 *
 * @return int Return 0 on success, or an ERRNO value on error.
 */
int bt_csip_discover(struct bt_conn *conn, bool subscribe);

typedef void (*bt_csip_discover_sets_cb_t)(struct bt_conn *conn,
					   int err, uint8_t set_count,
					   struct bt_csip_set_t *sets);

/**
 * @brief Reads CSIS characteristics from a device, to find more information
 * about the set(s) that the device is part of.
 *
 * @param conn The connection to the device to read CSIS characteristics
 *
 * @return int Return 0 on success, or an ERRNO value on error.
 */
int bt_csip_discover_sets(struct bt_conn *conn);

typedef void (*bt_csip_discover_members_cb_t)(
	int err, uint8_t set_size, uint8_t members_found);

/**
 * @brief Start scanning for all devices that are part of a set.
 *
 * @param set The set to find devices for
 *
 * @return int Return 0 on success, or an ERRNO value on error.
 */
int bt_csip_discover_members(struct bt_csip_set_t *set);

typedef void (*bt_csip_lock_set_cb_t)(int err);

/**
 * @brief Callback when the lock value on a set of a connected device changes.
 *
 * @param conn    Connection of the CSIS server.
 * @param set     The set that was changed.
 * @param locked  Whether the lock is locked or release.
 *
 * @return int Return 0 on success, or an ERRNO value on error.
 */
typedef void (*bt_csip_lock_changed_cb_t)(struct bt_conn *conn,
					  struct bt_csip_set_t *set,
					  bool locked);

struct bt_csip_cb_t {
	bt_csip_lock_set_cb_t             lock_set;
	bt_csip_lock_set_cb_t             release_set;
	bt_csip_discover_members_cb_t     members;
	bt_csip_discover_sets_cb_t        sets;
	bt_csip_discover_cb_t             discover;
	bt_csip_lock_changed_cb_t         locked;
};

/** @brief Lock the set
 *
 *  Connect to and set the lock for all devices in a set.
 *
 *  @return Return 0 on success, or an ERRNO value on error.
 */
int bt_csip_lock_set(void);

/**
 * @brief Connect to and release the lock for all devices in a set
 *
 * @return int Return 0 on success, or an ERRNO value on error.
 */
int bt_csip_release_set(void);

/**
 * @brief Disconnect all connected set members
 *
 * @return int Return 0 on success, or an ERRNO value on error.
 */
int bt_csip_disconnect(void);

/** @brief Registers callbacks for CSIP.
 *
 *  @param cb   Pointer to the callback structure.
 */
void bt_csip_register_cb(struct bt_csip_cb_t *cb);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CSIP_ */
