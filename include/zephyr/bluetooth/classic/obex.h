/* obex.h - IrDA Oject Exchange Protocol handling */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_OBEX_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_OBEX_H_

/**
 * @brief Bluetooth APIs
 * @defgroup bluetooth Bluetooth APIs
 * @ingroup connectivity
 * @{
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IrDA Oject Exchange Protocol (OBEX)
 * @defgroup bt_obex IrDA Oject Exchange Protocol (OBEX)
 * @since 1.0
 * @version 1.0.0
 * @ingroup bluetooth
 * @{
 */

/** @brief OBEX Response Code. */
enum __packed bt_obex_rsp_code {
	/** Continue */
	BT_OBEX_RSP_CODE_CONTINUE = 0x90,
	/** OK */
	BT_OBEX_RSP_CODE_OK = 0xa0,
	/** Success */
	BT_OBEX_RSP_CODE_SUCCESS = 0xa0,
	/** Created */
	BT_OBEX_RSP_CODE_CREATED = 0xa1,
	/** Accepted */
	BT_OBEX_RSP_CODE_ACCEPTED = 0xa2,
	/** Non-Authoritative Information  */
	BT_OBEX_RSP_CODE_NON_AUTH_INFO = 0xa3,
	/** No Content */
	BT_OBEX_RSP_CODE_NO_CONTENT = 0xa4,
	/** Reset Content */
	BT_OBEX_RSP_CODE_RESET_CONTENT = 0xa5,
	/** Partial Content */
	BT_OBEX_RSP_CODE_PARTIAL_CONTENT = 0xa6,
	/** Multiple Choices */
	BT_OBEX_RSP_CODE_MULTI_CHOICES = 0xb0,
	/** Moved Permanently */
	BT_OBEX_RSP_CODE_MOVED_PERM = 0xb1,
	/** Moved temporarily */
	BT_OBEX_RSP_CODE_MOVED_TEMP = 0xb2,
	/** See Other */
	BT_OBEX_RSP_CODE_SEE_OTHER = 0xb3,
	/** Not modified */
	BT_OBEX_RSP_CODE_NOT_MODIFIED = 0xb4,
	/** Use Proxy */
	BT_OBEX_RSP_CODE_USE_PROXY = 0xb5,
	/** Bad Request - server couldn’t understand request */
	BT_OBEX_RSP_CODE_BAD_REQ = 0xc0,
	/** Unauthorized */
	BT_OBEX_RSP_CODE_UNAUTH = 0xc1,
	/** Payment Required */
	BT_OBEX_RSP_CODE_PAY_REQ = 0xc2,
	/** Forbidden - operation is understood but refused */
	BT_OBEX_RSP_CODE_FORBIDDEN = 0xc3,
	/** Not Found */
	BT_OBEX_RSP_CODE_NOT_FOUND = 0xc4,
	/** Method Not Allowed */
	BT_OBEX_RSP_CODE_NOT_ALLOW = 0xc5,
	/** Not Acceptable */
	BT_OBEX_RSP_CODE_NOT_ACCEPT = 0xc6,
	/** Proxy Authentication Required */
	BT_OBEX_RSP_CODE_PROXY_AUTH_REQ = 0xc7,
	/** Request Time Out */
	BT_OBEX_RSP_CODE_REQ_TIMEOUT = 0xc8,
	/** Conflict */
	BT_OBEX_RSP_CODE_CONFLICT = 0xc9,
	/** Gone */
	BT_OBEX_RSP_CODE_GONE = 0xca,
	/** Length Required */
	BT_OBEX_RSP_CODE_LEN_REQ = 0xcb,
	/** Precondition Failed */
	BT_OBEX_RSP_CODE_PRECON_FAIL = 0xcc,
	/** Requested Entity Too Large */
	BT_OBEX_RSP_CODE_ENTITY_TOO_LARGE = 0xcd,
	/** Requested URL Too Large */
	BT_OBEX_RSP_CODE_URL_TOO_LARGE = 0xce,
	/** Unsupported media type */
	BT_OBEX_RSP_CODE_UNSUPP_MEDIA_TYPE = 0xcf,
	/** Internal serve Error */
	BT_OBEX_RSP_CODE_INTER_ERROR = 0xd0,
	/** Not Implemented */
	BT_OBEX_RSP_CODE_NOT_IMPL = 0xd1,
	/** Bad Gateway */
	BT_OBEX_RSP_CODE_BAD_GATEWAY = 0xd2,
	/** Service Unavailable */
	BT_OBEX_RSP_CODE_UNAVAIL = 0xd3,
	/** Gateway Timeout */
	BT_OBEX_RSP_CODE_GATEWAY_TIMEOUT = 0xd4,
	/** HTTP Version not supported */
	BT_OBEX_RSP_CODE_VER_UNSUPP = 0xd5,
	/** Database Full */
	BT_OBEX_RSP_CODE_DB_FULL = 0xe0,
	/** Database Locked */
	BT_OBEX_RSP_CODE_DB_LOCK = 0xe1,
};

/** Converts a OBEX response code to string.
 *
 * @param rsp_code Response code.
 *
 * @return The string representation of the response code.
 *         If @kconfig{CONFIG_BT_OEBX_RSP_CODE_TO_STR} is not enabled,
 *         this just returns the empty string.
 */
#if defined(CONFIG_BT_OEBX_RSP_CODE_TO_STR)
const char *bt_obex_rsp_code_to_str(enum bt_obex_rsp_code rsp_code);
#else
static inline const char *bt_obex_rsp_code_to_str(enum bt_obex_rsp_code rsp_code)
{
	ARG_UNUSED(rsp_code);

	return "";
}
#endif /* CONFIG_BT_OEBX_RSP_CODE_TO_STR */

