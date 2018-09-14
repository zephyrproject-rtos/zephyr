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
#include <misc/printk.h>
#include <misc/util.h>
#include <net/buf.h>
#include <bluetooth/hci.h>
#include <bluetooth/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @def BT_ID_DEFAULT
 *
 *  Convenience macro for specifying the default identity. This helps
 *  make the code more readable, especially when only one identity is
 *  supported.
 */
#define BT_ID_DEFAULT 0

/**
 * @brief Generic Access Profile
 * @defgroup bt_gap Generic Access Profile
 * @ingroup bluetooth
 * @{
 */

/**
 * @typedef bt_ready_cb_t
 * @brief Callback for notifying that Bluetooth has been enabled.
 *
 *  @param err zero on success or (negative) error code otherwise.
 */
typedef void (*bt_ready_cb_t)(int err);

/** @brief Enable Bluetooth
 *
 *  Enable Bluetooth. Must be the called before any calls that
 *  require communication with the local Bluetooth hardware.
 *
 *  @param cb Callback to notify completion or NULL to perform the
 *  enabling synchronously.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_enable(bt_ready_cb_t cb);

/** @brief Set Bluetooth Device Name
 *
 *  Set Bluetooth GAP Device Name.
 *
 *  @param name New name
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_set_name(const char *name);

/** @brief Get Bluetooth Device Name
 *
 *  Get Bluetooth GAP Device Name.
 *
 *  @return Bluetooth Device Name
 */
const char *bt_get_name(void);

/** @brief Set the local Identity Address
 *
 *  Allows setting the local Identity Address from the application.
 *  This API must be called before calling bt_enable(). Calling it at any
 *  other time will cause it to fail. In most cases the application doesn't
 *  need to use this API, however there are a few valid cases where
 *  it can be useful (such as for testing).
 *
 *  At the moment, the given address must be a static random address. In the
 *  future support for public addresses may be added.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_set_id_addr(const bt_addr_le_t *addr);

/** @brief Get the currently configured identities.
 *
 *  Returns an array of the currently configured identity addresses. To
 *  make sure all available identities can be retrieved, the number of
 *  elements in the @a addrs array should be CONFIG_BT_ID_MAX. The identity
 *  identifier that some APIs expect (such as advertising parameters) is
 *  simply the index of the identity in the @a addrs array.
 *
 *  Note: Deleted identities may show up as BT_LE_ADDR_ANY in the returned
 *  array.
 *
 *  @param addrs Array where to store the configured identities.
 *  @param count Should be initialized to the array size. Once the function
 *               returns it will contain the number of returned identities.
 */
void bt_id_get(bt_addr_le_t *addrs, size_t *count);

/** @brief Create a new identity.
 *
 *  Create a new identity using the given address and IRK. This function
 *  can be called before calling bt_enable(), in which case it can be used
 *  to override the controller's public address (in case it has one). However,
 *  the new identity will only be stored persistently in flash when this API
 *  is used after bt_enable(). The reason is that the persistent settings
 *  are loaded after bt_enable() and would therefore cause potential conflicts
 *  with the stack blindly overwriting what's stored in flash. The identity
 *  will also not be written to flash in case a pre-defined address is
 *  provided, since in such a situation the app clearly has some place it got
 *  the address from and will be able to repeat the procedure on every power
 *  cycle, i.e. it would be redundant to also store the information in flash.
 *
 *  If the application wants to have the stack randomly generate identities
 *  and store them in flash for later recovery, the way to do it would be
 *  to first initialize the stack (using bt_enable), then call settings_load(),
 *  and after that check with bt_id_get() how many identities were recovered.
 *  If an insufficient amount of identities were recovered the app may then
 *  call bt_id_create() to create new ones.
 *
 *  @param addr Address to use for the new identity. If NULL or initialized
 *              to BT_ADDR_LE_ANY the stack will generate a new static
 *              random address for the identity and copy it to the given
 *              parameter upon return from this function (in case the
 *              parameter was non-NULL).
 *  @param irk  Identity Resolving Key (16 bytes) to be used with this
 *              identity. If set to all zeroes or NULL, the stack will
 *              generate a random IRK for the identity and copy it back
 *              to the parameter upon return from this function (in case
 *              the parameter was non-NULL). If privacy support
 *              (CONFIG_BT_PRIVACY) is not enabled this parameter must
 *              be NULL.
 *
 *  @return Identity identifier (>= 0) in case of success, or a negative
 *          error code on failure.
 */
