/** @file
 *  @brief Phone Book Access Profile handling.
 */

/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_PBAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_PBAP_H_

/**
 * @brief Phone Book Access Profile (PBAP)
 * @defgroup bt_pbap Phone Book Access Profile (PBAP)
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <zephyr/bluetooth/classic/goep.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PBAP target UUID for service identification.
 *
 *  128-bit UUID (796135f0-f0c5-11d8-0966-0800200c9a66) used to identify
 *  the PBAP service during OBEX connection establishment.
 */
#define BT_PBAP_UUID                                                                               \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0x796135f0, 0xf0c5, 0x11d8, 0x0966, 0x0800200c9a66))

/** @brief OBEX type header for pull phone book operation. */
#define BT_PBAP_PULL_PHONE_BOOK_TYPE "x-bt/phonebook"

/** @brief OBEX type header for pull vCard listing operation. */
#define BT_PBAP_PULL_VCARD_LISTING_TYPE "x-bt/vcard-listing"

/** @brief OBEX type header for pull vCard entry operation. */
#define BT_PBAP_PULL_VCARD_ENTRY_TYPE "x-bt/vcard"

/** @brief Set phone book flags: Navigate up to parent directory.
 *
 *  Combination of OBEX setpath flags used to navigate to the parent directory
 *  in the phone book hierarchy.
 */
#define BT_PBAP_SET_PHONE_BOOK_FLAGS_UP                                                            \
	(BT_OBEX_SETPATH_FLAG_BACKUP | BT_OBEX_SETPATH_FLAG_NO_CREATE)

/** @brief Set phone book flags: Navigate down to child directory or to root.
 *
 *  OBEX setpath flag used to navigate down to a child directory or to the root
 *  directory in the phone book hierarchy.
 */
#define BT_PBAP_SET_PHONE_BOOK_FLAGS_DOWN_OR_ROOT BT_OBEX_SETPATH_FLAG_NO_CREATE

/** @brief PBAP response codes.
 *
 *  Response codes used in PBAP operations, mapped from OBEX response codes.
 *  These indicate the status of PBAP requests and operations.
 */
enum __packed bt_pbap_rsp_code {
	/** Continue. */
	BT_PBAP_RSP_CODE_CONTINUE = BT_OBEX_RSP_CODE_CONTINUE,
	/** OK. */
	BT_PBAP_RSP_CODE_OK = BT_OBEX_RSP_CODE_OK,
	/** Success. */
	BT_PBAP_RSP_CODE_SUCCESS = BT_OBEX_RSP_CODE_SUCCESS,
	/** Bad request - Server couldn't understand request. */
	BT_PBAP_RSP_CODE_BAD_REQ = BT_OBEX_RSP_CODE_BAD_REQ,
	/** Unauthorized. */
	BT_PBAP_RSP_CODE_UNAUTH = BT_OBEX_RSP_CODE_UNAUTH,
	/** Forbidden - Operation is understood but refused. */
	BT_PBAP_RSP_CODE_FORBIDDEN = BT_OBEX_RSP_CODE_FORBIDDEN,
	/** Not found. */
	BT_PBAP_RSP_CODE_NOT_FOUND = BT_OBEX_RSP_CODE_NOT_FOUND,
	/** Not acceptable. */
	BT_PBAP_RSP_CODE_NOT_ACCEPT = BT_OBEX_RSP_CODE_NOT_ACCEPT,
	/** Precondition failed. */
	BT_PBAP_RSP_CODE_PRECON_FAIL = BT_OBEX_RSP_CODE_PRECON_FAIL,
	/** Not implemented. */
	BT_PBAP_RSP_CODE_NOT_IMPL = BT_OBEX_RSP_CODE_NOT_IMPL,
	/** Service unavailable. */
	BT_PBAP_RSP_CODE_UNAVAIL = BT_OBEX_RSP_CODE_UNAVAIL,
};

/** @brief PBAP application parameter Order parameter values. */
enum bt_pbap_appl_param_order {
	/** Indexed order. */
	BT_PBAP_APPL_PARAM_ORDER_INDEXED = 0x00,
	/** Alphabetical order. */
	BT_PBAP_APPL_PARAM_ORDER_ALPHABETICAL = 0x01,
	/** Phonetic order. */
	BT_PBAP_APPL_PARAM_ORDER_PHONETIC = 0x02,
};

/** @brief PBAP application parameter Search Property parameter values. */
enum bt_pbap_appl_param_search_property {
	/** Name. */
	BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_NAME = 0x00,
	/** Number. */
	BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_NUMBER = 0x01,
	/** Sound. */
	BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_SOUND = 0x02,
};

