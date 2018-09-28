/** @file
 *  @brief Bluetooth connection handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CONN_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CONN_H_

/**
 * @brief Connection management
 * @defgroup bt_conn Connection management
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

/** Opaque type representing a connection to a remote device */
struct bt_conn;

/** Connection parameters for LE connections */
struct bt_le_conn_param {
	u16_t interval_min;
	u16_t interval_max;
	u16_t latency;
	u16_t timeout;
};

/** Helper to declare connection parameters inline
  *
  * @param int_min  Minimum Connection Interval (N * 1.25 ms)
  * @param int_max  Maximum Connection Interval (N * 1.25 ms)
  * @param lat      Connection Latency
  * @param to       Supervision Timeout (N * 10 ms)
  */
#define BT_LE_CONN_PARAM(int_min, int_max, lat, to) \
	(&(struct bt_le_conn_param) { \
		.interval_min = (int_min), \
		.interval_max = (int_max), \
		.latency = (lat), \
		.timeout = (to), \
	 })

/** Default LE connection parameters:
  *   Connection Interval: 30-50 ms
  *   Latency: 0
  *   Timeout: 4 s
  */
#define BT_LE_CONN_PARAM_DEFAULT BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, \
						  BT_GAP_INIT_CONN_INT_MAX, \
						  0, 400)

/** @brief Increment a connection's reference count.
 *
 *  Increment the reference count of a connection object.
 *
 *  @param conn Connection object.
 *
 *  @return Connection object with incremented reference count.
 */
struct bt_conn *bt_conn_ref(struct bt_conn *conn);

/** @brief Decrement a connection's reference count.
 *
 *  Decrement the reference count of a connection object.
 *
 *  @param conn Connection object.
 */
void bt_conn_unref(struct bt_conn *conn);

/** @brief Look up an existing connection by address.
 *
 *  Look up an existing connection based on the remote address.
 *
 *  @param id   Local identity (in most cases BT_ID_DEFAULT).
 *  @param peer Remote address.
 *
 *  @return Connection object or NULL if not found. The caller gets a
 *  new reference to the connection object which must be released with
 *  bt_conn_unref() once done using the object.
 */
struct bt_conn *bt_conn_lookup_addr_le(u8_t id, const bt_addr_le_t *peer);

/** @brief Get destination (peer) address of a connection.
 *
 *  @param conn Connection object.
 *
 *  @return Destination address.
 */
const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn);

/** Connection Type */
enum {
	/** LE Connection Type */
	BT_CONN_TYPE_LE,
	/** BR/EDR Connection Type */
	BT_CONN_TYPE_BR,
	/** SCO Connection Type */
	BT_CONN_TYPE_SCO,
};

/** LE Connection Info Structure */
struct bt_conn_le_info {
	const bt_addr_le_t *src; /** Source Address */
	const bt_addr_le_t *dst; /** Destination Address */
	u16_t interval; /** Connection interval */
	u16_t latency; /** Connection slave latency */
	u16_t timeout; /** Connection supervision timeout */
};

/** BR/EDR Connection Info Structure */
struct bt_conn_br_info {
	const bt_addr_t *dst; /** Destination BR/EDR address */
};

/** Connection role (master or slave) */
enum {
	BT_CONN_ROLE_MASTER,
	BT_CONN_ROLE_SLAVE,
};

/** @brief Connection Info Structure
 *
 *
 *  @param type Connection Type
 *  @param role Connection Role
 *  @param id Which local identity the connection was created with
 *  @param le LE Connection specific Info
 *  @param br BR/EDR Connection specific Info
 */
struct bt_conn_info {
	u8_t type;

	u8_t role;

	u8_t id;

	union {
		struct bt_conn_le_info le;

		struct bt_conn_br_info br;
	};
};

