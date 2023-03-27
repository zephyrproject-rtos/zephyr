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

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_err.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/direction.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque type representing a connection to a remote device */
struct bt_conn;

/** Connection parameters for LE connections */
struct bt_le_conn_param {
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
};

/** @brief Initialize connection parameters
 *
 *  @param int_min  Minimum Connection Interval (N * 1.25 ms)
 *  @param int_max  Maximum Connection Interval (N * 1.25 ms)
 *  @param lat      Connection Latency
 *  @param to       Supervision Timeout (N * 10 ms)
 */
#define BT_LE_CONN_PARAM_INIT(int_min, int_max, lat, to) \
{ \
	.interval_min = (int_min), \
	.interval_max = (int_max), \
	.latency = (lat), \
	.timeout = (to), \
}

/** Helper to declare connection parameters inline
 *
 *  @param int_min  Minimum Connection Interval (N * 1.25 ms)
 *  @param int_max  Maximum Connection Interval (N * 1.25 ms)
 *  @param lat      Connection Latency
 *  @param to       Supervision Timeout (N * 10 ms)
 */
#define BT_LE_CONN_PARAM(int_min, int_max, lat, to) \
	((struct bt_le_conn_param[]) { \
		BT_LE_CONN_PARAM_INIT(int_min, int_max, lat, to) \
	 })

/** Default LE connection parameters:
 *    Connection Interval: 30-50 ms
 *    Latency: 0
 *    Timeout: 4 s
 */
#define BT_LE_CONN_PARAM_DEFAULT BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, \
						  BT_GAP_INIT_CONN_INT_MAX, \
						  0, 400)

/** Connection PHY information for LE connections */
struct bt_conn_le_phy_info {
	uint8_t tx_phy; /** Connection transmit PHY */
	uint8_t rx_phy; /** Connection receive PHY */
};

/** Connection PHY options */
enum {
	/** Convenience value when no options are specified. */
	BT_CONN_LE_PHY_OPT_NONE = 0,

	/** LE Coded using S=2 coding preferred when transmitting. */
	BT_CONN_LE_PHY_OPT_CODED_S2  = BIT(0),

	/** LE Coded using S=8 coding preferred when transmitting. */
	BT_CONN_LE_PHY_OPT_CODED_S8  = BIT(1),
};

/** Preferred PHY parameters for LE connections */
struct bt_conn_le_phy_param {
	uint16_t options;     /** Connection PHY options. */
	uint8_t  pref_tx_phy; /** Bitmask of preferred transmit PHYs */
	uint8_t  pref_rx_phy; /** Bitmask of preferred receive PHYs */
};

/** Initialize PHY parameters
 *
 * @param _pref_tx_phy Bitmask of preferred transmit PHYs.
 * @param _pref_rx_phy Bitmask of preferred receive PHYs.
 */
#define BT_CONN_LE_PHY_PARAM_INIT(_pref_tx_phy, _pref_rx_phy) \
{ \
	.options = BT_CONN_LE_PHY_OPT_NONE, \
	.pref_tx_phy = (_pref_tx_phy), \
	.pref_rx_phy = (_pref_rx_phy), \
}

/** Helper to declare PHY parameters inline
 *
 * @param _pref_tx_phy Bitmask of preferred transmit PHYs.
 * @param _pref_rx_phy Bitmask of preferred receive PHYs.
 */
#define BT_CONN_LE_PHY_PARAM(_pref_tx_phy, _pref_rx_phy) \
	((struct bt_conn_le_phy_param []) { \
		BT_CONN_LE_PHY_PARAM_INIT(_pref_tx_phy, _pref_rx_phy) \
	 })

/** Only LE 1M PHY */
#define BT_CONN_LE_PHY_PARAM_1M BT_CONN_LE_PHY_PARAM(BT_GAP_LE_PHY_1M, \
						     BT_GAP_LE_PHY_1M)

/** Only LE 2M PHY */
#define BT_CONN_LE_PHY_PARAM_2M BT_CONN_LE_PHY_PARAM(BT_GAP_LE_PHY_2M, \
						     BT_GAP_LE_PHY_2M)

/** Only LE Coded PHY. */
#define BT_CONN_LE_PHY_PARAM_CODED BT_CONN_LE_PHY_PARAM(BT_GAP_LE_PHY_CODED, \
							BT_GAP_LE_PHY_CODED)

/** All LE PHYs. */
#define BT_CONN_LE_PHY_PARAM_ALL BT_CONN_LE_PHY_PARAM(BT_GAP_LE_PHY_1M |   \
						      BT_GAP_LE_PHY_2M |   \
						      BT_GAP_LE_PHY_CODED, \
						      BT_GAP_LE_PHY_1M |   \
						      BT_GAP_LE_PHY_2M |   \
						      BT_GAP_LE_PHY_CODED)

/** Connection data length information for LE connections */
struct bt_conn_le_data_len_info {
	/** Maximum Link Layer transmission payload size in bytes. */
	uint16_t tx_max_len;
	/** Maximum Link Layer transmission payload time in us. */
	uint16_t tx_max_time;
	/** Maximum Link Layer reception payload size in bytes. */
	uint16_t rx_max_len;
	/** Maximum Link Layer reception payload time in us. */
	uint16_t rx_max_time;
};

/** Connection data length parameters for LE connections */
struct bt_conn_le_data_len_param {
	/** Maximum Link Layer transmission payload size in bytes. */
	uint16_t tx_max_len;
	/** Maximum Link Layer transmission payload time in us. */
	uint16_t tx_max_time;
};

/** Initialize transmit data length parameters
 *
 * @param  _tx_max_len  Maximum Link Layer transmission payload size in bytes.
 * @param  _tx_max_time Maximum Link Layer transmission payload time in us.
 */
