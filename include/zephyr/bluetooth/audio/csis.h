/**
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_CSIS_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_CSIS_H_

/**
 * @brief Coordinated Set Identification Service (CSIS)
 *
 * @defgroup bt_gatt_csis Coordinated Set Identification Service  (CSIS)
 *
 * @ingroup bluetooth
 * @{
 * *
 * [Experimental] Users should note that the APIs can change as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Recommended timer for member discovery */
#define CSIS_CLIENT_DISCOVER_TIMER_VALUE        K_SECONDS(10)

#if defined(CONFIG_BT_CSIS_CLIENT)
#define BT_CSIS_CLIENT_MAX_CSIS_INSTANCES CONFIG_BT_CSIS_CLIENT_MAX_CSIS_INSTANCES
#else
#define BT_CSIS_CLIENT_MAX_CSIS_INSTANCES 0
#endif /* CONFIG_BT_CSIS_CLIENT */

/** Minimum size of a set */
#define BT_CSIS_MINIMUM_SET_SIZE                2

/** Accept the request to read the SIRK as plaintext */
#define BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT        0x00
/** Accept the request to read the SIRK, but return encrypted SIRK */
#define BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC    0x01
/** Reject the request to read the SIRK */
#define BT_CSIS_READ_SIRK_REQ_RSP_REJECT        0x02
/** SIRK is available only via an OOB procedure */
#define BT_CSIS_READ_SIRK_REQ_RSP_OOB_ONLY      0x03

/** Size of the Set Identification Resolving Key (SIRK) */
#define BT_CSIS_SET_SIRK_SIZE 16

/* Coordinate Set Identification Service Error codes */
/** Service is already locked */
#define BT_CSIS_ERROR_LOCK_DENIED               0x80
/** Service is not locked */
#define BT_CSIS_ERROR_LOCK_RELEASE_DENIED       0x81
/** Invalid lock value */
#define BT_CSIS_ERROR_LOCK_INVAL_VALUE          0x82
/** SIRK only available out-of-band */
#define BT_CSIS_ERROR_SIRK_OOB_ONLY             0x83
/** Client is already owner of the lock */
#define BT_CSIS_ERROR_LOCK_ALREADY_GRANTED      0x84

/** @brief Opaque Coordinated Set Identification Service instance. */
struct bt_csis;

/** Callback structure for the Coordinated Set Identification Service */
struct bt_csis_cb {
	/**
	 * @brief Callback whenever the lock changes on the server.
	 *
	 * @param conn    The connection to the client that changed the lock.
	 *                NULL if server changed it, either by calling
	 *                bt_csis_lock() or by timeout.
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
	 * If set to 0, the set size characteristic won't be initialized.
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
 * @brief Register a Coordinated Set Identification Service instance.
 *
 * This will register and enable the service and make it discoverable by
 * clients.
 *
 * This shall only be done as a server.
 *
 * @param      param Coordinated Set Identification Service register parameters.
 * @param[out] csis  Pointer to the registered Coordinated Set Identification
 *                   Service.
 *
 * @return 0 if success, errno on failure.
 */
int bt_csis_register(const struct bt_csis_register_param *param,
		     struct bt_csis **csis);

/**
 * @brief Print the SIRK to the debug output
 *
 * @param csis   Pointer to the Coordinated Set Identification Service.
 */
void bt_csis_print_sirk(const struct bt_csis *csis);

/**
 * @brief Starts advertising the Resolvable Set Identifier value.
 *
 * This cannot be used with other connectable advertising sets.
 *
 * @param csis          Pointer to the Coordinated Set Identification Service.
 * @param enable	If true start advertising, if false stop advertising
 *
 * @return int		0 if on success, errno on error.
 */
int bt_csis_advertise(struct bt_csis *csis, bool enable);

/**
 * @brief Locks a specific Coordinated Set Identification Service instance on the server.
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

/** Information about a specific set */
struct bt_csis_client_set_info {
	/**
	 * @brief The 16 octet set Set Identity Resolving Key (SIRK)
	 *
	 * The Set SIRK may not be exposed by the server over Bluetooth, and
	 * may require an out-of-band solution.
	 */
	uint8_t set_sirk[BT_CSIS_SET_SIRK_SIZE];

	/**
	 * @brief The size of the set
	 *
	 * Will be 0 if not exposed by the server.
	 */
	uint8_t set_size;

	/**
	 * @brief The rank of the set on on the remote device
	 *
	 * Will be 0 if not exposed by the server.
	 */
	uint8_t rank;
};

/**
 * @brief Struct representing a coordinated set instance on a remote device
 *
 * The values in this struct will be populated during discovery of sets
 * (bt_csis_client_discover()).
 */
struct bt_csis_client_csis_inst {
	struct bt_csis_client_set_info info;

