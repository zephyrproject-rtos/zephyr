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
 *
 * This implementation targets Object Push Profile v1.1, which builds on
 * GOEP v1.1 (OBEX over RFCOMM) and IrOBEX v1.2. GOEP v2.0 adds OBEX over L2CAP,
 * which IrDA Interoperability v2.0 defines, and the IrOBEX v1.5 features Single
 * Response Mode and reliable sessions. The OPP ICS only allows those from
 * OPP v1.2 on, so they are not used: the APIs below reject a buffer that
 * carries a header of such a feature, as they do for every header OPP Spec
 * Table 5.2 and Table 5.3 exclude.
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
#define BT_OPP_SDP_UUID BT_UUID_DECLARE_16(BT_SDP_OBEX_OBJPUSH_SVCLASS)

/**
 * @brief SDP attribute ID for the Supported Formats List attribute.
 *
 * Mandatory for OPP servers. Contains a sequence of @ref bt_opp_format
 * values describing the object types the server can receive.
 */
#define BT_OPP_SDP_ATTR_SUPPORTED_FORMATS BT_SDP_ATTR_SUPPORTED_FORMATS_LIST

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
 * response code received from a remote device is passed on unchecked, so it
 * may also hold a value that @ref bt_obex_rsp_code does not define.
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
 * Every callback is mandatory except @ref bt_opp_client_cb.pull(), and
 * @ref bt_opp_client_transport_connect validates that they are all set:
 *
 * - The two transport callbacks are the only report the application gets of the
 *   transport coming up or going away, and the API exposes no way to query that
 *   state.
 * - Per the OPP Spec the Push Client shall support the Connect, Disconnect, Put
 *   and Abort operations.
 *
 * The Get operation is optional, so pull() is only needed by a client that
 * pulls the server's business card; @ref bt_opp_client_pull rejects the request
 * when it is NULL.
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
	 * Called when the transport connection to the Push Server is closed,
	 * either by the peer or on completion of
	 * @ref bt_opp_client_transport_disconnect. It is also called, without a
	 * preceding @ref bt_opp_client_cb.transport_connected(), when a connection
	 * attempt fails after @ref bt_opp_client_transport_connect has returned 0.
	 *
	 * @param client  OPP Push Client object.
	 */
	void (*transport_disconnected)(struct bt_opp_client *client);

	/**
	 * @brief OBEX connect response callback.
	 *
	 * Called when the OBEX CONNECT response is received from the Push
	 * Server.
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
	 * Called when the OBEX DISCONNECT response is received. Per IrOBEX 1.2
	 * section 3.3.2 a disconnect may not be refused. With @p rsp_code
	 * @ref BT_OPP_RSP_CODE_SUCCESS the OBEX connection is gone once this is
	 * called. Any other @p rsp_code means the server refused it: the OBEX
	 * connection is still established while this callback runs, and the
	 * transport is closed right after it returns, which the
	 * @ref bt_opp_client_cb.transport_disconnected() callback reports.
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
	 * This callback is optional; only a client that pulls the server's
	 * business card needs it.
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
	 * Called when the server answers the OBEX ABORT request with
	 * @ref BT_OPP_RSP_CODE_SUCCESS, which per IrOBEX 1.2 section 3.3.5
	 * tells the client that the server is resynchronized with it.
	 *
	 * Any other response code leaves the client without that guarantee, so
	 * the stack closes the transport without calling this callback. The
	 * application only gets @ref bt_opp_client_cb.transport_disconnected()
	 * and never sees the response code.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, always @ref BT_OPP_RSP_CODE_SUCCESS.
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

	/** @internal Push operation state (atomic). */
	atomic_t _push_state;

	/** @internal Pull operation state (atomic). */
	atomic_t _pull_state;
};

