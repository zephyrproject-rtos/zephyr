/** @file
 *  @brief Service Discovery Protocol handling.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SDP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SDP_H_

/**
 * @brief Service Discovery Protocol (SDP)
 * @defgroup bt_sdp Service Discovery Protocol (SDP)
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * All definitions are based on Bluetooth Assigned Numbers
 * of the Bluetooth Specification
 */

/*
 * Service class identifiers of standard services and service groups
 */
#define BT_SDP_SDP_SERVER_SVCLASS           0x1000
#define BT_SDP_BROWSE_GRP_DESC_SVCLASS      0x1001
#define BT_SDP_PUBLIC_BROWSE_GROUP          0x1002
#define BT_SDP_SERIAL_PORT_SVCLASS          0x1101
#define BT_SDP_LAN_ACCESS_SVCLASS           0x1102
#define BT_SDP_DIALUP_NET_SVCLASS           0x1103
#define BT_SDP_IRMC_SYNC_SVCLASS            0x1104
#define BT_SDP_OBEX_OBJPUSH_SVCLASS         0x1105
#define BT_SDP_OBEX_FILETRANS_SVCLASS       0x1106
#define BT_SDP_IRMC_SYNC_CMD_SVCLASS        0x1107
#define BT_SDP_HEADSET_SVCLASS              0x1108
#define BT_SDP_CORDLESS_TELEPHONY_SVCLASS   0x1109
#define BT_SDP_AUDIO_SOURCE_SVCLASS         0x110a
#define BT_SDP_AUDIO_SINK_SVCLASS           0x110b
#define BT_SDP_AV_REMOTE_TARGET_SVCLASS     0x110c
#define BT_SDP_ADVANCED_AUDIO_SVCLASS       0x110d
#define BT_SDP_AV_REMOTE_SVCLASS            0x110e
#define BT_SDP_AV_REMOTE_CONTROLLER_SVCLASS 0x110f
#define BT_SDP_INTERCOM_SVCLASS             0x1110
#define BT_SDP_FAX_SVCLASS                  0x1111
#define BT_SDP_HEADSET_AGW_SVCLASS          0x1112
#define BT_SDP_WAP_SVCLASS                  0x1113
#define BT_SDP_WAP_CLIENT_SVCLASS           0x1114
#define BT_SDP_PANU_SVCLASS                 0x1115
#define BT_SDP_NAP_SVCLASS                  0x1116
#define BT_SDP_GN_SVCLASS                   0x1117
#define BT_SDP_DIRECT_PRINTING_SVCLASS      0x1118
#define BT_SDP_REFERENCE_PRINTING_SVCLASS   0x1119
#define BT_SDP_IMAGING_SVCLASS              0x111a
#define BT_SDP_IMAGING_RESPONDER_SVCLASS    0x111b
#define BT_SDP_IMAGING_ARCHIVE_SVCLASS      0x111c
#define BT_SDP_IMAGING_REFOBJS_SVCLASS      0x111d
#define BT_SDP_HANDSFREE_SVCLASS            0x111e
#define BT_SDP_HANDSFREE_AGW_SVCLASS        0x111f
#define BT_SDP_DIRECT_PRT_REFOBJS_SVCLASS   0x1120
#define BT_SDP_REFLECTED_UI_SVCLASS         0x1121
#define BT_SDP_BASIC_PRINTING_SVCLASS       0x1122
#define BT_SDP_PRINTING_STATUS_SVCLASS      0x1123
#define BT_SDP_HID_SVCLASS                  0x1124
#define BT_SDP_HCR_SVCLASS                  0x1125
#define BT_SDP_HCR_PRINT_SVCLASS            0x1126
#define BT_SDP_HCR_SCAN_SVCLASS             0x1127
#define BT_SDP_CIP_SVCLASS                  0x1128
#define BT_SDP_VIDEO_CONF_GW_SVCLASS        0x1129
#define BT_SDP_UDI_MT_SVCLASS               0x112a
#define BT_SDP_UDI_TA_SVCLASS               0x112b
#define BT_SDP_AV_SVCLASS                   0x112c
#define BT_SDP_SAP_SVCLASS                  0x112d
#define BT_SDP_PBAP_PCE_SVCLASS             0x112e
#define BT_SDP_PBAP_PSE_SVCLASS             0x112f
#define BT_SDP_PBAP_SVCLASS                 0x1130
#define BT_SDP_MAP_MSE_SVCLASS              0x1132
#define BT_SDP_MAP_MCE_SVCLASS              0x1133
#define BT_SDP_MAP_SVCLASS                  0x1134
#define BT_SDP_GNSS_SVCLASS                 0x1135
#define BT_SDP_GNSS_SERVER_SVCLASS          0x1136
#define BT_SDP_MPS_SC_SVCLASS               0x113a
#define BT_SDP_MPS_SVCLASS                  0x113b
#define BT_SDP_PNP_INFO_SVCLASS             0x1200
#define BT_SDP_GENERIC_NETWORKING_SVCLASS   0x1201
#define BT_SDP_GENERIC_FILETRANS_SVCLASS    0x1202
#define BT_SDP_GENERIC_AUDIO_SVCLASS        0x1203
#define BT_SDP_GENERIC_TELEPHONY_SVCLASS    0x1204
#define BT_SDP_UPNP_SVCLASS                 0x1205
#define BT_SDP_UPNP_IP_SVCLASS              0x1206
#define BT_SDP_UPNP_PAN_SVCLASS             0x1300
#define BT_SDP_UPNP_LAP_SVCLASS             0x1301
#define BT_SDP_UPNP_L2CAP_SVCLASS           0x1302
#define BT_SDP_VIDEO_SOURCE_SVCLASS         0x1303
#define BT_SDP_VIDEO_SINK_SVCLASS           0x1304
#define BT_SDP_VIDEO_DISTRIBUTION_SVCLASS   0x1305
#define BT_SDP_HDP_SVCLASS                  0x1400
#define BT_SDP_HDP_SOURCE_SVCLASS           0x1401
#define BT_SDP_HDP_SINK_SVCLASS             0x1402
#define BT_SDP_GENERIC_ACCESS_SVCLASS       0x1800
#define BT_SDP_GENERIC_ATTRIB_SVCLASS       0x1801
#define BT_SDP_APPLE_AGENT_SVCLASS          0x2112