	/** Internally used pointer value */
	struct bt_csis *csis;
};

/** Struct representing a remote device as a set member */
struct bt_csis_client_set_member {
	/** Connection pointer to the remote device, populated by the user */
	struct bt_conn *conn;
	/** Array of Coordinated Set Identification Service instances for the remote device */
	struct bt_csis_client_csis_inst insts[BT_CSIS_CLIENT_MAX_CSIS_INSTANCES];
};

/**
 * @typedef bt_csis_client_discover_cb
 * @brief Callback for discovering Coordinated Set Identification Services.
 *
 * @param member    Pointer to the set member.
 * @param err       0 on success, or an errno value on error.
 * @param set_count Number of sets on the member.
 */
typedef void (*bt_csis_client_discover_cb)(struct bt_csis_client_set_member *member,
					   int err, uint8_t set_count);

/**
 * @brief Initialise the csis_client instance for a connection. This will do a
 * discovery on the device and prepare the instance for following commands.
 *
 * @param member Pointer to a set member struct to store discovery results in.
 *
 * @return int Return 0 on success, or an errno value on error.
 */
int bt_csis_client_discover(struct bt_csis_client_set_member *member);

/**
 * @typedef bt_csis_client_lock_set_cb
 * @brief Callback for locking a set across one or more devices
 *
 * @param err       0 on success, or an errno value on error.
 */
typedef void (*bt_csis_client_lock_set_cb)(int err);

/**
 * @typedef bt_csis_client_lock_changed_cb
 * @brief Callback when the lock value on a set of a connected device changes.
 *
 * @param inst    The Coordinated Set Identification Service instance that was
 *                changed.
 * @param locked  Whether the lock is locked or release.
 *
 * @return int Return 0 on success, or an errno value on error.
 */
typedef void (*bt_csis_client_lock_changed_cb)(struct bt_csis_client_csis_inst *inst,
					       bool locked);

/**
 * @typedef bt_csis_client_lock_state_read_cb
 * @brief Callback for bt_csis_client_get_lock_state()
 *
 * If any of the set members supplied to bt_csis_client_get_lock_state() is
 * in the locked state, this will be called with @p locked true. If any
 * set member is in the locked state, the remaining (if any) won't be read.
 * Likewise, if any error occurs, the procedure will also be aborted.
 *
 * @param set_info  Pointer to the a specific set_info struct.
 * @param err       Error value. 0 on success, GATT error or errno on fail.
 * @param locked    Whether the lock is locked or release.
 */
typedef void (*bt_csis_client_lock_state_read_cb)(const struct bt_csis_client_set_info *set_info,
						  int err, bool locked);

struct bt_csis_client_cb {
	/* Set callbacks */
	bt_csis_client_lock_set_cb             lock_set;
	bt_csis_client_lock_set_cb             release_set;
	bt_csis_client_lock_changed_cb         lock_changed;

	/* Device specific callbacks */
	bt_csis_client_discover_cb             discover;
	bt_csis_client_lock_state_read_cb      lock_state_read;
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
 * @brief Registers callbacks for csis_client.
 *
 * @param cb   Pointer to the callback structure.
 */
void bt_csis_client_register_cb(struct bt_csis_client_cb *cb);

/**
 * @brief Check if an array of set members are unlocked
 *
 * This will read the set lock value on all members and respond with a single
 * state.
 *
 * @param members   Array of set members to check lock state for.
 * @param count     Number of set members in @p members.
 * @param set_info  Pointer to the a specific set_info struct, as a member may
 *                  be part of multiple sets.
 *
 * @return Return 0 on success, or an errno value on error.
 */
int bt_csis_client_get_lock_state(const struct bt_csis_client_set_member **members,
				  uint8_t count,
				  const struct bt_csis_client_set_info *set_info);

/**
 * @brief Lock an array of set members
 *
 * The members will be locked starting from lowest rank going up.
 *
 * TODO: If locking fails, the already locked members will not be unlocked.
 *
 * @param members   Array of set members to lock.
 * @param count     Number of set members in @p members.
 * @param set_info  Pointer to the a specific set_info struct, as a member may
 *                  be part of multiple sets.
 *
 * @return Return 0 on success, or an errno value on error.
 */
int bt_csis_client_lock(const struct bt_csis_client_set_member **members,
			uint8_t count,
			const struct bt_csis_client_set_info *set_info);

/**
 * @brief Release an array of set members
 *
 * The members will be released starting from highest rank going down.
 *
 * @param members   Array of set members to lock.
 * @param count     Number of set members in @p members.
 * @param set_info  Pointer to the a specific set_info struct, as a member may
 *                  be part of multiple sets.
 *
 * @return Return 0 on success, or an errno value on error.
 */
int bt_csis_client_release(const struct bt_csis_client_set_member **members,
			   uint8_t count,
			   const struct bt_csis_client_set_info *set_info);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_CSIS_H_ */
