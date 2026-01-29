/* pbap.h - Phone Book Access Profile handling */

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

#include <zephyr/bluetooth/classic/obex.h>
#include <zephyr/bluetooth/classic/goep.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Length of PBAP target UUID */
#define PBAP_TARGET_LEN 16

/** @brief PBAP target UUID for service identification
 *  This UUID is used to identify the PBAP service in OBEX connections
 */
static const uint8_t pbap_target[PBAP_TARGET_LEN] = {0x79U, 0x61U, 0x35U, 0xf0U, 0xf0U, 0xc5U,
						     0x11U, 0xd8U, 0x09U, 0x66U, 0x08U, 0x00U,
						     0x20U, 0x0cU, 0x9aU, 0x66U};

/** @brief MIME type for pull phonebook operation */
#define BT_PBAP_PULL_PHONEBOOK_TYPE "x-bt/phonebook"

/** @brief MIME type for pull vCard listing operation */
#define BT_PBAP_PULL_VCARD_LISTING_TYPE "x-bt/vcard-listing"

/** @brief MIME type for pull vCard entry operation */
#define BT_PBAP_PULL_VCARD_ENTRY_TYPE "x-bt/vcard"

/** @brief SetPath flags: Navigate up to parent directory */
#define BT_PBAP_SET_PATH_FLAGS_UP (BIT(0) | BIT(1))

/** @brief SetPath flags: Navigate down to child directory or to root */
#define BT_PBAP_SET_PATH_FLAGS_DOWN_OR_ROOT BIT(1)

/** @brief PBAP response Code. */
enum __packed bt_pbap_rsp_code {
	/* Continue */
	BT_PBAP_RSP_CODE_CONTINUE = BT_OBEX_RSP_CODE_CONTINUE,
	/* OK */
	BT_PBAP_RSP_CODE_OK = BT_OBEX_RSP_CODE_OK,
	/* Success */
	BT_PBAP_RSP_CODE_SUCCESS = BT_OBEX_RSP_CODE_SUCCESS,
	/* Bad Request - server couldn’t understand request */
	BT_PBAP_RSP_CODE_BAD_REQ = BT_OBEX_RSP_CODE_BAD_REQ,
	/* Unauthorized */
	BT_PBAP_RSP_CODE_UNAUTH = BT_OBEX_RSP_CODE_UNAUTH,
	/* Forbidden - operation is understood but refused */
	BT_PBAP_RSP_CODE_FORBIDDEN = BT_OBEX_RSP_CODE_FORBIDDEN,
	/* Not Found */
	BT_PBAP_RSP_CODE_NOT_FOUND = BT_OBEX_RSP_CODE_NOT_FOUND,
	/* Not Acceptable */
	BT_PBAP_RSP_CODE_NOT_ACCEPT = BT_OBEX_RSP_CODE_NOT_ACCEPT,
	/* Precondition Failed */
	BT_PBAP_RSP_CODE_PRECON_FAIL = BT_OBEX_RSP_CODE_PRECON_FAIL,
	/* Not Implemented */
	BT_PBAP_RSP_CODE_NOT_IMPL = BT_OBEX_RSP_CODE_NOT_IMPL,
	/* Service Unavailable */
	BT_PBAP_RSP_CODE_UNAVAIL = BT_OBEX_RSP_CODE_UNAVAIL,
};

