/** @file
 *  @brief Object Push Profile handling.
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
#define BT_OPP_SDP_UUID  BT_UUID_DECLARE_16(0x1105)

/**
 * @brief SDP attribute ID for the Supported Formats List attribute.
 *
 * Mandatory for OPP servers. Contains a sequence of @ref bt_opp_format
 * values describing the object types the server can receive.
 */
#define BT_OPP_SDP_ATTR_SUPPORTED_FORMATS  0x0303

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
	BT_OPP_FORMAT_ANY       = 0xFF,
};

/**
 * @brief OPP response codes.
 *
 * Mapped from OBEX response codes. Used in all OPP operation callbacks to
 * indicate the result of a request or response.
 */
enum bt_opp_rsp_code {
	/** Continue - more data follows. */
	BT_OPP_RSP_CODE_CONTINUE         = BT_OBEX_RSP_CODE_CONTINUE,
	/** OK / Success (final response, same numeric value as SUCCESS). */
	BT_OPP_RSP_CODE_OK                = BT_OBEX_RSP_CODE_OK,
	/** Success. */
	BT_OPP_RSP_CODE_SUCCESS           = BT_OBEX_RSP_CODE_SUCCESS,
	/** Bad request - server could not understand request. */
	BT_OPP_RSP_CODE_BAD_REQ          = BT_OBEX_RSP_CODE_BAD_REQ,
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
 * All callbacks are optional. The structure must remain valid and constant for
 * the entire lifetime of the Push Client instance.
 */
struct bt_opp_client_cb {
	/**
	 * @brief RFCOMM connection established callback.
	 *
	 * Called when the RFCOMM connection to the Push Server is established.
	 * After this, call @ref bt_opp_client_connect to establish the OBEX
	 * session.
	 *
	 * @param conn    ACL connection object.
	 * @param client  OPP Push Client object.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_opp_client *client);

	/**
	 * @brief RFCOMM connection closed callback.
	 *
	 * Called when the RFCOMM connection to the Push Server is closed.
	 *
	 * @param client  OPP Push Client object.
	 */
	void (*rfcomm_disconnected)(struct bt_opp_client *client);

	/**
	 * @brief OBEX session connect response callback.
	 *
	 * Called when the OBEX CONNECT response is received from the Push Server.
	 * Per OPP Spec section 5.4, the Target header is not used.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code from server, @ref bt_opp_rsp_code.
	 * @param version   OBEX version reported by the server.
	 * @param mopl      Maximum OBEX packet length of the server.
	 * @param buf       Optional response headers buffer, or NULL.
	 */
	void (*connect)(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			uint8_t version, uint16_t mopl, struct net_buf *buf);

	/**
	 * @brief OBEX session disconnect response callback.
	 *
	 * Called when the OBEX DISCONNECT response is received.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
	 * @param buf       Optional response headers buffer, or NULL.
	 */
	void (*disconnect)(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			   struct net_buf *buf);

	/**
	 * @brief Object push (PUT) response callback.
	 *
	 * Called when a PUT response is received from the Push Server.
	 * For multi-packet transfers this is called for each response packet.
	 * @ref BT_OPP_RSP_CODE_CONTINUE indicates the server is ready for the
	 * next body chunk; @ref BT_OPP_RSP_CODE_SUCCESS indicates the transfer
	 * is complete.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
	 * @param buf       Optional response headers buffer, or NULL.
	 */
	void (*push)(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
		     struct net_buf *buf);

	/**
	 * @brief Business card pull (GET) response callback.
	 *
	 * Called when a GET response is received while pulling the server's
	 * default business card. Per spec section 5.6, the server returns
	 * @ref BT_OPP_RSP_CODE_NOT_FOUND when no default object is available.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
	 * @param buf       Response headers buffer containing body data, or NULL.
	 */
	void (*pull_bcard)(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			   struct net_buf *buf);

	/**
	 * @brief Abort response callback.
	 *
	 * Called when the OBEX ABORT response is received.
	 *
	 * @param client    OPP Push Client object.
	 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
	 * @param buf       Optional response headers buffer, or NULL.
	 */
	void (*abort)(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
		      struct net_buf *buf);
};

/** @brief OPP Push Client structure. */
struct bt_opp_client {
	/** @brief Callback operations structure pointer. */
	const struct bt_opp_client_cb *cb;