/** @brief Get connection info
 *
 *  @param conn Connection object.
 *  @param info Connection info object.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info);

/** @brief Update the connection parameters.
 *
 *  @param conn Connection object.
 *  @param param Updated connection parameters.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param);

/** @brief Disconnect from a remote device or cancel pending connection.
 *
 *  Disconnect an active connection with the specified reason code or cancel
 *  pending outgoing connection.
 *
 *  @param conn Connection to disconnect.
 *  @param reason Reason code for the disconnection.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_disconnect(struct bt_conn *conn, u8_t reason);

/** @brief Initiate an LE connection to a remote device.
 *
 *  Allows initiate new LE link to remote peer using its address.
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param peer  Remote address.
 *  @param param Initial connection parameters.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,
				  const struct bt_le_conn_param *param);

/** @brief Automatically connect to remote device if it's in range.
 *
 *  This function enables/disables automatic connection initiation.
 *  Every time the device loses the connection with peer, this connection
 *  will be re-established if connectable advertisement from peer is received.
 *
 *  @param addr Remote Bluetooth address.
 *  @param param If non-NULL, auto connect is enabled with the given
 *  parameters. If NULL, auto connect is disabled.
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_le_set_auto_conn(bt_addr_le_t *addr,
			const struct bt_le_conn_param *param);

/** @brief Initiate directed advertising to a remote device
 *
 *  Allows initiating a new LE connection to remote peer with the remote
 *  acting in central role and the local device in peripheral role.
 *
 *  The advertising type must be either BT_LE_ADV_DIRECT_IND or
 *  BT_LE_ADV_DIRECT_IND_LOW_DUTY.
 *
 *  In case of high duty cycle this will result in a callback with
 *  connected() with a new connection or with an error.
 *
 *  The advertising may be canceled with bt_conn_disconnect().
 *
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param peer  Remote address.
 *  @param param Directed advertising parameters.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *bt_conn_create_slave_le(const bt_addr_le_t *peer,
					const struct bt_le_adv_param *param);

/** Security level. */
typedef enum __packed {
	/** Only for BR/EDR special cases, like SDP */
	BT_SECURITY_NONE,
	/** No encryption and no authentication. */
	BT_SECURITY_LOW,
	/** Encryption and no authentication (no MITM). */
	BT_SECURITY_MEDIUM,
	/** Encryption and authentication (MITM). */
	BT_SECURITY_HIGH,
	/** Authenticated Secure Connections */
	BT_SECURITY_FIPS,
} bt_security_t;

/** @brief Set security level for a connection.
 *
 *  This function enable security (encryption) for a connection. If device is
 *  already paired with sufficiently strong key encryption will be enabled. If
 *  link is already encrypted with sufficiently strong key this function does
 *  nothing.
 *
 *  If device is not paired pairing will be initiated. If device is paired and
 *  keys are too weak but input output capabilities allow for strong enough keys
 *  pairing will be initiated.
 *
 *  This function may return error if required level of security is not possible
 *  to achieve due to local or remote device limitation (e.g., input output
 *  capabilities).
 *
 *  @param conn Connection object.
 *  @param sec Requested security level.
 *
 *  @return 0 on success or negative error
 */
int bt_conn_security(struct bt_conn *conn, bt_security_t sec);

/** @brief Get encryption key size.
 *
 *  This function gets encryption key size.
 *  If there is no security (encryption) enabled 0 will be returned.
 *
 *  @param conn Existing connection object.
 *
 *  @return Encryption key size.
 */
u8_t bt_conn_enc_key_size(struct bt_conn *conn);

/** @brief Connection callback structure.
 *
 *  This structure is used for tracking the state of a connection.
 *  It is registered with the help of the bt_conn_cb_register() API.
 *  It's permissible to register multiple instances of this @ref bt_conn_cb
 *  type, in case different modules of an application are interested in
 *  tracking the connection state. If a callback is not of interest for
 *  an instance, it may be set to NULL and will as a consequence not be
 *  used for that instance.
 */
struct bt_conn_cb {
	/** @brief A new connection has been established.
	 *
	 *  This callback notifies the application of a new connection.
	 *  In case the err parameter is non-zero it means that the
	 *  connection establishment failed.
	 *
	 *  @param conn New connection object.
	 *  @param err HCI error. Zero for success, non-zero otherwise.
	 */
	void (*connected)(struct bt_conn *conn, u8_t err);

