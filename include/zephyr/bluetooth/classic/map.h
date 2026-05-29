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
 * Used to identify MAS service in OBEX connections.
 */
#define BT_MAP_UUID_MAS                                                                            \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0xBB582B40, 0x420C, 0x11DB, 0xB0DE, 0x0800200C9A66))

/** @brief MAP Message Notification Server (MNS) UUID
 *
 * 128-bit UUID for the Message Notification Server service.
 * Used to identify MNS service in OBEX connections.
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
	BT_MAP_NTF_REG_FEATURE = BIT(0),
	/** Notification feature */
	BT_MAP_NTF_FEATURE = BIT(1),
	/** Browsing feature */
	BT_MAP_BROWSING_FEATURE = BIT(2),
	/** Uploading feature */
	BT_MAP_UPLOADING_FEATURE = BIT(3),
	/** Delete feature */
	BT_MAP_DELETE_FEATURE = BIT(4),
	/** Instance information feature */
	BT_MAP_INST_INFO_FEATURE = BIT(5),
	/** Extended event report version 1.1 */
	BT_MAP_EXT_EVENT_REPORT_V11 = BIT(6),
	/** Extended event version 1.2 */
	BT_MAP_EXT_EVENT_REPORT_V12 = BIT(7),
	/** Message format version 1.1 */
	BT_MAP_MSG_FORMAT_V11 = BIT(8),
	/** Message listing format version 1.1 */
	BT_MAP_MSG_LISTING_FORMAT_V11 = BIT(9),
	/** Persistent message handle */
	BT_MAP_PERSISTENT_MSG_HANDLE = BIT(10),
	/** Database identifier */
	BT_MAP_DB_ID = BIT(11),
	/** Folder version counter */
	BT_MAP_FOLDER_VER_CNTR = BIT(12),
	/** Conversation version counter */
	BT_MAP_CONVO_VER_CNTR = BIT(13),
	/** Participant presence change notification */
	BT_MAP_PARTICIPANT_PRESENCE_CHANGE_NTF = BIT(14),
	/** Participant chat state change notification */
	BT_MAP_PARTICIPANT_CHAT_STATE_CHANGE_NTF = BIT(15),
	/** PBAP contact cross reference */
	BT_MAP_PBAP_CONTACT_XREF = BIT(16),
	/** Notification filtering */
	BT_MAP_NTF_FILTERING = BIT(17),
	/** UTC offset timestamp format */
	BT_MAP_UTC_OFFSET_TIMESTAMP_FORMAT = BIT(18),
	/** Supported features in connect request */
	BT_MAP_SUPPORTED_FEATURES_CONN_REQ = BIT(19),
	/** Conversation listing */
	BT_MAP_CONVO_LISTING = BIT(20),
	/** Owner status */
	BT_MAP_OWNER_STATUS = BIT(21),
	/** Message forwarding */
	BT_MAP_MSG_FORWARDING = BIT(22),
};

/** @brief MAP supported message types
 *
 * Bitmask indicating which message types are supported by a MAS instance.
 */
enum __packed bt_map_supported_msg_type {
	/** Email message type */
	BT_MAP_MSG_TYPE_EMAIL = BIT(0),
	/** SMS GSM message type */
	BT_MAP_MSG_TYPE_SMS_GSM = BIT(1),
	/** SMS CDMA message type */
	BT_MAP_MSG_TYPE_SMS_CDMA = BIT(2),
	/** MMS message type */
	BT_MAP_MSG_TYPE_MMS = BIT(3),
	/** Instant messaging type */
	BT_MAP_MSG_TYPE_IM = BIT(4),
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
	BT_MAP_APPL_PARAM_TAG_ID_MAX_LIST_CNT = 0x01,
	/** List start offset (2 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_LIST_START_OFFSET = 0x02,
	/** Filter message type (1 byte) @ref bt_map_filter_msg_type */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_MSG_TYPE = 0x03,
	/** Filter period begin (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_PERIOD_BEGIN = 0x04,
	/** Filter period end (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_PERIOD_END = 0x05,
	/** Filter read status (1 byte) @ref bt_map_filter_read_status */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_READ_STATUS = 0x06,
	/** Filter recipient (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_RECIPIENT = 0x07,
	/** Filter originator (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_ORIGINATOR = 0x08,
	/** Filter priority (1 byte) @ref bt_map_filter_priority */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_PRIORITY = 0x09,
	/** Attachment (1 byte) @ref bt_map_attachment */
	BT_MAP_APPL_PARAM_TAG_ID_ATTACHMENT = 0x0a,
	/** Transparent (1 byte) @ref bt_map_transparent */
	BT_MAP_APPL_PARAM_TAG_ID_TRANSPARENT = 0x0b,
	/** Retry (1 byte) @ref bt_map_retry */
	BT_MAP_APPL_PARAM_TAG_ID_RETRY = 0x0c,
	/** New message (1 byte) @ref bt_map_new_msg */
	BT_MAP_APPL_PARAM_TAG_ID_NEW_MSG = 0x0d,
	/** Notification status (1 byte) @ref bt_map_ntf_status */
	BT_MAP_APPL_PARAM_TAG_ID_NTF_STATUS = 0x0e,
	/** MAS instance ID (1 byte) */
	BT_MAP_APPL_PARAM_TAG_ID_MAS_INST_ID = 0x0f,
	/** Parameter mask (4 bytes) @ref bt_map_param_mask */
	BT_MAP_APPL_PARAM_TAG_ID_PARAM_MASK = 0x10,
	/** Folder listing size (2 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_FOLDER_LISTING_SIZE = 0x11,
	/** Listing size (2 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_LISTING_SIZE = 0x12,
	/** Subject length (variable, max 255 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_SUBJECT_LEN = 0x13,
	/** Charset (1 byte) @ref bt_map_charset */
	BT_MAP_APPL_PARAM_TAG_ID_CHARSET = 0x14,
	/** Fraction request (1 byte) @ref bt_map_fraction_request */
	BT_MAP_APPL_PARAM_TAG_ID_FRACTION_REQUEST = 0x15,
	/** Fraction deliver (1 byte) @ref bt_map_fraction_deliver */
	BT_MAP_APPL_PARAM_TAG_ID_FRACTION_DELIVER = 0x16,
	/** Status indicator (1 byte) @ref bt_map_status_ind */
	BT_MAP_APPL_PARAM_TAG_ID_STATUS_IND = 0x17,
	/** Status value (1 byte) @ref bt_map_status_val */
	BT_MAP_APPL_PARAM_TAG_ID_STATUS_VAL = 0x18,
	/** MSE time (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_MSE_TIME = 0x19,
	/** Database identifier (variable, max 32 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_DB_ID = 0x1a,
	/** Conversation listing version counter (variable, max 32 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_CONVO_LISTING_VER_CNTR = 0x1b,
	/** Presence availability (1 byte) @ref bt_map_presence */
	BT_MAP_APPL_PARAM_TAG_ID_PRESENCE_AVAIL = 0x1c,
	/** Presence text (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_PRESENCE_TEXT = 0x1d,
	/** Last activity (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_LAST_ACTIVITY = 0x1e,
	/** Filter last activity begin (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_LAST_ACTIVITY_BEGIN = 0x1f,
	/** Filter last activity end (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_LAST_ACTIVITY_END = 0x20,
	/** Chat state (1 byte) @ref bt_map_chat_state */
	BT_MAP_APPL_PARAM_TAG_ID_CHAT_STATE = 0x21,
	/** Conversation ID (variable, max 32 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_CONVO_ID = 0x22,
	/** Folder version counter (variable, max 32 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_FOLDER_VER_CNTR = 0x23,
	/** Filter message handle (variable, max 16 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_FILTER_MSG_HANDLE = 0x24,
	/** Notification filter mask (4 bytes) @ref bt_map_ntf_filter_mask */
	BT_MAP_APPL_PARAM_TAG_ID_NTF_FILTER_MASK = 0x25,
	/** Conversation parameter mask (4 bytes) @ref bt_map_convo_param_mask */
	BT_MAP_APPL_PARAM_TAG_ID_CONVO_PARAM_MASK = 0x26,
	/** Owner UCI (variable length) */
	BT_MAP_APPL_PARAM_TAG_ID_OWNER_UCI = 0x27,
	/** Extended data (variable length) @ref bt_map_msg_ext_data */
	BT_MAP_APPL_PARAM_TAG_ID_EXT_DATA = 0x28,
	/** MAP supported features (4 bytes) @ref bt_map_supported_features */
	BT_MAP_APPL_PARAM_TAG_ID_MAP_SUPPORTED_FEATURES = 0x29,
	/** Message handle (variable, max 16 bytes) */
	BT_MAP_APPL_PARAM_TAG_ID_MSG_HANDLE = 0x2a,
	/** Modify text (1 byte) @ref bt_map_modify_text */
	BT_MAP_APPL_PARAM_TAG_ID_MODIFY_TEXT = 0x2b,
};