/** @brief PBAP application parameter Format parameter values. */
enum bt_pbap_appl_param_format {
	/** vCard version 2.1. */
	BT_PBAP_APPL_PARAM_FORMAT_2_1 = 0x00,
	/** vCard version 3.0. */
	BT_PBAP_APPL_PARAM_FORMAT_3_0 = 0x01,
};

/** @brief PBAP application parameter vCard Selector Operator parameter values. */
enum bt_pbap_appl_param_vcard_selector_operator {
	/** OR operator. */
	BT_PBAP_APPL_PARAM_VCARD_SELECTOR_OPERATOR_OR = 0x00,
	/** AND operator. */
	BT_PBAP_APPL_PARAM_VCARD_SELECTOR_OPERATOR_AND = 0x01,
};

/** @brief PBAP application parameter reset neww missed calls parameter values. */
enum bt_pbap_appl_param_reset_new_missed_calls {
	/** Reset. */
	BT_PBAP_APPL_PARAM_RESET_NEW_MISSED_CALLS = 0x01,
};

/** @brief PBAP property bitmask. */
enum bt_pbap_appl_param_property_mask {
	/** vCard version. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_VERSION = BIT64(0),
	/** Formatted name. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_FN = BIT64(1),
	/** Structured presentation of name. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_N = BIT64(2),
	/** Associated image or photo. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_PHOTO = BIT64(3),
	/** Birthday. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_BDAY = BIT64(4),
	/** Delivery address. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_ADR = BIT64(5),
	/** Delivery address label. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_LABEL = BIT64(6),
	/** Telephone number. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_TEL = BIT64(7),
	/** Electronic mail address. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_EMAIL = BIT64(8),
	/** Email program name. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_MAILER = BIT64(9),
	/** Time zone. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_TZ = BIT64(10),
	/** Geographic position. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_GEO = BIT64(11),
	/** Job title. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_TITLE = BIT64(12),
	/** Role or occupation. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_ROLE = BIT64(13),
	/** Organization logo. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_LOGO = BIT64(14),
	/** vCard of person who acts as agent. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_AGENT = BIT64(15),
	/** Name of organization. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_ORG = BIT64(16),
	/** Comments. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_NOTE = BIT64(17),
	/** Revision. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_REV = BIT64(18),
	/** Pronunciation of name. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_SOUND = BIT64(19),
	/** Uniform resource locator. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_URL = BIT64(20),
	/** Unique ID. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_UID = BIT64(21),
	/** Public key. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_KEY = BIT64(22),
	/** Nickname. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_NICKNAME = BIT64(23),
	/** Categories. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_CATEGORIES = BIT64(24),
	/** Product ID. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_PRODID = BIT64(25),
	/** Class information. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_CLASS = BIT64(26),
	/** String used for sorting operations. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_SORT_STRING = BIT64(27),
	/** Time stamp of a missed call. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_X_IRMC_CALL_DATETIME = BIT64(28),
	/** Speed-dial shortcut. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_X_BT_SPEEDDIALKEY = BIT64(29),
	/** Bluetooth UCI (Unique Contact Identifier). */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_X_BT_UCI = BIT64(30),
	/** Bluetooth UID. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_X_BT_UID = BIT64(31),
	/** Proprietary filtering. */
	BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_PROPRIETARY_FILTER = BIT64(39),
};

/** @brief PBAP application parameter tag IDs. */
enum __packed bt_pbap_appl_param_tag_id {
	/** Order @ref bt_pbap_appl_param_order. */
	BT_PBAP_APPL_PARAM_TAG_ID_ORDER = 0x01,
	/** Search value. */
	BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_VALUE = 0x02,
	/** Search property @ref bt_pbap_appl_param_search_property. */
	BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_PROPERTY = 0x03,
	/** Max list count. */
	BT_PBAP_APPL_PARAM_TAG_ID_MAX_LIST_COUNT = 0x04,
	/** List start offset. */
	BT_PBAP_APPL_PARAM_TAG_ID_LIST_START_OFFSET = 0x05,
	/** Property selector @ref bt_pbap_appl_param_property_mask. */
	BT_PBAP_APPL_PARAM_TAG_ID_PROPERTY_SELECTOR = 0x06,
	/** Format @ref bt_pbap_appl_param_format. */
	BT_PBAP_APPL_PARAM_TAG_ID_FORMAT = 0x07,
	/** Phonebook size. */
	BT_PBAP_APPL_PARAM_TAG_ID_PHONEBOOK_SIZE = 0x08,
	/** New missed calls. */
	BT_PBAP_APPL_PARAM_TAG_ID_NEW_MISSED_CALLS = 0x09,
	/** Primary folder version. */
	BT_PBAP_APPL_PARAM_TAG_ID_PRIMARY_FOLDER_VERSION = 0x0a,
	/** Secondary folder version. */
	BT_PBAP_APPL_PARAM_TAG_ID_SECONDARY_FOLDER_VERSION = 0x0b,
	/** vCard selector @ref bt_pbap_appl_param_property_mask . */
	BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR = 0x0c,
	/** Database identifier. */
	BT_PBAP_APPL_PARAM_TAG_ID_DATABASE_IDENTIFIER = 0x0d,
	/** vCard selector operator @ref bt_pbap_appl_param_property_mask. */
	BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR_OPERATOR = 0x0e,
	/** Reset new missed calls @ref bt_pbap_appl_param_reset_new_missed_calls. */
	BT_PBAP_APPL_PARAM_TAG_ID_RESET_NEW_MISSED_CALLS = 0x0f,
	/** PBAP supported features @ref bt_pbap_supported_feature. */
	BT_PBAP_APPL_PARAM_TAG_ID_SUPPORTED_FEATURES = 0x10,
};

