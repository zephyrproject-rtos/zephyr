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

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bluetooth UUID types */
enum {
	/** UUID type 16-bit. */
	BT_UUID_TYPE_16,
	/** UUID type 32-bit. */
	BT_UUID_TYPE_32,
	/** UUID type 128-bit. */
	BT_UUID_TYPE_128,
};

/** Size in octets of a 16-bit UUID */
#define BT_UUID_SIZE_16                  2

/** Size in octets of a 32-bit UUID */
#define BT_UUID_SIZE_32                  4

/** Size in octets of a 128-bit UUID */
#define BT_UUID_SIZE_128                 16

/** @brief This is a 'tentative' type and should be used as a pointer only */
struct bt_uuid {
	uint8_t type;
};

struct bt_uuid_16 {
	/** UUID generic type. */
	struct bt_uuid uuid;
	/** UUID value, 16-bit in host endianness. */
	uint16_t val;
};

struct bt_uuid_32 {
	/** UUID generic type. */
	struct bt_uuid uuid;
	/** UUID value, 32-bit in host endianness. */
	uint32_t val;
};

struct bt_uuid_128 {
	/** UUID generic type. */
	struct bt_uuid uuid;
	/** UUID value, 128-bit in little-endian format. */
	uint8_t val[BT_UUID_SIZE_128];
};

/** @brief Initialize a 16-bit UUID.
 *
 *  @param value 16-bit UUID value in host endianness.
 */
#define BT_UUID_INIT_16(value)		\
{					\
	.uuid = { BT_UUID_TYPE_16 },	\
	.val = (value),			\
}

/** @brief Initialize a 32-bit UUID.
 *
 *  @param value 32-bit UUID value in host endianness.
 */
#define BT_UUID_INIT_32(value)		\
{					\
	.uuid = { BT_UUID_TYPE_32 },	\
	.val = (value),			\
}

/** @brief Initialize a 128-bit UUID.
 *
 *  @param value 128-bit UUID array values in little-endian format.
 *               Can be combined with @ref BT_UUID_128_ENCODE to initialize a
 *               UUID from the readable form of UUIDs.
 */
#define BT_UUID_INIT_128(value...)	\
{					\
	.uuid = { BT_UUID_TYPE_128 },	\
	.val = { value },		\
}

/** @brief Helper to declare a 16-bit UUID inline.
 *
 *  @param value 16-bit UUID value in host endianness.
 *
 *  @return Pointer to a generic UUID.
 */
#define BT_UUID_DECLARE_16(value) \
	((struct bt_uuid *) ((struct bt_uuid_16[]) {BT_UUID_INIT_16(value)}))

/** @brief Helper to declare a 32-bit UUID inline.
 *
 *  @param value 32-bit UUID value in host endianness.
 *
 *  @return Pointer to a generic UUID.
 */
#define BT_UUID_DECLARE_32(value) \
	((struct bt_uuid *) ((struct bt_uuid_32[]) {BT_UUID_INIT_32(value)}))

/** @brief Helper to declare a 128-bit UUID inline.
 *
 *  @param value 128-bit UUID array values in little-endian format.
 *               Can be combined with @ref BT_UUID_128_ENCODE to declare a
 *               UUID from the readable form of UUIDs.
 *
 *  @return Pointer to a generic UUID.
 */
#define BT_UUID_DECLARE_128(value...) \
	((struct bt_uuid *) ((struct bt_uuid_128[]) {BT_UUID_INIT_128(value)}))

/** Helper macro to access the 16-bit UUID from a generic UUID. */
#define BT_UUID_16(__u) CONTAINER_OF(__u, struct bt_uuid_16, uuid)

/** Helper macro to access the 32-bit UUID from a generic UUID. */
#define BT_UUID_32(__u) CONTAINER_OF(__u, struct bt_uuid_32, uuid)

/** Helper macro to access the 128-bit UUID from a generic UUID. */
#define BT_UUID_128(__u) CONTAINER_OF(__u, struct bt_uuid_128, uuid)

/** @brief Encode 128 bit UUID into array values in little-endian format.
 *
 *  Helper macro to initialize a 128-bit UUID array value from the readable form
 *  of UUIDs, or encode 128-bit UUID values into advertising data
 *  Can be combined with BT_UUID_DECLARE_128 to declare a 128-bit UUID.
 *
 *  Example of how to declare the UUID `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
 *
 *  @code
 *  BT_UUID_DECLARE_128(
 *       BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E))
 *  @endcode
 *
 *  Example of how to encode the UUID `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
 *  into advertising data.
 *
 *  @code
 *  BT_DATA_BYTES(BT_DATA_UUID128_ALL,
 *       BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E))
 *  @endcode
 *
 *  Just replace the hyphen by the comma and add `0x` prefixes.
 *
 *  @param w32 First part of the UUID (32 bits)
 *  @param w1  Second part of the UUID (16 bits)
 *  @param w2  Third part of the UUID (16 bits)
 *  @param w3  Fourth part of the UUID (16 bits)
 *  @param w48 Fifth part of the UUID (48 bits)
 *
 *  @return The comma separated values for UUID 128 initializer that
 *          may be used directly as an argument for
 *          @ref BT_UUID_INIT_128 or @ref BT_UUID_DECLARE_128
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

/** @brief Encode 16-bit UUID into array values in little-endian format.
 *
 *  Helper macro to encode 16-bit UUID values into advertising data.
 *
 *  Example of how to encode the UUID `0x180a` into advertising data.
 *
 *  @code
 *  BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(0x180a))
 *  @endcode
 *
 * @param w16 UUID value (16-bits)
 *
 * @return The comma separated values for UUID 16 value that
 *         may be used directly as an argument for @ref BT_DATA_BYTES.
 */
#define BT_UUID_16_ENCODE(w16)  \
	(((w16) >>  0) & 0xFF), \
	(((w16) >>  8) & 0xFF)

/** @brief Encode 32-bit UUID into array values in little-endian format.
 *
 *  Helper macro to encode 32-bit UUID values into advertising data.
 *
 *  Example of how to encode the UUID `0x180a01af` into advertising data.
 *
 *  @code
 *  BT_DATA_BYTES(BT_DATA_UUID32_ALL, BT_UUID_32_ENCODE(0x180a01af))
 *  @endcode
 *
 * @param w32 UUID value (32-bits)
 *
 * @return The comma separated values for UUID 32 value that
 *         may be used directly as an argument for @ref BT_DATA_BYTES.
 */
#define BT_UUID_32_ENCODE(w32)  \
	(((w32) >>  0) & 0xFF), \
	(((w32) >>  8) & 0xFF), \
	(((w32) >> 16) & 0xFF), \
	(((w32) >> 24) & 0xFF)

/** @def BT_UUID_STR_LEN
 *
 *  @brief Recommended length of user string buffer for Bluetooth UUID.
 *
 *  @details The recommended length guarantee the output of UUID
 *  conversion will not lose valuable information about the UUID being
 *  processed. If the length of the UUID is known the string can be shorter.
 */
#define BT_UUID_STR_LEN 37

/** @def BT_UUID_GAP_VAL
 *  @brief Generic Access UUID value
 */
#define BT_UUID_GAP_VAL 0x1800
/** @def BT_UUID_GAP
 *  @brief Generic Access
 */
#define BT_UUID_GAP \
	BT_UUID_DECLARE_16(BT_UUID_GAP_VAL)
/** @def BT_UUID_GATT_VAL
 *  @brief Generic attribute UUID value
 */
#define BT_UUID_GATT_VAL 0x1801
/** @def BT_UUID_GATT
 *  @brief Generic Attribute
 */
#define BT_UUID_GATT \
	BT_UUID_DECLARE_16(BT_UUID_GATT_VAL)
/** @def BT_UUID_IAS_VAL
 *  @brief Immediate Alert Service UUID value
 */
#define BT_UUID_IAS_VAL 0x1802
/** @def BT_UUID_IAS
 *  @brief Immediate Alert Service
 */
#define BT_UUID_IAS \
	BT_UUID_DECLARE_16(BT_UUID_IAS_VAL)
/** @def BT_UUID_LLS_VAL
 *  @brief Link Loss Service UUID value
 */
#define BT_UUID_LLS_VAL 0x1803
/** @def BT_UUID_LLS
 *  @brief Link Loss Service
 */
#define BT_UUID_LLS \
	BT_UUID_DECLARE_16(BT_UUID_LLS_VAL)
/** @def BT_UUID_TPS_VAL
 *  @brief Tx Power Service UUID value
 */
#define BT_UUID_TPS_VAL 0x1804
/** @def BT_UUID_TPS
 *  @brief Tx Power Service
 */
#define BT_UUID_TPS \
	BT_UUID_DECLARE_16(BT_UUID_TPS_VAL)
/** @def BT_UUID_CTS_VAL
 *  @brief Current Time Service UUID value
 */
#define BT_UUID_CTS_VAL 0x1805
/** @def BT_UUID_CTS
 *  @brief Current Time Service
 */
#define BT_UUID_CTS \
	BT_UUID_DECLARE_16(BT_UUID_CTS_VAL)
/** @def BT_UUID_HTS_VAL
 *  @brief Health Thermometer Service UUID value
 */
#define BT_UUID_HTS_VAL 0x1809
/** @def BT_UUID_HTS
 *  @brief Health Thermometer Service
 */
#define BT_UUID_HTS \
	BT_UUID_DECLARE_16(BT_UUID_HTS_VAL)
/** @def BT_UUID_DIS_VAL
 *  @brief Device Information Service UUID value
 */
#define BT_UUID_DIS_VAL 0x180a
/** @def BT_UUID_DIS
 *  @brief Device Information Service
 */
#define BT_UUID_DIS \
	BT_UUID_DECLARE_16(BT_UUID_DIS_VAL)
/** @def BT_UUID_HRS_VAL
 *  @brief Heart Rate Service UUID value
 */
#define BT_UUID_HRS_VAL 0x180d
/** @def BT_UUID_HRS
 *  @brief Heart Rate Service
 */
#define BT_UUID_HRS \
	BT_UUID_DECLARE_16(BT_UUID_HRS_VAL)
/** @def BT_UUID_BAS_VAL
 *  @brief Battery Service UUID value
 */
#define BT_UUID_BAS_VAL 0x180f
/** @def BT_UUID_BAS
 *  @brief Battery Service
 */
#define BT_UUID_BAS \
	BT_UUID_DECLARE_16(BT_UUID_BAS_VAL)
/** @def BT_UUID_HIDS_VAL
 *  @brief HID Service UUID value
 */
#define BT_UUID_HIDS_VAL 0x1812
/** @def BT_UUID_HIDS
 *  @brief HID Service
 */
#define BT_UUID_HIDS \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_VAL)
/** @def BT_UUID_RSCS_VAL
 *  @brief Running Speed and Cadence Service UUID value
 */
#define BT_UUID_RSCS_VAL 0x1814
/** @def BT_UUID_RSCS
 *  @brief Running Speed and Cadence Service
 */
#define BT_UUID_RSCS \
	BT_UUID_DECLARE_16(BT_UUID_RSCS_VAL)
/** @def BT_UUID_CSC_VAL
 *  @brief Cycling Speed and Cadence Service UUID value
 */
#define BT_UUID_CSC_VAL 0x1816
/** @def BT_UUID_CSC
 *  @brief Cycling Speed and Cadence Service
 */
#define BT_UUID_CSC \
	BT_UUID_DECLARE_16(BT_UUID_CSC_VAL)
/** @def BT_UUID_ESS_VAL
 *  @brief Environmental Sensing Service UUID value
 */
#define BT_UUID_ESS_VAL 0x181a
/** @def BT_UUID_ESS
 *  @brief Environmental Sensing Service
 */
#define BT_UUID_ESS \
	BT_UUID_DECLARE_16(BT_UUID_ESS_VAL)
/** @def BT_UUID_BMS_VAL
 *  @brief Bond Management Service UUID value
 */
#define BT_UUID_BMS_VAL 0x181e
/** @def BT_UUID_BMS
 *  @brief Bond Management Service
 */
#define BT_UUID_BMS \
	BT_UUID_DECLARE_16(BT_UUID_BMS_VAL)
/** @def BT_UUID_CGMS_VAL
 *  @brief Continuous Glucose Monitoring Service UUID value
 */
#define BT_UUID_CGMS_VAL 0x181f
/** @def BT_UUID_CGMS
 *  @brief Continuous Glucose Monitoring Service
 */
#define BT_UUID_CGMS \
	BT_UUID_DECLARE_16(BT_UUID_CGMS_VAL)
/** @def BT_UUID_IPSS_VAL
 *  @brief IP Support Service UUID value
 */
#define BT_UUID_IPSS_VAL 0x1820
/** @def BT_UUID_IPSS
 *  @brief IP Support Service
 */
#define BT_UUID_IPSS \
	BT_UUID_DECLARE_16(BT_UUID_IPSS_VAL)
/** @def BT_UUID_HPS_VAL
 *  @brief HTTP Proxy Service UUID value
 */
#define BT_UUID_HPS_VAL 0x1823
/** @def BT_UUID_HPS
 *  @brief HTTP Proxy Service
 */
#define BT_UUID_HPS \
	BT_UUID_DECLARE_16(BT_UUID_HPS_VAL)
/** @def BT_UUID_OTS_VAL
 *  @brief Object Transfer Service UUID value
 */
#define BT_UUID_OTS_VAL 0x1825
/** @def BT_UUID_OTS
 *  @brief Object Transfer Service
 */
#define BT_UUID_OTS \
	BT_UUID_DECLARE_16(BT_UUID_OTS_VAL)
/** @def BT_UUID_MESH_PROV_VAL
 *  @brief Mesh Provisioning Service UUID value
 */
