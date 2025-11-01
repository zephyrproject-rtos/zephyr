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

/** @brief AV/C subunit type, also used for unit type */
typedef enum __packed {
	BT_AVRCP_SUBUNIT_TYPE_PANEL = 0x09,
	BT_AVRCP_SUBUNIT_TYPE_UNIT = 0x1f,
} bt_avrcp_subunit_type_t;

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

/**
 * @brief AVRCP status and error codes.
 *
 * These status codes are used in AVRCP responses to indicate the result of a command.
 */
typedef enum __packed {
	/** Invalid command.
	 * Valid for Commands: All
	 */
	BT_AVRCP_STATUS_INVALID_COMMAND = 0x00,

	/** Invalid parameter.
	 * Valid for Commands: All
	 */
	BT_AVRCP_STATUS_INVALID_PARAMETER = 0x01,

	/** Parameter content error.
	 * Valid for Commands: All
	 */
	BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR = 0x02,

	/** Internal error.
	 * Valid for Commands: All
	 */
	BT_AVRCP_STATUS_INTERNAL_ERROR = 0x03,

	/** Operation completed without error.
	 * Valid for Commands: All except where the response CType is AV/C REJECTED
	 */
	BT_AVRCP_STATUS_OPERATION_COMPLETED = 0x04,

	/** The UIDs on the device have changed.
	 * Valid for Commands: All
	 */
	BT_AVRCP_STATUS_UID_CHANGED = 0x05,

	/** The Direction parameter is invalid.
	 * Valid for Commands: ChangePath
	 */
	BT_AVRCP_STATUS_INVALID_DIRECTION = 0x07,

	/** The UID provided does not refer to a folder item.
	 * Valid for Commands: ChangePath
	 */
	BT_AVRCP_STATUS_NOT_A_DIRECTORY = 0x08,

	/** The UID provided does not refer to any currently valid item.
	 * Valid for Commands: ChangePath, PlayItem, AddToNowPlaying, GetItemAttributes
	 */
	BT_AVRCP_STATUS_DOES_NOT_EXIST = 0x09,

	/** Invalid scope.
	 * Valid for Commands: GetFolderItems, PlayItem, AddToNowPlayer, GetItemAttributes,
	 * GetTotalNumberOfItems
	 */
	BT_AVRCP_STATUS_INVALID_SCOPE = 0x0a,

	/** Range out of bounds.
	 * Valid for Commands: GetFolderItems
	 */
	BT_AVRCP_STATUS_RANGE_OUT_OF_BOUNDS = 0x0b,

	/** Folder item is not playable.
	 * Valid for Commands: Play Item, AddToNowPlaying
	 */
	BT_AVRCP_STATUS_FOLDER_ITEM_IS_NOT_PLAYABLE = 0x0c,

	/** Media in use.
	 * Valid for Commands: PlayItem, AddToNowPlaying
	 */
	BT_AVRCP_STATUS_MEDIA_IN_USE = 0x0d,

	/** Now Playing List full.
	 * Valid for Commands: AddToNowPlaying
	 */
	BT_AVRCP_STATUS_NOW_PLAYING_LIST_FULL = 0x0e,

	/** Search not supported.
	 * Valid for Commands: Search
	 */
	BT_AVRCP_STATUS_SEARCH_NOT_SUPPORTED = 0x0f,

	/** Search in progress.
	 * Valid for Commands: Search
	 */
	BT_AVRCP_STATUS_SEARCH_IN_PROGRESS = 0x10,

	/** The specified Player Id does not refer to a valid player.
	 * Valid for Commands: SetAddressedPlayer, SetBrowsedPlayer
	 */
	BT_AVRCP_STATUS_INVALID_PLAYER_ID = 0x11,

	/** Player not browsable.
	 * Valid for Commands: SetBrowsedPlayer
	 */
	BT_AVRCP_STATUS_PLAYER_NOT_BROWSABLE = 0x12,

	/** Player not addressed.
	 * Valid for Commands: Search, SetBrowsedPlayer
	 */
	BT_AVRCP_STATUS_PLAYER_NOT_ADDRESSED = 0x13,

	/** No valid search results.
	 * Valid for Commands: GetFolderItems
	 */
	BT_AVRCP_STATUS_NO_VALID_SEARCH_RESULTS = 0x14,

	/** No available players.
	 * Valid for Commands: ALL
	 */
	BT_AVRCP_STATUS_NO_AVAILABLE_PLAYERS = 0x15,

	/** Addressed player changed.
	 * Valid for Commands: All Register Notification commands
	 */
	BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED = 0x16,

	/** In transition response.
	 * The target is currently changing state (e.g., between play/pause).
	 * Note: This status is not used for browsing commands.
	 */
	BT_AVRCP_STATUS_IN_TRANSITION = 0xfd,

	/** Not implemented response.
	 * The command/PDU is not supported by the target device.
	 * Note: This status is not used for browsing commands.
	 */
	BT_AVRCP_STATUS_NOT_IMPLEMENTED = 0xfe,

	/** Successful response.
	 * The requested command or PDU was processed successfully by the target device.
	 *
	 * For control commands, it means the request was accepted.
	 * For status commands, it means the state is stable and reported successfully.
	 */
	BT_AVRCP_STATUS_SUCCESS = BT_AVRCP_STATUS_OPERATION_COMPLETED,
} bt_avrcp_status_t;

/** @brief AVRCP CT structure */
struct bt_avrcp_ct;
/** @brief AVRCP TG structure */
struct bt_avrcp_tg;

struct bt_avrcp_unit_info_rsp {
	bt_avrcp_subunit_type_t unit_type;
	uint32_t company_id;
};

struct bt_avrcp_subunit_info_rsp {
	bt_avrcp_subunit_type_t subunit_type;
	uint8_t max_subunit_id;
	const uint8_t *extended_subunit_type; /**< contains max_subunit_id items */
	const uint8_t *extended_subunit_id;   /**< contains max_subunit_id items */
};

#define BT_AVRCP_PASSTHROUGH_GET_STATE(payload)                                                    \
	((bt_avrcp_opid_t)(FIELD_GET(BIT(7), ((payload)->opid_state))))
#define BT_AVRCP_PASSTHROUGH_GET_OPID(payload)                                                     \
	((bt_avrcp_button_state_t)(FIELD_GET(GENMASK(6, 0), ((payload)->opid_state))))
#define BT_AVRCP_PASSTHROUGH_SET_STATE_OPID(payload, state, opid)                                  \
	(payload)->opid_state = FIELD_PREP(BIT(7), state) | FIELD_PREP(GENMASK(6, 0), opid)

struct bt_avrcp_passthrough_opvu_data {
	uint8_t company_id[BT_AVRCP_COMPANY_ID_SIZE];
	uint16_t opid_vu;
} __packed;

struct bt_avrcp_passthrough_cmd {
	uint8_t opid_state; /**< [7]: state_flag, [6:0]: opid */
	uint8_t data_len;
	struct bt_avrcp_passthrough_opvu_data data[0]; /**< opvu data */
} __packed;

struct bt_avrcp_passthrough_rsp {
	uint8_t opid_state; /**< [7]: state_flag, [6:0]: opid */
	uint8_t data_len;
	struct bt_avrcp_passthrough_opvu_data data[0]; /**< opvu data */
} __packed;

struct bt_avrcp_get_caps_rsp {
	uint8_t cap_id;  /**< bt_avrcp_cap_t */
	uint8_t cap_cnt; /**< number of items contained in *cap */
	uint8_t cap[];   /**< 1 or 3 octets each depends on cap_id */
} __packed;

