/** @file
 *  @brief Bluetooth File Transfer Profile handling.
 */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_FTP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_FTP_H_

/**
 * @brief File Transfer Profile (FTP)
 * @defgroup bt_ftp File Transfer Profile (FTP)
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
		BT_UUID_128_ENCODE(0xF9EC7BC4, 0x953C, 0x11D2, 0x984E, 0x525400DC9E09))

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
 *  All callbacks are optional. The structure must remain valid and constant
 *  for the entire lifetime of the FTP client object.
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
	 *  All pending operations are cancelled.
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
	 *  All pending operations are cancelled.
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
	 *  Called when OBEX ABORT response is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code.
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

	/** @brief Delete response callback.
	 *
	 *  Called when Delete response is received.
	 *
	 *  @param client   FTP client object.
	 *  @param rsp_code Response code @ref bt_ftp_rsp_code.
	 *  @param buf      Buffer containing response headers.
	 */
	void (*delete)(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf);

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
	struct bt_ftp_client_cb *_cb;

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
 *  @note The @p client object must be zeroed by the caller before this call.
 *
 *  @param conn    Bluetooth connection object.
 *  @param client  FTP client object, @ref bt_ftp_client.
 *  @param cb      Callback table, @ref bt_ftp_client_cb.
 *  @param channel RFCOMM server channel (from SDP discovery).
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_rfcomm_connect(struct bt_conn *conn, struct bt_ftp_client *client,
				 struct bt_ftp_client_cb *cb, uint8_t channel);