/**
 * @brief Connect the Push Client to a Push Server transport.
 *
 * Establishes the GOEP v1.1 (RFCOMM) transport connection using the channel
 * obtained from SDP discovery (Protocol Descriptor List). Once the transport
 * is up the @ref bt_opp_client_cb.transport_connected() callback is called;
 * after that, call @ref bt_opp_client_connect to establish the OBEX
 * connection. If this function returns 0 but the transport cannot be
 * established, @ref bt_opp_client_cb.transport_disconnected() is called
 * instead.
 *
 * @param conn     ACL connection object.
 * @param client   Push Client instance to connect.
 * @param cb       Callback operations structure, @ref bt_opp_client_cb. Must not
 *                 be NULL and must provide every mandatory callback.
 * @param channel  RFCOMM channel from SDP discovery.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, or a mandatory callback is missing.
 * @retval -EALREADY Transport is connected, or a transport connection or
 *         disconnection is in progress.
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
 *         @p client is NULL or has not been passed to
 *         @ref bt_opp_client_transport_connect yet. When the pool is empty
 *         the call blocks until a buffer is freed, except when called from any work queue
 *         (including the system work queue), and returns NULL instead.
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
 * @retval -EINVAL Invalid parameter, or @p buf carries a header the profile
 *         excludes, such as Target.
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
 * @retval -EINVAL Invalid parameter, or @p buf carries a header the profile
 *         excludes.
 * @retval -ENOTCONN OBEX connection is not established.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_client_disconnect(struct bt_opp_client *client, struct net_buf *buf);

/**
 * @brief Push an object to the Push Server (OBEX PUT).
 *
 * Sends an OBEX PUT request to deliver an object to the server's inbox. OPP Spec
 * Table 5.2 gives the headers the Object Push feature uses, and the first
 * request of a push operation is where they describe the object:
 * - The Name header (@ref BT_OBEX_HEADER_ID_NAME) is mandatory (M).
 * - The Length header (@ref BT_OBEX_HEADER_ID_LEN) is mandatory (M).
 * - The Type header (@ref BT_OBEX_HEADER_ID_TYPE) is optional (O), but OPP Spec
 *   section 5.5 highly recommends it: it is what tells the server the object
 *   format, for example @ref BT_OPP_TYPE_VCARD, and a server that does not
 *   support a format has to be able to refuse a push of it (OPP Spec Table 4.1,
 *   first footnote). A push without it is dispatched on the Name header alone,
 *   so include it whenever the format is known.
 *
 * GOEP section 5.5.1, Table 5.8, which OPP Spec section 5.5 refers to, puts the
 * Name header in the first request of the operation. This API requires Length
 * there as well, following the IrOBEX 1.2 section 3.3.3.1 recommendation that
 * the headers describing the object precede its Body. It does not enforce
 * Type; intermediate requests repeat none of them.
 *
 * The object data travels in the Body (@ref BT_OBEX_HEADER_ID_BODY) header,
 * except for the last chunk, which uses End-of-Body
 * (@ref BT_OBEX_HEADER_ID_END_BODY). Per IrOBEX 1.2 section 2.2.9 End-of-Body
 * has to be the final header associated with the object, so a request may carry
 * Body followed by End-of-Body but nothing of the object after it. This API only
 * requires that the request with @p final set carries End-of-Body; the order of
 * the headers inside @p buf is the caller's responsibility.
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
 * @retval -EINVAL Invalid parameter, or @p buf carries a header the profile
 *         excludes.
 * @retval -ENOTCONN OBEX connection is not established.
 * @retval -ENODATA A mandatory OBEX header is missing from @p buf: Name or
 *         Length in the first request of a push operation, End-of-Body when
 *         @p final is true.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_client_push(struct bt_opp_client *client, bool final, struct net_buf *buf);

/**
 * @brief Pull the server's default business card (OBEX GET).
 *
 * Sends an OBEX GET request for the server's Default Get Object (owner
 * business card). Per OPP Spec sections 4.3.2 and 5.6 the first request of a
 * pull operation identifies that object:
 * - The Type header (@ref BT_OBEX_HEADER_ID_TYPE) shall be set to
 *   @ref BT_OPP_TYPE_VCARD (case insensitive).
 * - The Name header (@ref BT_OBEX_HEADER_ID_NAME) shall be absent or empty.
 *
 * @note OPP Spec Tables 4.3 and 4.4 state that the Name header must not be
 *       used, and IrOBEX 1.2 section 4.3 does not allow it on the inbox
 *       connection either. IrOBEX 1.2 section 8.4 describes the request with
 *       an empty Name header instead and treats that the same as no Name
 *       header, and OPP Spec section 5.6 only refuses a non-empty one, so a
 *       conforming Push Server treats both forms alike. The empty form is
 *       added with `bt_obex_add_header_name(buf, 0, NULL)`, which per
 *       IrOBEX 1.2 section 2.2.2 encodes a Name header with no value. The
 *       Name header is mandatory to support in OPP Spec Table 5.3 like Length
 *       and Body, which this request does not carry either.
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
 * @param buf     Buffer carrying the OBEX headers of the first request, or an
 *                empty buffer for a continuation request.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, @p buf carries a header the profile
 *         excludes, or @p buf does not address the Default Get Object: it names
 *         an object or asks for another format.
 * @retval -ENODATA The Type header is missing from the first request of a pull
 *         operation.
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
 * server answers with @ref BT_OPP_RSP_CODE_SUCCESS. On any other answer the
 * stack closes the transport, which only
 * @ref bt_opp_client_cb.transport_disconnected() reports.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param client  Push Client instance.
 * @param buf     Optional OBEX headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, @p buf carries a header the profile
 *         excludes, or no operation is in progress.
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
 * Push Client. Every callback is mandatory except
 * @ref bt_opp_server_cb.transport_connected(), and @ref bt_opp_server_register
 * validates that they are all set:
 *
 * - Per OPP Spec Table 5.1 the Push Server shall support the Connect,
 *   Disconnect, Put, Get and Abort operations.
 * - @ref bt_opp_server_cb.transport_disconnected() is the only signal that the
 *   @ref bt_opp_server instance handed out by
 *   @ref bt_opp_server_transport.accept() is free again.
 *
 * @ref bt_opp_server_cb.transport_connected() is optional because accept() has
 * already reported the incoming connection.
 *
 * The stack itself answers a request that is invalid on its own terms, such as
 * a push that does not name the object it carries or a pull that addresses
 * something other than the Default Get Object. The corresponding callback is
 * not invoked for those; see each callback for the response code the case gets.
 * Every request that does reach a callback has to be answered by the
 * application.
 *
 * The structure must remain valid and constant for the lifetime of the server.
 */
