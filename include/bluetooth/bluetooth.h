/** @file
 *  @brief Bluetooth subsystem core APIs.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_

/**
 * @brief Bluetooth APIs
 * @defgroup bluetooth Bluetooth APIs
 * @{
 */

#include <stdbool.h>
#include <string.h>
#include <sys/util.h>
#include <net/buf.h>
#include <bluetooth/gap.h>
#include <bluetooth/addr.h>
#include <bluetooth/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic Access Profile
 * @defgroup bt_gap Generic Access Profile
 * @ingroup bluetooth
 * @{
 */

/**
 * @def BT_ID_DEFAULT
 *
 * Convenience macro for specifying the default identity. This helps
 * make the code more readable, especially when only one identity is
 * supported.
 */
#define BT_ID_DEFAULT 0

/** Opaque type representing an advertiser. */
struct bt_le_ext_adv;

/** Opaque type representing an periodic advertising sync. */
struct bt_le_per_adv_sync;

/* Don't require everyone to include conn.h */
struct bt_conn;

struct bt_le_ext_adv_sent_info {
	/** The number of advertising events completed. */
	uint8_t num_sent;
};

struct bt_le_ext_adv_connected_info {
	/** Connection object of the new connection */
	struct bt_conn *conn;
};

struct bt_le_ext_adv_scanned_info {
	/** Active scanner LE address and type */
	bt_addr_le_t *addr;
};

struct bt_le_ext_adv_cb {
	/**
	 * @brief The advertising set has finished sending adv data.
	 *
	 * This callback notifies the application that the advertising set has
	 * finished sending advertising data.
	 * The advertising set can either have been stopped by a timeout or
	 * because the specified number of advertising events has been reached.
	 *
	 * @param adv  The advertising set object.
	 * @param info Information about the sent event.
	 */
	void (*sent)(struct bt_le_ext_adv *adv,
		     struct bt_le_ext_adv_sent_info *info);

	/**
	 * @brief The advertising set has accepted a new connection.
	 *
	 * This callback notifies the application that the advertising set has
	 * accepted a new connection.
	 *
	 * @param adv  The advertising set object.
	 * @param info Information about the connected event.
	 */
	void (*connected)(struct bt_le_ext_adv *adv,
			  struct bt_le_ext_adv_connected_info *info);

	/**
	 * @brief The advertising set has sent scan response data.
	 *
	 * This callback notifies the application that the advertising set has
	 * has received a Scan Request packet, and has sent a Scan Response
	 * packet.
	 *
	 * @param adv  The advertising set object.
	 * @param addr Information about the scanned event.
	 */
	void (*scanned)(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_scanned_info *info);
};

/**
 * @typedef bt_ready_cb_t
 * @brief Callback for notifying that Bluetooth has been enabled.
 *
 * @param err zero on success or (negative) error code otherwise.
 */
typedef void (*bt_ready_cb_t)(int err);

/**
 * @brief Enable Bluetooth
 *
 * Enable Bluetooth. Must be the called before any calls that
 * require communication with the local Bluetooth hardware.
 *
 * @param cb Callback to notify completion or NULL to perform the
 * enabling synchronously.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_enable(bt_ready_cb_t cb);

/**
 * @brief Set Bluetooth Device Name
 *
 * Set Bluetooth GAP Device Name.
 *
 * When advertising with device name in the advertising data the name should
 * be updated by calling @ref bt_le_adv_update_data or
 * @ref bt_le_ext_adv_set_data.
 *
 * @param name New name
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_set_name(const char *name);

/**
 * @brief Get Bluetooth Device Name
 *
 * Get Bluetooth GAP Device Name.
 *
 * @return Bluetooth Device Name
 */
const char *bt_get_name(void);

/**
 * @brief Set the local Identity Address
 *
 * Allows setting the local Identity Address from the application.
 * This API must be called before calling bt_enable(). Calling it at any
 * other time will cause it to fail. In most cases the application doesn't
 * need to use this API, however there are a few valid cases where
 * it can be useful (such as for testing).
 *
 * At the moment, the given address must be a static random address. In the
 * future support for public addresses may be added.
 *
 * @deprecated in 2.5 release, replace with bt_id_create before bt_enable.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
__deprecated int bt_set_id_addr(const bt_addr_le_t *addr);

/**
 * @brief Get the currently configured identities.
 *
 * Returns an array of the currently configured identity addresses. To
 * make sure all available identities can be retrieved, the number of
 * elements in the @a addrs array should be CONFIG_BT_ID_MAX. The identity
 * identifier that some APIs expect (such as advertising parameters) is
 * simply the index of the identity in the @a addrs array.
 *
 * If @a addrs is passed as NULL, then returned @a count contains the
 * count of all available identities that can be retrieved with a
 * subsequent call to this function with non-NULL @a addrs parameter.
 *
 * @note Deleted identities may show up as BT_LE_ADDR_ANY in the returned
 * array.
 *
 * @param addrs Array where to store the configured identities.
 * @param count Should be initialized to the array size. Once the function
 *              returns it will contain the number of returned identities.
 */
void bt_id_get(bt_addr_le_t *addrs, size_t *count);

/**
 * @brief Create a new identity.
 *
 * Create a new identity using the given address and IRK. This function
 * can be called before calling bt_enable(), in which case it can be used
 * to override the controller's public address (in case it has one). However,
 * the new identity will only be stored persistently in flash when this API
 * is used after bt_enable(). The reason is that the persistent settings
 * are loaded after bt_enable() and would therefore cause potential conflicts
 * with the stack blindly overwriting what's stored in flash. The identity
 * will also not be written to flash in case a pre-defined address is
 * provided, since in such a situation the app clearly has some place it got
 * the address from and will be able to repeat the procedure on every power
 * cycle, i.e. it would be redundant to also store the information in flash.
 *
 * Generating random static address or random IRK is not supported when calling
 * this function before bt_enable().
 *
 * If the application wants to have the stack randomly generate identities
 * and store them in flash for later recovery, the way to do it would be
 * to first initialize the stack (using bt_enable), then call settings_load(),
 * and after that check with bt_id_get() how many identities were recovered.
 * If an insufficient amount of identities were recovered the app may then
 * call bt_id_create() to create new ones.
 *
 * @param addr Address to use for the new identity. If NULL or initialized
 *             to BT_ADDR_LE_ANY the stack will generate a new random
 *             static address for the identity and copy it to the given
 *             parameter upon return from this function (in case the
 *             parameter was non-NULL).
 * @param irk  Identity Resolving Key (16 bytes) to be used with this
 *             identity. If set to all zeroes or NULL, the stack will
 *             generate a random IRK for the identity and copy it back
 *             to the parameter upon return from this function (in case
 *             the parameter was non-NULL). If privacy
 *             @option{CONFIG_BT_PRIVACY} is not enabled this parameter must
 *             be NULL.
 *
 * @return Identity identifier (>= 0) in case of success, or a negative
 *         error code on failure.
 */
int bt_id_create(bt_addr_le_t *addr, uint8_t *irk);

/**
 * @brief Reset/reclaim an identity for reuse.
 *
 * The semantics of the @a addr and @a irk parameters of this function
 * are the same as with bt_id_create(). The difference is the first
 * @a id parameter that needs to be an existing identity (if it doesn't
 * exist this function will return an error). When given an existing
 * identity this function will disconnect any connections created using it,
 * remove any pairing keys or other data associated with it, and then create
 * a new identity in the same slot, based on the @a addr and @a irk
 * parameters.
 *
 * @note the default identity (BT_ID_DEFAULT) cannot be reset, i.e. this
 * API will return an error if asked to do that.
 *
 * @param id   Existing identity identifier.
 * @param addr Address to use for the new identity. If NULL or initialized
 *             to BT_ADDR_LE_ANY the stack will generate a new static
 *             random address for the identity and copy it to the given
 *             parameter upon return from this function (in case the
 *             parameter was non-NULL).
 * @param irk  Identity Resolving Key (16 bytes) to be used with this
 *             identity. If set to all zeroes or NULL, the stack will
 *             generate a random IRK for the identity and copy it back
 *             to the parameter upon return from this function (in case
 *             the parameter was non-NULL). If privacy
 *             @option{CONFIG_BT_PRIVACY} is not enabled this parameter must
 *             be NULL.
 *
 * @return Identity identifier (>= 0) in case of success, or a negative
 *         error code on failure.
 */
int bt_id_reset(uint8_t id, bt_addr_le_t *addr, uint8_t *irk);

/**
 * @brief Delete an identity.
 *
 * When given a valid identity this function will disconnect any connections
 * created using it, remove any pairing keys or other data associated with
 * it, and then flag is as deleted, so that it can not be used for any
 * operations. To take back into use the slot the identity was occupying the
 * bt_id_reset() API needs to be used.
 *
 * @note the default identity (BT_ID_DEFAULT) cannot be deleted, i.e. this
 * API will return an error if asked to do that.
 *
 * @param id   Existing identity identifier.
 *
 * @return 0 in case of success, or a negative error code on failure.
 */
int bt_id_delete(uint8_t id);

/**
 * @brief Bluetooth data.
 *
 * Description of different data types that can be encoded into
 * advertising data. Used to form arrays that are passed to the
 * bt_le_adv_start() function.
 */
struct bt_data {
	uint8_t type;
	uint8_t data_len;
	const uint8_t *data;
};

/**
 * @brief Helper to declare elements of bt_data arrays
 *
 * This macro is mainly for creating an array of struct bt_data
 * elements which is then passed to e.g. @ref bt_le_adv_start().
 *
 * @param _type Type of advertising data field
 * @param _data Pointer to the data field payload
 * @param _data_len Number of bytes behind the _data pointer
 */
#define BT_DATA(_type, _data, _data_len) \
	{ \
		.type = (_type), \
		.data_len = (_data_len), \
		.data = (const uint8_t *)(_data), \
	}

/**
 * @brief Helper to declare elements of bt_data arrays
 *
 * This macro is mainly for creating an array of struct bt_data
 * elements which is then passed to e.g. @ref bt_le_adv_start().
 *
 * @param _type Type of advertising data field
 * @param _bytes Variable number of single-byte parameters
 */
#define BT_DATA_BYTES(_type, _bytes...) \
	BT_DATA(_type, ((uint8_t []) { _bytes }), \
		sizeof((uint8_t []) { _bytes }))

/** Advertising options */
enum {
	/** Convenience value when no options are specified. */
	BT_LE_ADV_OPT_NONE = 0,

	/**
	 * @brief Advertise as connectable.
	 *
	 * Advertise as connectable. If not connectable then the type of
	 * advertising is determined by providing scan response data.
	 * The advertiser address is determined by the type of advertising
	 * and/or enabling privacy @option{CONFIG_BT_PRIVACY}.
	 */
	BT_LE_ADV_OPT_CONNECTABLE = BIT(0),

	/**
	 * @brief Advertise one time.
	 *
	 * Don't try to resume connectable advertising after a connection.
	 * This option is only meaningful when used together with
	 * BT_LE_ADV_OPT_CONNECTABLE. If set the advertising will be stopped
	 * when bt_le_adv_stop() is called or when an incoming (slave)
	 * connection happens. If this option is not set the stack will
	 * take care of keeping advertising enabled even as connections
	 * occur.
	 * If Advertising directed or the advertiser was started with
	 * @ref bt_le_ext_adv_start then this behavior is the default behavior
	 * and this flag has no effect.
	 */
	BT_LE_ADV_OPT_ONE_TIME = BIT(1),

	/**
	 * @brief Advertise using identity address.
	 *
	 * Advertise using the identity address as the advertiser address.
	 * @warning This will compromise the privacy of the device, so care
	 *          must be taken when using this option.
	 * @note The address used for advertising will not be the same as
	 *        returned by @ref bt_le_oob_get_local, instead @ref bt_id_get
	 *        should be used to get the LE address.
	 */
	BT_LE_ADV_OPT_USE_IDENTITY = BIT(2),

	/** Advertise using GAP device name.
	 *
	 *  Include the GAP device name automatically when advertising.
	 *  By default the GAP device name is put at the end of the scan
	 *  response data.
	 *  When advertising using @ref BT_LE_ADV_OPT_EXT_ADV and not
	 *  @ref BT_LE_ADV_OPT_SCANNABLE then it will be put at the end of the
	 *  advertising data.
	 *  If the GAP device name does not fit into advertising data it will be
	 *  converted to a shortened name if possible.
	 *
	 *  The application can set the device name itself by including the
	 *  following in the advertising data.
	 *  @code
	 *  BT_DATA(BT_DATA_NAME_COMPLETE, name, strlen(name))
	 *  @endcode
	 */
	BT_LE_ADV_OPT_USE_NAME = BIT(3),

	/**
	 * @brief Low duty cycle directed advertising.
	 *
	 * Use low duty directed advertising mode, otherwise high duty mode
	 * will be used.
	 */
	BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY = BIT(4),

	/**
	 * @brief Directed advertising to privacy-enabled peer.
	 *
	 * Enable use of Resolvable Private Address (RPA) as the target address
	 * in directed advertisements.
	 * This is required if the remote device is privacy-enabled and
	 * supports address resolution of the target address in directed
	 * advertisement.
	 * It is the responsibility of the application to check that the remote
	 * device supports address resolution of directed advertisements by
	 * reading its Central Address Resolution characteristic.
	 */
	BT_LE_ADV_OPT_DIR_ADDR_RPA = BIT(5),

	/** Use whitelist to filter devices that can request scan response data.
	 */
	BT_LE_ADV_OPT_FILTER_SCAN_REQ = BIT(6),

	/** Use whitelist to filter devices that can connect. */
	BT_LE_ADV_OPT_FILTER_CONN = BIT(7),

	/** Notify the application when a scan response data has been sent to an
	 *  active scanner.
	 */
	BT_LE_ADV_OPT_NOTIFY_SCAN_REQ = BIT(8),

	/**
	 * @brief Support scan response data.
	 *
	 * When used together with @ref BT_LE_ADV_OPT_EXT_ADV then this option
	 * cannot be used together with the @ref BT_LE_ADV_OPT_CONNECTABLE
	 * option.
	 * When used together with @ref BT_LE_ADV_OPT_EXT_ADV then scan
	 * response data must be set.
	 */
	BT_LE_ADV_OPT_SCANNABLE = BIT(9),

	/**
	 * @brief Advertise with extended advertising.
	 *
	 * This options enables extended advertising in the advertising set.
	 * In extended advertising the advertising set will send a small header
	 * packet on the three primary advertising channels. This small header
	 * points to the advertising data packet that will be sent on one of
	 * the 37 secondary advertising channels.
	 * The advertiser will send primary advertising on LE 1M PHY, and
	 * secondary advertising on LE 2M PHY.
	 * Connections will be established on LE 2M PHY.
	 *
	 * Without this option the advertiser will send advertising data on the
	 * three primary advertising channels.
	 *
	 * @note Enabling this option requires extended advertising support in
	 *       the peer devices scanning for advertisement packets.
	 */
	BT_LE_ADV_OPT_EXT_ADV = BIT(10),

	/**
	 * @brief Disable use of LE 2M PHY on the secondary advertising
	 * channel.
	 *
	 * Disabling the use of LE 2M PHY could be necessary if scanners don't
	 * support the LE 2M PHY.
	 * The advertiser will send primary advertising on LE 1M PHY, and
	 * secondary advertising on LE 1M PHY.
	 * Connections will be established on LE 1M PHY.
	 *
	 * @note Cannot be set if BT_LE_ADV_OPT_CODED is set.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV.
	 */
	BT_LE_ADV_OPT_NO_2M = BIT(11),

	/**
	 * @brief Advertise on the LE Coded PHY (Long Range).
	 *
	 * The advertiser will send both primary and secondary advertising
	 * on the LE Coded PHY. This gives the advertiser increased range with
	 * the trade-off of lower data rate and higher power consumption.
	 * Connections will be established on LE Coded PHY.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	BT_LE_ADV_OPT_CODED = BIT(12),

	/**
	 * @brief Advertise without a device address (identity or RPA).
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	BT_LE_ADV_OPT_ANONYMOUS = BIT(13),

	/**
	 * @brief Advertise with transmit power.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	BT_LE_ADV_OPT_USE_TX_POWER = BIT(14),

	/** Disable advertising on channel index 37. */
	BT_LE_ADV_OPT_DISABLE_CHAN_37 = BIT(15),

	/** Disable advertising on channel index 38. */
	BT_LE_ADV_OPT_DISABLE_CHAN_38 = BIT(16),

	/** Disable advertising on channel index 39. */
	BT_LE_ADV_OPT_DISABLE_CHAN_39 = BIT(17),
};

/** LE Advertising Parameters. */
struct bt_le_adv_param {
	/**
	 * @brief Local identity.
	 *
	 * @note When extended advertising @option{CONFIG_BT_EXT_ADV} is not
	 *       enabled or not supported by the controller it is not possible
	 *       to scan and advertise simultaneously using two different
	 *       random addresses.
	 *
	 * @note It is not possible to have multiple connectable advertising
	 *       sets advertising simultaneously using different identities.
	 */
	uint8_t  id;

	/**
	 * @brief Advertising Set Identifier, valid range 0x00 - 0x0f.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 **/
	uint8_t  sid;

	/**
	 * @brief Secondary channel maximum skip count.
	 *
	 * Maximum advertising events the advertiser can skip before it must
	 * send advertising data on the secondary advertising channel.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	uint8_t  secondary_max_skip;

	/** Bit-field of advertising options */
	uint32_t options;

	/** Minimum Advertising Interval (N * 0.625 milliseconds)
	 * Minimum Advertising Interval shall be less than or equal to the
	 * Maximum Advertising Interval. The Minimum Advertising Interval and
	 * Maximum Advertising Interval should not be the same value (as stated
	 * in Bluetooth Core Spec 5.2, section 7.8.5)
	 * Range: 0x0020 to 0x4000
	 */
	uint32_t interval_min;

	/** Maximum Advertising Interval (N * 0.625 milliseconds)
	 * Minimum Advertising Interval shall be less than or equal to the
	 * Maximum Advertising Interval. The Minimum Advertising Interval and
	 * Maximum Advertising Interval should not be the same value (as stated
	 * in Bluetooth Core Spec 5.2, section 7.8.5)
	 * Range: 0x0020 to 0x4000
	 */
	uint32_t interval_max;

	/**
	 * @brief Directed advertising to peer
	 *
	 * When this parameter is set the advertiser will send directed
	 * advertising to the remote device.
	 *
	 * The advertising type will either be high duty cycle, or low duty
	 * cycle if the BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY option is enabled.
	 * When using @ref BT_LE_ADV_OPT_EXT_ADV then only low duty cycle is
	 * allowed.
	 *
	 * In case of connectable high duty cycle if the connection could not
	 * be established within the timeout the connected() callback will be
	 * called with the status set to @ref BT_HCI_ERR_ADV_TIMEOUT.
	 */
	const bt_addr_le_t *peer;
};


/** Periodic Advertising options */
enum {
	/** Convenience value when no options are specified. */
	BT_LE_PER_ADV_OPT_NONE = 0,

	/**
	 * @brief Advertise with transmit power.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV
	 */
	BT_LE_PER_ADV_OPT_USE_TX_POWER = BIT(1),
};

struct bt_le_per_adv_param {
	/** Minimum Periodic Advertising Interval (N * 1.25 ms) */
	uint16_t interval_min;

	/** Maximum Periodic Advertising Interval (N * 1.25 ms) */
	uint16_t interval_max;

	/** Bit-field of periodic advertising options */
	uint32_t options;
};

/**
 * @brief Initialize advertising parameters
 *
 * @param _options   Advertising Options
 * @param _int_min   Minimum advertising interval
 * @param _int_max   Maximum advertising interval
 * @param _peer      Peer address, set to NULL for undirected advertising or
 *                   address of peer for directed advertising.
 */
#define BT_LE_ADV_PARAM_INIT(_options, _int_min, _int_max, _peer) \
{ \
	.id = BT_ID_DEFAULT, \
	.sid = 0, \
	.secondary_max_skip = 0, \
	.options = (_options), \
	.interval_min = (_int_min), \
	.interval_max = (_int_max), \
	.peer = (_peer), \
}

/**
 * @brief Helper to declare advertising parameters inline
 *
 * @param _options   Advertising Options
 * @param _int_min   Minimum advertising interval
 * @param _int_max   Maximum advertising interval
 * @param _peer      Peer address, set to NULL for undirected advertising or
 *                   address of peer for directed advertising.
 */
#define BT_LE_ADV_PARAM(_options, _int_min, _int_max, _peer) \
	((struct bt_le_adv_param[]) { \
		BT_LE_ADV_PARAM_INIT(_options, _int_min, _int_max, _peer) \
	 })

#define BT_LE_ADV_CONN_DIR(_peer) BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE |  \
						  BT_LE_ADV_OPT_ONE_TIME, 0, 0,\
						  _peer)