/** @brief PBAP supported features bitmask. */
enum bt_pbap_supported_feature {
	/** Download feature - Support for phone book download. */
	BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD = BIT(0),
	/** Browsing feature - Support for phone book browsing. */
	BT_PBAP_SUPPORTED_FEATURE_BROWSING = BIT(1),
	/** Database identifier feature - Support for database change tracking. */
	BT_PBAP_SUPPORTED_FEATURE_DATABASE_IDENTIFIER = BIT(2),
	/** Folder version counters feature - Support for folder version tracking. */
	BT_PBAP_SUPPORTED_FEATURE_FOLDER_VERSION_COUNTERS = BIT(3),
	/** vCard selector feature - Support for vCard property filtering. */
	BT_PBAP_SUPPORTED_FEATURE_VCARD_SELECTOR = BIT(4),
	/** Enhanced missed calls feature - Support for enhanced missed call handling. */
	BT_PBAP_SUPPORTED_FEATURE_ENHANCED_MISSED_CALLS = BIT(5),
	/** UCI vCard property feature - Support for Unique Contact Identifier. */
	BT_PBAP_SUPPORTED_FEATURE_UCI_VCARD_PROPERTY = BIT(6),
	/** UID vCard property feature - Support for vCard UID property. */
	BT_PBAP_SUPPORTED_FEATURE_UID_VCARD_PROPERTY = BIT(7),
	/** Contact referencing feature - Support for contact reference handling. */
	BT_PBAP_SUPPORTED_FEATURE_CONTACT_REFERENCING = BIT(8),
	/** Default contact image feature - Support for default contact images. */
	BT_PBAP_SUPPORTED_FEATURE_DEFAULT_CONTACT_IMAGE = BIT(9),
};

/** @brief PBAP supported repositories bitmask. */
enum bt_pbap_supported_repositories {
	/** Local phone book repository. */
	BT_PBAP_SUPPORTED_REPOSITORIES_LOCAL_PHONE_BOOK = BIT(0),
	/** SIM card repository. */
	BT_PBAP_SUPPORTED_REPOSITORIES_SIM = BIT(1),
	/** Speed dial repository. */
	BT_PBAP_SUPPORTED_REPOSITORIES_SPEED_DIAL = BIT(2),
	/** Favorites repository. */
	BT_PBAP_SUPPORTED_REPOSITORIES_FAVORITES = BIT(3),
};

/** @brief Forward declaration of PBAP PCE structure. */
struct bt_pbap_pce;

/** @brief PBAP PCE (Phone Book Client Equipment) callback operations structure.
 *
 *  This structure must remain valid and constant for the lifetime of the PBAP client.
 */
struct bt_pbap_pce_cb {
	/** @brief RFCOMM transport connected callback.
	 *
	 *  Called when the underlying RFCOMM transport is connected.
	 *
	 *  @param conn ACL connection.
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce);

	/** @brief RFCOMM transport disconnected callback.
	 *
	 *  Called when the underlying RFCOMM transport is disconnected.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 */
	void (*rfcomm_disconnected)(struct bt_pbap_pce *pbap_pce);

	/** @brief L2CAP transport connected callback.
	 *
	 *  Called when the underlying L2CAP transport is connected.
	 *
	 *  @param conn ACL connection.
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce);

	/** @brief L2CAP transport disconnected callback.
	 *
	 *  Called when the underlying L2CAP transport is disconnected.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 */
	void (*l2cap_disconnected)(struct bt_pbap_pce *pbap_pce);

