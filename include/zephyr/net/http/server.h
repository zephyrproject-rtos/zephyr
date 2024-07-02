/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_HTTP_SERVER_H_

/**
 * @file server.h
 *
 * @brief HTTP server API
 *
 * @defgroup http_server HTTP server API
 * @ingroup networking
 * @{
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/http/hpack.h>
#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_HTTP_SERVER)
#define HTTP_SERVER_CLIENT_BUFFER_SIZE   CONFIG_HTTP_SERVER_CLIENT_BUFFER_SIZE
#define HTTP_SERVER_MAX_STREAMS          CONFIG_HTTP_SERVER_MAX_STREAMS
#define HTTP_SERVER_MAX_CONTENT_TYPE_LEN CONFIG_HTTP_SERVER_MAX_CONTENT_TYPE_LENGTH
#define HTTP_SERVER_MAX_URL_LENGTH       CONFIG_HTTP_SERVER_MAX_URL_LENGTH
#else
#define HTTP_SERVER_CLIENT_BUFFER_SIZE   0
#define HTTP_SERVER_MAX_STREAMS          0
#define HTTP_SERVER_MAX_CONTENT_TYPE_LEN 0
#define HTTP_SERVER_MAX_URL_LENGTH       0
#endif

/* Maximum header field name / value length. This is only used to detect Upgrade and
 * websocket header fields and values in the http1 server so the value is quite short.
 */
#define HTTP_SERVER_MAX_HEADER_LEN 32

#define HTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"

/** @endcond */

/**
 *  @brief HTTP server resource type.
 */
enum http_resource_type {
	/** Static resource, cannot be modified on runtime. */
	HTTP_RESOURCE_TYPE_STATIC,

	/** Dynamic resource, server interacts with the application via registered
	 *  @ref http_resource_dynamic_cb_t.
	 */
	HTTP_RESOURCE_TYPE_DYNAMIC,

	/** Websocket resource, application takes control over Websocket connection
	 *  after and upgrade.
	 */
	HTTP_RESOURCE_TYPE_WEBSOCKET,
};

/**
 * @brief Representation of a server resource, common for all resource types.
 */
struct http_resource_detail {
	/** Bitmask of supported HTTP methods (@ref http_method). */
	uint32_t bitmask_of_supported_http_methods;

	/** Resource type. */
	enum http_resource_type type;

	/** Length of the URL path. */
	int path_len;

	/** Content encoding of the resource. */
	const char *content_encoding;

	/** Content type of the resource. */
	const char *content_type;
};

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(NUM_BITS(
	     sizeof(((struct http_resource_detail *)0)->bitmask_of_supported_http_methods))
	     >= (HTTP_METHOD_END_VALUE - 1));
/** @endcond */

/**
 * @brief Representation of a static server resource.
 */
struct http_resource_detail_static {
	/** Common resource details. */
	struct http_resource_detail common;

	/** Content of the static resource. */
	const void *static_data;

	/** Size of the static resource. */
	size_t static_data_len;
};

/** @cond INTERNAL_HIDDEN */
/* Make sure that the common is the first in the struct. */
BUILD_ASSERT(offsetof(struct http_resource_detail_static, common) == 0);
/** @endcond */

struct http_client_ctx;

/** Indicates the status of the currently processed piece of data.  */
enum http_data_status {
	/** Transaction aborted, data incomplete. */
	HTTP_SERVER_DATA_ABORTED = -1,
	/** Transaction incomplete, more data expected. */
	HTTP_SERVER_DATA_MORE = 0,
	/** Final data fragment in current transaction. */
	HTTP_SERVER_DATA_FINAL = 1,
};

/**
 * @typedef http_resource_dynamic_cb_t
 * @brief Callback used when data is received. Data to be sent to client
 *        can be specified.
 *
 * @param client HTTP context information for this client connection.
 * @param status HTTP data status, indicate whether more data is expected or not.
 * @param data_buffer Data received.
 * @param data_len Amount of data received.
 * @param user_data User specified data.
 *
 * @return >0 amount of data to be sent to client, let server to call this
 *            function again when new data is received.
 *          0 nothing to sent to client, close the connection
 *         <0 error, close the connection.
 */