#define BT_LE_ADV_CONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				       BT_GAP_ADV_FAST_INT_MIN_2, \
				       BT_GAP_ADV_FAST_INT_MAX_2, NULL)

#define BT_LE_ADV_CONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
					    BT_LE_ADV_OPT_USE_NAME, \
					    BT_GAP_ADV_FAST_INT_MIN_2, \
					    BT_GAP_ADV_FAST_INT_MAX_2, NULL)

#define BT_LE_ADV_CONN_DIR_LOW_DUTY(_peer) \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME | \
			BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY, \
			BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, \
			_peer)

/** Non-connectable advertising with private address */
#define BT_LE_ADV_NCONN BT_LE_ADV_PARAM(0, BT_GAP_ADV_FAST_INT_MIN_2, \
					BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/** Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_NAME */
#define BT_LE_ADV_NCONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME, \
					     BT_GAP_ADV_FAST_INT_MIN_2, \
					     BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/** Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY */
#define BT_LE_ADV_NCONN_IDENTITY BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_FAST_INT_MIN_2, \
						 BT_GAP_ADV_FAST_INT_MAX_2, \
						 NULL)

/** Non-connectable extended advertising with private address */
#define BT_LE_EXT_ADV_NCONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV, \
					    BT_GAP_ADV_FAST_INT_MIN_2, \
					    BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/** Non-connectable extended advertising with @ref BT_LE_ADV_OPT_USE_NAME */
