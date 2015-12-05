/** @file
 *  @brief Bluetooth subsystem core APIs.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __BT_BLUETOOTH_H
#define __BT_BLUETOOTH_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <bluetooth/hci.h>
#include <bluetooth/conn.h>

/** @brief Callback for notifying that Bluetooth has been enabled.
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

struct bt_eir {
	uint8_t len;
	uint8_t type;
	uint8_t data[29];
} __packed;

struct bt_le_adv_param {
	uint8_t type;
};

#define BT_LE_ADV(t) (&(struct bt_le_adv_param) { .type = t })

/** @brief Start advertising
 *
 *  Set advertisement data, scan response data, advertisement parameters
 *  and start advertising.
 *
 *  @param param Advertising parameters.
 *  @param ad Data to be used in advertisement packets.
 *  @param sd Data to be used in scan response packets.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_eir *ad, const struct bt_eir *sd);

/** @brief Stop advertising
 *
 *  Stops ongoing advertising.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_le_adv_stop(void);

/** @brief Define a type allowing user to implement a function that can
 *  be used to get back active LE scan results.
 *
 *  A function of this type will be called back when user application
 *  triggers active LE scan. The caller will populate all needed
 *  parameters based on data coming from scan result.
 *  Such function can be set by user when LE active scan API is used.
 *
 *  @param addr Advertiser LE address and type.
 *  @param rssi Strength of advertiser signal.
 *  @param adv_type Type of advertising response from advertiser.
 *  @param adv_data Address of buffer containig advertiser data.
 *  @param len Length of advertiser data contained in buffer.
 */
typedef void bt_le_scan_cb_t(const bt_addr_le_t *addr, int8_t rssi,
			     uint8_t adv_type, const uint8_t *adv_data,
			     uint8_t len);

/** Helper macro to enable active scanning to discover new devices.
 *
 *  The interval and window are intentionally set to the same value to
 *  perform continuous scanning.
 */
#define BT_LE_SCAN_ACTIVE \
		(&(struct bt_le_scan_param) { \
			.type = BT_HCI_LE_SCAN_ACTIVE, \
			.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE, \
			.interval = BT_GAP_SCAN_FAST_INTERVAL, \
			.window = BT_GAP_SCAN_FAST_INTERVAL, \
		 })

/** LE scan parameters */
struct bt_le_scan_param {
	/** Type of scanning (active/passive) */
	uint8_t  type;

	/** Enable duplicate filtering */
	uint8_t  filter_dup;

	/** Scan interval */
	uint16_t interval;

	/** Scan window */
	uint16_t window;
};

/** @brief Start (LE) scanning
 *
 *  Start LE scanning with and provide results through the specified
 *  callback.
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

#if defined(CONFIG_BLUETOOTH_CENTRAL)
/** @brief Automatically connect to remote device if it's in range.
 *
 *  This function enables/disables automatic connection initiation.
 *  Everytime the device looses the connection with peer, this connection
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

#endif /* CONFIG_BLUETOOTH_CENTRAL */

#if defined(CONFIG_BLUETOOTH_SMP)
/** Authenticated pairing callback structure */
struct bt_auth_cb {
	void (*passkey_display)(struct bt_conn *conn, unsigned int passkey);
	void (*passkey_entry)(struct bt_conn *conn);
	void (*passkey_confirm)(struct bt_conn *conn, unsigned int passkey);
	void (*cancel)(struct bt_conn *conn);
};

/** @brief Register authentication callbacks.
 *
 *  Register callbacks to handle authenticated pairing. Passing NULL unregisters
 *  previous callbacks structure.
 *
 *  @param cb Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_auth_cb_register(const struct bt_auth_cb *cb);

/** @brief Reply with entered passkey.
 *
 *  This function should be called only after passkey_entry callback from
 *  bt_auth_cb structure was called.
 *
 *  @param conn Connection object.
 *  @param passkey Entered passkey.
 */
void bt_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey);

/** @brief Cancel ongoing authenticated pairing.
 *
 *  This function allows to cancel ongoing authenticated pairing.
 *
 *  @param conn Connection object.
 */
void bt_auth_cancel(struct bt_conn *conn);

/** @brief Reply if passkey was confirmed by user.
 *
 *  This function should be called only after passkey_confirm callback from
 *  bt_auth_cb structure was called. If passkey is confirmed to match then match
 *  should be true. Otherwise match should be false.
 *
 *  @param conn Connection object.
 *  @param match True if passkey was confirmed to match, false otherwise.
 */
void bt_auth_passkey_confirm(struct bt_conn *conn, bool match);

#endif /* CONFIG_BLUETOOTH_SMP */

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
	return snprintf(str, len, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
			addr->val[5], addr->val[4], addr->val[3],
			addr->val[2], addr->val[1], addr->val[0]);
}

/** @brief Converts binary LE Bluetooth address to string.
 *
 *  @param addr Address of buffer containing binary LE Bluetooth address.
 *  @param user_buf Address of user buffer with enough room to store
 *  formatted string containing binary LE address.
 *  @param len Length of data to be copied to user string buffer. Refer to
 *  BT_ADDR_LE_STR_LEN about recommended value.
 *
 *  @return Number of successfully formatted bytes from binary address.
 */
static inline int bt_addr_le_to_str(const bt_addr_le_t *addr, char *str,
				    size_t len)
{
	char type[7];

	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		strcpy(type, "public");
		break;
	case BT_ADDR_LE_RANDOM:
		strcpy(type, "random");
		break;
	default:
		sprintf(type, "0x%02x", addr->type);
		break;
	}

	return snprintf(str, len, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X (%s)",
			addr->val[5], addr->val[4], addr->val[3],
			addr->val[2], addr->val[1], addr->val[0], type);
}

#if defined(CONFIG_BLUETOOTH_BREDR)
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
#endif
#endif /* __BT_BLUETOOTH_H */