/*
 * Attribute identifier codes
 */
#define BT_SDP_SERVER_RECORD_HANDLE 0x0000

/*
 * Possible values for attribute-id are listed below.
 * See SDP Spec, section "Service Attribute Definitions" for more details.
 */
#define BT_SDP_ATTR_RECORD_HANDLE               0x0000
#define BT_SDP_ATTR_SVCLASS_ID_LIST             0x0001
#define BT_SDP_ATTR_RECORD_STATE                0x0002
#define BT_SDP_ATTR_SERVICE_ID                  0x0003
#define BT_SDP_ATTR_PROTO_DESC_LIST             0x0004
#define BT_SDP_ATTR_BROWSE_GRP_LIST             0x0005
#define BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST      0x0006
#define BT_SDP_ATTR_SVCINFO_TTL                 0x0007
#define BT_SDP_ATTR_SERVICE_AVAILABILITY        0x0008
#define BT_SDP_ATTR_PROFILE_DESC_LIST           0x0009
#define BT_SDP_ATTR_DOC_URL                     0x000a
#define BT_SDP_ATTR_CLNT_EXEC_URL               0x000b
#define BT_SDP_ATTR_ICON_URL                    0x000c
#define BT_SDP_ATTR_ADD_PROTO_DESC_LIST         0x000d

#define BT_SDP_ATTR_GROUP_ID                    0x0200
#define BT_SDP_ATTR_IP_SUBNET                   0x0200
#define BT_SDP_ATTR_VERSION_NUM_LIST            0x0200
#define BT_SDP_ATTR_SUPPORTED_FEATURES_LIST     0x0200
#define BT_SDP_ATTR_GOEP_L2CAP_PSM              0x0200
#define BT_SDP_ATTR_SVCDB_STATE                 0x0201

#define BT_SDP_ATTR_MPSD_SCENARIOS              0x0200
#define BT_SDP_ATTR_MPMD_SCENARIOS              0x0201
#define BT_SDP_ATTR_MPS_DEPENDENCIES            0x0202

