/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth File Transfer Profile handling.
 * @ingroup bt_ftp
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_FTP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_FTP_H_

/**
 * @brief File Transfer Profile (FTP)
 * @defgroup bt_ftp File Transfer Profile (FTP)
 * @since 4.6
 * @version 0.0.1
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdbool.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/goep.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief FTP service target UUID (F9EC7BC4-953C-11D2-984E-525400DC9E09).
 *
 *  128-bit UUID used to identify the FTP service during OBEX connection establishment.
 */
#define BT_FTP_UUID                                                                                \
	(const struct bt_uuid_128 *)BT_UUID_DECLARE_128(                                           \
		BT_UUID_128_ENCODE(0xf9ec7bc4, 0x953c, 0x11d2, 0x984e, 0x525400dc9e09))

/** @brief OBEX type header for folder listing operation. */
#define BT_FTP_FOLDER_LISTING_TYPE "x-obex/folder-listing"

/** @brief FTP set folder operation flags
 *
 * Flags controlling folder navigation behavior in SetFolder operations.
 */
enum __packed bt_ftp_set_folder_flags {
	/** Navigate to root folder */
	BT_FTP_SET_FOLDER_FLAGS_ROOT = BT_OBEX_SETPATH_FLAG_NO_CREATE,
	/** Navigate down to child folder */
	BT_FTP_SET_FOLDER_FLAGS_DOWN = BT_OBEX_SETPATH_FLAG_NO_CREATE,
	/** Navigate up to parent folder */
	BT_FTP_SET_FOLDER_FLAGS_UP = BT_OBEX_SETPATH_FLAG_BACKUP | BT_OBEX_SETPATH_FLAG_NO_CREATE,
	/** Create a new folder */
	BT_FTP_SET_FOLDER_FLAGS_NEW = 0,
};

/** @brief FTP response codes.
 *
 *  Response codes used in FTP operations, mapped from OBEX response codes.
 */
enum __packed bt_ftp_rsp_code {
	/** Continue - more data to follow. */
	BT_FTP_RSP_CODE_CONTINUE = BT_OBEX_RSP_CODE_CONTINUE,
	/** Success. */
	BT_FTP_RSP_CODE_SUCCESS = BT_OBEX_RSP_CODE_SUCCESS,
	/** Bad request. */
	BT_FTP_RSP_CODE_BAD_REQ = BT_OBEX_RSP_CODE_BAD_REQ,
	/** Unauthorized. */
	BT_FTP_RSP_CODE_UNAUTH = BT_OBEX_RSP_CODE_UNAUTH,
	/** Forbidden - operation understood but refused. */
	BT_FTP_RSP_CODE_FORBIDDEN = BT_OBEX_RSP_CODE_FORBIDDEN,
	/** Not found. */
	BT_FTP_RSP_CODE_NOT_FOUND = BT_OBEX_RSP_CODE_NOT_FOUND,
	/** Not acceptable. */
	BT_FTP_RSP_CODE_NOT_ACCEPT = BT_OBEX_RSP_CODE_NOT_ACCEPT,
	/** Precondition failed. */
	BT_FTP_RSP_CODE_PRECON_FAIL = BT_OBEX_RSP_CODE_PRECON_FAIL,
	/** Not implemented. */
	BT_FTP_RSP_CODE_NOT_IMPL = BT_OBEX_RSP_CODE_NOT_IMPL,
	/** Service unavailable. */
	BT_FTP_RSP_CODE_UNAVAIL = BT_OBEX_RSP_CODE_UNAVAIL,
};

/* Forward declarations. */
struct bt_ftp_client;
struct bt_ftp_server;

/**
 * @defgroup bt_ftp_client FTP Client
 * @ingroup bt_ftp
 * @{
 */

/** @brief FTP client callback structure.
 *
 *  The structure must remain valid and constant for the entire lifetime of
 *  the FTP client object.
 */
struct bt_ftp_client_cb {
	/** @brief RFCOMM transport connected callback.
	 *
	 *  Called when the underlying transport (RFCOMM) connection is established.
	 *  OBEX connection has not yet been negotiated at this point.
	 *
	 *  @param conn  Bluetooth connection object.
	 *  @param client FTP client object.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_ftp_client *client);

	/** @brief RFCOMM transport disconnected callback.
	 *
	 *  Called when the underlying transport (RFCOMM) connection is closed.
	 *  All pending operations are cancelled. Also called when the connection attempt
	 *  fails, possibly from within a failing @ref bt_ftp_client_rfcomm_connect call that
	 *  then returns an error as well.
	 *
	 *  @note The transport still uses @p client until this callback has returned. Keep
	 *        @p client valid until then, and do not free, zero or reconnect it from within
	 *        this callback.
	 *
	 *  @param client FTP client object.
	 */
	void (*rfcomm_disconnected)(struct bt_ftp_client *client);

	/** @brief L2CAP transport connected callback.
	 *
	 *  Called when the underlying transport (L2CAP) connection is established.
	 *  OBEX connection has not yet been negotiated at this point.
	 *
	 *  @param conn  Bluetooth connection object.
	 *  @param client FTP client object.
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_ftp_client *client);

	/** @brief L2CAP transport disconnected callback.
	 *
	 *  Called when the underlying transport (L2CAP) connection is closed.
	 *  All pending operations are cancelled. Also called when the connection attempt
	 *  fails, possibly from within a failing @ref bt_ftp_client_l2cap_connect call that
	 *  then returns an error as well.
	 *
	 *  @note The transport still uses @p client until this callback has returned. Keep
	 *        @p client valid until then, and do not free, zero or reconnect it from within
	 *        this callback.
	 *
	 *  @param client FTP client object.
	 */
	void (*l2cap_disconnected)(struct bt_ftp_client *client);