	/** @brief A connection has been disconnected.
	 *
	 *  This callback notifies the application that a connection
	 *  has been disconnected.
	 *
	 *  @param conn Connection object.
	 *  @param reason HCI reason for the disconnection.
	 */
	void (*disconnected)(struct bt_conn *conn, u8_t reason);

	/** @brief LE connection parameter update request.
	 *
	 *  This callback notifies the application that a remote device
	 *  is requesting to update the connection parameters. The
	 *  application accepts the parameters by returning true, or
	 *  rejects them by returning false. Before accepting, the
	 *  application may also adjust the parameters to better suit
	 *  its needs.
	 *
	 *  It is recommended for an application to have just one of these
	 *  callbacks for simplicity. However, if an application registers
	 *  multiple it needs to manage the potentially different
	 *  requirements for each callback. Each callback gets the
	 *  parameters as returned by previous callbacks, i.e. they are not
	 *  necessarily the same ones as the remote originally sent.
	 *
	 *  @param conn Connection object.
	 *  @param param Proposed connection parameters.
	 *
	 *  @return true to accept the parameters, or false to reject them.
	 */
	bool (*le_param_req)(struct bt_conn *conn,
			     struct bt_le_conn_param *param);

	/** @brief The parameters for an LE connection have been updated.
	 *
	 *  This callback notifies the application that the connection
	 *  parameters for an LE connection have been updated.
	 *
	 *  @param conn Connection object.
	 *  @param interval Connection interval.
	 *  @param latency Connection latency.
	 *  @param timeout Connection supervision timeout.
	 */
	void (*le_param_updated)(struct bt_conn *conn, u16_t interval,
				 u16_t latency, u16_t timeout);
#if defined(CONFIG_BT_SMP)
	/** @brief Remote Identity Address has been resolved.
	 *
	 *  This callback notifies the application that a remote
	 *  Identity Address has been resolved
	 *
	 *  @param conn Connection object.
	 *  @param rpa Resolvable Private Address.
	 *  @param identity Identity Address.
	 */
	void (*identity_resolved)(struct bt_conn *conn,
				  const bt_addr_le_t *rpa,
				  const bt_addr_le_t *identity);
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	/** @brief The security level of a connection has changed.
	 *
	 *  This callback notifies the application that the security level
	 *  of a connection has changed.
	 *
	 *  @param conn Connection object.
	 *  @param level New security level of the connection.
	 */
	void (*security_changed)(struct bt_conn *conn, bt_security_t level);
#endif /* defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR) */
	struct bt_conn_cb *_next;
};

/** @brief Register connection callbacks.
 *
 *  Register callbacks to monitor the state of connections.
 *
 *  @param cb Callback struct.
 */
void bt_conn_cb_register(struct bt_conn_cb *cb);

/** Enable/disable bonding.
 *
 *  Set/clear the Bonding flag in the Authentication Requirements of
 *  SMP Pairing Request/Response data.
 *  The initial value of this flag depends on BT_BONDABLE Kconfig setting.
 *  For the vast majority of applications calling this function shouldn't be
 *  needed.
 *
 *  @param enable Value allowing/disallowing to be bondable.
 */
void bt_set_bondable(bool enable);

/** @def BT_PASSKEY_INVALID
 *
 *  Special passkey value that can be used to disable a previously
 *  set fixed passkey.
 */
#define BT_PASSKEY_INVALID 0xffffffff

/** @brief Set a fixed passkey to be used for pairing.
 *
 *  This API is only available when the CONFIG_BT_FIXED_PASSKEY
 *  configuration option has been enabled.
 *
 *  Sets a fixed passkey to be used for pairing. If set, the
 *  pairing_confim() callback will be called for all incoming pairings.
 *
 *  @param passkey A valid passkey (0 - 999999) or BT_PASSKEY_INVALID
 *                 to disable a previously set fixed passkey.
 *
 *  @return 0 on success or a negative error code on failure.
 */
int bt_passkey_set(unsigned int passkey);