/** @brief AVRCP Character Set IDs */
typedef enum __packed {
	BT_AVRCP_CHARSET_UTF8 = 0x006a,
} bt_avrcp_charset_t;

/** @brief get folder name (response) */
struct bt_avrcp_folder_name {
	uint16_t folder_name_len;
	uint8_t folder_name[];
} __packed;

/** @brief Set browsed player response structure */
struct bt_avrcp_set_browsed_player_rsp {
	uint8_t status;                              /**< Status see bt_avrcp_status_t.*/
	uint16_t uid_counter;                        /**< UID counter */
	uint32_t num_items;                          /**< Number of items in the folder */
	uint16_t charset_id;                         /**< Character set ID */
	uint8_t folder_depth;                        /**< Folder depth */
	struct bt_avrcp_folder_name folder_names[0]; /**< Folder names data */
} __packed;

/** @brief AVRCP Playback Status */
typedef enum __packed {
	BT_AVRCP_PLAYBACK_STATUS_STOPPED  = 0x00,
	BT_AVRCP_PLAYBACK_STATUS_PLAYING  = 0x01,
	BT_AVRCP_PLAYBACK_STATUS_PAUSED   = 0x02,
	BT_AVRCP_PLAYBACK_STATUS_FWD_SEEK = 0x03,
	BT_AVRCP_PLAYBACK_STATUS_REV_SEEK = 0x04,
	BT_AVRCP_PLAYBACK_STATUS_ERROR    = 0xFF,
} bt_avrcp_playback_status_t;

/** @brief AVRCP System Status Code. */
typedef enum __packed {
	BT_AVRCP_SYSTEM_STATUS_POWER_ON  = 0x00,
	BT_AVRCP_SYSTEM_STATUS_POWER_OFF = 0x01,
	BT_AVRCP_SYSTEM_STATUS_UNPLUGGED = 0x02,
} bt_avrcp_system_status_t;

/** @brief AVRCP Battery Status Code. */
typedef enum __packed {
	BT_AVRCP_BATTERY_STATUS_NORMAL   = 0x00,
	BT_AVRCP_BATTERY_STATUS_WARNING  = 0x01,
	BT_AVRCP_BATTERY_STATUS_CRITICAL = 0x02,
	BT_AVRCP_BATTERY_STATUS_EXTERNAL = 0x03,
	BT_AVRCP_BATTERY_STATUS_FULL     = 0x04,
} bt_avrcp_battery_status_t;

/** AVRCP MAX absolute volume. */
#define BT_AVRCP_MAX_ABSOLUTE_VOLUME 0x7F

/** @brief AVRCP Media Attribute IDs */
typedef enum __packed {
	BT_AVRCP_MEDIA_ATTR_TITLE = 0x01,
	BT_AVRCP_MEDIA_ATTR_ARTIST = 0x02,
	BT_AVRCP_MEDIA_ATTR_ALBUM = 0x03,
	BT_AVRCP_MEDIA_ATTR_TRACK_NUMBER = 0x04,
	BT_AVRCP_MEDIA_ATTR_TOTAL_TRACKS = 0x05,
	BT_AVRCP_MEDIA_ATTR_GENRE = 0x06,
	BT_AVRCP_MEDIA_ATTR_PLAYING_TIME = 0x07,
} bt_avrcp_media_attr_t;

/** @brief GetElementAttributes command request structure */
struct bt_avrcp_get_element_attrs_cmd {
	uint8_t identifier[8]; /**< Element identifier (0x0 for currently playing) */
	uint8_t num_attrs;     /**< Number of attributes requested (0 = all) */
	uint32_t attr_ids[];   /**< Array of requested attribute IDs */
} __packed;

/** @brief AVRCP Media Attribute structure */
struct bt_avrcp_media_attr {
	uint32_t attr_id;    /**< Media attribute ID, see @ref bt_avrcp_media_attr_t */
	uint16_t charset_id; /**< Character set ID, see @ref bt_avrcp_charset_t */
	uint16_t attr_len;   /**< Length of attribute value */
	uint8_t attr_val[];  /**< Attribute value data */
} __packed;

/** @brief GetElementAttributes response structure */
struct bt_avrcp_get_element_attrs_rsp {
	uint8_t num_attrs;			/**< Number of attributes in response */
	struct bt_avrcp_media_attr attrs[];	/**< Array of media attributes */
} __packed;

/** @brief AVRCP Player Application Setting Attribute IDs */
typedef enum __packed {
	BT_AVRCP_PLAYER_ATTR_EQUALIZER = 0x01U,
	BT_AVRCP_PLAYER_ATTR_REPEAT_MODE = 0x02U,
	BT_AVRCP_PLAYER_ATTR_SHUFFLE = 0x03U,
	BT_AVRCP_PLAYER_ATTR_SCAN = 0x04U,
} bt_avrcp_player_attr_id_t;

/** @brief AVRCP Player Application Setting Values for Equalizer */
typedef enum __packed {
	BT_AVRCP_EQUALIZER_OFF = 0x01U,
	BT_AVRCP_EQUALIZER_ON = 0x02U,
} bt_avrcp_equalizer_value_t;

/** @brief AVRCP Player Application Setting Values for Repeat Mode */
typedef enum __packed {
	BT_AVRCP_REPEAT_MODE_OFF = 0x01U,
	BT_AVRCP_REPEAT_MODE_SINGLE_TRACK = 0x02U,
	BT_AVRCP_REPEAT_MODE_ALL_TRACKS = 0x03U,
	BT_AVRCP_REPEAT_MODE_GROUP = 0x04U,
} bt_avrcp_repeat_mode_value_t;

/** @brief AVRCP Player Application Setting Values for Shuffle */
typedef enum __packed {
	BT_AVRCP_SHUFFLE_OFF = 0x01U,
	BT_AVRCP_SHUFFLE_ALL_TRACKS = 0x02U,
	BT_AVRCP_SHUFFLE_GROUP = 0x03U,
} bt_avrcp_shuffle_value_t;

/** @brief AVRCP Player Application Setting Values for Scan */
typedef enum __packed {
	BT_AVRCP_SCAN_OFF = 0x01U,
	BT_AVRCP_SCAN_ALL_TRACKS = 0x02U,
	BT_AVRCP_SCAN_GROUP = 0x03U,
} bt_avrcp_scan_value_t;

/** @brief AVRCP Scope Values
 *  0x00 = Media Player List
 *  0x01 = Filesystem
 *  0x02 = Search
 *  0x03 = Now Playing
 */
typedef enum __packed {
	BT_AVRCP_SCOPE_MEDIA_PLAYER_LIST = 0x00U,
	BT_AVRCP_SCOPE_FILESYSTEM        = 0x01U,
	BT_AVRCP_SCOPE_SEARCH            = 0x02U,
	BT_AVRCP_SCOPE_NOW_PLAYING       = 0x03U,
} bt_avrcp_scope_t;

/** @brief ListPlayerApplicationSettingAttributes response */
struct bt_avrcp_list_app_setting_attr_rsp {
	uint8_t num_attrs;       /**< Number of application setting attributes */
	uint8_t attr_ids[];      /**< Array of attribute IDs @ref bt_avrcp_player_attr_id_t */
} __packed;

/** @brief ListPlayerApplicationSettingValues command request */
struct bt_avrcp_list_player_app_setting_vals_cmd {
	uint8_t attr_id;         /**< Attribute ID to query values for */
} __packed;

/** @brief ListPlayerApplicationSettingValues response */
struct bt_avrcp_list_player_app_setting_vals_rsp {
	uint8_t num_values;      /**< Number of values for the attribute */
	uint8_t values[];        /**< Array of possible values */
} __packed;