	/** @brief PBAP PCE connect response callback.
	 *
	 *  Called when the PBAP connect response is received.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
	 *  @param version OBEX version supported by PSE.
	 *  @param mopl Maximum OBEX packet length of PSE.
	 *  @param buf Optional response headers buffer.
	 */
	void (*connect)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, uint8_t version,
			uint16_t mopl, struct net_buf *buf);

	/** @brief PBAP PCE disconnect response callback.
	 *
	 *  Called when the PBAP disconnect response is received.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
	 *  @param buf Optional response headers buffer.
	 */
	void (*disconnect)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE pull phone book response callback.
	 *
	 *  Called when the PCE pull phone book response is received.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
	 *  @param buf Optional response headers.
	 */
	void (*pull_phone_book)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				struct net_buf *buf);

	/** @brief PBAP PCE pull vCard listing response callback.
	 *
	 *  Called when the PCE pull vCard listing response is received.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
	 *  @param buf Optional response headers.
	 */
	void (*pull_vcard_listing)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				   struct net_buf *buf);

	/** @brief PBAP PCE pull vCard entry response callback.
	 *
	 *  Called when the PCE pull vCard entry response is received.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
	 *  @param buf Optional response headers.
	 */
	void (*pull_vcard_entry)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				 struct net_buf *buf);

	/** @brief PBAP PCE set phone book response callback.
	 *
	 *  Called when the PCE set phone book response is received.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
	 *  @param buf Optional response headers buffer.
	 */
	void (*set_phone_book)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE abort response callback.
	 *
	 *  Called when the PCE abort response is received.
	 *
	 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
	 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
	 *  @param buf Optional response headers buffer.
	 */
	void (*abort)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);
};

/** @brief PBAP PCE (Phone Book Client Equipment) structure. */
struct bt_pbap_pce {
	/** @brief Callback operations structure. */
	struct bt_pbap_pce_cb *cb;

	/** @internal GOEP (Generic Object Exchange Profile) instance. */
	struct bt_goep _goep;

	/** @internal Transport state (atomic). */
	atomic_t _transport_state;

	/** @internal Connection ID.
	 *
	 *  Unique identifier for the OBEX connection session with the PSE.
	 */
	uint32_t _conn_id;

	/** @internal OBEX client handle.
	 *
	 *  Internal OBEX client structure for managing OBEX protocol operations.
	 */
	struct bt_obex_client _client;

	/** @internal PBAP connection state.
	 *
	 *  Atomic variable tracking the current state of the PBAP connection.
	 */
	atomic_t _state;

	/** @internal Pending response callback. */
	void (*_rsp_cb)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);

	/** @internal Current request function type. */
	const char *_req_type;
};

/** @brief PCE Connect to PSE over RFCOMM transport.
 *
 *  Establishes the underlying RFCOMM transport connection for PBAP PCE.
 *  Once connected, the rfcomm_connected callback will be called.
 *  After transport is connected, call @ref bt_pbap_pce_connect to establish
 *  the PBAP protocol connection.
 *
 *  @param conn ACL connection.
 *  @param pbap_pce PBAP PCE instance to be connected, @ref bt_pbap_pce.
 *  @param cb Callback operations structure for handling PBAP events, @ref bt_pbap_pce_cb.
 *  @param channel RFCOMM server channel number to connect to.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_rfcomm_connect(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce,
			       struct bt_pbap_pce_cb *cb, uint8_t channel);

/** @brief Disconnect PBAP PCE RFCOMM transport.
 *
 *  Closes the underlying RFCOMM transport connection.
 *
 *  @param pbap_pce PBAP PCE instance to disconnect, @ref bt_pbap_pce.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_rfcomm_disconnect(struct bt_pbap_pce *pbap_pce);

/** @brief Connect PBAP PCE client over L2CAP transport.
 *
 *  Establishes the underlying L2CAP transport connection for PBAP PCE.
 *  Once connected, the l2cap_connected callback will be called.
 *  After transport is connected, call @ref bt_pbap_pce_connect to establish
 *  the PBAP protocol connection.
 *
 *  @param conn ACL connection.
 *  @param pbap_pce PBAP PCE instance to be connected, @ref bt_pbap_pce.
 *  @param cb Callback operations structure for handling PBAP events, @ref bt_pbap_pce_cb.
 *  @param psm L2CAP PSM (Protocol Service Multiplexer) to connect to.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_l2cap_connect(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce,
			      struct bt_pbap_pce_cb *cb, uint16_t psm);

/** @brief Disconnect PBAP PCE L2CAP transport.
 *
 *  Closes the underlying L2CAP transport connection.
 *
 *  @param pbap_pce PBAP PCE instance to disconnect, @ref bt_pbap_pce.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_l2cap_disconnect(struct bt_pbap_pce *pbap_pce);

/** @brief Allocate buffer from pool with reserved headroom for PBAP PCE.
 *
 *  Allocates a network buffer from the given pool after reserving headroom for PBAP.
 *  For PBAP connection over RFCOMM, the reserved headroom includes OBEX, RFCOMM, L2CAP and ACL
 *  headers.
 *  For PBAP connection over L2CAP, the reserved headroom includes OBEX, L2CAP and ACL headers.
 *  This ensures proper packet formatting for the underlying transport.
 *  If pool is NULL, allocates from the default pool.
 *
 *  @param pbap_pce PBAP PCE object that will use this buffer, @ref bt_pbap_pce.
 *  @param pool Network buffer pool from which to allocate the buffer, or NULL for default pool.
 *
 *  @return Pointer to newly allocated buffer with reserved headroom, or NULL on failure.
 */
