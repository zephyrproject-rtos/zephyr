/** @file
 *  @brief Bluetooth UUID handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_UUID_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_UUID_H_

/**
 * @brief UUIDs
 * @defgroup bt_uuid UUIDs
 * @ingroup bluetooth
 * @{
 */

#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bluetooth UUID types */
enum {
	BT_UUID_TYPE_16,
	BT_UUID_TYPE_32,
	BT_UUID_TYPE_128,
};

/** @brief This is a 'tentative' type and should be used as a pointer only */
struct bt_uuid {
	u8_t type;
};

struct bt_uuid_16 {
	struct bt_uuid uuid;
	u16_t val;
};

struct bt_uuid_32 {
	struct bt_uuid uuid;
	u32_t val;
};

struct bt_uuid_128 {
	struct bt_uuid uuid;
	u8_t val[16];
};

#define BT_UUID_INIT_16(value)		\
{					\
	.uuid.type = BT_UUID_TYPE_16,	\
	.val = (value),			\
}

#define BT_UUID_INIT_32(value)		\
{					\
	.uuid.type = BT_UUID_TYPE_32,	\
	.val = (value),			\
}

#define BT_UUID_INIT_128(value...)	\
{					\
	.uuid.type = BT_UUID_TYPE_128,	\
	.val = { value },		\
}

#define BT_UUID_DECLARE_16(value) \
	((struct bt_uuid *) (&(struct bt_uuid_16) BT_UUID_INIT_16(value)))
#define BT_UUID_DECLARE_32(value) \
	((struct bt_uuid *) (&(struct bt_uuid_32) BT_UUID_INIT_32(value)))
#define BT_UUID_DECLARE_128(value...) \
	((struct bt_uuid *) (&(struct bt_uuid_128) BT_UUID_INIT_128(value)))

#define BT_UUID_16(__u) CONTAINER_OF(__u, struct bt_uuid_16, uuid)
#define BT_UUID_32(__u) CONTAINER_OF(__u, struct bt_uuid_32, uuid)
#define BT_UUID_128(__u) CONTAINER_OF(__u, struct bt_uuid_128, uuid)


/**
 * @brief Encode 128 bit UUID into an array values
 *
 * Helper macro to initialize a 128-bit UUID value from the UUID format.
 * Can be combined with BT_UUID_DECLARE_128 to declare a 128-bit UUID from
 * the readable form of UUIDs.
 *
 * Example for how to declare the UUID `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
 *
 * @code
 * BT_UUID_DECLARE_128(
 *       BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E))
 * @endcode
 *
 * Just replace the hyphen by the comma and add `0x` prefixes.
 *
 * @param w32 First part of the UUID (32 bits)
 * @param w1  Second part of the UUID (16 bits)
 * @param w2  Third part of the UUID (16 bits)
 * @param w3  Fourth part of the UUID (16 bits)
 * @param w48 Fifth part of the UUID (48 bits)
 *
 * @return The comma separated values for UUID 128 initializer that
 *         may be used directly as an argument for
 *         @ref BT_UUID_INIT_128 or @ref BT_UUID_DECLARE_128
 */
#define BT_UUID_128_ENCODE(w32, w1, w2, w3, w48) \
	(((w48) >>  0) & 0xFF), \
	(((w48) >>  8) & 0xFF), \
	(((w48) >> 16) & 0xFF), \
	(((w48) >> 24) & 0xFF), \
	(((w48) >> 32) & 0xFF), \
	(((w48) >> 40) & 0xFF), \
	(((w3)  >>  0) & 0xFF), \
	(((w3)  >>  8) & 0xFF), \
	(((w2)  >>  0) & 0xFF), \
	(((w2)  >>  8) & 0xFF), \
	(((w1)  >>  0) & 0xFF), \
	(((w1)  >>  8) & 0xFF), \
	(((w32) >>  0) & 0xFF), \
	(((w32) >>  8) & 0xFF), \
	(((w32) >> 16) & 0xFF), \
	(((w32) >> 24) & 0xFF)

/** @def BT_UUID_GAP
 *  @brief Generic Access
 */
#define BT_UUID_GAP                       BT_UUID_DECLARE_16(0x1800)
/** @def BT_UUID_GATT
 *  @brief Generic Attribute
 */
#define BT_UUID_GATT                      BT_UUID_DECLARE_16(0x1801)
/** @def BT_UUID_CTS
 *  @brief Current Time Service
 */