/** @brief GetCurrentPlayerApplicationSettingValue command request */
struct bt_avrcp_get_curr_player_app_setting_val_cmd {
	uint8_t num_attrs;       /**< Number of attributes to query */
	uint8_t attr_ids[];      /**< Array of attribute IDs */
} __packed;

/** @brief AVRCP Attribute-Value Pair */
struct bt_avrcp_app_setting_attr_val {
	uint8_t attr_id;   /**< Attribute ID */
	uint8_t value_id;  /**< Value ID */
} __packed;

/** @brief GetCurrentPlayerApplicationSettingValue response */
struct bt_avrcp_get_curr_player_app_setting_val_rsp {
	uint8_t num_attrs;       /**< Number of attributes returned */
	struct bt_avrcp_app_setting_attr_val attr_vals[]; /**< Array of attribute-value pairs */
} __packed;

/** @brief SetPlayerApplicationSettingValue command request */
struct bt_avrcp_set_player_app_setting_val_cmd {
	uint8_t num_attrs;       /**< Number of attributes to set */
	struct bt_avrcp_app_setting_attr_val attr_vals[]; /**< Array of attribute-value pairs */
} __packed;

/** @brief GetPlayerApplicationSettingAttributeText command request */
struct bt_avrcp_get_player_app_setting_attr_text_cmd {
	uint8_t num_attrs;       /**< Number of attributes to get text for */
	uint8_t attr_ids[];      /**< Array of attribute IDs */
} __packed;

/** @brief AVRCP Attribute Text Entry */
struct bt_avrcp_app_setting_attr_text {
	uint8_t attr_id;       /**< Attribute ID */
	uint16_t charset_id;   /**< Charset ID */
	uint8_t text_len;      /**< Length of text */
	uint8_t text[];        /**< Text string */
} __packed;

/** @brief GetPlayerApplicationSettingAttributeText response */
struct bt_avrcp_get_player_app_setting_attr_text_rsp {
	uint8_t num_attrs;       /**< Number of attributes returned */
	struct bt_avrcp_app_setting_attr_text attr_text[];
} __packed;

/** @brief GetPlayerApplicationSettingValueText command request */
struct bt_avrcp_get_player_app_setting_val_text_cmd {
	uint8_t attr_id;         /**< Attribute ID */
	uint8_t num_values;      /**< Number of values to get text for */
	uint8_t value_ids[];     /**< Array of value IDs */
} __packed;

/** @brief AVRCP Attribute Text Entry */
struct bt_avrcp_app_setting_val_text {
	uint8_t value_id;    /**< Value ID */
	uint16_t charset_id;   /**< Charset ID */
	uint8_t text_len;      /**< Length of text */
	uint8_t text[];        /**< Text string */
} __packed;

/** @brief GetPlayerApplicationSettingValueText response */
struct bt_avrcp_get_player_app_setting_val_text_rsp {
	uint8_t num_values;      /**< Number of values returned */
	struct bt_avrcp_app_setting_val_text value_text[];
} __packed;

/** @brief InformDisplayableCharacterSet command request */
struct bt_avrcp_inform_displayable_char_set_cmd {
	uint8_t num_charsets;    /**< Number of character sets supported */
	uint16_t charset_ids[];  /**< Array of character set IDs */
} __packed;

/** @brief InformBatteryStatusOfCT command request */
struct bt_avrcp_inform_batt_status_of_ct_cmd {
	uint8_t battery_status;  /**< Battery status value @ref bt_avrcp_battery_status_t */
} __packed;

/** @brief GetPlayStatus response */
struct bt_avrcp_get_play_status_rsp {
	uint32_t song_length;    /**< Total length of the song in milliseconds */
	uint32_t song_position;  /**< Current position in the song in milliseconds */
	uint8_t play_status;     /**< Play status: @ref bt_avrcp_playback_status_t */
} __packed;

/** @brief RegisterNotification command request */
struct bt_avrcp_register_notification_cmd {
	uint8_t event_id;        /**< Event ID to register for */
	uint32_t interval;       /**< Playback interval (used only for event_id = 0x05) */
} __packed;

/** @brief SetAbsoluteVolume command request */
struct bt_avrcp_set_absolute_volume_cmd {
	uint8_t absolute_volume;          /**< Volume level (0x00 to 0x7F) */
} __packed;

/** @brief SetAbsoluteVolume response */
struct bt_avrcp_set_absolute_volume_rsp {
	uint8_t absolute_volume;          /**< Volume level acknowledged */
} __packed;

/** @brief SetAddressedPlayer command request */
struct bt_avrcp_set_addressed_player_cmd {
	uint16_t player_id;      /**< Player ID to be addressed */
} __packed;

/** @brief PlayItem command request */
struct bt_avrcp_play_item_cmd {
	uint8_t scope;        /**< Scope: @ref bt_avrcp_scope_t */
	uint8_t uid[8];       /**< UID of the item */
	uint16_t uid_counter; /**< UID counter */
} __packed;

/** @brief AddToNowPlaying command request */
struct bt_avrcp_add_to_now_playing_cmd {
	uint8_t scope;           /**< Scope: @ref bt_avrcp_scope_t */
	uint8_t uid[8];         /**< UID of the item */
	uint16_t uid_counter;    /**< UID counter */
} __packed;

struct bt_avrcp_event_data {
	union {
		/* EVENT_PLAYBACK_STATUS_CHANGED */
		uint8_t play_status;

		/* EVENT_TRACK_CHANGED */
		uint8_t identifier[8];

		/* EVENT_PLAYBACK_POS_CHANGED */
		uint32_t playback_pos;

		/* EVENT_BATT_STATUS_CHANGED */
		uint8_t battery_status;

		/* EVENT_SYSTEM_STATUS_CHANGED */
		uint8_t system_status;

		/* EVENT_PLAYER_APPLICATION_SETTING_CHANGED */
		struct __packed {
			uint8_t num_of_attr;
			struct bt_avrcp_app_setting_attr_val *attr_vals;
		} setting_changed;

		/* EVENT_ADDRESSED_PLAYER_CHANGED */
		struct __packed {
			uint16_t player_id;
			uint16_t uid_counter;
		} addressed_player_changed;

		/* EVENT_UIDS_CHANGED */
		uint16_t uid_counter;

		/* EVENT_VOLUME_CHANGED */
		uint8_t absolute_volume;
	};
};

/** @brief Callback for AVRCP event notifications (CHANGED only).
 *
 * This callback is invoked by the AVRCP Target (TG) when a registered event
 * occurs and a "changed" notification needs to be sent to the Controller (CT).
 *
 * For interim and rejected error cases, the callback will trigger the
 * `notification` function registered by the Controller
 *
 *  @param ct AVRCP CT connection object.
 *
 *  @param event_id The AVRCP event identifier. @ref bt_avrcp_event_data
 *        This corresponds to one of the AVRCP event types such as
 *         EVENT_PLAYBACK_STATUS_CHANGED, EVENT_TRACK_CHANGED, etc.
 *
 *  @param data Pointer to @ref bt_avrcp_event_data structure containing the event-specific
 *        data. The content of the union depends on the event_id.
 *
 *  @note The callback implementation should not block or perform heavy operations.
 *       If needed, defer processing to another thread or task.
 */
typedef void(*bt_avrcp_notify_changed_cb_t)(struct bt_avrcp_ct *ct, uint8_t event_id,
					    struct bt_avrcp_event_data *data);

struct bt_avrcp_ct_cb {
	/** @brief An AVRCP CT connection has been established.
	 *
	 *  This callback notifies the application of an avrcp connection,
	 *  i.e., an AVCTP L2CAP connection.
	 *
	 *  @param conn Connection object.
	 *  @param ct AVRCP CT connection object.
	 */
	void (*connected)(struct bt_conn *conn, struct bt_avrcp_ct *ct);