/** @brief MAP filter message type values
 *
 * Bitmask for filtering messages by type.
 */
enum __packed bt_map_filter_msg_type {
	/** No filtering */
	BT_MAP_FILTER_MSG_TYPE_NO_FILTERING = 0x00,
	/** SMS GSM */
	BT_MAP_FILTER_MSG_TYPE_SMS_GSM = BIT(0),
	/** SMS CDMA */
	BT_MAP_FILTER_MSG_TYPE_SMS_CDMA = BIT(1),
	/** Email */
	BT_MAP_FILTER_MSG_TYPE_EMAIL = BIT(2),
	/** MMS */
	BT_MAP_FILTER_MSG_TYPE_MMS = BIT(3),
	/** Instant Message */
	BT_MAP_FILTER_MSG_TYPE_IM = BIT(4),
};

/** @brief MAP filter read status values
 *
 * Filter for read/unread messages.
 */
enum __packed bt_map_filter_read_status {
	/** No filtering */
	BT_MAP_FILTER_READ_STATUS_NO_FILTERING = 0x00,
	/** Unread messages only */
	BT_MAP_FILTER_READ_STATUS_UNREAD = 0x01,
	/** Read messages only */
	BT_MAP_FILTER_READ_STATUS_READ = 0x02,
};

/** @brief MAP filter priority values
 *
 * Filter for message priority.
 */
enum __packed bt_map_filter_priority {
	/** No filtering */
	BT_MAP_FILTER_PRIORITY_NO_FILTERING = 0x00,
	/** High priority messages only */
	BT_MAP_FILTER_PRIORITY_HIGH = 0x01,
	/** Non-high priority messages only */
	BT_MAP_FILTER_PRIORITY_NON_HIGH = 0x02,
};

/** @brief MAP attachment values
 *
 * Indicates whether to include attachments.
 */
enum __packed bt_map_attachment {
	/** Off - no attachments */
	BT_MAP_ATTACHMENT_OFF = 0x00,
	/** On - include attachments */
	BT_MAP_ATTACHMENT_ON = 0x01,
};

/** @brief MAP transparent values
 *
 * Indicates whether to use transparent mode.
 */
enum __packed bt_map_transparent {
	/** Off - not transparent */
	BT_MAP_TRANSPARENT_OFF = 0x00,
	/** On - transparent */
	BT_MAP_TRANSPARENT_ON = 0x01,
};

/** @brief MAP retry values
 *
 * Indicates whether to retry sending.
 */
enum __packed bt_map_retry {
	/** Off - no retry */
	BT_MAP_RETRY_OFF = 0x00,
	/** On - retry */
	BT_MAP_RETRY_ON = 0x01,
};

/** @brief MAP new message values
 *
 * Indicates whether a message is new.
 */
enum __packed bt_map_new_msg {
	/** Not a new message */
	BT_MAP_NEW_MSG_OFF = 0x00,
	/** New message */
	BT_MAP_NEW_MSG_ON = 0x01,
};

/** @brief MAP notification status values
 *
 * Status for notification registration.
 */
enum __packed bt_map_ntf_status {
	/** Notifications off */
	BT_MAP_NTF_STATUS_OFF = 0x00,
	/** Notifications on */
	BT_MAP_NTF_STATUS_ON = 0x01,
};

/** @brief MAP parameter mask bits
 *
 * Bitmask for selecting which message attributes to include.
 */
enum __packed bt_map_param_mask {
	/** Subject */
	BT_MAP_PARAM_MASK_SUBJECT = BIT(0),
	/** Datetime */
	BT_MAP_PARAM_MASK_DATETIME = BIT(1),
	/** Sender name */
	BT_MAP_PARAM_MASK_SENDER_NAME = BIT(2),
	/** Sender addressing */
	BT_MAP_PARAM_MASK_SENDER_ADDRESSING = BIT(3),
	/** Recipient name */
	BT_MAP_PARAM_MASK_RECIPIENT_NAME = BIT(4),
	/** Recipient addressing */
	BT_MAP_PARAM_MASK_RECIPIENT_ADDRESSING = BIT(5),
	/** Type */
	BT_MAP_PARAM_MASK_TYPE = BIT(6),
	/** Size */
	BT_MAP_PARAM_MASK_SIZE = BIT(7),
	/** Reception status */
	BT_MAP_PARAM_MASK_RECEPTION_STATUS = BIT(8),
	/** Text */
	BT_MAP_PARAM_MASK_TEXT = BIT(9),
	/** Attachment size */
	BT_MAP_PARAM_MASK_ATTACHMENT_SIZE = BIT(10),
	/** Priority */
	BT_MAP_PARAM_MASK_PRIORITY = BIT(11),
	/** Read */
	BT_MAP_PARAM_MASK_READ = BIT(12),
	/** Sent */
	BT_MAP_PARAM_MASK_SENT = BIT(13),
	/** Protected */
	BT_MAP_PARAM_MASK_PROTECTED = BIT(14),
	/** Replyto addressing */
	BT_MAP_PARAM_MASK_REPLYTO_ADDRESSING = BIT(15),
	/** Delivery status */
	BT_MAP_PARAM_MASK_DELIVERY_STATUS = BIT(16),
	/** Conversation ID */
	BT_MAP_PARAM_MASK_CONVO_ID = BIT(17),
	/** Conversation name */
	BT_MAP_PARAM_MASK_CONVO_NAME = BIT(18),
	/** Direction */
	BT_MAP_PARAM_MASK_DIRECTION = BIT(19),
	/** Attachment MIME */
	BT_MAP_PARAM_MASK_ATTACHMENT_MIME = BIT(20),
};

/** @brief MAP charset values
 *
 * Character set for message content.
 */
enum __packed bt_map_charset {
	/** Native charset */
	BT_MAP_CHARSET_NATIVE = 0x00,
	/** UTF-8 charset */
	BT_MAP_CHARSET_UTF8 = 0x01,
};

/** @brief MAP fraction request values
 *
 * Fraction of message to request.
 */
enum __packed bt_map_fraction_request {
	/** First fraction */
	BT_MAP_FRACTION_REQUEST_FIRST = 0x00,
	/** Next fraction */
	BT_MAP_FRACTION_REQUEST_NEXT = 0x01,
};

/** @brief MAP fraction deliver values
 *
 * Fraction of message delivered.
 */
enum __packed bt_map_fraction_deliver {
	/** More fractions to follow */
	BT_MAP_FRACTION_DELIVER_MORE = 0x00,
	/** Last fraction */
	BT_MAP_FRACTION_DELIVER_LAST = 0x01,
};

/** @brief MAP status indicator values
 *
 * Type of status to modify.
 */
enum __packed bt_map_status_ind {
	/** Read status */
	BT_MAP_STATUS_IND_READ = 0x00,
	/** Deleted status */
	BT_MAP_STATUS_IND_DELETED = 0x01,
	/** Set extended data */
	BT_MAP_STATUS_IND_EXTENDED_DATA = 0x02,
};

/** @brief MAP status value values
 *
 * New status value.
 */
enum __packed bt_map_status_val {
	/** No/Off */
	BT_MAP_STATUS_VAL_NO = 0x00,
	/** Yes/On */
	BT_MAP_STATUS_VAL_YES = 0x01,
};

/** @brief MAP presence availability states
 *
 * Enumeration of possible presence states for the message owner.
 */
enum __packed bt_map_presence {
	/** Unknown presence state */
	BT_MAP_PRESENCE_UNKNOWN = 0x00,
	/** Offline */
	BT_MAP_PRESENCE_OFFLINE = 0x01,
	/** Online */
	BT_MAP_PRESENCE_ONLINE = 0x02,
	/** Away */
	BT_MAP_PRESENCE_AWAY = 0x03,
	/** Do not disturb */
	BT_MAP_PRESENCE_DO_NOT_DISTURB = 0x04,
	/** Busy */
	BT_MAP_PRESENCE_BUSY = 0x05,
	/** In a meeting */
	BT_MAP_PRESENCE_IN_A_MEETING = 0x06,
};