#define BT_UUID_CTS                       BT_UUID_DECLARE_16(0x1805)
/** @def BT_UUID_HTS
 *  @brief Health Thermometer Service
 */
#define BT_UUID_HTS                       BT_UUID_DECLARE_16(0x1809)
/** @def BT_UUID_DIS
 *  @brief Device Information Service
 */
#define BT_UUID_DIS                       BT_UUID_DECLARE_16(0x180a)
/** @def BT_UUID_HRS
 *  @brief Heart Rate Service
 */
#define BT_UUID_HRS                       BT_UUID_DECLARE_16(0x180d)
/** @def BT_UUID_BAS
 *  @brief Battery Service
 */
#define BT_UUID_BAS                       BT_UUID_DECLARE_16(0x180f)
/** @def BT_UUID_HIDS
 *  @brief HID Service
 */
#define BT_UUID_HIDS                      BT_UUID_DECLARE_16(0x1812)
/** @def BT_UUID_CSC
 *  @brief Cycling Speed and Cadence Service
 */
#define BT_UUID_CSC                       BT_UUID_DECLARE_16(0x1816)
/** @def BT_UUID_ESS
 *  @brief Environmental Sensing Service
 */
#define BT_UUID_ESS                       BT_UUID_DECLARE_16(0x181a)
/** @def BT_UUID_IPSS
 *  @brief IP Support Service
 */
#define BT_UUID_IPSS                      BT_UUID_DECLARE_16(0x1820)
/** @def BT_UUID_MESH_PROV
 *  @brief Mesh Provisioning Service
 */
#define BT_UUID_MESH_PROV                 BT_UUID_DECLARE_16(0x1827)
/** @def BT_UUID_MESH_PROXY
 *  @brief Mesh Proxy Service
 */
#define BT_UUID_MESH_PROXY                BT_UUID_DECLARE_16(0x1828)
/** @def BT_UUID_GATT_PRIMARY
 *  @brief GATT Primary Service
 */
#define BT_UUID_GATT_PRIMARY              BT_UUID_DECLARE_16(0x2800)
/** @def BT_UUID_GATT_SECONDARY
 *  @brief GATT Secondary Service
 */
#define BT_UUID_GATT_SECONDARY            BT_UUID_DECLARE_16(0x2801)
/** @def BT_UUID_GATT_INCLUDE
 *  @brief GATT Include Service
 */
#define BT_UUID_GATT_INCLUDE              BT_UUID_DECLARE_16(0x2802)
/** @def BT_UUID_GATT_CHRC
 *  @brief GATT Characteristic
 */
#define BT_UUID_GATT_CHRC                 BT_UUID_DECLARE_16(0x2803)
/** @def BT_UUID_GATT_CEP
 *  @brief GATT Characteristic Extended Properties
 */
#define BT_UUID_GATT_CEP                  BT_UUID_DECLARE_16(0x2900)
/** @def BT_UUID_GATT_CUD
 *  @brief GATT Characteristic User Description
 */
#define BT_UUID_GATT_CUD                  BT_UUID_DECLARE_16(0x2901)
/** @def BT_UUID_GATT_CCC
 *  @brief GATT Client Characteristic Configuration
 */
#define BT_UUID_GATT_CCC                  BT_UUID_DECLARE_16(0x2902)
/** @def BT_UUID_GATT_SCC
 *  @brief GATT Server Characteristic Configuration
 */
#define BT_UUID_GATT_SCC                  BT_UUID_DECLARE_16(0x2903)
/** @def BT_UUID_GATT_CPF
 *  @brief GATT Characteristic Presentation Format
 */
#define BT_UUID_GATT_CPF                  BT_UUID_DECLARE_16(0x2904)
/** @def BT_UUID_VALID_RANGE
 *  @brief Valid Range Descriptor
 */
#define BT_UUID_VALID_RANGE               BT_UUID_DECLARE_16(0x2906)
/** @def BT_UUID_HIDS_EXT_REPORT
 *  @brief HID External Report Descriptor
 */
#define BT_UUID_HIDS_EXT_REPORT           BT_UUID_DECLARE_16(0x2907)
/** @def BT_UUID_HIDS_REPORT_REF
 *  @brief HID Report Reference Descriptor
 */
#define BT_UUID_HIDS_REPORT_REF           BT_UUID_DECLARE_16(0x2908)
/** @def BT_UUID_ES_CONFIGURATION
 *  @brief Environmental Sensing Configuration Descriptor
 */
#define BT_UUID_ES_CONFIGURATION          BT_UUID_DECLARE_16(0x290b)
/** @def BT_UUID_ES_MEASUREMENT
 *  @brief Environmental Sensing Measurement Descriptor
 */
