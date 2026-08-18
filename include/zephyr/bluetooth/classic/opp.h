/** @file
 *  @brief Object Push Profile header.
 */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_OPP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_OPP_H_

/**
 * @brief Object Push Profile (OPP)
 * @defgroup bt_opp Object Push Profile (OPP)
 * @since 4.6
 * @version 0.1.0
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/classic/goep.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDP service class UUID for the Object Push service.
 *
 * Use this UUID as the search target when performing SDP discovery for OPP.
 * Defined as OBEXObjectPush (0x1105) in the Bluetooth Assigned Numbers.
 */
#define BT_OPP_SDP_UUID BT_UUID_DECLARE_16(0x1105)

/**
 * @brief SDP attribute ID for the Supported Formats List attribute.
 *
 * Mandatory for OPP servers. Contains a sequence of @ref bt_opp_format
 * values describing the object types the server can receive.
 */
#define BT_OPP_SDP_ATTR_SUPPORTED_FORMATS 0x0303

/** @brief OBEX Type header value for vCard 2.1 / vCard 3.0 objects. */
#define BT_OPP_TYPE_VCARD    "text/x-vcard"

/** @brief OBEX Type header value for vCalendar 1.0 objects. */
#define BT_OPP_TYPE_VCAL     "text/x-vcalendar"

/** @brief OBEX Type header value for iCalendar 2.0 objects. */
#define BT_OPP_TYPE_ICAL     "text/calendar"

/** @brief OBEX Type header value for vNote objects. */
#define BT_OPP_TYPE_VNOTE    "text/x-vnote"

/** @brief OBEX Type header value for vMessage objects. */
#define BT_OPP_TYPE_VMESSAGE "text/x-vmsg"

/**
 * @brief OPP supported object format codes.
 *
 * Defined in OPP Spec v1.1, Table 6.1 (Supported Formats List in SDP).
 * Used when registering a Push Server to advertise supported object types.
 */
enum bt_opp_format {
	/** vCard 2.1 format. */
	BT_OPP_FORMAT_VCARD_2_1 = 0x01,
	/** vCard 3.0 format. */
	BT_OPP_FORMAT_VCARD_3_0 = 0x02,
	/** vCalendar 1.0 format. */
	BT_OPP_FORMAT_VCAL_1_0  = 0x03,
	/** iCalendar 2.0 format. */
	BT_OPP_FORMAT_ICAL_2_0  = 0x04,
	/** vNote format. */
	BT_OPP_FORMAT_VNOTE     = 0x05,
	/** vMessage format. */
	BT_OPP_FORMAT_VMESSAGE  = 0x06,
	/** Any object type. */
	BT_OPP_FORMAT_ANY       = 0xff,
};

/**
 * @brief OPP response codes.
 *
 * Mapped from OBEX response codes. These are the values carried by the
 * `rsp_code` parameter of the OPP operation callbacks and response APIs. A
 * response code received from a remote device is not validated against this
 * list, so it may hold any value defined by @ref bt_obex_rsp_code.
 */
enum bt_opp_rsp_code {
	/** Continue - more data follows. */
	BT_OPP_RSP_CODE_CONTINUE          = BT_OBEX_RSP_CODE_CONTINUE,
	/** OK / Success (final response, same numeric value as SUCCESS). */
	BT_OPP_RSP_CODE_OK                = BT_OBEX_RSP_CODE_OK,
	/** Success. */
	BT_OPP_RSP_CODE_SUCCESS           = BT_OBEX_RSP_CODE_SUCCESS,
	/** Bad request - server could not understand request. */
	BT_OPP_RSP_CODE_BAD_REQ           = BT_OBEX_RSP_CODE_BAD_REQ,
	/** Forbidden - operation understood but refused. */
	BT_OPP_RSP_CODE_FORBIDDEN         = BT_OBEX_RSP_CODE_FORBIDDEN,
	/** Not found - requested object does not exist. */
	BT_OPP_RSP_CODE_NOT_FOUND         = BT_OBEX_RSP_CODE_NOT_FOUND,
	/** Not acceptable. */
	BT_OPP_RSP_CODE_NOT_ACCEPT        = BT_OBEX_RSP_CODE_NOT_ACCEPT,
	/** Precondition failed. */
	BT_OPP_RSP_CODE_PRECON_FAIL       = BT_OBEX_RSP_CODE_PRECON_FAIL,
	/** Requested entity too large - object size exceeds server capacity. */
	BT_OPP_RSP_CODE_ENTITY_TOO_LARGE  = BT_OBEX_RSP_CODE_ENTITY_TOO_LARGE,
	/** Unsupported media type - server does not support this object format. */
	BT_OPP_RSP_CODE_UNSUPP_MEDIA_TYPE = BT_OBEX_RSP_CODE_UNSUPP_MEDIA_TYPE,
	/** Not implemented. */
	BT_OPP_RSP_CODE_NOT_IMPL          = BT_OBEX_RSP_CODE_NOT_IMPL,
	/** Internal server error. */
	BT_OPP_RSP_CODE_INTER_ERROR       = BT_OBEX_RSP_CODE_INTER_ERROR,
	/** Service unavailable. */
	BT_OPP_RSP_CODE_UNAVAIL           = BT_OBEX_RSP_CODE_UNAVAIL,
};