	/** @brief OBEX connect response callback.
	 *
	 *  Called when OBEX CONNECT response is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code (@ref BT_FTP_RSP_CODE_SUCCESS on
	 *                  success).
	 *  @param version  OBEX protocol version supported by server.
	 *  @param mopl     Maximum OBEX packet length supported by server.
	 *  @param buf      Buffer containing additional response headers.
	 */
	void (*connect)(struct bt_ftp_client *client, uint8_t rsp_code, uint8_t version,
			uint16_t mopl, struct net_buf *buf);

	/** @brief OBEX disconnect response callback.
	 *
	 *  Called when OBEX DISCONNECT response is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code.
	 *  @param buf      Buffer containing response headers.
	 */
	void (*disconnect)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @brief OBEX ABORT response callback.
	 *
	 *  Called when a successful OBEX ABORT response is received. Any other
	 *  response code disconnects the transport instead.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code, always @ref BT_FTP_RSP_CODE_SUCCESS.
	 *  @param buf      Buffer containing response headers.
	 */
	void (*abort)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Set folder response callback.
	 *
	 *  Called when SetFolder response is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code.
	 *  @param buf      Buffer containing response headers.
	 */
	void (*set_folder)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Pull folder listing response callback.
	 *
	 *  Called when GetFolderListing response is received.
	 *  May be called multiple times for a single request if response is fragmented.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code (@ref BT_FTP_RSP_CODE_CONTINUE for
	 *                  partial data).
	 *  @param buf      Buffer containing folder listing data and headers.
	 */
	void (*pull_folder_listing)(struct bt_ftp_client *client, uint8_t rsp_code,
				    struct net_buf *buf);

	/** @brief Push file response callback.
	 *
	 *  Called when PutFile response is received.
	 *  May be called multiple times if request is fragmented (@ref BT_FTP_RSP_CODE_CONTINUE).
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code (@ref BT_FTP_RSP_CODE_CONTINUE for
	 *                  partial upload).
	 *  @param buf      Buffer containing response headers.
	 */
	void (*push_file)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Pull file response callback.
	 *
	 *  Called when GetFile response is received.
	 *  May be called multiple times for a single request if response is fragmented.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code (@ref BT_FTP_RSP_CODE_CONTINUE for
	 *                  partial data).
	 *  @param buf      Buffer containing file data and headers.
	 */
	void (*pull_file)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Remove response callback.
	 *
	 *  Called when the response to a remove request, which deletes a file or
	 *  folder, is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code.
	 *  @param buf      Buffer containing response headers.
	 */
	void (*remove)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Rename/Move response callback.
	 *
	 *  Called when rename/move ACTION response is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code.
	 *  @param buf      Buffer containing response headers.
	 */
	void (*rename)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Copy response callback.
	 *
	 *  Called when copy ACTION response is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code.
	 *  @param buf      Buffer containing response headers.
	 */
	void (*copy)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @brief Set permission response callback.
	 *
	 *  Called when set-permissions ACTION response is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code.
	 *  @param buf      Buffer containing response headers.
	 */
	void (*set_permission)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);
};

/** @brief FTP client structure.
 *
 *  Must be zeroed by the caller before use. The structure must remain
 *  address-stable (not moved or freed) for the entire lifetime of the
 *  transport connection.
 */
struct bt_ftp_client {
	/** @internal Callbacks (set via @ref bt_ftp_client_rfcomm_connect or @ref
	 * bt_ftp_client_l2cap_connect).
	 */
	const struct bt_ftp_client_cb *_cb;

	/** @internal Underlying GOEP transport instances (v1 RFCOMM + v2 L2CAP). */
	struct bt_goep_transport _goep_transport;

	/** @internal GOEP session object. */
	struct bt_goep _goep;

	/** @internal Transport-layer connection state (atomic). */
	atomic_t _transport_state;

	/** @internal OBEX session state (atomic). */
	atomic_t _state;

	/** @internal Connection ID assigned by the server. */
	uint32_t _conn_id;

	/** @internal OBEX client handle. */
	struct bt_obex_client _client;

	/** @internal Pending response callback for the current operation. */
	void (*_rsp_cb)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

	/** @internal Current request function type */
	atomic_t _optype;
};

/** @brief Connect FTP client over RFCOMM (GOEP v1.1).
 *
 *  Initiates RFCOMM transport connection to a remote FTP server.
 *  On success, the rfcomm_connected callback @ref bt_ftp_client_cb::rfcomm_connected will be
 *  called. The caller must then issue an OBEX CONNECT via @ref bt_ftp_client_connect.
 *
 *  @note @p cb must provide the rfcomm_connected, rfcomm_disconnected, connect,
 *        disconnect, abort, set_folder and pull_folder_listing callbacks, and at least
 *        one of push_file and pull_file; this is checked with an assertion.
 *
 *  @param conn    Bluetooth connection object.
 *  @param client  FTP client object, @ref bt_ftp_client.
 *  @param cb      Callback table, @ref bt_ftp_client_cb.
 *  @param channel RFCOMM server channel (from SDP discovery).
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p conn, @p client, or @p cb is NULL, or @p channel is 0.
 *  @retval -EINPROGRESS The client's transport is already connecting, connected, or
 *          disconnecting.
 *  @retval Other negative error codes from the underlying GOEP RFCOMM transport layer.
 */
int bt_ftp_client_rfcomm_connect(struct bt_conn *conn, struct bt_ftp_client *client,
				 const struct bt_ftp_client_cb *cb, uint8_t channel);

/** @brief Disconnect FTP client RFCOMM transport.
 *
 *  Closes the RFCOMM transport connection.
 *  On success, the rfcomm_disconnected callback @ref bt_ftp_client_cb::rfcomm_disconnected will
 *  be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client is NULL.
 *  @retval -EINPROGRESS The client's transport is not currently connected.
 *  @retval Other negative error codes from the underlying GOEP RFCOMM transport layer.
 */
int bt_ftp_client_rfcomm_disconnect(struct bt_ftp_client *client);