typedef int (*http_resource_dynamic_cb_t)(struct http_client_ctx *client,
					  enum http_data_status status,
					  uint8_t *data_buffer,
					  size_t data_len,
					  void *user_data);

/**
 * @brief Representation of a dynamic server resource.
 */
struct http_resource_detail_dynamic {
	/** Common resource details. */
	struct http_resource_detail common;

	/** Resource callback used by the server to interact with the
	 *  application.
	 */
	http_resource_dynamic_cb_t cb;

	/** Data buffer used to exchanged data between server and the,
	 *  application.
	 */
	uint8_t *data_buffer;

	/** Length of the data in the data buffer. */
	size_t data_buffer_len;

	/** A pointer to the client currently processing resource, used to
	 *  prevent concurrent access to the resource from multiple clients.
	 */
	struct http_client_ctx *holder;

	/** A pointer to the user data registered by the application.  */
	void *user_data;
};

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(offsetof(struct http_resource_detail_dynamic, common) == 0);
/** @endcond */

/**
 * @typedef http_resource_websocket_cb_t
 * @brief Callback used when a Websocket connection is setup. The application
 *        will need to handle all functionality related to the connection like
 *        reading and writing websocket data, and closing the connection.
 *
 * @param ws_socket A socket for the Websocket data.
 * @param user_data User specified data.
 *
 * @return  0 Accepting the connection, HTTP server library will no longer
 *            handle data to/from the socket and it is application responsibility
 *            to send and receive data to/from the supplied socket.
 *         <0 error, close the connection.
 */
typedef int (*http_resource_websocket_cb_t)(int ws_socket,
					    void *user_data);

/** @brief Representation of a websocket server resource */
struct http_resource_detail_websocket {
	/** Common resource details. */
	struct http_resource_detail common;

	/** Websocket socket value */
	int ws_sock;

	/** Resource callback used by the server to interact with the
	 *  application.
	 */
	http_resource_websocket_cb_t cb;

	/** Data buffer used to exchanged data between server and the,
	 *  application.
	 */
	uint8_t *data_buffer;

	/** Length of the data in the data buffer. */
	size_t data_buffer_len;

	/** A pointer to the user data registered by the application.  */
	void *user_data;
};

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(offsetof(struct http_resource_detail_websocket, common) == 0);
/** @endcond */

/** @cond INTERNAL_HIDDEN */

enum http_stream_state {
	HTTP_SERVER_STREAM_IDLE,
	HTTP_SERVER_STREAM_RESERVED_LOCAL,
	HTTP_SERVER_STREAM_RESERVED_REMOTE,
	HTTP_SERVER_STREAM_OPEN,
	HTTP_SERVER_STREAM_HALF_CLOSED_LOCAL,
	HTTP_SERVER_STREAM_HALF_CLOSED_REMOTE,
	HTTP_SERVER_STREAM_CLOSED
};

enum http_server_state {
	HTTP_SERVER_FRAME_HEADER_STATE,
	HTTP_SERVER_PREFACE_STATE,
	HTTP_SERVER_REQUEST_STATE,
	HTTP_SERVER_FRAME_DATA_STATE,
	HTTP_SERVER_FRAME_HEADERS_STATE,
	HTTP_SERVER_FRAME_SETTINGS_STATE,
	HTTP_SERVER_FRAME_PRIORITY_STATE,
	HTTP_SERVER_FRAME_WINDOW_UPDATE_STATE,
	HTTP_SERVER_FRAME_CONTINUATION_STATE,
	HTTP_SERVER_FRAME_PING_STATE,
	HTTP_SERVER_FRAME_RST_STREAM_STATE,
	HTTP_SERVER_FRAME_GOAWAY_STATE,
	HTTP_SERVER_DONE_STATE,
};

enum http1_parser_state {
	HTTP1_INIT_HEADER_STATE,
	HTTP1_WAITING_HEADER_STATE,
	HTTP1_RECEIVING_HEADER_STATE,
	HTTP1_RECEIVED_HEADER_STATE,
	HTTP1_RECEIVING_DATA_STATE,
	HTTP1_MESSAGE_COMPLETE_STATE,
};