#define BT_UUID_MESH_PROV_VAL 0x1827
/** @def BT_UUID_MESH_PROV
 *  @brief Mesh Provisioning Service
 */
#define BT_UUID_MESH_PROV \
	BT_UUID_DECLARE_16(BT_UUID_MESH_PROV_VAL)
/** @def BT_UUID_MESH_PROXY_VAL
 *  @brief Mesh Proxy Service UUID value
 */
#define BT_UUID_MESH_PROXY_VAL 0x1828
/** @def BT_UUID_MESH_PROXY
 *  @brief Mesh Proxy Service
 */
#define BT_UUID_MESH_PROXY \
	BT_UUID_DECLARE_16(BT_UUID_MESH_PROXY_VAL)
/** @def BT_UUID_AICS_VAL
 *  @brief Audio Input Control Service value
 */
#define BT_UUID_AICS_VAL 0x1843
/** @def BT_UUID_AICS
 *  @brief Audio Input Control Service
 */
#define BT_UUID_AICS \
	BT_UUID_DECLARE_16(BT_UUID_AICS_VAL)
/** @def BT_UUID_VCS_VAL
 *  @brief Volume Control Service value
 */
#define BT_UUID_VCS_VAL 0x1844
/** @def BT_UUID_VCS
 *  @brief Volume Control Service
 */
#define BT_UUID_VCS \
	BT_UUID_DECLARE_16(BT_UUID_VCS_VAL)
/** @def BT_UUID_VOCS_VAL
 *  @brief Volume Offset Control Service value
 */
#define BT_UUID_VOCS_VAL 0x1845
/** @def BT_UUID_VOCS
 *  @brief Volume Offset Control Service
 */
#define BT_UUID_VOCS \
	BT_UUID_DECLARE_16(BT_UUID_VOCS_VAL)
/** @def BT_UUID_CSIS_VAL
 *  @brief Coordinated Set Identification Service value
 */
#define BT_UUID_CSIS_VAL 0x1846
/** @def BT_UUID_CSIS
 *  @brief Coordinated Set Identification Service
 */
#define BT_UUID_CSIS \
	BT_UUID_DECLARE_16(BT_UUID_CSIS_VAL)
/** @def BT_UUID_MCS_VAL
 *  @brief Media Control Service value
 */
#define BT_UUID_MCS_VAL 0x1848
/** @def BT_UUID_MCS
 *  @brief Media Control Service
 */
#define BT_UUID_MCS \
	BT_UUID_DECLARE_16(BT_UUID_MCS_VAL)
/** @def BT_UUID_GMCS_VAL
 *  @brief Generic Media Control Service value
 */
#define BT_UUID_GMCS_VAL 0x1849
/** @def BT_UUID_GMCS
 *  @brief Generic Media Control Service
 */
#define BT_UUID_GMCS \
	BT_UUID_DECLARE_16(BT_UUID_GMCS_VAL)
/** @def BT_UUID_TBS_VAL
 *  @brief Telephone Bearer Service value
 */
#define BT_UUID_TBS_VAL 0x184B
/** @def BT_UUID_TBS
 *  @brief Telephone Bearer Service
 */
#define BT_UUID_TBS \
	BT_UUID_DECLARE_16(BT_UUID_TBS_VAL)
/** @def BT_UUID_GTBS_VAL
 *  @brief Generic Telephone Bearer Service value
 */
#define BT_UUID_GTBS_VAL 0x184C
/** @def BT_UUID_GTBS
 *  @brief Generic Telephone Bearer Service
 */
#define BT_UUID_GTBS \
	BT_UUID_DECLARE_16(BT_UUID_GTBS_VAL)
/** @def BT_UUID_MICS_VAL
 *  @brief Microphone Input Control Service value
 */
#define BT_UUID_MICS_VAL 0x184D
/** @def BT_UUID_MICS
 *  @brief Microphone Input Control Service
 */
#define BT_UUID_MICS \
	BT_UUID_DECLARE_16(BT_UUID_MICS_VAL)
/** @def BT_UUID_ASCS_VAL
 *  @brief Audio Stream Control Service value
 */
#define BT_UUID_ASCS_VAL 0x184E
/** @def BT_UUID_ASCS
 *  @brief Audio Stream Control Service
 */
#define BT_UUID_ASCS \
	BT_UUID_DECLARE_16(BT_UUID_ASCS_VAL)
/** @def BT_UUID_BASS_VAL
 *  @brief Broadcast Audio Scan Service value
 */
#define BT_UUID_BASS_VAL 0x184F
/** @def BT_UUID_BASS
 *  @brief Broadcast Audio Scan Service
 */
#define BT_UUID_BASS \
	BT_UUID_DECLARE_16(BT_UUID_BASS_VAL)
/** @def BT_UUID_PACS_VAL
 *  @brief Published Audio Capabilities Service value
 */
#define BT_UUID_PACS_VAL 0x1850
/** @def BT_UUID_PACS
 *  @brief Published Audio Capabilities Service
 */
#define BT_UUID_PACS \
	BT_UUID_DECLARE_16(BT_UUID_PACS_VAL)
/** @def BT_UUID_BASIC_AUDIO_VAL
 *  @brief Basic Audio Announcement Service value
 */
#define BT_UUID_BASIC_AUDIO_VAL 0x1851
/** @def BT_UUID_BASIC_AUDIO
 *  @brief Basic Audio Announcement Service
 */
#define BT_UUID_BASIC_AUDIO \
	BT_UUID_DECLARE_16(BT_UUID_BASIC_AUDIO_VAL)
/** @def BT_UUID_BROADCAST_AUDIO_VAL
 *  @brief Broadcast Audio Announcement Service value
 */
#define BT_UUID_BROADCAST_AUDIO_VAL 0x1852
/** @def BT_UUID_BROADCAST_AUDIO
 *  @brief Broadcast Audio Announcement Service
 */
#define BT_UUID_BROADCAST_AUDIO \
	BT_UUID_DECLARE_16(BT_UUID_BROADCAST_AUDIO_VAL)
/** @def BT_UUID_CAS_VAL
 *  @brief Common Audio Service value
 */
#define BT_UUID_CAS_VAL 0x1853
/** @def BT_UUID_CAS
 *  @brief Common Audio Service
 */
#define BT_UUID_CAS \
	BT_UUID_DECLARE_16(BT_UUID_CAS_VAL)
/** @def BT_UUID_HAS_VAL
 *  @brief Hearing Access Service value
 */
#define BT_UUID_HAS_VAL 0x1854
/** @def BT_UUID_HAS
 *  @brief Hearing Access Service
 */
#define BT_UUID_HAS \
	BT_UUID_DECLARE_16(BT_UUID_HAS_VAL)
/** @def BT_UUID_GATT_PRIMARY_VAL
 *  @brief GATT Primary Service UUID value
 */
#define BT_UUID_GATT_PRIMARY_VAL 0x2800
/** @def BT_UUID_GATT_PRIMARY
 *  @brief GATT Primary Service
 */
#define BT_UUID_GATT_PRIMARY \
	BT_UUID_DECLARE_16(BT_UUID_GATT_PRIMARY_VAL)
/** @def BT_UUID_GATT_SECONDARY_VAL
 *  @brief GATT Secondary Service UUID value
 */
#define BT_UUID_GATT_SECONDARY_VAL 0x2801
/** @def BT_UUID_GATT_SECONDARY
 *  @brief GATT Secondary Service
 */
#define BT_UUID_GATT_SECONDARY \
	BT_UUID_DECLARE_16(BT_UUID_GATT_SECONDARY_VAL)
/** @def BT_UUID_GATT_INCLUDE_VAL
 *  @brief GATT Include Service UUID value
 */
#define BT_UUID_GATT_INCLUDE_VAL 0x2802
/** @def BT_UUID_GATT_INCLUDE
 *  @brief GATT Include Service
 */
#define BT_UUID_GATT_INCLUDE \
	BT_UUID_DECLARE_16(BT_UUID_GATT_INCLUDE_VAL)
/** @def BT_UUID_GATT_CHRC_VAL
 *  @brief GATT Characteristic UUID value
 */
#define BT_UUID_GATT_CHRC_VAL 0x2803
/** @def BT_UUID_GATT_CHRC
 *  @brief GATT Characteristic
 */
#define BT_UUID_GATT_CHRC \
	BT_UUID_DECLARE_16(BT_UUID_GATT_CHRC_VAL)
/** @def BT_UUID_GATT_CEP_VAL
 *  @brief GATT Characteristic Extended Properties UUID value
 */
#define BT_UUID_GATT_CEP_VAL 0x2900
/** @def BT_UUID_GATT_CEP
 *  @brief GATT Characteristic Extended Properties
 */
#define BT_UUID_GATT_CEP \
	BT_UUID_DECLARE_16(BT_UUID_GATT_CEP_VAL)
/** @def BT_UUID_GATT_CUD_VAL
 *  @brief GATT Characteristic User Description UUID value
 */
#define BT_UUID_GATT_CUD_VAL 0x2901
/** @def BT_UUID_GATT_CUD
 *  @brief GATT Characteristic User Description
 */
#define BT_UUID_GATT_CUD \
	BT_UUID_DECLARE_16(BT_UUID_GATT_CUD_VAL)
/** @def BT_UUID_GATT_CCC_VAL
 *  @brief GATT Client Characteristic Configuration UUID value
 */
#define BT_UUID_GATT_CCC_VAL 0x2902
/** @def BT_UUID_GATT_CCC
 *  @brief GATT Client Characteristic Configuration
 */
#define BT_UUID_GATT_CCC \
	BT_UUID_DECLARE_16(BT_UUID_GATT_CCC_VAL)
/** @def BT_UUID_GATT_SCC_VAL
 *  @brief GATT Server Characteristic Configuration UUID value
 */
#define BT_UUID_GATT_SCC_VAL 0x2903
/** @def BT_UUID_GATT_SCC
 *  @brief GATT Server Characteristic Configuration
 */
#define BT_UUID_GATT_SCC \
	BT_UUID_DECLARE_16(BT_UUID_GATT_SCC_VAL)
/** @def BT_UUID_GATT_CPF_VAL
 *  @brief GATT Characteristic Presentation Format UUID value
 */
#define BT_UUID_GATT_CPF_VAL 0x2904
/** @def BT_UUID_GATT_CPF
 *  @brief GATT Characteristic Presentation Format
 */
#define BT_UUID_GATT_CPF \
	BT_UUID_DECLARE_16(BT_UUID_GATT_CPF_VAL)
/** @def BT_UUID_GATT_CAF_VAL
 *  @brief GATT Characteristic Aggregated Format UUID value
 */
#define BT_UUID_GATT_CAF_VAL 0x2905
/** @def BT_UUID_GATT_CAF
 *  @brief GATT Characteristic Aggregated Format
 */
#define BT_UUID_GATT_CAF \
	BT_UUID_DECLARE_16(BT_UUID_GATT_CAF_VAL)
/** @def BT_UUID_VALID_RANGE_VAL
 *  @brief Valid Range Descriptor UUID value
 */
#define BT_UUID_VALID_RANGE_VAL 0x2906
/** @def BT_UUID_VALID_RANGE
 *  @brief Valid Range Descriptor
 */
#define BT_UUID_VALID_RANGE \
	BT_UUID_DECLARE_16(BT_UUID_VALID_RANGE_VAL)
/** @def BT_UUID_HIDS_EXT_REPORT_VAL
 *  @brief HID External Report Descriptor UUID value
 */
#define BT_UUID_HIDS_EXT_REPORT_VAL 0x2907
/** @def BT_UUID_HIDS_EXT_REPORT
 *  @brief HID External Report Descriptor
 */
#define BT_UUID_HIDS_EXT_REPORT \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_EXT_REPORT_VAL)
/** @def BT_UUID_HIDS_REPORT_REF_VAL
 *  @brief HID Report Reference Descriptor UUID value
 */
#define BT_UUID_HIDS_REPORT_REF_VAL 0x2908
/** @def BT_UUID_HIDS_REPORT_REF
 *  @brief HID Report Reference Descriptor
 */
#define BT_UUID_HIDS_REPORT_REF \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_REPORT_REF_VAL)
/** @def BT_UUID_ES_CONFIGURATION_VAL
 *  @brief Environmental Sensing Configuration Descriptor UUID value
 */
#define BT_UUID_ES_CONFIGURATION_VAL 0x290b
/** @def BT_UUID_ES_CONFIGURATION
 *  @brief Environmental Sensing Configuration Descriptor
 */
#define BT_UUID_ES_CONFIGURATION \
	BT_UUID_DECLARE_16(BT_UUID_ES_CONFIGURATION_VAL)
/** @def BT_UUID_ES_MEASUREMENT_VAL
 *  @brief Environmental Sensing Measurement Descriptor UUID value
 */
#define BT_UUID_ES_MEASUREMENT_VAL 0x290c
/** @def BT_UUID_ES_MEASUREMENT
 *  @brief Environmental Sensing Measurement Descriptor
 */
#define BT_UUID_ES_MEASUREMENT \
	BT_UUID_DECLARE_16(BT_UUID_ES_MEASUREMENT_VAL)
/** @def BT_UUID_ES_TRIGGER_SETTING_VAL
 *  @brief Environmental Sensing Trigger Setting Descriptor UUID value
 */
#define BT_UUID_ES_TRIGGER_SETTING_VAL 0x290d
/** @def BT_UUID_ES_TRIGGER_SETTING
 *  @brief Environmental Sensing Trigger Setting Descriptor
 */
#define BT_UUID_ES_TRIGGER_SETTING \
	BT_UUID_DECLARE_16(BT_UUID_ES_TRIGGER_SETTING_VAL)
/** @def BT_UUID_GAP_DEVICE_NAME_VAL
 *  @brief GAP Characteristic Device Name UUID value
 */