#define BT_UUID_ES_MEASUREMENT            BT_UUID_DECLARE_16(0x290c)
/** @def BT_UUID_ES_TRIGGER_SETTING
 *  @brief Environmental Sensing Trigger Setting Descriptor
 */
#define BT_UUID_ES_TRIGGER_SETTING        BT_UUID_DECLARE_16(0x290d)
/** @def BT_UUID_GAP_DEVICE_NAME
 *  @brief GAP Characteristic Device Name
 */
#define BT_UUID_GAP_DEVICE_NAME           BT_UUID_DECLARE_16(0x2a00)
/** @def BT_UUID_GAP_APPEARANCE
 *  @brief GAP Characteristic Appearance
 */
#define BT_UUID_GAP_APPEARANCE            BT_UUID_DECLARE_16(0x2a01)
/** @def BT_UUID_GAP_PPCP
 *  @brief GAP Characteristic Peripheral Preferred Connection Parameters
 */
#define BT_UUID_GAP_PPCP                  BT_UUID_DECLARE_16(0x2a04)
/** @def BT_UUID_GATT_SC
 *  @brief GATT Characteristic Service Changed
 */
#define BT_UUID_GATT_SC                   BT_UUID_DECLARE_16(0x2a05)
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_BAS_BATTERY_LEVEL         BT_UUID_DECLARE_16(0x2a19)
/** @def BT_UUID_HTS_MEASUREMENT
 *  @brief HTS Characteristic Measurement Value
 */
#define BT_UUID_HTS_MEASUREMENT           BT_UUID_DECLARE_16(0x2a1c)
/** @def BT_UUID_HIDS_BOOT_KB_IN_REPORT
 *  @brief HID Characteristic Boot Keyboard Input Report
 */
#define BT_UUID_HIDS_BOOT_KB_IN_REPORT    BT_UUID_DECLARE_16(0x2a22)
/** @def BT_UUID_DIS_SYSTEM_ID
 *  @brief DIS Characteristic System ID
 */
#define BT_UUID_DIS_SYSTEM_ID             BT_UUID_DECLARE_16(0x2a23)
/** @def BT_UUID_DIS_MODEL_NUMBER
 *  @brief DIS Characteristic Model Number String
 */
#define BT_UUID_DIS_MODEL_NUMBER          BT_UUID_DECLARE_16(0x2a24)
/** @def BT_UUID_DIS_SERIAL_NUMBER
 *  @brief DIS Characteristic Serial Number String
 */
#define BT_UUID_DIS_SERIAL_NUMBER         BT_UUID_DECLARE_16(0x2a25)
/** @def BT_UUID_DIS_FIRMWARE_REVISION
 *  @brief DIS Characteristic Firmware Revision String
 */
#define BT_UUID_DIS_FIRMWARE_REVISION     BT_UUID_DECLARE_16(0x2a26)
/** @def BT_UUID_DIS_HARDWARE_REVISION
 *  @brief DIS Characteristic Hardware Revision String
 */
#define BT_UUID_DIS_HARDWARE_REVISION     BT_UUID_DECLARE_16(0x2a27)
/** @def BT_UUID_DIS_SOFTWARE_REVISION
 *  @brief DIS Characteristic Software Revision String
 */
#define BT_UUID_DIS_SOFTWARE_REVISION     BT_UUID_DECLARE_16(0x2a28)
/** @def BT_UUID_DIS_MANUFACTURER_NAME
 *  @brief DIS Characteristic Manufacturer Name String
 */
#define BT_UUID_DIS_MANUFACTURER_NAME     BT_UUID_DECLARE_16(0x2a29)
/** @def BT_UUID_DIS_PNP_ID
 *  @brief DIS Characteristic PnP ID
 */
#define BT_UUID_DIS_PNP_ID                BT_UUID_DECLARE_16(0x2a50)
/** @def BT_UUID_CTS_CURRENT_TIME
 *  @brief CTS Characteristic Current Time
 */
#define BT_UUID_CTS_CURRENT_TIME          BT_UUID_DECLARE_16(0x2a2b)
/** @def BT_UUID_MAGN_DECLINATION
 *  @brief Magnetic Declination Characteristic
 */
#define BT_UUID_MAGN_DECLINATION          BT_UUID_DECLARE_16(0x2a2c)
/** @def BT_UUID_HIDS_BOOT_KB_OUT_REPORT
 *  @brief HID Boot Keyboard Output Report Characteristic
 */
