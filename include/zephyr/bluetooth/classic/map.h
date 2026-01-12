/** @file
 *  @brief Message Access Profile header.
 */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MAP_H_

/**
 * @brief Message Access Profile (MAP)
 * @defgroup bt_map Message Access Profile (MAP)
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/goep.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief MAP mandatory supported features bitmask
 *
 * Minimum set of features that must be supported by all MAP implementations.
 * Includes notification registration, notification, browsing, uploading, and delete features.
 */
#define BT_MAP_MANDATORY_SUPPORTED_FEATURES (0x0000001FU)

/** @brief MAP Message Access Server (MAS) UUID
 *
 * 128-bit UUID for the Message Access Server service.
 * Used to identify MAS service in SDP records and OBEX connections.
 */
#define BT_MAP_UUID_MAS                                                                            \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0xBB582B40, 0x420C, 0x11DB, 0xB0DE, 0x0800200C9A66))

/** @brief MAP Message Notification Server (MNS) UUID
 *
 * 128-bit UUID for the Message Notification Server service.
 * Used to identify MNS service in SDP records and OBEX connections.
 */
#define BT_MAP_UUID_MNS                                                                            \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0xBB582B41, 0x420C, 0x11DB, 0xB0DE, 0x0800200C9A66))

/** @brief Filler byte for padding
 *
 * Single byte used as padding in MAP operations where a body is required
 * but no actual data needs to be transmitted.
 */
#define BT_MAP_FILLER_BYTE "0"

/** @brief Type header for send event operation */
#define BT_MAP_HDR_TYPE_SEND_EVENT         "x-bt/MAP-event-report"
/** @brief Type header for set notification registration operation */
#define BT_MAP_HDR_TYPE_SET_NTF_REG        "x-bt/MAP-NotificationRegistration"
/** @brief Type header for get folder listing operation */
#define BT_MAP_HDR_TYPE_GET_FOLDER_LISTING "x-obex/folder-listing"
/** @brief Type header for get message listing operation */
#define BT_MAP_HDR_TYPE_GET_MSG_LISTING    "x-bt/MAP-msg-listing"
/** @brief Type header for get message operation */
#define BT_MAP_HDR_TYPE_GET_MSG            "x-bt/message"
/** @brief Type header for set message status operation */
#define BT_MAP_HDR_TYPE_SET_MSG_STATUS     "x-bt/messageStatus"
/** @brief Type header for push message operation */
#define BT_MAP_HDR_TYPE_PUSH_MSG           "x-bt/message"
/** @brief Type header for update inbox operation */
#define BT_MAP_HDR_TYPE_UPDATE_INBOX       "x-bt/MAP-messageUpdate"
/** @brief Type header for get MAS instance info operation */
#define BT_MAP_HDR_TYPE_GET_MAS_INST_INFO  "x-bt/MASInstanceInformation"
/** @brief Type header for set owner status operation */
#define BT_MAP_HDR_TYPE_SET_OWNER_STATUS   "x-bt/ownerStatus"
/** @brief Type header for get owner status operation */
#define BT_MAP_HDR_TYPE_GET_OWNER_STATUS   "x-bt/ownerStatus"
/** @brief Type header for get conversation listing operation */
#define BT_MAP_HDR_TYPE_GET_CONVO_LISTING  "x-bt/MAP-convo-listing"
/** @brief Type header for set notification filter operation */
#define BT_MAP_HDR_TYPE_SET_NTF_FILTER     "x-bt/MAP-notification-filter"

/** @brief MAP supported features
 *
 * Feature flags indicating optional MAP capabilities.
 * These are exchanged during SDP discovery or MAP connection establishment.
 */
enum __packed bt_map_supported_features {
	/** Notification registration feature */
	BT_MAP_NTF_REG_FEATURE = BIT(0U),
	/** Notification feature */
	BT_MAP_NTF_FEATURE = BIT(1U),
	/** Browsing feature */
	BT_MAP_BROWSING_FEATURE = BIT(2U),
	/** Uploading feature */
	BT_MAP_UPLOADING_FEATURE = BIT(3U),
	/** Delete feature */
	BT_MAP_DELETE_FEATURE = BIT(4U),
	/** Instance information feature */
	BT_MAP_INST_INFO_FEATURE = BIT(5U),
	/** Extended event report version 1.1 */
	BT_MAP_EXT_EVENT_REPORT_1_1 = BIT(6U),
	/** Extended event version 1.2 */
	BT_MAP_EXT_EVENT_VERSION_1_2 = BIT(7U),
	/** Message format version 1.1 */
	BT_MAP_MSG_FORMAT_VERSION_1_1 = BIT(8U),
	/** Message listing format version 1.1 */
	BT_MAP_MSG_LISTING_FORMAT_VERSION_1_1 = BIT(9U),
	/** Persistent message handle */
	BT_MAP_PERSISTENT_MSG_HANDLE = BIT(10U),
	/** Database identifier */
	BT_MAP_DATABASE_ID = BIT(11U),
	/** Folder version counter */
	BT_MAP_FOLDER_VERSION_CNTR = BIT(12U),
	/** Conversation version counter */
	BT_MAP_CONVO_VERSION_CNTR = BIT(13U),
	/** Participant presence change notification */
	BT_MAP_PARTICIPANT_PRESENCE_CHANGE_NTF = BIT(14U),
	/** Participant chat state change notification */
	BT_MAP_PARTICIPANT_CHAT_STATE_CHANGE_NTF = BIT(15U),
	/** PBAP contact cross reference */
	BT_MAP_PBAP_CONTACT_CROSS_REF = BIT(16U),
	/** Notification filtering */
	BT_MAP_NTF_FILTERING = BIT(17U),
	/** UTC offset timestamp format */
	BT_MAP_UTC_OFFSET_TIMESTAMP_FORMAT = BIT(18U),
	/** Supported features in connect request */
	BT_MAP_SUPPORTED_FEATURES_CONNECT_REQ = BIT(19U),
	/** Conversation listing */
	BT_MAP_CONVO_LISTING = BIT(20U),
	/** Owner status */
	BT_MAP_OWNER_STATUS = BIT(21U),
	/** Message forwarding */
	BT_MAP_MSG_FORWARDING = BIT(22U),
};

/** @brief MAP supported message types
 *
 * Bitmask indicating which message types are supported by a MAS instance.
 */
enum __packed bt_map_supported_msg_type {
	/** Email message type */
	BT_MAP_MSG_TYPE_EMAIL = BIT(0U),
	/** SMS GSM message type */
	BT_MAP_MSG_TYPE_SMS_GSM = BIT(1U),
	/** SMS CDMA message type */
	BT_MAP_MSG_TYPE_SMS_CDMA = BIT(2U),
	/** MMS message type */
	BT_MAP_MSG_TYPE_MMS = BIT(3U),
	/** Instant messaging type */
	BT_MAP_MSG_TYPE_IM = BIT(4U),
};

/** @brief MAP set folder operation flags
 *
 * Flags controlling folder navigation behavior in SetFolder operations.
 */