#define BT_UUID_GAP_DEVICE_NAME_VAL 0x2a00
/** @def BT_UUID_GAP_DEVICE_NAME
 *  @brief GAP Characteristic Device Name
 */
#define BT_UUID_GAP_DEVICE_NAME \
	BT_UUID_DECLARE_16(BT_UUID_GAP_DEVICE_NAME_VAL)
/** @def BT_UUID_GAP_APPEARANCE_VAL
 *  @brief GAP Characteristic Appearance UUID value
 */
#define BT_UUID_GAP_APPEARANCE_VAL 0x2a01
/** @def BT_UUID_GAP_APPEARANCE
 *  @brief GAP Characteristic Appearance
 */
#define BT_UUID_GAP_APPEARANCE \
	BT_UUID_DECLARE_16(BT_UUID_GAP_APPEARANCE_VAL)
/** @def BT_UUID_GAP_PPCP_VAL
 *  @brief GAP Characteristic Peripheral Preferred Connection Parameters UUID
 *         value
 */
#define BT_UUID_GAP_PPCP_VAL 0x2a04
/** @def BT_UUID_GAP_PPCP
 *  @brief GAP Characteristic Peripheral Preferred Connection Parameters
 */
#define BT_UUID_GAP_PPCP \
	BT_UUID_DECLARE_16(BT_UUID_GAP_PPCP_VAL)
/** @def BT_UUID_GATT_SC_VAL
 *  @brief GATT Characteristic Service Changed UUID value
 */
#define BT_UUID_GATT_SC_VAL 0x2a05
/** @def BT_UUID_GATT_SC
 *  @brief GATT Characteristic Service Changed
 */
#define BT_UUID_GATT_SC \
	BT_UUID_DECLARE_16(BT_UUID_GATT_SC_VAL)
/** @def BT_UUID_ALERT_LEVEL_VAL
 *  @brief Alert Level UUID value
 */
#define BT_UUID_ALERT_LEVEL_VAL  0x2a06
/** @def BT_UUID_ALERT_LEVEL
 *  @brief Alert Level
 */
#define BT_UUID_ALERT_LEVEL \
	BT_UUID_DECLARE_16(BT_UUID_ALERT_LEVEL_VAL)
/** @def BT_UUID_TPS_TX_POWER_LEVEL_VAL
 *  @brief TPS Characteristic Tx Power Level UUID value
 */
#define BT_UUID_TPS_TX_POWER_LEVEL_VAL 0x2a07
/** @def BT_UUID_TPS_TX_POWER_LEVEL
 *  @brief TPS Characteristic Tx Power Level
 */
#define BT_UUID_TPS_TX_POWER_LEVEL \
	BT_UUID_DECLARE_16(BT_UUID_TPS_TX_POWER_LEVEL_VAL)
/** @def BT_UUID_BAS_BATTERY_LEVEL_VAL
 *  @brief BAS Characteristic Battery Level UUID value
 */
#define BT_UUID_BAS_BATTERY_LEVEL_VAL 0x2a19
/** @def BT_UUID_BAS_BATTERY_LEVEL
 *  @brief BAS Characteristic Battery Level
 */
#define BT_UUID_BAS_BATTERY_LEVEL \
	BT_UUID_DECLARE_16(BT_UUID_BAS_BATTERY_LEVEL_VAL)
/** @def BT_UUID_HTS_MEASUREMENT_VAL
 *  @brief HTS Characteristic Measurement Value UUID value
 */
#define BT_UUID_HTS_MEASUREMENT_VAL 0x2a1c
/** @def BT_UUID_HTS_MEASUREMENT
 *  @brief HTS Characteristic Measurement Value
 */
#define BT_UUID_HTS_MEASUREMENT \
	BT_UUID_DECLARE_16(BT_UUID_HTS_MEASUREMENT_VAL)
/** @def BT_UUID_HIDS_BOOT_KB_IN_REPORT_VAL
 *  @brief HID Characteristic Boot Keyboard Input Report UUID value
 */
#define BT_UUID_HIDS_BOOT_KB_IN_REPORT_VAL 0x2a22
/** @def BT_UUID_HIDS_BOOT_KB_IN_REPORT
 *  @brief HID Characteristic Boot Keyboard Input Report
 */
#define BT_UUID_HIDS_BOOT_KB_IN_REPORT \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_BOOT_KB_IN_REPORT_VAL)
/** @def BT_UUID_DIS_SYSTEM_ID_VAL
 *  @brief DIS Characteristic System ID UUID value
 */
#define BT_UUID_DIS_SYSTEM_ID_VAL 0x2a23
/** @def BT_UUID_DIS_SYSTEM_ID
 *  @brief DIS Characteristic System ID
 */
#define BT_UUID_DIS_SYSTEM_ID \
	BT_UUID_DECLARE_16(BT_UUID_DIS_SYSTEM_ID_VAL)
/** @def BT_UUID_DIS_MODEL_NUMBER_VAL
 *  @brief DIS Characteristic Model Number String UUID value
 */
#define BT_UUID_DIS_MODEL_NUMBER_VAL 0x2a24
/** @def BT_UUID_DIS_MODEL_NUMBER
 *  @brief DIS Characteristic Model Number String
 */
#define BT_UUID_DIS_MODEL_NUMBER \
	BT_UUID_DECLARE_16(BT_UUID_DIS_MODEL_NUMBER_VAL)
/** @def BT_UUID_DIS_SERIAL_NUMBER_VAL
 *  @brief DIS Characteristic Serial Number String UUID value
 */
#define BT_UUID_DIS_SERIAL_NUMBER_VAL 0x2a25
/** @def BT_UUID_DIS_SERIAL_NUMBER
 *  @brief DIS Characteristic Serial Number String
 */
#define BT_UUID_DIS_SERIAL_NUMBER \
	BT_UUID_DECLARE_16(BT_UUID_DIS_SERIAL_NUMBER_VAL)
/** @def BT_UUID_DIS_FIRMWARE_REVISION_VAL
 *  @brief DIS Characteristic Firmware Revision String UUID value
 */
#define BT_UUID_DIS_FIRMWARE_REVISION_VAL 0x2a26
/** @def BT_UUID_DIS_FIRMWARE_REVISION
 *  @brief DIS Characteristic Firmware Revision String
 */
#define BT_UUID_DIS_FIRMWARE_REVISION \
	BT_UUID_DECLARE_16(BT_UUID_DIS_FIRMWARE_REVISION_VAL)
/** @def BT_UUID_DIS_HARDWARE_REVISION_VAL
 *  @brief DIS Characteristic Hardware Revision String UUID value
 */
#define BT_UUID_DIS_HARDWARE_REVISION_VAL 0x2a27
/** @def BT_UUID_DIS_HARDWARE_REVISION
 *  @brief DIS Characteristic Hardware Revision String
 */
#define BT_UUID_DIS_HARDWARE_REVISION \
	BT_UUID_DECLARE_16(BT_UUID_DIS_HARDWARE_REVISION_VAL)
/** @def BT_UUID_DIS_SOFTWARE_REVISION_VAL
 *  @brief DIS Characteristic Software Revision String UUID value
 */
#define BT_UUID_DIS_SOFTWARE_REVISION_VAL 0x2a28
/** @def BT_UUID_DIS_SOFTWARE_REVISION
 *  @brief DIS Characteristic Software Revision String
 */
#define BT_UUID_DIS_SOFTWARE_REVISION \
	BT_UUID_DECLARE_16(BT_UUID_DIS_SOFTWARE_REVISION_VAL)
/** @def BT_UUID_DIS_MANUFACTURER_NAME_VAL
 *  @brief DIS Characteristic Manufacturer Name String UUID Value
 */
#define BT_UUID_DIS_MANUFACTURER_NAME_VAL 0x2a29
/** @def BT_UUID_DIS_MANUFACTURER_NAME
 *  @brief DIS Characteristic Manufacturer Name String
 */
#define BT_UUID_DIS_MANUFACTURER_NAME \
	BT_UUID_DECLARE_16(BT_UUID_DIS_MANUFACTURER_NAME_VAL)
/** @def BT_UUID_DIS_PNP_ID_VAL
 *  @brief DIS Characteristic PnP ID UUID value
 */
#define BT_UUID_DIS_PNP_ID_VAL 0x2a50
/** @def BT_UUID_DIS_PNP_ID
 *  @brief DIS Characteristic PnP ID
 */
#define BT_UUID_DIS_PNP_ID \
	BT_UUID_DECLARE_16(BT_UUID_DIS_PNP_ID_VAL)
/** @def BT_UUID_CTS_CURRENT_TIME_VAL
 *  @brief CTS Characteristic Current Time UUID value
 */
#define BT_UUID_CTS_CURRENT_TIME_VAL 0x2a2b
/** @def BT_UUID_CTS_CURRENT_TIME
 *  @brief CTS Characteristic Current Time
 */
#define BT_UUID_CTS_CURRENT_TIME \
	BT_UUID_DECLARE_16(BT_UUID_CTS_CURRENT_TIME_VAL)
/** @def BT_UUID_MAGN_DECLINATION_VAL
 *  @brief Magnetic Declination Characteristic UUID value
 */
#define BT_UUID_MAGN_DECLINATION_VAL 0x2a2c
/** @def BT_UUID_MAGN_DECLINATION
 *  @brief Magnetic Declination Characteristic
 */
#define BT_UUID_MAGN_DECLINATION \
	BT_UUID_DECLARE_16(BT_UUID_MAGN_DECLINATION_VAL)
/** @def BT_UUID_HIDS_BOOT_KB_OUT_REPORT_VAL
 *  @brief HID Boot Keyboard Output Report Characteristic UUID value
 */
#define BT_UUID_HIDS_BOOT_KB_OUT_REPORT_VAL 0x2a32
/** @def BT_UUID_HIDS_BOOT_KB_OUT_REPORT
 *  @brief HID Boot Keyboard Output Report Characteristic
 */
#define BT_UUID_HIDS_BOOT_KB_OUT_REPORT \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_BOOT_KB_OUT_REPORT_VAL)
/** @def BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT_VAL
 *  @brief HID Boot Mouse Input Report Characteristic UUID value
 */
#define BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT_VAL 0x2a33
/** @def BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT
 *  @brief HID Boot Mouse Input Report Characteristic
 */
#define BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT_VAL)
/** @def BT_UUID_HRS_MEASUREMENT_VAL
 *  @brief HRS Characteristic Measurement Interval UUID value
 */
#define BT_UUID_HRS_MEASUREMENT_VAL 0x2a37
/** @def BT_UUID_HRS_MEASUREMENT
 *  @brief HRS Characteristic Measurement Interval
 */
#define BT_UUID_HRS_MEASUREMENT \
	BT_UUID_DECLARE_16(BT_UUID_HRS_MEASUREMENT_VAL)
/** @def BT_UUID_HRS_BODY_SENSOR
 *  @brief HRS Characteristic Body Sensor Location
 */
#define BT_UUID_HRS_BODY_SENSOR_VAL 0x2a38
/** @def BT_UUID_HRS_CONTROL_POINT
 *  @brief HRS Characteristic Control Point
 */
#define BT_UUID_HRS_BODY_SENSOR \
	BT_UUID_DECLARE_16(BT_UUID_HRS_BODY_SENSOR_VAL)
/** @def BT_UUID_HRS_CONTROL_POINT_VAL
 *  @brief HRS Characteristic Control Point UUID value
 */
#define BT_UUID_HRS_CONTROL_POINT_VAL 0x2a39
/** @def BT_UUID_HRS_CONTROL_POINT
 *  @brief HRS Characteristic Control Point
 */
#define BT_UUID_HRS_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_HRS_CONTROL_POINT_VAL)
/** @def BT_UUID_HIDS_INFO_VAL
 *  @brief HID Information Characteristic UUID value
 */
#define BT_UUID_HIDS_INFO_VAL 0x2a4a
/** @def BT_UUID_HIDS_INFO
 *  @brief HID Information Characteristic
 */
#define BT_UUID_HIDS_INFO \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_INFO_VAL)
/** @def BT_UUID_HIDS_REPORT_MAP_VAL
 *  @brief HID Report Map Characteristic UUID value
 */
#define BT_UUID_HIDS_REPORT_MAP_VAL 0x2a4b
/** @def BT_UUID_HIDS_REPORT_MAP
 *  @brief HID Report Map Characteristic
 */
#define BT_UUID_HIDS_REPORT_MAP \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_REPORT_MAP_VAL)
/** @def BT_UUID_HIDS_CTRL_POINT_VAL
 *  @brief HID Control Point Characteristic UUID value
 */
#define BT_UUID_HIDS_CTRL_POINT_VAL 0x2a4c
/** @def BT_UUID_HIDS_CTRL_POINT
 *  @brief HID Control Point Characteristic
 */
#define BT_UUID_HIDS_CTRL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_CTRL_POINT_VAL)
/** @def BT_UUID_HIDS_REPORT_VAL
 *  @brief HID Report Characteristic UUID value
 */
#define BT_UUID_HIDS_REPORT_VAL 0x2a4d
/** @def BT_UUID_HIDS_REPORT
 *  @brief HID Report Characteristic
 */
#define BT_UUID_HIDS_REPORT \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_REPORT_VAL)
/** @def BT_UUID_HIDS_PROTOCOL_MODE_VAL
 *  @brief HID Protocol Mode Characteristic UUID value
 */
#define BT_UUID_HIDS_PROTOCOL_MODE_VAL 0x2a4e
/** @def BT_UUID_HIDS_PROTOCOL_MODE
 *  @brief HID Protocol Mode Characteristic
 */
#define BT_UUID_HIDS_PROTOCOL_MODE \
	BT_UUID_DECLARE_16(BT_UUID_HIDS_PROTOCOL_MODE_VAL)