#define BT_CONN_LE_DATA_LEN_PARAM_INIT(_tx_max_len, _tx_max_time) \
{ \
	.tx_max_len = (_tx_max_len), \
	.tx_max_time = (_tx_max_time), \
}

/** Helper to declare transmit data length parameters inline
 *
 * @param  _tx_max_len  Maximum Link Layer transmission payload size in bytes.
 * @param  _tx_max_time Maximum Link Layer transmission payload time in us.
 */
#define BT_CONN_LE_DATA_LEN_PARAM(_tx_max_len, _tx_max_time) \
	((struct bt_conn_le_data_len_param[]) { \
		BT_CONN_LE_DATA_LEN_PARAM_INIT(_tx_max_len, _tx_max_time) \
	 })

/** Default LE data length parameters. */
#define BT_LE_DATA_LEN_PARAM_DEFAULT \
	BT_CONN_LE_DATA_LEN_PARAM(BT_GAP_DATA_LEN_DEFAULT, \
				  BT_GAP_DATA_TIME_DEFAULT)

/** Maximum LE data length parameters. */
#define BT_LE_DATA_LEN_PARAM_MAX \
	BT_CONN_LE_DATA_LEN_PARAM(BT_GAP_DATA_LEN_MAX, \
				  BT_GAP_DATA_TIME_MAX)

/** @brief Increment a connection's reference count.
 *
 *  Increment the reference count of a connection object.
 *
 *  @note Will return NULL if the reference count is zero.
 *
 *  @param conn Connection object.
 *
 *  @return Connection object with incremented reference count, or NULL if the
 *          reference count is zero.
 */
struct bt_conn *bt_conn_ref(struct bt_conn *conn);

/** @brief Decrement a connection's reference count.
 *
 *  Decrement the reference count of a connection object.
 *
 *  @param conn Connection object.
 */
void bt_conn_unref(struct bt_conn *conn);

/** @brief Iterate through all bt_conn objects.
 *
 * Iterates trough all bt_conn objects that are alive in the Host allocator.
 *
 * To find established connections, combine this with @ref bt_conn_get_info.
 * Check that @ref bt_conn_info.state is @ref BT_CONN_STATE_CONNECTED.
 *
 * Thread safety: This API is thread safe, but it does not guarantee a
 * sequentially-consistent view for objects allocated during the current
 * invocation of this API. E.g. If preempted while allocations A then B then C
 * happen then results may include A and C but miss B.
 *
 * @param type  Connection Type
 * @param func  Function to call for each connection.
 * @param data  Data to pass to the callback function.
 */
void bt_conn_foreach(int type, void (*func)(struct bt_conn *conn, void *data),
		     void *data);

/** @brief Look up an existing connection by address.
 *
 *  Look up an existing connection based on the remote address.
 *
 *  The caller gets a new reference to the connection object which must be
 *  released with bt_conn_unref() once done using the object.
 *
 *  @param id   Local identity (in most cases BT_ID_DEFAULT).
 *  @param peer Remote address.
 *
 *  @return Connection object or NULL if not found.
 */
struct bt_conn *bt_conn_lookup_addr_le(uint8_t id, const bt_addr_le_t *peer);

/** @brief Get destination (peer) address of a connection.
 *
 *  @param conn Connection object.
 *
 *  @return Destination address.
 */
const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn);

/** @brief Get array index of a connection
 *
 *  This function is used to map bt_conn to index of an array of
 *  connections. The array has CONFIG_BT_MAX_CONN elements.
 *
 *  @param conn Connection object.
 *
 *  @return Index of the connection object.
 *          The range of the returned value is 0..CONFIG_BT_MAX_CONN-1
 */
uint8_t bt_conn_index(const struct bt_conn *conn);

/** Connection Type */
enum {
	/** LE Connection Type */
	BT_CONN_TYPE_LE = BIT(0),
	/** BR/EDR Connection Type */
	BT_CONN_TYPE_BR = BIT(1),
	/** SCO Connection Type */
	BT_CONN_TYPE_SCO = BIT(2),
	/** ISO Connection Type */
	BT_CONN_TYPE_ISO = BIT(3),
	/** All Connection Type */
	BT_CONN_TYPE_ALL = BT_CONN_TYPE_LE | BT_CONN_TYPE_BR |
			   BT_CONN_TYPE_SCO | BT_CONN_TYPE_ISO,
};

/** LE Connection Info Structure */
struct bt_conn_le_info {
	/** Source (Local) Identity Address */
	const bt_addr_le_t *src;
	/** Destination (Remote) Identity Address or remote Resolvable Private
	 *  Address (RPA) before identity has been resolved.
	 */
	const bt_addr_le_t *dst;
	/** Local device address used during connection setup. */
	const bt_addr_le_t *local;
	/** Remote device address used during connection setup. */
	const bt_addr_le_t *remote;
	uint16_t interval; /** Connection interval */
	uint16_t latency; /** Connection peripheral latency */
	uint16_t timeout; /** Connection supervision timeout */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	const struct bt_conn_le_phy_info      *phy;
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	/* Connection maximum single fragment parameters */
	const struct bt_conn_le_data_len_info *data_len;
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */
};

/** @brief Convert connection interval to milliseconds
 *
 *  Multiply by 1.25 to get milliseconds.
 *
 *  Note that this may be inaccurate, as something like 7.5 ms cannot be
 *  accurately presented with integers.
 */
#define BT_CONN_INTERVAL_TO_MS(interval) ((interval) * 5U / 4U)

/** @brief Convert connection interval to microseconds
 *
 *  Multiply by 1250 to get microseconds.
 */