/** @brief Connect FTP client over L2CAP (GOEP v2).
 *
 *  Initiates L2CAP transport connection to a remote FTP server.
 *  On success, the l2cap_connected callback @ref bt_ftp_client_cb::l2cap_connected will be
 *  called. The caller must then issue an OBEX CONNECT via @ref bt_ftp_client_connect.
 *
 *  @note @p cb must provide the l2cap_connected, l2cap_disconnected, connect,
 *        disconnect, abort, set_folder and pull_folder_listing callbacks, and at least
 *        one of push_file and pull_file; this is checked with an assertion.
 *
 *  @param conn   Bluetooth connection object.
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param cb     Callback table, @ref bt_ftp_client_cb.
 *  @param psm    L2CAP PSM (from SDP discovery).
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p conn, @p client, or @p cb is NULL, or @p psm is 0.
 *  @retval -EINPROGRESS The client's transport is already connecting, connected, or
 *          disconnecting.
 *  @retval Other negative error codes from the underlying GOEP L2CAP transport layer.
 */
int bt_ftp_client_l2cap_connect(struct bt_conn *conn, struct bt_ftp_client *client,
				const struct bt_ftp_client_cb *cb, uint16_t psm);

/** @brief Disconnect FTP client L2CAP transport.
 *
 *  Closes the L2CAP transport connection.
 *  On success, the l2cap_disconnected callback @ref bt_ftp_client_cb::l2cap_disconnected will be
 *  called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client is NULL.
 *  @retval -EINPROGRESS The client's transport is not currently connected.
 *  @retval Other negative error codes from the underlying GOEP L2CAP transport layer.
 */
int bt_ftp_client_l2cap_disconnect(struct bt_ftp_client *client);