	/** @brief An AVRCP CT connection has been disconnected.
	 *
	 *  This callback notifies the application that an avrcp connection
	 *  has been disconnected.
	 *
	 *  @param ct AVRCP CT connection object.
	 */
	void (*disconnected)(struct bt_avrcp_ct *ct);

	/** @brief An AVRCP CT browsing connection has been established.
	 *
	 *  This callback notifies the application of an avrcp browsing connection,
	 *  i.e., an AVCTP browsing L2CAP connection.
	 *
	 *  @param conn Connection object.
	 *  @param ct AVRCP CT connection object.
	 */
	void (*browsing_connected)(struct bt_conn *conn, struct bt_avrcp_ct *ct);

	/** @brief An AVRCP CT browsing connection has been disconnected.
	 *
	 *  This callback notifies the application that an avrcp browsing connection
	 *  has been disconnected.
	 *
	 *  @param ct AVRCP CT connection object.
	 */
	void (*browsing_disconnected)(struct bt_avrcp_ct *ct);

	/** @brief Callback function for bt_avrcp_get_caps().
	 *
	 *  Called when the get capabilities process is completed.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the response.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param buf The response buffer containing the BT_AVRCP_PDU_ID_GET_CAPS
	 *            payload returned by the TG. The application can parse this
	 *            payload according to the format, defined in @ref bt_avrcp_get_caps_rsp.
	 *            If status is in the range BT_AVRCP_STATUS_INVALID_COMMAND to
	 *            BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, and is not equal to
	 *            BT_AVRCP_STATUS_OPERATION_COMPLETED, it indicates that the AVRCP response
	 *            code is an AV/C REJECTED response, and buf is NULL.
	 *            Note that all multi-octet fields are encoded in big-endian format.
	 */
	void (*get_caps)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status, struct net_buf *buf);

	/** @brief Callback function for bt_avrcp_get_unit_info().
	 *
	 *  Called when the get unit info process is completed.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the response.
	 *  @param rsp The response for UNIT INFO command.
	 */
	void (*unit_info_rsp)(struct bt_avrcp_ct *ct, uint8_t tid,
			      struct bt_avrcp_unit_info_rsp *rsp);

	/** @brief Callback function for bt_avrcp_get_subunit_info().
	 *
	 *  Called when the get subunit info process is completed.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the response.
	 *  @param rsp The response for SUBUNIT INFO command.
	 */
	void (*subunit_info_rsp)(struct bt_avrcp_ct *ct, uint8_t tid,
				 struct bt_avrcp_subunit_info_rsp *rsp);

	/** @brief Callback function for bt_avrcp_passthrough().
	 *
	 *  Called when a passthrough response is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the response.
	 *  @param result The result of the operation.
	 *  @param rsp The response for PASS THROUGH command.
	 */
	void (*passthrough_rsp)(struct bt_avrcp_ct *ct, uint8_t tid, bt_avrcp_rsp_t result,
				const struct bt_avrcp_passthrough_rsp *rsp);

	/** @brief Callback function for bt_avrcp_ct_set_browsed_player().
	 *
	 *  Called when the set browsed player process is completed.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the response.
	 *  @param buf The response buffer containing the set browsed player response data.
	 *             The application can parse this payload according to the format defined in
	 *             @ref bt_avrcp_set_browsed_player_rsp. Note that the data is encoded in
	 *             big-endian format.
	 */
	void (*browsed_player_rsp)(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf);

	/** @brief Callback function for Event Notification response (CT).
	 *
	 * Called when the AVRCP Target (TG) sends a response to a previously
	 * registered event (Register Notification). This callback reports the
	 * event type, the response phase (e.g., Interim), and the event-specific
	 * payload.
	 *
	 * @param ct       AVRCP Controller (CT) connection context.
	 * @param tid      Transaction label that correlates this notification
	 *                 with the original Register Notification request.
	 * @param status   TG status/phase code (BT_AVRCP_STATUS_*). Typically
	 *                 BT_AVRCP_STATUS_SUCCESS for an interim notification.
	 *                 Error codes may be returned for invalid parameters or
	 *                 unsupported events.
	 * @param event_id The AVRCP event identifier. @ref bt_avrcp_event_data
	 *        This corresponds to one of the AVRCP event types such as
	 *        EVENT_PLAYBACK_STATUS_CHANGED, EVENT_TRACK_CHANGED, etc.
	 *
	 * @param data Pointer to @ref bt_avrcp_event_data structure containing the
	 *         event-specific data. The content of the union depends on the event_id.
	 *
	 * @note This callback is only invoked for interim notifications and error
	 *       statuses from the TG. For CHANGED event notifications, the event must
	 *       first be registered using @ref bt_avrcp_notify_changed_cb_t.
	 */
	void (*notification)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status, uint8_t event_id,
			     struct bt_avrcp_event_data *data);

	/** @brief Callback for PDU ID LIST_PLAYER_APP_SETTING_ATTRS.
	 *
	 *  Called when the response for LIST_PLAYER_APP_SETTING_ATTRS is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param buf The response buffer containing the LIST_PLAYER_APP_SETTING_ATTRS
	 *            payload returned by the TG.
	 *            The application can parse this payload according to the format
	 *            defined in @ref bt_avrcp_list_app_setting_attr_rsp.
	 *            If status is in the range BT_AVRCP_STATUS_INVALID_COMMAND to
	 *            BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, and is not equal to
	 *            BT_AVRCP_STATUS_OPERATION_COMPLETED, it indicates that the AVRCP response
	 *            code is an AV/C REJECTED response, and buf is NULL.
	 *            Note that all multi-octet fields are encoded in big-endian format.
	 */
	void (*list_player_app_setting_attrs)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
					      struct net_buf *buf);

	/** @brief Callback for PDU ID LIST_PLAYER_APP_SETTING_VALS.
	 *
	 *  Called when the response for LIST_PLAYER_APP_SETTING_VALS is received.`
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param buf The response buffer containing the LIST_PLAYER_APP_SETTING_VALS
	 *            payload returned by the TG.
	 *            The application can parse this payload according to the format
	 *            defined in @ref bt_avrcp_list_player_app_setting_vals_rsp.
	 *            If status is in the range BT_AVRCP_STATUS_INVALID_COMMAND to
	 *            BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, and is not equal to
	 *            BT_AVRCP_STATUS_OPERATION_COMPLETED, it indicates that the AVRCP response
	 *            code is an AV/C REJECTED response, and buf is NULL.
	 *            Note that all multi-octet fields are encoded in big-endian format.
	 */
	void (*list_player_app_setting_vals)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
					     struct net_buf *buf);

	/** @brief Callback for PDU ID GET_CURR_PLAYER_APP_SETTING_VAL.
	 *
	 *  Called when the response for GET_CURR_PLAYER_APP_SETTING_VAL is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param buf The response buffer containing the GET_CURR_PLAYER_APP_SETTING_VAL
	 *            payload returned by the TG.
	 *            The application can parse this payload according to the format
	 *            defined in @ref bt_avrcp_get_curr_player_app_setting_val_rsp.
	 *            If status is in the range BT_AVRCP_STATUS_INVALID_COMMAND to
	 *            BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, and is not equal to
	 *            BT_AVRCP_STATUS_OPERATION_COMPLETED, it indicates that the AVRCP response
	 *            code is an AV/C REJECTED response, and buf is NULL.
	 *            Note that all multi-octet fields are encoded in big-endian format.
	 */
	void (*get_curr_player_app_setting_val)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
						struct net_buf *buf);

	/** @brief Callback for PDU ID SET_PLAYER_APP_SETTING_VAL.
	 *
	 *  Called when the response for SET_PLAYER_APP_SETTING_VAL is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 */
	void (*set_player_app_setting_val)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status);

	/** @brief Callback for PDU ID GET_PLAYER_APP_SETTING_ATTR_TEXT.
	 *
	 *  Called when the response for GET_PLAYER_APP_SETTING_ATTR_TEXT is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param buf The response buffer containing the GET_PLAYER_APP_SETTING_ATTR_TEXT
	 *            payload returned by the TG, formatted as
	 *            @ref bt_avrcp_get_player_app_setting_attr_text_rsp.
	 *            If status is in the range BT_AVRCP_STATUS_INVALID_COMMAND to
	 *            BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, and is not equal to
	 *            BT_AVRCP_STATUS_OPERATION_COMPLETED, it indicates that the AVRCP response
	 *            code is an AV/C REJECTED response, and buf is NULL.
	 *            Note that all multi-octet fields are encoded in big-endian format.
	 */
	void (*get_player_app_setting_attr_text)(struct bt_avrcp_ct *ct, uint8_t tid,
						 uint8_t status, struct net_buf *buf);

	/** @brief Callback for PDU ID GET_PLAYER_APP_SETTING_VAL_TEXT.
	 *
	 *  Called when the response for GET_PLAYER_APP_SETTING_VAL_TEXT is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param buf The response buffer containing the GET_PLAYER_APP_SETTING_VAL_TEXT
	 *            payload returned by the TG, formatted as
	 *            @ref bt_avrcp_get_player_app_setting_val_text_rsp.
	 *            If status is in the range BT_AVRCP_STATUS_INVALID_COMMAND to
	 *            BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, and is not equal to
	 *            BT_AVRCP_STATUS_OPERATION_COMPLETED, it indicates that the AVRCP response
	 *            code is an AV/C REJECTED response, and buf is NULL.
	 *            Note that all multi-octet fields are encoded in big-endian format.
	 */
	void (*get_player_app_setting_val_text)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
						struct net_buf *buf);

	/** @brief Callback for PDU ID INFORM_DISPLAYABLE_CHAR_SET.
	 *
	 *  Called when the response for INFORM_DISPLAYABLE_CHAR_SET is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 */
	void (*inform_displayable_char_set)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status);

	/** @brief Callback for PDU ID INFORM_BATT_STATUS_OF_CT.
	 *
	 *  Called when the response for INFORM_BATT_STATUS_OF_CT is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 */
	void (*inform_batt_status_of_ct)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status);

	/** @brief Callback function for Set Absolute Volume response (CT).
	 *
	 *  Called when the Set Absolute Volume response is received from the TG.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the response.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param absolute_volume The absolute volume value (0x00-0x7F).
	 */
	void (*set_absolute_volume)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
				   uint8_t absolute_volume);

	/** @brief Callback for PDU ID GET_ELEMENT_ATTRS.
	 *
	 *  Called when the response for GET_ELEMENT_ATTRS is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param buf The response buffer containing the GET_ELEMENT_ATTRS payload
	 *            returned by the TG, formatted as
	 *            @ref bt_avrcp_get_element_attrs_rsp.
	 *            If status is in the range BT_AVRCP_STATUS_INVALID_COMMAND to
	 *            BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, and is not equal to
	 *            BT_AVRCP_STATUS_OPERATION_COMPLETED, it indicates that the AVRCP response
	 *            code is an AV/C REJECTED response, and buf is NULL.
	 *            Note that all multi-octet fields are encoded in big-endian format.
	 */
	void (*get_element_attrs)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
				  struct net_buf *buf);

	/** @brief Callback for PDU ID GET_PLAY_STATUS.
	 *
	 *  Called when the response for GET_PLAY_STATUS is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation, @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 *  @param buf The response buffer containing the GET_PLAY_STATUS payload
	 *            returned by the TG, formatted as
	 *            @ref bt_avrcp_get_play_status_rsp.
	 *            If status is in the range BT_AVRCP_STATUS_INVALID_COMMAND to
	 *            BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED, and is not equal to
	 *            BT_AVRCP_STATUS_OPERATION_COMPLETED, it indicates that the AVRCP response
	 *            code is an AV/C REJECTED response, and buf is NULL.
	 *            Note that all multi-octet fields are encoded in big-endian format.
	 */
	void (*get_play_status)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status,
				struct net_buf *buf);

	/** @brief Callback for PDU ID SET_ADDRESSED_PLAYER.
	 *
	 *  Called when the response for SET_ADDRESSED_PLAYER is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation. @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 */
	void (*set_addressed_player)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status);

	/** @brief Callback for PDU ID PLAY_ITEM.
	 *
	 *  Called when the response for PLAY_ITEM is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation. @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 */
	void (*play_item)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status);

	/** @brief Callback for PDU ID ADD_TO_NOW_PLAYING.
	 *
	 *  Called when the response for ADD_TO_NOW_PLAYING is received.
	 *
	 *  @param ct AVRCP CT connection object.
	 *  @param tid The transaction label of the request.
	 *  @param status The status code returned by the TG, indicating the result of the
	 *               operation. @ref bt_avrcp_status_t. Typically corresponds to
	 *               BT_AVRCP_STATUS_* values such as BT_AVRCP_STATUS_SUCCESS
	 *               or BT_AVRCP_STATUS_INVALID_PARAMETER.
	 */
	void (*add_to_now_playing)(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t status);
};