#define BT_SDP_ATTR_SERVICE_VERSION             0x0300
#define BT_SDP_ATTR_EXTERNAL_NETWORK            0x0301
#define BT_SDP_ATTR_SUPPORTED_DATA_STORES_LIST  0x0301
#define BT_SDP_ATTR_DATA_EXCHANGE_SPEC          0x0301
#define BT_SDP_ATTR_NETWORK                     0x0301
#define BT_SDP_ATTR_FAX_CLASS1_SUPPORT          0x0302
#define BT_SDP_ATTR_REMOTE_AUDIO_VOLUME_CONTROL 0x0302
#define BT_SDP_ATTR_MCAP_SUPPORTED_PROCEDURES   0x0302
#define BT_SDP_ATTR_FAX_CLASS20_SUPPORT         0x0303
#define BT_SDP_ATTR_SUPPORTED_FORMATS_LIST      0x0303
#define BT_SDP_ATTR_FAX_CLASS2_SUPPORT          0x0304
#define BT_SDP_ATTR_AUDIO_FEEDBACK_SUPPORT      0x0305
#define BT_SDP_ATTR_NETWORK_ADDRESS             0x0306
#define BT_SDP_ATTR_WAP_GATEWAY                 0x0307
#define BT_SDP_ATTR_HOMEPAGE_URL                0x0308
#define BT_SDP_ATTR_WAP_STACK_TYPE              0x0309
#define BT_SDP_ATTR_SECURITY_DESC               0x030a
#define BT_SDP_ATTR_NET_ACCESS_TYPE             0x030b
#define BT_SDP_ATTR_MAX_NET_ACCESSRATE          0x030c
#define BT_SDP_ATTR_IP4_SUBNET                  0x030d
#define BT_SDP_ATTR_IP6_SUBNET                  0x030e
#define BT_SDP_ATTR_SUPPORTED_CAPABILITIES      0x0310
#define BT_SDP_ATTR_SUPPORTED_FEATURES          0x0311
#define BT_SDP_ATTR_SUPPORTED_FUNCTIONS         0x0312
#define BT_SDP_ATTR_TOTAL_IMAGING_DATA_CAPACITY 0x0313
#define BT_SDP_ATTR_SUPPORTED_REPOSITORIES      0x0314
#define BT_SDP_ATTR_MAS_INSTANCE_ID             0x0315
#define BT_SDP_ATTR_SUPPORTED_MESSAGE_TYPES     0x0316
#define BT_SDP_ATTR_PBAP_SUPPORTED_FEATURES     0x0317
#define BT_SDP_ATTR_MAP_SUPPORTED_FEATURES      0x0317

#define BT_SDP_ATTR_SPECIFICATION_ID            0x0200
#define BT_SDP_ATTR_VENDOR_ID                   0x0201
#define BT_SDP_ATTR_PRODUCT_ID                  0x0202
#define BT_SDP_ATTR_VERSION                     0x0203
#define BT_SDP_ATTR_PRIMARY_RECORD              0x0204
#define BT_SDP_ATTR_VENDOR_ID_SOURCE            0x0205

#define BT_SDP_ATTR_HID_DEVICE_RELEASE_NUMBER   0x0200
#define BT_SDP_ATTR_HID_PARSER_VERSION          0x0201
#define BT_SDP_ATTR_HID_DEVICE_SUBCLASS         0x0202
#define BT_SDP_ATTR_HID_COUNTRY_CODE            0x0203
#define BT_SDP_ATTR_HID_VIRTUAL_CABLE           0x0204
#define BT_SDP_ATTR_HID_RECONNECT_INITIATE      0x0205
#define BT_SDP_ATTR_HID_DESCRIPTOR_LIST         0x0206
#define BT_SDP_ATTR_HID_LANG_ID_BASE_LIST       0x0207
#define BT_SDP_ATTR_HID_SDP_DISABLE             0x0208
#define BT_SDP_ATTR_HID_BATTERY_POWER           0x0209
#define BT_SDP_ATTR_HID_REMOTE_WAKEUP           0x020a
#define BT_SDP_ATTR_HID_PROFILE_VERSION         0x020b
#define BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT     0x020c
#define BT_SDP_ATTR_HID_NORMALLY_CONNECTABLE    0x020d
#define BT_SDP_ATTR_HID_BOOT_DEVICE             0x020e