int bt_id_create(bt_addr_le_t *addr, u8_t *irk);

/** @brief Reset/reclaim an identity for reuse.
 *
 *  The semantics of the @a addr and @a irk parameters of this function
 *  are the same as with bt_id_create(). The difference is the first
 *  @a id parameter that needs to be an existing identity (if it doesn't
 *  exist this function will return an error). When given an existing
 *  identity this function will disconnect any connections created using it,
 *  remove any pairing keys or other data associated with it, and then create
 *  a new identity in the same slot, based on the @a addr and @a irk
 *  parameters.
 *
 *  Note: the default identity (BT_ID_DEFAULT) cannot be reset, i.e. this
 *  API will return an error if asked to do that.
 *
 *  @param id   Existing identity identifier.
 *  @param addr Address to use for the new identity. If NULL or initialized
 *              to BT_ADDR_LE_ANY the stack will generate a new static
 *              random address for the identity and copy it to the given
 *              parameter upon return from this function (in case the
 *              parameter was non-NULL).
 *  @param irk  Identity Resolving Key (16 bytes) to be used with this
 *              identity. If set to all zeroes or NULL, the stack will
 *              generate a random IRK for the identity and copy it back
 *              to the parameter upon return from this function (in case
 *              the parameter was non-NULL). If privacy support
 *              (CONFIG_BT_PRIVACY) is not enabled this parameter must
 *              be NULL.
 *
 *  @return Identity identifier (>= 0) in case of success, or a negative
 *          error code on failure.
 */
int bt_id_reset(u8_t id, bt_addr_le_t *addr, u8_t *irk);

/** @brief Delete an identity.
 *
 *  When given a valid identity this function will disconnect any connections
 *  created using it, remove any pairing keys or other data associated with
 *  it, and then flag is as deleted, so that it can not be used for any
 *  operations. To take back into use the slot the identity was occupying the
 *  bt_id_reset() API needs to be used.
 *
 *  Note: the default identity (BT_ID_DEFAULT) cannot be deleted, i.e. this
 *  API will return an error if asked to do that.
 *
 *  @param id   Existing identity identifier.
 *
 *  @return 0 in case of success, or a negative error code on failure.
 */
int bt_id_delete(u8_t id);

/* Advertising API */

/** Description of different data types that can be encoded into
  * advertising data. Used to form arrays that are passed to the
  * bt_le_adv_start() function.
  */
struct bt_data {
	u8_t type;
	u8_t data_len;
	const u8_t *data;
};

/** @brief Helper to declare elements of bt_data arrays
 *
 *  This macro is mainly for creating an array of struct bt_data
 *  elements which is then passed to bt_le_adv_start().
 *
 *  @param _type Type of advertising data field
 *  @param _data Pointer to the data field payload
 *  @param _data_len Number of bytes behind the _data pointer
 */
#define BT_DATA(_type, _data, _data_len) \
	{ \
		.type = (_type), \
		.data_len = (_data_len), \
		.data = (const u8_t *)(_data), \
	}

/** @brief Helper to declare elements of bt_data arrays
 *
 *  This macro is mainly for creating an array of struct bt_data
 *  elements which is then passed to bt_le_adv_start().
 *
 *  @param _type Type of advertising data field
 *  @param _bytes Variable number of single-byte parameters
 */
#define BT_DATA_BYTES(_type, _bytes...) \
	BT_DATA(_type, ((u8_t []) { _bytes }), \
		sizeof((u8_t []) { _bytes }))

/** Advertising options */
enum {
	/** Convenience value when no options are specified. */
	BT_LE_ADV_OPT_NONE = 0,

	/** Advertise as connectable. Type of advertising is determined by
	 * providing SCAN_RSP data and/or enabling local privacy support.
	 */
	BT_LE_ADV_OPT_CONNECTABLE = BIT(0),

	/** Don't try to resume connectable advertising after a connection.
	 *  This option is only meaningful when used together with
	 *  BT_LE_ADV_OPT_CONNECTABLE. If set the advertising will be stopped
	 *  when bt_le_adv_stop() is called or when an incoming (slave)
	 *  connection happens. If this option is not set the stack will
	 *  take care of keeping advertising enabled even as connections
	 *  occur.
	 */
	BT_LE_ADV_OPT_ONE_TIME = BIT(1),