/** @brief MAP chat states
 *
 * Enumeration of possible chat states for the message owner.
 */
enum __packed bt_map_chat_state {
	/** Unknown chat state */
	BT_MAP_CHAT_STATE_UNKNOWN = 0x00,
	/** Inactive */
	BT_MAP_CHAT_STATE_INACTIVE = 0x01,
	/** Active */
	BT_MAP_CHAT_STATE_ACTIVE = 0x02,
	/** Composing */
	BT_MAP_CHAT_STATE_COMPOSING = 0x03,
	/** Paused composing */
	BT_MAP_CHAT_STATE_PAUSED_COMPOSING = 0x04,
	/** Gone */
	BT_MAP_CHAT_STATE_GONE = 0x05,
};

/** @brief MAP notification filter mask bits
 *
 * Bitmask for filtering which event types to receive notifications for.
 */
enum __packed bt_map_ntf_filter_mask {
	/** NewMessage */
	BT_MAP_NTF_FILTER_MASK_NEW_MSG = BIT(0),
	/** MessageDeleted */
	BT_MAP_NTF_FILTER_MASK_MSG_DELETED = BIT(1),
	/** MessageShift */
	BT_MAP_NTF_FILTER_MASK_MSG_SHIFT = BIT(2),
	/** SendingSuccess */
	BT_MAP_NTF_FILTER_MASK_SENDING_SUCCESS = BIT(3),
	/** SendingFailure */
	BT_MAP_NTF_FILTER_MASK_SENDING_FAILURE = BIT(4),
	/** DeliverySuccess */
	BT_MAP_NTF_FILTER_MASK_DELIVERY_SUCCESS = BIT(5),
	/** DeliveryFailure */
	BT_MAP_NTF_FILTER_MASK_DELIVERY_FAILURE = BIT(6),
	/** MemoryFull */
	BT_MAP_NTF_FILTER_MASK_MEM_FULL = BIT(7),
	/** MemoryAvailable */
	BT_MAP_NTF_FILTER_MASK_MEM_AVAIL = BIT(8),
	/** ReadStatusChanged */
	BT_MAP_NTF_FILTER_MASK_READ_STATUS_CHANGED = BIT(9),
	/** ConversationChanged */
	BT_MAP_NTF_FILTER_MASK_CONVO_CHANGED = BIT(10),
	/** ParticipantPresenceChanged */
	BT_MAP_NTF_FILTER_MASK_PARTICIPANT_PRESENCE_CHANGED = BIT(11),
	/** ParticipantChatStateChanged */
	BT_MAP_NTF_FILTER_MASK_PARTICIPANT_CHAT_STATE_CHANGED = BIT(12),
	/** MessageExtendedDataChanged */
	BT_MAP_NTF_FILTER_MASK_MSG_EXT_DATA_CHANGED = BIT(13),
	/** MessageRemoved */
	BT_MAP_NTF_FILTER_MASK_MSG_REMOVED = BIT(14),
};

/** @brief MAP conversation parameter mask bits
 *
 * Bitmask for selecting which conversation attributes to include.
 */
enum __packed bt_map_convo_param_mask {
	/** Conversation name */
	BT_MAP_CONVO_PARAM_MASK_CONVO_NAME = BIT(0),
	/** Conversation last activity */
	BT_MAP_CONVO_PARAM_MASK_CONVO_LAST_ACTIVITY = BIT(1),
	/** Read status */
	BT_MAP_CONVO_PARAM_MASK_READ_STATUS = BIT(2),
	/** Version counter */
	BT_MAP_CONVO_PARAM_MASK_VER_CNTR = BIT(3),
	/** Summary */
	BT_MAP_CONVO_PARAM_MASK_SUMMARY = BIT(4),
	/** Participants */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANTS = BIT(5),
	/** Participant UCI */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_UCI = BIT(6),
	/** Participant display name */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_DISP_NAME = BIT(7),
	/** Participant chat state */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_CHAT_STATE = BIT(8),
	/** Participant last activity */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_LAST_ACTIVITY = BIT(9),
	/** Participant X-BT-UID */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_X_BT_UID = BIT(10),
	/** Participant name */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_NAME = BIT(11),
	/** Participant presence availability */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_PRESENCE_AVAIL = BIT(12),
	/** Participant presence text */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_PRESENCE_TEXT = BIT(13),
	/** Participant priority */
	BT_MAP_CONVO_PARAM_MASK_PARTICIPANT_PRIORITY = BIT(14),
};

/** @brief MAP message extended data types
 *
 * Types of extended metadata that can be associated with messages.
 */
enum __packed bt_map_msg_ext_data {
	/** Number of Facebook likes */
	BT_MAP_MSG_EXT_DATA_FACEBOOK_LIKES = 0x00,
	/** Number of Twitter followers */
	BT_MAP_MSG_EXT_DATA_TWITTER_FOLLOWERS = 0x01,
	/** Number of Twitter retweets */
	BT_MAP_MSG_EXT_DATA_TWITTER_RETWEETS = 0x02,
	/** Number of Google +1s */
	BT_MAP_MSG_EXT_DATA_GOOGLE_1S = 0x03,
};

/** @brief MAP modify text values
 *
 * Indicates how message text is modified.
 */
enum __packed bt_map_modify_text {
	/** Replace text */
	BT_MAP_MODIFY_TEXT_REPLACE = 0x00,
	/** Prepend text */
	BT_MAP_MODIFY_TEXT_PREPEND = 0x01,
};

/**
 * @brief Messaging Client Equipment(MCE) OBEX Client
 * @defgroup bt_map_mce_mas Messaging Client Equipment(MCE) OBEX Client
 * @ingroup bt_map
 * @{
 */

/** @brief Forward declaration of MAP Client MAS structure */
struct bt_map_mce_mas;

/** @brief MAP Client MAS callbacks
 *
 * Callbacks for MAP Client MAS events.
 */
struct bt_map_mce_mas_cb {
	/** @brief RFCOMM transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object.
	 * @param mce_mas MAS client instance.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas);

	/** @brief RFCOMM transport disconnected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is closed.
	 * All pending operations are cancelled.
	 *
	 * @param mce_mas MAS client instance.
	 */
	void (*rfcomm_disconnected)(struct bt_map_mce_mas *mce_mas);

	/** @brief L2CAP transport connected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object.
	 * @param mce_mas MAS client instance.
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas);

	/** @brief L2CAP transport disconnected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is closed.
	 * All pending operations are cancelled.
	 *
	 * @param mce_mas MAS client instance.
	 */
	void (*l2cap_disconnected)(struct bt_map_mce_mas *mce_mas);