/** @brief Disconnect FTP client RFCOMM transport.
 *
 *  Closes the RFCOMM transport connection.
 *  On success, the rfcomm_disconnected callback @ref bt_ftp_client_cb::rfcomm_disconnected will
 *  be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_rfcomm_disconnect(struct bt_ftp_client *client);

/** @brief Connect FTP client over L2CAP (GOEP v2).
 *
 *  Initiates L2CAP transport connection to a remote FTP server.
 *  On success, the l2cap_connected callback @ref bt_ftp_client_cb::l2cap_connected will be
 *  called. The caller must then issue an OBEX CONNECT via @ref bt_ftp_client_connect.
 *
 *  @note The @p client object must be zeroed by the caller before this call.
 *
 *  @param conn   Bluetooth connection object.
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param cb     Callback table, @ref bt_ftp_client_cb.
 *  @param psm    L2CAP PSM (from SDP discovery).
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_l2cap_connect(struct bt_conn *conn, struct bt_ftp_client *client,
				struct bt_ftp_client_cb *cb, uint16_t psm);

/** @brief Disconnect FTP client L2CAP transport.
 *
 *  Closes the L2CAP transport connection.
 *  On success, the l2cap_disconnected callback @ref bt_ftp_client_cb::l2cap_disconnected will be
 *  called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *
 *  @return 0 on success, negative error code on failure.
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
 *  @param buf    Buffer containing connect headers (target UUID, etc.).
 *                If NULL, a default connect request is sent.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_connect(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send an OBEX DISCONNECT request to the FTP server.
 *
 *  Initiates OBEX session termination.
 *  Once OBEX disconnect response received, the disconnect callback @ref
 *  bt_ftp_client_cb::disconnect will be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer containing disconnect headers (connection ID, etc.).
 *                If NULL, a default disconnect request is sent.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_disconnect(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send an OBEX ABORT request to the FTP server.
 *
 *  Aborts the currently ongoing operation.
 *  Once OBEX abort response received, the abort callback @ref bt_ftp_client_cb::abort will be
 *  called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param buf    Buffer containing abort headers (connection ID, etc.).
 *                If NULL, a default abort request is sent.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_abort(struct bt_ftp_client *client, struct net_buf *buf);

/** @brief Send a SetPath (set folder) request to the FTP server.
 *
 *  Navigates the folder hierarchy on the FTP server.
 *  Once set_folder response received, the set_folder callback @ref
 *  bt_ftp_client_cb::set_folder will be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param flags  Navigation flags, @ref bt_ftp_set_folder_flags.
 *  @param buf    Buffer containing folder name and headers.
 *                Must include connection ID.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
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
 *                Must include connection ID and type header for initial request.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
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
 *                Must include end-of-body header when final is true.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
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
 *                Must include connection ID and name header for initial request.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_pull_file(struct bt_ftp_client *client, bool final, struct net_buf *buf);

/** @brief Send a DELETE request to the FTP server.
 *
 *  Deletes the object identified by the Name header. This is implemented as
 *  a PUT with an empty body.
 *  Once delete response received, the delete callback @ref bt_ftp_client_cb::delete will be
 *  called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param final  True if this is the final packet, false if more data follows.
 *  @param buf    Buffer containing the Name header of the object to delete.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_delete(struct bt_ftp_client *client, bool final, struct net_buf *buf);

/** @brief Send a RENAME/MOVE action request to the FTP server.
 *
 *  Renames or moves the object identified by the Name header to the path given
 *  in the DestName header.
 *  Once rename response received, the rename callback @ref bt_ftp_client_cb::rename will be
 *  called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param final  True if this is the final packet, false if more data follows.
 *  @param buf    Buffer containing the Name (source) and DestName (destination) headers.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_rename(struct bt_ftp_client *client, bool final, struct net_buf *buf);

/** @brief Send a COPY action request to the FTP server.
 *
 *  Copies the object identified by the Name header to the path given in the
 *  DestName header.
 *  Once copy response received, the copy callback @ref bt_ftp_client_cb::copy will be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param final  True if this is the final packet, false if more data follows.
 *  @param buf    Buffer containing the Name (source) and DestName (destination) headers.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_copy(struct bt_ftp_client *client, bool final, struct net_buf *buf);

/** @brief Send a SET PERMISSIONS action request to the FTP server.
 *
 *  Changes the permissions of the object identified by the Name header.
 *  Once set_permission response received, the set_permission callback @ref
 *  bt_ftp_client_cb::set_permission will be called.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param final  True if this is the final packet, false if more data follows.
 *  @param buf    Buffer containing the Name header and the Permissions header.
 *                If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_client_set_permission(struct bt_ftp_client *client, bool final, struct net_buf *buf);

/** @brief Allocate a PDU buffer for an FTP client operation.
 *
 *  Allocates a buffer for building FTP request packets.
 *  The buffer is sized appropriately for the negotiated MTU.
 *
 *  @param client FTP client object, @ref bt_ftp_client.
 *  @param pool   Buffer pool to allocate from (NULL for default pool).
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
 *  All callbacks are optional. The structure must remain valid and constant
 *  for the entire lifetime of the FTP server object.
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
	 *  called multiple times for a fragmented file upload.
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

	/** @brief Delete request callback.
	 *
	 *  Called when the client sends a DELETE (PUT with empty body) request.
	 *  The server must reply via @ref bt_ftp_server_delete.
	 *
	 *  @param server FTP server object.
	 *  @param final  True if this is the final packet from the client.
	 *  @param buf    Buffer containing the Name header.
	 */
	void (*delete)(struct bt_ftp_server *server, bool final, struct net_buf *buf);

	/** @brief Rename/Move action request callback.
	 *
	 *  Called when the client sends a rename ACTION request. The server
	 *  must reply via @ref bt_ftp_server_rename.
	 *
	 *  @param server FTP server object.
	 *  @param final  True if this is the final packet from the client.
	 *  @param buf    Buffer containing the Name and DestName headers.
	 */
	void (*rename)(struct bt_ftp_server *server, bool final, struct net_buf *buf);

	/** @brief Copy action request callback.
	 *
	 *  Called when the client sends a copy ACTION request. The server must
	 *  reply via @ref bt_ftp_server_copy.
	 *
	 *  @param server FTP server object.
	 *  @param final  True if this is the final packet from the client.
	 *  @param buf    Buffer containing the Name and DestName headers.
	 */
	void (*copy)(struct bt_ftp_server *server, bool final, struct net_buf *buf);

	/** @brief Set permission action request callback.
	 *
	 *  Called when the client sends a set-permissions ACTION request. The
	 *  server must reply via @ref bt_ftp_server_set_permission.
	 *
	 *  @param server FTP server object.
	 *  @param final  True if this is the final packet from the client.
	 *  @param buf    Buffer containing the Name and Permissions headers.
	 */
	void (*set_permission)(struct bt_ftp_server *server, bool final, struct net_buf *buf);
};

/** @brief FTP server structure.
 *
 *  Must be zeroed by the caller before use. Must remain address-stable for
 *  the lifetime of the transport connection.
 */