#define BT_LE_EXT_ADV_NCONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
						 BT_LE_ADV_OPT_USE_NAME, \
						 BT_GAP_ADV_FAST_INT_MIN_2, \
						 BT_GAP_ADV_FAST_INT_MAX_2, \
						 NULL)

/** Non-connectable extended advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY */
#define BT_LE_EXT_ADV_NCONN_IDENTITY \
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
				BT_LE_ADV_OPT_USE_IDENTITY, \
				BT_GAP_ADV_FAST_INT_MIN_2, \
				BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/** Non-connectable extended advertising on coded PHY with private address */
#define BT_LE_EXT_ADV_CODED_NCONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
						  BT_LE_ADV_OPT_CODED, \
						  BT_GAP_ADV_FAST_INT_MIN_2, \
						  BT_GAP_ADV_FAST_INT_MAX_2, \
						  NULL)

/** Non-connectable extended advertising on coded PHY with
 *  @ref BT_LE_ADV_OPT_USE_NAME
 */
#define BT_LE_EXT_ADV_CODED_NCONN_NAME \
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CODED | \
				BT_LE_ADV_OPT_USE_NAME, \
				BT_GAP_ADV_FAST_INT_MIN_2, \
				BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/** Non-connectable extended advertising on coded PHY with
 *  @ref BT_LE_ADV_OPT_USE_IDENTITY
 */