#define BT_CONN_INTERVAL_TO_US(interval) ((interval) * 1250U)

/** BR/EDR Connection Info Structure */
struct bt_conn_br_info {
	const bt_addr_t *dst; /** Destination (Remote) BR/EDR address */
};

enum {
	BT_CONN_ROLE_CENTRAL = 0,
	BT_CONN_ROLE_PERIPHERAL = 1,
};

enum bt_conn_state {
	/** Channel disconnected */
	BT_CONN_STATE_DISCONNECTED,
	/** Channel in connecting state */
	BT_CONN_STATE_CONNECTING,
	/** Channel connected and ready for upper layer traffic on it */
	BT_CONN_STATE_CONNECTED,
	/** Channel in disconnecting state */
	BT_CONN_STATE_DISCONNECTING,
};

/** Security level. */
typedef enum __packed {
	/** Level 0: Only for BR/EDR special cases, like SDP */
	BT_SECURITY_L0,
	/** Level 1: No encryption and no authentication. */
	BT_SECURITY_L1,
	/** Level 2: Encryption and no authentication (no MITM). */
	BT_SECURITY_L2,
	/** Level 3: Encryption and authentication (MITM). */
	BT_SECURITY_L3,
	/** Level 4: Authenticated Secure Connections and 128-bit key. */
	BT_SECURITY_L4,
	/** Bit to force new pairing procedure, bit-wise OR with requested
	 *  security level.
	 */
	BT_SECURITY_FORCE_PAIR = BIT(7),
} bt_security_t;

/** Security Info Flags. */
enum bt_security_flag {
	/** Paired with Secure Connections. */
	BT_SECURITY_FLAG_SC = BIT(0),
	/** Paired with Out of Band method. */
	BT_SECURITY_FLAG_OOB = BIT(1),
};

/** Security Info Structure. */
struct bt_security_info {
	/** Security Level. */
	bt_security_t level;
	/** Encryption Key Size. */
	uint8_t enc_key_size;
	/** Flags. */
	enum bt_security_flag flags;
};

/** Connection role (central or peripheral) */
#define BT_CONN_ROLE_MASTER __DEPRECATED_MACRO BT_CONN_ROLE_CENTRAL
#define BT_CONN_ROLE_SLAVE __DEPRECATED_MACRO BT_CONN_ROLE_PERIPHERAL

/** Connection Info Structure */
struct bt_conn_info {
	/** Connection Type. */
	uint8_t type;
	/** Connection Role. */
	uint8_t role;
	/** Which local identity the connection was created with */
	uint8_t id;
	/** Connection Type specific Info.*/
	union {
		/** LE Connection specific Info. */
		struct bt_conn_le_info le;
		/** BR/EDR Connection specific Info. */
		struct bt_conn_br_info br;
	};
	/** Connection state. */
	enum bt_conn_state state;
	/** Security specific info. */
	struct bt_security_info security;
};

/** LE Connection Remote Info Structure */
struct bt_conn_le_remote_info {

	/** Remote LE feature set (bitmask). */
	const uint8_t *features;
};

/** BR/EDR Connection Remote Info structure */
struct bt_conn_br_remote_info {

	/** Remote feature set (pages of bitmasks). */
	const uint8_t *features;

	/** Number of pages in the remote feature set. */
	uint8_t num_pages;
};

/** @brief Connection Remote Info Structure
 *
 *  @note The version, manufacturer and subversion fields will only contain
 *        valid data if @kconfig{CONFIG_BT_REMOTE_VERSION} is enabled.
 */
struct bt_conn_remote_info {
	/** Connection Type */
	uint8_t  type;

	/** Remote Link Layer version */
	uint8_t  version;

	/** Remote manufacturer identifier */
	uint16_t manufacturer;

	/** Per-manufacturer unique revision */
	uint16_t subversion;

	union {
		/** LE connection remote info */
		struct bt_conn_le_remote_info le;

		/** BR/EDR connection remote info */
		struct bt_conn_br_remote_info br;
	};
};

enum bt_conn_le_tx_power_phy {
	/** Convenience macro for when no PHY is set. */
	BT_CONN_LE_TX_POWER_PHY_NONE,
	/** LE 1M PHY */
	BT_CONN_LE_TX_POWER_PHY_1M,
	 /** LE 2M PHY */
	BT_CONN_LE_TX_POWER_PHY_2M,
	/** LE Coded PHY using S=8 coding. */
	BT_CONN_LE_TX_POWER_PHY_CODED_S8,
	/** LE Coded PHY using S=2 coding. */
	BT_CONN_LE_TX_POWER_PHY_CODED_S2,
};

/** LE Transmit Power Level Structure */
struct bt_conn_le_tx_power {

	/** Input: 1M, 2M, Coded S2 or Coded S8 */
	uint8_t phy;

	/** Output: current transmit power level */
	int8_t current_level;

	/** Output: maximum transmit power level */
	int8_t max_level;
};

/** @brief Passkey Keypress Notification type
 *
 *  The numeric values are the same as in the Core specification for Pairing
 *  Keypress Notification PDU.
 */
enum bt_conn_auth_keypress {
	BT_CONN_AUTH_KEYPRESS_ENTRY_STARTED = 0x00,
	BT_CONN_AUTH_KEYPRESS_DIGIT_ENTERED = 0x01,
	BT_CONN_AUTH_KEYPRESS_DIGIT_ERASED = 0x02,
	BT_CONN_AUTH_KEYPRESS_CLEARED = 0x03,
	BT_CONN_AUTH_KEYPRESS_ENTRY_COMPLETED = 0x04,
};