/** @brief Connect AVRCP.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish AVRCP
 *  connection between devices.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return 0 in case of success or error code in case of error.
 *
 */
int bt_avrcp_connect(struct bt_conn *conn);

/** @brief Disconnect AVRCP.
 *
 *  This function close AVCTP L2CAP connection.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_disconnect(struct bt_conn *conn);

/**
 * @brief Allocate a net_buf for AVRCP PDU transmission, reserving headroom
 *        for AVRCP, AVRCTP, L2CAP, and ACL headers.
 *
 * This function allocates a buffer from the specified pool and reserves
 * sufficient headroom for protocol headers required by AVRCP over Bluetooth.
 *
 * @param pool The buffer pool to allocate from.
 *
 * @return A newly allocated net_buf with reserved headroom.
 */
struct net_buf *bt_avrcp_create_pdu(struct net_buf_pool *pool);

/**
 * @brief Allocate a net_buf for AVRCP Vendor-Dependent PDU transmission, reserving
 *        headroom for the Vendor PDU header in addition to AVRCP, AVCTP, L2CAP,
 *        and ACL headers.
 *
 * This function allocates a buffer from the specified pool and reserves
 * sufficient headroom for protocol headers required by AVRCP Vendor-Dependent
 * PDUs over Bluetooth.
 *
 * @param pool The buffer pool to allocate from.
 *
 * @return A newly allocated net_buf with reserved headroom.
 */
struct net_buf *bt_avrcp_create_vendor_pdu(struct net_buf_pool *pool);