/** @brief Send an OBEX CONNECT request to the FTP server.
 *
 *  Initiates OBEX session establishment with the FTP server.
 *  Must be called after transport connection is established.
 *  Once OBEX connect response received, the connect callback @ref bt_ftp_client_cb::connect will
 *  be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer containing connect headers, or NULL. If @p buf is NULL or empty, a
 *                TARGET header with the FTP service UUID is added. Otherwise @p buf must
 *                contain a TARGET header with the FTP service UUID, as its first header
 *                (IrOBEX 1.5, section 2.2.7).
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client is NULL, the transport is not connected, the client is not in
 *          the disconnected OBEX state, or the TARGET header in @p buf is malformed or does
 *          not match the FTP service UUID.
 *  @retval -ENODATA @p buf is not empty and has no TARGET header, or its TARGET header is
 *          empty.
 *  @retval -ENOBUFS Buffer allocation failed when @p buf is NULL.
 *  @retval -ENOMEM @p buf has insufficient tailroom for the TARGET header.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_connect(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send an OBEX DISCONNECT request to the FTP server.
 *
 *  Initiates OBEX session termination.
 *  Once OBEX disconnect response received, the disconnect callback @ref
 *  bt_ftp_client_cb::disconnect will be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer containing disconnect headers, or NULL. If @p buf is NULL or empty, a
 *                CONN_ID header is added. Otherwise @p buf must contain the CONN_ID header,
 *                as its first header (IrOBEX 1.5, section 2.2.11).
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client is NULL, the client is not in the connected OBEX state, or the
 *          CONN_ID header in @p buf does not match the session's connection ID.
 *  @retval -ENODATA @p buf is not empty and has no valid CONN_ID header.
 *  @retval -ENOBUFS Buffer allocation failed when @p buf is NULL.
 *  @retval -ENOMEM @p buf has insufficient tailroom for the CONN_ID header.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_disconnect(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send an OBEX ABORT request to the FTP server.
 *
 *  Aborts the currently ongoing operation.
 *  Once OBEX abort response received, the abort callback @ref bt_ftp_client_cb::abort will be
 *  called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer containing abort headers, or NULL. If @p buf is NULL or empty, a
 *                CONN_ID header is added. Otherwise the CONN_ID header is optional (IrOBEX
 *                1.5, section 2.2.11); if @p buf has one, it must be the first header.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client is NULL, the client is not in the connected OBEX state, or the
 *          CONN_ID header in @p buf does not match the session's connection ID.
 *  @retval -ENODATA The CONN_ID header in @p buf is present but malformed.
 *  @retval -ENOBUFS Buffer allocation failed when @p buf is NULL.
 *  @retval -ENOMEM @p buf has insufficient tailroom for the CONN_ID header.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_abort(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send a SetPath (set folder) request to the FTP server.
 *
 *  Navigates the folder hierarchy on the FTP server, or creates a new folder when @p flags is
 *  @ref BT_FTP_SET_FOLDER_FLAGS_NEW.
 *  Once set_folder response received, the set_folder callback @ref
 *  bt_ftp_client_cb::set_folder will be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param flags  Navigation flags, @ref bt_ftp_set_folder_flags.
 *                - @ref BT_FTP_SET_FOLDER_FLAGS_ROOT navigates to the root folder, using an
 *                  empty Name header.
 *                - @ref BT_FTP_SET_FOLDER_FLAGS_DOWN enters the child folder named by the
 *                  Name header.
 *                - @ref BT_FTP_SET_FOLDER_FLAGS_UP returns to the parent folder; no Name
 *                  header is needed.
 *                - @ref BT_FTP_SET_FOLDER_FLAGS_NEW creates the folder named by the Name
 *                  header and makes it the current folder.
 *  @param buf    Buffer containing the folder name and headers. A Name header is required for
 *                every @p flags value except @ref BT_FTP_SET_FOLDER_FLAGS_UP. The connection
 *                ID header is required as the first header and is validated. If @p buf is
 *                NULL, a buffer with only the connection ID header is allocated.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client is NULL, the client is not in the connected OBEX state, @p flags
 *          is not a valid @ref bt_ftp_set_folder_flags value, @p buf is missing the required
 *          Name header for @p flags, @p buf contains a Name header while @p flags is
 *          @ref BT_FTP_SET_FOLDER_FLAGS_UP, the Name header is empty while @p flags is
 *          @ref BT_FTP_SET_FOLDER_FLAGS_NEW, or the CONN_ID header in @p buf does not match
 *          the session's connection ID.
 *  @retval -ENODATA The CONN_ID header is missing from @p buf or malformed.
 *  @retval -ENOBUFS Buffer allocation failed when @p buf is NULL.
 *  @retval -ENOMEM The buffer allocated when @p buf is NULL has insufficient tailroom for the
 *          CONN_ID header.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_set_folder(struct bt_ftp_client *client, uint8_t flags, struct net_buf *buf);

/** @brief Send a GET folder listing request to the FTP server.
 *
 *  Retrieves the XML folder listing of the current folder.
 *  Once get folder listing response received, the pull_folder_listing callback @ref
 *  bt_ftp_client_cb::pull_folder_listing will be called (possibly multiple times).
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param final  True if this is the final packet, false if more data follows.
 *  @param buf    Buffer containing filter parameters and headers.
 *                The initial request must start with the connection ID header and include
 *                the type header.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client or @p buf is NULL, the client is not in the connected state,
 *          the CONN_ID header does not match the session's connection ID, the TYPE header
 *          does not equal @ref BT_FTP_FOLDER_LISTING_TYPE, or no pull_folder_listing
 *          callback is registered.
 *  @retval -ENODATA The CONN_ID or TYPE header is missing from the initial request.
 *  @retval -EBUSY Another operation is still in progress.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_pull_folder_listing(struct bt_ftp_client *client, bool final,
				      struct net_buf *buf);

/** @brief Send a PUT request to push a file to the FTP server.
 *
 *  Uploads a file (or a fragment of a file) to the server.
 *  Once push file response received, the push_file callback @ref
 *  bt_ftp_client_cb::push_file will be called (possibly multiple times).
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param final  True if this is the last packet, false if more data follows.
 *  @param buf    Buffer containing the Name and Body/End-Body headers and file data.
 *                The initial request must start with the connection ID header and include
 *                the Name header.
 *                A non-final packet may carry headers only, with the body following in
 *                later packets. Must include end-of-body header when final is true.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client or @p buf is NULL, the client is not in the connected state,
 *          the CONN_ID header does not match the session's connection ID, the Name header
 *          is missing, no push_file callback is registered, or the End-of-Body header is
 *          missing while @p final is true.
 *  @retval -ENODATA The CONN_ID header is missing from the initial request.
 *  @retval -EBUSY Another operation is still in progress.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_push_file(struct bt_ftp_client *client, bool final, struct net_buf *buf);

/** @brief Send a GET request to pull a file from the FTP server.
 *
 *  Downloads a file (or a fragment) from the server.
 *  Once get file response received, the pull_file callback @ref
 *  bt_ftp_client_cb::pull_file will be called (possibly multiple times).
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param final  True if this is the final packet, false if more data follows.
 *  @param buf    Buffer containing the Name header and optional headers.
 *                The initial request must start with the connection ID header and include
 *                the Name header.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client or @p buf is NULL, the client is not in the connected state,
 *          the CONN_ID header does not match the session's connection ID, the Name header
 *          is missing, or no pull_file callback is registered.
 *  @retval -ENODATA The CONN_ID header is missing from the initial request.
 *  @retval -EBUSY Another operation is still in progress.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_pull_file(struct bt_ftp_client *client, bool final, struct net_buf *buf);

/** @brief Send a remove request to the FTP server.
 *
 *  Removes (deletes) the file or folder identified by the Name header. This is
 *  implemented as a PUT without any Body or End-of-Body header.
 *  Once remove response received, the remove callback @ref bt_ftp_client_cb::remove will
 *  be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer starting with the connection ID header and containing the Name header
 *                of the file or folder to remove.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client or @p buf is NULL, the client is not in the connected state,
 *          the CONN_ID header does not match the session's connection ID, the Name header
 *          is missing, no remove callback is registered, or @p buf unexpectedly contains a
 *          Body/End-of-Body header.
 *  @retval -ENODATA The CONN_ID header is missing from @p buf.
 *  @retval -EBUSY Another operation is still in progress.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_remove(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send a RENAME/MOVE action request to the FTP server.
 *
 *  Renames or moves the object identified by the Name header to the path given
 *  in the DestName header.
 *  Once rename response received, the rename callback @ref bt_ftp_client_cb::rename will be
 *  called.
 *
 *  @note The ACTION operation is only allowed over the L2CAP transport (GOEP v2).
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer containing the Name (source) and DestName (destination) headers.
 *                The connection ID header must be the first header and the Action-ID header
 *                the second (IrOBEX 1.5, section 2.2.20).
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client or @p buf is NULL, the client is not in the connected state,
 *          the CONN_ID header does not match the session's connection ID, the Name or
 *          DestName header is missing, the Action-ID header is not
 *          @c BT_OBEX_ACTION_MOVE_RENAME, or no rename callback is registered.
 *  @retval -ENODATA The CONN_ID or Action-ID header is missing from @p buf.
 *  @retval -EBUSY Another operation is still in progress.
 *  @retval -ENOTSUP The transport is RFCOMM.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_rename(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send a COPY action request to the FTP server.
 *
 *  Copies the object identified by the Name header to the path given in the
 *  DestName header.
 *  Once copy response received, the copy callback @ref bt_ftp_client_cb::copy will be called.
 *
 *  @note The ACTION operation is only allowed over the L2CAP transport (GOEP v2).
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer containing the Name (source) and DestName (destination) headers.
 *                The connection ID header must be the first header and the Action-ID header
 *                the second (IrOBEX 1.5, section 2.2.20).
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client or @p buf is NULL, the client is not in the connected state,
 *          the CONN_ID header does not match the session's connection ID, the Name or
 *          DestName header is missing, the Action-ID header is not @c BT_OBEX_ACTION_COPY,
 *          or no copy callback is registered.
 *  @retval -ENODATA The CONN_ID or Action-ID header is missing from @p buf.
 *  @retval -EBUSY Another operation is still in progress.
 *  @retval -ENOTSUP The transport is RFCOMM.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_copy(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send a SET PERMISSIONS action request to the FTP server.
 *
 *  Changes the permissions of the object identified by the Name header.
 *  Once set_permission response received, the set_permission callback @ref
 *  bt_ftp_client_cb::set_permission will be called.
 *
 *  @note The ACTION operation is only allowed over the L2CAP transport (GOEP v2).
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer containing the Name header and the Permissions header.
 *                The connection ID header must be the first header and the Action-ID header
 *                the second (IrOBEX 1.5, section 2.2.20).
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p client or @p buf is NULL, the client is not in the connected state,
 *          the CONN_ID header does not match the session's connection ID, the Name or
 *          Permissions header is missing, the Action-ID header is not
 *          @c BT_OBEX_ACTION_SET_PERM, or no set_permission callback is registered.
 *  @retval -ENODATA The CONN_ID or Action-ID header is missing from @p buf.
 *  @retval -EBUSY Another operation is still in progress.
 *  @retval -ENOTSUP The transport is RFCOMM.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_client_set_permission(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Allocate a PDU buffer for an FTP client operation.
 *
 *  Allocates a buffer for building FTP request packets. Only headroom for the OBEX and
 *  transport headers is reserved; the data size is that of the buffers of @p pool.
 *
 *  Depending on the calling thread, the allocation may wait until a buffer is free. Must not
 *  be called from an ISR.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param pool   Buffer pool to allocate from, or NULL for the default pool.
 *
 *  @return Allocated buffer, or NULL on allocation failure.
 */