/** @brief The tag id used in PBAP application parameters. */
enum __packed bt_pbap_appl_param_tag_id {
	/* Order */
	BT_PBAP_APPL_PARAM_TAG_ID_ORDER = 0x01,
	/* SearchValue */
	BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_VALUE = 0x02,
	/* SearchProperty */
	BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_PROPERTY = 0x03,
	/* MaxListCount */
	BT_PBAP_APPL_PARAM_TAG_ID_MAX_LIST_COUNT = 0x04,
	/* ListStartOffset */
	BT_PBAP_APPL_PARAM_TAG_ID_LIST_START_OFFSET = 0x05,
	/* PropertySelector */
	BT_PBAP_APPL_PARAM_TAG_ID_PROPERTY_SELECTOR = 0x06,
	/* Format */
	BT_PBAP_APPL_PARAM_TAG_ID_FORMAT = 0x07,
	/* PhonebookSize */
	BT_PBAP_APPL_PARAM_TAG_ID_PHONEBOOK_SIZE = 0x08,
	/* NewMissedCalls */
	BT_PBAP_APPL_PARAM_TAG_ID_NEW_MISSED_CALLS = 0x09,
	/* PrimaryFolderVersion */
	BT_PBAP_APPL_PARAM_TAG_ID_PRIMARY_FOLDER_VERSION = 0x0A,
	/* SecondaryFolderVersion */
	BT_PBAP_APPL_PARAM_TAG_ID_SECONDARY_FOLDER_VERSION = 0x0B,
	/* vCardSelector */
	BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR = 0x0C,
	/* DatabaseIdentifier */
	BT_PBAP_APPL_PARAM_TAG_ID_DATABASE_IDENTIFIER = 0x0D,
	/* vCardSelectorOperator */
	BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR_OPERATOR = 0x0E,
	/* ResetNewMissedCalls */
	BT_PBAP_APPL_PARAM_TAG_ID_RESET_NEW_MISSED_CALLS = 0x0F,
	/* PbapSupportedFeatures */
	BT_PBAP_APPL_PARAM_TAG_ID_SUPPORTED_FEATURES = 0x10,
};

/** @brief The PBAP PropertySelector bitmask. */
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_VERSION              BIT(0)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_FN                   BIT(1)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_N                    BIT(2)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_PHOTO                BIT(3)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_BDAY                 BIT(4)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_ADR                  BIT(5)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_LABEL                BIT(6)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_TEL                  BIT(7)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_EMAIL                BIT(8)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_MAILER               BIT(9)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_TZ                   BIT(10)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_GEO                  BIT(11)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_TITLE                BIT(12)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_ROLE                 BIT(13)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_LOGO                 BIT(14)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_AGENT                BIT(15)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_ORG                  BIT(16)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_NOTE                 BIT(17)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_REV                  BIT(18)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_SOUND                BIT(19)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_URL                  BIT(20)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_UID                  BIT(21)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_KEY                  BIT(22)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_NICKNAME             BIT(23)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_CATEGORIES           BIT(24)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_PRODID               BIT(25)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_CLASS                BIT(26)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_SORT_STRING          BIT(27)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_X_IRMC_CALL_DATETIME BIT(28)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_X_BT_SPEEDDIALKEY    BIT(29)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_X_BT_UCI             BIT(30)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_X_BT_UID             BIT(31)
#define BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_PROPRIETARY_FILTER   BIT(39)

/** @brief The PBAP Supported features bitmask. */
#define BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD                BIT(0)
#define BT_PBAP_SUPPORTED_FEATURE_BROWSING                BIT(1)
#define BT_PBAP_SUPPORTED_FEATURE_DATABASE_IDENTIFIER     BIT(2)
#define BT_PBAP_SUPPORTED_FEATURE_FOLDER_VERSION_COUNTERS BIT(3)
#define BT_PBAP_SUPPORTED_FEATURE_VCARD_SELECTOR          BIT(4)
#define BT_PBAP_SUPPORTED_FEATURE_ENHANCED_MISSED_CALLS   BIT(5)
#define BT_PBAP_SUPPORTED_FEATURE_UCI_VCARD_PROPERTY      BIT(6)
#define BT_PBAP_SUPPORTED_FEATURE_UID_VCARD_PROPERTY      BIT(7)
#define BT_PBAP_SUPPORTED_FEATURE_CONTACT_REFERENCING     BIT(8)
#define BT_PBAP_SUPPORTED_FEATURE_DEFAULT_CONTACT_IMAGE   BIT(9)

/** @brief Default assumed support features for remote PCE
 *  If the PbapSupportedFeatures parameter is not present, 0x00000003 shall be assumed
 *  for a remote PCE (Download and Browsing features).
 */
#define PSE_ASSUMED_SUPPORT_FEATURE                                                                \
	BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD || BT_PBAP_SUPPORTED_FEATURE_BROWSING