/** @brief Connect AVRCP browsing channel.
 *
 *  This function is to be called after the AVRCP control channel is established.
 *  The API is to be used to establish AVRCP browsing connection between devices.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_browsing_connect(struct bt_conn *conn);

/** @brief Disconnect AVRCP browsing channel.
 *
 *  This function close AVCTP browsing channel L2CAP connection.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_browsing_disconnect(struct bt_conn *conn);

/** @brief Register callback.
 *
 *  Register AVRCP callbacks to monitor the state and interact with the remote device.
 *
 *  @param cb The AVRCP CT callback function.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_ct_register_cb(const struct bt_avrcp_ct_cb *cb);

/** @brief Get AVRCP Capabilities.
 *
 *  This function gets the capabilities supported by remote device.
 *
 *  @param ct The AVRCP CT instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param cap_id Specific capability requested, see @ref bt_avrcp_cap_t.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_ct_get_caps(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t cap_id);

/** @brief Get AVRCP Unit Info.
 *
 *  This function obtains information that pertains to the AV/C unit as a whole.
 *
 *  @param ct The AVRCP CT instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_ct_get_unit_info(struct bt_avrcp_ct *ct, uint8_t tid);

/** @brief Get AVRCP Subunit Info.
 *
 *  This function obtains information about the subunit(s) of an AV/C unit. A device with AVRCP
 *  may support other subunits than the panel subunit if other profiles co-exist in the device.
 *
 *  @param ct The AVRCP CT instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_ct_get_subunit_info(struct bt_avrcp_ct *ct, uint8_t tid);

/** @brief Send AVRCP Pass Through command.
 *
 *  This function send a pass through command to the remote device. Passsthhrough command is used
 *  to transfer user operation information from a CT to Panel subunit of TG.
 *
 *  @param ct The AVRCP CT instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param opid The user operation id, see @ref bt_avrcp_opid_t.
 *  @param state The button state, see @ref bt_avrcp_button_state_t.
 *  @param payload The payload of the pass through command. Should not be NULL if len is not zero.
 *  @param len The length of the payload.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_ct_passthrough(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t opid, uint8_t state,
			    const uint8_t *payload, uint8_t len);

/** @brief Set browsed player.
 *
 *  This function sets the browsed player on the remote device.
 *
 *  @param ct The AVRCP CT instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param player_id The player ID to be set as browsed player.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_ct_set_browsed_player(struct bt_avrcp_ct *ct, uint8_t tid, uint16_t player_id);

/** @brief Register for AVRCP changed notifications with callback.
 *
 *  This function registers for notifications from the target device.
 *  The notification response will be received through the provided callback function.
 *
 *  @param ct The AVRCP CT instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param event_id The event ID to register for, see @ref bt_avrcp_evt_t.
 *  @param interval The playback interval for position changed events.
 *         Other events will have this value set to 0 to ignore.
 *  @param cb The callback function to handle the changed notification response.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_ct_register_notification(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t event_id,
				      uint32_t interval, bt_avrcp_notify_changed_cb_t cb);

/** @brief Send AVRCP vendor dependent command for LIST_PLAYER_APP_SETTING_ATTRS.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_list_player_app_setting_attrs(struct bt_avrcp_ct *ct, uint8_t tid);

/** @brief Send AVRCP vendor dependent command for LIST_PLAYER_APP_SETTING_VALS.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param attr_id Player application setting attribute ID for which the possible
 *        values should be listed (e.g., Equalizer, Repeat Mode, Shuffle, Scan).
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_list_player_app_setting_vals(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t attr_id);

/** @brief Send AVRCP vendor dependent command for GET_CURR_PLAYER_APP_SETTING_VAL.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The command buffer containing the GET_CURR_PLAYER_APP_SETTING_VAL
 *          request payload, formatted as
 *          @ref bt_avrcp_get_curr_player_app_setting_val_cmd.
 *          Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_get_curr_player_app_setting_val(struct bt_avrcp_ct *ct, uint8_t tid,
						struct net_buf *buf);

/** @brief Send AVRCP vendor dependent command for SET_PLAYER_APP_SETTING_VAL.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The command buffer containing the SET_PLAYER_APP_SETTING_VAL
 *            request payload, formatted as
 *            @ref bt_avrcp_set_player_app_setting_val_cmd.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_set_player_app_setting_val(struct bt_avrcp_ct *ct, uint8_t tid,
					   struct net_buf *buf);

/** @brief Send AVRCP vendor dependent command for GET_PLAYER_APP_SETTING_ATTR_TEXT.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The command buffer containing the GET_PLAYER_APP_SETTING_ATTR_TEXT
 *            request payload, formatted as
 *            @ref bt_avrcp_get_player_app_setting_attr_text_cmd.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_get_player_app_setting_attr_text(struct bt_avrcp_ct *ct, uint8_t tid,
						 struct net_buf *buf);

/** @brief Send AVRCP vendor dependent command for GET_PLAYER_APP_SETTING_VAL_TEXT.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The command buffer containing the GET_PLAYER_APP_SETTING_VAL_TEXT
 *            request payload, formatted as
 *            @ref bt_avrcp_get_player_app_setting_val_text_cmd.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_get_player_app_setting_val_text(struct bt_avrcp_ct *ct, uint8_t tid,
						struct net_buf *buf);

/** @brief Send AVRCP vendor dependent command for INFORM_DISPLAYABLE_CHAR_SET.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The command buffer containing the INFORM_DISPLAYABLE_CHAR_SET
 *            request payload, formatted as
 *            @ref bt_avrcp_inform_displayable_char_set_cmd.
 *            Note that all multi-octet fields are encoded in big-endian format
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_inform_displayable_char_set(struct bt_avrcp_ct *ct, uint8_t tid,
					    struct net_buf *buf);

/** @brief Send AVRCP vendor dependent command for INFORM_BATT_STATUS_OF_CT.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param battery_status Battery status value @ref bt_avrcp_battery_status_t
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_inform_batt_status_of_ct(struct bt_avrcp_ct *ct, uint8_t tid,
					 uint8_t battery_status);

/** @brief Send Set Absolute Volume command (CT).
 *
 *  This function sends the Set Absolute Volume command to the TG.
 *
 *  @param ct The AVRCP CT instance.
 *  @param tid The transaction label of the command, valid from 0 to 15.
 *  @param absolute_volume The absolute volume value (0x00-0x7F).
 *
 * @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_ct_set_absolute_volume(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t absolute_volume);

/** @brief Send AVRCP vendor dependent command for GET_ELEMENT_ATTRS.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The command buffer containing the GET_ELEMENT_ATTRS
 *            request payload, formatted as
 *            @ref bt_avrcp_get_element_attrs_cmd.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_get_element_attrs(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf);

/** @brief Send AVRCP vendor dependent command for GET_PLAY_STATUS.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_get_play_status(struct bt_avrcp_ct *ct, uint8_t tid);

/** @brief Send AVRCP vendor dependent command for SET_ADDRESSED_PLAYER.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param player_id The player ID to be set as addressed player.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_set_addressed_player(struct bt_avrcp_ct *ct, uint8_t tid, uint16_t player_id);

/** @brief Send AVRCP vendor dependent command for PLAY_ITEM.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The command buffer containing the PLAY_ITEM
 *            request payload, formatted as
 *            @ref bt_avrcp_play_item_cmd.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_play_item(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf);

/** @brief Send AVRCP vendor dependent command for ADD_TO_NOW_PLAYING.
 *
 *  @param ct AVRCP CT connection object.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The command buffer containing the ADD_TO_NOW_PLAYING
 *            request payload, formatted as
 *            @ref bt_avrcp_add_to_now_playing_cmd.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_ct_add_to_now_playing(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf);

struct bt_avrcp_tg_cb {
	/** @brief An AVRCP TG connection has been established.
	 *
	 *  This callback notifies the application of an avrcp connection,
	 *  i.e., an AVCTP L2CAP connection.
	 *
	 *  @param conn Connection object.
	 *  @param tg AVRCP TG connection object.
	 */
	void (*connected)(struct bt_conn *conn, struct bt_avrcp_tg *tg);

	/** @brief An AVRCP TG connection has been disconnected.
	 *
	 *  This callback notifies the application that an avrcp connection
	 *  has been disconnected.
	 *
	 *  @param tg AVRCP TG connection object.
	 */
	void (*disconnected)(struct bt_avrcp_tg *tg);

	/** @brief Unit info request callback.
	 *
	 *  This callback is called whenever an AVRCP unit info is requested.
	 *
	 *  @param tid The transaction label of the request.
	 *  @param tg AVRCP TG connection object.
	 */
	void (*unit_info_req)(struct bt_avrcp_tg *tg, uint8_t tid);

	/** @brief Register notification request callback.
	 *
	 *  This callback is called whenever an AVRCP register notification is requested.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the request.
	 *  @param event_id The event ID that the CT wants to register for @ref bt_avrcp_evt_t.
	 *  @param interval The playback interval for position changed event.
	 *         other events will have this value set to 0 for ignoring.
	 */
	void (*register_notification)(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t event_id,
				      uint32_t interval);

	/** @brief Subunit Info Request callback.
	 *
	 *  This callback is called whenever an AVRCP subunit info is requested.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the request.
	 */
	void (*subunit_info_req)(struct bt_avrcp_tg *tg, uint8_t tid);

	/** @brief Get capabilities request callback.
	 *
	 *  This callback is called whenever an AVRCP get capabilities command is received.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the request.
	 *  @param cap_id The capability ID requested.
	 */
	void (*get_caps)(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t cap_id);

	/** @brief An AVRCP TG browsing connection has been established.
	 *
	 *  This callback notifies the application of an avrcp browsing connection,
	 *  i.e., an AVCTP browsing L2CAP connection.
	 *
	 *  @param conn Connection object.
	 *  @param tg AVRCP TG connection object.
	 */
	void (*browsing_connected)(struct bt_conn *conn, struct bt_avrcp_tg *tg);

	/** @brief An AVRCP TG browsing connection has been disconnected.
	 *
	 *  This callback notifies the application that an avrcp browsing connection
	 *  has been disconnected.
	 *
	 *  @param tg AVRCP TG connection object.
	 */
	void (*browsing_disconnected)(struct bt_avrcp_tg *tg);

	/** @brief Set browsed player request callback.
	 *
	 *  This callback is called whenever an AVRCP set browsed player request is received.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the request.
	 *  @param player_id The player ID to be set as browsed player.
	 */
	void (*set_browsed_player_req)(struct bt_avrcp_tg *tg, uint8_t tid, uint16_t player_id);

	/** @brief Pass Through command request callback.
	 *
	 *  This callback is called whenever an AVRCP Pass Through command is request.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the request.
	 *  @param buf The buffer containing the PASS THROUGH command payload.
	 *             The application can parse this payload according to the format defined
	 *             in @ref bt_avrcp_passthrough_rsp. Note that the data is encoded
	 *             in big-endian format.
	 */
	void (*passthrough_req)(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_ATTRS.
	 *
	 *  Called when the TG receives a vendor dependent command LIST_PLAYER_APP_SETTING_ATTRS.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 */
	void (*list_player_app_setting_attrs)(struct bt_avrcp_tg *tg, uint8_t tid);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_VALS.
	 *
	 *  Called when the TG receives a vendor dependent command for LIST_PLAYER_APP_SETTING_VALS.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param attr_id Player application setting attribute ID for which the possible
	 *                 values should be listed (e.g., Equalizer, Repeat Mode, Shuffle, Scan).
	 */
	void (*list_player_app_setting_vals)(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t attr_id);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_GET_CURR_PLAYER_APP_SETTING_VAL.
	 *
	 *  Called when the TG receives a vendor dependent command GET_CURR_PLAYER_APP_SETTING_VAL.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param buf The buffer containing the GET_CURR_PLAYER_APP_SETTING_VAL command payload,
	 *             formatted as @ref bt_avrcp_get_curr_player_app_setting_val_cmd.
	 *             The application should parse fields in big-endian order.
	 */
	void (*get_curr_player_app_setting_val)(struct bt_avrcp_tg *tg, uint8_t tid,
						struct net_buf *buf);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_SET_PLAYER_APP_SETTING_VAL.
	 *
	 *  Called when the TG receives a vendor dependent command for SET_PLAYER_APP_SETTING_VAL.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param buf The buffer containing the SET_PLAYER_APP_SETTING_VAL command payload,
	 *             formatted as @ref bt_avrcp_set_player_app_setting_val_cmd.
	 *             The application should parse fields in big-endian order.
	 */
	void (*set_player_app_setting_val)(struct bt_avrcp_tg *tg, uint8_t tid,
					   struct net_buf *buf);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_ATTR_TEXT.
	 *
	 *  Called when the TG receives a vendor dependent command GET_PLAYER_APP_SETTING_ATTR_TEXT.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param buf The buffer containing the GET_PLAYER_APP_SETTING_VAL_TEXT command payload,
	 *            formatted as @ref bt_avrcp_get_player_app_setting_val_text_cmd.
	 *            The application should parse fields in big-endian order.
	 */
	void (*get_player_app_setting_attr_text)(struct bt_avrcp_tg *tg, uint8_t tid,
						 struct net_buf *buf);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_VAL_TEXT.
	 *
	 *  Called when the TG receives a vendor dependent command GET_PLAYER_APP_SETTING_VAL_TEXT.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param buf The buffer containing the GET_PLAYER_APP_SETTING_VAL_TEXT command payload,
	 *            formatted as @ref bt_avrcp_get_player_app_setting_val_text_cmd.
	 *            The application should parse fields in big-endian order.
	 */
	void (*get_player_app_setting_val_text)(struct bt_avrcp_tg *tg, uint8_t tid,
						struct net_buf *buf);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHAR_SET.
	 *
	 *  Called when the TG receives a vendor dependent command for INFORM_DISPLAYABLE_CHAR_SET.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param buf The buffer containing the INFORM_DISPLAYABLE_CHAR_SET command payload,
	 *            formatted as @ref bt_avrcp_inform_displayable_char_set_cmd.
	 *            The application should parse fields in big-endian order.
	 */
	void (*inform_displayable_char_set)(struct bt_avrcp_tg *tg, uint8_t tid,
					    struct net_buf *buf);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_INFORM_BATT_STATUS_OF_CT.
	 *
	 *  Called when the TG receives a vendor dependent command for INFORM_BATT_STATUS_OF_CT.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param battery_status Battery status value @ref bt_avrcp_battery_status_t
	 */
	void (*inform_batt_status_of_ct)(struct bt_avrcp_tg *tg, uint8_t tid,
					 uint8_t battery_status);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRS.
	 *
	 *  Called when the TG receives a vendor dependent command for GET_ELEMENT_ATTRS.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param buf The buffer containing the GET_ELEMENT_ATTRS command payload,
	 *            formatted as @ref bt_avrcp_get_element_attrs_cmd.
	 *            The application should parse fields in big-endian order.
	 */
	void (*get_element_attrs)(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf);

	/** @brief Callback function for Set Absolute Volume command (TG).
	 *
	 *  Called when the Set Absolute Volume command is received from the CT.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param absolute_volume The absolute volume value (0x00-0x7F).
	 */
	void (*set_absolute_volume)(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t absolute_volume);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_GET_PLAY_STATUS.
	 *
	 *  Called when the TG receives a vendor dependent command for GET_PLAY_STATUS.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 */
	void (*get_play_status)(struct bt_avrcp_tg *tg, uint8_t tid);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER.
	 *
	 *  Called when the TG receives a vendor dependent command for SET_ADDRESSED_PLAYER.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param player_id The player ID to be set as addressed player.
	 */
	void (*set_addressed_player)(struct bt_avrcp_tg *tg, uint8_t tid, uint16_t player_id);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_PLAY_ITEM.
	 *
	 *  Called when the TG receives a vendor dependent command for PLAY_ITEM.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param buf The buffer containing the PLAY_ITEM command payload,
	 *            formatted as @ref bt_avrcp_play_item_cmd.
	 *            The application should parse fields in big-endian order.
	 */
	void (*play_item)(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf);

	/** @brief Callback for PDU ID BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING.
	 *
	 *  Called when the TG receives a vendor dependent command for ADD_TO_NOW_PLAYING.
	 *
	 *  @param tg AVRCP TG connection object.
	 *  @param tid The transaction label of the command.
	 *  @param buf The buffer containing the ADD_TO_NOW_PLAYING command payload,
	 *            formatted as @ref bt_avrcp_add_to_now_playing_cmd.
	 *            The application should parse fields in big-endian order.
	 */
	void (*add_to_now_playing)(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf);
};