#define BT_UUID_HIDS_BOOT_KB_OUT_REPORT   BT_UUID_DECLARE_16(0x2a32)
/** @def BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT
 *  @brief HID Boot Mouse Input Report Characteristic
 */
#define BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT BT_UUID_DECLARE_16(0x2a33)
/** @def BT_UUID_HRS_MEASUREMENT
 *  @brief HRS Characteristic Measurement Interval
 */
#define BT_UUID_HRS_MEASUREMENT           BT_UUID_DECLARE_16(0x2a37)
/** @def BT_UUID_HRS_BODY_SENSOR
 *  @brief HRS Characteristic Body Sensor Location
 */
#define BT_UUID_HRS_BODY_SENSOR           BT_UUID_DECLARE_16(0x2a38)
/** @def BT_UUID_HRS_CONTROL_POINT
 *  @brief HRS Characteristic Control Point
 */
#define BT_UUID_HRS_CONTROL_POINT         BT_UUID_DECLARE_16(0x2a39)
/** @def BT_UUID_HIDS_INFO
 *  @brief HID Information Characteristic
 */
#define BT_UUID_HIDS_INFO                 BT_UUID_DECLARE_16(0x2a4a)
/** @def BT_UUID_HIDS_REPORT_MAP
 *  @brief HID Report Map Characteristic
 */
#define BT_UUID_HIDS_REPORT_MAP           BT_UUID_DECLARE_16(0x2a4b)
/** @def BT_UUID_HIDS_CTRL_POINT
 *  @brief HID Control Point Characteristic
 */
#define BT_UUID_HIDS_CTRL_POINT           BT_UUID_DECLARE_16(0x2a4c)
/** @def BT_UUID_HIDS_REPORT
 *  @brief HID Report Characteristic
 */
#define BT_UUID_HIDS_REPORT               BT_UUID_DECLARE_16(0x2a4d)
/** @def BT_UUID_HIDS_PROTOCOL_MODE
 *  @brief HID Protocol Mode Characteristic
 */
#define BT_UUID_HIDS_PROTOCOL_MODE        BT_UUID_DECLARE_16(0x2a4e)
/** @def BT_UUID_CSC_MEASUREMENT
 *  @brief CSC Measurement Characteristic
 */
#define BT_UUID_CSC_MEASUREMENT           BT_UUID_DECLARE_16(0x2a5b)
/** @def BT_UUID_CSC_FEATURE
 *  @brief CSC Feature Characteristic
 */
#define BT_UUID_CSC_FEATURE               BT_UUID_DECLARE_16(0x2a5c)
/** @def BT_UUID_SENSOR_LOCATION
 *  @brief Sensor Location Characteristic
 */
#define BT_UUID_SENSOR_LOCATION           BT_UUID_DECLARE_16(0x2a5d)
/** @def BT_UUID_SC_CONTROL_POINT
 *  @brief SC Control Point Characteristic
 */
#define BT_UUID_SC_CONTROL_POINT          BT_UUID_DECLARE_16(0x2a55)
/** @def BT_UUID_ELEVATION
 *  @brief Elevation Characteristic
 */
#define BT_UUID_ELEVATION                 BT_UUID_DECLARE_16(0x2a6c)
/** @def BT_UUID_PRESSURE
 *  @brief Pressure Characteristic
 */
#define BT_UUID_PRESSURE                  BT_UUID_DECLARE_16(0x2a6d)
/** @def BT_UUID_TEMPERATURE
 *  @brief Temperature Characteristic
 */
#define BT_UUID_TEMPERATURE               BT_UUID_DECLARE_16(0x2a6e)
/** @def BT_UUID_HUMIDITY
 *  @brief Humidity Characteristic
 */
#define BT_UUID_HUMIDITY                  BT_UUID_DECLARE_16(0x2a6f)
/** @def BT_UUID_TRUE_WIND_SPEED
 *  @brief True Wind Speed Characteristic
 */
#define BT_UUID_TRUE_WIND_SPEED           BT_UUID_DECLARE_16(0x2a70)
/** @def BT_UUID_TRUE_WIND_DIR
 *  @brief True Wind Direction Characteristic
 */
#define BT_UUID_TRUE_WIND_DIR             BT_UUID_DECLARE_16(0x2a71)
/** @def BT_UUID_APPARENT_WIND_SPEED
 *  @brief Apparent Wind Speed Characteristic
 */