/** @brief PBAP connection state enumeration */
enum __packed bt_pbap_state {
	/** PBAP disconnected */
	BT_PBAP_STATE_DISCONNECTED = 0,
	/** PBAP disconnecting */
	BT_PBAP_STATE_DISCONNECTING = 1,
	/** PBAP ready for upper layer traffic on it */
	BT_PBAP_STATE_CONNECTED = 2,
	/** PBAP in connecting state */
	BT_PBAP_STATE_CONNECTING = 3,
};

/** @brief Forward declaration of PBAP structure */
struct bt_pbap;

/** @brief Transport layer state enumeration */
enum __packed bt_pbap_transport_state {
	/** @brief Transport is disconnected */
	BT_PBAP_TRANSPORT_STATE_DISCONNECTED = 0,
	/** @brief Transport is connecting */
	BT_PBAP_TRANSPORT_STATE_CONNECTING = 1,
	/** @brief Transport is connected */
	BT_PBAP_TRANSPORT_STATE_CONNECTED = 2,
	/** @brief Transport is disconnecting */
	BT_PBAP_TRANSPORT_STATE_DISCONNECTING = 3,
};

/** @brief Transport operations structure
 *  Defines callbacks for transport layer events
 */
struct bt_pbap_transport_ops {
	/**
	 * @brief Transport connected callback
	 *
	 * Called when the underlying transport (RFCOMM/L2CAP) is connected
	 *
	 * @param conn Bluetooth connection
	 * @param pbap PBAP instance
	 */
	void (*connected)(struct bt_conn *conn, struct bt_pbap *pbap);

	/**
	 * @brief Transport disconnected callback
	 *
	 * Called when the underlying transport is disconnected
	 *
	 * @param pbap PBAP instance
	 */
	void (*disconnected)(struct bt_pbap *pbap);
};

/** @brief Connect PBAP over RFCOMM transport
 *
 *  @param conn Bluetooth connection
 *  @param pbap PBAP instance
 *  @param channel RFCOMM channel number
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pce_rfcomm_connect(struct bt_conn *conn, struct bt_pbap *pbap, uint8_t channel);
/** @brief Disconnect PBAP RFCOMM transport
 *
 *  @param pbap PBAP instance
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pce_rfcomm_disconnect(struct bt_pbap *pbap);

/** @brief Connect PBAP over L2CAP transport
 *
 *  @param conn Bluetooth connection
 *  @param pbap PBAP instance
 *  @param psm L2CAP PSM (Protocol Service Multiplexer)
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pce_l2cap_connect(struct bt_conn *conn, struct bt_pbap *pbap, uint16_t psm);

/** @brief Disconnect PBAP L2CAP transport
 *
 *  @param pbap PBAP instance
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pce_l2cap_disconnect(struct bt_pbap *pbap);

/** @brief Allocate buffer from pool with reserved headroom for PBAP.
 *
 *  Allocates a network buffer from the given pool after reserving headroom for PBAP.
 *  For PBAP connection over RFCOMM, the reserved headroom includes OBEX, RFCOMM, L2CAP and ACL
 *  headers.
 *  For PBAP connection over L2CAP, the reserved headroom includes OBEX, L2CAP and ACL headers.
 *  This ensures proper packet formatting for the underlying transport.
 *  If pool is NULL, allocates from the default pool.
 *
 *  @param pbap PBAP object that will use this buffer.
 *  @param pool Network buffer pool from which to allocate the buffer.
 *
 *  @return Pointer to newly allocated buffer with reserved headroom, or NULL on failure.
 */
struct net_buf *bt_pbap_create_pdu(struct bt_pbap *pbap, struct net_buf_pool *pool);

/** @brief PBAP base structure
 *  Contains common elements for both PCE and PSE
 */
struct bt_pbap {
	/** GOEP (Generic Object Exchange Profile) instance */
	struct bt_goep goep;

	/** Transport operations callbacks */
	const struct bt_pbap_transport_ops *ops;

	/** @internal Transport state (atomic) */
	atomic_t _transport_state;
};

