/* pbap.h - Phone Book Access Profile handling */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <zephyr/bluetooth/classic/obex.h>
#include <zephyr/bluetooth/classic/goep.h>

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_PBAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_PBAP_H_

/**
 * @brief Bluetooth APIs
 * @defgroup bluetooth Bluetooth APIs
 * @ingroup connectivity
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Phone Book Access Profile(PBAP)
 * @defgroup bt_pbap Phone Book Access Profile (PBAP)
 * @since 1.0
 * @version 1.0.0
 * @ingroup bluetooth
 * @{
 */

/** @brief PBAP response Code. */
enum __packed bt_pbap_rsp_code {
	/* Continue */
	BT_PBAP_RSP_CODE_CONTINUE = 0x90,
	/* OK */
	BT_PBAP_RSP_CODE_OK = 0xa0,
	/* Success */
	BT_PBAP_RSP_CODE_SUCCESS = 0xa0,
	/* Bad Request - server couldn’t understand request */
	BT_PBAP_RSP_CODE_BAD_REQ = 0xc0,
	/* Unauthorized */
	BT_PBAP_RSP_CODE_UNAUTH = 0xc1,
	/* Forbidden - operation is understood but refused */
	BT_PBAP_RSP_CODE_FORBIDDEN = 0xc3,
	/* Not Found */
	BT_PBAP_RSP_CODE_NOT_FOUND = 0xc4,
	/* Not Acceptable */
	BT_PBAP_RSP_CODE_NOT_ACCEPT = 0xc6,
	/* Precondition Failed */
	BT_PBAP_RSP_CODE_PRECON_FAIL = 0xcc,
	/* Not Implemented */
	BT_PBAP_RSP_CODE_NOT_IMPL = 0xd1,
	/* Service Unavailable */
	BT_PBAP_RSP_CODE_UNAVAIL = 0xd3,
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

/*If the PbapSupportedFeatures parameter is not present 0x00000003 shall be assumed
for a remote PCE.*/
#define PSE_ASSUMED_SUPPORT_FEATURE                                                                \
	BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD || BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD

enum __packed bt_pbap_state {
	/** PBAP disconnected */
	BT_PBAP_DISCONNECTED,
	/** PBAP disconnecting */
	BT_PBAP_DISCONNECTING,
	/** PBAP in connecting state */
	BT_PBAP_CONNECTING,
	/** PBAP ready for upper layer traffic on it */
	BT_PBAP_CONNECTED,
	/** PBAP in PULL Phonebook Function state  */
	BT_PBAP_PULL_PHONEBOOK,
	/** PBAP in Set Path Function state */
	B_PBAP_SET_PATH,
	/** PBAP in PULL Vcardlisting Function state  */
	BT_PBAP_PULL_VCARDLISTING,
	/** PBAP in PULL Vcardentry Function state  */
	BT_PBAP_PULL_VCARDENTRY,
	/** PBAP in idel state */
	BT_PBAP_IDEL,
	/** PBAP in abort state */
	BT_PBAP_ABORT,
};

/** @brief PBAP PCE (Phone Book Access Profile Client Equipment) structure */
struct bt_pbap_pce {
	/** @brief Connection handle
	 *  Pointer to the Bluetooth connection object associated with this PBAP session
	 */
	struct bt_conn *_conn;

	/** @brief Password for authentication
	 *  When connecting, the application must provide this parameter when the component
	 *  wants to authenticate with the server. Set to NULL if no authentication is required.
	 */
	const uint8_t *_pwd;

	/** @brief Local authentication flag
	 *  Indicates whether local authentication is enabled for this PBAP session
	 */
	bool local_auth;

	/** @brief Authentication challenge nonce
	 *  Stores the nonce value used in OBEX authentication challenge/response mechanism
	 */
	uint8_t _auth_challenge_nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN];

	/** @brief Connection ID
	 *  Unique identifier for the OBEX connection session with the PSE
	 */
	uint32_t _conn_id;

	/** @brief Single Response Mode Parameter (SRMP) flag
	 *  Indicates whether SRMP is enabled. When true, the server should wait for
	 *  the client's next request after sending a reply (applicable for L2CAP connections)
	 */
	bool _srmp;