/** @brief  OBEX Header ID */
enum __packed bt_obex_header_id {
	/** Number of objects (used by Connect) */
	BT_OBEX_HEADER_ID_COUNT = 0xC0,

	/** Name of the object (often a file name) */
	BT_OBEX_HEADER_ID_NAME = 0x01,

	/** Type of object - e.g. text, html, binary, manufacturer specific */
	BT_OBEX_HEADER_ID_TYPE = 0x42,

	/** The length of the object in bytes */
	BT_OBEX_HEADER_ID_LEN = 0xC3,

	/** Date/time stamp – ISO 8601 version - preferred */
	BT_OBEX_HEADER_ID_TIME_ISO_8601 = 0x44,

	/** Date/time stamp – 4 byte version (for compatibility only) */
	BT_OBEX_HEADER_ID_TIME = 0xC4,

	/** Text description of the object */
	BT_OBEX_HEADER_ID_DES = 0x05,

	/** Name of service that operation is targeted to */
	BT_OBEX_HEADER_ID_TARGET = 0x46,

	/** An HTTP 1.x header */
	BT_OBEX_HEADER_ID_HTTP = 0x47,

	/** A chunk of the object body. */
	BT_OBEX_HEADER_ID_BODY = 0x48,

	/** The final chunk of the object body. */
	BT_OBEX_HEADER_ID_END_BODY = 0x49,

	/** Identifies the OBEX application, used to tell if talking to a peer. */
	BT_OBEX_HEADER_ID_WHO = 0x4A,

	/** An identifier used for OBEX connection multiplexing. */
	BT_OBEX_HEADER_ID_CONN_ID = 0xCB,

	/** Extended application request & response information. */
	BT_OBEX_HEADER_ID_APP_PARAM = 0x4C,

	/** Authentication digest-challenge. */
	BT_OBEX_HEADER_ID_AUTH_CHALLENGE = 0x4D,

	/** Authentication digest-response. */
	BT_OBEX_HEADER_ID_AUTH_RSP = 0x4E,

	/** Indicates the creator of an object. */
	BT_OBEX_HEADER_ID_CREATE_ID = 0xCF,

	/** Uniquely identifies the network client (OBEX server). */
	BT_OBEX_HEADER_ID_WAN_UUID = 0x50,

	/** OBEX Object class of object. */
	BT_OBEX_HEADER_ID_OBJECT_CLASS = 0x51,

	/** Parameters used in session commands/responses. */
	BT_OBEX_HEADER_ID_SESSION_PARAM = 0x52,

	/** Sequence number used in each OBEX packet for reliability. */
	BT_OBEX_HEADER_ID_SESSION_SEQ_NUM = 0x93,

	/** Specifies the action to be performed (used in ACTION operation). */
	BT_OBEX_HEADER_ID_ACTION_ID = 0x94,

	/** The destination object name (used in certain ACTION operations). */
	BT_OBEX_HEADER_ID_DEST_NAME = 0x15,

	/** 4-byte bit mask for setting permissions. */
	BT_OBEX_HEADER_ID_PERM = 0xD6,

	/** 1-byte value to setup Single Response Mode (SRM). */
	BT_OBEX_HEADER_ID_SRM = 0x97,

	/** 1-byte value for setting parameters used during Single Response Mode (SRM). */
	BT_OBEX_HEADER_ID_SRM_PARAM = 0x98,
};

#define BT_OBEX_SEND_BUF_RESERVE 7

struct bt_obex;

/** @brief OBEX server operations structure.
 *
 * The object has to stay valid and constant for the lifetime of the OBEX server.
 */
struct bt_obex_server_ops {
	/** @brief OBEX connect request callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX connect request
	 *  is received.
	 *
	 *  @param obex The OBEX object.
	 *  @param version OBEX version number.
	 *  @param mopl Maximum OBEX packet length.
	 *  @param buf Sequence of headers.
	 */
	void (*connect)(struct bt_obex *obex, uint8_t version, uint16_t mopl, struct net_buf *buf);

	/** @brief OBEX disconnect request callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX disconnect request
	 *  is received.
	 *
	 *  @param obex The OBEX object.
	 *  @param buf Sequence of headers.
	 */
	void (*disconnect)(struct bt_obex *obex, struct net_buf *buf);

	/** @brief OBEX put request callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX put request is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param final If the final bit is set.
	 *  @param buf Sequence of headers.
	 */
	void (*put)(struct bt_obex *obex, bool final, struct net_buf *buf);

	/** @brief OBEX get request callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX get request is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param final If the final bit is set.
	 *  @param buf Sequence of headers.
	 */
	void (*get)(struct bt_obex *obex, bool final, struct net_buf *buf);

	/** @brief OBEX abort request callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX abort request is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param buf Optional headers.
	 */
	void (*abort)(struct bt_obex *obex, struct net_buf *buf);

	/** @brief OBEX SetPath request callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX SetPath request is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param flags The flags.
	 *  @param buf Optional headers.
	 */
	void (*setpath)(struct bt_obex *obex, uint8_t flags, struct net_buf *buf);

	/** @brief OBEX action request callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX action request is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param final If the final bit is set.
	 *  @param buf Sequence of headers (Including action identifier header if it exists).
	 */
	void (*action)(struct bt_obex *obex, bool final, struct net_buf *buf);
};

/** @brief OBEX client operations structure.
 *
 * The object has to stay valid and constant for the lifetime of the OBEX client.
 */
