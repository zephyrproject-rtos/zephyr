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
 * @since 3.7
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/http/hpack.h>
#include <zephyr/net/http/status.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_HTTP_SERVER)
#define HTTP_SERVER_CLIENT_BUFFER_SIZE   CONFIG_HTTP_SERVER_CLIENT_BUFFER_SIZE
#define HTTP_SERVER_MAX_STREAMS          CONFIG_HTTP_SERVER_MAX_STREAMS
#define HTTP_SERVER_MAX_CONTENT_TYPE_LEN CONFIG_HTTP_SERVER_MAX_CONTENT_TYPE_LENGTH
#define HTTP_SERVER_MAX_URL_LENGTH       CONFIG_HTTP_SERVER_MAX_URL_LENGTH
#define HTTP_SERVER_MAX_HEADER_LEN       CONFIG_HTTP_SERVER_MAX_HEADER_LEN
#else
#define HTTP_SERVER_CLIENT_BUFFER_SIZE   0
#define HTTP_SERVER_MAX_STREAMS          0
#define HTTP_SERVER_MAX_CONTENT_TYPE_LEN 0
#define HTTP_SERVER_MAX_URL_LENGTH       0
#define HTTP_SERVER_MAX_HEADER_LEN       0
#endif

#define HTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"

/** @endcond */

/**
 *  @brief HTTP server resource type.
 */
enum http_resource_type {
	/** Static resource, cannot be modified on runtime. */
	HTTP_RESOURCE_TYPE_STATIC,

	/** serves static gzipped files from a filesystem */
	HTTP_RESOURCE_TYPE_STATIC_FS,

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

/**
 * @brief Representation of a static filesystem server resource.
 */
struct http_resource_detail_static_fs {
	/** Common resource details. */
	struct http_resource_detail common;

	/** Path in the local filesystem */
	const char *fs_path;
};

/** @cond INTERNAL_HIDDEN */
/* Make sure that the common is the first in the struct. */
BUILD_ASSERT(offsetof(struct http_resource_detail_static_fs, common) == 0);
/** @endcond */

struct http_content_type {
	const char *extension;
	size_t extension_len;
	const char *content_type;
};

#define HTTP_SERVER_CONTENT_TYPE(_extension, _content_type)                                        \
	const STRUCT_SECTION_ITERABLE(http_content_type, _extension) = {                           \
		.extension = STRINGIFY(_extension),                                                \
		.extension_len = sizeof(STRINGIFY(_extension)) - 1,                                \
		.content_type = _content_type,                                                     \
	};

#define HTTP_SERVER_CONTENT_TYPE_FOREACH(_it) STRUCT_SECTION_FOREACH(http_content_type, _it)

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

/** @brief HTTP header representation */
struct http_header {
	const char *name;  /**< Pointer to header name NULL-terminated string. */
	const char *value; /**< Pointer to header value NULL-terminated string. */
};

/** @brief HTTP response context */
struct http_response_ctx {
	enum http_status status;           /** HTTP status code to include in response */
	const struct http_header *headers; /** Array of HTTP headers */
	size_t header_count;               /** Length of headers array */
	const uint8_t *body;               /** Pointer to body data */
	size_t body_len;                   /** Length of body data */
	bool final_chunk; /** Flag set to true when the application has no more data to send */
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
 * @param response_ctx
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
					  struct http_response_ctx *response_ctx,
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

enum http2_stream_state {
	HTTP2_STREAM_IDLE,
	HTTP2_STREAM_RESERVED_LOCAL,
	HTTP2_STREAM_RESERVED_REMOTE,
	HTTP2_STREAM_OPEN,
	HTTP2_STREAM_HALF_CLOSED_LOCAL,
	HTTP2_STREAM_HALF_CLOSED_REMOTE,
	HTTP2_STREAM_CLOSED
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
	HTTP_SERVER_FRAME_PADDING_STATE,
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
struct http2_stream_ctx {
	int stream_id; /**< Stream identifier. */
	enum http2_stream_state stream_state; /**< Stream state. */
	int window_size; /**< Stream-level window size. */

	/** Flag indicating that headers were sent in the reply. */
	bool headers_sent : 1;