	/** @brief Connect callback
	 *
	 * Called when OBEX CONNECT response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS on
	 * success).
	 * @param version OBEX protocol version supported by server.
	 * @param mopl Maximum OBEX packet length supported by server.
	 * @param buf Buffer containing additional response headers (connection ID, etc.).
	 */
	void (*connect)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, uint8_t version,
			uint16_t mopl, struct net_buf *buf);

	/** @brief Disconnect callback
	 *
	 * Called when OBEX DISCONNECT response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*disconnect)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Abort callback
	 *
	 * Called when OBEX ABORT response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*abort)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Set notification registration callback
	 *
	 * Called when SetNotificationRegistration response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*set_ntf_reg)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Set folder callback
	 *
	 * Called when SetFolder response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*set_folder)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Get folder listing callback
	 *
	 * Called when GetFolderListing response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for
	 * partial data).
	 * @param buf Buffer containing folder listing XML data and headers.
	 */
	void (*get_folder_listing)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				   struct net_buf *buf);

	/** @brief Get message listing callback
	 *
	 * Called when GetMessageListing response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for
	 * partial data).
	 * @param buf Buffer containing message listing XML data and headers.
	 */
	void (*get_msg_listing)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				struct net_buf *buf);

	/** @brief Get message callback
	 *
	 * Called when GetMessage response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for
	 * partial data).
	 * @param buf Buffer containing message bMessage data and headers.
	 */
	void (*get_msg)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Set message status callback
	 *
	 * Called when SetMessageStatus response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*set_msg_status)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
			       struct net_buf *buf);

	/** @brief Push message callback
	 *
	 * Called when PushMessage response is received.
	 * May be called multiple times if request is fragmented (@ref BT_OBEX_RSP_CODE_CONTINUE).
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for
	 * partial upload).
	 * @param buf Buffer containing message handle (on success) and headers.
	 */
	void (*push_msg)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Update inbox callback
	 *
	 * Called when UpdateInbox response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*update_inbox)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Get MAS instance info callback
	 *
	 * Called when GetMASInstanceInformation response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for
	 * partial data).
	 * @param buf Buffer containing instance information XML data and headers.
	 */
	void (*get_mas_inst_info)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				  struct net_buf *buf);

	/** @brief Set owner status callback
	 *
	 * Called when SetOwnerStatus response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*set_owner_status)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				 struct net_buf *buf);

	/** @brief Get owner status callback
	 *
	 * Called when GetOwnerStatus response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for
	 * partial data).
	 * @param buf Buffer containing owner status data and headers.
	 */
	void (*get_owner_status)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				 struct net_buf *buf);

	/** @brief Get conversation listing callback
	 *
	 * Called when GetConversationListing response is received.
	 * May be called multiple times for a single request if response is fragmented.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for
	 * partial data).
	 * @param buf Buffer containing conversation listing XML data and headers.
	 */
	void (*get_convo_listing)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				  struct net_buf *buf);

	/** @brief Set notification filter callback
	 *
	 * Called when SetNotificationFilter response is received.
	 *
	 * @param mce_mas MAS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*set_ntf_filter)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
			       struct net_buf *buf);
};

/** @brief MAP Client MAS instance structure
 *
 * Represents a client connection to a Message Access Server.
 * Used for browsing folders and manipulating messages on the server.
 */
struct bt_map_mce_mas {
	/** @brief Underlying GOEP instance */
	struct bt_goep goep;

	/** @internal Callbacks */
	const struct bt_map_mce_mas_cb *_cb;

	/** @internal Transport type */
	uint8_t _transport_type;

	/** @internal Transport state (atomic) */
	atomic_t _transport_state;

	/** @internal Connection ID */
	uint32_t _conn_id;

	/** @internal Underlying OBEX client */
	struct bt_obex_client _client;

	/** @internal Client state (atomic) */
	atomic_t _state;

	/** @internal Pending response callback */
	void (*_rsp_cb)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);

	/** @internal Current request function type */
	const char *_req_type;
};

/** @brief Register callbacks for MAP Client MAS
 *
 * Registers callback functions to handle MAS events and responses.
 * Must be called before initiating any MAS operations.
 *
 * @param mce_mas MAS client instance.
 * @param cb Pointer to callback structure (must remain valid).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_cb_register(struct bt_map_mce_mas *mce_mas, const struct bt_map_mce_mas_cb *cb);

/** @brief Connect MAP Client MAS over RFCOMM
 *
 * Initiates RFCOMM transport connection to a remote MAS.
 * On success, the rfcomm_connected callback @ref bt_map_mce_mas_cb::rfcomm_connected will be
 * called.
 *
 * @param conn Bluetooth connection to remote device.
 * @param mce_mas MAS client instance.
 * @param channel RFCOMM server channel (from SDP discovery).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_rfcomm_connect(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas,
				  uint8_t channel);

/** @brief Disconnect MAP Client MAS over RFCOMM
 *
 * Closes the RFCOMM transport connection.
 * On success, the rfcomm_disconnected callback @ref bt_map_mce_mas_cb::rfcomm_disconnected will be
 * called.
 *
 * @param mce_mas MAS client instance.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_rfcomm_disconnect(struct bt_map_mce_mas *mce_mas);

/** @brief Connect MAP Client MAS over L2CAP
 *
 * Initiates L2CAP transport connection to a remote MAS.
 * On success, the l2cap_connected callback @ref bt_map_mce_mas_cb::l2cap_connected will be called.
 *
 * @param conn Bluetooth connection to remote device.
 * @param mce_mas MAS client instance.
 * @param psm L2CAP PSM (from SDP discovery).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_l2cap_connect(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas,
				 uint16_t psm);

/** @brief Disconnect MAP Client MAS over L2CAP
 *
 * Closes the L2CAP transport connection.
 * On success, the l2cap_disconnected callback @ref bt_map_mce_mas_cb::l2cap_disconnected will be
 * called.
 *
 * @param mce_mas MAS client instance.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_l2cap_disconnect(struct bt_map_mce_mas *mce_mas);

/** @brief Create PDU for MAP Client MAS
 *
 * Allocates a buffer for building MAP request packets.
 * The buffer is sized appropriately for the negotiated MTU.
 *
 * @param mce_mas MAS client instance.
 * @param pool Buffer pool to allocate from (NULL for default pool).
 *
 * @return Allocated buffer, or NULL on allocation failure.
 */
struct net_buf *bt_map_mce_mas_create_pdu(struct bt_map_mce_mas *mce_mas,
					  struct net_buf_pool *pool);

/** @brief Send OBEX connect request
 *
 * Initiates OBEX session establishment with the MAS.
 * Must be called after transport connection is established.
 * Once OBEX connect response received, the connect callback @ref bt_map_mce_mas_cb::connect will be
 * called.
 *
 * @param mce_mas MAS client instance.
 * @param buf Buffer containing connect headers (target UUID, supported features, etc.).
 *            If NULL, a default connect request is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_connect(struct bt_map_mce_mas *mce_mas, struct net_buf *buf);

/** @brief Send OBEX disconnect request
 *
 * Initiates OBEX session termination.
 * Once OBEX disconnect response received, the disconnect callback @ref
 * bt_map_mce_mas_cb::disconnect will be called.
 *
 * @param mce_mas MAS client instance.
 * @param buf Buffer containing disconnect headers (connection ID, etc.).
 *            If NULL, a default disconnect request is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_disconnect(struct bt_map_mce_mas *mce_mas, struct net_buf *buf);

/** @brief Send OBEX abort request
 *
 * Aborts the currently ongoing operation.
 * Once OBEX abort response received, the abort callback @ref bt_map_mce_mas_cb::abort will be
 * called.
 *
 * @param mce_mas MAS client instance.
 * @param buf Buffer containing abort headers (connection ID, etc.).
 *            If NULL, a default abort request is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_abort(struct bt_map_mce_mas *mce_mas, struct net_buf *buf);

/** @brief Send set folder request
 *
 * Navigates the folder hierarchy on the MAS.
 * Once set_folder response received, the set_folder callback @ref
 * bt_map_mce_mas_cb::set_folder will be called.
 *
 * @param mce_mas MAS client instance.
 * @param flags Navigation flags (see @ref bt_map_set_folder_flags).
 * @param buf Buffer containing folder name (for DOWN) and headers.
 *            Must include connection ID.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_set_folder(struct bt_map_mce_mas *mce_mas, uint8_t flags, struct net_buf *buf);

/** @brief Send set notification registration request
 *
 * Enables or disables event notifications from the MAS.
 * Once set_ntf_reg response received, the set_ntf_reg callback @ref
 * bt_map_mce_mas_cb::set_ntf_reg will be called.
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing notification status and headers.
 *            Must include connection ID, type header, and application parameters.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_set_ntf_reg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send get folder listing request
 *
 * Retrieves list of subfolders in the current folder.
 * Once get_folder_listing response received, the get_folder_listing callback @ref
 * bt_map_mce_mas_cb::get_folder_listing will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing filter parameters and headers.
 *            Must include connection ID and type header for initial request.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_get_folder_listing(struct bt_map_mce_mas *mce_mas, bool final,
				      struct net_buf *buf);

/** @brief Send get message listing request
 *
 * Retrieves list of messages in the current folder.
 * Once get_msg_listing response received, the get_msg_listing callback @ref
 * bt_map_mce_mas_cb::get_msg_listing will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing folder name, filter parameters, and headers.
 *            Must include connection ID, type header, and name header for initial request.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_get_msg_listing(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send get message request
 *
 * Retrieves a specific message by handle.
 * Once get_msg response received, the get_msg callback @ref bt_map_mce_mas_cb::get_msg will be
 * called (possibly multiple times).
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing message handle, parameters, and headers.
 *            Must include connection ID, type header, and name header for initial request.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_get_msg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send set message status request
 *
 * Modifies the status of a message (read/deleted/etc).
 * Once set_msg_status response received, the set_msg_status callback @ref
 * bt_map_mce_mas_cb::set_msg_status will be called.
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing message handle, status parameters, and headers.
 *            Must include connection ID, type header, name header, and application parameters.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_set_msg_status(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send push message request
 *
 * Uploads a new message to the server.
 * Once push_msg response received, the push_msg callback @ref bt_map_mce_mas_cb::push_msg will
 * be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the last packet, false if more data follows.
 * @param buf Buffer containing message data (bMessage format) and headers.
 *            Must include connection ID, type header, name header, and application parameters
 *            for initial request. Must include end-of-body header when final is true.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_push_msg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send update inbox request
 *
 * Requests the server to check for new messages.
 * Once update_inbox response received, the update_inbox callback @ref
 * bt_map_mce_mas_cb::update_inbox will be called.
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing headers.
 *            Must include connection ID and type header.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_update_inbox(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @brief Send get MAS instance info request
 *
 * Retrieves information about the MAS instance.
 * Once get_mas_inst_info response received, the get_mas_inst_info callback @ref
 * bt_map_mce_mas_cb::get_mas_inst_info will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing instance ID and headers.
 *            Must include connection ID, type header, and application parameters for initial
 *            request.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_get_mas_inst_info(struct bt_map_mce_mas *mce_mas, bool final,
				     struct net_buf *buf);

/** @brief Send set owner status request
 *
 * Sets the owner's presence and chat state.
 * Once set_owner_status response received, the set_owner_status callback @ref
 * bt_map_mce_mas_cb::set_owner_status will be called.
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing owner status data and headers.
 *            Must include connection ID, type header, and application parameters.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_set_owner_status(struct bt_map_mce_mas *mce_mas, bool final,
				    struct net_buf *buf);

/** @brief Send get owner status request
 *
 * Retrieves the owner's presence and chat state.
 * Once get_owner_status response received, the get_owner_status callback @ref
 * bt_map_mce_mas_cb::get_owner_status will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing headers.
 *            Must include connection ID and type header for initial request.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_get_owner_status(struct bt_map_mce_mas *mce_mas, bool final,
				    struct net_buf *buf);

/** @brief Send get conversation listing request
 *
 * Retrieves list of conversations.
 * Once get_convo_listing response received, the get_convo_listing callback @ref
 * bt_map_mce_mas_cb::get_convo_listing will be called (possibly multiple times).
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing filter parameters and headers.
 *            Must include connection ID and type header for initial request.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_get_convo_listing(struct bt_map_mce_mas *mce_mas, bool final,
				     struct net_buf *buf);

/** @brief Send set notification filter request
 *
 * Configures which event types to receive notifications for.
 * Once set_ntf_filter response received, the set_ntf_filter callback @ref
 * bt_map_mce_mas_cb::set_ntf_filter will be called.
 *
 * @param mce_mas MAS client instance.
 * @param final True if this is the final packet, false if more data follows.
 * @param buf Buffer containing filter mask and headers.
 *            Must include connection ID, type header, and application parameters.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mas_set_ntf_filter(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf);

/** @} */ /* end of bt_map_mce_mas */