	/** Advertise using the identity address as the own address.
	 *  @warning This will compromise the privacy of the device, so care
	 *           must be taken when using this option.
	 */
	BT_LE_ADV_OPT_USE_IDENTITY = BIT(2),

	/* Advertise using GAP device name */
	BT_LE_ADV_OPT_USE_NAME = BIT(3),

	/** Use low duty directed advertising mode, otherwise high duty mode
	 *  will be used. This option is only effective when used with
	 *  bt_conn_create_slave_le().
	 */
	BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY = BIT(4),
};

/** LE Advertising Parameters. */
struct bt_le_adv_param {
	/** Local identity */
	u8_t  id;

	/** Bit-field of advertising options */
	u8_t  options;

	/** Minimum Advertising Interval (N * 0.625) */
	u16_t interval_min;

	/** Maximum Advertising Interval (N * 0.625) */
	u16_t interval_max;
};

/** Helper to declare advertising parameters inline
  *
  * @param _options   Advertising Options
  * @param _int_min   Minimum advertising interval
  * @param _int_max   Maximum advertising interval
  */
#define BT_LE_ADV_PARAM(_options, _int_min, _int_max) \
		(&(struct bt_le_adv_param) { \
			.options = (_options), \
			.interval_min = (_int_min), \
			.interval_max = (_int_max), \
		 })

#define BT_LE_ADV_CONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				       BT_GAP_ADV_FAST_INT_MIN_2, \
				       BT_GAP_ADV_FAST_INT_MAX_2)

#define BT_LE_ADV_CONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
					    BT_LE_ADV_OPT_USE_NAME, \
					    BT_GAP_ADV_FAST_INT_MIN_2, \
					    BT_GAP_ADV_FAST_INT_MAX_2)

#define BT_LE_ADV_CONN_DIR_LOW_DUTY \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME | \
			BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY, \
			BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2)

#define BT_LE_ADV_CONN_DIR BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
					   BT_LE_ADV_OPT_ONE_TIME, 0, 0)

#define BT_LE_ADV_NCONN BT_LE_ADV_PARAM(0, BT_GAP_ADV_FAST_INT_MIN_2, \
					BT_GAP_ADV_FAST_INT_MAX_2)

#define BT_LE_ADV_NCONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME, \
					     BT_GAP_ADV_FAST_INT_MIN_2, \
					     BT_GAP_ADV_FAST_INT_MAX_2)

/** @brief Start advertising
 *
 *  Set advertisement data, scan response data, advertisement parameters
 *  and start advertising.
 *
 *  @param param Advertising parameters.
 *  @param ad Data to be used in advertisement packets.
 *  @param ad_len Number of elements in ad
 *  @param sd Data to be used in scan response packets.
 *  @param sd_len Number of elements in sd
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len);

/** @brief Stop advertising
 *
 *  Stops ongoing advertising.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_le_adv_stop(void);

/** @typedef bt_le_scan_cb_t
 *  @brief Callback type for reporting LE scan results.
 *
 *  A function of this type is given to the bt_le_scan_start() function
 *  and will be called for any discovered LE device.
 *
 *  @param addr Advertiser LE address and type.
 *  @param rssi Strength of advertiser signal.
 *  @param adv_type Type of advertising response from advertiser.
 *  @param data Buffer containing advertiser data.
 */
typedef void bt_le_scan_cb_t(const bt_addr_le_t *addr, s8_t rssi,
			     u8_t adv_type, struct net_buf_simple *buf);

/** LE scan parameters */
struct bt_le_scan_param {
	/** Scan type (BT_HCI_LE_SCAN_ACTIVE or BT_HCI_LE_SCAN_PASSIVE) */
	u8_t  type;

	/** Duplicate filtering (BT_HCI_LE_SCAN_FILTER_DUP_ENABLE or
	 *  BT_HCI_LE_SCAN_FILTER_DUP_DISABLE)
	 */
	u8_t  filter_dup;

	/** Scan interval (N * 0.625 ms) */
	u16_t interval;

	/** Scan window (N * 0.625 ms) */
	u16_t window;
};

/** Helper to declare scan parameters inline
  *
  * @param _type     Scan Type (BT_HCI_LE_SCAN_ACTIVE/BT_HCI_LE_SCAN_PASSIVE)
  * @param _filter   Filter Duplicates
  * @param _interval Scan Interval (N * 0.625 ms)
  * @param _window   Scan Window (N * 0.625 ms)
  */