/** @brief Forward declaration of PBAP PCE structure */
struct bt_pbap_pce;

/** @brief PBAP client PCE operations structure.
 *
 * The object has to stay valid and constant for the lifetime of the PBAP client.
 */
struct bt_pbap_pce_cb {
	/** @brief PBAP PCE connect response callback
	 *
	 * if this callback is provided it will be called when the PBAP connect response
	 * is received.
	 *
	 * @param pbap_pce  The PBAP PCE object
	 * @param rsp_code  Response code @ref bt_pbap_rsp_code
	 * @param version   PBAP version supported by PSE
	 * @param mopl      The max package length of buf that application can use
	 * @param buf       Optional response headers buffer
	 */
	void (*connect)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, uint8_t version,
			uint16_t mopl, struct net_buf *buf);

	/** @brief PBAP PCE disconnect response callback
	 *
	 * if this callback is provided it will be called when the PBAP disconnect response
	 * is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code  Response code @ref bt_pbap_rsp_code
	 * @param buf       Optional response headers buffer
	 */
	void (*disconnect)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE pull phonebook response callback
	 *
	 * if this callback is provided it will be called when the PCE pull phonebook
	 * response is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers and phonebook data
	 */
	void (*pull_phonebook)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE pull vcard listing response callback
	 *
	 * if this callback is provided it will be called when the PCE pull vcard listing
	 * response is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers and vCard listing data
	 */
	void (*pull_vcardlisting)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				  struct net_buf *buf);

	/** @brief PBAP PCE pull vcard entry response callback
	 *
	 * if this callback is provided it will be called when the PCE pull vcard entry
	 * response is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers and vCard entry data
	 */
	void (*pull_vcardentry)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				struct net_buf *buf);

	/** @brief PBAP PCE set path response callback
	 *
	 * if this callback is provided it will be called when the PCE set path
	 * response is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers buffer
	 */
	void (*set_path)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE abort response callback
	 *
	 * if this callback is provided it will be called when the PCE abort
	 * response is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers buffer
	 */
	void (*abort)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);
};

/** @brief PBAP PCE (Phone Book Access Profile Client Equipment) structure */
struct bt_pbap_pce {
	/** Callback operations structure */
	struct bt_pbap_pce_cb *cb;

	/** @internal Pointer to base PBAP structure */
	struct bt_pbap *_pbap;

	/** @brief Connection ID
	 *  Unique identifier for the OBEX connection session with the PSE
	 */
	uint32_t _conn_id;

	/** @brief OBEX client handle
	 *  Internal OBEX client structure for managing OBEX protocol operations
	 */
	struct bt_obex_client _client;

	/** @brief PBAP connection state
	 *  Atomic variable tracking the current state of the PBAP connection
	 *  (see enum bt_pbap_state for possible values)
	 */
	atomic_t _state;

	/** @internal Pending response callback */
	void (*_rsp_cb)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);

	/** @internal current request function type */
	const char *_req_type;
};