enum __packed bt_map_set_folder_flags {
	/** Navigate to root folder */
	BT_MAP_SET_FOLDER_FLAGS_ROOT = BT_OBEX_SETPATH_FLAG_NO_CREATE,
	/** Navigate down to child folder */
	BT_MAP_SET_FOLDER_FLAGS_DOWN = BT_OBEX_SETPATH_FLAG_NO_CREATE,
	/** Navigate up to parent folder */
	BT_MAP_SET_FOLDER_FLAGS_UP = BT_OBEX_SETPATH_FLAG_BACKUP | BT_OBEX_SETPATH_FLAG_NO_CREATE,
};

/** @brief MAP application parameter tag identifiers
 *
 * Tag IDs for application parameters used in MAP operations.
 * These are used in OBEX Application Parameters header to pass operation-specific data.
 */
enum __packed bt_map_appl_param_tag_id {
	/** Maximum list count (2 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_MAX_LIST_COUNT = 0x01U,
	/** List start offset (2 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_LIST_START_OFFSET = 0x02U,
	/** Filter message type (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_MESSAGE_TYPE = 0x03U,
	/** Filter period begin (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_PERIOD_BEGIN = 0x04U,
	/** Filter period end (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_PERIOD_END = 0x05U,
	/** Filter read status (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_READ_STATUS = 0x06U,
	/** Filter recipient (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_RECIPIENT = 0x07U,
	/** Filter originator (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_ORIGINATOR = 0x08U,
	/** Filter priority (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_PRIORITY = 0x09U,
	/** Attachment (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_ATTACHMENT = 0x0AU,
	/** Transparent (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_TRANSPARENT = 0x0BU,
	/** Retry (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_RETRY = 0x0CU,
	/** New message (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_NEW_MESSAGE = 0x0DU,
	/** Notification status (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_NOTIFICATION_STATUS = 0x0EU,
	/** MAS instance ID (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_MAS_INSTANCE_ID = 0x0FU,
	/** Parameter mask (4 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_PARAMETER_MASK = 0x10U,
	/** Folder listing size (2 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_FOLDER_LISTING_SIZE = 0x11U,
	/** Listing size (2 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_LISTING_SIZE = 0x12U,
	/** Subject length (variable, max 255 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_SUBJECT_LENGTH = 0x13U,
	/** Charset (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_CHARSET = 0x14U,
	/** Fraction request (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_FRACTION_REQUEST = 0x15U,
	/** Fraction deliver (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_FRACTION_DELIVER = 0x16U,
	/** Status indicator (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_STATUS_INDICATOR = 0x17U,
	/** Status value (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_STATUS_VALUE = 0x18U,
	/** MSE time (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_MSE_TIME = 0x19U,
	/** Database identifier (variable, max 32 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_DATABASE_IDENTIFIER = 0x1AU,
	/** Conversation listing version counter (variable, max 32 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_CONV_LISTING_VER_CNTR = 0x1BU,
	/** Presence availability (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_PRESENCE_AVAILABILITY = 0x1CU,
	/** Presence text (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_PRESENCE_TEXT = 0x1DU,
	/** Last activity (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_LAST_ACTIVITY = 0x1EU,
	/** Filter last activity begin (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_LAST_ACTIVITY_BEGIN = 0x1FU,
	/** Filter last activity end (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_LAST_ACTIVITY_END = 0x20U,
	/** Chat state (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_CHAT_STATE = 0x21U,
	/** Conversation ID (variable, max 32 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_CONVERSATION_ID = 0x22U,
	/** Folder version counter (variable, max 32 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_FOLDER_VER_CNTR = 0x23U,
	/** Filter message handle (variable, max 16 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_MSG_HANDLE = 0x24U,
	/** Notification filter mask (4 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_NOTIFICATION_FILTER_MASK = 0x25U,
	/** Conversation parameter mask (4 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_CONV_PARAMETER_MASK = 0x26U,
	/** Owner UCI (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_OWNER_UCI = 0x27U,
	/** Extended data (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_EXTENDED_DATA = 0x28U,
	/** MAP supported features (4 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_MAP_SUPPORTED_FEATURES = 0x29U,
	/** Message handle (8 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_MESSAGE_HANDLE = 0x2AU,
	/** Modify text (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_MODIFY_TEXT = 0x2BU,
};

/** @brief MAP presence availability states
 *
 * Enumeration of possible presence states for the message owner.
 */
enum __packed bt_map_presence {
	/** Unknown presence state */
	BT_MAP_PRESENCE_UNKNOWN = 0,
	/** Offline */
	BT_MAP_PRESENCE_OFFLINE = 1,
	/** Online */
	BT_MAP_PRESENCE_ONLINE = 2,
	/** Away */
	BT_MAP_PRESENCE_AWAY = 3,
	/** Do not disturb */
	BT_MAP_PRESENCE_DO_NOT_DISTURB = 4,
	/** Busy */
	BT_MAP_PRESENCE_BUSY = 5,
	/** In a meeting */
	BT_MAP_PRESENCE_IN_A_MEETING = 6,
};

/** @brief MAP chat states
 *
 * Enumeration of possible chat states for the message owner.
 */
enum __packed bt_map_chat_state {
	/** Unknown chat state */
	BT_MAP_CHAT_STATE_UNKNOWN = 0,
	/** Inactive */
	BT_MAP_CHAT_STATE_INACTIVE = 1,
	/** Active */
	BT_MAP_CHAT_STATE_ACTIVE = 2,
	/** Composing */
	BT_MAP_CHAT_STATE_COMPOSING = 3,
	/** Paused composing */
	BT_MAP_CHAT_STATE_PAUSED_COMPOSING = 4,
	/** Gone */
	BT_MAP_CHAT_STATE_GONE = 5,
};

/** @brief MAP message extended data types
 *
 * Types of extended metadata that can be associated with messages.
 */
enum __packed bt_map_msg_extended_data {
	/** Number of Facebook likes */
	BT_MAP_MSG_EXTENDED_DATA_FACEBOOK_LIKES = 0,
	/** Number of Twitter followers */
	BT_MAP_MSG_EXTENDED_DATA_TWITTER_FOLLOWERS = 1,
	/** Number of Twitter retweets */
	BT_MAP_MSG_EXTENDED_DATA_TWITTER_RETWEETS = 2,
	/** Number of Google +1s */
	BT_MAP_MSG_EXTENDED_DATA_GOOGLE_1S = 3,
};

/** @brief MAP transport connection states
 *
 * States of the underlying transport layer (RFCOMM or L2CAP) connection.
 */
enum __packed bt_map_transport_state {
	/** Transport is disconnected */
	BT_MAP_TRANSPORT_STATE_DISCONNECTED = 0,
	/** Transport is connecting */
	BT_MAP_TRANSPORT_STATE_CONNECTING = 1,
	/** Transport is connected */
	BT_MAP_TRANSPORT_STATE_CONNECTED = 2,
	/** Transport is disconnecting */
	BT_MAP_TRANSPORT_STATE_DISCONNECTING = 3,
};

/** @brief MAP connection states
 *
 * States of the MAP OBEX connection layer.
 */
enum __packed bt_map_state {
	/** Connection is disconnected */
	BT_MAP_STATE_DISCONNECTED = 0,
	/** Connection is being established */
	BT_MAP_STATE_CONNECTING = 1,
	/** Connection is established */
	BT_MAP_STATE_CONNECTED = 2,
	/** Connection is being terminated */
	BT_MAP_STATE_DISCONNECTING = 3,
};

/** @brief Forward declaration of MAP Client MAS structure */
struct bt_map_mce_mas;

/** @brief Forward declaration of MAP Client MNS structure */
struct bt_map_mce_mns;

/** @brief MAP Client MNS RFCOMM server
 *
 * This structure represents an RFCOMM server for MAP Client MNS.
 */
struct bt_map_mce_mns_rfcomm_server {
	/** GOEP RFCOMM transport server - underlying transport layer */
	struct bt_goep_transport_rfcomm_server server;
	/** Accept callback for incoming connections
	 *
	 * Called when a remote MSE attempts to connect to the MNS server.
	 *
	 * @param conn Bluetooth connection object
	 * @param server RFCOMM server instance that received the connection
	 * @param mce_mns Output parameter to store the allocated MNS instance
	 *
	 * @return 0 on success, negative error code to reject the connection
	 */
	int (*accept)(struct bt_conn *conn, struct bt_map_mce_mns_rfcomm_server *server,
		      struct bt_map_mce_mns **mce_mns);
};

/** @brief MAP Client MNS L2CAP server
 *
 * This structure represents an L2CAP server for MAP Client MNS.
 */
struct bt_map_mce_mns_l2cap_server {
	/** GOEP L2CAP transport server - underlying transport layer */
	struct bt_goep_transport_l2cap_server server;
	/** Accept callback for incoming connections
	 *
	 * Called when a remote MSE attempts to connect to the MNS server.
	 *
	 * @param conn Bluetooth connection object
	 * @param server L2CAP server instance that received the connection
	 * @param mce_mns Output parameter to store the allocated MNS instance
	 *
	 * @return 0 on success, negative error code to reject the connection
	 */
	int (*accept)(struct bt_conn *conn, struct bt_map_mce_mns_l2cap_server *server,
		      struct bt_map_mce_mns **mce_mns);
};

/** @brief MAP Client MAS callbacks
 *
 * Callbacks for MAP Client MAS events.
 */
struct bt_map_mce_mas_cb {
	/** RFCOMM transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object
	 * @param mce_mas MAS client instance
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas);

	/** RFCOMM transport disconnected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is closed.
	 * All pending operations are cancelled.
	 *
	 * @param mce_mas MAS client instance
	 */
	void (*rfcomm_disconnected)(struct bt_map_mce_mas *mce_mas);