	/** @internal Underlying GOEP transport instance. */
	struct bt_goep_transport _goep_transport;

	/** @internal GOEP instance. */
	struct bt_goep _goep;

	/** @internal Transport layer state (atomic). */
	atomic_t _transport_state;

	/** @internal OBEX client handle. */
	struct bt_obex_client _client;

	/** @internal OPP session state (atomic). */
	atomic_t _state;
};

/**
 * @brief Connect the Push Client to a Push Server over RFCOMM.
 *
 * Establishes the RFCOMM connection using the channel obtained from SDP
 * discovery (Protocol Descriptor List). On success the
 * @ref bt_opp_client_cb.rfcomm_connected() callback is called; after that,
 * call @ref bt_opp_client_connect to establish the OBEX session.
 *
 * @param conn     ACL connection object.
 * @param client   Push Client instance to connect.
 * @param cb       Callback operations structure, @ref bt_opp_client_cb.
 * @param channel  RFCOMM channel from SDP discovery.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_client_connect_rfcomm(struct bt_conn *conn, struct bt_opp_client *client,
				 const struct bt_opp_client_cb *cb, uint8_t channel);

/**
 * @brief Disconnect the Push Client RFCOMM connection.
 *
 * Closes the RFCOMM connection. The
 * @ref bt_opp_client_cb.rfcomm_disconnected() callback is called when done.
 *
 * @param client  Push Client instance to disconnect.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_client_disconnect_rfcomm(struct bt_opp_client *client);

/**
 * @brief Allocate a PDU buffer with headroom reserved for OPP Push Client.
 *
 * The headroom accounts for all lower-layer headers (OBEX, RFCOMM, and ACL).
 * If @p pool is NULL the default pool is used.
 *
 * @param client  Push Client object.
 * @param pool    Buffer pool to allocate from, or NULL for the default pool.
 *
 * @return Pointer to the allocated buffer with reserved headroom, or NULL.
 */
struct net_buf *bt_opp_client_create_pdu(struct bt_opp_client *client,
					 struct net_buf_pool *pool);

/**
 * @brief Establish an OBEX session with the Push Server.
 *
 * Sends an OBEX CONNECT request. Per OPP Spec section 5.4, the Target header
 * shall not be included. The RFCOMM connection must already be established via
 * @ref bt_opp_client_connect_rfcomm. The @ref bt_opp_client_cb.connect()
 * callback is invoked with the server response.
 *
 * @param client  Push Client instance (RFCOMM must be connected).
 * @param mopl    Maximum OBEX packet length proposed by this client.
 * @param buf     Optional additional OBEX headers buffer, or NULL.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_client_connect(struct bt_opp_client *client, uint16_t mopl, struct net_buf *buf);

/**
 * @brief Terminate the OBEX session.
 *
 * Sends an OBEX DISCONNECT request. The @ref bt_opp_client_cb.disconnect()
 * callback is invoked when the response is received.
 *
 * @param client  Push Client instance.
 * @param buf     Optional OBEX headers buffer, or NULL.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_client_disconnect(struct bt_opp_client *client, struct net_buf *buf);

/**
 * @brief Push an object to the Push Server (OBEX PUT).
 *
 * Sends an OBEX PUT request to deliver an object to the server's inbox.
 * Per OPP Spec sections 4.2.2 and 5.5:
 * - The Name header shall be included.
 * - The Length header shall be included.
 * - The Type header should be included (see @ref BT_OPP_TYPE_VCARD etc.).
 * - Body and End-of-Body headers carry the object data.
 *
 * For multi-packet transfers, set @p final to false and use the Body header
 * (0x48) for every intermediate packet. Set @p final to true and use the
 * End-of-Body header (0x49) for the last (or only) packet. The caller MUST
 * NOT mix Body and End-of-Body within the same packet, and MUST use
 * End-of-Body exclusively in the packet where @p final is true.
 *
 * The @ref bt_opp_client_cb.push() callback is called after each PUT response;
 * @ref BT_OPP_RSP_CODE_CONTINUE means the server is ready for the next chunk.
 *
 * @param client  Push Client instance.
 * @param final   True if this is the last (or only) PUT packet (use End-of-Body
 *                header in @p buf). False for intermediate packets (use Body
 *                header in @p buf).
 * @param buf     Buffer carrying OBEX headers (Name, Type, Length, Body or
 *                End-of-Body, etc.).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_client_push(struct bt_opp_client *client, bool final, struct net_buf *buf);

/**
 * @brief Pull the server's default business card (OBEX GET).
 *
 * Sends an OBEX GET request for the server's Default Get Object (owner
 * business card). Per OPP Spec sections 4.3.2 and 5.6:
 * - The Type header shall be set to @ref BT_OPP_TYPE_VCARD (case insensitive).
 * - The Name header shall be absent or empty.
 *
 * @note OPP Spec Table 5.3 lists the Name header as Mandatory (M) for GET
 *       requests, yet section 5.6 states the Name header is not used.  The
 *       recommended practice for interoperability is to include an empty Name
 *       header (a two-byte UTF-16BE null terminator: 0x00 0x00) in @p buf.
 *
 * The @ref bt_opp_client_cb.pull_bcard() callback is called with each response.
 * @ref BT_OPP_RSP_CODE_NOT_FOUND is returned by the server when no default
 * object exists.
 *
 * @param client  Push Client instance.
 * @param buf     Buffer carrying OBEX headers (Type header required; empty
 *                Name header recommended for interoperability).
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_client_pull_bcard(struct bt_opp_client *client, struct net_buf *buf);

/**
 * @brief Abort the current ongoing operation.
 *
 * Sends an OBEX ABORT request to cancel a push or pull operation that is in
 * progress. The @ref bt_opp_client_cb.abort() callback is invoked when the
 * server responds.
 *
 * @param client  Push Client instance.
 * @param buf     Optional OBEX headers buffer, or NULL.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_client_abort(struct bt_opp_client *client, struct net_buf *buf);

/** @brief Forward declaration of OPP Push Server structure. */
struct bt_opp_server;

/**
 * @brief OPP Push Server callback operations structure.
 *
 * Defines the handlers invoked when the Push Server receives requests from a
 * Push Client. Callbacks marked as optional may be set to NULL; the stack
 * will respond with @ref BT_OPP_RSP_CODE_NOT_IMPL when a NULL handler is
 * invoked. The structure must remain valid and constant for the lifetime of
 * the server.
 */
struct bt_opp_server_cb {
	/**
	 * @brief RFCOMM connection accepted callback.
	 *
	 * Called when a new RFCOMM connection from a Push Client is accepted.
	 *
	 * @param conn    ACL connection object.
	 * @param server  OPP Push Server object.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_opp_server *server);

	/**
	 * @brief RFCOMM connection closed callback.
	 *
	 * Called when the RFCOMM connection from a Push Client is closed.
	 *
	 * @param server  OPP Push Server object.
	 */
	void (*rfcomm_disconnected)(struct bt_opp_server *server);