#define BT_LE_EXT_ADV_CODED_NCONN_IDENTITY \
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CODED | \
				BT_LE_ADV_OPT_USE_IDENTITY, \
				BT_GAP_ADV_FAST_INT_MIN_2, \
				BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/**
 * Helper to initialize extended advertising start parameters inline
 *
 * @param _timeout Advertiser timeout
 * @param _n_evts  Number of advertising events
 */
#define BT_LE_EXT_ADV_START_PARAM_INIT(_timeout, _n_evts) \
{ \
	.timeout = (_timeout), \
	.num_events = (_n_evts), \
}

/**
 * Helper to declare extended advertising start parameters inline
 *
 * @param _timeout Advertiser timeout
 * @param _n_evts  Number of advertising events
 */
#define BT_LE_EXT_ADV_START_PARAM(_timeout, _n_evts) \
	((struct bt_le_ext_adv_start_param[]) { \
		BT_LE_EXT_ADV_START_PARAM_INIT((_timeout), (_n_evts)) \
	})

#define BT_LE_EXT_ADV_START_DEFAULT BT_LE_EXT_ADV_START_PARAM(0, 0)

/**
 * Helper to declare periodic advertising parameters inline
 *
 * @param _int_min     Minimum periodic advertising interval
 * @param _int_max     Maximum periodic advertising interval
 * @param _options     Periodic advertising properties bitfield.
 */
#define BT_LE_PER_ADV_PARAM_INIT(_int_min, _int_max, _options) \
{ \
	.interval_min = (_int_min), \
	.interval_max = (_int_max), \
	.options = (_options), \
}

/**
 * Helper to declare periodic advertising parameters inline
 *
 * @param _int_min     Minimum periodic advertising interval
 * @param _int_max     Maximum periodic advertising interval
 * @param _options     Periodic advertising properties bitfield.
 */
#define BT_LE_PER_ADV_PARAM(_int_min, _int_max, _options) \
	((struct bt_le_per_adv_param[]) { \
		BT_LE_PER_ADV_PARAM_INIT(_int_min, _int_max, _options) \
	})

#define BT_LE_PER_ADV_DEFAULT BT_LE_PER_ADV_PARAM(BT_GAP_ADV_SLOW_INT_MIN, \
						  BT_GAP_ADV_SLOW_INT_MAX, \
						  BT_LE_PER_ADV_OPT_NONE)

/**
 * @brief Start advertising
 *
 * Set advertisement data, scan response data, advertisement parameters
 * and start advertising.
 *
 * When the advertisement parameter peer address has been set the advertising
 * will be directed to the peer. In this case advertisement data and scan
 * response data parameters are ignored. If the mode is high duty cycle
 * the timeout will be @ref BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT.
 *
 * @param param Advertising parameters.
 * @param ad Data to be used in advertisement packets.
 * @param ad_len Number of elements in ad
 * @param sd Data to be used in scan response packets.
 * @param sd_len Number of elements in sd
 *
 * @return Zero on success or (negative) error code otherwise.
 * @return -ENOMEM No free connection objects available for connectable
 *                 advertiser.
 * @return -ECONNREFUSED When connectable advertising is requested and there
 *                       is already maximum number of connections established
 *                       in the controller.
 *                       This error code is only guaranteed when using Zephyr
 *                       controller, for other controllers code returned in
 *                       this case may be -EIO.
 */
int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len);

/**
 * @brief Update advertising
 *
 * Update advertisement and scan response data.
 *
 * @param ad Data to be used in advertisement packets.
 * @param ad_len Number of elements in ad
 * @param sd Data to be used in scan response packets.
 * @param sd_len Number of elements in sd
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_adv_update_data(const struct bt_data *ad, size_t ad_len,
			  const struct bt_data *sd, size_t sd_len);

/**
 * @brief Stop advertising
 *
 * Stops ongoing advertising.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_adv_stop(void);

/**
 * @brief Create advertising set.
 *
 * Create a new advertising set and set advertising parameters.
 * Advertising parameters can be updated with @ref bt_le_ext_adv_update_param.
 *
 * @param[in] param Advertising parameters.
 * @param[in] cb    Callback struct to notify about advertiser activity. Can be
 *                  NULL. Must point to valid memory during the lifetime of the
 *                  advertising set.
 * @param[out] adv  Valid advertising set object on success.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_create(const struct bt_le_adv_param *param,
			 const struct bt_le_ext_adv_cb *cb,
			 struct bt_le_ext_adv **adv);

struct bt_le_ext_adv_start_param {
	/**
	 * @brief Advertiser timeout (N * 10 ms).
	 *
	 * Application will be notified by the advertiser sent callback.
	 * Set to zero for no timeout.
	 *
	 * When using high duty cycle directed connectable advertising then
	 * this parameters must be set to a non-zero value less than or equal
	 * to the maximum of @ref BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT.
	 *
	 * If privacy @option{CONFIG_BT_PRIVACY} is enabled then the timeout
	 * must be less than @option{CONFIG_BT_RPA_TIMEOUT}.
	 */
	uint16_t timeout;
	/**
	 * @brief Number of advertising events.
	 *
	 * Application will be notified by the advertiser sent callback.
	 * Set to zero for no limit.
	 */
	uint8_t  num_events;
};

/**
 * @brief Start advertising with the given advertising set
 *
 * If the advertiser is limited by either the timeout or number of advertising
 * events the application will be notified by the advertiser sent callback once
 * the limit is reached.
 * If the advertiser is limited by both the timeout and the number of
 * advertising events then the limit that is reached first will stop the
 * advertiser.
 *
 * @param adv    Advertising set object.
 * @param param  Advertise start parameters.
 */
int bt_le_ext_adv_start(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_start_param *param);

/**
 * @brief Stop advertising with the given advertising set
 *
 * Stop advertising with a specific advertising set. When using this function
 * the advertising sent callback will not be called.
 *
 * @param adv Advertising set object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_stop(struct bt_le_ext_adv *adv);

/**
 * @brief Set an advertising set's advertising or scan response data.
 *
 * Set advertisement data or scan response data. If the advertising set is
 * currently advertising then the advertising data will be updated in
 * subsequent advertising events.
 *
 * When both @ref BT_LE_ADV_OPT_EXT_ADV and @ref BT_LE_ADV_OPT_SCANNABLE are
 * enabled then advertising data is ignored.
 * When @ref BT_LE_ADV_OPT_SCANNABLE is not enabled then scan response data is
 * ignored.
 *
 * If the advertising set has been configured to send advertising data on the
 * primary advertising channels then the maximum data length is
 * @ref BT_GAP_ADV_MAX_ADV_DATA_LEN bytes.
 * If the advertising set has been configured for extended advertising,
 * then the maximum data length is defined by the controller with the maximum
 * possible of @ref BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN bytes.
 *
 * @note Not all scanners support extended data length advertising data.
 *
 * @note When updating the advertising data while advertising the advertising
 *       data and scan response data length must be smaller or equal to what
 *       can be fit in a single advertising packet. Otherwise the
 *       advertiser must be stopped.
 *
 * @param adv     Advertising set object.
 * @param ad      Data to be used in advertisement packets.
 * @param ad_len  Number of elements in ad
 * @param sd      Data to be used in scan response packets.
 * @param sd_len  Number of elements in sd
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_set_data(struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len);

/**
 * @brief Update advertising parameters.
 *
 * Update the advertising parameters. The function will return an error if the
 * advertiser set is currently advertising. Stop the advertising set before
 * calling this function.
 *
 * @param adv   Advertising set object.
 * @param param Advertising parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_update_param(struct bt_le_ext_adv *adv,
			       const struct bt_le_adv_param *param);

/**
 * @brief Delete advertising set.
 *
 * Delete advertising set. This will free up the advertising set and make it
 * possible to create a new advertising set.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_delete(struct bt_le_ext_adv *adv);

/**
 * @brief Get array index of an advertising set.
 *
 * This function is used to map bt_adv to index of an array of
 * advertising sets. The array has CONFIG_BT_EXT_ADV_MAX_ADV_SET elements.
 *
 * @param adv Advertising set.
 *
 * @return Index of the advertising set object.
 * The range of the returned value is 0..CONFIG_BT_EXT_ADV_MAX_ADV_SET-1
 */