/** @def BT_UUID_RECORD_ACCESS_CONTROL_POINT_VAL
 *  @brief Record Access Control Point Characteristic value
 */
#define BT_UUID_RECORD_ACCESS_CONTROL_POINT_VAL 0x2a52
/** @def BT_UUID_RECORD_ACCESS_CONTROL_POINT
 *  @brief Record Access Control Point
 */
#define BT_UUID_RECORD_ACCESS_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_RECORD_ACCESS_CONTROL_POINT_VAL)
/** @def BT_UUID_RSC_MEASUREMENT_VAL
 *  @brief RSC Measurement Characteristic UUID value
 */
#define BT_UUID_RSC_MEASUREMENT_VAL 0x2a53
/** @def BT_UUID_RSC_MEASUREMENT
 *  @brief RSC Measurement Characteristic
 */
#define BT_UUID_RSC_MEASUREMENT \
	BT_UUID_DECLARE_16(BT_UUID_RSC_MEASUREMENT_VAL)
/** @def BT_UUID_RSC_FEATURE_VAL
 *  @brief RSC Feature Characteristic UUID value
 */
#define BT_UUID_RSC_FEATURE_VAL 0x2a54
/** @def BT_UUID_RSC_FEATURE
 *  @brief RSC Feature Characteristic
 */
#define BT_UUID_RSC_FEATURE \
	BT_UUID_DECLARE_16(BT_UUID_RSC_FEATURE_VAL)
/** @def BT_UUID_CSC_MEASUREMENT_VAL
 *  @brief CSC Measurement Characteristic UUID value
 */
#define BT_UUID_CSC_MEASUREMENT_VAL 0x2a5b
/** @def BT_UUID_CSC_MEASUREMENT
 *  @brief CSC Measurement Characteristic
 */
#define BT_UUID_CSC_MEASUREMENT \
	BT_UUID_DECLARE_16(BT_UUID_CSC_MEASUREMENT_VAL)
/** @def BT_UUID_CSC_FEATURE_VAL
 *  @brief CSC Feature Characteristic UUID value
 */
#define BT_UUID_CSC_FEATURE_VAL 0x2a5c
/** @def BT_UUID_CSC_FEATURE
 *  @brief CSC Feature Characteristic
 */
#define BT_UUID_CSC_FEATURE \
	BT_UUID_DECLARE_16(BT_UUID_CSC_FEATURE_VAL)
/** @def BT_UUID_SENSOR_LOCATION_VAL
 *  @brief Sensor Location Characteristic UUID value
 */
#define BT_UUID_SENSOR_LOCATION_VAL 0x2a5d
/** @def BT_UUID_SENSOR_LOCATION
 *  @brief Sensor Location Characteristic
 */
#define BT_UUID_SENSOR_LOCATION \
	BT_UUID_DECLARE_16(BT_UUID_SENSOR_LOCATION_VAL)
/** @def BT_UUID_SC_CONTROL_POINT_VAL
 *  @brief SC Control Point Characteristic UUID value
 */
#define BT_UUID_SC_CONTROL_POINT_VAL 0x2a55
/** @def BT_UUID_SC_CONTROL_POINT
 *  @brief SC Control Point Characteristic
 */
#define BT_UUID_SC_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_SC_CONTROL_POINT_VAL)
/** @def BT_UUID_ELEVATION_VAL
 *  @brief Elevation Characteristic UUID value
 */
#define BT_UUID_ELEVATION_VAL 0x2a6c
/** @def BT_UUID_ELEVATION
 *  @brief Elevation Characteristic
 */
#define BT_UUID_ELEVATION \
	BT_UUID_DECLARE_16(BT_UUID_ELEVATION_VAL)
/** @def BT_UUID_PRESSURE_VAL
 *  @brief Pressure Characteristic UUID value
 */
#define BT_UUID_PRESSURE_VAL 0x2a6d
/** @def BT_UUID_PRESSURE
 *  @brief Pressure Characteristic
 */
#define BT_UUID_PRESSURE \
	BT_UUID_DECLARE_16(BT_UUID_PRESSURE_VAL)
/** @def BT_UUID_TEMPERATURE_VAL
 *  @brief Temperature Characteristic UUID value
 */
#define BT_UUID_TEMPERATURE_VAL 0x2a6e
/** @def BT_UUID_TEMPERATURE
 *  @brief Temperature Characteristic
 */
#define BT_UUID_TEMPERATURE \
	BT_UUID_DECLARE_16(BT_UUID_TEMPERATURE_VAL)
/** @def BT_UUID_HUMIDITY_VAL
 *  @brief Humidity Characteristic UUID value
 */
#define BT_UUID_HUMIDITY_VAL 0x2a6f
/** @def BT_UUID_HUMIDITY
 *  @brief Humidity Characteristic
 */
#define BT_UUID_HUMIDITY \
	BT_UUID_DECLARE_16(BT_UUID_HUMIDITY_VAL)
/** @def BT_UUID_TRUE_WIND_SPEED_VAL
 *  @brief True Wind Speed Characteristic UUID value
 */
#define BT_UUID_TRUE_WIND_SPEED_VAL 0x2a70
/** @def BT_UUID_TRUE_WIND_SPEED
 *  @brief True Wind Speed Characteristic
 */
#define BT_UUID_TRUE_WIND_SPEED \
	BT_UUID_DECLARE_16(BT_UUID_TRUE_WIND_SPEED_VAL)
/** @def BT_UUID_TRUE_WIND_DIR_VAL
 *  @brief True Wind Direction Characteristic UUID value
 */
#define BT_UUID_TRUE_WIND_DIR_VAL 0x2a71
/** @def BT_UUID_TRUE_WIND_DIR
 *  @brief True Wind Direction Characteristic
 */
#define BT_UUID_TRUE_WIND_DIR \
	BT_UUID_DECLARE_16(BT_UUID_TRUE_WIND_DIR_VAL)
/** @def BT_UUID_APPARENT_WIND_SPEED_VAL
 *  @brief Apparent Wind Speed Characteristic UUID value
 */
#define BT_UUID_APPARENT_WIND_SPEED_VAL 0x2a72
/** @def BT_UUID_APPARENT_WIND_SPEED
 *  @brief Apparent Wind Speed Characteristic
 */
#define BT_UUID_APPARENT_WIND_SPEED \
	BT_UUID_DECLARE_16(BT_UUID_APPARENT_WIND_SPEED_VAL)
/** @def BT_UUID_APPARENT_WIND_DIR_VAL
 *  @brief Apparent Wind Direction Characteristic UUID value
 */
#define BT_UUID_APPARENT_WIND_DIR_VAL 0x2a73
/** @def BT_UUID_APPARENT_WIND_DIR
 *  @brief Apparent Wind Direction Characteristic
 */
#define BT_UUID_APPARENT_WIND_DIR \
	BT_UUID_DECLARE_16(BT_UUID_APPARENT_WIND_DIR_VAL)
/** @def BT_UUID_GUST_FACTOR_VAL
 *  @brief Gust Factor Characteristic UUID value
 */
#define BT_UUID_GUST_FACTOR_VAL 0x2a74
/** @def BT_UUID_GUST_FACTOR
 *  @brief Gust Factor Characteristic
 */
#define BT_UUID_GUST_FACTOR \
	BT_UUID_DECLARE_16(BT_UUID_GUST_FACTOR_VAL)
/** @def BT_UUID_POLLEN_CONCENTRATION_VAL
 *  @brief Pollen Concentration Characteristic UUID value
 */
#define BT_UUID_POLLEN_CONCENTRATION_VAL 0x2a75
/** @def BT_UUID_POLLEN_CONCENTRATION
 *  @brief Pollen Concentration Characteristic
 */
#define BT_UUID_POLLEN_CONCENTRATION \
	BT_UUID_DECLARE_16(BT_UUID_POLLEN_CONCENTRATION_VAL)
/** @def BT_UUID_UV_INDEX_VAL
 *  @brief UV Index Characteristic UUID value
 */
#define BT_UUID_UV_INDEX_VAL 0x2a76
/** @def BT_UUID_UV_INDEX
 *  @brief UV Index Characteristic
 */
#define BT_UUID_UV_INDEX \
	BT_UUID_DECLARE_16(BT_UUID_UV_INDEX_VAL)
/** @def BT_UUID_IRRADIANCE_VAL
 *  @brief Irradiance Characteristic UUID value
 */
#define BT_UUID_IRRADIANCE_VAL 0x2a77
/** @def BT_UUID_IRRADIANCE
 *  @brief Irradiance Characteristic
 */
#define BT_UUID_IRRADIANCE \
	BT_UUID_DECLARE_16(BT_UUID_IRRADIANCE_VAL)
/** @def BT_UUID_RAINFALL_VAL
 *  @brief Rainfall Characteristic UUID value
 */
#define BT_UUID_RAINFALL_VAL 0x2a78
/** @def BT_UUID_RAINFALL
 *  @brief Rainfall Characteristic
 */
#define BT_UUID_RAINFALL \
	BT_UUID_DECLARE_16(BT_UUID_RAINFALL_VAL)
/** @def BT_UUID_WIND_CHILL_VAL
 *  @brief Wind Chill Characteristic UUID value
 */
#define BT_UUID_WIND_CHILL_VAL 0x2a79
/** @def BT_UUID_WIND_CHILL
 *  @brief Wind Chill Characteristic
 */
#define BT_UUID_WIND_CHILL \
	BT_UUID_DECLARE_16(BT_UUID_WIND_CHILL_VAL)
/** @def BT_UUID_HEAT_INDEX_VAL
 *  @brief Heat Index Characteristic UUID value
 */
#define BT_UUID_HEAT_INDEX_VAL 0x2a7a
/** @def BT_UUID_HEAT_INDEX
 *  @brief Heat Index Characteristic
 */
#define BT_UUID_HEAT_INDEX \
	BT_UUID_DECLARE_16(BT_UUID_HEAT_INDEX_VAL)
/** @def BT_UUID_DEW_POINT_VAL
 *  @brief Dew Point Characteristic UUID value
 */
#define BT_UUID_DEW_POINT_VAL 0x2a7b
/** @def BT_UUID_DEW_POINT
 *  @brief Dew Point Characteristic
 */
#define BT_UUID_DEW_POINT \
	BT_UUID_DECLARE_16(BT_UUID_DEW_POINT_VAL)
/** @def BT_UUID_DESC_VALUE_CHANGED_VAL
 *  @brief Descriptor Value Changed Characteristic UUID value
 */
#define BT_UUID_DESC_VALUE_CHANGED_VAL 0x2a7d
/** @def BT_UUID_DESC_VALUE_CHANGED
 *  @brief Descriptor Value Changed Characteristic
 */
#define BT_UUID_DESC_VALUE_CHANGED \
	BT_UUID_DECLARE_16(BT_UUID_DESC_VALUE_CHANGED_VAL)
/** @def BT_UUID_MAGN_FLUX_DENSITY_2D_VAL
 *  @brief Magnetic Flux Density - 2D Characteristic UUID value
 */
#define BT_UUID_MAGN_FLUX_DENSITY_2D_VAL 0x2aa0
/** @def BT_UUID_MAGN_FLUX_DENSITY_2D
 *  @brief Magnetic Flux Density - 2D Characteristic
 */
#define BT_UUID_MAGN_FLUX_DENSITY_2D \
	BT_UUID_DECLARE_16(BT_UUID_MAGN_FLUX_DENSITY_2D_VAL)
/** @def BT_UUID_MAGN_FLUX_DENSITY_3D_VAL
 *  @brief Magnetic Flux Density - 3D Characteristic UUID value
 */
#define BT_UUID_MAGN_FLUX_DENSITY_3D_VAL 0x2aa1
/** @def BT_UUID_MAGN_FLUX_DENSITY_3D
 *  @brief Magnetic Flux Density - 3D Characteristic
 */
#define BT_UUID_MAGN_FLUX_DENSITY_3D \
	BT_UUID_DECLARE_16(BT_UUID_MAGN_FLUX_DENSITY_3D_VAL)
/** @def BT_UUID_BAR_PRESSURE_TREND_VAL
 *  @brief Barometric Pressure Trend Characteristic UUID value
 */
#define BT_UUID_BAR_PRESSURE_TREND_VAL 0x2aa3
/** @def BT_UUID_BAR_PRESSURE_TREND
 *  @brief Barometric Pressure Trend Characteristic
 */
#define BT_UUID_BAR_PRESSURE_TREND \
	BT_UUID_DECLARE_16(BT_UUID_BAR_PRESSURE_TREND_VAL)
/** @def BT_UUID_BMS_CONTROL_POINT_VAL
 *  @brief Bond Management Control Point UUID value
 */
#define BT_UUID_BMS_CONTROL_POINT_VAL 0x2aa4
/** @def BT_UUID_BMS_CONTROL_POINT
 *  @brief Bond Management Control Point
 */
#define BT_UUID_BMS_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_BMS_CONTROL_POINT_VAL)
/** @def BT_UUID_BMS_FEATURE_VAL
 *  @brief Bond Management Feature UUID value
 */
#define BT_UUID_BMS_FEATURE_VAL 0x2aa5
/** @def BT_UUID_BMS_FEATURE
 *  @brief Bond Management Feature
 */
#define BT_UUID_BMS_FEATURE \
	BT_UUID_DECLARE_16(BT_UUID_BMS_FEATURE_VAL)
/** @def BT_UUID_CENTRAL_ADDR_RES_VAL
 *  @brief Central Address Resolution Characteristic UUID value
 */
#define BT_UUID_CENTRAL_ADDR_RES_VAL 0x2aa6
/** @def BT_UUID_CENTRAL_ADDR_RES
 *  @brief Central Address Resolution Characteristic
 */