#define HTTP_SERVER_INITIAL_WINDOW_SIZE 65536
#define HTTP_SERVER_WS_MAX_SEC_KEY_LEN 32

/** @endcond */

/** @brief HTTP/2 stream representation. */
struct http_stream_ctx {
	int stream_id; /**< Stream identifier. */
	enum http_stream_state stream_state; /**< Stream state. */
	int window_size; /**< Stream-level window size. */
};

/** @brief HTTP/2 frame representation. */
struct http_frame {
	uint32_t length; /**< Frame payload length. */
	uint32_t stream_identifier; /**< Stream ID the frame belongs to. */
	uint8_t type; /**< Frame type. */
	uint8_t flags; /**< Frame flags. */
};

/**
 * @brief Representation of an HTTP client connected to the server.
 */
struct http_client_ctx {
	/** Socket descriptor associated with the server. */
	int fd;

	/** Client data buffer.  */
	unsigned char buffer[HTTP_SERVER_CLIENT_BUFFER_SIZE];

	/** Cursor indicating currently processed byte. */
	unsigned char *cursor;

	/** Data left to process in the buffer. */
	size_t data_len;

	/** Connection-level window size. */
	int window_size;

	/** Server state for the associated client. */
	enum http_server_state server_state;

	/** Currently processed HTTP/2 frame. */
	struct http_frame current_frame;

	/** Currently processed resource detail. */
	struct http_resource_detail *current_detail;

	/** HTTP/2 header parser context. */
	struct http_hpack_header_buf header_field;

	/** HTTP/2 streams context. */
	struct http_stream_ctx streams[HTTP_SERVER_MAX_STREAMS];

	/** HTTP/1 parser configuration. */
	struct http_parser_settings parser_settings;

	/** HTTP/1 parser context. */
	struct http_parser parser;

	/** Request URL. */
	unsigned char url_buffer[HTTP_SERVER_MAX_URL_LENGTH];

	/** Request content type. */
	unsigned char content_type[HTTP_SERVER_MAX_CONTENT_TYPE_LEN];

	/** Temp buffer for currently processed header (HTTP/1 only). */
	unsigned char header_buffer[HTTP_SERVER_MAX_HEADER_LEN];

	/** Request content length. */
	size_t content_len;

	/** Request method. */
	enum http_method method;

	/** HTTP/1 parser state. */
	enum http1_parser_state parser_state;

	/** Length of the payload length in the currently processed request
	 * fragment (HTTP/1 only).
	 */
	int http1_frag_data_len;

	/** Client inactivity timer. The client connection is closed by the
	 *  server when it expires.
	 */
	struct k_work_delayable inactivity_timer;

/** @cond INTERNAL_HIDDEN */
	/** Websocket security key. */
	IF_ENABLED(CONFIG_WEBSOCKET, (uint8_t ws_sec_key[HTTP_SERVER_WS_MAX_SEC_KEY_LEN]));
/** @endcond */

	/** Flag indicating that headers were sent in the reply. */
	bool headers_sent : 1;

	/** Flag indicating that HTTP2 preface was sent. */
	bool preface_sent : 1;

	/** Flag indicating that upgrade header was present in the request. */
	bool has_upgrade_header : 1;

	/** Flag indicating HTTP/2 upgrade takes place. */
	bool http2_upgrade : 1;

	/** Flag indicating Websocket upgrade takes place. */
	bool websocket_upgrade : 1;

	/** Flag indicating Websocket key is being processed. */
	bool websocket_sec_key_next : 1;
};

/** @brief Start the HTTP2 server.
 *
 * The server runs in a background thread. Once started, the server will create
 * a server socket for all HTTP services registered in the system and accept
 * connections from clients (see @ref HTTP_SERVICE_DEFINE).
 */
int http_server_start(void);

/** @brief Stop the HTTP2 server.
 *
 * All server sockets are closed and the server thread is suspended.
 */
int http_server_stop(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