	/** L2CAP transport connected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object
	 * @param mce_mas MAS client instance
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas);

	/** L2CAP transport disconnected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is closed.
	 * All pending operations are cancelled.
	 *
	 * @param mce_mas MAS client instance
	 */
	void (*l2cap_disconnected)(struct bt_map_mce_mas *mce_mas);

	/** Connected callback
	 *
	 * Called when OBEX CONNECT response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_SUCCESS on success)
	 * @param version OBEX protocol version supported by server
	 * @param mopl Maximum OBEX packet length supported by server
	 * @param buf Buffer containing additional response headers (connection ID, etc.)
	 */
	void (*connected)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, uint8_t version,
			  uint16_t mopl, struct net_buf *buf);

	/** Disconnected callback
	 *
	 * Called when OBEX DISCONNECT response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*disconnected)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** Abort callback
	 *
	 * Called when OBEX ABORT response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*abort)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** Set notification registration callback
	 *
	 * Called when SetNotificationRegistration response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*set_ntf_reg)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** Set folder callback
	 *
	 * Called when SetFolder response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*set_folder)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** Get folder listing callback
	 *
	 * Called when GetFolderListing response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_CONTINUE for partial data)
	 * @param buf Buffer containing folder listing XML data and headers
	 */
	void (*get_folder_listing)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				   struct net_buf *buf);

	/** Get message listing callback
	 *
	 * Called when GetMessageListing response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_CONTINUE for partial data)
	 * @param buf Buffer containing message listing XML data and headers
	 */
	void (*get_msg_listing)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				struct net_buf *buf);

	/** Get message callback
	 *
	 * Called when GetMessage response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_CONTINUE for partial data)
	 * @param buf Buffer containing message bMessage data and headers
	 */
	void (*get_msg)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** Set message status callback
	 *
	 * Called when SetMessageStatus response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*set_msg_status)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
			       struct net_buf *buf);

	/** Push message callback
	 *
	 * Called when PushMessage response is received.
	 * May be called multiple times if request is fragmented (BT_OBEX_RSP_CODE_CONTINUE).
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_CONTINUE for partial upload)
	 * @param buf Buffer containing message handle (on success) and headers
	 */
	void (*push_msg)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** Update inbox callback
	 *
	 * Called when UpdateInbox response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*update_inbox)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** Get MAS instance info callback
	 *
	 * Called when GetMASInstanceInformation response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_CONTINUE for partial data)
	 * @param buf Buffer containing instance information XML data and headers
	 */
	void (*get_mas_inst_info)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				  struct net_buf *buf);

	/** Set owner status callback
	 *
	 * Called when SetOwnerStatus response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*set_owner_status)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				 struct net_buf *buf);

	/** Get owner status callback
	 *
	 * Called when GetOwnerStatus response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_CONTINUE for partial data)
	 * @param buf Buffer containing owner status data and headers
	 */
	void (*get_owner_status)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				 struct net_buf *buf);

	/** Get conversation listing callback
	 *
	 * Called when GetConversationListing response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_CONTINUE for partial data)
	 * @param buf Buffer containing conversation listing XML data and headers
	 */
	void (*get_convo_listing)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				  struct net_buf *buf);

	/** Set notification filter callback
	 *
	 * Called when SetNotificationFilter response is received.
	 *
	 * @param mce_mas MAS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*set_ntf_filter)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
			       struct net_buf *buf);
};

/** @brief MAP Client MNS callbacks
 *
 * Callbacks for MAP Client MNS events.
 */
struct bt_map_mce_mns_cb {
	/** RFCOMM transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object
	 * @param mce_mns MNS server instance
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns);

	/** RFCOMM transport disconnected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is closed.
	 *
	 * @param mce_mns MNS server instance
	 */
	void (*rfcomm_disconnected)(struct bt_map_mce_mns *mce_mns);

	/** L2CAP transport connected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object
	 * @param mce_mns MNS server instance
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns);

	/** L2CAP transport disconnected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is closed.
	 *
	 * @param mce_mns MNS server instance
	 */
	void (*l2cap_disconnected)(struct bt_map_mce_mns *mce_mns);