	/** @brief Peer device supported features
	 *  After performing SDP to the server, the PCE can obtain the features supported
	 *  by the PSE. If the PbapSupportedFeatures of PSE is not present, 0x00000003
	 *  (PSE_ASSUMED_SUPPORT_FEATURE) shall be assumed for a PCE.
	 */
	uint32_t _peer_feature;

	/** @brief Maximum OBEX packet length
	 *  When performing a connect operation, the application must provide this parameter.
	 *  Defines the maximum size of OBEX packets that can be exchanged
	 */
	uint16_t _mopl;

	/** @brief GOEP (Generic Object Exchange Profile) handle
	 *  Internal GOEP structure for managing object exchange operations
	 */
	struct bt_goep _goep;

	/** @brief OBEX client handle
	 *  Internal OBEX client structure for managing OBEX protocol operations
	 */
	struct bt_obex_client _client;

	/** @brief PBAP connection state
	 *  Atomic variable tracking the current state of the PBAP connection
	 *  (see enum bt_pbap_state for possible values)
	 */
	atomic_t _state;
};

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
	 * @param mpl   The max package length of buf that application can use
	 */
	void (*connect)(struct bt_pbap_pce *pbap_pce, uint16_t mpl);

	/** PBAP_PCE get authentication information callback to application
	 *
	 * If this callback is provided it will be called whenever pse asks to authenticate pce,
	 * and pce does not provide authentication information when initiating the connection.
	 * The application can provide validation authenticate information in this callback.
	 * Authication information include password and user_id(optional).
	 *
	 *  @param pbap_pce    PBAP PCE object.
	 */
	void (*get_auth_info)(struct bt_pbap_pce *pbap_pce);

	/** @brief PBAP PCE disconnect response callback
	 *
	 * if this callback is provided it will be called when the PBAP disconnect response
	 * is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code  Response code @ref bt_pbap_rsp_code
	 */
	void (*disconnect)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code);

	/** @brief PBAP PCE pull phonebook response callback
	 *
	 * if this callback is provided it will be called when the PCE pull phonebook
	 * response is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers
	 */
	void (*pull_phonebook)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE pull vcardlisting response callback
	 *
	 * if this callback is provided it will be called when the PCE pull vcardlisting
	 * response is received.
	 *
	 * @param pbap_pce The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers
	 */
	void (*pull_vcardlisting)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				  struct net_buf *buf);

	/** @brief PBAP PCE pull vcardentey response callback
	 *
	 * if this callback is provided it will be called when the PCE pull vcardentey
	 * response is received.
	 *
	 * @param pbap_pce The PBAP object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers
	 */
	void (*pull_vcardentry)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				struct net_buf *buf);

	/** @brief PBAP PCE set path response callback
	 *
	 * if this callback is provided it will be called when the PCE set path
	 * response is received.
	 *
	 * @param pbap_pce The PBAP object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 */
	void (*set_path)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code);

	/** @brief PBAP PCE abort response callback
	 *
	 * if this callback is provided it will be called when the PCE abort
	 * response is received.
	 *
	 * @param pbap_pce The PBAP object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 */
	void (*abort)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code);
};

/** @brief Register PBAP client PCE callbacks.
 *
 *  Register PCE application callback structure @ref bt_pbap_pce_cb.
 *  All other PBAP operations need to be performed after this function.
 *  This function must be called before any PBAP PCE connection can be established.
 *
 *  @param cb PBAP client callback structure @ref bt_pbap_pce_cb containing
 *            callback functions for various PBAP events.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_register(struct bt_pbap_pce_cb *cb);

/** @brief Allocate buffer from pool with reserved headroom for PBAP client PCE.
 *
 *  Allocates a network buffer from the given pool after reserving headroom for PBAP client PCE.
 *  For PBAP connection over RFCOMM, the reserved headroom includes OBEX, RFCOMM, L2CAP and ACL
 *  headers.
 *  For PBAP connection over L2CAP, the reserved headroom includes OBEX, L2CAP and ACL headers.
 *  This ensures proper packet formatting for the underlying transport.
 *
 *  @param pbap_pce PBAP PCE object that will use this buffer.
 *  @param pool Network buffer pool from which to allocate the buffer.
 *
 *  @return Pointer to newly allocated buffer with reserved headroom, or NULL on failure.
 */