#define BT_UUID_APPARENT_WIND_SPEED       BT_UUID_DECLARE_16(0x2a72)
/** @def BT_UUID_APPARENT_WIND_DIR
 *  @brief Apparent Wind Direction Characteristic
 */
#define BT_UUID_APPARENT_WIND_DIR         BT_UUID_DECLARE_16(0x2a73)
/** @def BT_UUID_GUST_FACTOR
 *  @brief Gust Factor Characteristic
 */
#define BT_UUID_GUST_FACTOR               BT_UUID_DECLARE_16(0x2a74)
/** @def BT_UUID_POLLEN_CONCENTRATION
 *  @brief Pollen Concentration Characteristic
 */
#define BT_UUID_POLLEN_CONCENTRATION      BT_UUID_DECLARE_16(0x2a75)
/** @def BT_UUID_UV_INDEX
 *  @brief UV Index Characteristic
 */
#define BT_UUID_UV_INDEX                  BT_UUID_DECLARE_16(0x2a76)
/** @def BT_UUID_IRRADIANCE
 *  @brief Irradiance Characteristic
 */
#define BT_UUID_IRRADIANCE                BT_UUID_DECLARE_16(0x2a77)
/** @def BT_UUID_RAINFALL
 *  @brief Rainfall Characteristic
 */
#define BT_UUID_RAINFALL                  BT_UUID_DECLARE_16(0x2a78)
/** @def BT_UUID_WIND_CHILL
 *  @brief Wind Chill Characteristic
 */
#define BT_UUID_WIND_CHILL                BT_UUID_DECLARE_16(0x2a79)
/** @def BT_UUID_HEAT_INDEX
 *  @brief Heat Index Characteristic
 */
#define BT_UUID_HEAT_INDEX                BT_UUID_DECLARE_16(0x2a7a)
/** @def BT_UUID_DEW_POINT
 *  @brief Dew Point Characteristic
 */
#define BT_UUID_DEW_POINT                 BT_UUID_DECLARE_16(0x2a7b)
/** @def BT_UUID_DESC_VALUE_CHANGED
 *  @brief Descriptor Value Changed Characteristic
 */
#define BT_UUID_DESC_VALUE_CHANGED        BT_UUID_DECLARE_16(0x2a7d)
/** @def BT_UUID_MAGN_FLUX_DENSITY_2D
 *  @brief Magnetic Flux Density - 2D Characteristic
 */
#define BT_UUID_MAGN_FLUX_DENSITY_2D      BT_UUID_DECLARE_16(0x2aa0)
/** @def BT_UUID_MAGN_FLUX_DENSITY_3D
 *  @brief Magnetic Flux Density - 3D Characteristic
 */
#define BT_UUID_MAGN_FLUX_DENSITY_3D      BT_UUID_DECLARE_16(0x2aa1)
/** @def BT_UUID_BAR_PRESSURE_TREND
 *  @brief Barometric Pressure Trend Characteristic
 */
#define BT_UUID_BAR_PRESSURE_TREND        BT_UUID_DECLARE_16(0x2aa3)
/** @def BT_UUID_CENTRAL_ADDR_RES
 *  @brief Central Address Resolution Characteristic
 */
#define BT_UUID_CENTRAL_ADDR_RES          BT_UUID_DECLARE_16(0x2aa6)
/** @def BT_UUID_MESH_PROV_DATA_IN
 *  @brief Mesh Provisioning Data In
 */
#define BT_UUID_MESH_PROV_DATA_IN         BT_UUID_DECLARE_16(0x2adb)
/** @def BT_UUID_MESH_PROV_DATA_OUT
 *  @brief Mesh Provisioning Data Out
 */
#define BT_UUID_MESH_PROV_DATA_OUT        BT_UUID_DECLARE_16(0x2adc)
/** @def BT_UUID_MESH_PROXY_DATA_IN
 *  @brief Mesh Proxy Data In
 */
#define BT_UUID_MESH_PROXY_DATA_IN        BT_UUID_DECLARE_16(0x2add)
/** @def BT_UUID_MESH_PROXY_DATA_OUT
 *  @brief Mesh Proxy Data Out
 */
#define BT_UUID_MESH_PROXY_DATA_OUT       BT_UUID_DECLARE_16(0x2ade)
/** @def BT_UUID_GATT_CLIENT_FEATURES
 *  @brief Client Supported Features
 */
#define BT_UUID_GATT_CLIENT_FEATURES      BT_UUID_DECLARE_16(0x2b29)
/** @def BT_UUID_GATT_DB_HASH
 *  @brief Database Hash
 */
