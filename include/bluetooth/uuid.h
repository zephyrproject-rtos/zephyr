/** @file
 *  @brief Bluetooth UUID handling
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
#ifndef __BT_UUID_H
#define __BT_UUID_H

/** @def BBT_UUID_GAP
 *  @brief Generic Access
 */
#define BT_UUID_GAP				0x1800
/** @def BBT_UUID_GATT
 *  @brief Generic Attribute
 */
#define BT_UUID_GATT				0x1801
/** @def BBT_UUID_CTS
 *  @brief Current Time Service
 */
#define BT_UUID_CTS				0x1805
/** @def BBT_UUID_DIS
 *  @brief Device Information Service
 */
#define BT_UUID_DIS				0x180a
/** @def BBT_UUID_HRS
 *  @brief Heart Rate Service
 */
#define BT_UUID_HRS				0x180d
/** @def BBT_UUID_BAS
 *  @brief Battery Service
 */
#define BT_UUID_BAS				0x180f
/** @def BT_UUID_IPSP
 *  @brief IP Support Service
 */
#define BT_UUID_IPSS				0x1820
/** @def BT_UUID_GATT_PRIMARY
 *  @brief GATT Primary Service
 */
#define BT_UUID_GATT_PRIMARY			0x2800
/** @def BT_UUID_GATT_SECONDARY
 *  @brief GATT Secondary Service
 */
#define BT_UUID_GATT_SECONDARY			0x2801
/** @def BT_UUID_GATT_INCLUDE
 *  @brief GATT Include Service
 */
#define BT_UUID_GATT_INCLUDE			0x2802
/** @def BT_UUID_GATT_CHRC
 *  @brief GATT Characteristic
 */
#define BT_UUID_GATT_CHRC			0x2803
/** @def BT_UUID_GATT_CEP
 *  @brief GATT Characteristic Extended Properties
 */
#define BT_UUID_GATT_CEP			0x2900
/** @def BT_UUID_GATT_CUD
 *  @brief GATT Characteristic User Description
 */
#define BT_UUID_GATT_CUD			0x2901
/** @def BT_UUID_GATT_CCC
 *  @brief GATT Client Characteristic Configuration
 */
#define BT_UUID_GATT_CCC			0x2902
/** @def BT_UUID_GAP_DEVICE_NAME
 *  @brief GAP Characteristic Device Name
 */
#define BT_UUID_GAP_DEVICE_NAME			0x2a00
/** @def BT_UUID_GAP_APPEARANCE
 *  @brief GAP Characteristic Appearance
 */
#define BT_UUID_GAP_APPEARANCE			0x2a01
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_BAS_BATTERY_LEVEL		0x2a19
/** @def BT_UUID_DIS_SYSTEM_ID
 *  @brief DIS Characteristic System ID
 */
#define BT_UUID_DIS_SYSTEM_ID			0x2a23
/** @def BT_UUID_DIS_MODEL_NUMBER_STRING
 *  @brief DIS Characteristic Model Number String
 */
#define BT_UUID_DIS_MODEL_NUMBER_STRING		0x2a24
/** @def BT_UUID_DIS_SERIAL_NUMBER_STRING
 *  @brief DIS Characteristic Serial Number String
 */
#define BT_UUID_DIS_SERIAL_NUMBER_STRING	0x2a25
/** @def BT_UUID_DIS_FIRMWARE_REVISION_STRING
 *  @brief DIS Characteristic Firmware Revision String
 */
#define BT_UUID_DIS_FIRMWARE_REVISION_STRING	0x2a26
/** @def BT_UUID_DIS_HARDWARE_REVISION_STRING
 *  @brief DIS Characteristic Hardware Revision String
 */
#define BT_UUID_DIS_HARDWARE_REVISION_STRING	0x2a27
/** @def BT_UUID_DIS_SOFTWARE_REVISION_STRING
 *  @brief DIS Characteristic Software Revision String
 */
#define BT_UUID_DIS_SOFTWARE_REVISION_STRING	0x2a28
/** @def BT_UUID_DIS_MANUFACTURER_NAME_STRING
 *  @brief DIS Characteristic Manufacturer Name String
 */
#define BT_UUID_DIS_MANUFACTURER_NAME_STRING	0x2a29
/** @def BT_UUID_DIS_PNP_ID
 *  @brief DIS Characteristic PnP ID
 */
#define BT_UUID_DIS_PNP_ID			0x2a50
/** @def BT_UUID_CTS_CURRENT_TIME
 *  @brief CTS Characteristic Current Time
 */
#define BT_UUID_CTS_CURRENT_TIME		0x2a2b
/** @def BT_UUID_HR_MEASUREMENT
 *  @brief HRS Characteristic Measurement Interval
 */
#define BT_UUID_HRS_MEASUREMENT			0x2a37
/** @def BT_UUID_HRS_BODY_SENSOR
 *  @brief HRS Characteristic Body Sensor Location
 */
#define BT_UUID_HRS_BODY_SENSOR			0x2a38
/** @def BT_UUID_HR_CONTROL_POINT
 *  @brief HRS Characteristic Control Point
 */
#define BT_UUID_HRS_CONTROL_POINT		0x2a39

/** @brief Bluetooth UUID types */
enum bt_uuid_type {
	BT_UUID_16,
	BT_UUID_128,
};

#define BT_UUID_DECLARE_16(value)					\
	((struct bt_uuid *) (&(struct __bt_uuid_16) { .type = BT_UUID_16, \
						   .u16 = value }))
#define BT_UUID_DECLARE_128(value)					\
	((struct bt_uuid *) (&(struct __bt_uuid_128) { .type = BT_UUID_128, \
						    .u128 = value }))

struct __bt_uuid_16 {
	uint8_t type;
	uint16_t u16;
};

struct __bt_uuid_128 {
	uint8_t type;
	uint8_t u128[16];
};

/** @brief Bluetooth UUID structure */
struct bt_uuid {
	/** UUID type */
	uint8_t  type;
	union {
		/** UUID 16 bits value */
		uint16_t u16;
		/** UUID 128 bits value */
		uint8_t  u128[16];
	};
};

/** @brief Compare Bluetooth UUIDs.
 *
 *  Compares 2 Bluetooth UUIDs, if the types are different both UUIDs are
 *  first converted to 128 bits format before comparing.
 *
 *  @param u1 First Bluetooth UUID to compare
 *  @param u2 Second Bluetooth UUID to compare
 *
 *  @return negative value if <u1> < <u2>, 0 if <u1> == <u2>, else positive
 */
int bt_uuid_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2);

#if defined(CONFIG_BLUETOOTH_DEBUG)
/** @brief Convert Bluetooth UUID to string.
 *
 *  Converts Bluetooth UUID to string. UUID has to be in 16 bits or 128 bits
 *  format.
 *
 *  @param uuid Bluetooth UUID
 *  @param str pointer where to put converted string
 *  @param len length of str
 *
 *  @return N/A
 */
void bt_uuid_to_str(const struct bt_uuid *uuid, char *str, size_t len);

/** @brief Convert Bluetooth UUID to string in place.
 *
 *  Converts Bluetooth UUID to string in place. UUID has to be in 16 bits or
 *  128 bits format.
 *
 *  @param uuid Bluetooth UUID
 *
 *  @return String representation of the UUID given
 */
const char *bt_uuid_str(const struct bt_uuid *uuid);
#endif /* CONFIG_BLUETOOTH_DEBUG */

#endif /* __BT_UUID_H */
