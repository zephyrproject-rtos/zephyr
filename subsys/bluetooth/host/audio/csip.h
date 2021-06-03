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

#define BT_CSIP_SIRK_TYPE_ENCRYPTED             0x00
#define BT_CSIP_SIRK_TYPE_PLAIN                 0x01

/* Recommended timer for member discovery */
#define CSIP_DISCOVER_TIMER_VALUE               K_SECONDS(10)

#if defined(CONFIG_BT_CSIP)
#define BT_CSIP_MAX_CSIS_INSTANCES CONFIG_BT_CSIP_MAX_CSIS_INSTANCES
#else
#define BT_CSIP_MAX_CSIS_INSTANCES 0
#endif /* CONFIG_BT_CSIP */

struct bt_csip_set_sirk_t {
	uint8_t type;
	uint8_t value[BT_CSIP_SET_SIRK_SIZE];
} __packed;

struct bt_csip_set_t {
	struct bt_csip_set_sirk_t set_sirk;
	uint8_t set_size;
	uint8_t rank;
};

struct bt_csip_set_member {
	struct bt_conn *conn;
	bt_addr_le_t addr;
	struct bt_csip_set_t sets[BT_CSIP_MAX_CSIS_INSTANCES];
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

/**
 * @brief Callback when the lock value is read on a device.
 *
 * @param conn      Connection of the CSIS server.
 * @param err       Error value. 0 on success, GATT error or ERRNO on fail.
 * @param inst_idx  The index of the CSIS service.
 * @param locked  Whether the lock is locked or release.
 */
typedef void (*bt_csip_lock_read_cb_t)(struct bt_conn *conn, int err,
				       uint8_t inst_idx, bool locked);

/**
 * @brief Callback when the lock value is written to a device.
 *
 * @param conn      Connection of the CSIS server.
 * @param err       Error value. 0 on success, GATT error or ERRNO on fail.
 * @param inst_idx  The index of the CSIS service.
 */
typedef void (*bt_csip_lock_cb_t)(struct bt_conn *conn, int err,
				  uint8_t inst_idx);

/**
 * @brief Callback when the release value is written to a device.
 *
 * @param conn      Connection of the CSIS server.
 * @param err       Error value. 0 on success, GATT error or ERRNO on fail.
 * @param inst_idx  The index of the CSIS service.
 */
typedef void (*bt_csip_release_cb_t)(struct bt_conn *conn, int err,
				     uint8_t inst_idx);

struct bt_csip_cb_t {
	/* Set callbacks */
	bt_csip_lock_set_cb_t             lock_set;
	bt_csip_lock_set_cb_t             release_set;
	bt_csip_discover_members_cb_t     members;
	bt_csip_discover_sets_cb_t        sets;
	bt_csip_lock_changed_cb_t         lock_changed;

	/* Device specific callbacks */
	bt_csip_discover_cb_t             discover;
	bt_csip_lock_read_cb_t            lock_read;
	bt_csip_lock_cb_t                 lock;
	bt_csip_release_cb_t              release;
};

/**
 * @brief Check if advertising data indicates a set member
 *
 * @param set_sirk The SIRK of the set to check against
 * @param data     The advertising data
 *
 * @return true if the advertising data indicates a set member, false otherwise
 */
bool bt_csip_is_set_member(uint8_t set_sirk[BT_CSIP_SET_SIRK_SIZE],
			   struct bt_data *data);

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

/** @brief Read the lock value of a specific device and instance.
 *
 * @param conn      Pointer to the connection to the device.
 * @param inst_idx  Index of the CSIS index of the peer device (as it may have
 *                  multiple CSIS instances).
 *
 * @return Return 0 on success, or an ERRNO value on error.
 */
int bt_csip_lock_get(struct bt_conn *conn, uint8_t inst_idx);

/** @brief Lock an array of set members
 *
 * The members will be locked starting from lowest rank going up.
 *
 * TODO: If locking fails, the already locked members will not be unlocked.
 *
 * @param members   Array of set members to lock.
 * @param count     Number of set members in @p members.
 * @param set       Pointer to the specified set, as a member may be part of
 *                  multiple sets.
 *
 * @return Return 0 on success, or an errno value on error.
 */
int bt_csip_lock(const struct bt_csip_set_member **members, uint8_t count,
		 const struct bt_csip_set_t *set);

/** @brief Release an array of set members
 *
 * The members will be released starting from highest rank going down.
 *
 * @param members   Array of set members to lock.
 * @param count     Number of set members in @p members.
 * @param set       Pointer to the specified set, as a member may be part of
 *                  multiple sets.
 *
 * @return Return 0 on success, or an errno value on error.
 */
int bt_csip_release(const struct bt_csip_set_member **members, uint8_t count,
		    const struct bt_csip_set_t *set);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CSIP_ */