	/** Connected callback
	 *
	 * Called when OBEX CONNECT request is received from remote MSE.
	 * Application should respond using bt_map_mce_mns_connect().
	 *
	 * @param mce_mns MNS server instance
	 * @param version OBEX protocol version requested by client
	 * @param mopl Maximum OBEX packet length requested by client
	 * @param buf Buffer containing request headers (target UUID, etc.)
	 */
	void (*connected)(struct bt_map_mce_mns *mce_mns, uint8_t version, uint16_t mopl,
			  struct net_buf *buf);

	/** Disconnected callback
	 *
	 * Called when OBEX DISCONNECT request is received.
	 * Application should respond using bt_map_mce_mns_disconnect().
	 *
	 * @param mce_mns MNS server instance
	 * @param buf Buffer containing request headers
	 */
	void (*disconnected)(struct bt_map_mce_mns *mce_mns, struct net_buf *buf);

	/** Abort callback
	 *
	 * Called when OBEX ABORT request is received.
	 * Application should respond using bt_map_mce_mns_abort().
	 *
	 * @param mce_mns MNS server instance
	 * @param buf Buffer containing request headers
	 */
	void (*abort)(struct bt_map_mce_mns *mce_mns, struct net_buf *buf);

	/** Send event callback
	 *
	 * Called when SendEvent request is received from remote MSE.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mce_mns_send_event().
	 *
	 * @param mce_mns MNS server instance
	 * @param final True if this is the final packet, false if more data follows
	 * @param buf Buffer containing event report XML data and headers
	 */
	void (*send_event)(struct bt_map_mce_mns *mce_mns, bool final, struct net_buf *buf);
};

/** @brief MAP Client MAS instance structure
 *
 * Represents a client connection to a Message Access Server.
 * Used for browsing folders and manipulating messages on the server.
 */
struct bt_map_mce_mas {
	/** Underlying GOEP instance */
	struct bt_goep goep;

	/** Callbacks */
	const struct bt_map_mce_mas_cb *_cb;

	/** Transport type */
	uint8_t _transport_type;

	/** Transport state (atomic) */
	atomic_t _transport_state;

	/** Connection ID */
	uint32_t _conn_id;

	/** Underlying OBEX client */
	struct bt_obex_client _client;

	/** Client state (atomic) */
	atomic_t _state;

	/** Pending response callback */
	void (*_rsp_cb)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** Current request function type */
	const char *_req_type;
};

/** @brief MAP Client MNS instance structure
 *
 * Represents a server that receives event notifications from a Message Notification Server.
 * The MCE acts as a server for the MNS connection.
 */
struct bt_map_mce_mns {
	/** Underlying GOEP instance */
	struct bt_goep goep;

	/** Callbacks */
	const struct bt_map_mce_mns_cb *_cb;

	/** Transport type */
	uint8_t _transport_type;

	/** Transport state (atomic) */
	atomic_t _transport_state;

	/** Connection ID */
	uint32_t _conn_id;

	/** Underlying OBEX server */
	struct bt_obex_server _server;

	/** Server state (atomic) */
	atomic_t _state;

	/** Pending request callback */
	void (*_req_cb)(struct bt_map_mce_mns *mce_mns, bool final, struct net_buf *buf);

	/** Operation type */
	const char *_optype;

	/** Operation code */
	uint8_t _opcode;
};

/** @brief Register callbacks for MAP Client MAS
 *
 * Registers callback functions to handle MAS events and responses.
 * Must be called before initiating any MAS operations.
 *
 * @param mce_mas MAS client instance
 * @param cb Pointer to callback structure (must remain valid)
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_cb_register(struct bt_map_mce_mas *mce_mas, const struct bt_map_mce_mas_cb *cb);

/** @brief Connect MAP Client MAS over RFCOMM
 *
 * Initiates RFCOMM transport connection to a remote MAS.
 * On success, transport_connected callback will be called.
 *
 * @param conn Bluetooth connection to remote device
 * @param mce_mas MAS client instance
 * @param channel RFCOMM server channel (from SDP discovery)
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_rfcomm_connect(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas,
				  uint8_t channel);

/** @brief Disconnect MAP Client MAS over RFCOMM
 *
 * Closes the RFCOMM transport connection.
 * On success, transport_disconnected callback will be called.
 *
 * @param mce_mas MAS client instance
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_rfcomm_disconnect(struct bt_map_mce_mas *mce_mas);

/** @brief Connect MAP Client MAS over L2CAP
 *
 * Initiates L2CAP transport connection to a remote MAS.
 * On success, transport_connected callback will be called.
 *
 * @param conn Bluetooth connection to remote device
 * @param mce_mas MAS client instance
 * @param psm L2CAP PSM (from SDP discovery)
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_l2cap_connect(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas,
				 uint16_t psm);

/** @brief Disconnect MAP Client MAS over L2CAP
 *
 * Closes the L2CAP transport connection.
 * On success, transport_disconnected callback will be called.
 *
 * @param mce_mas MAS client instance
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_l2cap_disconnect(struct bt_map_mce_mas *mce_mas);

/** @brief Create PDU for MAP Client MAS
 *
 * Allocates a buffer for building MAP request packets.
 * The buffer is sized appropriately for the negotiated MTU.
 *
 * @param mce_mas MAS client instance
 * @param pool Buffer pool to allocate from (NULL for default pool)
 *
 * @return Allocated buffer, or NULL on allocation failure
 */
struct net_buf *bt_map_mce_mas_create_pdu(struct bt_map_mce_mas *mce_mas,
					  struct net_buf_pool *pool);