struct net_buf *bt_pbap_pce_create_pdu(struct bt_pbap_pce *pbap_pce, struct net_buf_pool *pool);

/** @brief Connect PBAP client PCE to PBAP server PSE over RFCOMM.
 *
 *  Initiates a PBAP connection over RFCOMM transport. Once the connection is completed,
 *  the callback @ref bt_pbap_pce_cb::connect is called. If the connection is rejected,
 *  @ref bt_pbap_pce_cb::disconnect callback is called instead.
 *
 *  The ACL connection handle @ref bt_conn is passed as first parameter.
 *  The RFCOMM channel is passed as second parameter. The RFCOMM channel
 *  of the PBAP server PSE can be obtained through SDP operation.
 *
 *  The PBAP PCE object is passed (over an address of it) as third parameter. Application should
 *  create and initialize the PBAP PCE object @ref bt_pbap_pce before calling this function.
 *
 *  @param conn ACL connection handle to the remote device.
 *  @param channel RFCOMM server channel number of the PSE (obtained via SDP).
 *  @param pbap_pce Pointer to PBAP PCE object @ref bt_pbap_pce.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_rfcomm_connect(struct bt_conn *conn, uint8_t channel, struct bt_pbap_pce *pbap_pce);

/** @brief Connect PBAP client PCE to PBAP server PSE over L2CAP.
 *
 *  Initiates a PBAP connection over L2CAP transport. Once the connection is completed,
 *  the callback @ref bt_pbap_pce_cb::connect is called. If the connection is rejected,
 *  @ref bt_pbap_pce_cb::disconnect callback is called instead.
 *
 *  The ACL connection handle @ref bt_conn is passed as first parameter.
 *  The L2CAP PSM is passed as second parameter. The L2CAP PSM
 *  of the PBAP server PSE can be obtained through SDP operation.
 *
 *  The PBAP PCE object is passed (over an address of it) as third parameter. Application should
 *  create and initialize the PBAP PCE object @ref bt_pbap_pce before calling this function.
 *
 *  @param conn ACL connection handle to the remote device.
 *  @param psm L2CAP Protocol Service Multiplexer of the PSE (obtained via SDP).
 *  @param pbap_pce Pointer to PBAP PCE object @ref bt_pbap_pce.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_l2cap_connect(struct bt_conn *conn, uint16_t psm, struct bt_pbap_pce *pbap_pce);

/** @brief Disconnect PBAP connection from PBAP client PCE.
 *
 *  Disconnects the PBAP connection. The behavior depends on the enforce parameter:
 *  - If enforce is true: Terminates the connection by closing the transport connection
 *    without issuing the OBEX DISCONNECT operation (forced disconnect).
 *  - If enforce is false: Sends an OBEX DISCONNECT first. If OBEX disconnect succeeds,
 *    automatically closes the transport connection. If OBEX disconnect fails,
 *    the disconnect callback registered via @ref bt_pbap_pce_register will be called,
 *    but the transport connection will NOT be closed. Application should recall
 *    @ref bt_pbap_pce_disconnect with enforce=true to force disconnect.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param enforce If true, closes the transport connection without issuing OBEX DISCONNECT;
 *                 if false, attempts graceful OBEX disconnect first.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_disconnect(struct bt_pbap_pce *pbap_pce, bool enforce);

/** @brief Create command for PBAP client PCE to pull phonebook from PBAP server PSE.
 *
 *  This API creates and sends a pull phonebook command to retrieve a complete phonebook
 *  object from the PSE.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer to be sent. Can be allocated by @ref bt_pbap_pce_create_pdu
 *             in application before this function is called. Should contain any optional
 *             application parameters (filters, format, etc.).
 *  @param name Absolute path in the virtual folders architecture of the PSE, appended
 *              with the name of the file representation of one of the Phone Book Objects.
 *              Example: "telecom/pb.vcf" or "SIM1/telecom/ich.vcf"
 *  @param wait Value of Single Response Mode Parameter (SRMP) header. If the PBAP connection
 *              is based on L2CAP and the client wants the server to wait for the client's
 *              next request after sending a reply, this should be true. Otherwise false.
 *              For RFCOMM connections, this value is ignored and can be true or false.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_phonebook(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name,
			       bool wait);

/** @brief Create command for PBAP client PCE to pull vCard listing from PBAP server PSE.
 *
 *  This API creates and sends a pull vCard listing command to retrieve a list of vCard
 *  entries from a specific folder.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer to be sent. Can be allocated by @ref bt_pbap_pce_create_pdu
 *             in application before this function is called. Should contain any optional
 *             application parameters (search criteria, list range, etc.).
 *  @param name Name of the folder to be retrieved. The value shall not include any path
 *              information, and it can be empty. An empty name header may be sent to
 *              retrieve the vCard Listing object of the current folder.
 *              Example: "pb" or "" (empty for current folder)
 *  @param wait Value of Single Response Mode Parameter (SRMP) header. If the PBAP connection
 *              is based on L2CAP and the client wants the server to wait for the client's
 *              next request after sending a reply, this should be true. Otherwise false.
 *              For RFCOMM connections, this value is ignored and can be true or false.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_vcardlisting(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name,
				  bool wait);

/** @brief Create command for PBAP client PCE to pull vCard entry from PBAP server PSE.
 *
 *  This API creates and sends a pull vCard entry command to retrieve a single vCard
 *  object from the current folder.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer to be sent. Can be allocated by @ref bt_pbap_pce_create_pdu
 *             in application before this function is called. Should contain any optional
 *             application parameters (format, property selector, etc.).
 *  @param name Name or X-BT-UID of the object to be retrieved. The value shall not include
 *              any path information.
 *              Example: "0.vcf" or "X-BT-UID:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
 *  @param wait Value of Single Response Mode Parameter (SRMP) header. If the PBAP connection
 *              is based on L2CAP and the client wants the server to wait for the client's
 *              next request after sending a reply, this should be true. Otherwise false.
 *              For RFCOMM connections, this value is ignored and can be true or false.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_vcardentry(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name,
				bool wait);

/** @brief Continue a multi-packet pull operation.
 *
 *  This API is used to continue receiving data from a pull operation that requires
 *  multiple packets (when the response code is BT_PBAP_RSP_CODE_CONTINUE).
 *  It sends a GET request without name header to retrieve the next chunk of data.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf Buffer to be sent. Can be allocated by @ref bt_pbap_pce_create_pdu.
 *             May contain application parameters if needed.
 *  @param wait Value of Single Response Mode Parameter (SRMP) header. If the PBAP connection
 *              is based on L2CAP and the client wants the server to wait for the client's
 *              next request after sending a reply, this should be true. Otherwise false.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_continue(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, bool wait);

/** @brief Send command to PSE to set current folder path.
 *
 *  This function is to be called after the PBAP connection is established.
 *  This API is used to set the current folder in PSE's virtual folder hierarchy.
 *
 *  Path navigation rules:
 *  - When name is "/": Go to root directory
 *  - When name is ".." or "../": Go up one level (to parent directory)
 *  - When name is "child" or "./child": Go down to child folder
 *
 *  For multilevel jumps, this operation needs to be done on a level-by-level basis.
 *  After receiving the response, the callback @ref bt_pbap_pce_cb::set_path that is
 *  registered by @ref bt_pbap_pce_register is called.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *  @param buf TX network buffer. Can be allocated by @ref bt_pbap_pce_create_pdu
 *             in application before this function is called.
 *  @param name Path name string indicating the target folder.
 *              Examples: "/" : return root; ".." :return parent folder;
 *                        "./telecom" : go to telecom folder
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_set_path(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name);

/** @brief Abort PBAP PCE operation.
 *
 *  Aborts the current PBAP PCE GET operation. This cancels the current outstanding operation.
 *  The return value of -EINPROGRESS means abort is queued and pending. The current
 *  outstanding operation will be aborted when receiving the next response from the PSE.
 *
 *  After the abort is processed, the callback @ref bt_pbap_pce_cb::abort that is
 *  registered by @ref bt_pbap_pce_register is called.
 *
 *  @param pbap_pce PBAP PCE object @ref bt_pbap_pce.
 *
 *  @return 0 in case of success or otherwise in case of error.
 */
