/** @file
 *  @brief Bluetooth subsystem core APIs.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_BLUETOOTH_H
#define __BT_BLUETOOTH_H

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
};

/** LE Advertising Parameters. */
struct bt_le_adv_param {
	/** Bit-field of advertising options */
	u8_t  options;

	/** Minimum Advertising Interval (N * 0.625) */
	u16_t interval_min;

	/** Maximum Advertising Interval (N * 0.625) */
	u16_t interval_max;

	/** Optional predefined (random) own address. Currently
	 *  the only permitted use of this is for NRPA with
	 *  non-connectable advertising.
	 */
	const bt_addr_t *own_addr;
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

#define BT_LE_ADV_NCONN BT_LE_ADV_PARAM(0, BT_GAP_ADV_FAST_INT_MIN_2, \
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
 * @param oob LE related information
 */
int bt_le_oob_get_local(struct bt_le_oob *oob);

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

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* __BT_BLUETOOTH_H */