/** @brief Send OBEX connect request
 *
 * Initiates OBEX session establishment with the MAS.
 * Must be called after transport connection is established.
 * On completion, connected callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param buf Buffer containing connect headers (target UUID, supported features, etc.)
 *            If NULL, a default connect request is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_connect(struct bt_map_mce_mas *mce_mas, struct net_buf *buf);

/** @brief Send OBEX disconnect request
 *
 * Initiates OBEX session termination.
 * On completion, disconnected callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param buf Buffer containing disconnect headers (connection ID, etc.)
 *            If NULL, a default disconnect request is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_disconnect(struct bt_map_mce_mas *mce_mas, struct net_buf *buf);

/** @brief Send OBEX abort request
 *
 * Aborts the currently ongoing operation.
 * On completion, abort callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param buf Buffer containing abort headers (connection ID, etc.)
 *            If NULL, a default abort request is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_abort(struct bt_map_mce_mas *mce_mas, struct net_buf *buf);

/** @brief Send set folder request
 *
 * Navigates the folder hierarchy on the MAS.
 * On completion, set_folder callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param flags Navigation flags (see @ref bt_map_set_folder_flags)
 * @param buf Buffer containing folder name (for DOWN) and headers.
 *            Must include connection ID.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_set_folder(struct bt_map_mce_mas *mce_mas, uint8_t flags, struct net_buf *buf);

/** @brief Send set notification registration request
 *
 * Enables or disables event notifications from the MAS.
 * On completion, set_ntf_reg callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param final True if this is a single-packet request
 * @param buf Buffer containing notification status and headers.
 *            Must include connection ID, type header, and application parameters.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_set_ntf_reg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send get folder listing request
 *
 * Retrieves list of subfolders in the current folder.
 * On completion, get_folder_listing callback will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance
 * @param final True if request is fragmented, or false
 * @param buf Buffer containing filter parameters and headers.
 *            Must include connection ID and type header for initial request.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_get_folder_listing(struct bt_map_mce_mas *mce_mas, bool final,
				      struct net_buf *buf);

/** @brief Send get message listing request
 *
 * Retrieves list of messages in the current folder.
 * On completion, get_msg_listing callback will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance
 * @param final True if request is fragmented, or false
 * @param buf Buffer containing folder name, filter parameters, and headers.
 *            Must include connection ID, type header, and name header for initial request.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_get_msg_listing(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send get message request
 *
 * Retrieves a specific message by handle.
 * On completion, get_msg callback will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance
 * @param final True if request is fragmented, or false
 * @param buf Buffer containing message handle, parameters, and headers.
 *            Must include connection ID, type header, and name header for initial request.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_get_msg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send set message status request
 *
 * Modifies the status of a message (read/deleted/etc).
 * On completion, set_msg_status callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param final True if this is a single-packet request
 * @param buf Buffer containing message handle, status parameters, and headers.
 *            Must include connection ID, type header, name header, and application parameters.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_set_msg_status(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send push message request
 *
 * Uploads a new message to the server.
 * On completion, push_msg callback will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance
 * @param final True if this is the last packet, false if more data follows
 * @param buf Buffer containing message data (bMessage format) and headers.
 *            Must include connection ID, type header, name header, and application parameters
 *            for initial request. Must include end-of-body header when final is true.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_push_msg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send update inbox request
 *
 * Requests the server to check for new messages.
 * On completion, update_inbox callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param final True if this is a single-packet request
 * @param buf Buffer containing headers.
 *            Must include connection ID and type header.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_update_inbox(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send get MAS instance info request
 *
 * Retrieves information about the MAS instance.
 * On completion, get_mas_inst_info callback will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance
 * @param final True if request is fragmented, or false
 * @param buf Buffer containing instance ID and headers.
 *            Must include connection ID, type header, and application parameters for initial
 * request. Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_get_mas_inst_info(struct bt_map_mce_mas *mce_mas, bool final,
				     struct net_buf *buf);

/** @brief Send set owner status request
 *
 * Sets the owner's presence and chat state.
 * On completion, set_owner_status callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param final True if this is a single-packet request
 * @param buf Buffer containing owner status data and headers.
 *            Must include connection ID, type header, and application parameters.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_set_owner_status(struct bt_map_mce_mas *mce_mas, bool final,
				    struct net_buf *buf);

/** @brief Send get owner status request
 *
 * Retrieves the owner's presence and chat state.
 * On completion, get_owner_status callback will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance
 * @param final True if request is fragmented, or false
 * @param buf Buffer containing headers.
 *            Must include connection ID and type header for initial request.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_get_owner_status(struct bt_map_mce_mas *mce_mas, bool final,
				    struct net_buf *buf);

/** @brief Send get conversation listing request
 *
 * Retrieves list of conversations.
 * On completion, get_convo_listing callback will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance
 * @param final True if request is fragmented, or false
 * @param buf Buffer containing filter parameters and headers.
 *            Must include connection ID and type header for initial request.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_get_convo_listing(struct bt_map_mce_mas *mce_mas, bool final,
				     struct net_buf *buf);

/** @brief Send set notification filter request
 *
 * Configures which event types to receive notifications for.
 * On completion, set_ntf_filter callback will be called.
 *
 * @param mce_mas MAS client instance
 * @param final True if this is a single-packet request
 * @param buf Buffer containing filter mask and headers.
 *            Must include connection ID, type header, and application parameters.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mas_set_ntf_filter(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Register callbacks for MAP Client MNS
 *
 * Registers callback functions to handle MNS events and requests.
 * Must be called before accepting any MNS connections.
 *
 * @param mce_mns MNS server instance
 * @param cb Pointer to callback structure (must remain valid)
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_cb_register(struct bt_map_mce_mns *mce_mns, const struct bt_map_mce_mns_cb *cb);

/** @brief Register MAP Client MNS RFCOMM server
 *
 * Registers an RFCOMM server to accept incoming MNS connections.
 * The server channel is allocated automatically or can be specified.
 *
 * @param server RFCOMM server structure with accept callback configured
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_rfcomm_register(struct bt_map_mce_mns_rfcomm_server *server);

/** @brief Disconnect MAP Client MNS over RFCOMM
 *
 * Closes the RFCOMM transport connection.
 * On success, transport_disconnected callback will be called.
 *
 * @param mce_mns MNS server instance
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_rfcomm_disconnect(struct bt_map_mce_mns *mce_mns);

/** @brief Register MAP Client MNS L2CAP server
 *
 * Registers an L2CAP server to accept incoming MNS connections.
 * The PSM is allocated automatically or can be specified.
 *
 * @param server L2CAP server structure with accept callback configured
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_l2cap_register(struct bt_map_mce_mns_l2cap_server *server);

/** @brief Disconnect MAP Client MNS over L2CAP
 *
 * Closes the L2CAP transport connection.
 * On success, transport_disconnected callback will be called.
 *
 * @param mce_mns MNS server instance
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_l2cap_disconnect(struct bt_map_mce_mns *mce_mns);

/** @brief Create PDU for MAP Client MNS
 *
 * Allocates a buffer for building MAP response packets.
 * The buffer is sized appropriately for the negotiated MTU.
 *
 * @param mce_mns MNS server instance
 * @param pool Buffer pool to allocate from (NULL for default pool)
 *
 * @return Allocated buffer, or NULL on failure
 */
struct net_buf *bt_map_mce_mns_create_pdu(struct bt_map_mce_mns *mce_mns,
					  struct net_buf_pool *pool);

/** @brief Send OBEX connect response
 *
 * Responds to an OBEX CONNECT request from the remote MSE.
 * Called from the connected callback.
 *
 * @param mce_mns MNS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS to accept)
 * @param buf Buffer containing response headers (WHO, connection ID, etc.)
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_connect(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send OBEX disconnect response
 *
 * Responds to an OBEX DISCONNECT request from the remote MSE.
 * Called from the disconnected callback.
 *
 * @param mce_mns MNS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS to accept)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_disconnect(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code,
			      struct net_buf *buf);

/** @brief Send OBEX abort response
 *
 * Responds to an OBEX ABORT request from the remote MSE.
 * Called from the abort callback.
 *
 * @param mce_mns MNS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS to accept)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_abort(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send event response
 *
 * Responds to a SendEvent request from the remote MSE.
 * Called from the send_event callback.
 *
 * @param mce_mns MNS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 BT_OBEX_RSP_CODE_SUCCESS for complete)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mce_mns_send_event(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code,
			      struct net_buf *buf);

/** @brief Forward declaration of MAP Server MAS structure */
struct bt_map_mse_mas;