#define BT_UUID_CENTRAL_ADDR_RES \
	BT_UUID_DECLARE_16(BT_UUID_CENTRAL_ADDR_RES_VAL)
/** @def BT_UUID_CGM_MEASUREMENT_VAL
 *  @brief CGM Measurement Characteristic value
 */
#define BT_UUID_CGM_MEASUREMENT_VAL 0x2aa7
/** @def BT_UUID_CGM_MEASUREMENT
 *  @brief CGM Measurement Characteristic
 */
#define BT_UUID_CGM_MEASUREMENT \
	BT_UUID_DECLARE_16(BT_UUID_CGM_MEASUREMENT_VAL)
/** @def BT_UUID_CGM_FEATURE_VAL
 *  @brief CGM Feature Characteristic value
 */
#define BT_UUID_CGM_FEATURE_VAL 0x2aa8
/** @def BT_UUID_CGM_FEATURE
 *  @brief CGM Feature Characteristic
 */
#define BT_UUID_CGM_FEATURE \
	BT_UUID_DECLARE_16(BT_UUID_CGM_FEATURE_VAL)
/** @def BT_UUID_CGM_STATUS_VAL
 *  @brief CGM Status Characteristic value
 */
#define BT_UUID_CGM_STATUS_VAL 0x2aa9
/** @def BT_UUID_CGM_STATUS
 *  @brief CGM Status Characteristic
 */
#define BT_UUID_CGM_STATUS \
	BT_UUID_DECLARE_16(BT_UUID_CGM_STATUS_VAL)
/** @def BT_UUID_CGM_SESSION_START_TIME_VAL
 *  @brief CGM Session Start Time Characteristic value
 */
#define BT_UUID_CGM_SESSION_START_TIME_VAL 0x2aaa
/** @def BT_UUID_CGM_SESSION_START_TIME
 *  @brief CGM Session Start Time
 */
#define BT_UUID_CGM_SESSION_START_TIME \
	BT_UUID_DECLARE_16(BT_UUID_CGM_SESSION_START_TIME_VAL)
/** @def BT_UUID_CGM_SESSION_RUN_TIME_VAL
 *  @brief CGM Session Run Time Characteristic value
 */
#define BT_UUID_CGM_SESSION_RUN_TIME_VAL 0x2aab
/** @def BT_UUID_CGM_SESSION_RUN_TIME
 *  @brief CGM Session Run Time
 */
#define BT_UUID_CGM_SESSION_RUN_TIME \
	BT_UUID_DECLARE_16(BT_UUID_CGM_SESSION_RUN_TIME_VAL)
/** @def BT_UUID_CGM_SPECIFIC_OPS_CONTROL_POINT_VAL
 *  @brief CGM Specific Ops Control Point Characteristic value
 */
#define BT_UUID_CGM_SPECIFIC_OPS_CONTROL_POINT_VAL 0x2aac
/** @def BT_UUID_CGM_SPECIFIC_OPS_CONTROL_POINT
 *  @brief CGM Specific Ops Control Point
 */
#define BT_UUID_CGM_SPECIFIC_OPS_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_CGM_SPECIFIC_OPS_CONTROL_POINT_VAL)
/** @def BT_UUID_URI_VAL
 *  @brief URI UUID value
 */
#define BT_UUID_URI_VAL 0x2ab6
/** @def BT_UUID_URI
 *  @brief URI
 */
#define BT_UUID_URI \
	BT_UUID_DECLARE_16(BT_UUID_URI_VAL)
/** @def BT_UUID_HTTP_HEADERS_VAL
 *  @brief HTTP Headers UUID value
 */
#define BT_UUID_HTTP_HEADERS_VAL 0x2ab7
/** @def BT_UUID_HTTP_HEADERS
 *  @brief HTTP Headers
 */
#define BT_UUID_HTTP_HEADERS \
	BT_UUID_DECLARE_16(BT_UUID_HTTP_HEADERS_VAL)
/** @def BT_UUID_HTTP_STATUS_CODE_VAL
 *  @brief HTTP Status Code UUID value
 */
#define BT_UUID_HTTP_STATUS_CODE_VAL 0x2ab8
/** @def BT_UUID_HTTP_STATUS_CODE
 *  @brief HTTP Status Code
 */
#define BT_UUID_HTTP_STATUS_CODE \
	BT_UUID_DECLARE_16(BT_UUID_HTTP_STATUS_CODE_VAL)
/** @def BT_UUID_HTTP_ENTITY_BODY_VAL
 *  @brief HTTP Entity Body UUID value
 */
#define BT_UUID_HTTP_ENTITY_BODY_VAL 0x2ab9
/** @def BT_UUID_HTTP_ENTITY_BODY
 *  @brief HTTP Entity Body
 */
#define BT_UUID_HTTP_ENTITY_BODY \
	BT_UUID_DECLARE_16(BT_UUID_HTTP_ENTITY_BODY_VAL)
/** @def BT_UUID_HTTP_CONTROL_POINT_VAL
 *  @brief HTTP Control Point UUID value
 */
#define BT_UUID_HTTP_CONTROL_POINT_VAL 0x2aba
/** @def BT_UUID_HTTP_CONTROL_POINT
 *  @brief HTTP Control Point
 */
#define BT_UUID_HTTP_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_HTTP_CONTROL_POINT_VAL)
/** @def BT_UUID_HTTPS_SECURITY_VAL
 *  @brief HTTPS Security UUID value
 */
#define BT_UUID_HTTPS_SECURITY_VAL 0x2abb
/** @def BT_UUID_HTTPS_SECURITY
 *  @brief HTTPS Security
 */
#define BT_UUID_HTTPS_SECURITY \
	BT_UUID_DECLARE_16(BT_UUID_HTTPS_SECURITY_VAL)
/** @def BT_UUID_OTS_FEATURE_VAL
 *  @brief OTS Feature Characteristic UUID value
 */
#define BT_UUID_OTS_FEATURE_VAL 0x2abd
/** @def BT_UUID_OTS_FEATURE
 *  @brief OTS Feature Characteristic
 */
#define BT_UUID_OTS_FEATURE \
	BT_UUID_DECLARE_16(BT_UUID_OTS_FEATURE_VAL)
/** @def BT_UUID_OTS_NAME_VAL
 *  @brief OTS Object Name Characteristic UUID value
 */
#define BT_UUID_OTS_NAME_VAL 0x2abe
/** @def BT_UUID_OTS_NAME
 *  @brief OTS Object Name Characteristic
 */
#define BT_UUID_OTS_NAME \
	BT_UUID_DECLARE_16(BT_UUID_OTS_NAME_VAL)
/** @def BT_UUID_OTS_TYPE_VAL
 *  @brief OTS Object Type Characteristic UUID value
 */
#define BT_UUID_OTS_TYPE_VAL 0x2abf
/** @def BT_UUID_OTS_TYPE
 *  @brief OTS Object Type Characteristic
 */
#define BT_UUID_OTS_TYPE \
	BT_UUID_DECLARE_16(BT_UUID_OTS_TYPE_VAL)
/** @def BT_UUID_OTS_SIZE_VAL
 *  @brief OTS Object Size Characteristic UUID value
 */
#define BT_UUID_OTS_SIZE_VAL 0x2ac0
/** @def BT_UUID_OTS_SIZE
 *  @brief OTS Object Size Characteristic
 */
#define BT_UUID_OTS_SIZE \
	BT_UUID_DECLARE_16(BT_UUID_OTS_SIZE_VAL)
/** @def BT_UUID_OTS_FIRST_CREATED_VAL
 *  @brief OTS Object First-Created Characteristic UUID value
 */
#define BT_UUID_OTS_FIRST_CREATED_VAL 0x2ac1
/** @def BT_UUID_OTS_FIRST_CREATED
 *  @brief OTS Object First-Created Characteristic
 */
#define BT_UUID_OTS_FIRST_CREATED \
	BT_UUID_DECLARE_16(BT_UUID_OTS_FIRST_CREATED_VAL)
/** @def BT_UUID_OTS_LAST_MODIFIED_VAL
 *  @brief OTS Object Last-Modified Characteristic UUI value
 */
#define BT_UUID_OTS_LAST_MODIFIED_VAL 0x2ac2
/** @def BT_UUID_OTS_LAST_MODIFIED
 *  @brief OTS Object Last-Modified Characteristic
 */
#define BT_UUID_OTS_LAST_MODIFIED \
	BT_UUID_DECLARE_16(BT_UUID_OTS_LAST_MODIFIED_VAL)
/** @def BT_UUID_OTS_ID_VAL
 *  @brief OTS Object ID Characteristic UUID value
 */
#define BT_UUID_OTS_ID_VAL 0x2ac3
/** @def BT_UUID_OTS_ID
 *  @brief OTS Object ID Characteristic
 */
#define BT_UUID_OTS_ID \
	BT_UUID_DECLARE_16(BT_UUID_OTS_ID_VAL)
/** @def BT_UUID_OTS_PROPERTIES_VAL
 *  @brief OTS Object Properties Characteristic UUID value
 */
#define BT_UUID_OTS_PROPERTIES_VAL 0x2ac4
/** @def BT_UUID_OTS_PROPERTIES
 *  @brief OTS Object Properties Characteristic
 */
#define BT_UUID_OTS_PROPERTIES \
	BT_UUID_DECLARE_16(BT_UUID_OTS_PROPERTIES_VAL)
/** @def BT_UUID_OTS_ACTION_CP_VAL
 *  @brief OTS Object Action Control Point Characteristic UUID value
 */
#define BT_UUID_OTS_ACTION_CP_VAL 0x2ac5
/** @def BT_UUID_OTS_ACTION_CP
 *  @brief OTS Object Action Control Point Characteristic
 */
#define BT_UUID_OTS_ACTION_CP \
	BT_UUID_DECLARE_16(BT_UUID_OTS_ACTION_CP_VAL)
/** @def BT_UUID_OTS_LIST_CP_VAL
 *  @brief OTS Object List Control Point Characteristic UUID value
 */
#define BT_UUID_OTS_LIST_CP_VAL 0x2ac6
/** @def BT_UUID_OTS_LIST_CP
 *  @brief OTS Object List Control Point Characteristic
 */
#define BT_UUID_OTS_LIST_CP \
	BT_UUID_DECLARE_16(BT_UUID_OTS_LIST_CP_VAL)
/** @def BT_UUID_OTS_LIST_FILTER_VAL
 *  @brief OTS Object List Filter Characteristic UUID value
 */
#define BT_UUID_OTS_LIST_FILTER_VAL 0x2ac7
/** @def BT_UUID_OTS_LIST_FILTER
 *  @brief OTS Object List Filter Characteristic
 */
#define BT_UUID_OTS_LIST_FILTER \
	BT_UUID_DECLARE_16(BT_UUID_OTS_LIST_FILTER_VAL)
/** @def BT_UUID_OTS_CHANGED_VAL
 *  @brief OTS Object Changed Characteristic UUID value
 */
#define BT_UUID_OTS_CHANGED_VAL 0x2ac8
/** @def BT_UUID_OTS_CHANGED
 *  @brief OTS Object Changed Characteristic
 */
#define BT_UUID_OTS_CHANGED \
	BT_UUID_DECLARE_16(BT_UUID_OTS_CHANGED_VAL)
/** @def BT_UUID_OTS_TYPE_UNSPECIFIED_VAL
 *  @brief OTS Unspecified Object Type UUID value
 */
#define BT_UUID_OTS_TYPE_UNSPECIFIED_VAL 0x2aca
/** @def BT_UUID_OTS_TYPE_UNSPECIFIED
 *  @brief OTS Unspecified Object Type
 */
#define BT_UUID_OTS_TYPE_UNSPECIFIED \
	BT_UUID_DECLARE_16(BT_UUID_OTS_TYPE_UNSPECIFIED_VAL)
/** @def BT_UUID_OTS_DIRECTORY_LISTING_VAL
 *  @brief OTS Directory Listing UUID value
 */
#define BT_UUID_OTS_DIRECTORY_LISTING_VAL 0x2acb
/** @def BT_UUID_OTS_DIRECTORY_LISTING
 *  @brief OTS Directory Listing
 */
#define BT_UUID_OTS_DIRECTORY_LISTING \
	BT_UUID_DECLARE_16(BT_UUID_OTS_DIRECTORY_LISTING_VAL)
/** @def BT_UUID_MESH_PROV_DATA_IN_VAL
 *  @brief Mesh Provisioning Data In UUID value
 */
#define BT_UUID_MESH_PROV_DATA_IN_VAL 0x2adb
/** @def BT_UUID_MESH_PROV_DATA_IN
 *  @brief Mesh Provisioning Data In
 */
#define BT_UUID_MESH_PROV_DATA_IN \
	BT_UUID_DECLARE_16(BT_UUID_MESH_PROV_DATA_IN_VAL)
/** @def BT_UUID_MESH_PROV_DATA_OUT_VAL
 *  @brief Mesh Provisioning Data Out UUID value
 */
#define BT_UUID_MESH_PROV_DATA_OUT_VAL 0x2adc
/** @def BT_UUID_MESH_PROV_DATA_OUT
 *  @brief Mesh Provisioning Data Out
 */
#define BT_UUID_MESH_PROV_DATA_OUT \
	BT_UUID_DECLARE_16(BT_UUID_MESH_PROV_DATA_OUT_VAL)
/** @def BT_UUID_MESH_PROXY_DATA_IN_VAL
 *  @brief Mesh Proxy Data In UUID value
 */
#define BT_UUID_MESH_PROXY_DATA_IN_VAL 0x2add
/** @def BT_UUID_MESH_PROXY_DATA_IN
 *  @brief Mesh Proxy Data In
 */
#define BT_UUID_MESH_PROXY_DATA_IN \
	BT_UUID_DECLARE_16(BT_UUID_MESH_PROXY_DATA_IN_VAL)