/** @brief Get connection info
 *
 *  @param conn Connection object.
 *  @param info Connection info object.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info);

/** @brief Get connection info for the remote device.
 *
 *  @param conn Connection object.
 *  @param remote_info Connection remote info object.
 *
 *  @note In order to retrieve the remote version (version, manufacturer
 *  and subversion) @kconfig{CONFIG_BT_REMOTE_VERSION} must be enabled
 *
 *  @note The remote information is exchanged directly after the connection has
 *  been established. The application can be notified about when the remote
 *  information is available through the remote_info_available callback.
 *
 *  @return Zero on success or (negative) error code on failure.
 *  @return -EBUSY The remote information is not yet available.
 */
int bt_conn_get_remote_info(struct bt_conn *conn,
			    struct bt_conn_remote_info *remote_info);

/** @brief Get connection transmit power level.
 *
 *  @param conn           Connection object.
 *  @param tx_power_level Transmit power level descriptor.
 *
 *  @return Zero on success or (negative) error code on failure.
 *  @return -ENOBUFS HCI command buffer is not available.
 */
int bt_conn_le_get_tx_power_level(struct bt_conn *conn,
				  struct bt_conn_le_tx_power *tx_power_level);

/** @brief Update the connection parameters.
 *
 *  If the local device is in the peripheral role then updating the connection
 *  parameters will be delayed. This delay can be configured by through the
 *  @kconfig{CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT} option.
 *
 *  @param conn Connection object.
 *  @param param Updated connection parameters.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param);

/** @brief Update the connection transmit data length parameters.
 *
 *  @param conn  Connection object.
 *  @param param Updated data length parameters.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_le_data_len_update(struct bt_conn *conn,
			       const struct bt_conn_le_data_len_param *param);

/** @brief Update the connection PHY parameters.
 *
 *  Update the preferred transmit and receive PHYs of the connection.
 *  Use @ref BT_GAP_LE_PHY_NONE to indicate no preference.
 *
 *  @param conn Connection object.
 *  @param param Updated connection parameters.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_le_phy_update(struct bt_conn *conn,
			  const struct bt_conn_le_phy_param *param);

/** @brief Disconnect from a remote device or cancel pending connection.
 *
 *  Disconnect an active connection with the specified reason code or cancel
 *  pending outgoing connection.
 *
 *  The disconnect reason for a normal disconnect should be:
 *  @ref BT_HCI_ERR_REMOTE_USER_TERM_CONN.
 *
 *  The following disconnect reasons are accepted:
 *   - @ref BT_HCI_ERR_AUTH_FAIL
 *   - @ref BT_HCI_ERR_REMOTE_USER_TERM_CONN
 *   - @ref BT_HCI_ERR_REMOTE_LOW_RESOURCES
 *   - @ref BT_HCI_ERR_REMOTE_POWER_OFF
 *   - @ref BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
 *   - @ref BT_HCI_ERR_PAIRING_NOT_SUPPORTED
 *   - @ref BT_HCI_ERR_UNACCEPT_CONN_PARAM
 *
 *  @param conn Connection to disconnect.
 *  @param reason Reason code for the disconnection.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason);

enum {
	/** Convenience value when no options are specified. */
	BT_CONN_LE_OPT_NONE = 0,

	/** @brief Enable LE Coded PHY.
	 *
	 *  Enable scanning on the LE Coded PHY.
	 */
	BT_CONN_LE_OPT_CODED = BIT(0),

	/** @brief Disable LE 1M PHY.
	 *
	 *  Disable scanning on the LE 1M PHY.
	 *
	 *  @note Requires @ref BT_CONN_LE_OPT_CODED.
	 */
	BT_CONN_LE_OPT_NO_1M = BIT(1),
};

struct bt_conn_le_create_param {

	/** Bit-field of create connection options. */
	uint32_t options;

	/** Scan interval (N * 0.625 ms) */
	uint16_t interval;

	/** Scan window (N * 0.625 ms) */
	uint16_t window;

	/** @brief Scan interval LE Coded PHY (N * 0.625 MS)
	 *
	 *  Set zero to use same as LE 1M PHY scan interval
	 */
	uint16_t interval_coded;

	/** @brief Scan window LE Coded PHY (N * 0.625 MS)
	 *
	 *  Set zero to use same as LE 1M PHY scan window.
	 */
	uint16_t window_coded;

	/** @brief Connection initiation timeout (N * 10 MS)
	 *
	 *  Set zero to use the default @kconfig{CONFIG_BT_CREATE_CONN_TIMEOUT}
	 *  timeout.
	 *
	 *  @note Unused in @ref bt_conn_le_create_auto
	 */
	uint16_t timeout;
};

/** @brief Initialize create connection parameters
 *
 *  @param _options  Create connection options.
 *  @param _interval Create connection scan interval (N * 0.625 ms).
 *  @param _window   Create connection scan window (N * 0.625 ms).
 */
#define BT_CONN_LE_CREATE_PARAM_INIT(_options, _interval, _window) \
{ \
	.options = (_options), \
	.interval = (_interval), \
	.window = (_window), \
	.interval_coded = 0, \
	.window_coded = 0, \
	.timeout = 0, \
}

/** Helper to declare create connection parameters inline
 *
 *  @param _options  Create connection options.
 *  @param _interval Create connection scan interval (N * 0.625 ms).
 *  @param _window   Create connection scan window (N * 0.625 ms).
 */
#define BT_CONN_LE_CREATE_PARAM(_options, _interval, _window) \
	((struct bt_conn_le_create_param[]) { \
		BT_CONN_LE_CREATE_PARAM_INIT(_options, _interval, _window) \
	 })

/** Default LE create connection parameters.
 *  Scan continuously by setting scan interval equal to scan window.
 */