	/** Flag indicating that END_STREAM flag was sent. */
	bool end_stream_sent : 1;
};

/** @brief HTTP/2 frame representation. */
struct http2_frame {
	uint32_t length; /**< Frame payload length. */
	uint32_t stream_identifier; /**< Stream ID the frame belongs to. */
	uint8_t type; /**< Frame type. */
	uint8_t flags; /**< Frame flags. */
	uint8_t padding_len; /**< Frame padding length. */
};

#if defined(CONFIG_HTTP_SERVER_CAPTURE_HEADERS)
/** @brief Status of captured headers */
enum http_header_status {
	HTTP_HEADER_STATUS_OK,      /**< All available headers were successfully captured. */
	HTTP_HEADER_STATUS_DROPPED, /**< One or more headers were dropped due to lack of space. */
};

/** @brief Context for capturing HTTP headers */
struct http_header_capture_ctx {
	/** Buffer for HTTP headers captured for application use */
	unsigned char buffer[CONFIG_HTTP_SERVER_CAPTURE_HEADER_BUFFER_SIZE];

	/** Descriptor of each captured HTTP header */
	struct http_header headers[CONFIG_HTTP_SERVER_CAPTURE_HEADER_COUNT];

	/** Status of captured headers */
	enum http_header_status status;

	/** Number of headers captured */
	size_t count;

	/** Current position in buffer */
	size_t cursor;

	/** The next HTTP header value should be stored */
	bool store_next_value;
};

/** @brief HTTP header name representation */
struct http_header_name {
	const char *name; /**< Pointer to header name NULL-terminated string. */
};
#endif /* defined(CONFIG_HTTP_SERVER_CAPTURE_HEADERS) */

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
	struct http2_frame current_frame;

	/** Currently processed resource detail. */
	struct http_resource_detail *current_detail;

	/** Currently processed stream. */
	struct http2_stream_ctx *current_stream;

	/** HTTP/2 header parser context. */
	struct http_hpack_header_buf header_field;

	/** HTTP/2 streams context. */
	struct http2_stream_ctx streams[HTTP_SERVER_MAX_STREAMS];

	/** HTTP/1 parser configuration. */
	struct http_parser_settings parser_settings;

	/** HTTP/1 parser context. */
	struct http_parser parser;

#if defined(CONFIG_HTTP_SERVER_CAPTURE_HEADERS)
	/** Header capture context */
	struct http_header_capture_ctx header_capture_ctx;
#endif /* defined(CONFIG_HTTP_SERVER_CAPTURE_HEADERS) */

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

	/** Flag indicating that HTTP2 preface was sent. */
	bool preface_sent : 1;

	/** Flag indicating that HTTP1 headers were sent. */
	bool http1_headers_sent : 1;

	/** Flag indicating that upgrade header was present in the request. */
	bool has_upgrade_header : 1;

	/** Flag indicating HTTP/2 upgrade takes place. */
	bool http2_upgrade : 1;

	/** Flag indicating Websocket upgrade takes place. */
	bool websocket_upgrade : 1;

	/** Flag indicating Websocket key is being processed. */
	bool websocket_sec_key_next : 1;

	/** The next frame on the stream is expectd to be a continuation frame. */
	bool expect_continuation : 1;
};

#if defined(CONFIG_HTTP_SERVER_CAPTURE_HEADERS)
/**
 * @brief Register an HTTP request header to be captured by the server
 *
 * @param _id variable name for the header capture instance
 * @param _header header to be captured, as literal string
 */
#define HTTP_SERVER_REGISTER_HEADER_CAPTURE(_id, _header)                                          \
	BUILD_ASSERT(sizeof(_header) <= CONFIG_HTTP_SERVER_MAX_HEADER_LEN,                         \
		     "Header is too long to be captured, try increasing "                          \
		     "CONFIG_HTTP_SERVER_MAX_HEADER_LEN");                                         \
	static const char *const _id##_str = _header;                                              \
	static const STRUCT_SECTION_ITERABLE(http_header_name, _id) = {                            \
		.name = _id##_str,                                                                 \
	}
#endif /* defined(CONFIG_HTTP_SERVER_CAPTURE_HEADERS) */

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