/** Authenticated pairing callback structure */
struct bt_conn_auth_cb {
	/** @brief Display a passkey to the user.
	 *
	 *  When called the application is expected to display the given
	 *  passkey to the user, with the expectation that the passkey will
	 *  then be entered on the peer device. The passkey will be in the
	 *  range of 0 - 999999, and is expected to be padded with zeroes so
	 *  that six digits are always shown. E.g. the value 37 should be
	 *  shown as 000037.
	 *
	 *  This callback may be set to NULL, which means that the local
	 *  device lacks the ability do display a passkey. If set
	 *  to non-NULL the cancel callback must also be provided, since
	 *  this is the only way the application can find out that it should
	 *  stop displaying the passkey.
	 *
	 *  @param conn Connection where pairing is currently active.
	 *  @param passkey Passkey to show to the user.
	 */
	void (*passkey_display)(struct bt_conn *conn, unsigned int passkey);

	/** @brief Request the user to enter a passkey.
	 *
	 *  When called the user is expected to enter a passkey. The passkey
	 *  must be in the range of 0 - 999999, and should be expected to
	 *  be zero-padded, as that's how the peer device will typically be
	 *  showing it (e.g. 37 would be shown as 000037).
	 *
	 *  Once the user has entered the passkey its value should be given
	 *  to the stack using the bt_conn_auth_passkey_entry() API.
	 *
	 *  This callback may be set to NULL, which means that the local
	 *  device lacks the ability to enter a passkey. If set to non-NULL
	 *  the cancel callback must also be provided, since this is the
	 *  only way the application can find out that it should stop
	 *  requesting the user to enter a passkey.
	 *
	 *  @param conn Connection where pairing is currently active.
	 */
	void (*passkey_entry)(struct bt_conn *conn);

	/** @brief Request the user to confirm a passkey.
	 *
	 *  When called the user is expected to confirm that the given
	 *  passkey is also shown on the peer device.. The passkey will
	 *  be in the range of 0 - 999999, and should be zero-padded to
	 *  always be six digits (e.g. 37 would be shown as 000037).
	 *
	 *  Once the user has confirmed the passkey to match, the
	 *  bt_conn_auth_passkey_confirm() API should be called. If the
	 *  user concluded that the passkey doesn't match the
	 *  bt_conn_auth_cancel() API should be called.
	 *
	 *  This callback may be set to NULL, which means that the local
	 *  device lacks the ability to confirm a passkey. If set to non-NULL
	 *  the cancel callback must also be provided, since this is the
	 *  only way the application can find out that it should stop
	 *  requesting the user to confirm a passkey.
	 *
	 *  @param conn Connection where pairing is currently active.
	 *  @param passkey Passkey to be confirmed.
	 */
	void (*passkey_confirm)(struct bt_conn *conn, unsigned int passkey);

	/** @brief Cancel the ongoing user request.
	 *
	 *  This callback will be called to notify the application that it
	 *  should cancel any previous user request (passkey display, entry
	 *  or confirmation).
	 *
	 *  This may be set to NULL, but must always be provided whenever the
	 *  passkey_display, passkey_entry passkey_confirm or pairing_confirm
	 *  callback has been provided.
	 *
	 *  @param conn Connection where pairing is currently active.
	 */
	void (*cancel)(struct bt_conn *conn);