struct bt_obex_client_ops {
	/** @brief OBEX connect response callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX connect response
	 *  is received.
	 *
	 *  @param obex The OBEX object.
	 *  @param rsp_code Response code.
	 *  @param version OBEX version number.
	 *  @param mopl Maximum OBEX packet length.
	 *  @param buf Sequence of headers.
	 */
	void (*connect)(struct bt_obex *obex, uint8_t rsp_code, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/** @brief OBEX disconnect response callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX disconnect response
	 *  is received.
	 *
	 *  @param obex The OBEX object.
	 *  @param rsp_code Response code.
	 *  @param buf Sequence of headers.
	 */
	void (*disconnect)(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

	/** @brief OBEX put response callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX put response is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param rsp_code Response code.
	 *  @param buf Optional response headers.
	 */
	void (*put)(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

	/** @brief OBEX get response callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX get response is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param rsp_code Response code.
	 *  @param buf Optional response headers.
	 */
	void (*get)(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

	/** @brief OBEX abort response callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX abort response is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param rsp_code Response code.
	 *  @param buf Optional response headers.
	 */
	void (*abort)(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

	/** @brief OBEX SetPath response callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX SetPath response is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param rsp_code Response code.
	 *  @param buf Optional response headers.
	 */
	void (*setpath)(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

	/** @brief OBEX action response callback
	 *
	 *  If this callback is provided it will be called whenever the OBEX action response is
	 *  received.
	 *
	 *  @param obex The OBEX object.
	 *  @param rsp_code Response code.
	 *  @param buf Optional response headers.
	 */
	void (*action)(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);
};

/** @brief OBEX transport operations structure.
 *
 * The object has to stay valid and constant for the lifetime of the OBEX client/server.
 */
struct bt_obex_transport_ops {
	/** @brief allocate buf callback
	 *
	 *  If this callback is provided it will be called to allocate a net buffer to store
	 *  the outgoing data.
	 *
	 *  @param obex The OBEX object.
	 *  @param pool Which pool to take the buffer from.
	 *
	 *  @return Allocated buffer.
	 */
	struct net_buf *(*alloc_buf)(struct bt_obex *obex, struct net_buf_pool *pool);

	/** @brief Send OBEX data via transport
	 *
	 *  If this callback is provided it will be called to send data via OBEX transport.
	 *
	 *  @param obex The OBEX object.
	 *  @param buf OBEX packet.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 *  @return -EMSGSIZE if `buf` is larger than `obex`'s MOPL.
	 */
	int (*send)(struct bt_obex *obex, struct net_buf *buf);

	/** @brief Disconnect from transport
	 *
	 *  If this callback is provided it will be called to disconnect transport.
	 *
	 *  @param obex The OBEX object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*disconnect)(struct bt_obex *obex);
};

/** @brief Life-span states of OBEX.
 *
 *  Used only by internal APIs dealing with setting OBEX to proper state depending on operational
 *  context.
 *
 *  OBEX enters the @ref BT_OBEX_CONNECTING state upon @ref bt_obex_connect, or upon returning
 *  from @ref bt_obex_server_ops::connect.
 *
 *  When OBEX leaves the @ref BT_OBEX_CONNECTING state and enters the @ref BT_OBEX_CONNECTED,
 *  @ref bt_obex_connect_rsp or @ref bt_obex_client_ops::connect is called with the response code
 *  @ref BT_OBEX_RSP_CODE_SUCCESS.
 *
 *  When OBEX enters the @ref BT_OBEX_DISCONNECTED from other states,
 *  @ref bt_obex_client_ops::disconnect or @ref bt_obex_connect_rsp is called with the response code
 *  @ref BT_OBEX_RSP_CODE_SUCCESS. Or OBEX transport enters the disconnected state from other OBEX
 *  transport states.
 */
enum __packed bt_obex_state {
	/** OBEX disconnected */
	BT_OBEX_DISCONNECTED,
	/** OBEX in connecting state */
	BT_OBEX_CONNECTING,
	/** OBEX ready for upper layer traffic on it */
	BT_OBEX_CONNECTED,
	/** OBEX in disconnecting state */
	BT_OBEX_DISCONNECTING,
};

/** @brief OBEX structure. */
struct bt_obex {
	/** @brief OBEX Server operations
	 *
	 *  If it is a obex sever, the upper layer should pass the operations of server to
	 *  `server_ops` when providing the OBEX structure.
	 */
	const struct bt_obex_server_ops *server_ops;

	/** @brief OBEX Client operations
	 *
	 *  If it is a obex client, the upper layer should pass the operations of client to
	 *  `client_ops` when providing the OBEX structure.
	 */
	const struct bt_obex_client_ops *client_ops;

	struct {
		/** @brief MTU of OBEX transport */
		uint16_t mtu;
		/** @brief The Maximum OBEX Packet Length (MOPL) */
		uint16_t mopl;
	} rx;

	struct {
		/** @brief MTU of OBEX transport */
		uint16_t mtu;
		/** @brief The Maximum OBEX Packet Length (MOPL) */
		uint16_t mopl;
	} tx;

	/** @internal OBEX transport operations */
	const struct bt_obex_transport_ops *_transport_ops;

	/** @internal Saves the current state, @ref bt_obex_state */
	atomic_t _state;

	/** @internal OBEX opcode */
	atomic_t _opcode;

	/** @internal OBEX previous opcode */
	atomic_t _pre_opcode;
};

/** @brief OBEX connect request
 *
 *  The connect operation initiates the connection and sets up the basic expectations of each side
 *  of the link. The connect request must fit in a single packet.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the connect request is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `Authenticate Challenge` is packed by calling
 *  @ref bt_obex_add_header_auth_challenge.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param mopl Maximum OBEX packet length.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_connect(struct bt_obex *obex, uint16_t mopl, struct net_buf *buf);

/** @brief OBEX connect response
 *
 *  The connect response is used to acknowledged connect request packet.
 *  The connect response must fit in a single packet.
 *
 *  The second parameter `rsp_code` is used to pass the response code @ref bt_obex_rsp_code. The
 *  typical values @ref BT_OBEX_RSP_CODE_SUCCESS for `success`.
 *  The 4th parameter `buf` saves the packet data (sequence of headers) of the connect response is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `Authenticate Response` is packed by calling
 *  @ref bt_obex_add_header_auth_rsp.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param rsp_code Response code.
 *  @param mopl Maximum OBEX packet length.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_connect_rsp(struct bt_obex *obex, uint8_t rsp_code, uint16_t mopl, struct net_buf *buf);

/** @brief OBEX disconnect request
 *
 *  The disconnect operation signals the end of the OBEX connection from client side.
 *  The disconnect request must fit in a single packet.
 *
 *  The second parameter `buf` saves the packet data (sequence of headers) of the disconnect
 *  request is stored in second parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `Connection Id` is packed by calling
 *  @ref bt_obex_add_header_conn_id.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_disconnect(struct bt_obex *obex, struct net_buf *buf);

/** @brief OBEX disconnect response
 *
 *  The disconnect response is used to acknowledged disconnect request packet.
 *  The disconnect response must fit in a single packet.
 *
 *  The second parameter `rsp_code` is used to pass the response code @ref bt_obex_rsp_code. The
 *  typical values, @ref BT_OBEX_RSP_CODE_SUCCESS for `success`, @ref BT_OBEX_RSP_CODE_UNAVAIL for
 *  `service unavailable` if the header `Connection Id` is invalid.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the connect response
 *  is stored in third parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param rsp_code Response code.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_disconnect_rsp(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

/** @brief OBEX put request
 *
 *  The put operation sends one object from the client to the server.
 *  The put operation consists of one or more request packets, the last of which should have the
 *  second parameter `final` set.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the put request is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `Name` is packed by calling @ref bt_obex_add_header_name.
 *
 *  @note A put operation with NO Body or End-of-Body headers whatsoever should be treated as a
 *  delete request. Similarly, a put operation with an empty End-of-Body header requests the
 *  recipient to create an empty object. This definition may not make sense or apply to every
 *  implementation (in other words devices are not required to support delete operations or
 *  empty objects).
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param final The final bit of opcode.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_put(struct bt_obex *obex, bool final, struct net_buf *buf);

/** @brief OBEX put response
 *
 *  The put response is used to acknowledged each put request packet. It is sent from the server
 *  to client.
 *  The second parameter `rsp_code` is used to pass the response code @ref bt_obex_rsp_code. The
 *  typical values, @ref BT_OBEX_RSP_CODE_CONTINUE for `continue`, @ref BT_OBEX_RSP_CODE_SUCCESS
 *  for `success`.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the put response is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `srm` is packed by calling @ref bt_obex_add_header_srm.
 *  Or, the `buf` could be NULL if there is not any data needs to be sent.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param rsp_code Response code.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_put_rsp(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

/** @brief OBEX get request
 *
 *  The get operation requests that the server return an object to the client.
 *  The get operation consists of one or more request packets, the last of which should have the
 *  second parameter `final` set, and the request phase of the get is complete. Once a get request
 *  is sent with the final bit, all subsequent get request packets must set the final bit until
 *  the operation is complete.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the get request is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `Name` is packed by calling @ref bt_obex_add_header_name.
 *  Or, the `buf` could be NULL if there is not any data needs to be sent.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param final The final bit of opcode.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get(struct bt_obex *obex, bool final, struct net_buf *buf);

/** @brief OBEX get response
 *
 *  The get response is used to acknowledged get request packets. It is sent from the server
 *  to client.
 *  If the server has more than one object that fits the request, the behavior is system
 *  dependent. But the server device must not begin sending the object body chunks until the
 *  request phase is complete.
 *
 *  The second parameter `rsp_code` is used to pass the response code @ref bt_obex_rsp_code. The
 *  typical values, @ref BT_OBEX_RSP_CODE_CONTINUE for `continue`, @ref BT_OBEX_RSP_CODE_SUCCESS
 *  for `success`.
 *  A successful response for an object that fits entirely in one response packet is
 *  @ref BT_OBEX_RSP_CODE_SUCCESS (Success, with Final bit set) in the response code, followed
 *  by the object body. If the response is large enough to require multiple GET responses, only
 *  the last response is @ref BT_OBEX_RSP_CODE_SUCCESS, and the others are all
 *  @ref BT_OBEX_RSP_CODE_CONTINUE (Continue). The object is returned as a sequence of headers
 *  just as with put operation. Any other response code @ref bt_obex_rsp_code indicates failure.
 *  The typical failure response codes, @ref BT_OBEX_RSP_CODE_BAD_REQ for `Bad request - server
 *  couldn’t understand request`, @ref BT_OBEX_RSP_CODE_FORBIDDEN for `Forbidden - operation is
 *  understood but refused`.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the get response is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `srm` is packed by calling @ref bt_obex_add_header_srm.
 *  Or, the `buf` could be NULL if there is not any data needs to be sent.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param rsp_code Response code.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_rsp(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

/** @brief OBEX abort request
 *
 *  The abort request is used when the client decides to terminate a multi-packet operation (such
 *  as put) before it would be normally end. The abort request always fits in one OBEX packet and
 *  have the Final bit set.
 *  The second parameter `buf` saves the packet data (sequence of headers) of the abort request is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `description` is packed by calling
 *  @ref bt_obex_add_header_description. Or, the `buf` could be NULL if there is not any data
 *  needs to be sent.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_abort(struct bt_obex *obex, struct net_buf *buf);

/** @brief OBEX abort response
 *
 *  The abort response is used to acknowledged abort request packet. It is sent from the server
 *  to client.
 *  The abort response always fits in one OBEX packet and have the Final bit set.
 *
 *  The second parameter `rsp_code` is used to pass the response code @ref bt_obex_rsp_code. The
 *  typical value @ref BT_OBEX_RSP_CODE_SUCCESS for `success`.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the abort response is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Or, the `buf` could be NULL if there is not any data needs to be sent.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param rsp_code Response code.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_abort_rsp(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

/** @brief OBEX setpath request
 *
 *  The setpath request is used to set the "current folder" on the receiving side in order to
 *  enable transfers that need additional path information. The Path name is contained in a Name
 *  header.
 *  The setpath request always fits in one OBEX packet and have the Final bit set.
 *
 *  The second parameter `flags` is the setpath operation flag bit maps. Bit 0 means `backup a
 *  level before applying name (equivalent to ../ on many systems)`. Bit 1 means `Don’t create
 *  folder if it does not exist, return an error instead.` Other bits must be set to zero by
 *  sender and ignored by receiver.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the setpath request is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `Name` is packed by calling @ref bt_obex_add_header_name.
 *  If the header `Name` is added, it means the client wants to go one level down relative to the
 *  current folder into the folder `name`. Or, the client wants to go back to the default folder
 *  when no bit of flags set.
 *
 *  @note The Name header may be omitted when flags or constants indicate the entire operation
 *  being requested. For example, back up one level (bit 0 of `flags` is set), equivalent to
 *  "cd .." on some systems.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param flags Flags for setpath request.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_setpath(struct bt_obex *obex, uint8_t flags, struct net_buf *buf);

/** @brief OBEX setpath response
 *
 *  The setpath response is used to acknowledged setpath request packet. It is sent from the
 *  server to client.
 *  The setpath response always fits in one OBEX packet and have the Final bit set.
 *
 *  The second parameter `rsp_code` is used to pass the response code @ref bt_obex_rsp_code. The
 *  typical value @ref BT_OBEX_RSP_CODE_SUCCESS for `success`. Any other response code
 *  @ref bt_obex_rsp_code indicates failure. The typical failure response codes,
 *  @ref BT_OBEX_RSP_CODE_BAD_REQ for `Bad request - server couldn’t understand request`,
 *  @ref BT_OBEX_RSP_CODE_FORBIDDEN for `Forbidden - operation is understood but refused`.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the setpath response
 *  is stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Or, the `buf` could be NULL if there is not any data needs to be sent.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param rsp_code Response code.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_setpath_rsp(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

/** @brief OBEX Actions. */
enum __packed bt_obex_action_id {
	/** Copy Object Action */
	BT_OBEX_ACTION_COPY = 0x00,
	/**  Move/Rename Object Action */
	BT_OBEX_ACTION_MOVE_RENAME = 0x01,
	/** Set Object Permissions Action */
	BT_OBEX_ACTION_SET_PERM = 0x02,
};

/** @brief OBEX action request
 *
 *  The action operation is defined to cover the needs of common actions. The `Action Id` header
 *  @ref BT_OBEX_HEADER_ID_ACTION_ID is used in the action operation and contains an action
 *  identifier which defines what action is to be performed. All actions are optional and depends
 *  on the implementation of server.
 *
 *  There are three actions defined @ref bt_obex_action_id,
 *
 *  @ref BT_OBEX_ACTION_COPY is used to copy an object from one location to another. The header
 *  `Name` @ref BT_OBEX_HEADER_ID_NAME specifies the source file name and the header `Dest Name`
 *  @ref BT_OBEX_HEADER_ID_DEST_NAME specifies the destination file name. These two headers are
 *  mandatory for this action.
 *
 *  @ref BT_OBEX_ACTION_MOVE_RENAME is used to move an object from one location to another. It
 *  can also be used to rename an object. The header `Name` @ref BT_OBEX_HEADER_ID_NAME specifies
 *  the source file name and the header `Dest Name` @ref BT_OBEX_HEADER_ID_DEST_NAME specifies the
 *  destination file name. These two headers are mandatory for this action.
 *
 *  @ref BT_OBEX_ACTION_SET_PERM is used to set the access permissions of an object or folder.
 *  The header `Name` @ref BT_OBEX_HEADER_ID_NAME specifies the source file name and the header
 *  `Permissions` @ref BT_OBEX_HEADER_ID_PERM specifies the new permissions for this object. These
 *  two headers are mandatory for this action.
 *
 *  The action operation consists of one or more request packets, the last of which should have
 *  the second parameter `final` set.
 *  The third parameter `buf` saves the packet data (sequence of headers) of the action request is
 *  stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Such as header `Name` is packed by calling @ref bt_obex_add_header_name.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param final The final bit of opcode.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_action(struct bt_obex *obex, bool final, struct net_buf *buf);

/** @brief OBEX action response
 *
 *  The action response is used to acknowledged action request packets. It is sent from the server
 *  to client.
 *  The second parameter `rsp_code` is used to pass the response code @ref bt_obex_rsp_code. The
 *  typical value @ref BT_OBEX_RSP_CODE_SUCCESS for `success`.
 *
 *  There are three actions defined @ref bt_obex_action_id. For each action request, there are
 *  failure response codes corresponding to it.
 *
 *  For action @ref BT_OBEX_ACTION_COPY,
 *  @ref BT_OBEX_RSP_CODE_NOT_FOUND - `Source object or destination folder does not exist`.
 *  @ref BT_OBEX_RSP_CODE_FORBIDDEN - `Cannot modify the permissions of the destination
 *  object/folder, permission denied`.
 *  @ref BT_OBEX_RSP_CODE_DB_FULL - `Cannot create object in destination folder, out of memory`
 *  @ref BT_OBEX_RSP_CODE_CONFLICT - `Cannot modify the permissions, sharing violation, object or
 *  Set Object Permissions Action busy`.
 *  @ref BT_OBEX_RSP_CODE_NOT_IMPL - `Set Object Permissions Action not supported`.
 *  @ref BT_OBEX_RSP_CODE_NOT_MODIFIED - `Cannot create folder/file, destination folder/file
 *  already exits`.
 *
 *  For action @ref BT_OBEX_ACTION_MOVE_RENAME,
 *  @ref BT_OBEX_RSP_CODE_NOT_FOUND - `Source object or destination folder does not exist`.
 *  @ref BT_OBEX_RSP_CODE_FORBIDDEN - `Cannot modify the permissions of the destination
 *  object/folder, permission denied`.
 *  @ref BT_OBEX_RSP_CODE_DB_FULL - `Cannot create object in destination folder, out of memory`
 *  @ref BT_OBEX_RSP_CODE_CONFLICT - `Cannot modify the permissions, sharing violation, object or
 *  Set Object Permissions Action busy`.
 *  @ref BT_OBEX_RSP_CODE_NOT_IMPL - `Set Object Permissions Action not supported`.
 *  @ref BT_OBEX_RSP_CODE_NOT_MODIFIED - `Cannot create folder/file, destination folder/file
 *  already exits`.
 *
 *  For action @ref BT_OBEX_ACTION_SET_PERM,
 *  @ref BT_OBEX_RSP_CODE_NOT_FOUND - `Source object or destination folder does not exist`.
 *  @ref BT_OBEX_RSP_CODE_FORBIDDEN - `Cannot modify the permissions of the destination
 *  object/folder, permission denied`.
 *  @ref BT_OBEX_RSP_CODE_NOT_IMPL - `Set Object Permissions Action not supported`.
 *  @ref BT_OBEX_RSP_CODE_CONFLICT - `Cannot modify the permissions, sharing violation, object or
 *  Set Object Permissions Action busy`.
 *
 *  The third parameter `buf` saves the packet data (sequence of headers) of the action response
 *  is stored in thrid parameter `buf`. All headers are packed by calling function
 *  bt_obex_add_header_*. Or, the `buf` could be NULL if there is not any data needs to be sent.
 *
 *  @note Buffer ownership is transferred to the stack in case of success, in case of an error
 *  the caller retains the ownership of the buffer.
 *
 *  @param obex OBEX object.
 *  @param rsp_code Response code.
 *  @param buf Sequence of headers to be sent out.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_action_rsp(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf);

/** @brief Add Header: number of objects (used by Connect)
 *
 *  @param buf Buffer needs to be sent.
 *  @param count Number of objects.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_count(struct net_buf *buf, uint32_t count);

/** @brief Add Header: name of the object (often a file name)
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of name.
 *  @param name Name of the object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_name(struct net_buf *buf, uint16_t len, const uint8_t *name);

/** @brief Add Header: type of object - e.g. text, html, binary, manufacturer specific
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of type.
 *  @param type Type of the object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_type(struct net_buf *buf, uint16_t len, const uint8_t *type);

/** @brief Add Header: the length of the object in bytes
 *
 *  @param buf Buffer needs to be sent.
 *  @param len The length of the object in bytes.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_len(struct net_buf *buf, uint32_t len);

/** @brief Add Header: date/time stamp – ISO 8601 version - preferred
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of data/time stamp.
 *  @param t Data/time stamp.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_time_iso_8601(struct net_buf *buf, uint16_t len, const uint8_t *t);

/** @brief Add Header: date/time stamp – 4 byte version (for compatibility only)
 *
 *  @param buf Buffer needs to be sent.
 *  @param t Data/time stamp.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_time(struct net_buf *buf, uint32_t t);

/** @brief Add Header: text description of the object
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of description.
 *  @param dec Description of the object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_description(struct net_buf *buf, uint16_t len, const uint8_t *dec);

/** @brief Add Header: name of service that operation is targeted to
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of target name.
 *  @param target Target name.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_target(struct net_buf *buf, uint16_t len, const uint8_t *target);

/** @brief Add Header: an HTTP 1.x header
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of http 1.x header.
 *  @param http Http 1.x header.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_http(struct net_buf *buf, uint16_t len, const uint8_t *http);

/** @brief Add Header: a chunk of the object body
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of body.
 *  @param body Object Body.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_body(struct net_buf *buf, uint16_t len, const uint8_t *body);

/** @brief Add Header: the final chunk of the object body.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of body.
 *  @param body Object Body.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_end_body(struct net_buf *buf, uint16_t len, const uint8_t *body);

/** @brief Add Header: identifies the OBEX application, used to tell if talking to a peer.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of who.
 *  @param who Who.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_who(struct net_buf *buf, uint16_t len, const uint8_t *who);

/** @brief Add Header: an identifier used for OBEX connection multiplexing.
 *
 *  @param buf Buffer needs to be sent.
 *  @param conn_id Connection Id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_conn_id(struct net_buf *buf, uint32_t conn_id);

/** @brief Add Header: extended application request & response information.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of app_param.
 *  @param app_param Application parameter.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_app_param(struct net_buf *buf, uint16_t len, const uint8_t *app_param);

/**
 * OBEX digest-challenge tag: Nonce
 * String of bytes representing the nonce.
 */
#define BT_OBEX_CHALLENGE_TAG_NONCE 0x00

/**
 * OBEX digest-challenge tag: Options
 * Optional Challenge Information.
 */
#define BT_OBEX_CHALLENGE_TAG_OPTIONS 0x01

/** Option BIT0: When set, the User Id must be sent in the authenticate response. */
#define BT_OBEX_CHALLENGE_TAG_OPTION_REQ_USER_ID BIT(0)
/** Option BIT1: Access mode: Read Only when set, otherwise Full access is permitted. */
#define BT_OBEX_CHALLENGE_TAG_OPTION_ACCESS_MODE BIT(1)

/**
 * OBEX digest-challenge tag: Realm
 * A displayable string indicating which userid and/or password to use. The first byte of the
 * string is the character set to use. The character set uses the same values as those defined
 * in IrLMP for the nickname.
 */
#define BT_OBEX_CHALLENGE_TAG_REALM 0x02

/** @brief Add Header: authentication digest-challenge.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of auth_challenge.
 *  @param auth Authentication challenge.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_auth_challenge(struct net_buf *buf, uint16_t len, const uint8_t *auth);

/**
 * OBEX digest-Response tag: Request-digest
 * String of bytes representing the request digest.
 */
#define BT_OBEX_RESPONSE_TAG_REQ_DIGEST 0x00

/**
 * OBEX digest-Response tag: User Id
 * User ID string of length n. Max size is 20 bytes.
 */
#define BT_OBEX_RESPONSE_TAG_USER_ID 0x01

/**
 * OBEX digest-Response tag: Nonce
 * The nonce sent in the digest challenge string.
 */
#define BT_OBEX_RESPONSE_TAG_NONCE 0x02

/** @brief Add Header: authentication digest-response.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of authentication response.
 *  @param auth Authentication response.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_auth_rsp(struct net_buf *buf, uint16_t len, const uint8_t *auth);

/** @brief Add Header: indicates the creator of an object.
 *
 *  @param buf Buffer needs to be sent.
 *  @param creator_id Creator Id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_creator_id(struct net_buf *buf, uint32_t creator_id);

/** @brief Add Header: uniquely identifies the network client (OBEX server).
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of UUID.
 *  @param uuid UUID.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_wan_uuid(struct net_buf *buf, uint16_t len, const uint8_t *uuid);

/** @brief Add Header: oBEX Object class of object.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of oject class.
 *  @param obj_class Class of object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_obj_class(struct net_buf *buf, uint16_t len, const uint8_t *obj_class);

/** @brief Add Header: parameters used in session commands/responses.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of session parameter.
 *  @param session_param Session parameter.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_session_param(struct net_buf *buf, uint16_t len,
				     const uint8_t *session_param);

/** @brief Add Header: sequence number used in each OBEX packet for reliability.
 *
 *  @param buf Buffer needs to be sent.
 *  @param session_seq_number Session sequence parameter.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_session_seq_number(struct net_buf *buf, uint32_t session_seq_number);

/** @brief Add Header: specifies the action to be performed (used in ACTION operation).
 *
 *  @param buf Buffer needs to be sent.
 *  @param action_id Action Id @ref bt_obex_action_id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_action_id(struct net_buf *buf, uint32_t action_id);

/** @brief Add Header: the destination object name (used in certain ACTION operations).
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of destination name.
 *  @param dest_name Destination name.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_dest_name(struct net_buf *buf, uint16_t len, const uint8_t *dest_name);

/** @brief Add Header: 4-byte bit mask for setting permissions.
 *
 *  @param buf Buffer needs to be sent.
 *  @param perm Permissions.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_perm(struct net_buf *buf, uint32_t perm);

/** @brief Add Header: 1-byte value to setup Single Response Mode (SRM).
 *
 *  @param buf Buffer needs to be sent.
 *  @param srm SRM.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_srm(struct net_buf *buf, uint8_t srm);

/** @brief Add Header: Single Response Mode (SRM) Parameter.
 *
 *  @param buf Buffer needs to be sent.
 *  @param srm_param SRM parameter.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_add_header_srm_param(struct net_buf *buf, uint8_t srm_param);

/** @brief OBEX Header structure */
struct bt_obex_hdr {
	/** Header ID @ref bt_obex_header_id */
	uint8_t id;
	/** The length of header value */
	uint16_t len;
	/** Header value */
	const uint8_t *data;
};

/** @brief Helper for parsing OBEX header.
 *
 *  A helper for parsing the header structure for OBEX packets. The most common scenario is to
 *  call this helper on the in the callback of OBEX server and client.
 *
 *  @warning This helper function will consume `buf` when parsing. The user should make a copy
 *  if the original data is to be used afterwards.
 *
 *  @param buf OBEX packet as given to the callback of OBEX server and client.
 *  @param func Callback function which will be called for each element that's found in the data
 *              The callback should return true to continue parsing, or false to stop parsing.
 *  @param user_data User data to be passed to the callback.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_header_parse(struct net_buf *buf,
			 bool (*func)(struct bt_obex_hdr *hdr, void *user_data), void *user_data);

/** @brief Get header value: number of objects (used by Connect)
 *
 *  @param buf Buffer needs to be sent.
 *  @param count Number of objects.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_count(struct net_buf *buf, uint32_t *count);

/** @brief Get header value: name of the object (often a file name)
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of name.
 *  @param name Name of the object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_name(struct net_buf *buf, uint16_t *len, const uint8_t **name);

/** @brief Get header value: type of object - e.g. text, html, binary, manufacturer specific
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of type.
 *  @param type Type of the object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_type(struct net_buf *buf, uint16_t *len, const uint8_t **type);

/** @brief Get header value: the length of the object in bytes
 *
 *  @param buf Buffer needs to be sent.
 *  @param len The length of the object in bytes.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_len(struct net_buf *buf, uint32_t *len);

/** @brief Get header value: date/time stamp – ISO 8601 version - preferred
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of data/time stamp.
 *  @param t Data/time stamp.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_time_iso_8601(struct net_buf *buf, uint16_t *len, const uint8_t **t);

/** @brief Get header value: date/time stamp – 4 byte version (for compatibility only)
 *
 *  @param buf Buffer needs to be sent.
 *  @param t Data/time stamp.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_time(struct net_buf *buf, uint32_t *t);

/** @brief Get header value: text description of the object
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of description.
 *  @param dec Description of the object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_description(struct net_buf *buf, uint16_t *len, const uint8_t **dec);

/** @brief Get header value: name of service that operation is targeted to
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of target name.
 *  @param target Target name.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_target(struct net_buf *buf, uint16_t *len, const uint8_t **target);

/** @brief Get header value: an HTTP 1.x header
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of http 1.x header.
 *  @param http Http 1.x header.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_http(struct net_buf *buf, uint16_t *len, const uint8_t **http);

/** @brief Get header value: a chunk of the object body.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of body.
 *  @param body Object Body.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_body(struct net_buf *buf, uint16_t *len, const uint8_t **body);

/** @brief Get header value: the final chunk of the object body.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of body.
 *  @param body Object Body.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_end_body(struct net_buf *buf, uint16_t *len, const uint8_t **body);

/** @brief Get header value: identifies the OBEX application, used to tell if talking to a peer.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of who.
 *  @param who Who.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_who(struct net_buf *buf, uint16_t *len, const uint8_t **who);

/** @brief Get header value: an identifier used for OBEX connection multiplexing.
 *
 *  @param buf Buffer needs to be sent.
 *  @param conn_id Connection Id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_conn_id(struct net_buf *buf, uint32_t *conn_id);

/** @brief Get header value: extended application request & response information.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of app_param.
 *  @param app_param Application parameter.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_app_param(struct net_buf *buf, uint16_t *len, const uint8_t **app_param);

/** @brief Get header value: authentication digest-challenge.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of auth_challenge.
 *  @param auth Authentication challenge.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_auth_challenge(struct net_buf *buf, uint16_t *len, const uint8_t **auth);

/** @brief Get header value: authentication digest-response.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of authentication response.
 *  @param auth Authentication response.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_auth_rsp(struct net_buf *buf, uint16_t *len, const uint8_t **auth);

/** @brief Get header value: indicates the creator of an object.
 *
 *  @param buf Buffer needs to be sent.
 *  @param creator_id Creator Id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_creator_id(struct net_buf *buf, uint32_t *creator_id);

/** @brief Get header value: uniquely identifies the network client (OBEX server).
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of UUID.
 *  @param uuid UUID.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_wan_uuid(struct net_buf *buf, uint16_t *len, const uint8_t **uuid);

/** @brief Get header value: oBEX Object class of object.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of oject class.
 *  @param obj_class Class of object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_obj_class(struct net_buf *buf, uint16_t *len, const uint8_t **obj_class);

/** @brief Get header value: parameters used in session commands/responses.
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of session parameter.
 *  @param session_param Session parameter.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_session_param(struct net_buf *buf, uint16_t *len,
				     const uint8_t **session_param);

/** @brief Get header value: sequence number used in each OBEX packet for reliability.
 *
 *  @param buf Buffer needs to be sent.
 *  @param session_seq_number Session sequence parameter.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_session_seq_number(struct net_buf *buf, uint32_t *session_seq_number);

/** @brief Get header value: specifies the action to be performed (used in ACTION operation).
 *
 *  @param buf Buffer needs to be sent.
 *  @param action_id Action Id @ref bt_obex_action_id.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_action_id(struct net_buf *buf, uint32_t *action_id);

/** @brief Get header value: the destination object name (used in certain ACTION operations).
 *
 *  @param buf Buffer needs to be sent.
 *  @param len Length of destination name.
 *  @param dest_name Destination name.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_dest_name(struct net_buf *buf, uint16_t *len, const uint8_t **dest_name);

/** @brief Get header value: 4-byte bit mask for setting permissions.
 *
 *  @param buf Buffer needs to be sent.
 *  @param perm Permissions.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_perm(struct net_buf *buf, uint32_t *perm);

/** @brief Get header value: 1-byte value to setup Single Response Mode (SRM).
 *
 *  @param buf Buffer needs to be sent.
 *  @param srm SRM.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_srm(struct net_buf *buf, uint8_t *srm);

/** @brief Get header value: Single Response Mode (SRM) Parameter.
 *
 *  @param buf Buffer needs to be sent.
 *  @param srm_param SRM parameter.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_obex_get_header_srm_param(struct net_buf *buf, uint8_t *srm_param);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_OBEX_H_ */