/** @def BT_UUID_MESH_PROXY_DATA_OUT_VAL
 *  @brief Mesh Proxy Data Out UUID value
 */
#define BT_UUID_MESH_PROXY_DATA_OUT_VAL 0x2ade
/** @def BT_UUID_MESH_PROXY_DATA_OUT
 *  @brief Mesh Proxy Data Out
 */
#define BT_UUID_MESH_PROXY_DATA_OUT \
	BT_UUID_DECLARE_16(BT_UUID_MESH_PROXY_DATA_OUT_VAL)
/** @def BT_UUID_GATT_CLIENT_FEATURES_VAL
 *  @brief Client Supported Features UUID value
 */
#define BT_UUID_GATT_CLIENT_FEATURES_VAL 0x2b29
/** @def BT_UUID_GATT_CLIENT_FEATURES
 *  @brief Client Supported Features
 */
#define BT_UUID_GATT_CLIENT_FEATURES \
	BT_UUID_DECLARE_16(BT_UUID_GATT_CLIENT_FEATURES_VAL)
/** @def BT_UUID_GATT_DB_HASH_VAL
 *  @brief Database Hash UUID value
 */
#define BT_UUID_GATT_DB_HASH_VAL 0x2b2a
/** @def BT_UUID_GATT_DB_HASH
 *  @brief Database Hash
 */
#define BT_UUID_GATT_DB_HASH \
	BT_UUID_DECLARE_16(BT_UUID_GATT_DB_HASH_VAL)

/** @def BT_UUID_GATT_SERVER_FEATURES_VAL
 *  @brief Server Supported Features UUID value
 */
#define BT_UUID_GATT_SERVER_FEATURES_VAL  0x2b3a
/** @def BT_UUID_GATT_SERVER_FEATURES
 *  @brief Server Supported Features
 */
#define BT_UUID_GATT_SERVER_FEATURES      \
	BT_UUID_DECLARE_16(BT_UUID_GATT_SERVER_FEATURES_VAL)

/** @def BT_UUID_AICS_STATE_VAL
 *  @brief Audio Input Control Service State value
 */
#define BT_UUID_AICS_STATE_VAL 0x2B77
/** @def BT_UUID_AICS_STATE
 *  @brief Audio Input Control Service State
 */
#define BT_UUID_AICS_STATE \
	BT_UUID_DECLARE_16(BT_UUID_AICS_STATE_VAL)
/** @def BT_UUID_AICS_GAIN_SETTINGS_VAL
 *  @brief Audio Input Control Service Gain Settings Properties value
 */
#define BT_UUID_AICS_GAIN_SETTINGS_VAL 0x2B78
/** @def BT_UUID_AICS_GAIN_SETTINGS
 *  @brief Audio Input Control Service Gain Settings Properties
 */
#define BT_UUID_AICS_GAIN_SETTINGS \
	BT_UUID_DECLARE_16(BT_UUID_AICS_GAIN_SETTINGS_VAL)
/** @def BT_UUID_AICS_INPUT_TYPE_VAL
 *  @brief Audio Input Control Service Input Type value
 */
#define BT_UUID_AICS_INPUT_TYPE_VAL 0x2B79
/** @def BT_UUID_AICS_INPUT_TYPE
 *  @brief Audio Input Control Service Input Type
 */
#define BT_UUID_AICS_INPUT_TYPE \
	BT_UUID_DECLARE_16(BT_UUID_AICS_INPUT_TYPE_VAL)
/** @def BT_UUID_AICS_INPUT_STATUS_VAL
 *  @brief Audio Input Control Service Input Status value
 */
#define BT_UUID_AICS_INPUT_STATUS_VAL 0x2B7A
/** @def BT_UUID_AICS_INPUT_STATUS
 *  @brief Audio Input Control Service Input Status
 */
#define BT_UUID_AICS_INPUT_STATUS \
	BT_UUID_DECLARE_16(BT_UUID_AICS_INPUT_STATUS_VAL)
/** @def BT_UUID_AICS_CONTROL_VAL
 *  @brief Audio Input Control Service Control Point value
 */
#define BT_UUID_AICS_CONTROL_VAL 0x2B7B
/** @def BT_UUID_AICS_CONTROL
 *  @brief Audio Input Control Service Control Point
 */
#define BT_UUID_AICS_CONTROL \
	BT_UUID_DECLARE_16(BT_UUID_AICS_CONTROL_VAL)
/** @def BT_UUID_AICS_DESCRIPTION_VAL
 *  @brief Audio Input Control Service Input Description value
 */
#define BT_UUID_AICS_DESCRIPTION_VAL 0x2B7C
/** @def BT_UUID_AICS_DESCRIPTION
 *  @brief Audio Input Control Service Input Description
 */
#define BT_UUID_AICS_DESCRIPTION \
	BT_UUID_DECLARE_16(BT_UUID_AICS_DESCRIPTION_VAL)
/** @def BT_UUID_VCS_STATE_VAL
 *  @brief Volume Control Setting value
 */
#define BT_UUID_VCS_STATE_VAL 0x2B7D
/** @def BT_UUID_VCS_STATE
 *  @brief Volume Control Setting
 */
#define BT_UUID_VCS_STATE \
	BT_UUID_DECLARE_16(BT_UUID_VCS_STATE_VAL)
/** @def BT_UUID_VCS_CONTROL_VAL
 *  @brief Volume Control Control point value
 */
#define BT_UUID_VCS_CONTROL_VAL 0x2B7E
/** @def BT_UUID_VCS_CONTROL
 *  @brief Volume Control Control point
 */
#define BT_UUID_VCS_CONTROL \
	BT_UUID_DECLARE_16(BT_UUID_VCS_CONTROL_VAL)
/** @def BT_UUID_VCS_FLAGS_VAL
 *  @brief Volume Control Flags value
 */
#define BT_UUID_VCS_FLAGS_VAL 0x2B7F
/** @def BT_UUID_VCS_FLAGS
 *  @brief Volume Control Flags
 */
#define BT_UUID_VCS_FLAGS \
	BT_UUID_DECLARE_16(BT_UUID_VCS_FLAGS_VAL)
/** @def BT_UUID_VOCS_STATE_VAL
 *  @brief Volume Offset State value
 */
#define BT_UUID_VOCS_STATE_VAL 0x2B80
/** @def BT_UUID_VOCS_STATE
 *  @brief Volume Offset State
 */
#define BT_UUID_VOCS_STATE \
	BT_UUID_DECLARE_16(BT_UUID_VOCS_STATE_VAL)
/** @def BT_UUID_VOCS_LOCATION_VAL
 *  @brief Audio Location value
 */
#define BT_UUID_VOCS_LOCATION_VAL 0x2B81
/** @def BT_UUID_VOCS_LOCATION
 *  @brief Audio Location
 */
#define BT_UUID_VOCS_LOCATION \
	BT_UUID_DECLARE_16(BT_UUID_VOCS_LOCATION_VAL)
/** @def BT_UUID_VOCS_CONTROL_VAL
 *  @brief Volume Offset Control Point value
 */
#define BT_UUID_VOCS_CONTROL_VAL 0x2B82
/** @def BT_UUID_VOCS_CONTROL
 *  @brief Volume Offset Control Point
 */
#define BT_UUID_VOCS_CONTROL \
	BT_UUID_DECLARE_16(BT_UUID_VOCS_CONTROL_VAL)
/** @def BT_UUID_VOCS_DESCRIPTION_VAL
 *  @brief Volume Offset Audio Output Description value
 */
#define BT_UUID_VOCS_DESCRIPTION_VAL 0x2B83
/** @def BT_UUID_VOCS_DESCRIPTION
 *  @brief Volume Offset Audio Output Description
 */
#define BT_UUID_VOCS_DESCRIPTION \
	BT_UUID_DECLARE_16(BT_UUID_VOCS_DESCRIPTION_VAL)
/** @def BT_UUID_CSIS_SET_SIRK_VAL
 *  @brief Set Identity Resolving Key value
 */
#define BT_UUID_CSIS_SET_SIRK_VAL 0x2B84
/** @def BT_UUID_CSIS_SET_SIRK
 *  @brief Set Identity Resolving Key
 */
#define BT_UUID_CSIS_SET_SIRK \
	BT_UUID_DECLARE_16(BT_UUID_CSIS_SET_SIRK_VAL)
/** @def BT_UUID_CSIS_SET_SIZE_VAL
 *  @brief Set size value
 */
#define BT_UUID_CSIS_SET_SIZE_VAL 0x2B85
/** @def BT_UUID_CSIS_SET_SIZE
 *  @brief Set size
 */
#define BT_UUID_CSIS_SET_SIZE \
	BT_UUID_DECLARE_16(BT_UUID_CSIS_SET_SIZE_VAL)
/** @def BT_UUID_CSIS_SET_LOCK_VAL
 *  @brief Set lock value
 */
#define BT_UUID_CSIS_SET_LOCK_VAL 0x2B86
/** @def BT_UUID_CSIS_SET_LOCK
 *  @brief Set lock
 */
#define BT_UUID_CSIS_SET_LOCK \
	BT_UUID_DECLARE_16(BT_UUID_CSIS_SET_LOCK_VAL)
/** @def BT_UUID_CSIS_RANK_VAL
 *  @brief Rank value
 */
#define BT_UUID_CSIS_RANK_VAL 0x2B87
/** @def BT_UUID_CSIS_RANK
 *  @brief Rank
 */
#define BT_UUID_CSIS_RANK \
	BT_UUID_DECLARE_16(BT_UUID_CSIS_RANK_VAL)
/** @def BT_UUID_MCS_PLAYER_NAME_VAL
 *  @brief Media player name value
 */
#define BT_UUID_MCS_PLAYER_NAME_VAL 0x2B93
/** @def BT_UUID_MCS_PLAYER_NAME
 *  @brief Media player name
 */
#define BT_UUID_MCS_PLAYER_NAME \
	BT_UUID_DECLARE_16(BT_UUID_MCS_PLAYER_NAME_VAL)
/** @def BT_UUID_MCS_ICON_OBJ_ID_VAL
 *  @brief Media Icon Object ID value
 */
#define BT_UUID_MCS_ICON_OBJ_ID_VAL 0x2B94
/** @def BT_UUID_MCS_ICON_OBJ_ID
 *  @brief Media Icon Object ID
 */
#define BT_UUID_MCS_ICON_OBJ_ID \
	BT_UUID_DECLARE_16(BT_UUID_MCS_ICON_OBJ_ID_VAL)
/** @def BT_UUID_MCS_ICON_URL_VAL
 *  @brief Media Icon URL value
 */
#define BT_UUID_MCS_ICON_URL_VAL 0x2B95
/** @def BT_UUID_MCS_ICON_URL
 *  @brief Media Icon URL
 */
#define BT_UUID_MCS_ICON_URL \
	BT_UUID_DECLARE_16(BT_UUID_MCS_ICON_URL_VAL)
/** @def BT_UUID_MCS_TRACK_CHANGED_VAL
 *  @brief Track Changed value
 */
#define BT_UUID_MCS_TRACK_CHANGED_VAL 0x2B96
/** @def BT_UUID_MCS_TRACK_CHANGED
 *  @brief Track Changed
 */
#define BT_UUID_MCS_TRACK_CHANGED \
	BT_UUID_DECLARE_16(BT_UUID_MCS_TRACK_CHANGED_VAL)
/** @def BT_UUID_MCS_TRACK_TITLE_VAL
 *  @brief Track Title value
 */
#define BT_UUID_MCS_TRACK_TITLE_VAL 0x2B97
/** @def BT_UUID_MCS_TRACK_TITLE
 *  @brief Track Title
 */
#define BT_UUID_MCS_TRACK_TITLE \
	BT_UUID_DECLARE_16(BT_UUID_MCS_TRACK_TITLE_VAL)
/** @def BT_UUID_MCS_TRACK_DURATION_VAL
 *  @brief Track Duration value
 */
#define BT_UUID_MCS_TRACK_DURATION_VAL 0x2B98
/** @def BT_UUID_MCS_TRACK_DURATION
 *  @brief Track Duration
 */
#define BT_UUID_MCS_TRACK_DURATION \
	BT_UUID_DECLARE_16(BT_UUID_MCS_TRACK_DURATION_VAL)
/** @def BT_UUID_MCS_TRACK_POSITION_VAL
 *  @brief Track Position value
 */
#define BT_UUID_MCS_TRACK_POSITION_VAL 0x2B99
/** @def BT_UUID_MCS_TRACK_POSITION
 *  @brief Track Position
 */
#define BT_UUID_MCS_TRACK_POSITION \
	BT_UUID_DECLARE_16(BT_UUID_MCS_TRACK_POSITION_VAL)
/** @def BT_UUID_MCS_PLAYBACK_SPEED_VAL
 *  @brief Playback Speed value
 */
#define BT_UUID_MCS_PLAYBACK_SPEED_VAL 0x2B9A
/** @def BT_UUID_MCS_PLAYBACK_SPEED
 *  @brief Playback Speed
 */
#define BT_UUID_MCS_PLAYBACK_SPEED \
	BT_UUID_DECLARE_16(BT_UUID_MCS_PLAYBACK_SPEED_VAL)
/** @def BT_UUID_MCS_SEEKING_SPEED_VAL
 *  @brief Seeking Speed value
 */
#define BT_UUID_MCS_SEEKING_SPEED_VAL 0x2B9B
/** @def BT_UUID_MCS_SEEKING_SPEED
 *  @brief Seeking Speed
 */
#define BT_UUID_MCS_SEEKING_SPEED \
	BT_UUID_DECLARE_16(BT_UUID_MCS_SEEKING_SPEED_VAL)
/** @def BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID_VAL
 *  @brief Track Segments Object ID value
 */
