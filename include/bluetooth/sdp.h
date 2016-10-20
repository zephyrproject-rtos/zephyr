/** @file
 *  @brief Service Discovery Protocol handling.
 */

/*
 * Copyright (c) 2016 Intel Corporation
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
#ifndef __BT_SDP_H
#define __BT_SDP_H

/**
 * @brief Service Discovery Protocol (SDP)
 * @defgroup bt_sdp Service Discovery Protocol (SDP)
 * @ingroup bluetooth
 * @{
 */

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
#define BT_SDP_ATTR_RECORD_HANDLE          0x0000
#define BT_SDP_ATTR_SVCLASS_ID_LIST        0x0001
#define BT_SDP_ATTR_RECORD_STATE           0x0002
#define BT_SDP_ATTR_SERVICE_ID             0x0003
#define BT_SDP_ATTR_PROTO_DESC_LIST        0x0004
#define BT_SDP_ATTR_BROWSE_GRP_LIST        0x0005
#define BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST 0x0006
#define BT_SDP_ATTR_SVCINFO_TTL            0x0007
#define BT_SDP_ATTR_SERVICE_AVAILABILITY   0x0008
#define BT_SDP_ATTR_PROFILE_DESC_LIST      0x0009
#define BT_SDP_ATTR_DOC_URL                0x000a
#define BT_SDP_ATTR_CLNT_EXEC_URL          0x000b
#define BT_SDP_ATTR_ICON_URL               0x000c
#define BT_SDP_ATTR_ADD_PROTO_DESC_LIST    0x000d

#define BT_SDP_ATTR_GROUP_ID                0x0200
#define BT_SDP_ATTR_IP_SUBNET               0x0200
#define BT_SDP_ATTR_VERSION_NUM_LIST        0x0200
#define BT_SDP_ATTR_SUPPORTED_FEATURES_LIST 0x0200
#define BT_SDP_ATTR_GOEP_L2CAP_PSM          0x0200
#define BT_SDP_ATTR_SVCDB_STATE             0x0201

#define BT_SDP_ATTR_MPSD_SCENARIOS   0x0200
#define BT_SDP_ATTR_MPMD_SCENARIOS   0x0201
#define BT_SDP_ATTR_MPS_DEPENDENCIES 0x0202

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

#define BT_SDP_ATTR_SPECIFICATION_ID 0x0200
#define BT_SDP_ATTR_VENDOR_ID        0x0201
#define BT_SDP_ATTR_PRODUCT_ID       0x0202
#define BT_SDP_ATTR_VERSION          0x0203
#define BT_SDP_ATTR_PRIMARY_RECORD   0x0204
#define BT_SDP_ATTR_VENDOR_ID_SOURCE 0x0205

#define BT_SDP_ATTR_HID_DEVICE_RELEASE_NUMBER 0x0200
#define BT_SDP_ATTR_HID_PARSER_VERSION        0x0201
#define BT_SDP_ATTR_HID_DEVICE_SUBCLASS       0x0202
#define BT_SDP_ATTR_HID_COUNTRY_CODE          0x0203
#define BT_SDP_ATTR_HID_VIRTUAL_CABLE         0x0204
#define BT_SDP_ATTR_HID_RECONNECT_INITIATE    0x0205
#define BT_SDP_ATTR_HID_DESCRIPTOR_LIST       0x0206
#define BT_SDP_ATTR_HID_LANG_ID_BASE_LIST     0x0207
#define BT_SDP_ATTR_HID_SDP_DISABLE           0x0208
#define BT_SDP_ATTR_HID_BATTERY_POWER         0x0209
#define BT_SDP_ATTR_HID_REMOTE_WAKEUP         0x020a
#define BT_SDP_ATTR_HID_PROFILE_VERSION       0x020b
#define BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT   0x020c
#define BT_SDP_ATTR_HID_NORMALLY_CONNECTABLE  0x020d
#define BT_SDP_ATTR_HID_BOOT_DEVICE           0x020e

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


/** @brief SDP Generic Data Element Value. */
struct bt_sdp_data_elem {
	uint8_t *header; /* Type and size descriptor */
	void    *data;   /* Data */
};

/** @brief SDP Attribute Value. */
struct bt_sdp_attribute {
	uint16_t                id;  /* Attribute ID */
	struct bt_sdp_data_elem val; /* Attribute data */
};

/** @brief SDP Service Record Value. */
struct bt_sdp_record {
	uint32_t                 handle;     /* Redundant, for quick ref */
	struct bt_sdp_attribute *attrs;      /* Base addr of attr array */
	size_t                   attr_count; /* Number of attributes */
	uint8_t                  index;      /* Index of the record in LL */
	struct bt_sdp_record    *next;
};

/*
 * ---------------------------------------------------    ------------------
 * | Service Hdl | Attr list ptr | Attr count | Next | -> | Service Hdl | ...
 * ---------------------------------------------------    ------------------
*/

#define BT_SDP_ARRAY_8(...) ((uint8_t[]) {__VA_ARGS__})
#define BT_SDP_ARRAY_16(...) ((uint16_t[]) {__VA_ARGS__})
#define BT_SDP_ARRAY_32(...) ((uint32_t[]) {__VA_ARGS__})

#define BT_SDP_TYPE_SIZE(...) BT_SDP_ARRAY_8(__VA_ARGS__)
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
	.id = BT_SDP_ATTR_RECORD_HANDLE, \
	.val.header = BT_SDP_TYPE_SIZE(BT_SDP_UINT32), \
	.val.data = BT_SDP_ARRAY_32(0), \
}, \
{ \
	.id = BT_SDP_ATTR_RECORD_STATE, \
	.val.header = BT_SDP_TYPE_SIZE(BT_SDP_UINT32), \
	.val.data = BT_SDP_ARRAY_32(0), \
}, \
{ \
	.id = BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST, \
	.val.header = BT_SDP_TYPE_SIZE(BT_SDP_SEQ8, 9), \
	.val.data = BT_SDP_DATA_ELEM_LIST( \
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_8('n', 'e') }, \
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(106) }, \
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
			BT_SDP_ARRAY_16(BT_SDP_PRIMARY_LANG_BASE) } \
	), \
}, \
{ \
	.id = BT_SDP_ATTR_BROWSE_GRP_LIST, \
	.val.header = BT_SDP_TYPE_SIZE(BT_SDP_SEQ8, 3), \
	.val.data = BT_SDP_DATA_ELEM_LIST( \
		{ BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_PUBLIC_BROWSE_GROUP) }, \
	), \
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
	.id = _att_id, \
	.val.header = _type_size, \
	.val.data = _data_elem_seq, \
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
	.id = BT_SDP_ATTR_SERVICE_ID, \
	.val.header = BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
	.val.data = &((struct bt_uuid_16) _uuid), \
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
	.id = BT_SDP_ATTR_SVCNAME_PRIMARY, \
	.val.header = BT_SDP_TYPE_SIZE(BT_SDP_TEXT_STR8, \
			(sizeof(_name)-1)), \
	.val.data = _name, \
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
	.id = BT_SDP_ATTR_SUPPORTED_FEATURES, \
	.val.header = BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
	.val.data = BT_SDP_ARRAY_16(_features), \
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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __BT_SDP_H */
