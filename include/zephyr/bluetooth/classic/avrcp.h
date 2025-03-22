/** @file
 *  @brief Audio Video Remote Control Profile header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BT_AVRCP_COMPANY_ID_SIZE          (3)
#define BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG (0x001958)

/** @brief AVRCP Capability ID */
typedef enum __packed {
	BT_AVRCP_CAP_COMPANY_ID = 0x2,
	BT_AVRCP_CAP_EVENTS_SUPPORTED = 0x3,
} bt_avrcp_cap_t;

/** @brief AVRCP Notification Events */
typedef enum __packed {
	BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED = 0x01,
	BT_AVRCP_EVT_TRACK_CHANGED = 0x02,
	BT_AVRCP_EVT_TRACK_REACHED_END = 0x03,
	BT_AVRCP_EVT_TRACK_REACHED_START = 0x04,
	BT_AVRCP_EVT_PLAYBACK_POS_CHANGED = 0x05,
	BT_AVRCP_EVT_BATT_STATUS_CHANGED = 0x06,
	BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED = 0x07,
	BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED = 0x08,
	BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED = 0x09,
	BT_AVRCP_EVT_AVAILABLE_PLAYERS_CHANGED = 0x0a,
	BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED = 0x0b,
	BT_AVRCP_EVT_UIDS_CHANGED = 0x0c,
	BT_AVRCP_EVT_VOLUME_CHANGED = 0x0d,
} bt_avrcp_evt_t;

/** @brief AV/C command types */
typedef enum __packed {
	BT_AVRCP_CTYPE_CONTROL = 0x0,
	BT_AVRCP_CTYPE_STATUS = 0x1,
	BT_AVRCP_CTYPE_SPECIFIC_INQUIRY = 0x2,
	BT_AVRCP_CTYPE_NOTIFY = 0x3,
	BT_AVRCP_CTYPE_GENERAL_INQUIRY = 0x4,
} bt_avrcp_ctype_t;

/** @brief AV/C response codes */
typedef enum __packed {
	BT_AVRCP_RSP_NOT_IMPLEMENTED = 0x8,
	BT_AVRCP_RSP_ACCEPTED = 0x9,
	BT_AVRCP_RSP_REJECTED = 0xa,
	BT_AVRCP_RSP_IN_TRANSITION = 0xb,
	BT_AVRCP_RSP_IMPLEMENTED = 0xc, /**< For SPECIFIC_INQUIRY and GENERAL_INQUIRY commands */
	BT_AVRCP_RSP_STABLE = 0xc,      /**< For STATUS commands */
	BT_AVRCP_RSP_CHANGED = 0xd,
	BT_AVRCP_RSP_INTERIM = 0xf,
} bt_avrcp_rsp_t;