struct net_buf *bt_pbap_pce_create_pdu(struct bt_pbap_pce *pbap_pce, struct net_buf_pool *pool);

/** @brief Establish PBAP protocol connection from PCE client to PSE server.
 *
 *  Initiates a PBAP protocol connection over an already-established transport connection.
 *  The transport (RFCOMM or L2CAP) must be connected first using @ref bt_pbap_pce_rfcomm_connect
 *  or @ref bt_pbap_pce_l2cap_connect.
 *
 *  Once the connection response is received, the connect callback in @ref bt_pbap_pce_cb
 *  will be called. If the connection is rejected, the callback will be called with an
 *  error response code.
 *
 *  @param pbap_pce PBAP PCE object (transport must be connected first), @ref bt_pbap_pce.
 *  @param mopl Maximum OBEX packet length for this connection.
 *  @param buf Buffer containing connection request headers (should include Target UUID,
 *             and optionally authentication challenge, supported features, etc.).
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_connect(struct bt_pbap_pce *pbap_pce, uint16_t mopl, struct net_buf *buf);

/** @brief Disconnect PBAP connection from PBAP client PCE.
 *
 *  Disconnects the PBAP connection by sending an OBEX DISCONNECT request.
 *  The disconnect callback in @ref bt_pbap_pce_cb will be called when the response is received.
 *
 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
 *  @param buf Buffer containing optional disconnect headers.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_disconnect(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Pull phone book from PBAP server PSE.
 *
 *  Sends a pull phone book command to retrieve a complete phone book object from the PSE.
 *
 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
 *  @param buf Buffer to be sent. Should contain:
 *             - Name header with phone book path (e.g., "telecom/pb.vcf")
 *             - Type header with @ref BT_PBAP_PULL_PHONE_BOOK_TYPE
 *             - Optional application parameters (format, etc.)
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_pull_phone_book(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Pull vCard listing from PBAP server PSE.
 *
 *  Sends a request to retrieve a listing of vCard entries from a specific folder.
 *
 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
 *  @param buf Buffer containing:
 *             - Name header with folder path
 *             - Type header with @ref BT_PBAP_PULL_VCARD_LISTING_TYPE
 *             - Optional application parameters (search, etc.)
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_pull_vcard_listing(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Pull specific vCard entry from PBAP server PSE.
 *
 *  Sends a request to retrieve a specific vCard entry.
 *
 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
 *  @param buf Buffer containing:
 *             - Name header with vCard entry name (e.g., "0.vcf")
 *             - Type header with @ref BT_PBAP_PULL_VCARD_ENTRY_TYPE
 *             - Optional application parameters (property selector, format, etc.)
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_pull_vcard_entry(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Set current phone book on PBAP server PSE.
 *
 *  Sets the current phone book on the PSE.
 *
 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
 *  @param flags Navigation flags:
 *               - @ref BT_PBAP_SET_PHONE_BOOK_FLAGS_UP, Navigate to parent
 *               - @ref BT_PBAP_SET_PHONE_BOOK_FLAGS_DOWN_OR_ROOT, Navigate to child or root
 *  @param buf Buffer containing optional Name header with folder name.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_set_phone_book(struct bt_pbap_pce *pbap_pce, uint8_t flags, struct net_buf *buf);

/** @brief Abort current operation on PBAP server PSE.
 *
 *  Sends an abort request to cancel the current ongoing operation.
 *
 *  @param pbap_pce PBAP PCE object, @ref bt_pbap_pce.
 *  @param buf Buffer containing optional abort headers.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pce_abort(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Forward declaration of PBAP PSE structure. */