/**
 * @brief Messaging Client Equipment(MCE) OBEX Server
 * @defgroup bt_map_mce_mns Messaging Client Equipment(MCE) OBEX Server
 * @ingroup bt_map
 * @{
 */

/** @brief Forward declaration of MAP Client MNS structure */
struct bt_map_mce_mns;

/** @brief MAP Client MNS RFCOMM server
 *
 * This structure represents an RFCOMM server for MAP Client MNS.
 */
struct bt_map_mce_mns_rfcomm_server {
	/** GOEP RFCOMM transport server - underlying transport layer */
	struct bt_goep_transport_rfcomm_server server;
	/** @brief Accept callback for incoming connections
	 *
	 * Called when a remote MSE attempts to connect to the MNS server.
	 *
	 * @param conn Bluetooth connection object.
	 * @param server RFCOMM server instance that received the connection.
	 * @param mce_mns Output parameter to store the allocated MNS instance.
	 *
	 * @return 0 on success, negative error code to reject the connection.
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
	/** @brief Accept callback for incoming connections
	 *
	 * Called when a remote MSE attempts to connect to the MNS server.
	 *
	 * @param conn Bluetooth connection object.
	 * @param server L2CAP server instance that received the connection.
	 * @param mce_mns Output parameter to store the allocated MNS instance.
	 *
	 * @return 0 on success, negative error code to reject the connection.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_map_mce_mns_l2cap_server *server,
		      struct bt_map_mce_mns **mce_mns);
};

/** @brief MAP Client MNS callbacks
 *
 * Callbacks for MAP Client MNS events.
 */
struct bt_map_mce_mns_cb {
	/** @brief RFCOMM transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object.
	 * @param mce_mns MNS server instance.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns);

	/** @brief RFCOMM transport disconnected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is closed.
	 *
	 * @param mce_mns MNS server instance.
	 */
	void (*rfcomm_disconnected)(struct bt_map_mce_mns *mce_mns);

	/** @brief L2CAP transport connected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object.
	 * @param mce_mns MNS server instance.
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns);

	/** @brief L2CAP transport disconnected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is closed.
	 *
	 * @param mce_mns MNS server instance.
	 */
	void (*l2cap_disconnected)(struct bt_map_mce_mns *mce_mns);

	/** @brief Connect callback
	 *
	 * Called when OBEX CONNECT request is received from remote MSE.
	 * Application should respond using bt_map_mce_mns_connect().
	 *
	 * @param mce_mns MNS server instance.
	 * @param version OBEX protocol version requested by client.
	 * @param mopl Maximum OBEX packet length requested by client.
	 * @param buf Buffer containing request headers (target UUID, etc.).
	 */
	void (*connect)(struct bt_map_mce_mns *mce_mns, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/** @brief Disconnect callback
	 *
	 * Called when OBEX DISCONNECT request is received.
	 * Application should respond using bt_map_mce_mns_disconnect().
	 *
	 * @param mce_mns MNS server instance.
	 * @param buf Buffer containing request headers.
	 */
	void (*disconnect)(struct bt_map_mce_mns *mce_mns, struct net_buf *buf);

	/** @brief Abort callback
	 *
	 * Called when OBEX ABORT request is received.
	 * Application should respond using bt_map_mce_mns_abort().
	 *
	 * @param mce_mns MNS server instance.
	 * @param buf Buffer containing request headers.
	 */
	void (*abort)(struct bt_map_mce_mns *mce_mns, struct net_buf *buf);

	/** @brief Send event callback
	 *
	 * Called when SendEvent request is received from remote MSE.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mce_mns_send_event().
	 *
	 * @param mce_mns MNS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing event report XML data and headers.
	 */
	void (*send_event)(struct bt_map_mce_mns *mce_mns, bool final, struct net_buf *buf);
};

/** @brief MAP Client MNS instance structure
 *
 * Represents a server that receives event notifications from a Message Notification Server.
 * The MCE acts as a server for the MNS connection.
 */
struct bt_map_mce_mns {
	/** @brief Underlying GOEP instance */
	struct bt_goep goep;

	/** @internal Callbacks */
	const struct bt_map_mce_mns_cb *_cb;

	/** @internal Transport type */
	uint8_t _transport_type;

	/** @internal Transport state (atomic) */
	atomic_t _transport_state;

	/** @internal Connection ID */
	uint32_t _conn_id;

	/** @internal Underlying OBEX server */
	struct bt_obex_server _server;

	/** @internal Server state (atomic) */
	atomic_t _state;

	/** @internal Pending request callback */
	void (*_req_cb)(struct bt_map_mce_mns *mce_mns, bool final, struct net_buf *buf);

	/** @internal Operation type */
	const char *_optype;