#define BT_LE_SCAN_PARAM(_type, _filter, _interval, _window) \
		(&(struct bt_le_scan_param) { \
			.type = (_type), \
			.filter_dup = (_filter), \
			.interval = (_interval), \
			.window = (_window), \
		 })

/** Helper macro to enable active scanning to discover new devices. */
#define BT_LE_SCAN_ACTIVE BT_LE_SCAN_PARAM(BT_HCI_LE_SCAN_ACTIVE, \
					   BT_HCI_LE_SCAN_FILTER_DUP_ENABLE, \
					   BT_GAP_SCAN_FAST_INTERVAL, \
					   BT_GAP_SCAN_FAST_WINDOW)

/** Helper macro to enable passive scanning to discover new devices.
 *
 * This macro should be used if information required for device identification
 * (e.g., UUID) are known to be placed in Advertising Data.
 */
#define BT_LE_SCAN_PASSIVE BT_LE_SCAN_PARAM(BT_HCI_LE_SCAN_PASSIVE, \
					    BT_HCI_LE_SCAN_FILTER_DUP_ENABLE, \
					    BT_GAP_SCAN_FAST_INTERVAL, \
					    BT_GAP_SCAN_FAST_WINDOW)

/** @brief Start (LE) scanning
 *
 *  Start LE scanning with given parameters and provide results through
 *  the specified callback.
 *
 *  @param param Scan parameters.
 *  @param cb Callback to notify scan results.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb);

/** @brief Stop (LE) scanning.
 *
 *  Stops ongoing LE scanning.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int bt_le_scan_stop(void);

/** @brief Helper for parsing advertising (or EIR or OOB) data.
 *
 *  A helper for parsing the basic data types used for Extended Inquiry
 *  Response (EIR), Advertising Data (AD), and OOB data blocks. The most
 *  common scenario is to call this helper on the advertising data
 *  received in the callback that was given to bt_le_scan_start().
 *
 *  @param ad        Advertising data as given to the bt_le_scan_cb_t callback.
 *  @param func      Callback function which will be called for each element
 *                   that's found in the data. The callback should return
 *                   true to continue parsing, or false to stop parsing.
 *  @param user_data User data to be passed to the callback.
 */
void bt_data_parse(struct net_buf_simple *ad,
		   bool (*func)(struct bt_data *data, void *user_data),
		   void *user_data);

struct bt_le_oob {
	/** LE address. If local privacy is enabled this is Resolvable Private
	 *  Address.
	 */
	bt_addr_le_t addr;
};

/**
 * @brief Get LE local Out Of Band information
 *
 * This function allows to get local information that are useful for Out Of Band
 * pairing or connection creation process.
 *
 * If privacy is enabled this will result in generating new Resolvable Private
 * Address that is valid for CONFIG_BT_RPA_TIMEOUT seconds. This address
 * will be used for advertising, active scanning and connection creation.
 *
 * @param id  Local identity, in most cases BT_ID_DEFAULT.
 * @param oob LE related information
 */
int bt_le_oob_get_local(u8_t id, struct bt_le_oob *oob);

/** @brief BR/EDR discovery result structure */
struct bt_br_discovery_result {
	/** private */
	u8_t _priv[4];

	/** Remote device address */
	bt_addr_t addr;

	/** RSSI from inquiry */
	s8_t rssi;

	/** Class of Device */
	u8_t cod[3];

	/** Extended Inquiry Response */
	u8_t eir[240];
};

/** @typedef bt_br_discovery_cb_t
 *  @brief Callback type for reporting BR/EDR discovery (inquiry)
 *         results.
 *
 *  A callback of this type is given to the bt_br_discovery_start()
 *  function and will be called at the end of the discovery with
 *  information about found devices populated in the results array.
 *
 *  @param results Storage used for discovery results
 *  @param count Number of valid discovery results.
 */
typedef void bt_br_discovery_cb_t(struct bt_br_discovery_result *results,
				  size_t count);

/** BR/EDR discovery parameters */
struct bt_br_discovery_param {
	/** Maximum length of the discovery in units of 1.28 seconds.
	 *  Valid range is 0x01 - 0x30.
	 */
	u8_t length;

	/** True if limited discovery procedure is to be used. */
	bool limited;
};