/*
 * These identifiers are based on the SDP spec stating that
 * "base attribute id of the primary (universal) language must be 0x0100"
 *
 * Other languages should have their own offset; e.g.:
 * #define XXXLangBase yyyy
 * #define AttrServiceName_XXX 0x0000+XXXLangBase
 */
#define BT_SDP_PRIMARY_LANG_BASE  0x0100

#define BT_SDP_ATTR_SVCNAME_PRIMARY (0x0000 + BT_SDP_PRIMARY_LANG_BASE)
#define BT_SDP_ATTR_SVCDESC_PRIMARY (0x0001 + BT_SDP_PRIMARY_LANG_BASE)
#define BT_SDP_ATTR_PROVNAME_PRIMARY (0x0002 + BT_SDP_PRIMARY_LANG_BASE)

/*
 * The Data representation in SDP PDUs (pps 339, 340 of BT SDP Spec)
 * These are the exact data type+size descriptor values
 * that go into the PDU buffer.
 *
 * The datatype (leading 5bits) + size descriptor (last 3 bits)
 * is 8 bits. The size descriptor is critical to extract the
 * right number of bytes for the data value from the PDU.
 *
 * For most basic types, the datatype+size descriptor is
 * straightforward. However for constructed types and strings,
 * the size of the data is in the next "n" bytes following the
 * 8 bits (datatype+size) descriptor. Exactly what the "n" is
 * specified in the 3 bits of the data size descriptor.
 *
 * TextString and URLString can be of size 2^{8, 16, 32} bytes
 * DataSequence and DataSequenceAlternates can be of size 2^{8, 16, 32}
 * The size are computed post-facto in the API and are not known apriori
 */
#define BT_SDP_DATA_NIL        0x00
#define BT_SDP_UINT8           0x08
#define BT_SDP_UINT16          0x09
#define BT_SDP_UINT32          0x0a
#define BT_SDP_UINT64          0x0b
#define BT_SDP_UINT128         0x0c
#define BT_SDP_INT8            0x10
#define BT_SDP_INT16           0x11
#define BT_SDP_INT32           0x12
#define BT_SDP_INT64           0x13
#define BT_SDP_INT128          0x14
#define BT_SDP_UUID_UNSPEC     0x18
#define BT_SDP_UUID16          0x19
#define BT_SDP_UUID32          0x1a
#define BT_SDP_UUID128         0x1c
#define BT_SDP_TEXT_STR_UNSPEC 0x20
#define BT_SDP_TEXT_STR8       0x25
#define BT_SDP_TEXT_STR16      0x26
#define BT_SDP_TEXT_STR32      0x27
#define BT_SDP_BOOL            0x28
#define BT_SDP_SEQ_UNSPEC      0x30
#define BT_SDP_SEQ8            0x35
#define BT_SDP_SEQ16           0x36
#define BT_SDP_SEQ32           0x37
#define BT_SDP_ALT_UNSPEC      0x38
#define BT_SDP_ALT8            0x3d
#define BT_SDP_ALT16           0x3e
#define BT_SDP_ALT32           0x3f
#define BT_SDP_URL_STR_UNSPEC  0x40
#define BT_SDP_URL_STR8        0x45
#define BT_SDP_URL_STR16       0x46
#define BT_SDP_URL_STR32       0x47

#define BT_SDP_TYPE_DESC_MASK 0xf8
#define BT_SDP_SIZE_DESC_MASK 0x07
#define BT_SDP_SIZE_INDEX_OFFSET 5

/** @brief SDP Generic Data Element Value. */
struct bt_sdp_data_elem {
	uint8_t        type;
	uint32_t       data_size;
	uint32_t       total_size;
	const void *data;
};

/** @brief SDP Attribute Value. */
struct bt_sdp_attribute {
	uint16_t                id;  /* Attribute ID */
	struct bt_sdp_data_elem val; /* Attribute data */
};

/** @brief SDP Service Record Value. */
struct bt_sdp_record {
	uint32_t                    handle;     /* Redundant, for quick ref */
	struct bt_sdp_attribute *attrs;      /* Base addr of attr array */
	size_t                   attr_count; /* Number of attributes */
	uint8_t                     index;      /* Index of the record in LL */
	struct bt_sdp_record    *next;
};