struct bt_pbap_pse;

/** @brief PBAP PSE (Phone Book Server Equipment) callback operations structure.
 *
 *  Defines callbacks for handling PBAP client requests on the server side.
 */
struct bt_pbap_pse_cb {

	/** @brief RFCOMM transport connected callback.
	 *
	 *  Called when the underlying RFCOMM transport is connected.
	 *
	 *  @param conn ACL connection.
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_pbap_pse *pbap_pse);

	/** @brief RFCOMM transport disconnected callback.
	 *
	 *  Called when the underlying RFCOMM transport is disconnected.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 */
	void (*rfcomm_disconnected)(struct bt_pbap_pse *pbap_pse);

	/** @brief L2CAP transport connected callback.
	 *
	 *  Called when the underlying L2CAP transport is connected.
	 *
	 *  @param conn ACL connection.
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_pbap_pse *pbap_pse);

	/** @brief L2CAP transport disconnected callback.
	 *
	 *  Called when the underlying L2CAP transport is disconnected.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 */
	void (*l2cap_disconnected)(struct bt_pbap_pse *pbap_pse);

	/** @brief PBAP PSE connect request callback.
	 *
	 *  Called when a PBAP connect request is received from a PCE client.
	 *  The PSE should validate the request and prepare a response.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 *  @param version PBAP version requested by PCE.
	 *  @param mopl Maximum OBEX packet length requested by PCE.
	 *  @param buf Request headers buffer (may contain target, authentication, etc.).
	 */
	void (*connect)(struct bt_pbap_pse *pbap_pse, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/** @brief PBAP PSE disconnect request callback.
	 *
	 *  Called when a PBAP disconnect request is received from a PCE client.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 *  @param buf Optional request headers buffer.
	 */
	void (*disconnect)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);

	/** @brief PBAP PSE pull phone book request callback.
	 *
	 *  Called when a pull phone book request is received from a PCE client.
	 *  The PSE should retrieve the requested phone book and send it back.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 *  @param buf Request headers buffer (contains name, type, app parameters, etc.).
	 */
	void (*pull_phone_book)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);

	/** @brief PBAP PSE pull vCard listing request callback.
	 *
	 *  Called when a pull vCard listing request is received from a PCE client.
	 *  The PSE should retrieve the requested vCard listing and send it back.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 *  @param buf Request headers buffer (contains name, type, app parameters, etc.).
	 */
	void (*pull_vcard_listing)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);

	/** @brief PBAP PSE pull vCard entry request callback.
	 *
	 *  Called when a pull vCard entry request is received from a PCE client.
	 *  The PSE should retrieve the requested vCard entry and send it back.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 *  @param buf Request headers buffer (contains name, type, app parameters, etc.).
	 */
	void (*pull_vcard_entry)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);

	/** @brief PBAP PSE set phone book request callback.
	 *
	 *  Called when a set phone book request is received from a PCE client.
	 *  The PSE should set the phone book to the requested one.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 *  @param flags Navigation flags (up/down/root).
	 *              - @ref BT_PBAP_SET_PHONE_BOOK_FLAGS_UP, Navigate to parent
	 *              - @ref BT_PBAP_SET_PHONE_BOOK_FLAGS_DOWN_OR_ROOT, Navigate to child or root
	 *  @param buf Optional request headers buffer (may contain folder name).
	 */
	void (*set_phone_book)(struct bt_pbap_pse *pbap_pse, uint8_t flags, struct net_buf *buf);

	/** @brief PBAP PSE abort request callback.
	 *
	 *  Called when an abort request is received from a PCE client.
	 *  The PSE should cancel the current ongoing operation.
	 *
	 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
	 *  @param buf Optional request headers buffer.
	 */
	void (*abort)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);
};

/** @brief PBAP PSE RFCOMM server structure.
 *
 *  Structure for managing PBAP server over RFCOMM transport.
 */
struct bt_pbap_pse_rfcomm {
	/** Underlying GOEP RFCOMM server. */
	struct bt_goep_transport_rfcomm_server server;

	/** @brief Accept connection callback.
	 *
	 *  Called when a new RFCOMM connection is accepted.
	 *  The application should allocate and initialize a PBAP PSE instance.
	 *
	 *  @param conn ACL connection.
	 *  @param pbap_pse_rfcomm PBAP PSE RFCOMM server instance, @ref bt_pbap_pse_rfcomm.
	 *  @param pbap_pse Pointer to store the created PBAP PSE instance, @ref bt_pbap_pse.
	 *
	 *  @return 0 on success, negative error code on failure.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_pbap_pse_rfcomm *pbap_pse_rfcomm,
		      struct bt_pbap_pse **pbap_pse);
};

/** @brief PBAP PSE L2CAP server structure.
 *
 *  Structure for managing PBAP server over L2CAP transport.
 */
struct bt_pbap_pse_l2cap {
	/** Underlying GOEP L2CAP server. */
	struct bt_goep_transport_l2cap_server server;