/** @brief Connect PBAP client PCE to PBAP server PSE.
 *
 *  Initiates a PBAP connection. Once the connection is completed,
 *  the callback @ref bt_pbap_pce_cb::connect is called. If the connection is rejected,
 *  @ref bt_pbap_pce_cb::disconnect callback is called instead.
 *
 *  The PBAP PCE object is passed (over an address of it) as parameter. Application should
 *  create and initialize the PBAP PCE object @ref bt_pbap_pce before calling this function.
 *
 *  @param pbap Base PBAP structure (transport must be connected first)
 *  @param pbap_pce Pointer to PBAP PCE object @ref bt_pbap_pce.
 *  @param mopl     Max obex packet length
 *  @param cb Callback operations structure
 *  @param buf Buffer containing connection request headers (can include target, authentication,
 * etc.)
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_connect(struct bt_pbap *pbap, struct bt_pbap_pce *pbap_pce, uint16_t mopl,
			struct bt_pbap_pce_cb *cb, struct net_buf *buf);

/** @brief Disconnect PBAP connection from PBAP client PCE.
 *
 *  Disconnects the PBAP connection by sending an OBEX DISCONNECT request.
 *  The disconnect callback will be called when the response is received.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer containing optional disconnect headers
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_disconnect(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Create command for PBAP client PCE to pull phonebook from PBAP server PSE.
 *
 *  This API creates and sends a pull phonebook command to retrieve a complete phonebook
 *  object from the PSE.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer to be sent. Can be allocated by @ref bt_pbap_create_pdu
 *             in application before this function is called. Should contain:
 *             - Name header with phonebook path (e.g., "telecom/pb.vcf")
 *             - Type header with BT_PBAP_PULL_PHONEBOOK_TYPE
 *             - Optional application parameters (filters, format, etc.)
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_phonebook(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Pull vCard listing from PBAP server PSE.
 *
 *  Sends a request to retrieve a listing of vCard entries from a specific folder.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer containing:
 *             - Name header with folder path
 *             - Type header with BT_PBAP_PULL_VCARD_LISTING_TYPE
 *             - Optional application parameters (search, filters, etc.)
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_vcardlisting(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Pull specific vCard entry from PBAP server PSE.
 *
 *  Sends a request to retrieve a specific vCard entry.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer containing:
 *             - Name header with vCard entry name (e.g., "0.vcf")
 *             - Type header with BT_PBAP_PULL_VCARD_ENTRY_TYPE
 *             - Optional application parameters (property selector, format, etc.)
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_vcardentry(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Set current folder path on PBAP server PSE.
 *
 *  Navigates the virtual folder structure on the PSE.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param flags Navigation flags:
 *               - BT_PBAP_SET_PATH_FLAGS_UP: navigate to parent
 *               - BT_PBAP_SET_PATH_FLAGS_DOWN_OR_ROOT: navigate to child or root
 *  @param buf Buffer containing optional Name header with folder name
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_setpath(struct bt_pbap_pce *pbap_pce, uint8_t flags, struct net_buf *buf);

/** @brief Abort current operation on PBAP server PSE.
 *
 *  Sends an abort request to cancel the current ongoing operation.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer containing optional abort headers
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_abort(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/**
 * @brief Helper for adding Single Response Mode (SRM) header to PBAP request.
 *
 * A helper macro for adding the SRM (Single Response Mode) header to a PBAP request.
 * SRM enables continuous data transfer without waiting for acknowledgment after each packet.
 * When set to 0x01 (Enable), it indicates that the sender wants to enable SRM mode.
 * This improves throughput by reducing round-trip delays.
 *
 * This is primarily used for L2CAP-based PBAP connections.
 *
 * @param buf    Network buffer to which the SRM header will be added.
 *               Should be allocated via bt_pbap_create_pdu.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_add_header_srm(buf) bt_obex_add_header_srm(buf, 0x01)

/**
 * @brief Helper for adding Single Response Mode Parameter (SRMP) header to PBAP request.
 *
 * A helper macro for adding the SRMP (Single Response Mode Parameter) header to a PBAP request.
 * SRMP is used in conjunction with SRM to control the flow of data transfer. When set to 0x01
 * (Wait), it indicates that the sender wants the receiver to wait before sending the next packet.
 * This allows the sender to process received data before continuing the transfer.
 *
 * This is primarily used for L2CAP-based PBAP connections.
 *
 * @param buf    Network buffer to which the SRMP header will be added.
 *               Should be allocated via bt_pbap_create_pdu.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_add_header_srm_param(buf) bt_obex_add_header_srm_param(buf, 0x01)

/**
 * @brief Helper for adding Target header to PBAP request.
 *
 * Adds the PBAP target UUID to the request buffer. This header identifies
 * the PBAP service and is typically used in CONNECT requests.
 *
 * @param buf    Network buffer to which the Target header will be added.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_add_header_target(buf) bt_obex_add_header_target(buf, PBAP_TARGET_LEN, pbap_target)

/**
 * @brief Helper for adding Who header to PBAP response.
 *
 * Adds the PBAP Who UUID to the response buffer. This header identifies
 * the PBAP service and is typically used in CONNECT responses from PSE.
 *
 * @param buf    Network buffer to which the Who header will be added.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_add_header_who(buf) bt_obex_add_header_who(buf, PBAP_TARGET_LEN, pbap_target)

/** @brief Forward declaration of PBAP PSE structure */
struct bt_pbap_pse;