struct net_buf *bt_ftp_client_create_pdu(struct bt_ftp_client *client, struct net_buf_pool *pool);

/** @} */ /* bt_ftp_client */

/**
 * @defgroup bt_ftp_server FTP Server
 * @ingroup bt_ftp
 * @{
 */

/** @brief FTP server callback structure.
 *
 *  The structure must remain valid and constant for the entire lifetime of
 *  the FTP server object.
 *
 *  The SETPATH flags and the headers passed to the request callbacks come from the remote
 *  device. The FTP server checks the Connection Id header of PUT, GET and ACTION requests and
 *  uses the Type and Action Id headers to select the callback. Of the other headers, it only
 *  checks that the ones an operation requires are present, not their values. The
 *  application must validate the names, paths, permissions and flags itself, and reject any
 *  request it does not accept with an error response. FTP v1.3.1, section 2.3: "For security
 *  reasons, Servers should reject attempts by a Client to browse files and folders above or
 *  outside the root folder by returning an appropriate error response."
 */
struct bt_ftp_server_cb {
	/** @brief RFCOMM transport connected callback.
	 *
	 *  Called when the underlying transport (RFCOMM) connection is established.
	 *  OBEX connection has not yet been negotiated at this point.
	 *
	 *  @param conn   Bluetooth connection object.
	 *  @param server FTP server object.
	 */
	void (*rfcomm_connected)(struct bt_conn *conn, struct bt_ftp_server *server);

	/** @brief RFCOMM transport disconnected callback.
	 *
	 *  Called when the underlying transport (RFCOMM) connection is closed.
	 *  All pending operations are cancelled.
	 *
	 *  @param server FTP server object.
	 */
	void (*rfcomm_disconnected)(struct bt_ftp_server *server);

	/** @brief L2CAP transport connected callback.
	 *
	 *  Called when the underlying transport (L2CAP) connection is established.
	 *  OBEX connection has not yet been negotiated at this point.
	 *
	 *  @param conn   Bluetooth connection object.
	 *  @param server FTP server object.
	 */
	void (*l2cap_connected)(struct bt_conn *conn, struct bt_ftp_server *server);

	/** @brief L2CAP transport disconnected callback.
	 *
	 *  Called when the underlying transport (L2CAP) connection is closed.
	 *  All pending operations are cancelled.
	 *
	 *  @param server FTP server object.
	 */
	void (*l2cap_disconnected)(struct bt_ftp_server *server);

	/** @brief OBEX CONNECT request callback.
	 *
	 *  Called when the client sends an OBEX CONNECT request. The server
	 *  must reply via @ref bt_ftp_server_connect.
	 *
	 *  @param server  FTP server object.
	 *  @param version OBEX version proposed by the client.
	 *  @param mopl    Maximum OBEX packet length proposed by the client.
	 *  @param buf     Buffer containing request headers.
	 */
	void (*connect)(struct bt_ftp_server *server, uint8_t version, uint16_t mopl,
			struct net_buf *buf);

	/** @brief OBEX DISCONNECT request callback.
	 *
	 *  Called when the client sends an OBEX DISCONNECT request. The server
	 *  must reply via @ref bt_ftp_server_disconnect.
	 *
	 *  @param server FTP server object.
	 *  @param buf    Buffer containing request headers.
	 */
	void (*disconnect)(struct bt_ftp_server *server, struct net_buf *buf);

	/** @brief OBEX ABORT request callback.
	 *
	 *  Called when the client sends an OBEX ABORT request to cancel the
	 *  current multi-packet operation. The server must reply via
	 *  @ref bt_ftp_server_abort.
	 *
	 *  @param server FTP server object.
	 *  @param buf    Buffer containing request headers.
	 */
	void (*abort)(struct bt_ftp_server *server, struct net_buf *buf);

	/** @brief Set folder request callback.
	 *
	 *  Called when the client sends a SetPath request. The server must
	 *  reply via @ref bt_ftp_server_set_folder.
	 *
	 *  The requested folder is determined by @p flags together with the
	 *  Name header in @p buf:
	 *  - @ref BT_FTP_SET_FOLDER_FLAGS_ROOT with an empty Name header (length 0):
	 *    the root folder.
	 *  - @ref BT_FTP_SET_FOLDER_FLAGS_DOWN with a non-empty Name header: the named
	 *    child folder of the current folder.
	 *  - @ref BT_FTP_SET_FOLDER_FLAGS_UP with no Name header: the parent folder of
	 *    the current folder.
	 *  - @ref BT_FTP_SET_FOLDER_FLAGS_NEW with a non-empty Name header: a new folder
	 *    with that name, created in the current folder.
	 *
	 *  @note @ref BT_FTP_SET_FOLDER_FLAGS_ROOT and @ref BT_FTP_SET_FOLDER_FLAGS_DOWN have
	 *        the same value, so the length of the Name header is what distinguishes them.
	 *
	 *  @param server FTP server object.
	 *  @param flags  Navigation flags, @ref bt_ftp_set_folder_flags.
	 *  @param buf    Buffer containing request headers (e.g. Name).
	 */
	void (*set_folder)(struct bt_ftp_server *server, uint8_t flags, struct net_buf *buf);