/** @brief Forward declaration of OPP Push Client structure. */
struct bt_opp_client;

/**
 * @brief OPP Push Client callback operations structure.
 *
 * The @ref bt_opp_client_cb.connect() and @ref bt_opp_client_cb.disconnect()
 * callbacks are mandatory and are validated by
 * @ref bt_opp_client_transport_connect. The response callback of an operation
 * is mandatory for that operation only: @ref bt_opp_client_push,
 * @ref bt_opp_client_pull and @ref bt_opp_client_abort reject the request when
 * the matching callback is NULL, so a client that never issues an operation
 * does not have to handle its response. The transport callbacks are optional.
 *
 * The structure must remain valid and constant for the entire lifetime of the
 * Push Client instance.
 */
struct bt_opp_client_cb {
	/**
	 * @brief Transport connected callback.
	 *
	 * Called when the transport connection to the Push Server is
	 * established. After this, call @ref bt_opp_client_connect to establish
	 * the OBEX connection.
	 *
	 * @param conn    ACL connection object.
	 * @param client  OPP Push Client object.
	 */
	void (*transport_connected)(struct bt_conn *conn, struct bt_opp_client *client);

	/**
	 * @brief Transport disconnected callback.
	 *
	 * Called when the transport connection to the Push Server is closed.
	 *
	 * @param client  OPP Push Client object.
	 */
	void (*transport_disconnected)(struct bt_opp_client *client);

	/**
	 * @brief OBEX connect response callback.
	 *
	 * Called when the OBEX CONNECT response is received from the Push
	 * Server. This callback is mandatory.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code from server, @ref bt_opp_rsp_code.
	 * @param version   OBEX version reported by the server.
	 * @param mopl      Maximum OBEX packet length of the server.
	 * @param buf       Optional response headers buffer, or NULL.
	 */
	void (*connect)(struct bt_opp_client *client, uint8_t rsp_code, uint8_t version,
			uint16_t mopl, struct net_buf *buf);