/** @brief PBAP server PSE operations structure.
 *
 * Defines callbacks for handling PBAP client requests on the server side.
 */
struct bt_pbap_pse_cb {
	/** @brief PBAP PSE connect request callback
	 *
	 * Called when a PBAP connect request is received from a PCE client.
	 * The PSE should validate the request and prepare a response.
	 *
	 * @param pbap_pse  The PBAP PSE object
	 * @param version   PBAP version requested by PCE
	 * @param mopl      Maximum OBEX packet length requested by PCE
	 * @param buf       Request headers buffer (may contain target, authentication, etc.)
	 */
	void (*connect)(struct bt_pbap_pse *pbap_pse, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/** @brief PBAP PSE disconnect request callback
	 *
	 * Called when a PBAP disconnect request is received from a PCE client.
	 *
	 * @param pbap_pse The PBAP PSE object
	 * @param buf      Optional request headers buffer
	 */
	void (*disconnect)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);

	/** @brief PBAP PSE pull phonebook request callback
	 *
	 * Called when a pull phonebook request is received from a PCE client.
	 * The PSE should retrieve the requested phonebook and send it back.
	 *
	 * @param pbap_pse The PBAP PSE object
	 * @param buf      Request headers buffer (contains name, type, app parameters, etc.)
	 */
	void (*pull_phonebook)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);

	/** @brief PBAP PSE pull vCard listing request callback
	 *
	 * Called when a pull vCard listing request is received from a PCE client.
	 * The PSE should retrieve the requested vCard listing and send it back.
	 *
	 * @param pbap_pse The PBAP PSE object
	 * @param buf      Request headers buffer (contains name, type, app parameters, etc.)
	 */
	void (*pull_vcardlisting)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);

	/** @brief PBAP PSE pull vCard entry request callback
	 *
	 * Called when a pull vCard entry request is received from a PCE client.
	 * The PSE should retrieve the requested vCard entry and send it back.
	 *
	 * @param pbap_pse The PBAP PSE object
	 * @param buf      Request headers buffer (contains name, type, app parameters, etc.)
	 */
	void (*pull_vcardentry)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);

	/** @brief PBAP PSE set path request callback
	 *
	 * Called when a set path request is received from a PCE client.
	 * The PSE should navigate to the requested folder.
	 *
	 * @param pbap_pse The PBAP PSE object
	 * @param flags    Navigation flags (up/down/root)
	 * @param buf      Optional request headers buffer (may contain folder name)
	 */
	void (*setpath)(struct bt_pbap_pse *pbap_pse, uint8_t flags, struct net_buf *buf);

	/** @brief PBAP PSE abort request callback
	 *
	 * Called when an abort request is received from a PCE client.
	 * The PSE should cancel the current ongoing operation.
	 *
	 * @param pbap_pse The PBAP PSE object
	 * @param buf      Optional request headers buffer
	 */
	void (*abort)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);
};

/** @brief PBAP PSE RFCOMM server structure
 *
 * Structure for managing PBAP server over RFCOMM transport.
 */
struct bt_pbap_pse_rfcomm {
	/** @brief Underlying GOEP RFCOMM server */
	struct bt_goep_transport_rfcomm_server server;

	/**
	 * @brief Accept connection callback
	 *
	 * Called when a new RFCOMM connection is accepted.
	 * The application should allocate and initialize a PBAP instance.
	 *
	 * @param conn Bluetooth connection
	 * @param pbap_pse RFCOMM server instance
	 * @param pbap Pointer to store the created PBAP instance
	 * @return 0 on success, negative error code on failure
	 */
	int (*accept)(struct bt_conn *conn, struct bt_pbap_pse_rfcomm *pbap_pse,
		      struct bt_pbap **pbap);
};