/** @brief AV/C operation ids used in AVRCP passthrough commands */
typedef enum __packed {
	BT_AVRCP_OPID_SELECT = 0x00,
	BT_AVRCP_OPID_UP = 0x01,
	BT_AVRCP_OPID_DOWN = 0x02,
	BT_AVRCP_OPID_LEFT = 0x03,
	BT_AVRCP_OPID_RIGHT = 0x04,
	BT_AVRCP_OPID_RIGHT_UP = 0x05,
	BT_AVRCP_OPID_RIGHT_DOWN = 0x06,
	BT_AVRCP_OPID_LEFT_UP = 0x07,
	BT_AVRCP_OPID_LEFT_DOWN = 0x08,
	BT_AVRCP_OPID_ROOT_MENU = 0x09,
	BT_AVRCP_OPID_SETUP_MENU = 0x0a,
	BT_AVRCP_OPID_CONTENTS_MENU = 0x0b,
	BT_AVRCP_OPID_FAVORITE_MENU = 0x0c,
	BT_AVRCP_OPID_EXIT = 0x0d,

	BT_AVRCP_OPID_0 = 0x20,
	BT_AVRCP_OPID_1 = 0x21,
	BT_AVRCP_OPID_2 = 0x22,
	BT_AVRCP_OPID_3 = 0x23,
	BT_AVRCP_OPID_4 = 0x24,
	BT_AVRCP_OPID_5 = 0x25,
	BT_AVRCP_OPID_6 = 0x26,
	BT_AVRCP_OPID_7 = 0x27,
	BT_AVRCP_OPID_8 = 0x28,
	BT_AVRCP_OPID_9 = 0x29,
	BT_AVRCP_OPID_DOT = 0x2a,
	BT_AVRCP_OPID_ENTER = 0x2b,
	BT_AVRCP_OPID_CLEAR = 0x2c,

	BT_AVRCP_OPID_CHANNEL_UP = 0x30,
	BT_AVRCP_OPID_CHANNEL_DOWN = 0x31,
	BT_AVRCP_OPID_PREVIOUS_CHANNEL = 0x32,
	BT_AVRCP_OPID_SOUND_SELECT = 0x33,
	BT_AVRCP_OPID_INPUT_SELECT = 0x34,
	BT_AVRCP_OPID_DISPLAY_INFORMATION = 0x35,
	BT_AVRCP_OPID_HELP = 0x36,
	BT_AVRCP_OPID_PAGE_UP = 0x37,
	BT_AVRCP_OPID_PAGE_DOWN = 0x38,

	BT_AVRCP_OPID_POWER = 0x40,
	BT_AVRCP_OPID_VOLUME_UP = 0x41,
	BT_AVRCP_OPID_VOLUME_DOWN = 0x42,
	BT_AVRCP_OPID_MUTE = 0x43,
	BT_AVRCP_OPID_PLAY = 0x44,
	BT_AVRCP_OPID_STOP = 0x45,
	BT_AVRCP_OPID_PAUSE = 0x46,
	BT_AVRCP_OPID_RECORD = 0x47,
	BT_AVRCP_OPID_REWIND = 0x48,
	BT_AVRCP_OPID_FAST_FORWARD = 0x49,
	BT_AVRCP_OPID_EJECT = 0x4a,
	BT_AVRCP_OPID_FORWARD = 0x4b,
	BT_AVRCP_OPID_BACKWARD = 0x4c,

	BT_AVRCP_OPID_ANGLE = 0x50,
	BT_AVRCP_OPID_SUBPICTURE = 0x51,

	BT_AVRCP_OPID_F1 = 0x71,
	BT_AVRCP_OPID_F2 = 0x72,
	BT_AVRCP_OPID_F3 = 0x73,
	BT_AVRCP_OPID_F4 = 0x74,
	BT_AVRCP_OPID_F5 = 0x75,
	BT_AVRCP_OPID_VENDOR_UNIQUE = 0x7e,
} bt_avrcp_opid_t;

/** @brief AVRCP button state flag */
typedef enum __packed {
	BT_AVRCP_BUTTON_PRESSED = 0,
	BT_AVRCP_BUTTON_RELEASED = 1,
} bt_avrcp_button_state_t;

/** @brief AVRCP structure */
struct bt_avrcp;

struct bt_avrcp_unit_info_rsp {
	uint8_t unit_type;
	uint32_t company_id;
};

struct bt_avrcp_subunit_info_rsp {
	uint8_t subunit_type;
	uint8_t max_subunit_id;
	const uint8_t *extended_subunit_type; /**< contains max_subunit_id items */
	const uint8_t *extended_subunit_id;   /**< contains max_subunit_id items */
};

#define BT_AVRCP_PASSTHROUGH_GET_STATE(payload)                                                    \
	((bt_avrcp_button_state_t)(FIELD_GET(BIT(7), ((payload)->byte0))))
#define BT_AVRCP_PASSTHROUGH_GET_OPID(payload)                                                     \
	((bt_avrcp_opid_t)(FIELD_GET(GENMASK(6, 0), ((payload)->byte0))))

struct bt_avrcp_passthrough_rsp {
	uint8_t byte0; /**< [7]: state_flag, [6:0]: opid */
	uint8_t data_len;
	uint8_t data[];
};

struct bt_avrcp_get_cap_rsp {
	uint8_t cap_id;  /**< bt_avrcp_cap_t */
	uint8_t cap_cnt; /**< number of items contained in *cap */
	uint8_t cap[];   /**< 1 or 3 octets each depends on cap_id */
};

struct bt_avrcp_cb {
	/** @brief An AVRCP connection has been established.
	 *
	 *  This callback notifies the application of an avrcp connection,
	 *  i.e., an AVCTP L2CAP connection.
	 *
	 *  @param avrcp AVRCP connection object.
	 */
	void (*connected)(struct bt_avrcp *avrcp);