#define BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID_VAL 0x2B9C
/** @def BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID
 *  @brief Track Segments Object ID
 */
#define BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID \
	BT_UUID_DECLARE_16(BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID_VAL)
/** @def BT_UUID_MCS_CURRENT_TRACK_OBJ_ID_VAL
 *  @brief Current Track Object ID value
 */
#define BT_UUID_MCS_CURRENT_TRACK_OBJ_ID_VAL 0x2B9D
/** @def BT_UUID_MCS_CURRENT_TRACK_OBJ_ID
 *  @brief Current Track Object ID
 */
#define BT_UUID_MCS_CURRENT_TRACK_OBJ_ID \
	BT_UUID_DECLARE_16(BT_UUID_MCS_CURRENT_TRACK_OBJ_ID_VAL)
/** @def BT_UUID_MCS_NEXT_TRACK_OBJ_ID_VAL
 *  @brief Next Track Object ID value
 */
#define BT_UUID_MCS_NEXT_TRACK_OBJ_ID_VAL 0x2B9E
/** @def BT_UUID_MCS_NEXT_TRACK_OBJ_ID
 *  @brief Next Track Object ID
 */
#define BT_UUID_MCS_NEXT_TRACK_OBJ_ID \
	BT_UUID_DECLARE_16(BT_UUID_MCS_NEXT_TRACK_OBJ_ID_VAL)
/** @def BT_UUID_MCS_PARENT_GROUP_OBJ_ID_VAL
 *  @brief Parent Group Object ID value
 */
#define BT_UUID_MCS_PARENT_GROUP_OBJ_ID_VAL 0x2B9F
/** @def BT_UUID_MCS_PARENT_GROUP_OBJ_ID
 *  @brief Parent Group Object ID
 */
#define BT_UUID_MCS_PARENT_GROUP_OBJ_ID \
	BT_UUID_DECLARE_16(BT_UUID_MCS_PARENT_GROUP_OBJ_ID_VAL)
/** @def BT_UUID_MCS_CURRENT_GROUP_OBJ_ID_VAL
 *  @brief Group Object ID value
 */
#define BT_UUID_MCS_CURRENT_GROUP_OBJ_ID_VAL 0x2BA0
/** @def BT_UUID_MCS_CURRENT_GROUP_OBJ_ID
 *  @brief Group Object ID
 */
#define BT_UUID_MCS_CURRENT_GROUP_OBJ_ID \
	BT_UUID_DECLARE_16(BT_UUID_MCS_CURRENT_GROUP_OBJ_ID_VAL)
/** @def BT_UUID_MCS_PLAYING_ORDER_VAL
 *  @brief Playing Order value
 */
#define BT_UUID_MCS_PLAYING_ORDER_VAL 0x2BA1
/** @def BT_UUID_MCS_PLAYING_ORDER
 *  @brief Playing Order
 */
#define BT_UUID_MCS_PLAYING_ORDER \
	BT_UUID_DECLARE_16(BT_UUID_MCS_PLAYING_ORDER_VAL)
/** @def BT_UUID_MCS_PLAYING_ORDERS_VAL
 *  @brief Playing Orders supported value
 */
#define BT_UUID_MCS_PLAYING_ORDERS_VAL 0x2BA2
/** @def BT_UUID_MCS_PLAYING_ORDERS
 *  @brief Playing Orders supported
 */
#define BT_UUID_MCS_PLAYING_ORDERS \
	BT_UUID_DECLARE_16(BT_UUID_MCS_PLAYING_ORDERS_VAL)
/** @def BT_UUID_MCS_MEDIA_STATE_VAL
 *  @brief Media State value
 */
#define BT_UUID_MCS_MEDIA_STATE_VAL 0x2BA3
/** @def BT_UUID_MCS_MEDIA_STATE
 *  @brief Media State
 */
#define BT_UUID_MCS_MEDIA_STATE \
	BT_UUID_DECLARE_16(BT_UUID_MCS_MEDIA_STATE_VAL)
/** @def BT_UUID_MCS_MEDIA_CONTROL_POINT_VAL
 *  @brief Media Control Point value
 */
#define BT_UUID_MCS_MEDIA_CONTROL_POINT_VAL 0x2BA4
/** @def BT_UUID_MCS_MEDIA_CONTROL_POINT
 *  @brief Media Control Point
 */
#define BT_UUID_MCS_MEDIA_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_MCS_MEDIA_CONTROL_POINT_VAL)
/** @def BT_UUID_MCS_MEDIA_CONTROL_OPCODES_VAL
 *  @brief Media control opcodes supported value
 */
#define BT_UUID_MCS_MEDIA_CONTROL_OPCODES_VAL 0x2BA5
/** @def BT_UUID_MCS_MEDIA_CONTROL_OPCODES
 *  @brief Media control opcodes supported
 */
#define BT_UUID_MCS_MEDIA_CONTROL_OPCODES \
	BT_UUID_DECLARE_16(BT_UUID_MCS_MEDIA_CONTROL_OPCODES_VAL)
/** @def BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID_VAL
 *  @brief Search result object ID value
 */
#define BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID_VAL 0x2BA6
/** @def BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID
 *  @brief Search result object ID
 */
#define BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID \
	BT_UUID_DECLARE_16(BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID_VAL)
/** @def BT_UUID_MCS_SEARCH_CONTROL_POINT_VAL
 *  @brief Search control point value
 */
#define BT_UUID_MCS_SEARCH_CONTROL_POINT_VAL 0x2BA7
/** @def BT_UUID_MCS_SEARCH_CONTROL_POINT
 *  @brief Search control point
 */
#define BT_UUID_MCS_SEARCH_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_MCS_SEARCH_CONTROL_POINT_VAL)
/** @def BT_UUID_OTS_TYPE_MPL_ICON_VAL
 *  @brief Media Player Icon Object Type value
 */
#define BT_UUID_OTS_TYPE_MPL_ICON_VAL 0x2BA9
/** @def BT_UUID_OTS_TYPE_MPL_ICON
 *  @brief Media Player Icon Object Type
 */
#define BT_UUID_OTS_TYPE_MPL_ICON \
	BT_UUID_DECLARE_16(BT_UUID_OTS_TYPE_MPL_ICON_VAL)
/** @def BT_UUID_OTS_TYPE_TRACK_SEGMENT_VAL
 *  @brief Track Segments Object Type value
 */
#define BT_UUID_OTS_TYPE_TRACK_SEGMENT_VAL 0x2BAA
/** @def BT_UUID_OTS_TYPE_TRACK_SEGMENT
 *  @brief Track Segments Object Type
 */
#define BT_UUID_OTS_TYPE_TRACK_SEGMENT \
	BT_UUID_DECLARE_16(BT_UUID_OTS_TYPE_TRACK_SEGMENT_VAL)
/** @def BT_UUID_OTS_TYPE_TRACK_VAL
 *  @brief Track Object Type value
 */
#define BT_UUID_OTS_TYPE_TRACK_VAL 0x2BAB
/** @def BT_UUID_OTS_TYPE_TRACK
 *  @brief Track Object Type
 */
#define BT_UUID_OTS_TYPE_TRACK \
	BT_UUID_DECLARE_16(BT_UUID_OTS_TYPE_TRACK_VAL)
/** @def BT_UUID_OTS_TYPE_GROUP_VAL
 *  @brief Group Object Type value
 */
#define BT_UUID_OTS_TYPE_GROUP_VAL 0x2BAC
/** @def BT_UUID_OTS_TYPE_GROUP
 *  @brief Group Object Type
 */
#define BT_UUID_OTS_TYPE_GROUP \
	BT_UUID_DECLARE_16(BT_UUID_OTS_TYPE_GROUP_VAL)
/** @def BT_UUID_TBS_PROVIDER_NAME_VAL
 *  @brief Bearer Provider Name value
 */
#define BT_UUID_TBS_PROVIDER_NAME_VAL 0x2BB3
/** @def BT_UUID_TBS_PROVIDER_NAME
 *  @brief Bearer Provider Name
 */
#define BT_UUID_TBS_PROVIDER_NAME \
	BT_UUID_DECLARE_16(BT_UUID_TBS_PROVIDER_NAME_VAL)
/** @def BT_UUID_TBS_UCI_VAL
 *  @brief Bearer UCI value
 */
#define BT_UUID_TBS_UCI_VAL 0x2BB4
/** @def BT_UUID_TBS_UCI
 *  @brief Bearer UCI
 */
#define BT_UUID_TBS_UCI \
	BT_UUID_DECLARE_16(BT_UUID_TBS_UCI_VAL)
/** @def BT_UUID_TBS_TECHNOLOGY_VAL
 *  @brief Bearer Technology value
 */
#define BT_UUID_TBS_TECHNOLOGY_VAL 0x2BB5
/** @def BT_UUID_TBS_TECHNOLOGY
 *  @brief Bearer Technology
 */
#define BT_UUID_TBS_TECHNOLOGY \
	BT_UUID_DECLARE_16(BT_UUID_TBS_TECHNOLOGY_VAL)
/** @def BT_UUID_TBS_URI_LIST_VAL
 *  @brief Bearer URI Prefixes Supported List value
 */
#define BT_UUID_TBS_URI_LIST_VAL 0x2BB6
/** @def BT_UUID_TBS_URI_LIST
 *  @brief Bearer URI Prefixes Supported List
 */
#define BT_UUID_TBS_URI_LIST \
	BT_UUID_DECLARE_16(BT_UUID_TBS_URI_LIST_VAL)
/** @def BT_UUID_TBS_SIGNAL_STRENGTH_VAL
 *  @brief Bearer Signal Strength value
 */
#define BT_UUID_TBS_SIGNAL_STRENGTH_VAL 0x2BB7
/** @def BT_UUID_TBS_SIGNAL_STRENGTH
 *  @brief Bearer Signal Strength
 */
#define BT_UUID_TBS_SIGNAL_STRENGTH \
	BT_UUID_DECLARE_16(BT_UUID_TBS_SIGNAL_STRENGTH_VAL)
/** @def BT_UUID_TBS_SIGNAL_INTERVAL_VAL
 *  @brief Bearer Signal Strength Reporting Interval value
 */
#define BT_UUID_TBS_SIGNAL_INTERVAL_VAL 0x2BB8
/** @def BT_UUID_TBS_SIGNAL_INTERVAL
 *  @brief Bearer Signal Strength Reporting Interval
 */
#define BT_UUID_TBS_SIGNAL_INTERVAL \
	BT_UUID_DECLARE_16(BT_UUID_TBS_SIGNAL_INTERVAL_VAL)
/** @def BT_UUID_TBS_LIST_CURRENT_CALLS_VAL
 *  @brief Bearer List Current Calls value
 */
#define BT_UUID_TBS_LIST_CURRENT_CALLS_VAL 0x2BB9
/** @def BT_UUID_TBS_LIST_CURRENT_CALLS
 *  @brief Bearer List Current Calls
 */
#define BT_UUID_TBS_LIST_CURRENT_CALLS \
	BT_UUID_DECLARE_16(BT_UUID_TBS_LIST_CURRENT_CALLS_VAL)
/** @def BT_UUID_CCID_VAL
 *  @brief Content Control ID value
 */
#define BT_UUID_CCID_VAL 0x2BBA
/** @def BT_UUID_CCID
 *  @brief Content Control ID
 */
#define BT_UUID_CCID \
	BT_UUID_DECLARE_16(BT_UUID_CCID_VAL)
/** @def BT_UUID_TBS_STATUS_FLAGS_VAL
 *  @brief Status flags value
 */
#define BT_UUID_TBS_STATUS_FLAGS_VAL 0x2BBB
/** @def BT_UUID_TBS_STATUS_FLAGS
 *  @brief Status flags
 */
#define BT_UUID_TBS_STATUS_FLAGS \
	BT_UUID_DECLARE_16(BT_UUID_TBS_STATUS_FLAGS_VAL)
/** @def BT_UUID_TBS_INCOMING_URI_VAL
 *  @brief Incoming Call Target Caller ID value
 */
#define BT_UUID_TBS_INCOMING_URI_VAL 0x2BBC
/** @def BT_UUID_TBS_INCOMING_URI
 *  @brief Incoming Call Target Caller ID
 */
#define BT_UUID_TBS_INCOMING_URI \
	BT_UUID_DECLARE_16(BT_UUID_TBS_INCOMING_URI_VAL)
/** @def BT_UUID_TBS_CALL_STATE_VAL
 *  @brief Call State value
 */
#define BT_UUID_TBS_CALL_STATE_VAL 0x2BBD
/** @def BT_UUID_TBS_CALL_STATE
 *  @brief Call State
 */
#define BT_UUID_TBS_CALL_STATE \
	BT_UUID_DECLARE_16(BT_UUID_TBS_CALL_STATE_VAL)
/** @def BT_UUID_TBS_CALL_CONTROL_POINT_VAL
 *  @brief Call Control Point value
 */
#define BT_UUID_TBS_CALL_CONTROL_POINT_VAL 0x2BBE
/** @def BT_UUID_TBS_CALL_CONTROL_POINT
 *  @brief Call Control Point
 */
#define BT_UUID_TBS_CALL_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_TBS_CALL_CONTROL_POINT_VAL)
/** @def BT_UUID_TBS_OPTIONAL_OPCODES_VAL
 *  @brief Optional Opcodes value
 */
#define BT_UUID_TBS_OPTIONAL_OPCODES_VAL 0x2BBF
/** @def BT_UUID_TBS_OPTIONAL_OPCODES
 *  @brief Optional Opcodes
 */
#define BT_UUID_TBS_OPTIONAL_OPCODES \
	BT_UUID_DECLARE_16(BT_UUID_TBS_OPTIONAL_OPCODES_VAL)