/** @brief Start BR/EDR discovery
 *
 *  Start BR/EDR discovery (inquiry) and provide results through the specified
 *  callback. When bt_br_discovery_cb_t is called it indicates that discovery
 *  has completed. If more inquiry results were received during session than
 *  fits in provided result storage, only ones with highest RSSI will be
 *  reported.
 *
 *  @param param Discovery parameters.
 *  @param results Storage for discovery results.
 *  @param count Number of results in storage. Valid range: 1-255.
 *  @param cb Callback to notify discovery results.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  struct bt_br_discovery_result *results, size_t count,
			  bt_br_discovery_cb_t cb);

/** @brief Stop BR/EDR discovery.
 *
 *  Stops ongoing BR/EDR discovery. If discovery was stopped by this call
 *  results won't be reported
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
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

/** @def BT_ADDR_STR_LEN
 *
 *  @brief Recommended length of user string buffer for Bluetooth address
 *
 *  @details The recommended length guarantee the output of address
 *  conversion will not lose valuable information about address being
 *  processed.
 */
#define BT_ADDR_STR_LEN 18

/** @def BT_ADDR_LE_STR_LEN
 *
 *  @brief Recommended length of user string buffer for Bluetooth LE address
 *
 *  @details The recommended length guarantee the output of address
 *  conversion will not lose valuable information about address being
 *  processed.
 */
#define BT_ADDR_LE_STR_LEN 27

/** @brief Converts binary Bluetooth address to string.
 *
 *  @param addr Address of buffer containing binary Bluetooth address.
 *  @param str Address of user buffer with enough room to store formatted
 *  string containing binary address.
 *  @param len Length of data to be copied to user string buffer. Refer to
 *  BT_ADDR_STR_LEN about recommended value.
 *
 *  @return Number of successfully formatted bytes from binary address.
 */
static inline int bt_addr_to_str(const bt_addr_t *addr, char *str, size_t len)
{
	return snprintk(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
			addr->val[5], addr->val[4], addr->val[3],
			addr->val[2], addr->val[1], addr->val[0]);
}

/** @brief Converts binary LE Bluetooth address to string.
 *
 *  @param addr Address of buffer containing binary LE Bluetooth address.
 *  @param str Address of user buffer with enough room to store
 *  formatted string containing binary LE address.
 *  @param len Length of data to be copied to user string buffer. Refer to
 *  BT_ADDR_LE_STR_LEN about recommended value.
 *
 *  @return Number of successfully formatted bytes from binary address.
 */
static inline int bt_addr_le_to_str(const bt_addr_le_t *addr, char *str,
				    size_t len)
{
	char type[10];

	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		strcpy(type, "public");
		break;
	case BT_ADDR_LE_RANDOM:
		strcpy(type, "random");
		break;
	case BT_ADDR_LE_PUBLIC_ID:
		strcpy(type, "public id");
		break;
	case BT_ADDR_LE_RANDOM_ID:
		strcpy(type, "random id");
		break;
	default:
		snprintk(type, sizeof(type), "0x%02x", addr->type);
		break;
	}

	return snprintk(str, len, "%02X:%02X:%02X:%02X:%02X:%02X (%s)",
			addr->a.val[5], addr->a.val[4], addr->a.val[3],
			addr->a.val[2], addr->a.val[1], addr->a.val[0], type);
}

/** @brief Enable/disable set controller in discoverable state.
 *
 *  Allows make local controller to listen on INQUIRY SCAN channel and responds
 *  to devices making general inquiry. To enable this state it's mandatory
 *  to first be in connectable state.
 *
 *  @param enable Value allowing/disallowing controller to become discoverable.
 *
 *  @return Negative if fail set to requested state or requested state has been
 *  already set. Zero if done successfully.
 */
int bt_br_set_discoverable(bool enable);

/** @brief Enable/disable set controller in connectable state.
 *
 *  Allows make local controller to be connectable. It means the controller
 *  start listen to devices requests on PAGE SCAN channel. If disabled also
 *  resets discoverability if was set.
 *
 *  @param enable Value allowing/disallowing controller to be connectable.
 *
 *  @return Negative if fail set to requested state or requested state has been
 *  already set. Zero if done successfully.
 */
int bt_br_set_connectable(bool enable);

/** Clear pairing information.
  *
  * @param id    Local identity (mostly just BT_ID_DEFAULT).
  * @param addr  Remote address, NULL or BT_ADDR_LE_ANY to clear all remote
  *              devices.
  *
  * @return 0 on success or negative error value on failure.
  */
int bt_unpair(u8_t id, const bt_addr_le_t *addr);

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