uint8_t bt_le_ext_adv_get_index(struct bt_le_ext_adv *adv);

/** @brief Advertising set info structure. */
struct bt_le_ext_adv_info {
	/* Local identity */
	uint8_t                    id;

	/** Currently selected Transmit Power (dBM). */
	int8_t                     tx_power;
};

/**
 * @brief Get advertising set info
 *
 * @param adv Advertising set object
 * @param info Advertising set info object
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_ext_adv_get_info(const struct bt_le_ext_adv *adv,
			   struct bt_le_ext_adv_info *info);

/**
 * @typedef bt_le_scan_cb_t
 * @brief Callback type for reporting LE scan results.
 *
 * A function of this type is given to the bt_le_scan_start() function
 * and will be called for any discovered LE device.
 *
 * @param addr Advertiser LE address and type.
 * @param rssi Strength of advertiser signal.
 * @param adv_type Type of advertising response from advertiser.
 * @param buf Buffer containing advertiser data.
 */
typedef void bt_le_scan_cb_t(const bt_addr_le_t *addr, int8_t rssi,
			     uint8_t adv_type, struct net_buf_simple *buf);

/**
 * @brief Set or update the periodic advertising parameters.
 *
 * The periodic advertising parameters can only be set or updated on an
 * extended advertisement set which is neither scannable, connectable nor
 * anonymous.
 *
 * @param adv   Advertising set object.
 * @param param Advertising parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_set_param(struct bt_le_ext_adv *adv,
			    const struct bt_le_per_adv_param *param);

/**
 * @brief Set or update the periodic advertising data.
 *
 * The periodic advertisement data can only be set or updated on an
 * extended advertisement set which is neither scannable, connectable nor
 * anonymous.
 *
 * @param adv       Advertising set object.
 * @param ad        Advertising data.
 * @param ad_len    Advertising data length.
 *
 * @return          Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_set_data(const struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len);

/**
 * @brief Starts periodic advertising.
 *
 * Enabling the periodic advertising can be done independently of extended
 * advertising, but both periodic advertising and extended advertising
 * shall be enabled before any periodic advertising data is sent. The
 * periodic advertising and extended advertising can be enabled in any order.
 *
 * Once periodic advertising has been enabled, it will continue advertising
 * until @ref bt_le_per_adv_stop() has been called, or if the advertising set
 * is deleted by @ref bt_le_ext_adv_delete(). Calling @ref bt_le_ext_adv_stop()
 * will not stop the periodic advertising.
 *
 * @param adv      Advertising set object.
 *
 * @return         Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_start(struct bt_le_ext_adv *adv);

/**
 * @brief Stops periodic advertising.
 *
 * Disabling the periodic advertising can be done independently of extended
 * advertising. Disabling periodic advertising will not disable extended
 * advertising.
 *
 * @param adv      Advertising set object.
 *
 * @return         Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_stop(struct bt_le_ext_adv *adv);

struct bt_le_per_adv_sync_synced_info {
	/** Advertiser LE address and type. */
	const bt_addr_le_t *addr;

	/** Advertiser SID */
	uint8_t sid;

	/** Periodic advertising interval (N * 1.25 ms) */
	uint16_t interval;

	/** Advertiser PHY */
	uint8_t phy;

	/** True if receiving periodic advertisements, false otherwise. */
	bool recv_enabled;

	/**
	 * @brief Service Data provided by the peer when sync is transferred
	 *
	 * Will always be 0 when the sync is locally created.
	 */
	uint16_t service_data;

	/**
	 * @brief Peer that transferred the periodic advertising sync
	 *
	 * Will always be 0 when the sync is locally created.
	 *
	 */
	struct bt_conn *conn;
};

struct bt_le_per_adv_sync_term_info {
	/** Advertiser LE address and type. */
	const bt_addr_le_t *addr;

	/** Advertiser SID */
	uint8_t sid;
};

struct bt_le_per_adv_sync_recv_info {
	/** Advertiser LE address and type. */
	const bt_addr_le_t *addr;

	/** Advertiser SID */
	uint8_t sid;

	/** The TX power of the advertisement. */
	int8_t tx_power;

	/** The RSSI of the advertisement excluding any CTE. */
	int8_t rssi;

	/** The Constant Tone Extension (CTE) of the advertisement */
	uint8_t cte_type;
};


struct bt_le_per_adv_sync_state_info {
	/** True if receiving periodic advertisements, false otherwise. */
	bool recv_enabled;
};

struct bt_le_per_adv_sync_cb {
	/**
	 * @brief The periodic advertising has been successfully synced.
	 *
	 * This callback notifies the application that the periodic advertising
	 * set has been successfully synced, and will now start to
	 * receive periodic advertising reports.
	 *
	 * @param sync The periodic advertising sync object.
	 * @param info Information about the sync event.
	 */
	void (*synced)(struct bt_le_per_adv_sync *sync,
		       struct bt_le_per_adv_sync_synced_info *info);

	/**
	 * @brief The periodic advertising sync has been terminated.
	 *
	 * This callback notifies the application that the periodic advertising
	 * sync has been terminated, either by local request, remote request or
	 * because due to missing data, e.g. by being out of range or sync.
	 *
	 * @param sync  The periodic advertising sync object.
	 */
	void (*term)(struct bt_le_per_adv_sync *sync,
		     const struct bt_le_per_adv_sync_term_info *info);

	/**
	 * @brief Periodic advertising data received.
	 *
	 * This callback notifies the application of an periodic advertising
	 * report.
	 *
	 * @param sync  The advertising set object.
	 * @param info  Information about the periodic advertising event.
	 * @param buf   Buffer containing the periodic advertising data.
	 */
	void (*recv)(struct bt_le_per_adv_sync *sync,
		     const struct bt_le_per_adv_sync_recv_info *info,
		     struct net_buf_simple *buf);

	/**
	 * @brief The periodic advertising sync state has changed.
	 *
	 * This callback notifies the application about changes to the sync
	 * state. Initialize sync and termination is handled by their individual
	 * callbacks, and won't be notified here.
	 *
	 * @param sync  The periodic advertising sync object.
	 * @param info  Information about the state change.
	 */
	void (*state_changed)(struct bt_le_per_adv_sync *sync,
			      const struct bt_le_per_adv_sync_state_info *info);


	sys_snode_t node;
};

/** Periodic advertising sync options */
enum {
	/** Convenience value when no options are specified. */
	BT_LE_PER_ADV_SYNC_OPT_NONE = 0,

	/**
	 * @brief Use the periodic advertising list to sync with advertiser
	 *
	 * When this option is set, the address and SID of the parameters
	 * are ignored.
	 */
	BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST = BIT(0),

	/**
	 * @brief Disables periodic advertising reports
	 *
	 * No advertisement reports will be handled until enabled.
	 */
	BT_LE_PER_ADV_SYNC_OPT_REPORTING_INITIALLY_DISABLED = BIT(1),

	/** Sync with Angle of Arrival (AoA) constant tone extension */
	BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOA = BIT(2),

	/** Sync with Angle of Departure (AoD) 1 us constant tone extension */
	BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_1US = BIT(3),

	/** Sync with Angle of Departure (AoD) 2 us constant tone extension */
	BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_2US = BIT(4),

	/** Do not sync to packets without a constant tone extension */
	BT_LE_PER_ADV_SYNC_OPT_SYNC_ONLY_CONST_TONE_EXT = BIT(5),
};