	/**
	 * @brief OBEX session connect request callback.
	 *
	 * Called when the Push Server receives an OBEX CONNECT request.
	 * Per OPP Spec section 5.4, the Target header will not be present.
	 * The application must call @ref bt_opp_server_connect_rsp to respond.
	 *
	 * @param server   OPP Push Server object.
	 * @param version  OBEX version requested by the client.
	 * @param mopl     Maximum OBEX packet length proposed by the client.
	 * @param buf      Optional request headers buffer, or NULL.
	 */
	void (*connect)(struct bt_opp_server *server, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/**
	 * @brief OBEX session disconnect request callback.
	 *
	 * Called when the Push Server receives an OBEX DISCONNECT request.
	 * The application must call @ref bt_opp_server_disconnect_rsp to respond.
	 *
	 * @param server  OPP Push Server object.
	 * @param buf     Optional request headers buffer, or NULL.
	 */
	void (*disconnect)(struct bt_opp_server *server, struct net_buf *buf);

	/**
	 * @brief Object push (PUT) request callback.
	 *
	 * Called when a PUT request is received from the Push Client. @p buf
	 * contains OBEX headers such as Name, Type, Length, and Body. For
	 * multi-packet objects this callback is invoked for each PUT packet;
	 * the End-of-Body header signals the last packet.
	 *
	 * The application must call @ref bt_opp_server_push_rsp to respond.
	 * If the object format is not supported, respond with
	 * @ref BT_OPP_RSP_CODE_UNSUPP_MEDIA_TYPE. If the object is too large,
	 * respond with @ref BT_OPP_RSP_CODE_ENTITY_TOO_LARGE.
	 *
	 * If this callback is NULL the stack automatically responds with
	 * @ref BT_OPP_RSP_CODE_NOT_IMPL.
	 *
	 * @param server  OPP Push Server object.
	 * @param final   True if this is the last PUT packet (End-of-Body present).
	 * @param buf     Request headers buffer (Name, Type, Length, Body, etc.).
	 */
	void (*push)(struct bt_opp_server *server, bool final, struct net_buf *buf);

	/**
	 * @brief Business card pull (GET) request callback.
	 *
	 * Called when a GET request for the default business card is received.
	 * Per OPP Spec section 4.3.1, the Type header will be set to
	 * @ref BT_OPP_TYPE_VCARD and the Name header will be absent or empty.
	 *
	 * The application must call @ref bt_opp_server_pull_bcard_rsp to respond.
	 * If no default object exists, respond with @ref BT_OPP_RSP_CODE_NOT_FOUND.
	 * If the Name header carries a non-empty value, the stack automatically
	 * responds with @ref BT_OPP_RSP_CODE_FORBIDDEN before invoking this
	 * callback.
	 *
	 * If this callback is NULL the stack automatically responds with
	 * @ref BT_OPP_RSP_CODE_NOT_IMPL.
	 *
	 * @param server  OPP Push Server object.
	 * @param buf     Request headers buffer (Type header present).
	 */
	void (*pull_bcard)(struct bt_opp_server *server, struct net_buf *buf);

	/**
	 * @brief Abort request callback.
	 *
	 * Called when an OBEX ABORT request is received from the Push Client.
	 * The application must call @ref bt_opp_server_abort_rsp to respond.
	 *
	 * @param server  OPP Push Server object.
	 * @param buf     Optional request headers buffer, or NULL.
	 */
	void (*abort)(struct bt_opp_server *server, struct net_buf *buf);
};

/**
 * @brief OPP Push Server RFCOMM registration structure.
 *
 * Used to register a Push Server that accepts incoming RFCOMM connections.
 */
struct bt_opp_server_rfcomm {
	/** @brief Underlying GOEP RFCOMM server. */
	struct bt_goep_transport_rfcomm_server server;