/** BT_UUID_TBS_TERMINATE_REASON_VAL
 *  @brief Terminate reason value
 */
#define BT_UUID_TBS_TERMINATE_REASON_VAL 0x2BC0
/** BT_UUID_TBS_TERMINATE_REASON
 *  @brief Terminate reason
 */
#define BT_UUID_TBS_TERMINATE_REASON \
	BT_UUID_DECLARE_16(BT_UUID_TBS_TERMINATE_REASON_VAL)
/** @def BT_UUID_TBS_INCOMING_CALL_VAL
 *  @brief Incoming Call value
 */
#define BT_UUID_TBS_INCOMING_CALL_VAL 0x2BC1
/** @def BT_UUID_TBS_INCOMING_CALL
 *  @brief Incoming Call
 */
#define BT_UUID_TBS_INCOMING_CALL \
	BT_UUID_DECLARE_16(BT_UUID_TBS_INCOMING_CALL_VAL)
/** @def BT_UUID_TBS_FRIENDLY_NAME_VAL
 *  @brief Incoming Call Friendly name value
 */
#define BT_UUID_TBS_FRIENDLY_NAME_VAL 0x2BC2
/** @def BT_UUID_TBS_FRIENDLY_NAME
 *  @brief Incoming Call Friendly name
 */
#define BT_UUID_TBS_FRIENDLY_NAME \
	BT_UUID_DECLARE_16(BT_UUID_TBS_FRIENDLY_NAME_VAL)
/** @def BT_UUID_MICS_MUTE_VAL
 *  @brief Microphone Input Control Service Mute value
 */
#define BT_UUID_MICS_MUTE_VAL 0x2BC3
/** @def BT_UUID_MICS_MUTE
 *  @brief Microphone Input Control Service Mute
 */
#define BT_UUID_MICS_MUTE \
	BT_UUID_DECLARE_16(BT_UUID_MICS_MUTE_VAL)
/** @def BT_UUID_ASCS_ASE_SNK_VAL
 *  @brief Audio Stream Endpoint Sink Characteristic value
 */
#define BT_UUID_ASCS_ASE_SNK_VAL 0x2BC4
/** @def BT_UUID_ASCS_ASE_SNK
 *  @brief Audio Stream Endpoint Sink Characteristic
 */
#define BT_UUID_ASCS_ASE_SNK \
	BT_UUID_DECLARE_16(BT_UUID_ASCS_ASE_SNK_VAL)
/** @def BT_UUID_ASCS_ASE_SRC_VAL
 *  @brief Audio Stream Endpoint Source Characteristic value
 */
#define BT_UUID_ASCS_ASE_SRC_VAL 0x2BC5
/** @def BT_UUID_ASCS_ASE_SNK
 *  @brief Audio Stream Endpoint Source Characteristic
 */
#define BT_UUID_ASCS_ASE_SRC \
	BT_UUID_DECLARE_16(BT_UUID_ASCS_ASE_SRC_VAL)
/** @def BT_UUID_ASCS_ASE_CP_VAL
 *  @brief Audio Stream Endpoint Control Point Characteristic value
 */
#define BT_UUID_ASCS_ASE_CP_VAL 0x2BC6
/** @def BT_UUID_ASCS_ASE_CP
 *  @brief Audio Stream Endpoint Control Point Characteristic
 */
#define BT_UUID_ASCS_ASE_CP \
	BT_UUID_DECLARE_16(BT_UUID_ASCS_ASE_CP_VAL)
/** @def BT_UUID_BASS_CONTROL_POINT_VAL
 *  @brief Broadcast Audio Scan Service Scan State value
 */
#define BT_UUID_BASS_CONTROL_POINT_VAL 0x2BC7
/** @def BT_UUID_BASS_CONTROL_POINT
 *  @brief Broadcast Audio Scan Service Scan State
 */
#define BT_UUID_BASS_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_BASS_CONTROL_POINT_VAL)
/** @def BT_UUID_BASS_RECV_STATE_VAL
 *  @brief Broadcast Audio Scan Service Receive State value
 */
#define BT_UUID_BASS_RECV_STATE_VAL 0x2BC8
/** @def BT_UUID_BASS_RECV_STATE
 *  @brief Broadcast Audio Scan Service Receive State
 */
#define BT_UUID_BASS_RECV_STATE \
	BT_UUID_DECLARE_16(BT_UUID_BASS_RECV_STATE_VAL)
/** @def BT_UUID_PACS_SNK_VAL
 *  @brief Sink PAC Characteristic value
 */
#define BT_UUID_PACS_SNK_VAL 0x2BC9
/** @def BT_UUID_PACS_SNK
 *  @brief Sink PAC Characteristic
 */
#define BT_UUID_PACS_SNK \
	BT_UUID_DECLARE_16(BT_UUID_PACS_SNK_VAL)
/** @def BT_UUID_PACS_SNK_LOC_VAL
 *  @brief Sink PAC Locations Characteristic value
 */
#define BT_UUID_PACS_SNK_LOC_VAL 0x2BCA
/** @def BT_UUID_PACS_SNK_LOC
 *  @brief Sink PAC Locations Characteristic
 */
#define BT_UUID_PACS_SNK_LOC \
	BT_UUID_DECLARE_16(BT_UUID_PACS_SNK_LOC_VAL)
/** @def BT_UUID_PACS_SRC_VAL
 *  @brief Source PAC Characteristic value
 */
#define BT_UUID_PACS_SRC_VAL 0x2BCB
/** @def BT_UUID_PACS_SRC
 *  @brief Source PAC Characteristic
 */
#define BT_UUID_PACS_SRC \
	BT_UUID_DECLARE_16(BT_UUID_PACS_SRC_VAL)
/** @def BT_UUID_PACS_SRC_LOC_VAL
 *  @brief Source PAC Locations Characteristic value
 */
#define BT_UUID_PACS_SRC_LOC_VAL 0x2BCC
/** @def BT_UUID_PACS_SRC_LOC
 *  @brief Source PAC Locations Characteristic
 */
#define BT_UUID_PACS_SRC_LOC \
	BT_UUID_DECLARE_16(BT_UUID_PACS_SRC_LOC_VAL)
/** @def BT_UUID_PACS_AVAILABLE_CONTEXT_VAL
 *  @brief Available Audio Contexts Characteristic value
 */
#define BT_UUID_PACS_AVAILABLE_CONTEXT_VAL 0x2BCD
/** @def BT_UUID_PACS_AVAILABLE_CONTEXT
 *  @brief Available Audio Contexts Characteristic
 */
#define BT_UUID_PACS_AVAILABLE_CONTEXT \
	BT_UUID_DECLARE_16(BT_UUID_PACS_AVAILABLE_CONTEXT_VAL)
/** @def BT_UUID_PACS_SUPPORTED_CONTEXT_VAL
 *  @brief Supported Audio Context Characteristic value
 */
#define BT_UUID_PACS_SUPPORTED_CONTEXT_VAL 0x2BCE
/** @def BT_UUID_PACS_SUPPORTED_CONTEXT
 *  @brief Supported Audio Context Characteristic
 */
#define BT_UUID_PACS_SUPPORTED_CONTEXT \
	BT_UUID_DECLARE_16(BT_UUID_PACS_SUPPORTED_CONTEXT_VAL)
/** @def BT_UUID_HAS_HEARING_AID_FEATURES_VAL
 *  @brief Hearing Aid Features Characteristic value
 */
#define BT_UUID_HAS_HEARING_AID_FEATURES_VAL 0x2BDA
/** @def BT_UUID_HAS_HEARING_AID_FEATURES
 *  @brief Hearing Aid Features Characteristic
 */
#define BT_UUID_HAS_HEARING_AID_FEATURES \
	BT_UUID_DECLARE_16(BT_UUID_HAS_HEARING_AID_FEATURES_VAL)
/** @def BT_UUID_HAS_PRESET_CONTROL_POINT_VAL
 *  @brief Hearing Aid Preset Control Point Characteristic value
 */
#define BT_UUID_HAS_PRESET_CONTROL_POINT_VAL 0x2BDB
/** @def BT_UUID_HAS_PRESET_CONTROL_POINT
 *  @brief Hearing Aid Preset Control Point Characteristic
 */
#define BT_UUID_HAS_PRESET_CONTROL_POINT \
	BT_UUID_DECLARE_16(BT_UUID_HAS_PRESET_CONTROL_POINT_VAL)
/** @def BT_UUID_HAS_ACTIVE_PRESET_INDEX_VAL
 *  @brief Active Preset Index Characteristic value
 */
#define BT_UUID_HAS_ACTIVE_PRESET_INDEX_VAL 0x2BDC
/** @def BT_UUID_HAS_ACTIVE_PRESET_INDEX
 *  @brief Active Preset Index Characteristic
 */
#define BT_UUID_HAS_ACTIVE_PRESET_INDEX \
	BT_UUID_DECLARE_16(BT_UUID_HAS_ACTIVE_PRESET_INDEX_VAL)
/*
 * Protocol UUIDs
 */
#define BT_UUID_SDP_VAL               0x0001
#define BT_UUID_SDP                   BT_UUID_DECLARE_16(BT_UUID_SDP_VAL)
#define BT_UUID_UDP_VAL               0x0002
#define BT_UUID_UDP                   BT_UUID_DECLARE_16(BT_UUID_UDP_VAL)
#define BT_UUID_RFCOMM_VAL            0x0003
#define BT_UUID_RFCOMM                BT_UUID_DECLARE_16(BT_UUID_RFCOMM_VAL)
#define BT_UUID_TCP_VAL               0x0004
#define BT_UUID_TCP                   BT_UUID_DECLARE_16(BT_UUID_TCP_VAL)
#define BT_UUID_TCS_BIN_VAL           0x0005
#define BT_UUID_TCS_BIN               BT_UUID_DECLARE_16(BT_UUID_TCS_BIN_VAL)
#define BT_UUID_TCS_AT_VAL            0x0006
#define BT_UUID_TCS_AT                BT_UUID_DECLARE_16(BT_UUID_TCS_AT_VAL)
#define BT_UUID_ATT_VAL               0x0007
#define BT_UUID_ATT                   BT_UUID_DECLARE_16(BT_UUID_ATT_VAL)
#define BT_UUID_OBEX_VAL              0x0008
#define BT_UUID_OBEX                  BT_UUID_DECLARE_16(BT_UUID_OBEX_VAL)
#define BT_UUID_IP_VAL                0x0009
#define BT_UUID_IP                    BT_UUID_DECLARE_16(BT_UUID_IP_VAL)
#define BT_UUID_FTP_VAL               0x000a
#define BT_UUID_FTP                   BT_UUID_DECLARE_16(BT_UUID_FTP_VAL)
#define BT_UUID_HTTP_VAL              0x000c
#define BT_UUID_HTTP                  BT_UUID_DECLARE_16(BT_UUID_HTTP_VAL)
#define BT_UUID_BNEP_VAL              0x000f
#define BT_UUID_BNEP                  BT_UUID_DECLARE_16(BT_UUID_BNEP_VAL)
#define BT_UUID_UPNP_VAL              0x0010
#define BT_UUID_UPNP                  BT_UUID_DECLARE_16(BT_UUID_UPNP_VAL)
#define BT_UUID_HIDP_VAL              0x0011
#define BT_UUID_HIDP                  BT_UUID_DECLARE_16(BT_UUID_HIDP_VAL)
#define BT_UUID_HCRP_CTRL_VAL         0x0012
#define BT_UUID_HCRP_CTRL             BT_UUID_DECLARE_16(BT_UUID_HCRP_CTRL_VAL)
#define BT_UUID_HCRP_DATA_VAL         0x0014
#define BT_UUID_HCRP_DATA             BT_UUID_DECLARE_16(BT_UUID_HCRP_DATA_VAL)
#define BT_UUID_HCRP_NOTE_VAL         0x0016
#define BT_UUID_HCRP_NOTE             BT_UUID_DECLARE_16(BT_UUID_HCRP_NOTE_VAL)
#define BT_UUID_AVCTP_VAL             0x0017
#define BT_UUID_AVCTP                 BT_UUID_DECLARE_16(BT_UUID_AVCTP_VAL)
#define BT_UUID_AVDTP_VAL             0x0019
#define BT_UUID_AVDTP                 BT_UUID_DECLARE_16(BT_UUID_AVDTP_VAL)
#define BT_UUID_CMTP_VAL              0x001b
#define BT_UUID_CMTP                  BT_UUID_DECLARE_16(BT_UUID_CMTP_VAL)
#define BT_UUID_UDI_VAL               0x001d
#define BT_UUID_UDI                   BT_UUID_DECLARE_16(BT_UUID_UDI_VAL)
#define BT_UUID_MCAP_CTRL_VAL         0x001e
#define BT_UUID_MCAP_CTRL             BT_UUID_DECLARE_16(BT_UUID_MCAP_CTRL_VAL)
#define BT_UUID_MCAP_DATA_VAL         0x001f
#define BT_UUID_MCAP_DATA             BT_UUID_DECLARE_16(BT_UUID_MCAP_DATA_VAL)
#define BT_UUID_L2CAP_VAL             0x0100
#define BT_UUID_L2CAP                 BT_UUID_DECLARE_16(BT_UUID_L2CAP_VAL)


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
bool bt_uuid_create(struct bt_uuid *uuid, const uint8_t *data, uint8_t data_len);

/** @brief Convert Bluetooth UUID to string.
 *
 *  Converts Bluetooth UUID to string.
 *  UUID can be in any format, 16-bit, 32-bit or 128-bit.
 *
 *  @param uuid Bluetooth UUID
 *  @param str pointer where to put converted string
 *  @param len length of str
 */
void bt_uuid_to_str(const struct bt_uuid *uuid, char *str, size_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_UUID_H_ */