	/** @brief Accept connection callback.
	 *
	 *  Called when a new L2CAP connection is accepted.
	 *  The application should allocate and initialize a PBAP PSE instance.
	 *
	 *  @param conn ACL connection.
	 *  @param pbap_pse_l2cap PBAP PSE L2CAP server instance, @ref bt_pbap_pse_l2cap.
	 *  @param pbap_pse Pointer to store the created PBAP PSE instance, @ref bt_pbap_pse.
	 *
	 *  @return 0 on success, negative error code on failure.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_pbap_pse_l2cap *pbap_pse_l2cap,
		      struct bt_pbap_pse **pbap_pse);
};

/** @brief PBAP PSE (Phone Book Server Equipment) structure. */
struct bt_pbap_pse {
	/** @brief Callback operations structure. */
	struct bt_pbap_pse_cb *cb;

	/** @internal GOEP (Generic Object Exchange Profile) instance. */
	struct bt_goep _goep;

	/** @internal Transport state (atomic). */
	atomic_t _transport_state;

	/** @internal Connection ID.
	 *
	 *  Unique identifier for the OBEX connection session with the PCE.
	 */
	uint32_t _conn_id;

	/** @internal OBEX server handle.
	 *
	 *  Internal OBEX server structure for managing OBEX protocol operations.
	 */
	struct bt_obex_server _server;

	/** @internal PBAP connection state.
	 *
	 *  Atomic variable tracking the current state of the PBAP connection.
	 */
	atomic_t _state;

	/** @internal Pending request callback. */
	void (*_req_cb)(struct bt_pbap_pse *pse, struct net_buf *buf);

	/** @internal Current operation type string. */
	const char *_optype;