	/**
	 * @brief Accept connection callback.
	 *
	 * Called when a new RFCOMM connection arrives. The application must
	 * allocate and initialize a @ref bt_opp_server instance and pass it
	 * back via @p opp_server. Call @ref bt_opp_server_register on the
	 * instance before returning.
	 *
	 * @param conn           ACL connection object.
	 * @param rfcomm_server  This RFCOMM server registration structure.
	 * @param opp_server     Output pointer for the allocated server instance.
	 *
	 * @return 0 on success, negative error code to reject the connection.
	 * @return -ENOMEM if no server instance is available.
	 * @return -EACCES if the application refuses the connection.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_opp_server_rfcomm *rfcomm_server,
		      struct bt_opp_server **opp_server);
};

/** @brief OPP Push Server structure. */
struct bt_opp_server {
	/** @brief Callback operations structure pointer. */
	const struct bt_opp_server_cb *cb;

	/** @internal Underlying GOEP transport instance. */
	struct bt_goep_transport _goep_transport;

	/** @internal GOEP instance. */
	struct bt_goep _goep;

	/** @internal Transport layer state (atomic). */
	atomic_t _transport_state;

	/** @internal OBEX server handle. */
	struct bt_obex_server _server;

	/** @internal OPP session state (atomic). */
	atomic_t _state;