	/** @internal Operation code */
	uint8_t _opcode;
};

/** @brief Register callbacks for MAP Client MNS
 *
 * Registers callback functions to handle MNS events and requests.
 * Must be called before accepting any MNS connections.
 *
 * @param mce_mns MNS server instance.
 * @param cb Pointer to callback structure (must remain valid).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_cb_register(struct bt_map_mce_mns *mce_mns, const struct bt_map_mce_mns_cb *cb);

/** @brief Register MAP Client MNS RFCOMM server
 *
 * Registers an RFCOMM server to accept incoming MNS connections.
 * The server channel is allocated automatically or can be specified.
 *
 * @param server RFCOMM server structure with accept callback configured.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_rfcomm_register(struct bt_map_mce_mns_rfcomm_server *server);

/** @brief Disconnect MAP Client MNS over RFCOMM
 *
 * Closes the RFCOMM transport connection.
 * On success, the rfcomm_disconnected callback @ref bt_map_mce_mns_cb::rfcomm_disconnected will be
 * called.
 *
 * @param mce_mns MNS server instance.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_rfcomm_disconnect(struct bt_map_mce_mns *mce_mns);

/** @brief Register MAP Client MNS L2CAP server
 *
 * Registers an L2CAP server to accept incoming MNS connections.
 * The PSM is allocated automatically or can be specified.
 *
 * @param server L2CAP server structure with accept callback configured.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_l2cap_register(struct bt_map_mce_mns_l2cap_server *server);

/** @brief Disconnect MAP Client MNS over L2CAP
 *
 * Closes the L2CAP transport connection.
 * On success, the l2cap_disconnected callback @ref bt_map_mce_mns_cb::l2cap_disconnected will be
 * called.
 *
 * @param mce_mns MNS server instance.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_l2cap_disconnect(struct bt_map_mce_mns *mce_mns);

/** @brief Create PDU for MAP Client MNS
 *
 * Allocates a buffer for building MAP response packets.
 * The buffer is sized appropriately for the negotiated MTU.
 *
 * @param mce_mns MNS server instance.
 * @param pool Buffer pool to allocate from (NULL for default pool).
 *
 * @return Allocated buffer, or NULL on failure.
 */
struct net_buf *bt_map_mce_mns_create_pdu(struct bt_map_mce_mns *mce_mns,
					  struct net_buf_pool *pool);

/** @brief Send OBEX connect response
 *
 * Responds to an OBEX CONNECT request from the remote MSE.
 * Called from the connect callback @ref bt_map_mce_mns_cb::connect.
 *
 * @param mce_mns MNS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS to accept).
 * @param buf Buffer containing response headers (WHO, connection ID, etc.).
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_connect(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send OBEX disconnect response
 *
 * Responds to an OBEX DISCONNECT request from the remote MSE.
 * Called from the disconnect callback @ref bt_map_mce_mns_cb::disconnect.
 *
 * @param mce_mns MNS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS to accept).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_disconnect(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code,
			      struct net_buf *buf);

/** @brief Send OBEX abort response
 *
 * Responds to an OBEX ABORT request from the remote MSE.
 * Called from the abort callback @ref bt_map_mce_mns_cb::abort.
 *
 * @param mce_mns MNS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS to accept).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_abort(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send event response
 *
 * Responds to a SendEvent request from the remote MSE.
 * Called from the send_event callback @ref bt_map_mce_mns_cb::send_event.
 *
 * @param mce_mns MNS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 @ref BT_OBEX_RSP_CODE_SUCCESS for complete).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mce_mns_send_event(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code,
			      struct net_buf *buf);

/** @} */ /* end of bt_map_mce_mns */

/**
 * @brief Messaging Server Equipment(MSE) OBEX Server
 * @defgroup bt_map_mse_mas Messaging Server Equipment(MSE) OBEX Server
 * @ingroup bt_map
 * @{
 */

/** @brief Forward declaration of MAP Server MAS structure */
struct bt_map_mse_mas;

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
	 * @param conn Bluetooth connection object.
	 * @param server RFCOMM server instance that received the connection.
	 * @param mse_mas Output parameter to store the allocated MAS instance.
	 *
	 * @return 0 on success, negative error code to reject the connection.
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
	 * @param conn Bluetooth connection object.
	 * @param server L2CAP server instance that received the connection.
	 * @param mse_mas Output parameter to store the allocated MAS instance.
	 *
	 * @return 0 on success, negative error code to reject the connection.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_map_mse_mas_l2cap_server *server,
		      struct bt_map_mse_mas **mse_mas);
};

/** @brief MAP Server MAS callbacks
 *
 * Callbacks for MAP Server MAS events.
 */
struct bt_map_mse_mas_cb {
	/** @brief RFCOMM transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object.
	 * @param mse_mas MAS server instance.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_map_mse_mas *mse_mas);

	/** @brief RFCOMM transport disconnected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is closed.
	 *
	 * @param mse_mas MAS server instance.
	 */
	void (*rfcomm_disconnected)(struct bt_map_mse_mas *mse_mas);

	/** @brief L2CAP transport connected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object.
	 * @param mse_mas MAS server instance.
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_map_mse_mas *mse_mas);

	/** @brief L2CAP transport disconnected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is closed.
	 *
	 * @param mse_mas MAS server instance.
	 */
	void (*l2cap_disconnected)(struct bt_map_mse_mas *mse_mas);

	/** @brief Connect callback
	 *
	 * Called when OBEX CONNECT request is received from remote MCE.
	 * Application should respond using bt_map_mse_mas_connect().
	 *
	 * @param mse_mas MAS server instance.
	 * @param version OBEX protocol version requested by client.
	 * @param mopl Maximum OBEX packet length requested by client.
	 * @param buf Buffer containing request headers (target UUID, supported features, etc.).
	 */
	void (*connect)(struct bt_map_mse_mas *mse_mas, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/** @brief Disconnect callback
	 *
	 * Called when OBEX DISCONNECT request is received.
	 * Application should respond using bt_map_mse_mas_disconnect().
	 *
	 * @param mse_mas MAS server instance.
	 * @param buf Buffer containing request headers.
	 */
	void (*disconnect)(struct bt_map_mse_mas *mse_mas, struct net_buf *buf);

	/** @brief Abort callback
	 *
	 * Called when OBEX ABORT request is received.
	 * Application should respond using bt_map_mse_mas_abort().
	 *
	 * @param mse_mas MAS server instance.
	 * @param buf Buffer containing request headers.
	 */
	void (*abort)(struct bt_map_mse_mas *mse_mas, struct net_buf *buf);

	/** @brief Set notification registration callback
	 *
	 * Called when SetNotificationRegistration request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_set_ntf_reg().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing notification status and headers.
	 */
	void (*set_ntf_reg)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Set folder callback
	 *
	 * Called when SetFolder request is received.
	 * Application should respond using bt_map_mse_mas_set_folder().
	 *
	 * @param mse_mas MAS server instance.
	 * @param flags Navigation flags (see @ref bt_map_set_folder_flags).
	 * @param buf Buffer containing folder name (for DOWN) and headers.
	 */
	void (*set_folder)(struct bt_map_mse_mas *mse_mas, uint8_t flags, struct net_buf *buf);

	/** @brief Get folder listing callback
	 *
	 * Called when GetFolderListing request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_folder_listing().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing filter parameters and headers.
	 */
	void (*get_folder_listing)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Get message listing callback
	 *
	 * Called when GetMessageListing request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_msg_listing().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing folder name, filter parameters, and headers.
	 */
	void (*get_msg_listing)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Get message callback
	 *
	 * Called when GetMessage request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_msg().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing message handle, parameters, and headers.
	 */
	void (*get_msg)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Set message status callback
	 *
	 * Called when SetMessageStatus request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_set_msg_status().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing message handle, status parameters, and headers.
	 */
	void (*set_msg_status)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Push message callback
	 *
	 * Called when PushMessage request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_push_msg().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing message data (bMessage format) and headers.
	 */
	void (*push_msg)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Update inbox callback
	 *
	 * Called when UpdateInbox request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_update_inbox().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing headers.
	 */
	void (*update_inbox)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Get MAS instance info callback
	 *
	 * Called when GetMASInstanceInformation request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_mas_inst_info().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing instance ID and headers.
	 */
	void (*get_mas_inst_info)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Set owner status callback
	 *
	 * Called when SetOwnerStatus request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_set_owner_status().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing owner status data and headers.
	 */
	void (*set_owner_status)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Get owner status callback
	 *
	 * Called when GetOwnerStatus request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_owner_status().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing headers.
	 */
	void (*get_owner_status)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Get conversation listing callback
	 *
	 * Called when GetConversationListing request is received.
	 * May be called multiple times if client requests continuation.
	 * Application should respond using bt_map_mse_mas_get_convo_listing().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing filter parameters and headers.
	 */
	void (*get_convo_listing)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @brief Set notification filter callback
	 *
	 * Called when SetNotificationFilter request is received.
	 * May be called multiple times if request is fragmented.
	 * Application should respond using bt_map_mse_mas_set_ntf_filter().
	 *
	 * @param mse_mas MAS server instance.
	 * @param final True if this is the final packet, false if more data follows.
	 * @param buf Buffer containing filter mask and headers.
	 */
	void (*set_ntf_filter)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);
};