/** @brief PBAP PSE L2CAP server structure
 *
 * Structure for managing PBAP server over L2CAP transport.
 */
struct bt_pbap_pse_l2cap {
	/** @brief Underlying GOEP L2CAP server */
	struct bt_goep_transport_l2cap_server server;

	/**
	 * @brief Accept connection callback
	 *
	 * Called when a new L2CAP connection is accepted.
	 * The application should allocate and initialize a PBAP instance.
	 *
	 * @param conn Bluetooth connection
	 * @param pbap_pse L2CAP server instance
	 * @param pbap Pointer to store the created PBAP instance
	 * @return 0 on success, negative error code on failure
	 */
	int (*accept)(struct bt_conn *conn, struct bt_pbap_pse_l2cap *pbap_pse,
		      struct bt_pbap **pbap);
};

/** @brief PBAP PSE (Phone Book Access Profile Server Equipment) structure */
struct bt_pbap_pse {
	/** Callback operations structure */
	struct bt_pbap_pse_cb *cb;

	/** @internal Pointer to base PBAP structure */
	struct bt_pbap *_pbap;

	/** @brief Connection ID
	 *  Unique identifier for the OBEX connection session with the PCE
	 */
	uint32_t _conn_id;

	/** @brief OBEX server handle
	 *  Internal OBEX server structure for managing OBEX protocol operations
	 */
	struct bt_obex_server _server;

	/** @brief Transport server union
	 *  Contains either RFCOMM or L2CAP server structure depending on transport type
	 */
	union {
		/** RFCOMM transport server */
		struct bt_goep_transport_rfcomm_server rfcomm_server;

		/** L2CAP transport server */
		struct bt_goep_transport_l2cap_server l2cap_server;
	} transport_server;

	/** @brief PBAP connection state
	 *  Atomic variable tracking the current state of the PBAP connection
	 *  (see enum bt_pbap_state for possible values)
	 */
	atomic_t _state;

	/** @internal Pending request callback */
	void (*_req_cb)(struct bt_pbap_pse *pse, struct net_buf *buf);