/** @brief Forward declaration of MAP Server MNS structure */
struct bt_map_mse_mns;

/** @brief MAP Server MAS RFCOMM server
 *
 * This structure represents an RFCOMM server for MAP Server MAS.
 */
struct bt_map_mse_mas_rfcomm_server {
	/** GOEP RFCOMM transport server - underlying transport layer */
	struct bt_goep_transport_rfcomm_server server;

	/** Accept callback for incoming connections
	 *
	 * Called when a remote MCE attempts to connect to the MAS server.
	 *
	 * @param conn Bluetooth connection object
	 * @param server RFCOMM server instance that received the connection
	 * @param mse_mas Output parameter to store the allocated MAS instance
	 *
	 * @return 0 on success, negative error code to reject the connection
	 */
	int (*accept)(struct bt_conn *conn, struct bt_map_mse_mas_rfcomm_server *server,
		      struct bt_map_mse_mas **mse_mas);
};

/** @brief MAP Server MAS L2CAP server
 *
 * This structure represents an L2CAP server for MAP Server MAS.
 */
struct bt_map_mse_mas_l2cap_server {
	/** GOEP L2CAP transport server - underlying transport layer */
	struct bt_goep_transport_l2cap_server server;

	/** Accept callback for incoming connections
	 *
	 * Called when a remote MCE attempts to connect to the MAS server.
	 *
	 * @param conn Bluetooth connection object
	 * @param server L2CAP server instance that received the connection
	 * @param mse_mas Output parameter to store the allocated MAS instance
	 *
	 * @return 0 on success, negative error code to reject the connection
	 */
	int (*accept)(struct bt_conn *conn, struct bt_map_mse_mas_l2cap_server *server,
		      struct bt_map_mse_mas **mse_mas);
};

/** @brief MAP Server MAS callbacks
 *
 * Callbacks for MAP Server MAS events.
 */
struct bt_map_mse_mas_cb {
	/** RFCOMM transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object
	 * @param mse_mas MAS server instance
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_map_mse_mas *mse_mas);

	/** RFCOMM transport disconnected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is closed.
	 *
	 * @param mse_mas MAS server instance
	 */
	void (*rfcomm_disconnected)(struct bt_map_mse_mas *mse_mas);

	/** L2CAP transport connected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object
	 * @param mse_mas MAS server instance
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_map_mse_mas *mse_mas);

	/** L2CAP transport disconnected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is closed.
	 *
	 * @param mse_mas MAS server instance
	 */
	void (*l2cap_disconnected)(struct bt_map_mse_mas *mse_mas);

	/** Connected callback
	 *
	 * Called when OBEX CONNECT request is received from remote MCE.
	 * Application should respond using bt_map_mse_mas_connect().
	 *
	 * @param mse_mas MAS server instance
	 * @param version OBEX protocol version requested by client
	 * @param mopl Maximum OBEX packet length requested by client
	 * @param buf Buffer containing request headers (target UUID, supported features, etc.)
	 */
	void (*connected)(struct bt_map_mse_mas *mse_mas, uint8_t version, uint16_t mopl,
			  struct net_buf *buf);

	/** Disconnected callback
	 *
	 * Called when OBEX DISCONNECT request is received.
	 * Application should respond using bt_map_mse_mas_disconnect().
	 *
	 * @param mse_mas MAS server instance
	 * @param buf Buffer containing request headers
	 */
	void (*disconnected)(struct bt_map_mse_mas *mse_mas, struct net_buf *buf);

	/** Abort callback
	 *
	 * Called when OBEX ABORT request is received.
	 * Application should respond using bt_map_mse_mas_abort().
	 *
	 * @param mse_mas MAS server instance
	 * @param buf Buffer containing request headers
	 */
	void (*abort)(struct bt_map_mse_mas *mse_mas, struct net_buf *buf);

	/** Set notification registration callback
	 *
	 * Called when SetNotificationRegistration request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_set_ntf_reg().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if this is the final packet, false if more data follows
	 * @param buf Buffer containing notification status and headers
	 */
	void (*set_ntf_reg)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Set folder callback
	 *
	 * Called when SetFolder request is received.
	 * Application should respond using bt_map_mse_mas_set_folder().
	 *
	 * @param mse_mas MAS server instance
	 * @param flags Navigation flags (see @ref bt_map_set_folder_flags)
	 * @param buf Buffer containing folder name (for DOWN) and headers
	 */
	void (*set_folder)(struct bt_map_mse_mas *mse_mas, uint8_t flags, struct net_buf *buf);

	/** Get folder listing callback
	 *
	 * Called when GetFolderListing request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_folder_listing().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if request is fragmented, or false
	 * @param buf Buffer containing filter parameters and headers
	 */
	void (*get_folder_listing)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Get message listing callback
	 *
	 * Called when GetMessageListing request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_msg_listing().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if request is fragmented, or false
	 * @param buf Buffer containing folder name, filter parameters, and headers
	 */
	void (*get_msg_listing)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Get message callback
	 *
	 * Called when GetMessage request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_msg().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if request is fragmented, or false
	 * @param buf Buffer containing message handle, parameters, and headers
	 */
	void (*get_msg)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Set message status callback
	 *
	 * Called when SetMessageStatus request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_set_msg_status().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if this is the final packet, false if more data follows
	 * @param buf Buffer containing message handle, status parameters, and headers
	 */
	void (*set_msg_status)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Push message callback
	 *
	 * Called when PushMessage request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_push_msg().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if this is the final packet, false if more data follows
	 * @param buf Buffer containing message data (bMessage format) and headers
	 */
	void (*push_msg)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Update inbox callback
	 *
	 * Called when UpdateInbox request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_update_inbox().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if this is the final packet, false if more data follows
	 * @param buf Buffer containing headers
	 */
	void (*update_inbox)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Get MAS instance info callback
	 *
	 * Called when GetMASInstanceInformation request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_mas_inst_info().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if request is fragmented, or false
	 * @param buf Buffer containing instance ID and headers
	 */
	void (*get_mas_inst_info)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Set owner status callback
	 *
	 * Called when SetOwnerStatus request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_set_owner_status().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if this is the final packet, false if more data follows
	 * @param buf Buffer containing owner status data and headers
	 */
	void (*set_owner_status)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Get owner status callback
	 *
	 * Called when GetOwnerStatus request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_owner_status().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if request is fragmented, or false
	 * @param buf Buffer containing headers
	 */
	void (*get_owner_status)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Get conversation listing callback
	 *
	 * Called when GetConversationListing request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_convo_listing().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if request is fragmented, or false
	 * @param buf Buffer containing filter parameters and headers
	 */
	void (*get_convo_listing)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Set notification filter callback
	 *
	 * Called when SetNotificationFilter request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_set_ntf_filter().
	 *
	 * @param mse_mas MAS server instance
	 * @param final True if this is the final packet, false if more data follows
	 * @param buf Buffer containing filter mask and headers
	 */
	void (*set_ntf_filter)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);
};