	/** @brief An AVRCP connection has been disconnected.
	 *
	 *  This callback notifies the application that an avrcp connection
	 *  has been disconnected.
	 *
	 *  @param avrcp AVRCP connection object.
	 */
	void (*disconnected)(struct bt_avrcp *avrcp);

	/** @brief Callback function for bt_avrcp_get_cap().
	 *
	 *  Called when the get capabilities process is completed.
	 *
	 *  @param avrcp AVRCP connection object.
	 *  @param tid The transaction label of the response.
	 *  @param rsp The response for Get Capabilities command.
	 */
	void (*get_cap_rsp)(struct bt_avrcp *avrcp, uint8_t tid,
			    const struct bt_avrcp_get_cap_rsp *rsp);

	/** @brief Callback function for bt_avrcp_get_unit_info().
	 *
	 *  Called when the get unit info process is completed.
	 *
	 *  @param avrcp AVRCP connection object.
	 *  @param tid The transaction label of the response.
	 *  @param rsp The response for UNIT INFO command.
	 */
	void (*unit_info_rsp)(struct bt_avrcp *avrcp, uint8_t tid,
			      struct bt_avrcp_unit_info_rsp *rsp);

	/** @brief Callback function for bt_avrcp_get_subunit_info().
	 *
	 *  Called when the get subunit info process is completed.
	 *
	 *  @param avrcp AVRCP connection object.
	 *  @param tid The transaction label of the response.
	 *  @param rsp The response for SUBUNIT INFO command.
	 */
	void (*subunit_info_rsp)(struct bt_avrcp *avrcp, uint8_t tid,
				 struct bt_avrcp_subunit_info_rsp *rsp);

	/** @brief Callback function for bt_avrcp_passthrough().
	 *
	 *  Called when a passthrough response is received.
	 *
	 *  @param avrcp AVRCP connection object.
	 *  @param tid The transaction label of the response.
	 *  @param result The result of the operation.
	 *  @param rsp The response for PASS THROUGH command.
	 */
	void (*passthrough_rsp)(struct bt_avrcp *avrcp, uint8_t tid, bt_avrcp_rsp_t result,
				const struct bt_avrcp_passthrough_rsp *rsp);
};

/** @brief Connect AVRCP.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish AVRCP
 *  connection between devices.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return pointer to struct bt_avrcp in case of success or NULL in case
 *  of error.
 */
struct bt_avrcp *bt_avrcp_connect(struct bt_conn *conn);

/** @brief Disconnect AVRCP.
 *
 *  This function close AVCTP L2CAP connection.
 *
 *  @param avrcp The AVRCP instance.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_disconnect(struct bt_avrcp *avrcp);

/** @brief Register callback.
 *
 *  Register AVRCP callbacks to monitor the state and interact with the remote device.
 *
 *  @param cb The callback function.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_register_cb(const struct bt_avrcp_cb *cb);

/** @brief Get AVRCP Capabilities.
 *
 *  This function gets the capabilities supported by remote device.
 *
 *  @param avrcp The AVRCP instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param cap_id Specific capability requested, see @ref bt_avrcp_cap_t.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_get_cap(struct bt_avrcp *avrcp, uint8_t tid, uint8_t cap_id);

/** @brief Get AVRCP Unit Info.
 *
 *  This function obtains information that pertains to the AV/C unit as a whole.
 *
 *  @param avrcp The AVRCP instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_get_unit_info(struct bt_avrcp *avrcp, uint8_t tid);

/** @brief Get AVRCP Subunit Info.
 *
 *  This function obtains information about the subunit(s) of an AV/C unit. A device with AVRCP
 *  may support other subunits than the panel subunit if other profiles co-exist in the device.
 *
 *  @param avrcp The AVRCP instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_get_subunit_info(struct bt_avrcp *avrcp, uint8_t tid);

/** @brief Send AVRCP Pass Through command.
 *
 *  This function send a pass through command to the remote device. Passsthhrough command is used
 *  to transfer user operation information from a CT to Panel subunit of TG.
 *
 *  @param avrcp The AVRCP instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param opid The user operation id, see @ref bt_avrcp_opid_t.
 *  @param state The button state, see @ref bt_avrcp_button_state_t.
 *  @param payload The payload of the pass through command. Should not be NULL if len is not zero.
 *  @param len The length of the payload.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_passthrough(struct bt_avrcp *avrcp, uint8_t tid, uint8_t opid, uint8_t state,
			 const uint8_t *payload, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_H_ */