struct bt_opp_server_cb {
	/**
	 * @brief Transport connected callback.
	 *
	 * Called when a new transport connection from a Push Client is accepted.
	 * This callback is optional; @ref bt_opp_server_transport.accept() has
	 * already reported the same connection.
	 *
	 * @param conn    ACL connection object.
	 * @param server  OPP Push Server object.
	 */
	void (*transport_connected)(struct bt_conn *conn, struct bt_opp_server *server);

	/**
	 * @brief Transport disconnected callback.
	 *
	 * Called when the transport connection from a Push Client is closed.
	 * The @ref bt_opp_server instance is no longer in use once this returns,
	 * so this is where the application releases it.
	 *
	 * @param server  OPP Push Server object.
	 */
	void (*transport_disconnected)(struct bt_opp_server *server);

	/**
	 * @brief OBEX connect request callback.
	 *
	 * Called when the Push Server receives an OBEX CONNECT request. The
	 * application must call @ref bt_opp_server_connect to respond.
	 *
	 * Per OPP Spec section 5.4 the Target header
	 * (@ref BT_OBEX_HEADER_ID_TARGET) is not used, so a request that carries
	 * one asks for a service this server does not provide. IrOBEX 1.2
	 * section 4.6 encourages the server to accept such a request anyway and
	 * to leave the decision to continue or abort with the client, so it is
	 * reported here like any other. Accepting it is not possible though: the
	 * OBEX layer requires a success response to a directed connect to carry
	 * the Who and Connection ID headers, both of which OPP Spec Table 5.2
	 * excludes, so answer it with an error response.
	 *
	 * The stack answers a connect request that carries a Connection ID
	 * header with @ref BT_OPP_RSP_CODE_FORBIDDEN and does not invoke this
	 * callback: per IrOBEX 1.2 section 2.2.11 the header does not make sense
	 * in a connect request and is forbidden.
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
	 *
	 * Per IrOBEX 1.2 section 3.3.2 a disconnect may not be refused, so the
	 * application answers with @ref BT_OPP_RSP_CODE_SUCCESS. The only other
	 * code that section allows is @ref BT_OPP_RSP_CODE_UNAVAIL, for a request
	 * the application finds to hold invalid information. The stack itself
	 * rejects a request that carries a Connection ID header, which OPP never
	 * assigns, and does not invoke this callback.
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
	 * @p final is set in the last one (IrOBEX 1.2 section 3.1). The
	 * End-of-Body header (@ref BT_OBEX_HEADER_ID_END_BODY) carries the last
	 * chunk of the object body (IrOBEX 1.2 section 2.2.9); the stack does not
	 * check that it arrives in the packet with @p final set.
	 *
	 * The stack checks the request before invoking this callback and answers
	 * it on its own when it is not a valid push, without invoking the
	 * callback:
	 *
	 * - A push operation has to name its object. Per IrOBEX 1.2 section
	 *   3.3.3.1 the Name header only has to precede the object body, so it
	 *   may arrive in any request of the operation up to the one that starts
	 *   the body. A request that carries body data before the name is
	 *   answered with @ref BT_OPP_RSP_CODE_BAD_REQ, as is a Name header that
	 *   is empty, not valid UTF-16BE or holds a path character (':', '/' or
	 *   '\'), which IrOBEX 1.2 section 4.3 does not accept for an inbox
	 *   object. The Length header is not enforced: section 2.2.4 makes it
	 *   optional for a sender that does not know the size in advance.
	 * - A push operation that carries no object body at all is a delete
	 *   request per IrOBEX 1.2 section 3.3.3.6, and OPP has no delete
	 *   feature (OPP Spec Table 4.1), so the last request of such an
	 *   operation is answered with @ref BT_OPP_RSP_CODE_FORBIDDEN.
	 * - A request carrying a Connection ID header, which OPP never assigns,
	 *   is rejected.
	 * - If an invalid request is received after a multi-packet push has
	 *   started, the stack closes the transport after rejecting it. The
	 *   @ref bt_opp_server_cb.transport_disconnected callback then signals
	 *   the application to discard the partial object.
	 *
	 * The application must call @ref bt_opp_server_push to respond. If the
	 * object format is not supported, respond with
	 * @ref BT_OPP_RSP_CODE_UNSUPP_MEDIA_TYPE. If the object is too large,
	 * respond with @ref BT_OPP_RSP_CODE_ENTITY_TOO_LARGE.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param server  OPP Push Server object.
	 * @param final   Final bit of the received PUT opcode: true in the last
	 *                request of the operation.
	 * @param buf     Request headers buffer.
	 */
	void (*push)(struct bt_opp_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Business card pull (GET) request callback.
	 *
	 * Called when a GET request for the Default Get Object is received.
	 * Per OPP Spec sections 4.3.2 and 5.6, the Type header
	 * (@ref BT_OBEX_HEADER_ID_TYPE) will be set to @ref BT_OPP_TYPE_VCARD
	 * and the Name header (@ref BT_OBEX_HEADER_ID_NAME) will be absent or
	 * empty. The stack checks both and does not invoke this callback for a
	 * request that addresses something else: a non-empty Name header is
	 * answered with @ref BT_OPP_RSP_CODE_FORBIDDEN and another object format
	 * with @ref BT_OPP_RSP_CODE_NOT_FOUND, which is what IrOBEX 1.2 section
	 * 8.4 prescribes for a default object the server does not have. A
	 * request carrying a Connection ID header, which OPP never assigns, is
	 * rejected as well.
	 *
	 * An absent Type header is accepted and reported here: a continuation
	 * request of a pull operation carries no headers at all, and per IrOBEX
	 * 1.2 section 3.3.4 a server holding a single object may serve it to a
	 * request that does not describe it.
	 *
	 * If an invalid request is received after a multi-packet pull has
	 * started, the stack closes the transport after rejecting it. The
	 * @ref bt_opp_server_cb.transport_disconnected callback then signals the
	 * application to reset the partial pull state.
	 *
	 * The application must call @ref bt_opp_server_pull to respond. If no
	 * default object exists, respond with @ref BT_OPP_RSP_CODE_NOT_FOUND. That
	 * includes a server that does not support Business Card Pull at all: OPP
	 * Spec section 4.3 refers it to section 5.6, and NOT FOUND is the error code
	 * the client is required to understand.
	 *
	 * @note @p buf is owned by the stack and is only valid for the duration
	 *       of this callback. The application must not unref it and has to
	 *       copy any data it needs afterwards.
	 *
	 * @param server  OPP Push Server object.
	 * @param final   Final bit of the received GET opcode. The client sets it
	 *                on the last request packet of the request phase and on
	 *                every continuation request after it (IrOBEX 1.2
	 *                sections 3.1 and 3.3.4), so a request with @p final false
	 *                is still part of the request phase and has to be answered
	 *                with @ref BT_OPP_RSP_CODE_CONTINUE and no body: the object
	 *                may only be sent once the whole request has been received.
	 * @param buf     Request headers buffer. The first request of a pull
	 *                operation carries the Type header; a continuation
	 *                request carries no headers at all.
	 */
	void (*pull)(struct bt_opp_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Abort request callback.
	 *
	 * Called when an OBEX ABORT request is received from the Push Client.
	 * The application must call @ref bt_opp_server_abort to respond. It also
	 * has to drop the object it was receiving, or rewind the Default Get Object
	 * it was sending so that the next pull starts from the beginning, because
	 * the transfer the abort cancels is tracked by the application and not by
	 * the stack.
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
	 * @note Registering the instance is mandatory and is checked by an
	 *       assertion rather than reported as an error: by the time this
	 *       callback returns, the application has already allocated the
	 *       instance, so rejecting the transport connection would leave the
	 *       application and the profile out of sync and most likely leak the
	 *       instance. Reject a connection by returning a negative error code
	 *       without allocating an instance instead.
	 *
	 * @note An instance that served an earlier connection has to be released
	 *       with @ref bt_opp_server_unregister, or zeroed, before it is
	 *       registered again, otherwise @ref bt_opp_server_register reports
	 *       -EALREADY.
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

	/** @internal Push operation state (atomic). */
	atomic_t _push_state;

	/** @internal Pull operation state (atomic). */
	atomic_t _pull_state;
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
 *         @p server is NULL or has not been handed out by
 *         @ref bt_opp_server_transport.accept() yet. When the pool is empty the
 *         call waits for a buffer to be freed, except when called from any work queue (including
 *         the system work queue), and returns NULL instead.
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
 * @note Per OPP Spec v1.1 section 3.1, a Push Server entering Object Exchange
 *       mode must ensure that the Object Transfer service class bit
 *       (@ref BT_COD_MAJOR_SVC_CLASS_OBJECT_TRANSFER) is set in the Class of
 *       Device (CoD) before it becomes discoverable. Set the CoD with
 *       @ref bt_br_set_class_of_device or the @kconfig{CONFIG_BT_COD} Kconfig
 *       option, then call @ref bt_br_set_discoverable.
 *
 * @param server  Transport registration structure, @ref bt_opp_server_transport.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, or @ref bt_opp_server_transport.accept() is
 *         missing.
 * @retval -EEXIST The transport is already registered.
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
 * The registration outlives the transport connection, so an instance that is to
 * be reused for a later connection has to be released first, either by
 * @ref bt_opp_server_unregister or by zeroing the whole structure. The natural
 * place for that is the @ref bt_opp_server_cb.transport_disconnected()
 * callback, which is where the instance becomes free again.
 *
 * @param server  Push Server instance to register, @ref bt_opp_server.
 * @param cb      Callback operations structure, @ref bt_opp_server_cb. Must not
 *                be NULL and must provide every mandatory callback.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, or a mandatory callback is missing.
 * @retval -EINPROGRESS The instance is in use by an OBEX connection.
 * @retval -EALREADY The instance is still registered from an earlier
 *         connection and is left unchanged, see @ref bt_opp_server_unregister.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_register(struct bt_opp_server *server, const struct bt_opp_server_cb *cb);

/**
 * @brief Release a Push Server instance.
 *
 * Undoes @ref bt_opp_server_register, so that the instance can be handed back
 * by @ref bt_opp_server_transport.accept() for a later connection. Call it once
 * the instance is free, which the
 * @ref bt_opp_server_cb.transport_disconnected() callback reports.
 *
 * @param server  Push Server instance to release, @ref bt_opp_server.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter.
 * @retval -EALREADY The instance is not registered.
 * @retval -EINPROGRESS The instance is in use by an OBEX connection.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_unregister(struct bt_opp_server *server);

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
 * @retval -EINVAL Invalid parameter, or @p buf carries a header the profile
 *         excludes.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_connect(struct bt_opp_server *server, uint16_t mopl, uint8_t rsp_code,
			      struct net_buf *buf);

/**
 * @brief Send an OBEX DISCONNECT response from the Push Server.
 *
 * Responds to a disconnect request received via
 * @ref bt_opp_server_cb.disconnect(). With @ref BT_OPP_RSP_CODE_SUCCESS the
 * OBEX connection is torn down.
 *
 * Per IrOBEX 1.2 section 3.3.2 a disconnect may not be refused. The only
 * other code that section allows is @ref BT_OPP_RSP_CODE_UNAVAIL, for a
 * request carrying invalid information. It leaves the OBEX connection of the
 * server established; the client may take it as a refusal and close the
 * transport, which @ref bt_opp_server_cb.transport_disconnected() reports.
 * Any other @p rsp_code is rejected without being sent.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param rsp_code  @ref BT_OPP_RSP_CODE_SUCCESS or @ref BT_OPP_RSP_CODE_UNAVAIL.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, @p rsp_code is neither
 *         @ref BT_OPP_RSP_CODE_SUCCESS nor @ref BT_OPP_RSP_CODE_UNAVAIL, or
 *         @p buf carries a header the profile excludes.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_disconnect(struct bt_opp_server *server, uint8_t rsp_code,
				 struct net_buf *buf);

/**
 * @brief Send an OBEX PUT response from the Push Server.
 *
 * Responds to a push request received via @ref bt_opp_server_cb.push().
 * Per IrOBEX 1.2 section 3.3.3.2, answer a request without the final bit with
 * @ref BT_OPP_RSP_CODE_CONTINUE to ask for the next packet, and the request
 * with the final bit with @ref BT_OPP_RSP_CODE_SUCCESS once the object has
 * been received. @ref BT_OPP_RSP_CODE_SUCCESS is refused for a request
 * without the final bit.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, or @p buf carries a header the profile
 *         excludes.
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
 * object is available, which is what both OPP Spec section 5.6 and IrOBEX 1.2
 * section 8.4 require.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Response headers buffer containing Body or End-of-Body, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, or @p buf carries a header the profile
 *         excludes.
 * @retval -ENODATA @p rsp_code is @ref BT_OPP_RSP_CODE_SUCCESS but @p buf carries
 *         no End-of-Body header.
 * @retval -errno Error code of the underlying OBEX layer on failure.
 */
int bt_opp_server_pull(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf);

/**
 * @brief Send an OBEX ABORT response from the Push Server.
 *
 * Responds to an abort request received via @ref bt_opp_server_cb.abort().
 *
 * Per IrOBEX 1.2 section 3.3.5 the response to an abort is
 * @ref BT_OPP_RSP_CODE_SUCCESS, which tells the client that the server is
 * resynchronized with it, and a client that receives anything else should
 * disconnect. @p rsp_code therefore has to be @ref BT_OPP_RSP_CODE_SUCCESS.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in case
 *       of an error the caller retains the ownership of the buffer.
 *
 * @param server    Push Server instance.
 * @param rsp_code  @ref BT_OPP_RSP_CODE_SUCCESS.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid parameter, @p rsp_code is not
 *         @ref BT_OPP_RSP_CODE_SUCCESS, or @p buf carries a header the profile
 *         excludes.
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