	/**
	 * @brief OBEX disconnect response callback.
	 *
	 * Called when the OBEX DISCONNECT response is received. This callback is
	 * mandatory.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
	 * @param buf       Optional response headers buffer, or NULL.
	 */
	void (*disconnect)(struct bt_opp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Object push (PUT) response callback.
	 *
	 * Called when a PUT response is received from the Push Server.
	 * For multi-packet transfers this is called for each response packet.
	 * @ref BT_OPP_RSP_CODE_CONTINUE indicates the server is ready for the
	 * next body chunk; @ref BT_OPP_RSP_CODE_SUCCESS indicates the transfer
	 * is complete.
	 *
	 * Required by @ref bt_opp_client_push.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
	 * @param buf       Optional response headers buffer, or NULL.
	 */
	void (*push)(struct bt_opp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Business card pull (GET) response callback.
	 *
	 * Called when a GET response is received while pulling the server's
	 * Default Get Object. Per OPP Spec section 5.6, the server returns
	 * @ref BT_OPP_RSP_CODE_NOT_FOUND when no default object is available.
	 *
	 * Required by @ref bt_opp_client_pull.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
	 * @param buf       Response headers buffer containing body data, or NULL.
	 */
	void (*pull)(struct bt_opp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/**
	 * @brief Abort response callback.
	 *
	 * Called when the OBEX ABORT response is received.
	 *
	 * Required by @ref bt_opp_client_abort.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
	 * @param buf       Optional response headers buffer, or NULL.
	 */
	void (*abort)(struct bt_opp_client *client, uint8_t rsp_code, struct net_buf *buf);
};

/** @brief OPP Push Client structure. */
struct bt_opp_client {
	/** @brief Callback operations structure pointer. */
	const struct bt_opp_client_cb *cb;

	/** @internal Underlying GOEP v1.1 transport instance. */
	struct bt_goep_transport_v1 _transport;

	/** @internal GOEP instance. */
	struct bt_goep _goep;

	/** @internal Transport layer state (atomic). */
	atomic_t _transport_state;

	/** @internal OBEX client handle. */
	struct bt_obex_client _client;

	/** @internal OPP OBEX connection state (atomic). */
	atomic_t _state;
};

/**
 * @brief Connect the Push Client to a Push Server transport.
 *
 * Establishes the GOEP v1.1 (RFCOMM) transport connection using the channel
 * obtained from SDP discovery (Protocol Descriptor List). On success the
 * @ref bt_opp_client_cb.transport_connected() callback is called; after that,
 * call @ref bt_opp_client_connect to establish the OBEX connection.
 *
 * @param conn     ACL connection object.
 * @param client   Push Client instance to connect.
 * @param cb       Callback operations structure, @ref bt_opp_client_cb. Must not
 *                 be NULL and must provide @ref bt_opp_client_cb.connect() and
 *                 @ref bt_opp_client_cb.disconnect().
 * @param channel  RFCOMM channel from SDP discovery.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, or a mandatory callback is missing.
 * @retval -EALREADY Transport has been connected.
 * @retval -errno Error code of the underlying transport on failure.
 */
int bt_opp_client_transport_connect(struct bt_conn *conn, struct bt_opp_client *client,
				    const struct bt_opp_client_cb *cb, uint8_t channel);

/**
 * @brief Disconnect the Push Client transport connection.
 *
 * Closes the transport connection. The
 * @ref bt_opp_client_cb.transport_disconnected() callback is called when done.
 *
 * @param client  Push Client instance to disconnect.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTCONN Transport is not connected.
 * @retval -errno Error code of the underlying transport on failure.
 */
int bt_opp_client_transport_disconnect(struct bt_opp_client *client);

/**
 * @brief Allocate a PDU buffer with headroom reserved for OPP Push Client.
 *
 * The headroom accounts for all lower-layer headers (OBEX, RFCOMM, L2CAP and
 * ACL), so the application can append OBEX headers with the
 * `bt_obex_add_header_*` helpers right away.
 *
 * If @p pool is NULL the buffer is taken from the pool owned by the OPP module.
 * That pool holds two buffers per ACL connection (@kconfig{CONFIG_BT_MAX_CONN}),
 * one for a request and one for a response, each sized to carry a full OBEX
 * packet over RFCOMM (@kconfig{CONFIG_BT_GOEP_RFCOMM_MTU}). An application that
 * needs to keep more buffers in flight has to pass its own pool.
 *
 * @param client  Push Client object.
 * @param pool    Buffer pool to allocate from, or NULL for the pool owned by
 *                the OPP module.
 *
 * @return Pointer to the allocated buffer with reserved headroom, or NULL if
 *         @p client is NULL or no buffer is available.
 */
struct net_buf *bt_opp_client_create_pdu(struct bt_opp_client *client, struct net_buf_pool *pool);

/**
 * @brief Establish an OBEX connection with the Push Server.
 *
 * Sends an OBEX CONNECT request. Per OPP Spec section 5.4, the Target header
 * (@ref BT_OBEX_HEADER_ID_TARGET) shall not be included. The transport must
 * already be connected via @ref bt_opp_client_transport_connect. The
 * @ref bt_opp_client_cb.connect() callback is invoked with the server response.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param client  Push Client instance (transport must be connected).
 * @param mopl    Maximum OBEX packet length proposed by this client.
 * @param buf     Optional additional OBEX headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTCONN Transport is not connected.
 * @retval -EALREADY OBEX connection has been established, or a CONNECT request
 *         is outstanding.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_client_connect(struct bt_opp_client *client, uint16_t mopl, struct net_buf *buf);

/**
 * @brief Terminate the OBEX connection.
 *
 * Sends an OBEX DISCONNECT request. The @ref bt_opp_client_cb.disconnect()
 * callback is invoked when the response is received.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param client  Push Client instance.
 * @param buf     Optional OBEX headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTCONN OBEX connection is not established.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_client_disconnect(struct bt_opp_client *client, struct net_buf *buf);

/**
 * @brief Push an object to the Push Server (OBEX PUT).
 *
 * Sends an OBEX PUT request to deliver an object to the server's inbox.
 * Per OPP Spec sections 4.2.2 and 5.5:
 * - The Name header (@ref BT_OBEX_HEADER_ID_NAME) shall be included.
 * - The Length header (@ref BT_OBEX_HEADER_ID_LEN) shall be included.
 * - The Type header (@ref BT_OBEX_HEADER_ID_TYPE) should be included, holding
 *   one of the object types listed above, such as @ref BT_OPP_TYPE_VCARD.
 * - The Body (@ref BT_OBEX_HEADER_ID_BODY) and End-of-Body
 *   (@ref BT_OBEX_HEADER_ID_END_BODY) headers carry the object data.
 *
 * For multi-packet transfers, set @p final to false and use
 * @ref BT_OBEX_HEADER_ID_BODY for every intermediate packet. Set @p final to
 * true and use @ref BT_OBEX_HEADER_ID_END_BODY for the last (or only) packet.
 * The caller shall not mix the two headers within the same packet, and shall use
 * End-of-Body exclusively in the packet where @p final is true.
 *
 * The @ref bt_opp_client_cb.push() callback is called after each PUT response;
 * @ref BT_OPP_RSP_CODE_CONTINUE means the server is ready for the next chunk.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param client  Push Client instance.
 * @param final   True if this is the last (or only) PUT packet (use
 *                @ref BT_OBEX_HEADER_ID_END_BODY in @p buf). False for
 *                intermediate packets (use @ref BT_OBEX_HEADER_ID_BODY).
 * @param buf     Buffer carrying the OBEX headers of the request.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTSUP @ref bt_opp_client_cb.push() is not provided.
 * @retval -ENOTCONN OBEX connection is not established.
 * @retval -ENODATA @p final is true but @p buf carries no End-of-Body header.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_client_push(struct bt_opp_client *client, bool final, struct net_buf *buf);

/**
 * @brief Pull the server's default business card (OBEX GET).
 *
 * Sends an OBEX GET request for the server's Default Get Object (owner
 * business card). Per OPP Spec sections 4.3.2 and 5.6, the first request
 * of a pull operation:
 * - The Type header (@ref BT_OBEX_HEADER_ID_TYPE) shall be set to
 *   @ref BT_OPP_TYPE_VCARD (case insensitive).
 * - The Name header (@ref BT_OBEX_HEADER_ID_NAME) shall be absent or empty.
 *
 * @note For the Default Get Object, OPP Spec Table 5.3 lists the Name header as
 *       Mandatory (M) while section 5.6 states that it is not used. The
 *       recommended practice for interoperability is to include an empty Name
 *       header (a two-byte UTF-16BE null terminator: 0x00 0x00) in @p buf.
 *
 * The @ref bt_opp_client_cb.pull() callback is called with each response.
 * @ref BT_OPP_RSP_CODE_NOT_FOUND is returned by the server when no default
 * object exists. If the response is @ref BT_OPP_RSP_CODE_CONTINUE, call this
 * function again to fetch the next chunk; per standard OBEX practice this
 * continuation request carries no headers at all, so @p buf may be empty.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param client  Push Client instance.
 * @param buf     Buffer carrying OBEX headers for the first request (Type
 *                header required; empty Name header recommended for
 *                interoperability), or an empty buffer for a continuation
 *                request.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTSUP @ref bt_opp_client_cb.pull() is not provided.
 * @retval -ENOTCONN OBEX connection is not established.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_client_pull(struct bt_opp_client *client, struct net_buf *buf);

/**
 * @brief Abort the current ongoing operation.
 *
 * Sends an OBEX ABORT request to cancel a push or pull operation that is in
 * progress. The @ref bt_opp_client_cb.abort() callback is invoked when the
 * server responds.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param client  Push Client instance.
 * @param buf     Optional OBEX headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTSUP @ref bt_opp_client_cb.abort() is not provided.
 * @retval -ENOTCONN OBEX connection is not established.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_client_abort(struct bt_opp_client *client, struct net_buf *buf);

/** @brief Forward declaration of OPP Push Server structure. */
struct bt_opp_server;

/**
 * @brief OPP Push Server callback operations structure.
 *
 * Defines the handlers invoked when the Push Server receives requests from a
 * Push Client. Every request a client may send without being solicited needs a
 * handler, so @ref bt_opp_server_cb.connect(),
 * @ref bt_opp_server_cb.disconnect(), @ref bt_opp_server_cb.push(),
 * @ref bt_opp_server_cb.pull() and @ref bt_opp_server_cb.abort() are mandatory
 * and are validated by @ref bt_opp_server_register. The stack never answers a
 * request on behalf of the application: each of these callbacks has to send its
 * response, which is also how a server that does not implement an operation
 * reports @ref BT_OPP_RSP_CODE_NOT_IMPL. The transport callbacks are optional.
 *
 * The structure must remain valid and constant for the lifetime of the server.
 */
struct bt_opp_server_cb {
	/**
	 * @brief Transport connected callback.
	 *
	 * Called when a new transport connection from a Push Client is accepted.
	 *
	 * @param conn    ACL connection object.
	 * @param server  OPP Push Server object.
	 */
	void (*transport_connected)(struct bt_conn *conn, struct bt_opp_server *server);

	/**
	 * @brief Transport disconnected callback.
	 *
	 * Called when the transport connection from a Push Client is closed.
	 *
	 * @param server  OPP Push Server object.
	 */
	void (*transport_disconnected)(struct bt_opp_server *server);

	/**
	 * @brief OBEX connect request callback.
	 *
	 * Called when the Push Server receives an OBEX CONNECT request.
	 * Per OPP Spec section 5.4, the Target header
	 * (@ref BT_OBEX_HEADER_ID_TARGET) will not be present. The application
	 * must call @ref bt_opp_server_connect to respond. This callback is
	 * mandatory.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param server   OPP Push Server object.
	 * @param version  OBEX version requested by the client.
	 * @param mopl     Maximum OBEX packet length proposed by the client.
	 * @param buf      Optional request headers buffer, or NULL.
	 */
	void (*connect)(struct bt_opp_server *server, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/**
	 * @brief OBEX disconnect request callback.
	 *
	 * Called when the Push Server receives an OBEX DISCONNECT request.
	 * The application must call @ref bt_opp_server_disconnect to respond.
	 * This callback is mandatory.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param server  OPP Push Server object.
	 * @param buf     Optional request headers buffer, or NULL.
	 */
	void (*disconnect)(struct bt_opp_server *server, struct net_buf *buf);

	/**
	 * @brief Object push (PUT) request callback.
	 *
	 * Called when a PUT request is received from the Push Client. @p buf
	 * contains OBEX headers such as Name (@ref BT_OBEX_HEADER_ID_NAME),
	 * Type (@ref BT_OBEX_HEADER_ID_TYPE), Length
	 * (@ref BT_OBEX_HEADER_ID_LEN) and Body (@ref BT_OBEX_HEADER_ID_BODY).
	 * For multi-packet objects this callback is invoked for each PUT packet;
	 * the End-of-Body header (@ref BT_OBEX_HEADER_ID_END_BODY) signals the
	 * last packet.
	 *
	 * The application must call @ref bt_opp_server_push to respond. If the
	 * object format is not supported, respond with
	 * @ref BT_OPP_RSP_CODE_UNSUPP_MEDIA_TYPE. If the object is too large,
	 * respond with @ref BT_OPP_RSP_CODE_ENTITY_TOO_LARGE. This callback is
	 * mandatory.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param server  OPP Push Server object.
	 * @param final   True if this is the last PUT packet (End-of-Body present).
	 * @param buf     Request headers buffer.
	 */
	void (*push)(struct bt_opp_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Business card pull (GET) request callback.
	 *
	 * Called when a GET request for the Default Get Object is received.
	 * Per OPP Spec section 4.3.1, the Type header
	 * (@ref BT_OBEX_HEADER_ID_TYPE) will be set to @ref BT_OPP_TYPE_VCARD
	 * and the Name header (@ref BT_OBEX_HEADER_ID_NAME) will be absent or
	 * empty. A GET request carrying a non-empty Name header does not address
	 * the Default Get Object; the stack answers it with
	 * @ref BT_OPP_RSP_CODE_FORBIDDEN and does not invoke this callback.
	 *
	 * The application must call @ref bt_opp_server_pull to respond. If no
	 * default object exists, respond with @ref BT_OPP_RSP_CODE_NOT_FOUND; a
	 * server that does not implement Business Card Pull at all responds with
	 * @ref BT_OPP_RSP_CODE_NOT_IMPL. This callback is mandatory.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param server  OPP Push Server object.
	 * @param buf     Request headers buffer (Type header present).
	 */
	void (*pull)(struct bt_opp_server *server, struct net_buf *buf);

	/**
	 * @brief Abort request callback.
	 *
	 * Called when an OBEX ABORT request is received from the Push Client.
	 * The application must call @ref bt_opp_server_abort to respond, and has
	 * to drop the object it was receiving, because the transfer the abort
	 * cancels is tracked by the application and not by the stack. This
	 * callback is mandatory.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param server  OPP Push Server object.
	 * @param buf     Optional request headers buffer, or NULL.
	 */
	void (*abort)(struct bt_opp_server *server, struct net_buf *buf);
};

/**
 * @brief OPP Push Server transport registration structure.
 *
 * Used to register a Push Server that accepts incoming GOEP v1.1 (RFCOMM)
 * transport connections.
 */
struct bt_opp_server_transport {
	/** @brief Underlying GOEP RFCOMM server. */
	struct bt_goep_transport_rfcomm_server server;

	/**
	 * @brief Accept connection callback.
	 *
	 * Called when a new transport connection arrives. The application must
	 * allocate and initialize a @ref bt_opp_server instance and pass it
	 * back via @p opp_server. Call @ref bt_opp_server_register on the
	 * instance before returning.
	 *
	 * @param conn             ACL connection object.
	 * @param transport_server This transport registration structure.
	 * @param opp_server       Output pointer for the allocated server instance.
	 *
	 * @retval 0 on success.
	 * @retval -ENOMEM No server instance is available.
	 * @retval -EACCES The application refuses the connection.
	 * @retval -errno Any other negative error code rejects the connection.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_opp_server_transport *transport_server,
		      struct bt_opp_server **opp_server);
};

/** @brief OPP Push Server structure. */
struct bt_opp_server {
	/** @brief Callback operations structure pointer. */
	const struct bt_opp_server_cb *cb;

	/** @internal Underlying GOEP v1.1 transport instance. */
	struct bt_goep_transport_v1 _transport;

	/** @internal GOEP instance. */
	struct bt_goep _goep;

	/** @internal Transport layer state (atomic). */
	atomic_t _transport_state;

	/** @internal OBEX server handle. */
	struct bt_obex_server _server;

	/** @internal OPP OBEX connection state (atomic). */
	atomic_t _state;
};

/**
 * @brief Allocate a PDU buffer with headroom reserved for OPP Push Server.
 *
 * The headroom accounts for all lower-layer headers (OBEX, RFCOMM, L2CAP and
 * ACL), so the application can append OBEX headers with the
 * `bt_obex_add_header_*` helpers right away.
 *
 * If @p pool is NULL the buffer is taken from the pool owned by the OPP module.
 * That pool holds two buffers per ACL connection (@kconfig{CONFIG_BT_MAX_CONN}),
 * one for a request and one for a response, each sized to carry a full OBEX
 * packet over RFCOMM (@kconfig{CONFIG_BT_GOEP_RFCOMM_MTU}). An application that
 * needs to keep more buffers in flight has to pass its own pool.
 *
 * @param server  Push Server object.
 * @param pool    Buffer pool to allocate from, or NULL for the pool owned by
 *                the OPP module.
 *
 * @return Pointer to the allocated buffer with reserved headroom, or NULL if
 *         @p server is NULL or no buffer is available.
 */
struct net_buf *bt_opp_server_create_pdu(struct bt_opp_server *server, struct net_buf_pool *pool);

/**
 * @brief Register an OPP Push Server transport listener.
 *
 * Registers the RFCOMM server to listen for incoming OPP connections. The
 * allocated RFCOMM channel must be advertised via SDP (Protocol Descriptor
 * List). If @ref bt_rfcomm_server.channel of
 * @ref bt_goep_transport_rfcomm_server.rfcomm is 0, a channel is auto-assigned
 * by RFCOMM.
 *
 * @note Per OPP Spec v1.1 section 3.1, a device that supports the Object
 *       Exchange feature must also set the Object Transfer service class bit in
 *       the Class of Device (CoD) before making the OPP server discoverable.
 *       Use @ref bt_br_set_discoverable after configuring the CoD via the HCI
 *       Write Class of Device command or the @kconfig{CONFIG_BT_COD} Kconfig
 *       option.
 *
 * @param server  Transport registration structure, @ref bt_opp_server_transport.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, or @ref bt_opp_server_transport.accept() is
 *         missing.
 * @retval -errno Error code of the underlying transport on failure.
 */
int bt_opp_server_transport_register(struct bt_opp_server_transport *server);

/**
 * @brief Register a Push Server instance with the OPP subsystem.
 *
 * Initializes the server object and associates the callback structure.
 * This must be called inside the @ref bt_opp_server_transport.accept() callback
 * before returning the instance.
 *
 * @param server  Push Server instance to register, @ref bt_opp_server.
 * @param cb      Callback operations structure, @ref bt_opp_server_cb. Must not
 *                be NULL and must provide every mandatory callback.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, or a mandatory callback is missing.
 * @retval -EINPROGRESS The instance is in use by an OBEX connection.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_register(struct bt_opp_server *server, const struct bt_opp_server_cb *cb);

/**
 * @brief Send an OBEX CONNECT response from the Push Server.
 *
 * Responds to a connect request received via @ref bt_opp_server_cb.connect().
 * The OBEX connection is established only if @p rsp_code is
 * @ref BT_OPP_RSP_CODE_SUCCESS.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param mopl      Maximum OBEX packet length accepted by this server.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_connect(struct bt_opp_server *server, uint16_t mopl, uint8_t rsp_code,
			  struct net_buf *buf);

/**
 * @brief Send an OBEX DISCONNECT response from the Push Server.
 *
 * Responds to a disconnect request received via
 * @ref bt_opp_server_cb.disconnect(). The OBEX connection is torn down only if
 * @p rsp_code is @ref BT_OPP_RSP_CODE_SUCCESS; any other response code leaves it
 * established.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_disconnect(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send an OBEX PUT response from the Push Server.
 *
 * Responds to a push request received via @ref bt_opp_server_cb.push().
 * Send @ref BT_OPP_RSP_CODE_CONTINUE to request the next body chunk, or
 * @ref BT_OPP_RSP_CODE_SUCCESS when the full object has been received.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_push(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send an OBEX GET response from the Push Server (business card pull).
 *
 * Responds to a business card pull request received via
 * @ref bt_opp_server_cb.pull(). Send @ref BT_OPP_RSP_CODE_CONTINUE for each
 * intermediate body chunk and @ref BT_OPP_RSP_CODE_SUCCESS with the final
 * End-of-Body chunk. Respond with @ref BT_OPP_RSP_CODE_NOT_FOUND if no default
 * object is available.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Response headers buffer containing Body or End-of-Body, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_pull(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send an OBEX ABORT response from the Push Server.
 *
 * Responds to an abort request received via @ref bt_opp_server_cb.abort().
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code (typically @ref BT_OPP_RSP_CODE_SUCCESS),
 *                  @ref bt_opp_rsp_code.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_abort(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_OPP_H_ */