struct bt_ftp_server {
	/** @internal Callbacks (set via @ref bt_ftp_server_register). */
	struct bt_ftp_server_cb *_cb;

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
	 *  Set when the first packet of a PUT/GET/ACTION operation is received.
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
	 *  allocate and initialize an @ref bt_ftp_server instance and return it
	 *  through @p ftp_server.
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
	 *  allocate and initialize an @ref bt_ftp_server instance and return it
	 *  through @p ftp_server.
	 *
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
 *  @param server FTP server object, @ref bt_ftp_server.
 *  @param cb     Callback table, @ref bt_ftp_server_cb.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_server_register(struct bt_ftp_server *server, struct bt_ftp_server_cb *cb);

/** @brief Register an FTP RFCOMM server.
 *
 *  Registers the server to listen for incoming RFCOMM connections. The
 *  allocated channel is stored in @c server->server.rfcomm.channel and should
 *  be advertised via SDP.
 *
 *  @param server RFCOMM server registration structure, @ref bt_ftp_server_rfcomm.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_server_rfcomm_register(struct bt_ftp_server_rfcomm *server);

/** @brief Register an FTP L2CAP server.
 *
 *  Registers the server to listen for incoming L2CAP connections. The
 *  allocated PSM is stored in @c server->server.l2cap.psm and should be
 *  advertised via SDP.
 *
 *  @param server L2CAP server registration structure, @ref bt_ftp_server_l2cap.
 *
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_server_pull_file(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

/** @brief Send a delete response from the FTP server.
 *
 *  Responds to a DELETE request from the remote FTP client.
 *  Called from the delete callback @ref bt_ftp_server_cb::delete.
 *
 *  @param server   FTP server object.
 *  @param rsp_code Response code, @ref bt_ftp_rsp_code.
 *  @param buf      Buffer with response headers, or NULL.
 *                  If success returned, the function has taken the ownership of @p buf.
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_server_delete(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf);

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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
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
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_server_set_permission(struct bt_ftp_server *server, uint8_t rsp_code,
				 struct net_buf *buf);

/** @brief Allocate a PDU buffer for an FTP server response.
 *
 *  Allocates a buffer for building FTP response packets.
 *  The buffer is sized appropriately for the negotiated MTU.
 *
 *  @param server FTP server object, @ref bt_ftp_server.
 *  @param pool   Buffer pool to allocate from (NULL for default pool).
 *
 *  @return Allocated buffer, or NULL on allocation failure.
 */
struct net_buf *bt_ftp_server_create_pdu(struct bt_ftp_server *server, struct net_buf_pool *pool);

/** @} */ /* bt_ftp_server */

/** @brief Calculate authentication nonce for FTP challenge.
 *
 *  Generates a nonce value used in OBEX authentication challenges.
 *  The nonce is used to prevent replay attacks during authentication.
 *
 *  @param pwd   Password string used for authentication (null-terminated).
 *  @param nonce Output buffer to store the generated nonce
 *               (@ref BT_OBEX_CHALLENGE_TAG_NONCE_LEN bytes).
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_calculate_nonce(const uint8_t *pwd, uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN]);

/** @brief Calculate response digest for FTP authentication.
 *
 *  Computes the MD5 digest for an authentication response based on the password
 *  and received nonce. This digest is sent back to prove knowledge of the password
 *  without transmitting the password itself.
 *
 *  @param pwd        Password string used for authentication (null-terminated).
 *  @param nonce      Nonce value received in the authentication challenge
 *                    (@ref BT_OBEX_CHALLENGE_TAG_NONCE_LEN bytes).
 *  @param rsp_digest Output buffer to store the calculated response digest
 *                    (@ref BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN bytes).
 *
 *  @return 0 on success, negative error code on failure.
 */
int bt_ftp_calculate_rsp_digest(const uint8_t *pwd,
				const uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN],
				uint8_t rsp_digest[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN]);

/** @brief Verify authentication response for FTP.
 *
 *  Verifies that the received response digest matches the expected value based on
 *  the password and nonce. Used by the authenticating party to validate the
 *  authentication response.
 *
 *  @param nonce      Nonce value that was sent in the authentication challenge
 *                    (@ref BT_OBEX_CHALLENGE_TAG_NONCE_LEN bytes).
 *  @param rsp_digest Response digest received from the authenticating peer
 *                    (@ref BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN bytes).
 *  @param pwd        Password string used for authentication (null-terminated).
 *
 *  @return 0 if authentication is successful, negative error code on failure.
 */
int bt_ftp_verify_authentication(uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN],
				 uint8_t rsp_digest[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN],
				 const uint8_t *pwd);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_FTP_H_ */
