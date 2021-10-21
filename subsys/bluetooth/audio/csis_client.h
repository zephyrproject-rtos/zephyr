/** @file
 *  @brief APIs for Bluetooth Coordinated Set Identification Client
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include "csis.h"
#include "csis_internal.h"

/* Recommended timer for member discovery */
#define CSIS_CLIENT_DISCOVER_TIMER_VALUE               K_SECONDS(10)

#if defined(CONFIG_BT_CSIS_CLIENT)
#define BT_CSIS_CLIENT_MAX_CSIS_INSTANCES CONFIG_BT_CSIS_CLIENT_MAX_CSIS_INSTANCES
#else
#define BT_CSIS_CLIENT_MAX_CSIS_INSTANCES 0
#endif /* CONFIG_BT_CSIS_CLIENT */

struct bt_csis_client_set {
	struct bt_csis_set_sirk set_sirk;
	uint8_t set_size;
	uint8_t rank;
};

struct bt_csis_client_set_member {
	struct bt_conn *conn;
	bt_addr_le_t addr;
	struct bt_csis_client_set sets[BT_CSIS_CLIENT_MAX_CSIS_INSTANCES];
};

typedef void (*bt_csis_client_discover_cb)(struct bt_conn *conn, int err,
					   uint8_t set_count);

/**
 * @brief Initialise the csis_client instance for a connection. This will do a
 * discovery on the device and prepare the instance for following commands.
 *
 * @param member Pointer to a set member struct to store discovery results in
 *
 * @return int Return 0 on success, or an errno value on error.
 */
int bt_csis_client_discover(struct bt_csis_client_set_member *member);

typedef void (*bt_csis_client_discover_sets_cb)(struct bt_conn *conn,
						int err, uint8_t set_count,
						struct bt_csis_client_set *sets);

/**
 * @brief Reads CSIS characteristics from a device, to find more information
 * about the set(s) that the device is part of.
 *
 * @param conn The connection to the device to read CSIS characteristics
 *
 * @return int Return 0 on success, or an errno value on error.
 */
int bt_csis_client_discover_sets(struct bt_conn *conn);

typedef void (*bt_csis_client_discover_members_cb)(int err, uint8_t set_size,
						   uint8_t members_found);

/**
 * @brief Start scanning for all devices that are part of a set.
 *
 * @param set The set to find devices for
 *
 * @return int Return 0 on success, or an errno value on error.
 */
int bt_csis_client_discover_members(struct bt_csis_client_set *set);

typedef void (*bt_csis_client_lock_set_cb)(int err);

/**
 * @brief Callback when the lock value on a set of a connected device changes.
 *
 * @param conn    Connection of the CSIS server.
 * @param set     The set that was changed.
 * @param locked  Whether the lock is locked or release.
 *
 * @return int Return 0 on success, or an errno value on error.
 */
typedef void (*bt_csis_client_lock_changed_cb)(struct bt_conn *conn,
					       struct bt_csis_client_set *set,
					       bool locked);

/**
 * @brief Callback when the lock value is read on a device.
 *
 * @param conn      Connection of the CSIS server.
 * @param err       Error value. 0 on success, GATT error or errno on fail.
 * @param inst_idx  The index of the CSIS service.
 * @param locked    Whether the lock is locked or release.
 */
typedef void (*bt_csis_client_lock_read_cb)(struct bt_conn *conn, int err,
					    uint8_t inst_idx, bool locked);

/**
 * @brief Callback when the lock value is written to a device.
 *
 * @param conn      Connection of the CSIS server.
 * @param err       Error value. 0 on success, GATT error or errno on fail.
 * @param inst_idx  The index of the CSIS service.
 */
typedef void (*bt_csis_client_lock_cb)(struct bt_conn *conn, int err,
				       uint8_t inst_idx);

/**
 * @brief Callback when the release value is written to a device.
 *
 * @param conn      Connection of the CSIS server.
 * @param err       Error value. 0 on success, GATT error or errno on fail.
 * @param inst_idx  The index of the CSIS service.
 */
typedef void (*bt_csis_client_release_cb)(struct bt_conn *conn, int err,
					  uint8_t inst_idx);

struct bt_csis_client_cb {
	/* Set callbacks */
	bt_csis_client_lock_set_cb             lock_set;
	bt_csis_client_lock_set_cb             release_set;
	bt_csis_client_discover_members_cb     members;
	bt_csis_client_discover_sets_cb        sets;
	bt_csis_client_lock_changed_cb         lock_changed;

	/* Device specific callbacks */
	bt_csis_client_discover_cb             discover;
	bt_csis_client_lock_read_cb            lock_read;
	bt_csis_client_lock_cb                 lock;
	bt_csis_client_release_cb              release;
};

/**
 * @brief Check if advertising data indicates a set member
 *
 * @param set_sirk The SIRK of the set to check against
 * @param data     The advertising data
 *
 * @return true if the advertising data indicates a set member, false otherwise
 */
bool bt_csis_client_is_set_member(uint8_t set_sirk[BT_CSIS_SET_SIRK_SIZE],
				  struct bt_data *data);

/**
 * @brief Lock the set
 *
 * Connect to and set the lock for all devices in a set.
 *
 * @return Return 0 on success, or an errno value on error.
 */
int bt_csis_client_lock_set(void);

/**
 * @brief Connect to and release the lock for all devices in a set
 *
 * @return int Return 0 on success, or an errno value on error.
 */
int bt_csis_client_release_set(void);

/**
 * @brief Registers callbacks for csis_client.
 *
 * @param cb   Pointer to the callback structure.
 */
void bt_csis_client_register_cb(struct bt_csis_client_cb *cb);

/**
 * @brief Read the lock value of a specific device and instance.
 *
 * @param conn      Pointer to the connection to the device.
 * @param inst_idx  Index of the CSIS index of the peer device (as it may have
 *                  multiple CSIS instances).
 *
 * @return Return 0 on success, or an errno value on error.
 */
int bt_csis_client_lock_get(struct bt_conn *conn, uint8_t inst_idx);

/**
 * @brief Lock an array of set members
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
int bt_csis_client_lock(const struct bt_csis_client_set_member **members,
			uint8_t count, const struct bt_csis_client_set *set);

/**
 * @brief Release an array of set members
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
int bt_csis_client_release(const struct bt_csis_client_set_member **members,
			   uint8_t count, const struct bt_csis_client_set *set);