/** @brief Register callback.
 *
 *  Register AVRCP callbacks to monitor the state and interact with the remote device.
 *
 *  @param cb The AVRCP TG callback function.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_tg_register_cb(const struct bt_avrcp_tg_cb *cb);

/** @brief Send the unit info response.
 *
 *  This function is called by the application to send the unit info response.
 *
 *  @param tg The AVRCP TG instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param rsp The response for UNIT INFO command.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_tg_send_unit_info_rsp(struct bt_avrcp_tg *tg, uint8_t tid,
				   struct bt_avrcp_unit_info_rsp *rsp);

/** @brief Send the subunit info response.
 *
 *  This function is called by the application to send the subunit info response.
 *
 *  @param tg The AVRCP TG instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_tg_send_subunit_info_rsp(struct bt_avrcp_tg *tg, uint8_t tid);

/** @brief Send GET_CAPABILITIES response.
 *
 *  This function is called by the application to send the GET_CAPABILITIES response.
 *
 *  @param tg The AVRCP TG instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param buf The response buffer containing the GET_CAPS payload,
 *            formatted as @ref bt_avrcp_get_caps_rsp.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_tg_get_caps(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status, struct net_buf *buf);

/** @brief Send notification response.
 *
 * This function sends a notification response from the AVRCP Target (TG) to
 * the Controller (CT) for a previously registered event. The response can be:
 *
 * - **INTERIM**: Sent on the first call for the given @p event_id to indicate
 *   that the event is being monitored.
 * - **CHANGED**: Sent on the next call for the same @p event_id when the event
 *   state has changed.
 *
 *  @param tg The AVRCP TG instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param event_id The AVRCP event ID for which the notification is sent, @ref bt_avrcp_evt_t.
 *  @param data Pointer to an bt_avrcp_event_data structure containing the event-specific
 *             data. The content of the union depends on the event_id.
 *
 *  @return 0 in case of success or error code in case of error.
 *
 * @note
 * - The first successful call for a given @p event_id sends an INTERIM response.
 * - The next successful call for the same @p event_id sends a CHANGED response.
 * - If @p status is BT_AVRCP_STATUS_NOT_IMPLEMENTED, a NOT_IMPLEMENTED response is sent.
 * - If @p status is any other non-SUCCESS value, a REJECTED response is sent.
 */