/** @brief MAP Server MAS instance structure
 *
 * Represents a server that handles message access requests from clients.
 * The MSE acts as a server for the MAS connection.
 */
struct bt_map_mse_mas {
	/** @brief Underlying GOEP instance */
	struct bt_goep goep;

	/** @internal Callbacks */
	const struct bt_map_mse_mas_cb *_cb;

	/** @internal Transport type */
	uint8_t _transport_type;

	/** @internal Transport state (atomic) */
	atomic_t _transport_state;

	/** @internal Connection ID */
	uint32_t _conn_id;

	/** @internal Underlying OBEX server */
	struct bt_obex_server _server;

	/** @internal Server state (atomic) */
	atomic_t _state;

	/** @internal Pending request callback */
	void (*_req_cb)(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf);

	/** @internal Operation type */
	const char *_optype;

	/** @internal Operation code */
	uint8_t _opcode;
};

/** @brief Register callbacks for MAP Server MAS
 *
 * Registers callback functions to handle MAS events and requests.
 * Must be called before accepting any MAS connections.
 *
 * @param mse_mas MAS server instance.
 * @param cb Pointer to callback structure (must remain valid).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_cb_register(struct bt_map_mse_mas *mse_mas, const struct bt_map_mse_mas_cb *cb);

/** @brief Register MAP Server MAS RFCOMM server
 *
 * Registers an RFCOMM server to accept incoming MAS connections.
 * The server channel is allocated automatically or can be specified.
 *
 * @param server RFCOMM server structure with accept callback configured.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_rfcomm_register(struct bt_map_mse_mas_rfcomm_server *server);

/** @brief Disconnect MAP Server MAS over RFCOMM
 *
 * Closes the RFCOMM transport connection.
 * On success, the rfcomm_disconnected callback @ref bt_map_mse_mas_cb::rfcomm_disconnected will be
 * called.
 *
 * @param mse_mas MAS server instance.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_rfcomm_disconnect(struct bt_map_mse_mas *mse_mas);

/** @brief Register MAP Server MAS L2CAP server
 *
 * Registers an L2CAP server to accept incoming MAS connections.
 * The PSM is allocated automatically or can be specified.
 *
 * @param server L2CAP server structure with accept callback configured.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_l2cap_register(struct bt_map_mse_mas_l2cap_server *server);

/** @brief Disconnect MAP Server MAS over L2CAP
 *
 * Closes the L2CAP transport connection.
 * On success, the l2cap_disconnected callback @ref bt_map_mse_mas_cb::l2cap_disconnected will be
 * called.
 *
 * @param mse_mas MAS server instance.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_l2cap_disconnect(struct bt_map_mse_mas *mse_mas);

/** @brief Create PDU for MAP Server MAS
 *
 * Allocates a buffer for building MAP response packets.
 * The buffer is sized appropriately for the negotiated MTU.
 *
 * @param mse_mas MAS server instance.
 * @param pool Buffer pool to allocate from (NULL for default pool).
 *
 * @return Allocated buffer, or NULL on allocation failure.
 */
struct net_buf *bt_map_mse_mas_create_pdu(struct bt_map_mse_mas *mse_mas,
					  struct net_buf_pool *pool);

/** @brief Send OBEX connect response
 *
 * Responds to an OBEX CONNECT request from the remote MCE.
 * Called from the connect callback @ref bt_map_mse_mas_cb::connect.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS to accept).
 * @param buf Buffer containing response headers (WHO, connection ID, etc.).
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_connect(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send OBEX disconnect response
 *
 * Responds to an OBEX DISCONNECT request from the remote MCE.
 * Called from the disconnect callback @ref bt_map_mse_mas_cb::disconnect.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS to accept).
 * @param buf Buffer containing response headers.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_disconnect(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
			      struct net_buf *buf);

/** @brief Send OBEX abort response
 *
 * Responds to an OBEX ABORT request from the remote MCE.
 * Called from the abort callback @ref bt_map_mse_mas_cb::abort.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS to accept).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_abort(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send set folder response
 *
 * Responds to a SetPath request from the remote MCE.
 * Called from the set_folder callback @ref bt_map_mse_mas_cb::set_folder.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS on success).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_set_folder(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
			      struct net_buf *buf);

/** @brief Send set notification registration response
 *
 * Responds to a SetNotificationRegistration request from the remote MCE.
 * Called from the set_ntf_reg callback @ref bt_map_mse_mas_cb::set_ntf_reg.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS on success).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_set_ntf_reg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
			       struct net_buf *buf);

/** @brief Send get folder listing response
 *
 * Responds to a GetFolderListing request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_folder_listing callback @ref bt_map_mse_mas_cb::get_folder_listing.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 @ref BT_OBEX_RSP_CODE_SUCCESS for complete).
 * @param buf Buffer containing folder listing XML data and headers.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_get_folder_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				      struct net_buf *buf);

/** @brief Send get message listing response
 *
 * Responds to a GetMessageListing request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_msg_listing callback @ref bt_map_mse_mas_cb::get_msg_listing.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 @ref BT_OBEX_RSP_CODE_SUCCESS for complete).
 * @param buf Buffer containing message listing XML data and headers.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_get_msg_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				   struct net_buf *buf);

/** @brief Send get message response
 *
 * Responds to a GetMessage request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_msg callback @ref bt_map_mse_mas_cb::get_msg.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 @ref BT_OBEX_RSP_CODE_SUCCESS for complete).
 * @param buf Buffer containing message bMessage data and headers.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_get_msg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send set message status response
 *
 * Responds to a SetMessageStatus request from the remote MCE.
 * Called from the set_msg_status callback @ref bt_map_mse_mas_cb::set_msg_status.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS on success).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_set_msg_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				  struct net_buf *buf);

/** @brief Send push message response
 *
 * Responds to a PushMessage request from the remote MCE.
 * May be called multiple times if request is fragmented.
 * Called from the push_msg callback @ref bt_map_mse_mas_cb::push_msg.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 @ref BT_OBEX_RSP_CODE_SUCCESS for complete).
 * @param buf Buffer containing message handle (on success) and headers.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_push_msg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send update inbox response
 *
 * Responds to an UpdateInbox request from the remote MCE.
 * Called from the update_inbox callback @ref bt_map_mse_mas_cb::update_inbox.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS on success).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_update_inbox(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				struct net_buf *buf);

/** @brief Send get MAS instance info response
 *
 * Responds to a GetMASInstanceInformation request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_mas_inst_info callback @ref bt_map_mse_mas_cb::get_mas_inst_info.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 @ref BT_OBEX_RSP_CODE_SUCCESS for complete).
 * @param buf Buffer containing instance information XML data and headers.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_get_mas_inst_info(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				     struct net_buf *buf);

/** @brief Send set owner status response
 *
 * Responds to a SetOwnerStatus request from the remote MCE.
 * Called from the set_owner_status callback @ref bt_map_mse_mas_cb::set_owner_status.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS on success).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_set_owner_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				    struct net_buf *buf);

/** @brief Send get owner status response
 *
 * Responds to a GetOwnerStatus request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_owner_status callback @ref bt_map_mse_mas_cb::get_owner_status.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 @ref BT_OBEX_RSP_CODE_SUCCESS for complete).
 * @param buf Buffer containing owner status data and headers.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_get_owner_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				    struct net_buf *buf);

/** @brief Send get conversation listing response
 *
 * Responds to a GetConversationListing request from the remote MCE.
 * May be called multiple times to send fragmented response.
 * Called from the get_convo_listing callback @ref bt_map_mse_mas_cb::get_convo_listing.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for partial,
 *                 @ref BT_OBEX_RSP_CODE_SUCCESS for complete).
 * @param buf Buffer containing conversation listing XML data and headers.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_get_convo_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				     struct net_buf *buf);

/** @brief Send set notification filter response
 *
 * Responds to a SetNotificationFilter request from the remote MCE.
 * Called from the set_ntf_filter callback @ref bt_map_mse_mas_cb::set_ntf_filter.
 *
 * @param mse_mas MAS server instance.
 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS on success).
 * @param buf Buffer containing response headers.
 *            If NULL, a default response is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mas_set_ntf_filter(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				  struct net_buf *buf);

/** @} */ /* end of bt_map_mse_mas */

