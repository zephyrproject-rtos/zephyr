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

/** @brief PBAP client PCE struct */
struct bt_pbap_pce {
	/** @brief connection handle */
	struct bt_conn *acl;
	/** @brief password for authication. When connecting, the application must provide this
	 * parameter when the component wants to authenticate with the server.
	 */
	const uint8_t *pwd;
	/** @brief peer device support features. After performing SDP to the server,
	 *  the PCE can obtain the features supported by the PSE.
	 */
	uint32_t peer_feature;
	/** @brief local device support feature. If PSE provide PCE, application should provide PCE
	 * features. */
	uint32_t lcl_feature;
	/** @brief max package length. When performing a connect operation, the application must
	 * provide this parameter.*/
	uint16_t mpl;
	/** @internal goep handle, @brief bt_goep */
	struct bt_goep *goep;
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
	 * @param pbap  The PBAP PCE object
	 * @param mpl   The max package length of buf that application can use
	 */
	void (*connect)(struct bt_pbap_pce *pbap, uint16_t mpl);

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
	 * @param pbap The PBAP PCE object
	 * @param rsp_code  Response code @ref bt_pbap_rsp_code
	 */
	void (*disconnect)(struct bt_pbap_pce *pbap, uint8_t rsp_code);

	/** @brief PBAP PCE pull phonebook response callback
	 *
	 * if this callback is provided it will be called when the PCE pull phonebook
	 * response is received.
	 *
	 * @param pbap The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers
	 */
	void (*pull_phonebook)(struct bt_pbap_pce *pbap, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE pull vcardlisting response callback
	 *
	 * if this callback is provided it will be called when the PCE pull vcardlisting
	 * response is received.
	 *
	 * @param pbap The PBAP PCE object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers
	 */
	void (*pull_vcardlisting)(struct bt_pbap_pce *pbap, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE pull vcardentey response callback
	 *
	 * if this callback is provided it will be called when the PCE pull vcardentey
	 * response is received.
	 *
	 * @param pbap The PBAP object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 * @param buf Optional response headers
	 */
	void (*pull_vcardentry)(struct bt_pbap_pce *pbap, uint8_t rsp_code, struct net_buf *buf);

	/** @brief PBAP PCE set path response callback
	 *
	 * if this callback is provided it will be called when the PCE set path
	 * response is received.
	 *
	 * @param pbap The PBAP object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 */
	void (*set_path)(struct bt_pbap_pce *pbap, uint8_t rsp_code);

	/** @brief PBAP PCE abort response callback
	 *
	 * if this callback is provided it will be called when the PCE abort
	 * response is received.
	 *
	 * @param pbap The PBAP object
	 * @param rsp_code Response code @ref bt_pbap_rsp_code
	 */
	void (*abort)(struct bt_pbap_pce *pbap, uint8_t rsp_code);
};

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

/** @brief PBAP client PCE register.
 *
 *  Register PCE application callback @brief bt_pbap_pce_cb.
 *  All other operations need to be performed after this function.
 *
 *  @param cb PBAP client callback @ref bt_pbap_pce_cb.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_register(struct bt_pbap_pce_cb *cb);

/** @brief Allocate the buffer from given pool after reserving head room for PBAP client PCE.
 *
 *  For PBAP connection over RFCOMM, the reserved head room includes OBEX, RFCOMM, L2CAP and ACL
 *  headers.
 *  For PBAP connection over L2CAP, the reserved head room includes OBEX, L2CAP and ACL headers.
 *
 *  @param pbap_pce PBAP PCE object
 *  @param pool Which pool to take the buffer from
 *
 *  @return New buffer.
 */
struct net_buf *bt_pbap_create_pdu(struct bt_pbap_pce *pbap_pce, struct net_buf_pool *pool);

/** @brief PBAP client PCE connect PBAP server PSE over RFCOMM.
 *
 *  PBAP connect over RFCOMM, once the connection is completed, the callback
 *  @ref bt_pbap_pce::connected is called. If the connection is rejected,
 *  @ref bt_pbap_pce::disconnected callback is called instead.
 *
 *  The Acl connection handle @brief bt_conn is passed as first parameter.
 *  The RFCOMM channel is passed as second paramter. The RFCOMM channel
 *  of the PBAP server PSE can be obtained through SDP operation.
 *
 *  The PBAP PCE object is passed (over an address of it) as third parameter, application should
 *  create PBAP PCE object @ref bt_pbap_pce. Then pass to this API the location (address).
 *
 *  @param conn Acl connection handle
 *  @param channel RFCOMM channel
 *  @param pbap_pce PBAP PCE object @brief bt_pbap_pce
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_rfcomm_connect(struct bt_conn *conn, uint8_t channel, struct bt_pbap_pce *pbap_pce);

/** @brief PBAP client PCE connect PBAP server PSE over L2CAP.
 *
 *  PBAP connect over L2CAP, once the connection is completed, the callback
 *  @ref bt_pbap_pce::connected is called. If the connection is rejected,
 *  @ref bt_pbap_pce::disconnected callback is called instead.
 *

 *  The Acl connection handle @brief bt_conn is passed as first parameter.
 *  The L2cap PSM is passed as second paramter. The L2cap PSM
 *  of the PBAP server PSE can be obtained through SDP operation.
 *
 *  The PBAP PCE object is passed (over an address of it) as third parameter, application should
 *  create PBAP PCE object @ref bt_pbap_pce. Then pass to this API the location (address).
 *
 *  @param conn Acl connection handle
 *  @param psm  L2cap PSM
 *  @param pbap_pce PBAP PCE object @brief bt_pbap_pce
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_l2cap_connect(struct bt_conn *conn, uint16_t psm, struct bt_pbap_pce *pbap_pce);

/** @brief Disconnect PBAP connection from PBAP client PCE. */
/**
 *  Disconnect PBAP connection.
 *  The enforce is passed as second paramter. If it is true,
 *  a connection to be terminated by closing the transport connection without issuing the
 *  OBEX DISCONNECT operation. If it is false, send an OBEX DISCONNECT firstly. If obex disconnect
 *  successfully, automatic close the transport connection. If obex disconnect fails,
 *  disconnect callback which register via @brief bt_pbap_pce_register will be called,
 *  and not close the transport connection. Application should recall @brief bt_pbap_pce_disconnect
 *  and @param enforce should be true to disconnect.
 *
 *  @param pbap_pce PBAP PCE object @brief bt_pbap_pce
 *  @param enforce  If true, closing the transport connection without issuing the
 *                  OBEX DISCONNECT operation
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_disconnect(struct bt_pbap_pce *pbap_pce, bool enforce);

/** @brief Create commmand for PBAP client PCE to pull phonebook from PBAP server PSE. */
/**
 *  This API is used to create a pull phonebook command.
 *
 *  The pbap pce handle @brief bt_pbap_pce is passed as first parameter.
 *  The buf is passed as second paramter. It can be allocated by func @brief bt_pbap_create_pdu
 *  in application before this funcation is called.
 *  The name is passed as third paramter. It shall contain the absolute path in the
 *  virtual folders architecture of the PSE, appended with the name of the file representation of
 *  one of the Phone Book Objects.Example: telecom/pb.vcf
 *  The wait is passed as fourth paramter.It means the value of header Single Response Mode
 * Param(SRMP). If the PBAP connection based on L2CAP and the client wants the server to wait for
 * the client's next request after sending a reply, this value should be True, Otherwith it should
 * be False. If the PBAP connection based on RFcomm, this value is meaningless, so it can be ture or
 * false.
 *
 *  @param pbap_pce PBAP PCE object @brief bt_pbap_pce
 *  @param buf Buffer needs to be sent.
 *  @param name the name of virtual folders which want to pull phonebook from.
 *  @param wait true if enable SRMP else false.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_phonebook_create_cmd(struct bt_pbap_pce *pbap_pce, struct net_buf *buf,
					  char *name, bool wait);

/** @brief Create commmand for PBAP client PCE to pull vcardlisting from PBAP server PSE. */
/**
 *  This API is used to create a pull vcardlisting command.
 *
 *  The pbap pce handle @brief bt_pbap_pce is passed as first parameter.
 *  The buf is passed as second paramter. It can be allocated by func @brief bt_pbap_create_pdu
 *  in application before this funcation is called.
 *  The name is passed as third paramter. It specifies the name of the folder to be retrieved.
 *  The value shall not include any path information,and it can be empty. An empty name
 *  header may be sent to retrieve the vCard Listing object of the current folder. Example pb.
 *  The wait is passed as fourth paramter.It means the value of header Single Response Mode
 * Param(SRMP). If the PBAP connection based on L2CAP and the client wants the server to wait for
 * the client's next request after sending a reply, this value should be True, Otherwith it should
 * be False. If the PBAP connection based on RFcomm, this value is meaningless, so it can be ture or
 * false.
 *
 *  @param pbap_pce PBAP PCE object @brief bt_pbap_pce
 *  @param buf Buffer needs to be sent.
 *  @param name the name of virtual folders which want to pull phonebook from.
 *  @param wait true if enable SRMP else false.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_vcardlisting_create_cmd(struct bt_pbap_pce *pbap_pce, struct net_buf *buf,
					     char *name, bool wait);

/** @brief Create commmand for PBAP client PCE to pull vcardentry from PBAP server PSE. */
/**
 *  This API is used to create a pull vcardentry command.
 *
 *  The pbap pce handle @brief bt_pbap_pce is passed as first parameter.
 *  The buf is passed as second paramter. It can be allocated by func @brief bt_pbap_create_pdu
 *  in application before this funcation is called.
 *  The name is passed as third paramter. The Name header shall be used to indicate the name or,
 *  if supported, the X-BT-UID of the object to be retrieved.The value shall not include any path
 * information. Exmaple 0.vcf or X-BT-UID:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX. The wait is passed as
 * fourth paramter.It means the value of header Single Response Mode Param(SRMP). If the PBAP
 * connection based on L2CAP and the client wants the server to wait for the client's next request
 *  after sending a reply, this value should be True, Otherwith it should be False.
 *  If the PBAP connection based on RFcomm, this value is meaningless, so it can be ture or false.
 *
 *  @param pbap_pce PBAP PCE object @brief bt_pbap_pce
 *  @param buf Buffer needs to be sent.
 *  @param name the name of virtual folders which want to pull phonebook from.
 *  @param wait true if enable SRMP else false.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_pull_vcardentry_create_cmd(struct bt_pbap_pce *pbap_pce, struct net_buf *buf,
					   char *name, bool wait);

/** @brief Send commmand for PBAP client PCE to pull something from PBAP server PSE. */
/**
 *  This API is used to send a pull command.
 *
 *  Before this function is called, bt_pbap_pce_pull_xxx_create_cmd must be called.
 *  If application want to add extra header, call bt_pbap_pce_add_header_xxx to add header to buf.
 *
 *  @param pbap_pce PBAP PCE object @brief bt_pbap_pce
 *  @param buf Buffer needs to be sent.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_pbap_pce_send_cmd(struct bt_pbap_pce *pbap_pce, struct net_buf *buf);

/** @brief Send cmd to PSE to set current folderpath.
 *
 *  This function is to be called after the pabp conn is established.
 *  This API is to be used to set sets the current folder in pse.
 *  When name is "/", go to parent directory.
 *  When name is ".." or "../", go up one level.
 *  When name is "child" or "./child", go to child
 *  For multilevel jumps, need to do it on a level-by-level basis
 *  After receive responese, the callback that is registered by
 *  bt_pbap_pce_register is called.
 *  The buf is passed as second paramter. It can be allocated by func @brief bt_pbap_create_pdu
 *  in application before this funcation is called.
 *
 *  @param pbap_pce  PBAP PCE object
 *  @param buf       tx net_buf
 *  @param name      path name string.
 *
 *  @return          0 in case of success or otherwise in case of error.
 */

int bt_pbap_pce_set_path(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name);

/** @brief Abort PBAP_PCE operation
 *
 *  Abort PBAP_PCE GET operation. This cancels the current outstanding operation.
 *  The return value of -EINPROGRESS means abort is queued and pending. The current
 *  outstanding operation will be aborted when receiving next response from the PSE.
 *
 *  @param pbap_pce  PBAP_PCE object.
 *
 *  @return         0 in case of success or otherwise in case of error.
 */
int bt_pbap_pce_abort(struct bt_pbap_pce *pbap_pce);

/**
 * @brief Helper for getting body.
 *
 * A helper for getting Body header. The most
 * common scenario is to call this helper on the body received in
 * the callback that was given to bt_pbap_register.
 *
 * @param buf       net buffer returned in the callback registered by bt_pbap_register.
 * @param body      pointer used for holding body data.
 * @param length    the length of body data.
 */
#define bt_pbap_pce_get_body(buf, len, body) bt_obex_get_header_body(buf, len, body)

/**
 * @brief Helper for getting end of body.
 *
 * A helper for getting end of body header. The most
 * common scenario is to call this helper on the end of body received in
 * the callback that was given to bt_pbap_register.
 *
 * @param buf       net buffer returned in the callback registered by bt_pbap_register.
 * @param body      pointer used for holding body data.
 * @param length    the length of body data.
 */
#define bt_pbap_pce_get_end_body(buf, len, body) bt_obex_get_header_end_body(buf, len, body)

#define bt_pbap_pce_add_app_param(buf, count, data) bt_obex_add_header_app_param(buf, count, data)

#define bt_pbap_pce_get_header_app_param(buf, len, app_param)                                      \
	bt_obex_get_header_app_param(buf, len, app_param)

#define bt_pbap_pce_tlv_parse(len, data, func, user_data)                                          \
	bt_obex_tlv_parse(len, data, func, user_data)
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