int bt_avrcp_tg_notification(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status, uint8_t event_id,
			     struct bt_avrcp_event_data *data);

/** @brief Send the set browsed player response.
 *
 *  This function is called by the application to send the set browsed player response.
 *
 *  @param tg The AVRCP TG instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param buf The response buffer containing the set browsed player response data.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_tg_send_set_browsed_player_rsp(struct bt_avrcp_tg *tg, uint8_t tid,
					    struct net_buf *buf);

/** @brief Send AVRCP Pass Through response.
 *
 *  This function is called by the application to send the Pass Through response.
 *
 *  @param tg The AVRCP TG instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param result The response code, see @ref bt_avrcp_rsp_t, can support
 *                0x8(NOT_IMPLEMENTED), 0x9 (ACCEPTED), 0xA (REJECTED)
 *  @param buf The buffer containing the PASS THROUGH command payload.
 *             The application can construct this payload according to the format defined
 *             in @ref bt_avrcp_passthrough_rsp. Note that the data is encoded
 *             in big-endian format.
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_tg_send_passthrough_rsp(struct bt_avrcp_tg *tg, uint8_t tid, bt_avrcp_rsp_t result,
				     struct net_buf *buf);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_ATTRS.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param buf The response buffer containing the LIST_PLAYER_APP_SETTING_ATTRS payload,
 *            formatted as @ref bt_avrcp_list_app_setting_attr_rsp.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_list_player_app_setting_attrs(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
					      struct net_buf *buf);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_VALS.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param buf The response buffer containing the LIST_PLAYER_APP_SETTING_VALS payload,
 *             formatted as @ref bt_avrcp_list_player_app_setting_vals_rsp.
 *             Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_list_player_app_setting_vals(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
					     struct net_buf *buf);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_GET_CURR_PLAYER_APP_SETTING_VAL.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param buf The response buffer containing the GET_CURR_PLAYER_APP_SETTING_VAL payload,
 *             formatted as @ref bt_avrcp_get_curr_player_app_setting_val_rsp.
 *             Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_get_curr_player_app_setting_val(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
						struct net_buf *buf);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_SET_PLAYER_APP_SETTING_VAL.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_set_player_app_setting_val(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_ATTR_TEXT.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param buf The response buffer containing the GET_PLAYER_APP_SETTING_ATTR_TEXT payload,
 *             formatted as @ref bt_avrcp_get_player_app_setting_attr_text_rsp.
 *             Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_get_player_app_setting_attr_text(struct bt_avrcp_tg *tg, uint8_t tid,
						 uint8_t status, struct net_buf *buf);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_VAL_TEXT.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param buf The response buffer containing the GET_PLAYER_APP_SETTING_VAL_TEXT payload,
 *            formatted as @ref bt_avrcp_get_player_app_setting_val_text_rsp.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_get_player_app_setting_val_text(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
						struct net_buf *buf);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHAR_SET.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_inform_displayable_char_set(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_INFORM_BATT_STATUS_OF_CT.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_inform_batt_status_of_ct(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status);

/** @brief Send Set Absolute Volume response (TG).
 *
 *  This function sends the Set Absolute Volume response to the CT.
 *
 *  @param tg The AVRCP TG instance.
 *  @param tid The transaction label of the response, valid from 0 to 15.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param absolute_volume The absolute volume value (0x00-0x7F).
 *
 *  @return 0 in case of success or error code in case of error.
 */
int bt_avrcp_tg_absolute_volume(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
				uint8_t absolute_volume);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRS.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param buf The response buffer containing the GET_ELEMENT_ATTRS payload,
 *            formatted as @ref bt_avrcp_get_element_attrs_rsp.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 * @return 0 on success or error code.
 */
int bt_avrcp_tg_get_element_attrs(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
				  struct net_buf *buf);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_GET_PLAY_STATUS.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *  @param buf The response buffer containing the GET_PLAY_STATUS payload,
 *            formatted as @ref bt_avrcp_get_play_status_rsp.
 *            Note that all multi-octet fields are encoded in big-endian format.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_get_play_status(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
				struct net_buf *buf);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_set_addressed_player(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_PLAY_ITEM.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_play_item(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status);

/** @brief Send response for PDU ID BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING.
 *
 *  @param tg AVRCP TG connection object.
 *  @param tid The transaction label of the request.
 *  @param status Status code of the operation @ref bt_avrcp_status_t.
 *
 *  @return 0 on success or error code.
 */
int bt_avrcp_tg_add_to_now_playing(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status);
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AVRCP_H_ */