struct bt_le_per_adv_sync_param {
	/**
	 * @brief Periodic Advertiser Address
	 *
	 * Only valid if not using the periodic advertising list
	 */
	bt_addr_le_t addr;

	/**
	 * @brief Advertiser SID
	 *
	 * Only valid if not using the periodic advertising list
	 */
	uint8_t sid;

	/** Bit-field of periodic advertising sync options. */
	uint32_t options;

	/**
	 * @brief Maximum event skip
	 *
	 * Maximum number of periodic advertising events that can be
	 * skipped after a successful receive
	 */
	uint16_t skip;

	/**
	 * @brief Synchronization timeout (N * 10 ms)
	 *
	 * Synchronization timeout for the periodic advertising sync.
	 * Range 0x000A to 0x4000 (100 ms to 163840 ms)
	 */
	uint16_t timeout;
};

/**
 * @brief Get array index of an periodic advertising sync object.
 *
 * This function is get the index of an array of periodic advertising sync
 * objects. The array has CONFIG_BT_PER_ADV_SYNC_MAX elements.
 *
 * @param per_adv_sync The periodic advertising sync object.
 *
 * @return Index of the periodic advertising sync object.
 * The range of the returned value is 0..CONFIG_BT_PER_ADV_SYNC_MAX-1
 */
uint8_t bt_le_per_adv_sync_get_index(struct bt_le_per_adv_sync *per_adv_sync);

/**
 * @brief Create a periodic advertising sync object.
 *
 * Create a periodic advertising sync object that can try to synchronize
 * to periodic advertising reports from an advertiser. Scan shall either be
 * disabled or extended scan shall be enabled.
 *
 * @param[in]  param     Periodic advertising sync parameters.
 * @param[out] out_sync  Periodic advertising sync object on.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_create(const struct bt_le_per_adv_sync_param *param,
			      struct bt_le_per_adv_sync **out_sync);

/**
 * @brief Delete periodic advertising sync.
 *
 * Delete the periodic advertising sync object. Can be called regardless of the
 * state of the sync. If the syncing is currently syncing, the syncing is
 * cancelled. If the sync has been established, it is terminated. The
 * periodic advertising sync object will be invalidated afterwards.
 *
 * If the state of the sync object is syncing, then a new periodic advertising
 * sync object may not be created until the controller has finished canceling
 * this object.
 *
 * @param per_adv_sync The periodic advertising sync object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_delete(struct bt_le_per_adv_sync *per_adv_sync);

/**
 * @brief Register periodic advertising sync callbacks.
 *
 * Adds the callback structure to the list of callback structures for periodic
 * adverising syncs.
 *
 * This callback will be called for all periodic advertising sync activity,
 * such as synced, terminated and when data is received.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_le_per_adv_sync_cb_register(struct bt_le_per_adv_sync_cb *cb);

/**
 * @brief Enables receiving periodic advertising reports for a sync.
 *
 * If the sync is already receiving the reports, -EALREADY is returned.
 *
 * @param per_adv_sync The periodic advertising sync object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_recv_enable(struct bt_le_per_adv_sync *per_adv_sync);

/**
 * @brief Disables receiving periodic advertising reports for a sync.
 *
 * If the sync report receiving is already disabled, -EALREADY is returned.
 *
 * @param per_adv_sync The periodic advertising sync object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_recv_disable(struct bt_le_per_adv_sync *per_adv_sync);

/** Periodic Advertising Sync Transfer options */
enum {
	/** Convenience value when no options are specified. */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_NONE = 0,

	/**
	 * @brief No Angle of Arrival (AoA)
	 *
	 * Do not sync with Angle of Arrival (AoA) constant tone extension
	 **/
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOA = BIT(0),

	/**
	 * @brief No Angle of Departure (AoD) 1 us
	 *
	 * Do not sync with Angle of Departure (AoD) 1 us
	 * constant tone extension
	 */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_1US = BIT(1),

	/**
	 * @brief No Angle of Departure (AoD) 2
	 *
	 * Do not sync with Angle of Departure (AoD) 2 us
	 * constant tone extension
	 */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_2US = BIT(2),

	/** Only sync to packets with constant tone extension */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_ONLY_CTE = BIT(3),
};

struct bt_le_per_adv_sync_transfer_param {
	/**
	 * @brief Maximum event skip
	 *
	 * The number of periodic advertising packets that can be skipped
	 * after a successful receive.
	 */
	uint16_t skip;

	/**
	 * @brief Synchronization timeout (N * 10 ms)
	 *
	 * Synchronization timeout for the periodic advertising sync.
	 * Range 0x000A to 0x4000 (100 ms to 163840 ms)
	 */
	uint16_t timeout;

	/** Periodic Advertising Sync Transfer options */
	uint32_t options;
};

/**
 * @brief Transfer the periodic advertising sync information to a peer device.
 *
 * This will allow another device to quickly synchronize to the same periodic
 * advertising train that this device is currently synced to.
 *
 * @param per_adv_sync  The periodic advertising sync to transfer.
 * @param conn          The peer device that will receive the sync information.
 * @param service_data  Application service data provided to the remote host.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_transfer(const struct bt_le_per_adv_sync *per_adv_sync,
				const struct bt_conn *conn,
				uint16_t service_data);


/**
 * @brief Transfer the information about a periodic advertising set.
 *
 * This will allow another device to quickly synchronize to periodic
 * advertising set from this device.
 *
 * @param adv           The periodic advertising set to transfer info of.
 * @param conn          The peer device that will receive the information.
 * @param service_data  Application service data provided to the remote host.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_set_info_transfer(const struct bt_le_ext_adv *adv,
				    const struct bt_conn *conn,
				    uint16_t service_data);

/**
 * @brief Subscribe to periodic advertising sync transfers (PASTs).
 *
 * Sets the parameters and allow other devices to transfer periodic advertising
 * syncs.
 *
 * @param conn    The connection to set the parameters for. If NULL default
 *                parameters for all connections will be set. Parameters set
 *                for specific connection will always have precedence.
 * @param param   The periodic advertising sync transfer parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_transfer_subscribe(
	const struct bt_conn *conn,
	const struct bt_le_per_adv_sync_transfer_param *param);

/**
 * @brief Unsubscribe from periodic advertising sync transfers (PASTs).
 *
 * Remove the parameters that allow other devices to transfer periodic
 * advertising syncs.
 *
 * @param conn    The connection to remove the parameters for. If NULL default
 *                parameters for all connections will be removed. Unsubscribing
 *                for a specific device, will still allow other devices to
 *                transfer periodic advertising syncs.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_transfer_unsubscribe(const struct bt_conn *conn);

/**
 * @brief Add a device to the periodic advertising list.
 *
 * Add peer device LE address to the periodic advertising list. This will make
 * it possibly to automatically create a periodic advertising sync to this
 * device.
 *
 * @param addr Bluetooth LE identity address.
 * @param sid  The advertising set ID. This value is obtained from the
 *             @ref bt_le_scan_recv_info in the scan callback.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_list_add(const bt_addr_le_t *addr, uint8_t sid);

/**
 * @brief Remove a device from the periodic advertising list.
 *
 * Removes peer device LE address from the periodic advertising list.
 *
 * @param addr Bluetooth LE identity address.
 * @param sid  The advertising set ID. This value is obtained from the
 *             @ref bt_le_scan_recv_info in the scan callback.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_list_remove(const bt_addr_le_t *addr, uint8_t sid);

/**
 * @brief Clear the periodic advertising list.
 *
 * Clears the entire periodic advertising list.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_list_clear(void);

enum {
	/** Convenience value when no options are specified. */
	BT_LE_SCAN_OPT_NONE = 0,

	/** Filter duplicates. */
	BT_LE_SCAN_OPT_FILTER_DUPLICATE = BIT(0),

	/** Filter using whitelist. */
	BT_LE_SCAN_OPT_FILTER_WHITELIST = BIT(1),

	/** Enable scan on coded PHY (Long Range).*/
	BT_LE_SCAN_OPT_CODED = BIT(2),

	/**
	 * @brief Disable scan on 1M phy.
	 *
	 * @note Requires @ref BT_LE_SCAN_OPT_CODED.
	 */
	BT_LE_SCAN_OPT_NO_1M = BIT(3),
};