/** @brief MAP Server MNS callbacks
 *
 * Callbacks for MAP Server MNS events.
 */
struct bt_map_mse_mns_cb {
	/** RFCOMM transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object
	 * @param mse_mns MNS client instance
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns);

	/** RFCOMM transport disconnected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is closed.
	 * All pending operations are cancelled.
	 *
	 * @param mse_mns MNS client instance
	 */
	void (*rfcomm_disconnected)(struct bt_map_mse_mns *mse_mns);

	/** L2CAP transport connected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object
	 * @param mse_mns MNS client instance
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns);

	/** L2CAP transport disconnected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is closed.
	 * All pending operations are cancelled.
	 *
	 * @param mse_mns MNS client instance
	 */
	void (*l2cap_disconnected)(struct bt_map_mse_mns *mse_mns);

	/** Connected callback
	 *
	 * Called when OBEX CONNECT response is received.
	 *
	 * @param mse_mns MNS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_SUCCESS on success)
	 * @param version OBEX protocol version supported by server
	 * @param mopl Maximum OBEX packet length supported by server
	 * @param buf Buffer containing additional response headers (connection ID, etc.)
	 */
	void (*connected)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, uint8_t version,
			  uint16_t mopl, struct net_buf *buf);

	/** Disconnected callback
	 *
	 * Called when OBEX DISCONNECT response is received.
	 *
	 * @param mse_mns MNS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*disconnected)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf);

	/** Abort callback
	 *
	 * Called when OBEX ABORT response is received.
	 * The aborted operation is cancelled.
	 *
	 * @param mse_mns MNS client instance
	 * @param rsp_code OBEX response code
	 * @param buf Buffer containing response headers
	 */
	void (*abort)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf);

	/** Send event callback
	 *
	 * Called when SendEvent response is received.
	 * May be called multiple times if request is fragmented (BT_OBEX_RSP_CODE_CONTINUE).
	 *
	 * @param mse_mns MNS client instance
	 * @param rsp_code OBEX response code (BT_OBEX_RSP_CODE_CONTINUE for partial upload)
	 * @param buf Buffer containing response headers
	 */
	void (*send_event)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf);
};

/** @brief MAP Server MAS instance structure
 *
 * Represents a server that handles message access requests from clients.
 * The MSE acts as a server for the MAS connection.
 */
struct bt_map_mse_mas {
	/** Underlying GOEP instance */
	struct bt_goep goep;

	/** Callbacks */
	const struct bt_map_mse_mas_cb *_cb;

	/** Transport type */
	uint8_t _transport_type;

	/** Transport state (atomic) */
	atomic_t _transport_state;

	/** Connection ID */
	uint32_t _conn_id;

	/** Underlying OBEX server */
	struct bt_obex_server _server;

	/** Server state (atomic) */
	atomic_t _state;

	/** Pending request callback */
	void (*_req_cb)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** Operation type */
	const char *_optype;

	/** Operation code */
	uint8_t _opcode;
};

/** @brief MAP Server MNS instance structure
 *
 * Represents a client connection to a Message Notification Server.
 * Used for sending event notifications to the MCE.
 */
struct bt_map_mse_mns {
	/** Underlying GOEP instance */
	struct bt_goep goep;

	/** Callbacks */
	const struct bt_map_mse_mns_cb *_cb;

	/** Transport type */
	uint8_t _transport_type;

	/** Transport state (atomic) */
	atomic_t _transport_state;

	/** Connection ID */
	uint32_t _conn_id;

	/** Underlying OBEX client */
	struct bt_obex_client _client;

	/** Client state (atomic) */
	atomic_t _state;

	/** Pending response callback */
	void (*_rsp_cb)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf);

	/** Current request function type */
	const char *_req_type;
};

/** @brief Register callbacks for MAP Server MAS
 *
 * Registers callback functions to handle MAS events and requests.
 * Must be called before accepting any MAS connections.
 *
 * @param mse_mas MAS server instance
 * @param cb Pointer to callback structure (must remain valid)
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_cb_register(struct bt_map_mse_mas *mse_mas, const struct bt_map_mse_mas_cb *cb);

/** @brief Register MAP Server MAS RFCOMM server
 *
 * Registers an RFCOMM server to accept incoming MAS connections.
 * The server channel is allocated automatically or can be specified.
 *
 * @param server RFCOMM server structure with accept callback configured
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_rfcomm_register(struct bt_map_mse_mas_rfcomm_server *server);

/** @brief Disconnect MAP Server MAS over RFCOMM
 *
 * Closes the RFCOMM transport connection.
 * On success, transport_disconnected callback will be called.
 *
 * @param mse_mas MAS server instance
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_rfcomm_disconnect(struct bt_map_mse_mas *mse_mas);

/** @brief Register MAP Server MAS L2CAP server
 *
 * Registers an L2CAP server to accept incoming MAS connections.
 * The PSM is allocated automatically or can be specified.
 *
 * @param server L2CAP server structure with accept callback configured
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_l2cap_register(struct bt_map_mse_mas_l2cap_server *server);

/** @brief Disconnect MAP Server MAS over L2CAP
 *
 * Closes the L2CAP transport connection.
 * On success, transport_disconnected callback will be called.
 *
 * @param mse_mas MAS server instance
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_l2cap_disconnect(struct bt_map_mse_mas *mse_mas);

/** @brief Create PDU for MAP Server MAS
 *
 * Allocates a buffer for building MAP response packets.
 * The buffer is sized appropriately for the negotiated MTU.
 *
 * @param mse_mas MAS server instance
 * @param pool Buffer pool to allocate from (NULL for default pool)
 *
 * @return Allocated buffer, or NULL on allocation failure
 */
struct net_buf *bt_map_mse_mas_create_pdu(struct bt_map_mse_mas *mse_mas,
					  struct net_buf_pool *pool);