/**
 * @brief Messaging Server Equipment(MSE) OBEX Client
 * @defgroup bt_map_mse_mns Messaging Server Equipment(MSE) OBEX Client
 * @ingroup bt_map
 * @{
 */

/** @brief Forward declaration of MAP Server MNS structure */
struct bt_map_mse_mns;

/** @brief MAP Server MNS callbacks
 *
 * Callbacks for MAP Server MNS events.
 */
struct bt_map_mse_mns_cb {
	/** @brief RFCOMM transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object.
	 * @param mse_mns MNS client instance.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns);

	/** @brief RFCOMM transport disconnected callback
	 *
	 * Called when the underlying transport (RFCOMM) connection is closed.
	 * All pending operations are cancelled.
	 *
	 * @param mse_mns MNS client instance.
	 */
	void (*rfcomm_disconnected)(struct bt_map_mse_mns *mse_mns);

	/** @brief L2CAP transport connected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is established.
	 * OBEX connection has not yet been negotiated at this point.
	 *
	 * @param conn Bluetooth connection object.
	 * @param mse_mns MNS client instance.
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns);

	/** @brief L2CAP transport disconnected callback
	 *
	 * Called when the underlying transport (L2CAP) connection is closed.
	 * All pending operations are cancelled.
	 *
	 * @param mse_mns MNS client instance.
	 */
	void (*l2cap_disconnected)(struct bt_map_mse_mns *mse_mns);

	/** @brief Connect callback
	 *
	 * Called when OBEX CONNECT response is received.
	 *
	 * @param mse_mns MNS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_SUCCESS on
	 * success).
	 * @param version OBEX protocol version supported by server.
	 * @param mopl Maximum OBEX packet length supported by server.
	 * @param buf Buffer containing additional response headers (connection ID, etc.).
	 */
	void (*connect)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, uint8_t version,
			uint16_t mopl, struct net_buf *buf);

	/** @brief Disconnect callback
	 *
	 * Called when OBEX DISCONNECT response is received.
	 *
	 * @param mse_mns MNS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*disconnect)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Abort callback
	 *
	 * Called when OBEX ABORT response is received.
	 * The aborted operation is cancelled.
	 *
	 * @param mse_mns MNS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code.
	 * @param buf Buffer containing response headers.
	 */
	void (*abort)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Send event callback
	 *
	 * Called when SendEvent response is received.
	 * May be called multiple times if request is fragmented (@ref BT_OBEX_RSP_CODE_CONTINUE).
	 *
	 * @param mse_mns MNS client instance.
	 * @param rsp_code Response code @ref bt_obex_rsp_code (@ref BT_OBEX_RSP_CODE_CONTINUE for
	 * partial upload).
	 * @param buf Buffer containing response headers.
	 */
	void (*send_event)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf);
};

/** @brief MAP Server MNS instance structure
 *
 * Represents a client connection to a Message Notification Server.
 * Used for sending event notifications to the MCE.
 */
struct bt_map_mse_mns {
	/** @brief Underlying GOEP instance */
	struct bt_goep goep;

	/** @internal Callbacks */
	const struct bt_map_mse_mns_cb *_cb;

	/** @internal Transport type */
	uint8_t _transport_type;

	/** @internal Transport state (atomic) */
	atomic_t _transport_state;

	/** @internal Connection ID */
	uint32_t _conn_id;

	/** @internal Underlying OBEX client */
	struct bt_obex_client _client;

	/** @internal Client state (atomic) */
	atomic_t _state;

	/** @internal Pending response callback */
	void (*_rsp_cb)(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf);

	/** @internal Current request function type */
	const char *_req_type;
};

/** @brief Register callbacks for MAP Server MNS
 *
 * Registers callback functions to handle MNS events and responses.
 * Must be called before initiating any MNS operations.
 *
 * @param mse_mns MNS client instance.
 * @param cb Pointer to callback structure (must remain valid).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_cb_register(struct bt_map_mse_mns *mse_mns, const struct bt_map_mse_mns_cb *cb);

/** @brief Connect MAP Server MNS over RFCOMM
 *
 * Initiates RFCOMM transport connection to a remote MNS.
 * On success, the rfcomm_connected callback @ref bt_map_mse_mns_cb::rfcomm_connected will be
 * called.
 *
 * @param conn Bluetooth connection to remote device.
 * @param mse_mns MNS client instance.
 * @param channel RFCOMM server channel (from SDP discovery).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_rfcomm_connect(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns,
				  uint8_t channel);

/** @brief Disconnect MAP Server MNS over RFCOMM
 *
 * Closes the RFCOMM transport connection.
 * On success, the rfcomm_disconnected callback @ref bt_map_mse_mns_cb::rfcomm_disconnected will be
 * called.
 *
 * @param mse_mns MNS client instance.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_rfcomm_disconnect(struct bt_map_mse_mns *mse_mns);

/** @brief Connect MAP Server MNS over L2CAP
 *
 * Initiates L2CAP transport connection to a remote MNS.
 * On success, the l2cap_connected callback @ref bt_map_mse_mns_cb::l2cap_connected will be called.
 *
 * @param conn Bluetooth connection to remote device.
 * @param mse_mns MNS client instance.
 * @param psm L2CAP PSM (from SDP discovery).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_l2cap_connect(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns,
				 uint16_t psm);

/** @brief Disconnect MAP Server MNS over L2CAP
 *
 * Closes the L2CAP transport connection.
 * On success, the l2cap_disconnected callback @ref bt_map_mse_mns_cb::l2cap_disconnected will be
 * called.
 *
 * @param mse_mns MNS client instance.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_l2cap_disconnect(struct bt_map_mse_mns *mse_mns);

/** @brief Create PDU for MAP Server MNS
 *
 * Allocates a buffer for building MAP request packets.
 * The buffer is sized appropriately for the negotiated MTU.
 *
 * @param mse_mns MNS client instance.
 * @param pool Buffer pool to allocate from (NULL for default pool).
 *
 * @return Allocated buffer, or NULL on allocation failure.
 */
struct net_buf *bt_map_mse_mns_create_pdu(struct bt_map_mse_mns *mse_mns,
					  struct net_buf_pool *pool);

/** @brief Send OBEX connect request
 *
 * Initiates OBEX session establishment with the MNS.
 * Must be called after transport connection is established.
 * Once OBEX connect response received, the connect callback @ref bt_map_mse_mns_cb::connect will be
 * called.
 *
 * @param mse_mns MNS client instance.
 * @param buf Buffer containing connect headers (target UUID, etc.).
 *            If NULL, a default connect request is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_connect(struct bt_map_mse_mns *mse_mns, struct net_buf *buf);

/** @brief Send OBEX disconnect request
 *
 * Initiates OBEX session termination.
 * Once OBEX disconnect response received, the disconnect callback @ref
 * bt_map_mse_mns_cb::disconnect will be called.
 *
 * @param mse_mns MNS client instance.
 * @param buf Buffer containing disconnect headers (connection ID, etc.).
 *            If NULL, a default disconnect request is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_disconnect(struct bt_map_mse_mns *mse_mns, struct net_buf *buf);

/** @brief Send OBEX abort request
 *
 * Aborts the currently ongoing operation.
 * Once OBEX abort response received, the abort callback @ref bt_map_mse_mns_cb::abort will be
 * called.
 *
 * @param mse_mns MNS client instance.
 * @param buf Buffer containing abort headers (connection ID, etc.).
 *            If NULL, a default abort request is sent.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_abort(struct bt_map_mse_mns *mse_mns, struct net_buf *buf);

/** @brief Send event notification request
 *
 * Sends an event notification to the remote MCE.
 * May be called multiple times to send fragmented event data.
 * Once send_event response received, the send_event callback @ref
 * bt_map_mse_mns_cb::send_event will be called (possibly multiple times).
 *
 * @param mse_mns MNS client instance.
 * @param final True if this is the last packet, false if more data follows.
 * @param buf Buffer containing event report XML data and headers.
 *            Must include connection ID, type header, and application parameters
 *            for initial request. Must include end-of-body header when final is true.
 *            If success returned, the function has taken the ownership of @p buf.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_map_mse_mns_send_event(struct bt_map_mse_mns *mse_mns, bool final, struct net_buf *buf);

/** @} */ /* end of bt_map_mse_mns */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MAP_H_ */