	/** @brief Request confirmation for an incoming pairing.
	 *
	 *  This callback will be called to confirm an incoming pairing
	 *  request where none of the other user callbacks is applicable.
	 *
	 *  If the user decides to accept the pairing the
	 *  bt_conn_auth_pairing_confirm() API should be called. If the
	 *  user decides to reject the pairing the bt_conn_auth_cancel() API
	 *  should be called.
	 *
	 *  This callback may be set to NULL, which means that the local
	 *  device lacks the ability to confirm a pairing request. If set
	 *  to non-NULL the cancel callback must also be provided, since
	 *  this is the only way the application can find out that it should
	 *  stop requesting the user to confirm a pairing request.
	 *
	 *  @param conn Connection where pairing is currently active.
	 */
	void (*pairing_confirm)(struct bt_conn *conn);

#if defined(CONFIG_BT_BREDR)
	/** @brief Request the user to enter a passkey.
	 *
	 *  This callback will be called for a BR/EDR (Bluetooth Classic)
	 *  connection where pairing is being performed. Once called the
	 *  user is expected to enter a PIN code with a length between
	 *  1 and 16 digits. If the @a highsec parameter is set to true
	 *  the PIN code must be 16 digits long.
	 *
	 *  Once entered, the PIN code should be given to the stack using
	 *  the bt_conn_auth_pincode_entry() API.
	 *
	 *  This callback may be set to NULL, however in that case pairing
	 *  over BR/EDR will not be possible. If provided, the cancel
	 *  callback must be provided as well.
	 *
	 *  @param conn Connection where pairing is currently active.
	 *  @param highsec true if 16 digit PIN is required.
	 */
	void (*pincode_entry)(struct bt_conn *conn, bool highsec);
#endif

	/** @brief notify that pairing process was complete.
	 *
	 * This callback notifies the applicaiton that the pairing process
	 * has been completed.
	 *
	 * @param conn Connection object.
	 * @param bonded pairing is bonded or not.
	 */
	void (*pairing_complete)(struct bt_conn *conn, bool bonded);

	/** @brief notify that pairing process has failed.
	 *
	 * @param conn Connection object.
	 */
	void (*pairing_failed)(struct bt_conn *conn);
};

/** @brief Register authentication callbacks.
 *
 *  Register callbacks to handle authenticated pairing. Passing NULL
 *  unregisters a previous callbacks structure.
 *
 *  @param cb Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb);

/** @brief Reply with entered passkey.
 *
 *  This function should be called only after passkey_entry callback from
 *  bt_conn_auth_cb structure was called.
 *
 *  @param conn Connection object.
 *  @param passkey Entered passkey.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey);

/** @brief Cancel ongoing authenticated pairing.
 *
 *  This function allows to cancel ongoing authenticated pairing.
 *
 *  @param conn Connection object.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_cancel(struct bt_conn *conn);

/** @brief Reply if passkey was confirmed to match by user.
 *
 *  This function should be called only after passkey_confirm callback from
 *  bt_conn_auth_cb structure was called.
 *
 *  @param conn Connection object.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_passkey_confirm(struct bt_conn *conn);

/** @brief Reply if incoming pairing was confirmed by user.
 *
 *  This function should be called only after pairing_confirm callback from
 *  bt_conn_auth_cb structure was called if user confirmed incoming pairing.
 *
 *  @param conn Connection object.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_pairing_confirm(struct bt_conn *conn);

/** @brief Reply with entered PIN code.
 *
 *  This function should be called only after PIN code callback from
 *  bt_conn_auth_cb structure was called. It's for legacy 2.0 devices.
 *
 *  @param conn Connection object.
 *  @param pin Entered PIN code.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_pincode_entry(struct bt_conn *conn, const char *pin);

/** Connection parameters for BR/EDR connections */
struct bt_br_conn_param {
	bool allow_role_switch;
};

/** Helper to declare BR/EDR connection parameters inline
  *
  * @param role_switch True if role switch is allowed
  */
#define BT_BR_CONN_PARAM(role_switch) \
	(&(struct bt_br_conn_param) { \
		.allow_role_switch = (role_switch), \
	 })

/** Default BR/EDR connection parameters:
  *   Role switch allowed
  */
#define BT_BR_CONN_PARAM_DEFAULT BT_BR_CONN_PARAM(true)


/** @brief Initiate an BR/EDR connection to a remote device.
 *
 *  Allows initiate new BR/EDR link to remote peer using its address.
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param peer  Remote address.
 *  @param param Initial connection parameters.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param);

/** @brief Initiate an SCO connection to a remote device.
 *
 *  Allows initiate new SCO link to remote peer using its address.
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param peer  Remote address.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *bt_conn_create_sco(const bt_addr_t *peer);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CONN_H_ */