enum {
	/** Scan without requesting additional information from advertisers. */
	BT_LE_SCAN_TYPE_PASSIVE = 0x00,

	/** Scan and request additional information from advertisers. */
	BT_LE_SCAN_TYPE_ACTIVE = 0x01,
};

/** LE scan parameters */
struct bt_le_scan_param {
	/** Scan type (BT_LE_SCAN_TYPE_ACTIVE or BT_LE_SCAN_TYPE_PASSIVE) */
	uint8_t  type;

	/** Bit-field of scanning options. */
	uint32_t options;

	/** Scan interval (N * 0.625 ms) */
	uint16_t interval;

	/** Scan window (N * 0.625 ms) */
	uint16_t window;

	/**
	 * @brief Scan timeout (N * 10 ms)
	 *
	 * Application will be notified by the scan timeout callback.
	 * Set zero to disable timeout.
	 */
	uint16_t timeout;

	/**
	 * @brief Scan interval LE Coded PHY (N * 0.625 MS)
	 *
	 * Set zero to use same as LE 1M PHY scan interval.
	 */
	uint16_t interval_coded;

	/**
	 * @brief Scan window LE Coded PHY (N * 0.625 MS)
	 *
	 * Set zero to use same as LE 1M PHY scan window.
	 */
	uint16_t window_coded;
};

/** LE advertisement packet information */
struct bt_le_scan_recv_info {
	/**
	 * @brief Advertiser LE address and type.
	 *
	 * If advertiser is anonymous then this address will be
	 * @ref BT_ADDR_LE_ANY.
	 */
	const bt_addr_le_t *addr;

	/** Advertising Set Identifier. */
	uint8_t sid;

	/** Strength of advertiser signal. */
	int8_t rssi;

	/** Transmit power of the advertiser. */
	int8_t tx_power;

	/** Advertising packet type. */
	uint8_t adv_type;

	/** Advertising packet properties. */
	uint16_t adv_props;

	/**
	 * @brief Periodic advertising interval.
	 *
	 * If 0 there is no periodic advertising.
	 */
	uint16_t interval;

	/** Primary advertising channel PHY. */
	uint8_t primary_phy;

	/** Secondary advertising channel PHY. */
	uint8_t secondary_phy;
};

/** Listener context for (LE) scanning. */
struct bt_le_scan_cb {

	/**
	 * @brief Advertisement packet received callback.
	 *
	 * @param info Advertiser packet information.
	 * @param buf  Buffer containing advertiser data.
	 */
	void (*recv)(const struct bt_le_scan_recv_info *info,
		     struct net_buf_simple *buf);

	/** @brief The scanner has stopped scanning after scan timeout. */
	void (*timeout)(void);

	sys_snode_t node;
};

/**
 * @brief Initialize scan parameters
 *
 * @param _type     Scan Type, BT_LE_SCAN_TYPE_ACTIVE or
 *                  BT_LE_SCAN_TYPE_PASSIVE.
 * @param _options  Scan options
 * @param _interval Scan Interval (N * 0.625 ms)
 * @param _window   Scan Window (N * 0.625 ms)
 */
#define BT_LE_SCAN_PARAM_INIT(_type, _options, _interval, _window) \
{ \
	.type = (_type), \
	.options = (_options), \
	.interval = (_interval), \
	.window = (_window), \
	.timeout = 0, \
	.interval_coded = 0, \
	.window_coded = 0, \
}

/**
 * @brief Helper to declare scan parameters inline
 *
 * @param _type     Scan Type, BT_LE_SCAN_TYPE_ACTIVE or
 *                  BT_LE_SCAN_TYPE_PASSIVE.
 * @param _options  Scan options
 * @param _interval Scan Interval (N * 0.625 ms)
 * @param _window   Scan Window (N * 0.625 ms)
 */
#define BT_LE_SCAN_PARAM(_type, _options, _interval, _window) \
	((struct bt_le_scan_param[]) { \
		BT_LE_SCAN_PARAM_INIT(_type, _options, _interval, _window) \
	 })

/**
 * @brief Helper macro to enable active scanning to discover new devices.
 */
#define BT_LE_SCAN_ACTIVE BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, \
					   BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
					   BT_GAP_SCAN_FAST_INTERVAL, \
					   BT_GAP_SCAN_FAST_WINDOW)

/**
 * @brief Helper macro to enable passive scanning to discover new devices.
 *
 * This macro should be used if information required for device identification
 * (e.g., UUID) are known to be placed in Advertising Data.
 */
#define BT_LE_SCAN_PASSIVE BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, \
					    BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
					    BT_GAP_SCAN_FAST_INTERVAL, \
					    BT_GAP_SCAN_FAST_WINDOW)

/**
 * @brief Helper macro to enable active scanning to discover new devices.
 * Include scanning on Coded PHY in addition to 1M PHY.
 */
#define BT_LE_SCAN_CODED_ACTIVE \
		BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, \
				 BT_LE_SCAN_OPT_CODED | \
				 BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
				 BT_GAP_SCAN_FAST_INTERVAL, \
				 BT_GAP_SCAN_FAST_WINDOW)

/**
 * @brief Helper macro to enable passive scanning to discover new devices.
 * Include scanning on Coded PHY in addition to 1M PHY.
 *
 * This macro should be used if information required for device identification
 * (e.g., UUID) are known to be placed in Advertising Data.
 */
#define BT_LE_SCAN_CODED_PASSIVE \
		BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, \
				 BT_LE_SCAN_OPT_CODED | \
				 BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
				 BT_GAP_SCAN_FAST_INTERVAL, \
				 BT_GAP_SCAN_FAST_WINDOW)

/**
 * @brief Start (LE) scanning
 *
 * Start LE scanning with given parameters and provide results through
 * the specified callback.
 *
 * @note The LE scanner by default does not use the Identity Address of the
 *       local device when @option{CONFIG_BT_PRIVACY} is disabled. This is to
 *       prevent the active scanner from disclosing the identity information
 *       when requesting additional information from advertisers.
 *       In order to enable directed advertiser reports then
 *       @option{CONFIG_BT_SCAN_WITH_IDENTITY} must be enabled.
 *
 * @param param Scan parameters.
 * @param cb Callback to notify scan results. May be NULL if callback
 *           registration through @ref bt_le_scan_cb_register is preferred.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb);

/**
 * @brief Stop (LE) scanning.
 *
 * Stops ongoing LE scanning.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_scan_stop(void);

/**
 * @brief Register scanner packet callbacks.
 *
 * Adds the callback structure to the list of callback structures that monitors
 * scanner activity.
 *
 * This callback will be called for all scanner activity, regardless of what
 * API was used to start the scanner.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_le_scan_cb_register(struct bt_le_scan_cb *cb);

/**
 * @brief Unregister scanner packet callbacks.
 *
 * Remove the callback structure from the list of scanner callbacks.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_le_scan_cb_unregister(struct bt_le_scan_cb *cb);

/**
 * @brief Add device (LE) to whitelist.
 *
 * Add peer device LE address to the whitelist.
 *
 * @note The whitelist cannot be modified when an LE role is using
 * the whitelist, i.e advertiser or scanner using a whitelist or automatic
 * connecting to devices using whitelist.
 *
 * @param addr Bluetooth LE identity address.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_whitelist_add(const bt_addr_le_t *addr);

/**
 * @brief Remove device (LE) from whitelist.
 *
 * Remove peer device LE address from the whitelist.
 *
 * @note The whitelist cannot be modified when an LE role is using
 * the whitelist, i.e advertiser or scanner using a whitelist or automatic
 * connecting to devices using whitelist.
 *
 * @param addr Bluetooth LE identity address.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_whitelist_rem(const bt_addr_le_t *addr);

/**
 * @brief Clear whitelist.
 *
 * Clear all devices from the whitelist.
 *
 * @note The whitelist cannot be modified when an LE role is using
 * the whitelist, i.e advertiser or scanner using a whitelist or automatic
 * connecting to devices using whitelist.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_whitelist_clear(void);

/**
 * @brief Set (LE) channel map.
 *
 * @param chan_map Channel map.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_set_chan_map(uint8_t chan_map[5]);

/**
 * @brief Helper for parsing advertising (or EIR or OOB) data.
 *
 * A helper for parsing the basic data types used for Extended Inquiry
 * Response (EIR), Advertising Data (AD), and OOB data blocks. The most
 * common scenario is to call this helper on the advertising data
 * received in the callback that was given to bt_le_scan_start().
 *
 * @param ad        Advertising data as given to the bt_le_scan_cb_t callback.
 * @param func      Callback function which will be called for each element
 *                  that's found in the data. The callback should return
 *                  true to continue parsing, or false to stop parsing.
 * @param user_data User data to be passed to the callback.
 */