	/** @internal Flags. */
	atomic_t _flags;
};

/** @brief Allocate buffer from pool with reserved headroom for PBAP PSE.
 *
 *  Allocates a network buffer from the given pool after reserving headroom for PBAP.
 *  For PBAP connection over RFCOMM, the reserved headroom includes OBEX, RFCOMM, L2CAP and ACL
 *  headers.
 *  For PBAP connection over L2CAP, the reserved headroom includes OBEX, L2CAP and ACL headers.
 *  This ensures proper packet formatting for the underlying transport.
 *  If pool is NULL, allocates from the default pool.
 *
 *  @param pbap_pse PBAP PSE object that will use this buffer, @ref bt_pbap_pse.
 *  @param pool Network buffer pool from which to allocate the buffer, or NULL for default pool.
 *
 *  @return Pointer to newly allocated buffer with reserved headroom, or NULL on failure.
 */
struct net_buf *bt_pbap_pse_create_pdu(struct bt_pbap_pse *pbap_pse, struct net_buf_pool *pool);

/** @brief Register PBAP PSE RFCOMM server.
 *
 *  Registers a PBAP server that listens for incoming RFCOMM connections.
 *  The server will be assigned an RFCOMM channel which should be advertised via SDP.
 *
 *  @param server PBAP PSE RFCOMM server structure, @ref bt_pbap_pse_rfcomm.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_rfcomm_register(struct bt_pbap_pse_rfcomm *server);

/** @brief Register PBAP PSE L2CAP server.
 *
 *  Registers a PBAP server that listens for incoming L2CAP connections.
 *  The server will be assigned an L2CAP PSM which should be advertised via SDP.
 *
 *  @param server PBAP PSE L2CAP server structure, @ref bt_pbap_pse_l2cap.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_l2cap_register(struct bt_pbap_pse_l2cap *server);

/** @brief Register PBAP PSE instance.
 *
 *  Registers a PBAP PSE instance with the PBAP subsystem.
 *  This initializes the PSE object and associates it with the provided callback structure
 *  for handling PBAP server operations.
 *
 *  @param pbap_pse PBAP PSE object to register, @ref bt_pbap_pse.
 *  @param cb Callback operations structure for PSE events, @ref bt_pbap_pse_cb.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_register(struct bt_pbap_pse *pbap_pse, struct bt_pbap_pse_cb *cb);

/** @brief Send connect response from PBAP PSE.
 *
 *  Sends a response to a connect request from a PCE client.
 *
 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
 *  @param mopl Maximum OBEX packet length.
 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
 *  @param buf Buffer containing response headers (should include Who header, connection ID, etc.).
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_connect_rsp(struct bt_pbap_pse *pbap_pse, uint16_t mopl, uint8_t rsp_code,
			    struct net_buf *buf);

/** @brief Send disconnect response from PBAP PSE.
 *
 *  Sends a response to a disconnect request from a PCE client.
 *
 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
 *  @param buf Buffer containing optional response headers.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_disconnect_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send pull phone book response from PBAP PSE.
 *
 *  Sends a response to a pull phone book request from a PCE client.
 *  For large phonebooks, multiple responses with @ref BT_PBAP_RSP_CODE_CONTINUE
 *  can be sent, followed by a final response with @ref BT_PBAP_RSP_CODE_SUCCESS.
 *
 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
 *  @param buf Buffer containing response headers.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_pull_phone_book_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				    struct net_buf *buf);

/** @brief Send pull vCard listing response from PBAP PSE.
 *
 *  Sends a response to a pull vCard listing request from a PCE client.
 *  For large listings, multiple responses with @ref BT_PBAP_RSP_CODE_CONTINUE
 *  can be sent, followed by a final response with @ref BT_PBAP_RSP_CODE_SUCCESS.
 *
 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
 *  @param buf Buffer containing response headers.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_pull_vcard_listing_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				       struct net_buf *buf);

/** @brief Send pull vCard entry response from PBAP PSE.
 *
 *  Sends a response to a pull vCard entry request from a PCE client.
 *  For large vCard entries, multiple responses with @ref BT_PBAP_RSP_CODE_CONTINUE
 *  can be sent, followed by a final response with @ref BT_PBAP_RSP_CODE_SUCCESS.
 *
 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
 *  @param buf Buffer containing response headers.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_pull_vcard_entry_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				     struct net_buf *buf);

/** @brief Send set phone book response from PBAP PSE.
 *
 *  Sends a response to a set phone book request from a PCE client.
 *
 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
 *  @param rsp_code Response code, @ref bt_pbap_rsp_code.
 *  @param buf Buffer containing optional response headers.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_set_phone_book_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				   struct net_buf *buf);

/** @brief Send abort response from PBAP PSE.
 *
 *  Sends a response to an abort request from a PCE client.
 *
 *  @param pbap_pse PBAP PSE object, @ref bt_pbap_pse.
 *  @param rsp_code Response code (typically @ref BT_PBAP_RSP_CODE_SUCCESS),
 *                  @ref bt_pbap_rsp_code.
 *  @param buf Buffer containing optional response headers.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_pse_abort_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf);

/** @brief Calculate authentication nonce for PBAP challenge.
 *
 *  Generates a random nonce value used in OBEX authentication challenges.
 *  The nonce is used to prevent replay attacks during authentication.
 *
 *  @param pwd Password string used for authentication (null-terminated).
 *  @param nonce Output buffer to store the generated nonce
 *               (@ref BT_OBEX_CHALLENGE_TAG_NONCE_LEN bytes).
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_calculate_nonce(const uint8_t *pwd, uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN]);

/** @brief Calculate response digest for PBAP authentication.
 *
 *  Computes the MD5 digest for an authentication response based on the password
 *  and received nonce. This digest is sent back to prove knowledge of the password
 *  without transmitting the password itself.
 *
 *  @param pwd Password string used for authentication (null-terminated).
 *  @param nonce Nonce value received in the authentication challenge
 *               (@ref BT_OBEX_CHALLENGE_TAG_NONCE_LEN bytes).
 *  @param rsp_digest Output buffer to store the calculated response digest
 *                    (@ref BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN bytes).
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_pbap_calculate_rsp_digest(const uint8_t *pwd,
				 const uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN],
				 uint8_t rsp_digest[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN]);

/** @brief Verify authentication response.
 *
 *  Verifies that the received response digest matches the expected value based on
 *  the password and nonce. Used by the authenticating party to validate the
 *  authentication response.
 *
 *  @param nonce Nonce value that was sent in the authentication challenge
 *               (@ref BT_OBEX_CHALLENGE_TAG_NONCE_LEN bytes).
 *  @param rsp_digest Response digest received from the authenticating peer
 *                    (@ref BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN bytes).
 *  @param pwd Password string used for authentication (null-terminated).
 *
 *  @return 0 if authentication is successful, negative error code on failure.
 */
int bt_pbap_verify_authentication(uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN],
				  uint8_t rsp_digest[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN],
				  const uint8_t *pwd);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_PBAP_H_ */