/*
 * ---------------------------------------------------    ------------------
 * | Service Hdl | Attr list ptr | Attr count | Next | -> | Service Hdl | ...
 * ---------------------------------------------------    ------------------
 */

/** @def BT_SDP_ARRAY_8
 *  @brief Declare an array of 8-bit elements in an attribute.
 */
#define BT_SDP_ARRAY_8(...) ((uint8_t[]) {__VA_ARGS__})

/** @def BT_SDP_ARRAY_16
 *  @brief Declare an array of 16-bit elements in an attribute.
 */
#define BT_SDP_ARRAY_16(...) ((uint16_t[]) {__VA_ARGS__})

/** @def BT_SDP_ARRAY_32
 *  @brief Declare an array of 32-bit elements in an attribute.
 */
#define BT_SDP_ARRAY_32(...) ((uint32_t[]) {__VA_ARGS__})

/** @def BT_SDP_TYPE_SIZE
 *  @brief Declare a fixed-size data element header.
 *
 *  @param _type Data element header containing type and size descriptors.
 */
#define BT_SDP_TYPE_SIZE(_type) .type = _type, \
			.data_size = BIT(_type & BT_SDP_SIZE_DESC_MASK), \
			.total_size = BIT(_type & BT_SDP_SIZE_DESC_MASK) + 1

/** @def BT_SDP_TYPE_SIZE_VAR
 *  @brief Declare a variable-size data element header.
 *
 *  @param _type Data element header containing type and size descriptors.
 *  @param _size The actual size of the data.
 */
#define BT_SDP_TYPE_SIZE_VAR(_type, _size) .type = _type, \
			.data_size = _size, \
			.total_size = BIT((_type & BT_SDP_SIZE_DESC_MASK) - \
					  BT_SDP_SIZE_INDEX_OFFSET) + _size + 1

/** @def BT_SDP_DATA_ELEM_LIST
 *  @brief Declare a list of data elements.
 */
#define BT_SDP_DATA_ELEM_LIST(...) ((struct bt_sdp_data_elem[]) {__VA_ARGS__})


/** @def BT_SDP_NEW_SERVICE
 *  @brief SDP New Service Record Declaration Macro.
 *
 *  Helper macro to declare a new service record.
 *  Default attributes: Record Handle, Record State,
 *  Language Base, Root Browse Group
 *
 */
#define BT_SDP_NEW_SERVICE \
{ \
	BT_SDP_ATTR_RECORD_HANDLE, \
	{ BT_SDP_TYPE_SIZE(BT_SDP_UINT32), BT_SDP_ARRAY_32(0) } \
}, \
{ \
	BT_SDP_ATTR_RECORD_STATE, \
	{ BT_SDP_TYPE_SIZE(BT_SDP_UINT32), BT_SDP_ARRAY_32(0) } \
}, \
{ \
	BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST, \
	{ BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 9), \
	  BT_SDP_DATA_ELEM_LIST( \
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_8('n', 'e') }, \
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(106) }, \
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
			BT_SDP_ARRAY_16(BT_SDP_PRIMARY_LANG_BASE) } \
	  ), \
	} \
}, \
{ \
	BT_SDP_ATTR_BROWSE_GRP_LIST, \
	{ BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
	  BT_SDP_DATA_ELEM_LIST( \
		{ BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_PUBLIC_BROWSE_GROUP) }, \
	  ), \
	} \
}


/** @def BT_SDP_LIST
 *  @brief Generic SDP List Attribute Declaration Macro.
 *
 *  Helper macro to declare a list attribute.
 *
 *  @param _att_id List Attribute ID.
 *  @param _data_elem_seq Data element sequence for the list.
 *  @param _type_size SDP type and size descriptor.
 */
#define BT_SDP_LIST(_att_id, _type_size, _data_elem_seq) \
{ \
	_att_id, { _type_size, _data_elem_seq } \
}

/** @def BT_SDP_SERVICE_ID
 *  @brief SDP Service ID Attribute Declaration Macro.
 *
 *  Helper macro to declare a service ID attribute.
 *
 *  @param _uuid Service ID 16bit UUID.
 */