/** @brief Send OBEX connect response
 *
 * Responds to an OBEX CONNECT request from the remote MCE.
 * Called from the connected callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS to accept)
 * @param buf Buffer containing response headers (WHO, connection ID, etc.)
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_connect(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send OBEX disconnect response
 *
 * Responds to an OBEX DISCONNECT request from the remote MCE.
 * Called from the disconnected callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS to accept)
 * @param buf Buffer containing response headers
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_disconnect(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
			      struct net_buf *buf);

/** @brief Send OBEX abort response
 *
 * Responds to an OBEX ABORT request from the remote MCE.
 * Called from the abort callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS to accept)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_abort(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send set folder response
 *
 * Responds to a SetPath request from the remote MCE.
 * Called from the set_folder callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS on success)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_set_folder(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
			      struct net_buf *buf);

/** @brief Send set notification registration response
 *
 * Responds to a SetNotificationRegistration request from the remote MCE.
 * Called from the set_ntf_reg callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS on success)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_set_ntf_reg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
			       struct net_buf *buf);

/** @brief Send get folder listing response
 *
 * Responds to a GetFolderListing request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_folder_listing callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 BT_OBEX_RSP_CODE_SUCCESS for complete)
 * @param buf Buffer containing folder listing XML data and headers
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_get_folder_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				      struct net_buf *buf);

/** @brief Send get message listing response
 *
 * Responds to a GetMessageListing request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_msg_listing callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 BT_OBEX_RSP_CODE_SUCCESS for complete)
 * @param buf Buffer containing message listing XML data and headers
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_get_msg_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				   struct net_buf *buf);

/** @brief Send get message response
 *
 * Responds to a GetMessage request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_msg callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 BT_OBEX_RSP_CODE_SUCCESS for complete)
 * @param buf Buffer containing message bMessage data and headers
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_get_msg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send set message status response
 *
 * Responds to a SetMessageStatus request from the remote MCE.
 * Called from the set_msg_status callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS on success)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_set_msg_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				  struct net_buf *buf);

/** @brief Send push message response
 *
 * Responds to a PushMessage request from the remote MCE.
 * May be called multiple times if request is fragmented.
 * Called from the push_msg callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 BT_OBEX_RSP_CODE_SUCCESS for complete)
 * @param buf Buffer containing message handle (on success) and headers
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_push_msg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send update inbox response
 *
 * Responds to an UpdateInbox request from the remote MCE.
 * Called from the update_inbox callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS on success)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_update_inbox(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				struct net_buf *buf);

/** @brief Send get MAS instance info response
 *
 * Responds to a GetMASInstanceInformation request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_mas_inst_info callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 BT_OBEX_RSP_CODE_SUCCESS for complete)
 * @param buf Buffer containing instance information XML data and headers
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_get_mas_inst_info(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				     struct net_buf *buf);

/** @brief Send set owner status response
 *
 * Responds to a SetOwnerStatus request from the remote MCE.
 * Called from the set_owner_status callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS on success)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_set_owner_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				    struct net_buf *buf);

/** @brief Send get owner status response
 *
 * Responds to a GetOwnerStatus request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_owner_status callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 BT_OBEX_RSP_CODE_SUCCESS for complete)
 * @param buf Buffer containing owner status data and headers
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_get_owner_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				    struct net_buf *buf);

/** @brief Send get conversation listing response
 *
 * Responds to a GetConversationListing request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_convo_listing callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 BT_OBEX_RSP_CODE_SUCCESS for complete)
 * @param buf Buffer containing conversation listing XML data and headers
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_get_convo_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				     struct net_buf *buf);

/** @brief Send set notification filter response
 *
 * Responds to a SetNotificationFilter request from the remote MCE.
 * Called from the set_ntf_filter callback.
 *
 * @param mse_mas MAS server instance
 * @param rsp_code Response code (BT_OBEX_RSP_CODE_SUCCESS on success)
 * @param buf Buffer containing response headers
 *            If NULL, a default response is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mas_set_ntf_filter(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				  struct net_buf *buf);

/** @brief Register callbacks for MAP Server MNS
 *
 * Registers callback functions to handle MNS events and responses.
 * Must be called before initiating any MNS operations.
 *
 * @param mse_mns MNS client instance
 * @param cb Pointer to callback structure (must remain valid)
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_cb_register(struct bt_map_mse_mns *mse_mns, const struct bt_map_mse_mns_cb *cb);

/** @brief Connect MAP Server MNS over RFCOMM
 *
 * Initiates RFCOMM transport connection to a remote MNS.
 * On success, transport_connected callback will be called.
 *
 * @param conn Bluetooth connection to remote device
 * @param mse_mns MNS client instance
 * @param channel RFCOMM server channel (from SDP discovery)
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_rfcomm_connect(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns,
				  uint8_t channel);

/** @brief Disconnect MAP Server MNS over RFCOMM
 *
 * Closes the RFCOMM transport connection.
 * On success, transport_disconnected callback will be called.
 *
 * @param mse_mns MNS client instance
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_rfcomm_disconnect(struct bt_map_mse_mns *mse_mns);

/** @brief Connect MAP Server MNS over L2CAP
 *
 * Initiates L2CAP transport connection to a remote MNS.
 * On success, transport_connected callback will be called.
 *
 * @param conn Bluetooth connection to remote device
 * @param mse_mns MNS client instance
 * @param psm L2CAP PSM (from SDP discovery)
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_l2cap_connect(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns,
				 uint16_t psm);

/** @brief Disconnect MAP Server MNS over L2CAP
 *
 * Closes the L2CAP transport connection.
 * On success, transport_disconnected callback will be called.
 *
 * @param mse_mns MNS client instance
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_l2cap_disconnect(struct bt_map_mse_mns *mse_mns);

/** @brief Create PDU for MAP Server MNS
 *
 * Allocates a buffer for building MAP request packets.
 * The buffer is sized appropriately for the negotiated MTU.
 *
 * @param mse_mns MNS client instance
 * @param pool Buffer pool to allocate from (NULL for default pool)
 *
 * @return Allocated buffer, or NULL on allocation failure
 */
struct net_buf *bt_map_mse_mns_create_pdu(struct bt_map_mse_mns *mse_mns,
					  struct net_buf_pool *pool);

/** @brief Send OBEX connect request
 *
 * Initiates OBEX session establishment with the MNS.
 * Must be called after transport connection is established.
 * On completion, connected callback will be called.
 *
 * @param mse_mns MNS client instance
 * @param buf Buffer containing connect headers (target UUID, etc.)
 *            If NULL, a default connect request is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_connect(struct bt_map_mse_mns *mse_mns, struct net_buf *buf);

/** @brief Send OBEX disconnect request
 *
 * Initiates OBEX session termination.
 * On completion, disconnected callback will be called.
 *
 * @param mse_mns MNS client instance
 * @param buf Buffer containing disconnect headers (connection ID, etc.)
 *            If NULL, a default disconnect request is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_disconnect(struct bt_map_mse_mns *mse_mns, struct net_buf *buf);

/** @brief Send OBEX abort request
 *
 * Aborts the currently ongoing operation.
 * On completion, abort callback will be called.
 *
 * @param mse_mns MNS client instance
 * @param buf Buffer containing abort headers (connection ID, etc.)
 *            If NULL, a default abort request is sent.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_abort(struct bt_map_mse_mns *mse_mns, struct net_buf *buf);

/** @brief Send event notification request
 *
 * Sends an event notification to the remote MCE.
 * May be called multiple times to send fragmented event data.
 * On completion, send_event callback will be called (possibly multiple times).
 *
 * @param mse_mns MNS client instance
 * @param final True if this is the last packet, false if more data follows
 * @param buf Buffer containing event report XML data and headers.
 *            Must include connection ID, type header, and application parameters
 *            for initial request. Must include end-of-body header when final is true.
 *            Buffer is consumed on success.
 *
 * @return 0 on success, negative error code on failure
 */
int bt_map_mse_mns_send_event(struct bt_map_mse_mns *mse_mns, bool final, struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MAP_H_ */