	/** @brief Pull folder listing request callback.
	 *
	 *  Called when the client sends a GET folder-listing request. May be
	 *  called multiple times if the client sends multiple GET packets.
	 *  The server must reply via @ref bt_ftp_server_pull_folder_listing.
	 *
	 *  @param server FTP server object.
	 *  @param final  True if this is the final GET packet from the client.
	 *  @param buf    Buffer containing the request headers.
	 */
	void (*pull_folder_listing)(struct bt_ftp_server *server, bool final, struct net_buf *buf);

	/** @brief Push file request callback.
	 *
	 *  Called when the client sends a PUT request to upload a file. May be
	 *  called multiple times for a fragmented file upload; non-final packets
	 *  may carry headers only, with the body following in later packets.
	 *  The server must reply via @ref bt_ftp_server_push_file.
	 *
	 *  @param server FTP server object.
	 *  @param final  True if this is the last PUT packet from the client.
	 *  @param buf    Buffer containing the Name and Body headers.
	 */
	void (*push_file)(struct bt_ftp_server *server, bool final, struct net_buf *buf);

	/** @brief Pull file request callback.
	 *
	 *  Called when the client sends a GET file request. May be called
	 *  multiple times if the client sends multiple GET packets.
	 *  The server must reply via @ref bt_ftp_server_pull_file.
	 *
	 *  @param server FTP server object.
	 *  @param final  True if this is the final GET packet from the client.
	 *  @param buf    Buffer containing the Name header.
	 */
	void (*pull_file)(struct bt_ftp_server *server, bool final, struct net_buf *buf);

	/** @brief Remove request callback.
	 *
	 *  Called when the client sends a DELETE request (a final PUT with a
	 *  Name header and without a Body/End-of-Body header). Remove is used to
	 *  delete a file or folder.
	 *  The server must reply via @ref bt_ftp_server_remove.
	 *
	 *  @param server FTP server object.
	 *  @param buf    Buffer containing the Name header.
	 */
	void (*remove)(struct bt_ftp_server *server, struct net_buf *buf);

	/** @brief Rename/Move action request callback.
	 *
	 *  Called when the client sends a rename ACTION request. The server
	 *  must reply via @ref bt_ftp_server_rename.
	 *
	 *  @note Only called on the L2CAP transport. An ACTION request received over
	 *        RFCOMM is rejected with @ref BT_FTP_RSP_CODE_NOT_IMPL.
	 *
	 *  @param server FTP server object.
	 *  @param buf    Buffer containing the Name and DestName headers.
	 */
	void (*rename)(struct bt_ftp_server *server, struct net_buf *buf);

	/** @brief Copy action request callback.
	 *
	 *  Called when the client sends a copy ACTION request. The server must
	 *  reply via @ref bt_ftp_server_copy.
	 *
	 *  @note Only called on the L2CAP transport. An ACTION request received over
	 *        RFCOMM is rejected with @ref BT_FTP_RSP_CODE_NOT_IMPL.
	 *
	 *  @param server FTP server object.
	 *  @param buf    Buffer containing the Name and DestName headers.
	 */
	void (*copy)(struct bt_ftp_server *server, struct net_buf *buf);

	/** @brief Set permission action request callback.
	 *
	 *  Called when the client sends a set-permissions ACTION request. The
	 *  server must reply via @ref bt_ftp_server_set_permission.
	 *
	 *  @note Only called on the L2CAP transport. An ACTION request received over
	 *        RFCOMM is rejected with @ref BT_FTP_RSP_CODE_NOT_IMPL.
	 *
	 *  @param server FTP server object.
	 *  @param buf    Buffer containing the Name and Permissions headers.
	 */
	void (*set_permission)(struct bt_ftp_server *server, struct net_buf *buf);
};

/** @brief FTP server structure.
 *
 *  Must be zeroed by the caller before use. Must remain address-stable for
 *  the lifetime of the transport connection.
 */
struct bt_ftp_server {
	/** @internal Callbacks (set via @ref bt_ftp_server_register). */
	const struct bt_ftp_server_cb *_cb;

	/** @internal Underlying GOEP transport instances (v1 RFCOMM + v2 L2CAP). */
	struct bt_goep_transport _goep_transport;

	/** @internal GOEP session object. */
	struct bt_goep _goep;

	/** @internal Transport-layer connection state (atomic). */
	atomic_t _transport_state;

	/** @internal OBEX session state (atomic). */
	atomic_t _state;

	/** @internal Connection ID assigned to this session. */
	uint32_t _conn_id;

	/** @internal OBEX server handle. */
	struct bt_obex_server _server;

	/** @internal Pending request callback for the current multi-packet operation.
	 *
	 *  Set when the first packet of a push file (PUT) or pull folder listing /
	 *  pull file (GET) operation is received. Single-packet operations (delete,
	 *  rename, copy and set permission) are dispatched directly and do not use it.
	 *  Cleared when the server sends a final response (non-CONTINUE) or an
	 *  error response to that operation.
	 */
	void (*_req_cb)(struct bt_ftp_server *server, bool final, struct net_buf *buf);

	/** @internal Current request function type */
	atomic_t _optype;
};

/** @brief FTP server RFCOMM registration structure. */
struct bt_ftp_server_rfcomm {
	/** Underlying GOEP RFCOMM server. */
	struct bt_goep_transport_rfcomm_server server;