#define BT_CONN_LE_CREATE_CONN \
	BT_CONN_LE_CREATE_PARAM(BT_CONN_LE_OPT_NONE, \
				BT_GAP_SCAN_FAST_INTERVAL, \
				BT_GAP_SCAN_FAST_INTERVAL)

/** Default LE create connection using filter accept list parameters.
 *  Scan window:   30 ms.
 *  Scan interval: 60 ms.
 */
#define BT_CONN_LE_CREATE_CONN_AUTO \
	BT_CONN_LE_CREATE_PARAM(BT_CONN_LE_OPT_NONE, \
				BT_GAP_SCAN_FAST_INTERVAL, \
				BT_GAP_SCAN_FAST_WINDOW)

/** @brief Initiate an LE connection to a remote device.
 *
 *  Allows initiate new LE link to remote peer using its address.
 *
 *  The caller gets a new reference to the connection object which must be
 *  released with bt_conn_unref() once done using the object.
 *
 *  This uses the General Connection Establishment procedure.
 *
 *  The application must disable explicit scanning before initiating
 *  a new LE connection.
 *
 *  @param[in]  peer         Remote address.
 *  @param[in]  create_param Create connection parameters.
 *  @param[in]  conn_param   Initial connection parameters.
 *  @param[out] conn         Valid connection object on success.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_le_create(const bt_addr_le_t *peer,
		      const struct bt_conn_le_create_param *create_param,
		      const struct bt_le_conn_param *conn_param,
		      struct bt_conn **conn);

/** @brief Automatically connect to remote devices in the filter accept list..
 *
 *  This uses the Auto Connection Establishment procedure.
 *  The procedure will continue until a single connection is established or the
 *  procedure is stopped through @ref bt_conn_create_auto_stop.
 *  To establish connections to all devices in the the filter accept list the
 *  procedure should be started again in the connected callback after a
 *  new connection has been established.
 *
 *  @param create_param Create connection parameters
 *  @param conn_param   Initial connection parameters.
 *
 *  @return Zero on success or (negative) error code on failure.
 *  @return -ENOMEM No free connection object available.
 */
int bt_conn_le_create_auto(const struct bt_conn_le_create_param *create_param,
			   const struct bt_le_conn_param *conn_param);

/** @brief Stop automatic connect creation.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_create_auto_stop(void);

/** @brief Automatically connect to remote device if it's in range.
 *
 *  This function enables/disables automatic connection initiation.
 *  Every time the device loses the connection with peer, this connection
 *  will be re-established if connectable advertisement from peer is received.
 *
 *  @note Auto connect is disabled during explicit scanning.
 *
 *  @param addr Remote Bluetooth address.
 *  @param param If non-NULL, auto connect is enabled with the given
 *  parameters. If NULL, auto connect is disabled.
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_le_set_auto_conn(const bt_addr_le_t *addr,
			const struct bt_le_conn_param *param);

/** @brief Set security level for a connection.
 *
 *  This function enable security (encryption) for a connection. If the device
 *  has bond information for the peer with sufficiently strong key encryption
 *  will be enabled. If the connection is already encrypted with sufficiently
 *  strong key this function does nothing.
 *
 *  If the device has no bond information for the peer and is not already paired
 *  then the pairing procedure will be initiated. Note that @p sec has no effect
 *  on the security level selected for the pairing process. The selection is
 *  instead controlled by the values of the registered @ref bt_conn_auth_cb. If
 *  the device has bond information or is already paired and the keys are too
 *  weak then the pairing procedure will be initiated.
 *
 *  This function may return error if required level of security is not possible
 *  to achieve due to local or remote device limitation (e.g., input output
 *  capabilities), or if the maximum number of paired devices has been reached.
 *
 *  This function may return error if the pairing procedure has already been
 *  initiated by the local device or the peer device.
 *
 *  @note When @kconfig{CONFIG_BT_SMP_SC_ONLY} is enabled then the security
 *        level will always be level 4.
 *
 *  @note When @kconfig{CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY} is enabled then the
 *        security level will always be level 3.
 *
 *  @param conn Connection object.
 *  @param sec Requested security level.
 *
 *  @return 0 on success or negative error
 */
int bt_conn_set_security(struct bt_conn *conn, bt_security_t sec);

/** @brief Get security level for a connection.
 *
 *  @return Connection security level
 */
bt_security_t bt_conn_get_security(const struct bt_conn *conn);

/** @brief Get encryption key size.
 *
 *  This function gets encryption key size.
 *  If there is no security (encryption) enabled 0 will be returned.
 *
 *  @param conn Existing connection object.
 *
 *  @return Encryption key size.
 */
uint8_t bt_conn_enc_key_size(const struct bt_conn *conn);

enum bt_security_err {
	/** Security procedure successful. */
	BT_SECURITY_ERR_SUCCESS,

	/** Authentication failed. */
	BT_SECURITY_ERR_AUTH_FAIL,

	/** PIN or encryption key is missing. */
	BT_SECURITY_ERR_PIN_OR_KEY_MISSING,

	/** OOB data is not available.  */
	BT_SECURITY_ERR_OOB_NOT_AVAILABLE,

	/** The requested security level could not be reached. */
	BT_SECURITY_ERR_AUTH_REQUIREMENT,

	/** Pairing is not supported */
	BT_SECURITY_ERR_PAIR_NOT_SUPPORTED,

	/** Pairing is not allowed. */
	BT_SECURITY_ERR_PAIR_NOT_ALLOWED,

	/** Invalid parameters. */
	BT_SECURITY_ERR_INVALID_PARAM,

	/** Distributed Key Rejected */
	BT_SECURITY_ERR_KEY_REJECTED,