int bt_pbap_pce_abort(struct bt_pbap_pce *pbap_pce);

/**
 * @brief Helper for adding application parameters to PBAP request.
 *
 * A helper macro for adding Application Parameters header to a PBAP request buffer.
 * Application parameters are used to specify various options for PBAP operations,
 * such as filters, search criteria, list ranges, format preferences, etc.
 *
 * The application parameters should be formatted as TLV (Tag-Length-Value) triplets
 * according to the PBAP specification.
 *
 * @param buf    Network buffer to which the application parameters will be added.
 *               Should be allocated via bt_pbap_pce_create_pdu.
 * @param count  Total length in bytes of the application parameters data.
 * @param data   Pointer to the application parameters data in TLV format.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_add_app_param(buf, count, data) bt_obex_add_header_app_param(buf, count, data)

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
 *               Should be allocated via bt_pbap_pce_create_pdu.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_add_header_srm_param(buf) bt_obex_add_header_srm_param(buf, 0x01)

/**
 * @brief Helper for extracting application parameters from PBAP response.
 *
 * A helper macro for extracting Application Parameters header from a received PBAP response.
 * The application parameters in the response contain information such as phonebook size,
 * new missed calls count, folder version counters, database identifier, etc.
 *
 * The extracted data is in TLV (Tag-Length-Value) format and should be parsed using
 * bt_pbap_tlv_parse or manually according to the PBAP specification.
 *
 * @param buf         Network buffer returned in the callback registered by bt_pbap_pce_register.
 * @param len         Pointer to variable that will receive the length of application parameters.
 * @param app_param   Pointer that will be set to point to the application parameters data.
 *
 * @return 0 on success, negative error code on failure.
 * @retval 0 Application parameters header found and extracted successfully.
 * @retval -ENOENT Application parameters header not present in buffer.
 */