	/** @internal Internal flags. */
	atomic_t _flags;
};

/**
 * @brief Allocate a PDU buffer with headroom reserved for OPP Push Server.
 *
 * The headroom accounts for all lower-layer headers (OBEX, RFCOMM, and ACL).
 * If @p pool is NULL the default pool is used.
 *
 * @param server  Push Server object.
 * @param pool    Buffer pool to allocate from, or NULL for the default pool.
 *
 * @return Pointer to the allocated buffer with reserved headroom, or NULL.
 */
struct net_buf *bt_opp_server_create_pdu(struct bt_opp_server *server,
					 struct net_buf_pool *pool);

/**
 * @brief Register an OPP Push Server RFCOMM listener.
 *
 * Registers the RFCOMM server to listen for incoming OPP connections. The
 * allocated RFCOMM channel must be advertised via SDP (Protocol Descriptor
 * List). If ``server.rfcomm.channel`` is 0, a
 * channel is auto-assigned by RFCOMM.
 *
 * @note Per OPP Spec v1.1 section 3.1, the application must also set the
 *       Object Transfer service class bit in the Class of Device (CoD) before
 *       making the OPP server discoverable. Use @ref bt_br_set_discoverable
 *       after configuring the CoD via the HCI Write Class of Device command or
 *       the @kconfig{CONFIG_BT_COD} Kconfig option.
 *
 * @param server  RFCOMM registration structure, @ref bt_opp_server_rfcomm.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_server_rfcomm_register(struct bt_opp_server_rfcomm *server);

/**
 * @brief Register a Push Server instance with the OPP subsystem.
 *
 * Initializes the server object and associates the callback structure.
 * This must be called inside the @ref bt_opp_server_rfcomm.accept() callback
 * before returning the instance.
 *
 * @param server  Push Server instance to register, @ref bt_opp_server.
 * @param cb      Callback operations structure, @ref bt_opp_server_cb.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_server_register(struct bt_opp_server *server, const struct bt_opp_server_cb *cb);

/**
 * @brief Send an OBEX CONNECT response from the Push Server.
 *
 * Responds to a connect request received via @ref bt_opp_server_cb.connect().
 *
 * @param server    Push Server instance.
 * @param mopl      Maximum OBEX packet length accepted by this server.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_server_connect_rsp(struct bt_opp_server *server, uint16_t mopl,
			      enum bt_opp_rsp_code rsp_code, struct net_buf *buf);

/**
 * @brief Send an OBEX DISCONNECT response from the Push Server.
 *
 * Responds to a disconnect request received via @ref bt_opp_server_cb.disconnect().
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_server_disconnect_rsp(struct bt_opp_server *server, enum bt_opp_rsp_code rsp_code,
				 struct net_buf *buf);

/**
 * @brief Send an OBEX PUT response from the Push Server.
 *
 * Responds to a push request received via @ref bt_opp_server_cb.push().
 * Send @ref BT_OPP_RSP_CODE_CONTINUE to request the next body chunk, or
 * @ref BT_OPP_RSP_CODE_SUCCESS when the full object has been received.
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_server_push_rsp(struct bt_opp_server *server, enum bt_opp_rsp_code rsp_code,
			   struct net_buf *buf);

/**
 * @brief Send an OBEX GET response from the Push Server (business card pull).
 *
 * Responds to a business card pull request received via
 * @ref bt_opp_server_cb.pull_bcard(). Send @ref BT_OPP_RSP_CODE_CONTINUE for
 * each intermediate body chunk and @ref BT_OPP_RSP_CODE_SUCCESS with the
 * final End-of-Body chunk. Respond with @ref BT_OPP_RSP_CODE_NOT_FOUND if no
 * default object is available.
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code, @ref bt_opp_rsp_code.
 * @param buf       Response headers buffer containing Body or End-of-Body, or NULL.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_server_pull_bcard_rsp(struct bt_opp_server *server, enum bt_opp_rsp_code rsp_code,
				 struct net_buf *buf);

/**
 * @brief Send an OBEX ABORT response from the Push Server.
 *
 * Responds to an abort request received via @ref bt_opp_server_cb.abort().
 *
 * @param server    Push Server instance.
 * @param rsp_code  Response code (typically @ref BT_OPP_RSP_CODE_SUCCESS).
 * @param buf       Optional response headers buffer, or NULL.
 *
 * @return 0 on success, negative error code on failure.
 */
int bt_opp_server_abort_rsp(struct bt_opp_server *server, enum bt_opp_rsp_code rsp_code,
			    struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_OPP_H_ */