void bt_data_parse(struct net_buf_simple *ad,
		   bool (*func)(struct bt_data *data, void *user_data),
		   void *user_data);

/** LE Secure Connections pairing Out of Band data. */
struct bt_le_oob_sc_data {
	/** Random Number. */
	uint8_t r[16];

	/** Confirm Value. */
	uint8_t c[16];
};

/** LE Out of Band information. */
struct bt_le_oob {
	/** LE address. If privacy is enabled this is a Resolvable Private
	 *  Address.
	 */
	bt_addr_le_t addr;

	/** LE Secure Connections pairing Out of Band data. */
	struct bt_le_oob_sc_data le_sc_data;
};

/**
 * @brief Get local LE Out of Band (OOB) information.
 *
 * This function allows to get local information that are useful for
 * Out of Band pairing or connection creation.
 *
 * If privacy @option{CONFIG_BT_PRIVACY} is enabled this will result in
 * generating new Resolvable Private Address (RPA) that is valid for
 * @option{CONFIG_BT_RPA_TIMEOUT} seconds. This address will be used for
 * advertising started by @ref bt_le_adv_start, active scanning and
 * connection creation.
 *
 * @note If privacy is enabled the RPA cannot be refreshed in the following
 *       cases:
 *       - Creating a connection in progress, wait for the connected callback.
 *      In addition when extended advertising @option{CONFIG_BT_EXT_ADV} is
 *      not enabled or not supported by the controller:
 *       - Advertiser is enabled using a Random Static Identity Address for a
 *         different local identity.
 *       - The local identity conflicts with the local identity used by other
 *         roles.
 *
 * @param[in]  id  Local identity, in most cases BT_ID_DEFAULT.
 * @param[out] oob LE OOB information
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_oob_get_local(uint8_t id, struct bt_le_oob *oob);

/**
 * @brief Get local LE Out of Band (OOB) information.
 *
 * This function allows to get local information that are useful for
 * Out of Band pairing or connection creation.
 *
 * If privacy @option{CONFIG_BT_PRIVACY} is enabled this will result in
 * generating new Resolvable Private Address (RPA) that is valid for
 * @option{CONFIG_BT_RPA_TIMEOUT} seconds. This address will be used by the
 * advertising set.
 *
 * @note When generating OOB information for multiple advertising set all
 *       OOB information needs to be generated at the same time.
 *
 * @note If privacy is enabled the RPA cannot be refreshed in the following
 *       cases:
 *       - Creating a connection in progress, wait for the connected callback.
 *
 * @param[in]  adv The advertising set object
 * @param[out] oob LE OOB information
 *
 * @return Zero on success or error code otherwise, positive in case
 * of protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_ext_adv_oob_get_local(struct bt_le_ext_adv *adv,
				struct bt_le_oob *oob);

/** @brief BR/EDR discovery result structure */
struct bt_br_discovery_result {
	/** private */
	uint8_t _priv[4];

	/** Remote device address */
	bt_addr_t addr;

	/** RSSI from inquiry */
	int8_t rssi;

	/** Class of Device */
	uint8_t cod[3];

	/** Extended Inquiry Response */
	uint8_t eir[240];
};

/**
 * @typedef bt_br_discovery_cb_t
 * @brief Callback type for reporting BR/EDR discovery (inquiry)
 *        results.
 *
 * A callback of this type is given to the bt_br_discovery_start()
 * function and will be called at the end of the discovery with
 * information about found devices populated in the results array.
 *
 * @param results Storage used for discovery results
 * @param count Number of valid discovery results.
 */
typedef void bt_br_discovery_cb_t(struct bt_br_discovery_result *results,
				  size_t count);

/** BR/EDR discovery parameters */
struct bt_br_discovery_param {
	/** Maximum length of the discovery in units of 1.28 seconds.
	 *  Valid range is 0x01 - 0x30.
	 */
	uint8_t length;

	/** True if limited discovery procedure is to be used. */
	bool limited;
};

/**
 * @brief Start BR/EDR discovery
 *
 * Start BR/EDR discovery (inquiry) and provide results through the specified
 * callback. When bt_br_discovery_cb_t is called it indicates that discovery
 * has completed. If more inquiry results were received during session than
 * fits in provided result storage, only ones with highest RSSI will be
 * reported.
 *
 * @param param Discovery parameters.
 * @param results Storage for discovery results.
 * @param count Number of results in storage. Valid range: 1-255.
 * @param cb Callback to notify discovery results.
 *
 * @return Zero on success or error code otherwise, positive in case
 * of protocol error or negative (POSIX) in case of stack internal error
 */
int bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  struct bt_br_discovery_result *results, size_t count,
			  bt_br_discovery_cb_t cb);

/**
 * @brief Stop BR/EDR discovery.
 *
 * Stops ongoing BR/EDR discovery. If discovery was stopped by this call
 * results won't be reported
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_br_discovery_stop(void);

struct bt_br_oob {
	/** BR/EDR address. */
	bt_addr_t addr;
};

/**
 * @brief Get BR/EDR local Out Of Band information
 *
 * This function allows to get local controller information that are useful
 * for Out Of Band pairing or connection creation process.
 *
 * @param oob Out Of Band information
 */
int bt_br_oob_get_local(struct bt_br_oob *oob);


/**
 * @brief Enable/disable set controller in discoverable state.
 *
 * Allows make local controller to listen on INQUIRY SCAN channel and responds
 * to devices making general inquiry. To enable this state it's mandatory
 * to first be in connectable state.
 *
 * @param enable Value allowing/disallowing controller to become discoverable.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_set_discoverable(bool enable);

/**
 * @brief Enable/disable set controller in connectable state.
 *
 * Allows make local controller to be connectable. It means the controller
 * start listen to devices requests on PAGE SCAN channel. If disabled also
 * resets discoverability if was set.
 *
 * @param enable Value allowing/disallowing controller to be connectable.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_set_connectable(bool enable);

/**
 * @brief Clear pairing information.
 *
 * @param id    Local identity (mostly just BT_ID_DEFAULT).
 * @param addr  Remote address, NULL or BT_ADDR_LE_ANY to clear all remote
 *              devices.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_unpair(uint8_t id, const bt_addr_le_t *addr);

/** Information about a bond with a remote device. */
struct bt_bond_info {
	/** Address of the remote device. */
	bt_addr_le_t addr;
};

/**
 * @brief Iterate through all existing bonds.
 *
 * @param id         Local identity (mostly just BT_ID_DEFAULT).
 * @param func       Function to call for each bond.
 * @param user_data  Data to pass to the callback function.
 */
void bt_foreach_bond(uint8_t id, void (*func)(const struct bt_bond_info *info,
					   void *user_data),
		     void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_ */