	/** Pairing failed but the exact reason could not be specified. */
	BT_SECURITY_ERR_UNSPECIFIED,
};

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
	 *  @note If the connection was established from an advertising set then
	 *        the advertising set cannot be restarted directly from this
	 *        callback. Instead use the connected callback of the
	 *        advertising set.
	 *
	 *  @param conn New connection object.
	 *  @param err HCI error. Zero for success, non-zero otherwise.
	 *
	 *  @p err can mean either of the following:
	 *  - @ref BT_HCI_ERR_UNKNOWN_CONN_ID Creating the connection started by
	 *    @ref bt_conn_le_create was canceled either by the user through
	 *    @ref bt_conn_disconnect or by the timeout in the host through
	 *    @ref bt_conn_le_create_param timeout parameter, which defaults to
	 *    @kconfig{CONFIG_BT_CREATE_CONN_TIMEOUT} seconds.
	 *  - @p BT_HCI_ERR_ADV_TIMEOUT High duty cycle directed connectable
	 *    advertiser started by @ref bt_le_adv_start failed to be connected
	 *    within the timeout.
	 */
	void (*connected)(struct bt_conn *conn, uint8_t err);

	/** @brief A connection has been disconnected.
	 *
	 *  This callback notifies the application that a connection
	 *  has been disconnected.
	 *
	 *  When this callback is called the stack still has one reference to
	 *  the connection object. If the application in this callback tries to
	 *  start either a connectable advertiser or create a new connection
	 *  this might fail because there are no free connection objects
	 *  available.
	 *  To avoid this issue it is recommended to either start connectable
	 *  advertise or create a new connection using @ref k_work_submit or
	 *  increase @kconfig{CONFIG_BT_MAX_CONN}.
	 *
	 *  @param conn Connection object.
	 *  @param reason BT_HCI_ERR_* reason for the disconnection.
	 */
	void (*disconnected)(struct bt_conn *conn, uint8_t reason);

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
	 *  If the application does not have this callback then the default
	 *  is to accept the parameters.
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
	void (*le_param_updated)(struct bt_conn *conn, uint16_t interval,
				 uint16_t latency, uint16_t timeout);
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
	 *  This callback notifies the application that the security of a
	 *  connection has changed.
	 *
	 *  The security level of the connection can either have been increased
	 *  or remain unchanged. An increased security level means that the
	 *  pairing procedure has been performed or the bond information from
	 *  a previous connection has been applied. If the security level
	 *  remains unchanged this means that the encryption key has been
	 *  refreshed for the connection.
	 *
	 *  @param conn Connection object.
	 *  @param level New security level of the connection.
	 *  @param err Security error. Zero for success, non-zero otherwise.
	 */
	void (*security_changed)(struct bt_conn *conn, bt_security_t level,
				 enum bt_security_err err);
#endif /* defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR) */

#if defined(CONFIG_BT_REMOTE_INFO)
	/** @brief Remote information procedures has completed.
	 *
	 *  This callback notifies the application that the remote information
	 *  has been retrieved from the remote peer.
	 *
	 *  @param conn Connection object.
	 *  @param remote_info Connection information of remote device.
	 */
	void (*remote_info_available)(struct bt_conn *conn,
				      struct bt_conn_remote_info *remote_info);
#endif /* defined(CONFIG_BT_REMOTE_INFO) */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	/** @brief The PHY of the connection has changed.
	 *
	 *  This callback notifies the application that the PHY of the
	 *  connection has changed.
	 *
	 *  @param conn Connection object.
	 *  @param info Connection LE PHY information.
	 */
	void (*le_phy_updated)(struct bt_conn *conn,
			       struct bt_conn_le_phy_info *param);
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	/** @brief The data length parameters of the connection has changed.
	 *
	 *  This callback notifies the application that the maximum Link Layer
	 *  payload length or transmission time has changed.
	 *
	 *  @param conn Connection object.
	 *  @param info Connection data length information.
	 */
	void (*le_data_len_updated)(struct bt_conn *conn,
				    struct bt_conn_le_data_len_info *info);
#endif /* defined(CONFIG_BT_USER_DATA_LEN_UPDATE) */

#if defined(CONFIG_BT_DF_CONNECTION_CTE_RX)
	/** @brief Callback for IQ samples report collected when sampling
	 *        CTE received by data channel PDU.
	 *
	 * @param conn      The connection object.
	 * @param iq_report Report data for collected IQ samples.
	 */
	void (*cte_report_cb)(struct bt_conn *conn,
			      const struct bt_df_conn_iq_samples_report *iq_report);
#endif /* CONFIG_BT_DF_CONNECTION_CTE_RX */

	struct bt_conn_cb *_next;
};

/** @brief Register connection callbacks.
 *
 *  Register callbacks to monitor the state of connections.
 *
 *  @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_conn_cb_register(struct bt_conn_cb *cb);

/**
 *  @brief Register a callback structure for connection events.
 *
 *  @param _name Name of callback structure.
 */
#define BT_CONN_CB_DEFINE(_name)					\
	static const STRUCT_SECTION_ITERABLE(bt_conn_cb,		\
						_CONCAT(bt_conn_cb_,	\
							_name))

/** @brief Enable/disable bonding.
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

/** @brief Allow/disallow remote LE SC OOB data to be used for pairing.
 *
 *  Set/clear the OOB data flag for LE SC SMP Pairing Request/Response data.
 *
 *  @param enable Value allowing/disallowing remote LE SC OOB data.
 */
void bt_le_oob_set_sc_flag(bool enable);

/** @brief Allow/disallow remote legacy OOB data to be used for pairing.
 *
 *  Set/clear the OOB data flag for legacy SMP Pairing Request/Response data.
 *
 *  @param enable Value allowing/disallowing remote legacy OOB data.
 */