	/** @internal Current operation type string */
	const char *_optype;
};
/** @brief Register PBAP PSE RFCOMM server.
 *
 *  Registers a PBAP server that listens for incoming RFCOMM connections.
 *  The server will be assigned an RFCOMM channel which should be advertised via SDP.
 *
 *  @param server PBAP PSE RFCOMM server structure
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_rfcomm_register(struct bt_pbap_pse_rfcomm *server);

/** @brief Register PBAP PSE L2CAP server.
 *
 *  Registers a PBAP server that listens for incoming L2CAP connections.
 *  The server will be assigned an L2CAP PSM which should be advertised via SDP.
 *
 *  @param server PBAP PSE L2CAP server structure
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_l2cap_register(struct bt_pbap_pse_l2cap *server);

/** @brief Register PBAP PSE instance.
 *
 *  Registers a PBAP PSE instance after transport connection is established.
 *  This associates the PSE with a specific PBAP connection and callback structure.
 *
 *  @param pbap Base PBAP structure (transport must be connected)
 *  @param pbap_pse PBAP PSE object
 *  @param cb Callback operations structure
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_register(struct bt_pbap *pbap, struct bt_pbap_pse *pbap_pse,
			 struct bt_pbap_pse_cb *cb);

/** @brief Send connect response from PBAP PSE.
 *
 *  Sends a response to a connect request from a PCE client.
 *
 *  @param pbap_pse PBAP PSE object
 *  @param mopl     Max obex packet length
 *  @param rsp_code Response code @ref bt_pbap_rsp_code
 *  @param buf Buffer containing response headers (should include Who header, connection ID, etc.)
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_connect_rsp(struct bt_pbap_pse *pbap_pse, uint16_t mopl, uint8_t rsp_code,
			    struct net_buf *buf);

/** @brief Send disconnect response from PBAP PSE.
 *
 *  Sends a response to a disconnect request from a PCE client.
 *
 *  @param pbap_pse PBAP PSE object
 *  @param rsp_code Response code @ref bt_pbap_rsp_code
 *  @param buf Buffer containing optional response headers
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_disconnect_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send pull phonebook response from PBAP PSE.
 *
 *  Sends a response to a pull phonebook request from a PCE client.
 *  For large phonebooks, multiple responses with BT_PBAP_RSP_CODE_CONTINUE
 *  can be sent, followed by a final response with BT_PBAP_RSP_CODE_SUCCESS.
 *
 *  @param pbap_pse PBAP PSE object
 *  @param rsp_code Response code @ref bt_pbap_rsp_code
 *  @param buf Buffer containing response headers and phonebook data (body/end-of-body)
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_pull_phonebook_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				   struct net_buf *buf);

/** @brief Send pull vCard listing response from PBAP PSE.
 *
 *  Sends a response to a pull vCard listing request from a PCE client.
 *  For large listings, multiple responses with BT_PBAP_RSP_CODE_CONTINUE
 *  can be sent, followed by a final response with BT_PBAP_RSP_CODE_SUCCESS.
 *
 *  @param pbap_pse PBAP PSE object
 *  @param rsp_code Response code @ref bt_pbap_rsp_code
 *  @param buf Buffer containing response headers and vCard listing data
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_pull_vcardlisting_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				      struct net_buf *buf);

/** @brief Send pull vCard entry response from PBAP PSE.
 *
 *  Sends a response to a pull vCard entry request from a PCE client.
 *  For large vCard entries, multiple responses with BT_PBAP_RSP_CODE_CONTINUE
 *  can be sent, followed by a final response with BT_PBAP_RSP_CODE_SUCCESS.
 *
 *  @param pbap_pse PBAP PSE object
 *  @param rsp_code Response code @ref bt_pbap_rsp_code
 *  @param buf Buffer containing response headers and vCard entry data
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_pull_vcardentry_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				    struct net_buf *buf);

/** @brief Send set path response from PBAP PSE.
 *
 *  Sends a response to a set path request from a PCE client.
 *
 *  @param pbap_pse PBAP PSE object
 *  @param rsp_code Response code @ref bt_pbap_rsp_code
 *  @param buf Buffer containing optional response headers
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_setpath_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send abort response from PBAP PSE.
 *
 *  Sends a response to an abort request from a PCE client.
 *
 *  @param pbap_pse PBAP PSE object
 *  @param rsp_code Response code @ref bt_pbap_rsp_code (typically BT_PBAP_RSP_CODE_SUCCESS)
 *  @param buf Buffer containing optional response headers
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_pse_abort_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf);

/** @brief Generate authentication challenge for PBAP connection.
 *
 *  Creates an authentication challenge header to be sent in a PBAP request or response.
 *  This is used to authenticate the remote device using a password/PIN.
 *
 *  @param pwd Password/PIN string for authentication
 *  @param auth_chal_req Buffer to store the generated authentication challenge
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_generate_auth_challenge(const uint8_t *pwd, uint8_t *auth_chal_req);

/** @brief Generate authentication response for PBAP connection.
 *
 *  Creates an authentication response header in reply to an authentication challenge.
 *  This proves knowledge of the password/PIN to the remote device.
 *
 *  @param pwd Password/PIN string for authentication
 *  @param auth_chal_req Received authentication challenge
 *  @param auth_chal_rsp Buffer to store the generated authentication response
 *
 *  @return 0 on success, negative error code on failure
 */
int bt_pbap_generate_auth_response(const uint8_t *pwd, uint8_t *auth_chal_req,
				   uint8_t *auth_chal_rsp);

/** @brief Verify authentication response for PBAP connection.
 *
 *  Verifies that the received authentication response matches the expected value
 *  based on the challenge sent and the password/PIN.
 *
 *  @param auth_chal_req Authentication challenge that was sent
 *  @param auth_chal_rsp Authentication response that was received
 *  @param pwd Password/PIN string for authentication
 *
 *  @return 0 if authentication is successful, negative error code on failure
 */
int bt_pbap_verify_auth(uint8_t *auth_chal_req, uint8_t *auth_chal_rsp, const uint8_t *pwd);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_PBAP_H_ */