#define BT_SDP_SERVICE_ID(_uuid) \
{ \
	BT_SDP_ATTR_SERVICE_ID, \
	{ BT_SDP_TYPE_SIZE(BT_SDP_UUID16), &((struct bt_uuid_16) _uuid) } \
}

/** @def BT_SDP_SERVICE_NAME
 *  @brief SDP Name Attribute Declaration Macro.
 *
 *  Helper macro to declare a service name attribute.
 *
 *  @param _name Service name as a string (up to 256 chars).
 */
#define BT_SDP_SERVICE_NAME(_name) \
{ \
	BT_SDP_ATTR_SVCNAME_PRIMARY, \
	{ BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR8, (sizeof(_name)-1)), _name } \
}

/** @def BT_SDP_SUPPORTED_FEATURES
 *  @brief SDP Supported Features Attribute Declaration Macro.
 *
 *  Helper macro to declare supported features of a profile/protocol.
 *
 *  @param _features Feature mask as 16bit unsigned integer.
 */
#define BT_SDP_SUPPORTED_FEATURES(_features) \
{ \
	BT_SDP_ATTR_SUPPORTED_FEATURES, \
	{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(_features) } \
}

/** @def BT_SDP_RECORD
 *  @brief SDP Service Declaration Macro.
 *
 *  Helper macro to declare a service.
 *
 *  @param _attrs List of attributes for the service record.
 */
#define BT_SDP_RECORD(_attrs) \
{ \
	.attrs = _attrs, \
	.attr_count = ARRAY_SIZE((_attrs)), \
}

/* Server API */

/** @brief Register a Service Record.
 *
 *  Register a Service Record. Applications can make use of
 *  macros such as BT_SDP_DECLARE_SERVICE, BT_SDP_LIST,
 *  BT_SDP_SERVICE_ID, BT_SDP_SERVICE_NAME, etc.
 *  A service declaration must start with BT_SDP_NEW_SERVICE.
 *
 *  @param service Service record declared using BT_SDP_DECLARE_SERVICE.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_sdp_register_service(struct bt_sdp_record *service);

/* Client API */

/** @brief Generic SDP Client Query Result data holder */
struct bt_sdp_client_result {
	/* buffer containing unparsed SDP record result for given UUID */
	struct net_buf        *resp_buf;
	/* flag pointing that there are more result chunks for given UUID */
	bool                   next_record_hint;
	/* Reference to UUID object on behalf one discovery was started */
	const struct bt_uuid  *uuid;
};

/** @brief Helper enum to be used as return value of bt_sdp_discover_func_t.
 *  The value informs the caller to perform further pending actions or stop them.
 */
enum {
	BT_SDP_DISCOVER_UUID_STOP = 0,
	BT_SDP_DISCOVER_UUID_CONTINUE,
};

/** @typedef bt_sdp_discover_func_t
 *
 *  @brief Callback type reporting to user that there is a resolved result
 *  on remote for given UUID and the result record buffer can be used by user
 *  for further inspection.
 *
 *  A function of this type is given by the user to the bt_sdp_discover_params
 *  object. It'll be called on each valid record discovery completion for given
 *  UUID. When UUID resolution gives back no records then NULL is passed
 *  to the user. Otherwise user can get valid record(s) and then the internal
 *  hint 'next record' is set to false saying the UUID resolution is complete or
 *  the hint can be set by caller to true meaning that next record is available
 *  for given UUID.
 *  The returned function value allows the user to control retrieving follow-up
 *  resolved records if any. If the user doesn't want to read more resolved
 *  records for given UUID since current record data fulfills its requirements
 *  then should return BT_SDP_DISCOVER_UUID_STOP. Otherwise returned value means
 *  more subcall iterations are allowable.
 *
 *  @param conn Connection object identifying connection to queried remote.
 *  @param result Object pointing to logical unparsed SDP record collected on
 *  base of response driven by given UUID.
 *
 *  @return BT_SDP_DISCOVER_UUID_STOP in case of no more need to read next
 *  record data and continue discovery for given UUID. By returning
 *  BT_SDP_DISCOVER_UUID_CONTINUE user allows this discovery continuation.
 */