void bt_le_oob_set_legacy_flag(bool enable);

/** @brief Set OOB Temporary Key to be used for pairing
 *
 *  This function allows to set OOB data for the LE legacy pairing procedure.
 *  The function should only be called in response to the oob_data_request()
 *  callback provided that the legacy method is user pairing.
 *
 *  @param conn Connection object
 *  @param tk Pointer to 16 byte long TK array
 *
 *  @return Zero on success or -EINVAL if NULL
 */
int bt_le_oob_set_legacy_tk(struct bt_conn *conn, const uint8_t *tk);

/** @brief Set OOB data during LE Secure Connections (SC) pairing procedure
 *
 *  This function allows to set OOB data during the LE SC pairing procedure.
 *  The function should only be called in response to the oob_data_request()
 *  callback provided that LE SC method is used for pairing.
 *
 *  The user should submit OOB data according to the information received in the
 *  callback. This may yield three different configurations: with only local OOB
 *  data present, with only remote OOB data present or with both local and
 *  remote OOB data present.
 *
 *  @param conn Connection object
 *  @param oobd_local Local OOB data or NULL if not present
 *  @param oobd_remote Remote OOB data or NULL if not present
 *
 *  @return Zero on success or error code otherwise, positive in case of
 *          protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_oob_set_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data *oobd_local,
			  const struct bt_le_oob_sc_data *oobd_remote);

/** @brief Get OOB data used for LE Secure Connections (SC) pairing procedure
 *
 *  This function allows to get OOB data during the LE SC pairing procedure that
 *  were set by the bt_le_oob_set_sc_data() API.
 *
 *  @note The OOB data will only be available as long as the connection object
 *  associated with it is valid.
 *
 *  @param conn Connection object
 *  @param oobd_local Local OOB data or NULL if not set
 *  @param oobd_remote Remote OOB data or NULL if not set
 *
 *  @return Zero on success or error code otherwise, positive in case of
 *          protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_oob_get_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data **oobd_local,
			  const struct bt_le_oob_sc_data **oobd_remote);

/**
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
 *  pairing_confirm() callback will be called for all incoming pairings.
 *
 *  @param passkey A valid passkey (0 - 999999) or BT_PASSKEY_INVALID
 *                 to disable a previously set fixed passkey.
 *
 *  @return 0 on success or a negative error code on failure.
 */
int bt_passkey_set(unsigned int passkey);

/** Info Structure for OOB pairing */
struct bt_conn_oob_info {
	/** Type of OOB pairing method */
	enum {
		/** LE legacy pairing */
		BT_CONN_OOB_LE_LEGACY,

		/** LE SC pairing */
		BT_CONN_OOB_LE_SC,
	} type;

	union {
		/** LE Secure Connections OOB pairing parameters */
		struct {
			/** OOB data configuration */
			enum {
				/** Local OOB data requested */
				BT_CONN_OOB_LOCAL_ONLY,

				/** Remote OOB data requested */
				BT_CONN_OOB_REMOTE_ONLY,

				/** Both local and remote OOB data requested */
				BT_CONN_OOB_BOTH_PEERS,

				/** No OOB data requested */
				BT_CONN_OOB_NO_DATA,
			} oob_config;
		} lesc;
	};
};

#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
/** @brief Pairing request and pairing response info structure.
 *
 *  This structure is the same for both smp_pairing_req and smp_pairing_rsp
 *  and a subset of the packet data, except for the initial Code octet.
 *  It is documented in Core Spec. Vol. 3, Part H, 3.5.1 and 3.5.2.
 */
struct bt_conn_pairing_feat {
	/** IO Capability, Core Spec. Vol 3, Part H, 3.5.1, Table 3.4 */
	uint8_t io_capability;

	/** OOB data flag, Core Spec. Vol 3, Part H, 3.5.1, Table 3.5 */
	uint8_t oob_data_flag;

	/** AuthReq, Core Spec. Vol 3, Part H, 3.5.1, Fig. 3.3 */
	uint8_t auth_req;

	/** Maximum Encryption Key Size, Core Spec. Vol 3, Part H, 3.5.1 */
	uint8_t max_enc_key_size;

	/** Initiator Key Distribution/Generation, Core Spec. Vol 3, Part H,
	 *  3.6.1, Fig. 3.11
	 */
	uint8_t init_key_dist;

	/** Responder Key Distribution/Generation, Core Spec. Vol 3, Part H
	 *  3.6.1, Fig. 3.11
	 */
	uint8_t resp_key_dist;
};
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */

/** Authenticated pairing callback structure */
struct bt_conn_auth_cb {
#if defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)
	/** @brief Query to proceed incoming pairing or not.
	 *
	 *  On any incoming pairing req/rsp this callback will be called for
	 *  the application to decide whether to allow for the pairing to
	 *  continue.
	 *
	 *  The pairing info received from the peer is passed to assist
	 *  making the decision.
	 *
	 *  As this callback is synchronous the application should return
	 *  a response value immediately. Otherwise it may affect the
	 *  timing during pairing. Hence, this information should not be
	 *  conveyed to the user to take action.
	 *
	 *  The remaining callbacks are not affected by this, but do notice
	 *  that other callbacks can be called during the pairing. Eg. if
	 *  pairing_confirm is registered both will be called for Just-Works
	 *  pairings.
	 *
	 *  This callback may be unregistered in which case pairing continues
	 *  as if the Kconfig flag was not set.
	 *
	 *  This callback is not called for BR/EDR Secure Simple Pairing (SSP).
	 *
	 *  @param conn Connection where pairing is initiated.
	 *  @param feat Pairing req/resp info.
	 */
	enum bt_security_err (*pairing_accept)(struct bt_conn *conn,
			      const struct bt_conn_pairing_feat *const feat);
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */

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

#if defined(CONFIG_BT_PASSKEY_KEYPRESS)
	/** @brief Receive Passkey Keypress Notification during pairing
	 *
	 *  This allows the remote device to use the local device to give users
	 *  feedback on the progress of entering the passkey over there. This is
	 *  useful when the remote device itself has no display suitable for
	 *  showing the progress.
	 *
	 *  The handler of this callback is expected to keep track of the number
	 *  of digits entered and show a password-field-like feedback to the
	 *  user.
	 *
	 *  This callback is only relevant while the local side does Passkey
	 *  Display.
	 *
	 *  The event type is verified to be in range of the enum. No other
	 *  sanitization has been done. The remote could send a large number of
	 *  events of any type in any order.
	 *
	 *  @param conn The related connection.
	 *  @param type Type of event. Verified in range of the enum.
	 */
	void (*passkey_display_keypress)(struct bt_conn *conn,
					 enum bt_conn_auth_keypress type);
#endif

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