#define bt_pbap_get_header_app_param(buf, len, app_param)                                          \
	bt_obex_get_header_app_param(buf, len, app_param)

/**
 * @brief Helper for getting body data from response buffer.
 *
 * A helper macro for extracting Body header data from a received PBAP response.
 * The most common scenario is to call this helper on the buffer received in
 * the callback that was registered via bt_pbap_pce_register.
 *
 * The Body header is used when the object being transferred is large and needs
 * to be sent in multiple packets. This is typically followed by an End of Body header
 * in the final packet.
 *
 * @param buf    Network buffer returned in the callback registered by bt_pbap_pce_register.
 * @param len    Pointer to variable that will receive the length of body data.
 * @param body   Pointer that will be set to point to the body data.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_get_body(buf, len, body) bt_obex_get_header_body(buf, len, body)

/**
 * @brief Helper for getting end of body data from response buffer.
 *
 * A helper macro for extracting End of Body header data from a received PBAP response.
 * The most common scenario is to call this helper on the buffer received in
 * the callback that was registered via bt_pbap_pce_register.
 *
 * The End of Body header indicates the final chunk of data in a multi-packet transfer,
 * or contains the complete data if the object fits in a single packet.
 *
 * @param buf    Network buffer returned in the callback registered by bt_pbap_pce_register.
 * @param len    Pointer to variable that will receive the length of body data.
 * @param body   Pointer that will be set to point to the end of body data.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_get_end_body(buf, len, body) bt_obex_get_header_end_body(buf, len, body)

/**
 * @brief Helper for parsing TLV-formatted application parameters.
 *
 * A helper macro for parsing TLV (Tag-Length-Value) formatted application parameters
 * received in PBAP responses. This macro iterates through the TLV data and calls
 * the provided callback function for each TLV triplet found.
 *
 * The callback function should have the signature:
 * bool callback(uint8_t tag, uint8_t len, const uint8_t *value, void *user_data)
 *
 * The callback should return true to continue parsing, or false to stop.
 *
 * @param len        Length of the TLV data to parse.
 * @param data       Pointer to the TLV-formatted data.
 * @param func       Callback function to be called for each TLV triplet.
 * @param user_data  User-defined data to be passed to the callback function.
 *
 * @return 0 on success, negative error code on failure.
 */
#define bt_pbap_tlv_parse(len, data, func, user_data) bt_obex_tlv_parse(len, data, func, user_data)
/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_PBAP_H_ */