	/** @brief Accept connection callback.
	 *
	 *  Called for each new incoming RFCOMM connection. The application must
	 *  return through @p ftp_server an @ref bt_ftp_server instance that has been
	 *  registered through bt_ftp_server_register() before accept() returns; this is
	 *  checked with an assertion. An instance stays registered after its transport has
	 *  disconnected and can be returned again for a later connection: do not zero it or
	 *  call bt_ftp_server_register() for it again (that returns @c -EALREADY). The
	 *  returned instance must not be in use (its transport has not fully disconnected);
	 *  this is checked with an assertion.
	 *
	 *  @param conn              Bluetooth connection object.
	 *  @param ftp_server_rfcomm The server registration structure.
	 *  @param ftp_server        Output pointer for the allocated server instance.
	 *
	 *  @return 0 on success, negative error code to reject the connection.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_ftp_server_rfcomm *ftp_server_rfcomm,
		      struct bt_ftp_server **ftp_server);
};

/** @brief FTP server L2CAP registration structure. */
struct bt_ftp_server_l2cap {
	/** Underlying GOEP L2CAP server. */
	struct bt_goep_transport_l2cap_server server;

	/** @brief Accept connection callback.
	 *
	 *  Called for each new incoming L2CAP connection. The application must
	 *  return through @p ftp_server an @ref bt_ftp_server instance that has been
	 *  registered through bt_ftp_server_register() before accept() returns; this is
	 *  checked with an assertion. An instance stays registered after its transport has
	 *  disconnected and can be returned again for a later connection: do not zero it or
	 *  call bt_ftp_server_register() for it again (that returns @c -EALREADY). The
	 *  returned instance must not be in use (its transport has not fully disconnected);
	 *  this is checked with an assertion.
	 *  @param conn             Bluetooth connection object.
	 *  @param ftp_server_l2cap The server registration structure.
	 *  @param ftp_server       Output pointer for the allocated server instance.
	 *
	 *  @return 0 on success, negative error code to reject the connection.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_ftp_server_l2cap *ftp_server_l2cap,
		      struct bt_ftp_server **ftp_server);
};

/** @brief Register an FTP server instance with the OBEX layer.
 *
 *  Associates a callback table with the server object and registers it with
 *  the underlying OBEX server.
 *
 *  @note The @p server object must be zero-initialized by the caller before
 *        this call. Each server instance can only be registered once; if it
 *        is already registered, this function returns @c -EALREADY rather
 *        than overwriting its state.
 *
 *  @note @p cb must provide the rfcomm_connected, rfcomm_disconnected, l2cap_connected,
 *        l2cap_disconnected, connect, disconnect, abort, set_folder and
 *        pull_folder_listing callbacks, and at least one of push_file and pull_file;
 *        this is checked with an assertion.
 *
 *  @param server FTP server object, @ref bt_ftp_server.
 *  @param cb     Callback table, @ref bt_ftp_server_cb.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server or @p cb is NULL.
 *  @retval -EALREADY @p server is already registered.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_register(struct bt_ftp_server *server, const struct bt_ftp_server_cb *cb);

/** @brief Register an FTP RFCOMM server.
 *
 *  Register the server to listen for incoming RFCOMM connections. The allocated channel is
 *  stored in @ref bt_rfcomm_server::channel of @ref bt_ftp_server_rfcomm::server and is
 *  advertised via SDP.
 *
 *  @ref bt_ftp_server_l2cap_register must also be called, before or after this function. The
 *  SDP record is registered once both the RFCOMM and the L2CAP servers have been registered.
 *
 *  @note This function can only be called once per application; only a
 *        single listening RFCOMM channel is tracked for SDP registration.
 *        A second call returns @c -EALREADY.
 *
 *  @param server RFCOMM server registration structure, @ref bt_ftp_server_rfcomm.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server or @c server->accept is NULL.
 *  @retval -EALREADY An RFCOMM server is already registered.
 *  @retval Other negative error codes from the underlying GOEP RFCOMM transport layer or
 *          SDP registration.
 */
int bt_ftp_server_rfcomm_register(struct bt_ftp_server_rfcomm *server);

/** @brief Register an FTP L2CAP server.
 *
 *  Register the server to listen for incoming L2CAP connections. The allocated PSM is
 *  stored in @ref bt_l2cap_server::psm of @ref bt_ftp_server_l2cap::server and is
 *  advertised via SDP.
 *
 *  @ref bt_ftp_server_rfcomm_register must also be called, before or after this function. The
 *  SDP record is registered once both the RFCOMM and the L2CAP servers have been registered.
 *
 *  @note This function can only be called once per application; only a
 *        single listening L2CAP PSM is tracked for SDP registration.
 *        A second call returns @c -EALREADY.
 *
 *  @param server L2CAP server registration structure, @ref bt_ftp_server_l2cap.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server or @c server->accept is NULL.
 *  @retval -EALREADY An L2CAP server is already registered.
 *  @retval Other negative error codes from the underlying GOEP L2CAP transport layer or
 *          SDP registration.
 */
int bt_ftp_server_l2cap_register(struct bt_ftp_server_l2cap *server);

/** @brief Disconnect the FTP server RFCOMM transport.
 *
 *  Closes the RFCOMM transport connection.
 *  On success, the rfcomm_disconnected callback @ref bt_ftp_server_cb::rfcomm_disconnected will
 *  be called.
 *
 *  @param server FTP server object, @ref bt_ftp_server.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL.
 *  @retval -EINPROGRESS The server's transport is not currently connected.
 *  @retval Other negative error codes from the underlying GOEP RFCOMM transport layer.
 */
int bt_ftp_server_rfcomm_disconnect(struct bt_ftp_server *server);

/** @brief Disconnect the FTP server L2CAP transport.
 *
 *  Closes the L2CAP transport connection.
 *  On success, the l2cap_disconnected callback @ref bt_ftp_server_cb::l2cap_disconnected will be
 *  called.
 *
 *  @param server FTP server object, @ref bt_ftp_server.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL.
 *  @retval -EINPROGRESS The server's transport is not currently connected.
 *  @retval Other negative error codes from the underlying GOEP L2CAP transport layer.
 */
int bt_ftp_server_l2cap_disconnect(struct bt_ftp_server *server);

/** @brief Send an OBEX CONNECT response from the FTP server.
 *
 *  Responds to an OBEX CONNECT request from the remote FTP client.
 *  Called from the connect callback @ref bt_ftp_server_cb::connect.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connecting OBEX state, the
 *          CONN_ID header in @p buf does not match the session's connection ID, the WHO
 *          header in @p buf is malformed or does not match the FTP service UUID, or
 *          @p rsp_code is @ref BT_FTP_RSP_CODE_CONTINUE.
 *  @retval -ENODATA The CONN_ID or WHO header in @p buf is present but empty or malformed.
 *  @retval -ENOBUFS Buffer allocation failed when @p buf is NULL.
 *  @retval -ENOMEM @p buf has insufficient tailroom for the CONN_ID or WHO header.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_connect(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send an OBEX DISCONNECT response from the FTP server.
 *
 *  Responds to an OBEX DISCONNECT request from the remote FTP client.
 *  Called from the disconnect callback @ref bt_ftp_server_cb::disconnect.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the disconnecting OBEX state, or
 *          @p rsp_code is @ref BT_FTP_RSP_CODE_CONTINUE.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_disconnect(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send an OBEX ABORT response from the FTP server.
 *
 *  Responds to an OBEX ABORT request from the remote FTP client.
 *  Called from the abort callback @ref bt_ftp_server_cb::abort.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, typically @ref BT_FTP_RSP_CODE_SUCCESS.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, or no
 *          ABORT request is pending.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_abort(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send a set folder response from the FTP server.
 *
 *  Responds to a SetPath request from the remote FTP client.
 *  Called from the set_folder callback @ref bt_ftp_server_cb::set_folder.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, or no
 *          SetPath request is pending.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_set_folder(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send a pull folder listing response from the FTP server.
 *
 *  Responds to a GET folder listing request from the remote FTP client.
 *  Called from the pull_folder_listing callback @ref bt_ftp_server_cb::pull_folder_listing.
 *  Set @p rsp_code to @ref BT_FTP_RSP_CODE_CONTINUE when there are more fragments to send,
 *  or @ref BT_FTP_RSP_CODE_SUCCESS for the final fragment.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer containing the folder listing data and headers.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, no
 *          pull_folder_listing operation is pending, or @p rsp_code is
 *          @ref BT_FTP_RSP_CODE_SUCCESS before the final request packet has been received.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_pull_folder_listing(struct bt_ftp_server *server, uint8_t rsp_code,
				      struct net_buf *buf);

/** @brief Send a push file response from the FTP server.
 *
 *  Responds to a PUT file request from the remote FTP client.
 *  Called from the push_file callback @ref bt_ftp_server_cb::push_file.
 *  Set @p rsp_code to @ref BT_FTP_RSP_CODE_CONTINUE to request more data from
 *  the client, or @ref BT_FTP_RSP_CODE_SUCCESS when the file upload is complete.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, no
 *          push_file operation is pending, or @p rsp_code is @ref BT_FTP_RSP_CODE_SUCCESS
 *          before the final request packet has been received.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_push_file(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send a pull file response from the FTP server.
 *
 *  Responds to a GET file request from the remote FTP client.
 *  Called from the pull_file callback @ref bt_ftp_server_cb::pull_file.
 *  Set @p rsp_code to @ref BT_FTP_RSP_CODE_CONTINUE when there are more fragments to send,
 *  or @ref BT_FTP_RSP_CODE_SUCCESS for the final fragment.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer containing the file data and headers.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, no
 *          pull_file operation is pending, or @p rsp_code is @ref BT_FTP_RSP_CODE_SUCCESS
 *          before the final request packet has been received.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_pull_file(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send a remove response from the FTP server.
 *
 *  Responds to a remove (DELETE) request from the remote FTP client.
 *  Called from the remove callback @ref bt_ftp_server_cb::remove.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, or no
 *          remove operation is pending.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_remove(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send a rename/move response from the FTP server.
 *
 *  Responds to a rename/move ACTION request from the remote FTP client.
 *  Called from the rename callback @ref bt_ftp_server_cb::rename.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, or no
 *          rename operation is pending.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_rename(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send a copy response from the FTP server.
 *
 *  Responds to a copy ACTION request from the remote FTP client.
 *  Called from the copy callback @ref bt_ftp_server_cb::copy.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, or no
 *          copy operation is pending.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_copy(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send a set permission response from the FTP server.
 *
 *  Responds to a set-permissions ACTION request from the remote FTP client.
 *  Called from the set_permission callback @ref bt_ftp_server_cb::set_permission.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @retval 0 on success
 *  @retval -EINVAL @p server is NULL, the server is not in the connected OBEX state, or no
 *          set_permission operation is pending.
 *  @retval Other negative error codes from the underlying OBEX layer.
 */
int bt_ftp_server_set_permission(struct bt_ftp_server *server, uint8_t rsp_code,
				 struct net_buf *buf);

/** @brief Allocate a PDU buffer for an FTP server response.
 *
 *  Allocates a buffer for building FTP response packets. Only headroom for the OBEX and
 *  transport headers is reserved; the data size is that of the buffers of @p pool.
 *
 *  Depending on the calling thread, the allocation may wait until a buffer is free. Must not
 *  be called from an ISR.
 *
 *  @param server FTP server object, @ref bt_ftp_server.
 *  @param pool   Buffer pool to allocate from, or NULL for the default pool.
 *
 *  @return Allocated buffer, or NULL on allocation failure.
 */
struct net_buf *bt_ftp_server_create_pdu(struct bt_ftp_server *server, struct net_buf_pool *pool);

/** @} */ /* bt_ftp_server */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_FTP_H_ */