#define BT_UUID_GATT_DB_HASH              BT_UUID_DECLARE_16(0x2b2a)

/*
 * Protocol UUIDs
 */
#define BT_UUID_SDP                       BT_UUID_DECLARE_16(0x0001)
#define BT_UUID_UDP                       BT_UUID_DECLARE_16(0x0002)
#define BT_UUID_RFCOMM                    BT_UUID_DECLARE_16(0x0003)
#define BT_UUID_TCP                       BT_UUID_DECLARE_16(0x0004)
#define BT_UUID_TCS_BIN                   BT_UUID_DECLARE_16(0x0005)
#define BT_UUID_TCS_AT                    BT_UUID_DECLARE_16(0x0006)
#define BT_UUID_ATT                       BT_UUID_DECLARE_16(0x0007)
#define BT_UUID_OBEX                      BT_UUID_DECLARE_16(0x0008)
#define BT_UUID_IP                        BT_UUID_DECLARE_16(0x0009)
#define BT_UUID_FTP                       BT_UUID_DECLARE_16(0x000a)
#define BT_UUID_HTTP                      BT_UUID_DECLARE_16(0x000c)
#define BT_UUID_BNEP                      BT_UUID_DECLARE_16(0x000f)
#define BT_UUID_UPNP                      BT_UUID_DECLARE_16(0x0010)
#define BT_UUID_HIDP                      BT_UUID_DECLARE_16(0x0011)
#define BT_UUID_HCRP_CTRL                 BT_UUID_DECLARE_16(0x0012)
#define BT_UUID_HCRP_DATA                 BT_UUID_DECLARE_16(0x0014)
#define BT_UUID_HCRP_NOTE                 BT_UUID_DECLARE_16(0x0016)
#define BT_UUID_AVCTP                     BT_UUID_DECLARE_16(0x0017)
#define BT_UUID_AVDTP                     BT_UUID_DECLARE_16(0x0019)
#define BT_UUID_CMTP                      BT_UUID_DECLARE_16(0x001b)
#define BT_UUID_UDI                       BT_UUID_DECLARE_16(0x001d)
#define BT_UUID_MCAP_CTRL                 BT_UUID_DECLARE_16(0x001e)
#define BT_UUID_MCAP_DATA                 BT_UUID_DECLARE_16(0x001f)
#define BT_UUID_L2CAP                     BT_UUID_DECLARE_16(0x0100)


/** @brief Compare Bluetooth UUIDs.
 *
 *  Compares 2 Bluetooth UUIDs, if the types are different both UUIDs are
 *  first converted to 128 bits format before comparing.
 *
 *  @param u1 First Bluetooth UUID to compare
 *  @param u2 Second Bluetooth UUID to compare
 *
 *  @return negative value if @a u1 < @a u2, 0 if @a u1 == @a u2, else positive
 */
int bt_uuid_cmp(const struct bt_uuid *u1, const struct bt_uuid *u2);

/** @brief Create a bt_uuid from a little-endian data buffer.
 *
 *  Create a bt_uuid from a little-endian data buffer. The data_len parameter
 *  is used to determine whether the UUID is in 16, 32 or 128 bit format
 *  (length 2, 4 or 16). Note: 32 bit format is not allowed over the air.
 *
 *  @param uuid Pointer to the bt_uuid variable
 *  @param data pointer to UUID stored in little-endian data buffer
 *  @param data_len length of the UUID in the data buffer
 *
 *  @return true if the data was valid and the UUID was successfully created.
 */
bool bt_uuid_create(struct bt_uuid *uuid, const u8_t *data, u8_t data_len);

#if defined(CONFIG_BT_DEBUG)
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

const char *bt_uuid_str_real(const struct bt_uuid *uuid);

/** @def bt_uuid_str
 *  @brief Convert Bluetooth UUID to string in place.
 *
 *  Converts Bluetooth UUID to string in place. UUID has to be in 16 bits or
 *  128 bits format.
 *
 *  @param uuid Bluetooth UUID
 *
 *  @return String representation of the UUID given
 */
#define bt_uuid_str(_uuid) log_strdup(bt_uuid_str_real(_uuid))
#else
static inline void bt_uuid_to_str(const struct bt_uuid *uuid, char *str,
				  size_t len)
{
	if (len > 0) {
		str[0] = '\0';
	}
}

static inline const char *bt_uuid_str(const struct bt_uuid *uuid)
{
	return "";
}
#endif /* CONFIG_BT_DEBUG */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_UUID_H_ */