	/** @brief Request the user to provide Out of Band (OOB) data.
	 *
	 *  When called the user is expected to provide OOB data. The required
	 *  data are indicated by the information structure.
	 *
	 *  For LE Secure Connections OOB pairing, the user should provide
	 *  local OOB data, remote OOB data or both depending on their
	 *  availability. Their value should be given to the stack using the
	 *  bt_le_oob_set_sc_data() API.
	 *
	 *  This callback must be set to non-NULL in order to support OOB
	 *  pairing.
	 *
	 *  @param conn Connection where pairing is currently active.
	 *  @param info OOB pairing information.
	 */
	void (*oob_data_request)(struct bt_conn *conn,
				 struct bt_conn_oob_info *info);

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
};

/** Authenticated pairing information callback structure */
struct bt_conn_auth_info_cb {
	/** @brief notify that pairing procedure was complete.
	 *
	 *  This callback notifies the application that the pairing procedure
	 *  has been completed.
	 *
	 *  @param conn Connection object.
	 *  @param bonded Bond information has been distributed during the
	 *                pairing procedure.
	 */
	void (*pairing_complete)(struct bt_conn *conn, bool bonded);

	/** @brief notify that pairing process has failed.
	 *
	 *  @param conn Connection object.
	 *  @param reason Pairing failed reason
	 */
	void (*pairing_failed)(struct bt_conn *conn,
			       enum bt_security_err reason);

	/** @brief Notify that bond has been deleted.
	 *
	 *  This callback notifies the application that the bond information
	 *  for the remote peer has been deleted
	 *
	 *  @param id   Which local identity had the bond.
	 *  @param peer Remote address.
	 */
	void (*bond_deleted)(uint8_t id, const bt_addr_le_t *peer);

	/** Internally used field for list handling */
	sys_snode_t node;
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

/** @brief Overlay authentication callbacks used for a given connection.
 *
 *  This function can be used only for Bluetooth LE connections.
 *  The @kconfig{CONFIG_BT_SMP} must be enabled for this function.
 *
 *  The authentication callbacks for a given connection cannot be overlaid if
 *  security procedures in the SMP module have already started. This function
 *  can be called only once per connection.
 *
 *  @param conn	Connection object.
 *  @param cb	Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_cb_overlay(struct bt_conn *conn, const struct bt_conn_auth_cb *cb);

/** @brief Register authentication information callbacks.
 *
 *  Register callbacks to get authenticated pairing information. Multiple
 *  registrations can be done.
 *
 *  @param cb Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_info_cb_register(struct bt_conn_auth_info_cb *cb);

/** @brief Unregister authentication information callbacks.
 *
 *  Unregister callbacks to stop getting authenticated pairing information.
 *
 *  @param cb Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_conn_auth_info_cb_unregister(struct bt_conn_auth_info_cb *cb);

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

/** @brief Send Passkey Keypress Notification during pairing
 *
 *  This function may be called only after passkey_entry callback from
 *  bt_conn_auth_cb structure was called.
 *
 *  Requires @kconfig{CONFIG_BT_PASSKEY_KEYPRESS}.
 *
 *  @param conn Destination for the notification.
 *  @param type What keypress event type to send. @see bt_conn_auth_keypress.
 *
 *  @retval 0 Success
 *  @retval -EINVAL Improper use of the API.
 *  @retval -ENOMEM Failed to allocate.
 *  @retval -ENOBUFS Failed to allocate.
 */
int bt_conn_auth_keypress_notify(struct bt_conn *conn, enum bt_conn_auth_keypress type);

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

/** @brief Initialize BR/EDR connection parameters
 *
 *  @param role_switch True if role switch is allowed
 */
#define BT_BR_CONN_PARAM_INIT(role_switch) \
{ \
	.allow_role_switch = (role_switch), \
}

/** Helper to declare BR/EDR connection parameters inline
  *
  * @param role_switch True if role switch is allowed
  */
#define BT_BR_CONN_PARAM(role_switch) \
	((struct bt_br_conn_param[]) { \
		BT_BR_CONN_PARAM_INIT(role_switch) \
	 })

/** Default BR/EDR connection parameters:
 *    Role switch allowed
 */
#define BT_BR_CONN_PARAM_DEFAULT BT_BR_CONN_PARAM(true)


/** @brief Initiate an BR/EDR connection to a remote device.
 *
 *  Allows initiate new BR/EDR link to remote peer using its address.
 *
 *  The caller gets a new reference to the connection object which must be
 *  released with bt_conn_unref() once done using the object.
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
 *
 *  The caller gets a new reference to the connection object which must be
 *  released with bt_conn_unref() once done using the object.
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