typedef uint8_t (*bt_sdp_discover_func_t)
		(struct bt_conn *conn, struct bt_sdp_client_result *result);

/** @brief Main user structure used in SDP discovery of remote. */
struct bt_sdp_discover_params {
	sys_snode_t		_node;
	/** UUID (service) to be discovered on remote SDP entity */
	const struct bt_uuid   *uuid;
	/** Discover callback to be called on resolved SDP record */
	bt_sdp_discover_func_t  func;
	/** Memory buffer enabled by user for SDP query results  */
	struct net_buf_pool    *pool;
};

/** @brief Allows user to start SDP discovery session.
 *
 *  The function performs SDP service discovery on remote server driven by user
 *  delivered discovery parameters. Discovery session is made as soon as
 *  no SDP transaction is ongoing between peers and if any then this one
 *  is queued to be processed at discovery completion of previous one.
 *  On the service discovery completion the callback function will be
 *  called to get feedback to user about findings.
 *
 * @param conn Object identifying connection to remote.
 * @param params SDP discovery parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */

int bt_sdp_discover(struct bt_conn *conn,
		    const struct bt_sdp_discover_params *params);

/** @brief Release waiting SDP discovery request.
 *
 *  It can cancel valid waiting SDP client request identified by SDP discovery
 *  parameters object.
 *
 * @param conn Object identifying connection to remote.
 * @param params SDP discovery parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_sdp_discover_cancel(struct bt_conn *conn,
			   const struct bt_sdp_discover_params *params);


/* Helper types & functions for SDP client to get essential data from server */

/** @brief Protocols to be asked about specific parameters */
enum bt_sdp_proto {
	BT_SDP_PROTO_RFCOMM = 0x0003,
	BT_SDP_PROTO_L2CAP  = 0x0100,
};

/** @brief Give to user parameter value related to given stacked protocol UUID.
 *
 *  API extracts specific parameter associated with given protocol UUID
 *  available in Protocol Descriptor List attribute.
 *
 *  @param buf Original buffered raw record data.
 *  @param proto Known protocol to be checked like RFCOMM or L2CAP.
 *  @param param On success populated by found parameter value.
 *
 *  @return 0 on success when specific parameter associated with given protocol
 *  value is found, or negative if error occurred during processing.
 */
int bt_sdp_get_proto_param(const struct net_buf *buf, enum bt_sdp_proto proto,
			   uint16_t *param);

/** @brief Get additional parameter value related to given stacked protocol UUID.
 *
 *  API extracts specific parameter associated with given protocol UUID
 *  available in Additional Protocol Descriptor List attribute.
 *
 *  @param buf Original buffered raw record data.
 *  @param proto Known protocol to be checked like RFCOMM or L2CAP.
 *  @param param_index There may be more than one parameter related to the
 *  given protocol UUID. This function returns the result that is
 *  indexed by this parameter. It's value is from 0, 0 means the
 *  first matched result, 1 means the second matched result.
 *  @param[out] param On success populated by found parameter value.
 *
 *  @return 0 on success when a specific parameter associated with a given protocol
 *  value is found, or negative if error occurred during processing.
 */
int bt_sdp_get_addl_proto_param(const struct net_buf *buf, enum bt_sdp_proto proto,
			uint8_t param_index, uint16_t *param);

/** @brief Get profile version.
 *
 *  Helper API extracting remote profile version number. To get it proper
 *  generic profile parameter needs to be selected usually listed in SDP
 *  Interoperability Requirements section for given profile specification.
 *
 *  @param buf Original buffered raw record data.
 *  @param profile Profile family identifier the profile belongs.
 *  @param version On success populated by found version number.
 *
 *  @return 0 on success, negative value if error occurred during processing.
 */
int bt_sdp_get_profile_version(const struct net_buf *buf, uint16_t profile,
			       uint16_t *version);

/** @brief Get SupportedFeatures attribute value
 *
 *  Allows if exposed by remote retrieve SupportedFeature attribute.
 *
 *  @param buf Buffer holding original raw record data from remote.
 *  @param features On success object to be populated with SupportedFeature
 *  mask.
 *
 *  @return 0 on success if feature found and valid, negative in case any error
 */
int bt_sdp_get_features(const struct net_buf *buf, uint16_t *features);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SDP_H_ */
