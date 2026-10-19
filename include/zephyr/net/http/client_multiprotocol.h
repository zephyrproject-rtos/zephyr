/** @file
 * @brief Multiprotocol HTTP client API
 *
 * An API for applications to send HTTP requests.
 */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_CLIENT_MULTIPROTOCOL_H_
#define ZEPHYR_INCLUDE_NET_HTTP_CLIENT_MULTIPROTOCOL_H_

/**
 * @brief Multiprotocol HTTP client API
 *
 * An HTTP client API designed to carry HTTP/1.1, HTTP/2 and HTTP/3 over
 * pluggable transports. Only HTTP/2 is implemented so far.
 *
 * @defgroup http_client_multiprotocol Multiprotocol HTTP client API
 * @since 4.6
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/net/http/frame.h>
#include <zephyr/net/http/hpack.h>
#include <zephyr/net/http/method.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_HTTP_CLIENT_MULTIPROTOCOL)
#define HTTP_CLIENT_MAX_STREAMS CONFIG_HTTP_CLIENT_MAX_STREAMS
#define HTTP_CLIENT_HEADERS_BUFFER_SIZE CONFIG_HTTP_CLIENT_HEADERS_BUFFER_SIZE
#define HTTP_CLIENT_RX_BUFFER_SIZE CONFIG_HTTP_CLIENT_RX_BUFFER_SIZE
#define HTTP_CLIENT_TX_BUFFER_SIZE CONFIG_HTTP_CLIENT_TX_BUFFER_SIZE
#define HTTP_CLIENT_MAX_CONTENT_TYPE_LEN CONFIG_HTTP_CLIENT_MAX_CONTENT_TYPE_LENGTH
#else
#define HTTP_CLIENT_MAX_STREAMS 1
#define HTTP_CLIENT_HEADERS_BUFFER_SIZE 1
#define HTTP_CLIENT_RX_BUFFER_SIZE 1
#define HTTP_CLIENT_TX_BUFFER_SIZE 1
#define HTTP_CLIENT_MAX_CONTENT_TYPE_LEN 1
#endif

/** @endcond */

struct http_client;
struct http_client_request;
struct http_client_response;

/** @cond INTERNAL_HIDDEN */

/* State kept by the protocol layers inside the public contexts below. */

/** PING frames whose acknowledgment is held back behind a request frame;
 *  beyond that, only the most recent ones are acknowledged.
 */
#define HTTP2_CLIENT_DEFERRED_PINGS 4

/** HTTP/2 stream state. */
struct http2_client_stream {
	/** Stream-level receive window. */
	int window_size;

	/** Stream-level send window, the flow control credit granted by the peer. */
	int send_window;

	/** Body bytes received so far, checked against content-length. */
	size_t body_received;

	/** Whether the header block being received holds :status already. */
	bool status_seen;

	/** Whether the header block being received holds a regular field already. */
	bool regular_field_seen;

	/** Stream window credit granted to the peer but not sent yet, see
	 *  http2_client_parser::deferred.
	 */
	uint32_t window_update_pending;
};

/** Control frames held back while a request frame is on its way out, sent by
 *  that sender once its frame is complete.
 */
struct http2_client_deferred {
	/** Number of SETTINGS frames to acknowledge. */
	uint8_t settings_acks;

	/** Number of PINGs to acknowledge. */
	uint8_t ping_count;

	/** Payloads of the PINGs to acknowledge, in the order received. */
	uint8_t ping_data[HTTP2_CLIENT_DEFERRED_PINGS][HTTP2_PING_FRAME_LEN];

	/** Connection window credit granted to the peer but not sent yet. */
	uint32_t conn_window;

	/** Streams to reset, with their error codes. */
	struct {
		uint32_t id;
		enum http2_error error;
	} rst[HTTP_CLIENT_MAX_STREAMS];

	/** Number of entries in rst. */
	uint8_t rst_count;
};

/** HTTP/2 incremental frame parser state. */
enum http2_client_parser_state {
	/** Waiting for a complete 9-byte frame header. */
	HTTP2_CLIENT_STATE_FRAME_HEADER,

	/** Processing a HEADERS frame header block. */
	HTTP2_CLIENT_STATE_HEADERS,

	/** Processing a CONTINUATION frame header block. */
	HTTP2_CLIENT_STATE_CONTINUATION,

	/** Processing a DATA frame payload. */
	HTTP2_CLIENT_STATE_DATA,

	/** Processing a SETTINGS frame payload. */
	HTTP2_CLIENT_STATE_SETTINGS,

	/** Processing a WINDOW_UPDATE frame payload. */
	HTTP2_CLIENT_STATE_WINDOW_UPDATE,

	/** Processing a RST_STREAM frame payload. */
	HTTP2_CLIENT_STATE_RST_STREAM,

	/** Processing a GOAWAY frame payload. */
	HTTP2_CLIENT_STATE_GOAWAY,

	/** Processing a PING frame payload. */
	HTTP2_CLIENT_STATE_PING,

	/** Processing a PRIORITY frame payload. */
	HTTP2_CLIENT_STATE_PRIORITY,

	/** Discarding the payload of an unsupported or oversized frame. */
	HTTP2_CLIENT_STATE_SKIP,
};

/** HTTP/2 client incremental parser context. */
struct http2_client_parser {
	/** Current parser state. */
	enum http2_client_parser_state state;

	/** Current frame. Its length counts the payload bytes still pending. */
	struct http2_frame frame;

	/** Whether a CONTINUATION frame is expected to follow. */
	bool expect_continuation;

	/** Stream of the header block being received. */
	uint32_t headers_stream_id;

	/** Whether the header block being received already holds a field. */
	bool block_field_seen;

	/** Error code to send in GOAWAY when the connection is failed. */
	enum http2_error goaway_error;

	/** Whether the HEADERS frame being assembled carried END_STREAM. */
	bool headers_end_stream;

	/** HPACK decoding context for the header field being parsed. */
	struct http_hpack_header_buf header_field;

	/** Write cursor (offset from the end) for response header strings packed
	 *  into the stream headers buffer while a header block is decoded.
	 */
	size_t rx_headers_str_top;

	/** Connection-level receive window. */
	int window_size;

	/** Connection-level send window, the flow control credit granted by the peer. */
	int send_window;

	/** Initial send window of a new stream, as set by the peer. */
	uint32_t peer_initial_window;

	/** Largest frame payload the peer accepts. */
	uint32_t peer_max_frame_size;

	/** Number of concurrent streams the peer allows. */
	uint32_t peer_max_streams;

	/** Whether the peer announced that it accepts no new streams. */
	bool going_away;

	/** The highest stream ID opened during the session. */
	uint32_t last_stream_id;

	/** Whether the SETTINGS frame opening the server's preface was received. */
	bool preface_received;

	/** Whether the server acknowledged the client's SETTINGS. */
	bool settings_acked;

	/** Number of SETTINGS_HEADER_TABLE_SIZE changes received from the peer.
	 *  A header block sent while it differs from table_size_acked opens
	 *  with a dynamic table size update.
	 */
	uint32_t table_size_changes;

	/** Value of table_size_changes acknowledged by the last size update sent. */
	uint32_t table_size_acked;

	/** Control frames waiting for the sender holding the TX lock. */
	struct http2_client_deferred deferred;
};

/** @endcond */

/** HTTP protocol version */
enum http_client_version {
	HTTP_CLIENT_VERSION_1_1, /**< HTTP/1.1 */
	HTTP_CLIENT_VERSION_2, /**< HTTP/2 */
	HTTP_CLIENT_VERSION_3, /**< HTTP/3 */
};

/** State of the HTTP exchange passed to the callback */
enum http_client_transaction_status {
	HTTP_CLIENT_TRANSACTION_ABORTED = -1, /**< Transaction aborted, data incomplete */
	HTTP_CLIENT_RESPONSE_DATA_MORE = 0,  /**< Transaction incomplete, more data to come */
	HTTP_CLIENT_RESPONSE_DATA_FINAL = 1, /**< Transaction complete, end of data */
};

/** An HTTP header field, a name and value pair. */
struct http_client_header {
	const char *name;  /**< Pointer to header name NULL-terminated string. */
	const char *value; /**< Pointer to header value NULL-terminated string. */
};

/** HTTP transport API */
struct http_client_transport_ops {
	/** Whether the transport is secured with TLS. Selects the URL scheme
	 *  (http or https) used in requests.
	 */
	bool secure;

	/** @brief Initialize transport context
	 *
	 * @param transport_ctx A pointer to the transport context to initialize
	 */
	void (*init)(void *transport_ctx);

	/** @brief Connect the transport layer
	 *
	 * The function is expected to establish the connection. For an
	 * asynchronous session (@ref http_client_session::async) it also sets
	 * up asynchronous reception and calls @ref http_client_received_data()
	 * whenever data is available. A synchronous session receives on the
	 * calling thread through wait() and recv() instead.
	 *
	 * @param client HTTP client instance
	 *
	 * @return 0 on success, a negative errno code otherwise.
	 */
	int (*connect)(struct http_client *client);

	/** @brief Send data over the transport layer
	 *
	 * The function is expected to send all of the data requested, or fail.
	 * The sending is always blocking.
	 *
	 * @param client HTTP client instance
	 * @param stream Transport stream to send on, NULL for the connection
	 *               itself. A single-stream transport (TCP, TLS) is only
	 *               ever asked to send on the connection.
	 * @param data Data to send
	 * @param len Length of the data to send
	 * @param end_stream Whether this is the last data sent on the stream.
	 *                   Ignored by single-stream transports.
	 *
	 * @return 0 on success, a negative errno code otherwise.
	 */
	int (*send)(struct http_client *client, void *stream, const uint8_t *data, size_t len,
		    bool end_stream);

	/** @brief Receive data from the transport layer
	 *
	 * The function is expected to be non-blocking, returning immediately
	 * with whatever data is currently available. If no data is available,
	 * the function should return -EAGAIN.
	 *
	 * A multi-stream transport receives from the stream requested in
	 * @p stream, or from any stream when NULL is requested, and reports
	 * the stream the data came from in the same parameter. It may return
	 * -EAGAIN when the requested stream has no data even if other streams
	 * do. A single-stream transport ignores the request and reports NULL.
	 *
	 * @param client HTTP client instance
	 * @param stream Transport stream to receive from on entry, the stream
	 *               the data came from on return
	 * @param buf Buffer for the received data
	 * @param bufsize Size of the receive buffer
	 * @param end_stream Set when the peer ended the stream after the
	 *                   returned data. Never set by single-stream
	 *                   transports, which report 0 bytes instead.
	 *
	 * @return Number of bytes received, 0 if transport was closed by the peer,
	 * a negative errno code otherwise.
	 */
	int (*recv)(struct http_client *client, void **stream, uint8_t *buf, size_t bufsize,
		    bool *end_stream);

	/** @brief Wait for data to become available
	 *
	 * Used by synchronous sessions, which receive on the calling thread.
	 *
	 * @param client HTTP client instance
	 * @param timeout Maximum time to wait
	 *
	 * @return 0 when data is available or the peer closed the transport,
	 * -EAGAIN when the timeout expired, a negative errno code on a
	 * transport error.
	 */
	int (*wait)(struct http_client *client, k_timeout_t timeout);

	/** @brief Stop reporting reception events
	 *
	 * Called when the session ends, before @ref close, which may be left
	 * to a thread still sending on the transport. From then on nothing
	 * received is read. May be NULL.
	 *
	 * @param client HTTP client instance
	 */
	void (*stop)(struct http_client *client);

	/** @brief Disconnect the transport layer and release resources
	 *
	 * Called with the client lock released, once the session is closed
	 * and nothing is sent any more. A transport with a receive context of
	 * its own waits here for a notification already dispatched for the
	 * client to return, so that the client can be reinitialized or freed
	 * once http_client_disconnect() returns.
	 *
	 * @param client HTTP client instance
	 *
	 * @return 0 on success, a negative errno code otherwise.
	 */
	int (*close)(struct http_client *client);
};

/** @brief Notify HTTP client about a transport reception event
 *
 * Should be called by the transport implementation when new data is available
 * to read, or when a transport-level error condition occurred.
 *
 * @param client HTTP client instance
 * @param status Reception status. 0 indicates that new data is available to
 *               read. A negative errno code indicates a transport-level error,
 *               in which case the affected transaction(s) are aborted and no
 *               data is read.
 */
void http_client_received_data(struct http_client *client, int status);

/**
 * @typedef http_client_socket_create_cb_t
 * @brief A function to offload socket creation for the transport layer
 *
 * The socket belongs to the transport from then on: it is configured,
 * connected and eventually closed by it.
 *
 * @param client HTTP client instance
 * @param af Address family
 * @param proto Protocol
 *
 * @return Socket file descriptor on success, a negative errno code otherwise.
 */
typedef int (*http_client_socket_create_cb_t)(struct http_client *client, int af,
					      int proto);

/**
 * @typedef http_client_payload_cb_t
 * @brief Callback for providing payload for the HTTP request.
 *
 * An optional callback for providing a payload for the HTTP client request. If
 * set in @ref http_client_request, the HTTP client library will call this
 * callback when sending the request.
 *
 * When called, the library provides the application with the current payload offset
 * for the transfer. In return, the application sets the payload pointer, payload size
 * and information whether more data is expected. The library will keep calling the
 * callback until the @p more parameter is set to false.
 *
 * The final call may report a zero @p len, in case the application does not know
 * upfront which chunk is the last one. A zero @p len is only valid when @p more
 * is set to false.
 *
 * @param offset Payload offset
 * @param payload A pointer for the buffer containing the payload
 * @param len A pointer to the payload length
 * @param more A pointer to the flag indicating whether callback should be called again
 * @param user_data User provided context specified in http_client_send_request()
 *
 * @return Zero on success, a negative error code to abort transaction.
 */
typedef int (*http_client_payload_cb_t)(size_t offset, const uint8_t **payload,
					size_t *len, bool *more, void *user_data);

/**
 * @typedef http_client_headers_cb_t
 * @brief Callback for providing additional headers for the HTTP request.
 *
 * An optional callback for providing additional headers for the HTTP client request.
 * If set in @ref http_client_request, the HTTP client library will call this
 * callback when sending the request.
 *
 * When called, the application sets the pointer to an array of headers, the number
 * of entries in it and whether more headers are to follow. The library will keep
 * calling the callback until the @p more parameter is set to false. The array must
 * stay valid until the next call or the completion of the request.
 *
 * The fields are sent as they are handed out. An invalid field, or one too large
 * for the transmit buffer, found after part of the request is already on the wire
 * cannot be recovered from: the session is closed, this request fails with the
 * cause and the other requests in flight with -ECONNABORTED.
 *
 * A Host header is only honoured in @ref http_client_request::header_fields,
 * as the authority is settled before the callback runs; one handed out here
 * fails the request.
 *
 * @param headers A pointer for the array containing additional headers
 * @param count Number of entries in the header array
 * @param more A pointer to the flag indicating whether callback should be called again
 * @param user_data User provided context specified in http_client_send_request()
 *
 * @return Zero on success, a negative error code to abort transaction.
 */
typedef int (*http_client_headers_cb_t)(const struct http_client_header **headers,
					size_t *count, bool *more, void *user_data);

/** TCP transport context for the HTTP client */
struct http_client_transport_tcp_ctx {
	/** Socket file descriptor in use, managed by the transport. */
	int sock;

	/** Name of the interface that the HTTP client instance should be bound to.
	 *  Leave as NULL if not specified.
	 */
	const char *if_name;

	/** Optional callback function for socket creation. If NULL, the
	 *  transport creates the socket itself: a TCP socket, or a TLS socket
	 *  when used by the TLS transport.
	 */
	http_client_socket_create_cb_t socket_create;
};

/** TLS configuration for the HTTP client */
struct http_client_transport_tls_config {
	/** The number of entries in the cipher list. */
	uint32_t cipher_count;

	/** The list of ciphers to be used for the session.
	 *  Leave as NULL to use the default ciphers.
	 */
	const int *cipher_list;

	/** The number of entries in the sec tag list. */
	uint32_t sec_tag_count;

	/** The list of security tags to be used for the session. */
	const sec_tag_t *sec_tag_list;

	/** Peer hostname, sent as the TLS server name and used for certificate
	 *  verification. NULL to use the session host when that is a domain
	 *  name.
	 */
	const char *hostname;

	/** Indicates the preference for copying certificates to the heap. */
	int cert_nocopy;

	/** Set socket to use native TLS (used with socket offloading). */
	bool set_native_tls;
};

/** TLS transport context for the HTTP client */
struct http_client_transport_tls_ctx {
	/** Underlying TCP transport context. A socket_create callback set
	 *  there must return a TLS socket.
	 */
	struct http_client_transport_tcp_ctx tcp;

	/** TLS configuration. */
	struct http_client_transport_tls_config config;
};

/**
 * @typedef http_client_response_cb_t
 * @brief Callback used when data is received from the server or an error occurred.
 *
 * A response is reported as a sequence of callbacks: one for each header block,
 * carrying the decoded headers and no body data (several when the block does
 * not fit in the headers buffer), then one for each chunk of body data,
 * carrying no headers. Header blocks are the response headers,
 * preceded by any 1xx interim response and followed by trailers if the server
 * sends them. The last callback carries @ref HTTP_CLIENT_RESPONSE_DATA_FINAL.
 * Pointers in @p response are only valid for the duration of the callback.
 *
 * In a synchronous session the callback runs on the thread that called
 * http_client_send_request(). It may call http_client_disconnect(), which
 * ends the session at once: a transaction whose response is not complete yet
 * is reported as aborted with -ECONNABORTED from inside the running callback,
 * and its http_client_send_request() call returns that error. In an
 * asynchronous session the callback runs on the transport's receive context
 * or, on a timeout, the client's own work queue, and must neither block nor
 * call back into the client API.
 *
 * A negative return value aborts the transaction, which is then reported once
 * more, with @ref HTTP_CLIENT_TRANSACTION_ABORTED and that value as the error.
 * This holds for the final chunk as well, although nothing is left to abort
 * on the wire by then.
 *
 * @param client HTTP client context
 * @param request HTTP request the response belongs to
 * @param response HTTP response data. On @ref HTTP_CLIENT_TRANSACTION_ABORTED
 *                 it holds whatever was received before the abort, with the
 *                 reason in @ref http_client_response::error.
 * @param status Transaction status (incomplete/complete/aborted).
 * @param user_data User specified data specified in http_client_send_request()
 *
 * @return 0 if the HTTP client should proceed with transaction,
 *         <0 if the HTTP client should abort the transaction.
 */
typedef int (*http_client_response_cb_t)(struct http_client *client,
					 const struct http_client_request *request,
					 struct http_client_response *response,
					 enum http_client_transaction_status status,
					 void *user_data);

/** HTTP response context. */
struct http_client_response {
	/** HTTP status code, 0 until the response headers are received. */
	uint16_t status_code;

	/** Reason for an aborted transaction (negative errno), 0 otherwise. */
	int error;

	/** Content type, truncated to fit. Empty if not available. */
	char content_type[HTTP_CLIENT_MAX_CONTENT_TYPE_LEN];

	/** Content encoding, truncated to fit. Empty if not available. */
	char content_encoding[HTTP_CLIENT_MAX_CONTENT_TYPE_LEN];

	/** Content length, valid when content_len_present is set. */
	size_t content_len;

	/** Whether the response carried a Content-Length header. */
	bool content_len_present;

	/** Headers of the header block being reported. Set only in the callback
	 *  reporting that block; NULL in body data callbacks, or when
	 *  skip_response_headers was set in the request.
	 */
	const struct http_client_header *headers;

	/** Length of headers array */
	size_t header_count;

	/** Pointer to body data */
	const uint8_t *body;

	/** Body length (in current chunk) */
	size_t body_len;
};

/** HTTP request context */
struct http_client_request {
	/** The HTTP method: GET, HEAD, OPTIONS, POST, ... */
	enum http_method method;

	/** User supplied callback function to call when response is received */
	http_client_response_cb_t response_cb;

	/** Do not report response headers to the application. The status code
	 *  and the dedicated response fields are still filled in.
	 */
	bool skip_response_headers;

	/** The request target in origin form, that is the path and query,
	 *  for example /index.html, with no fragment. A missing leading slash
	 *  is added. An OPTIONS request may target the server as a whole with
	 *  "*". An absolute URL is rejected, and so is the CONNECT method.
	 */
	const char *url;

	/** The value of the Content-Type header field, may be NULL */
	const char *content_type_value;

	/** User supplied callback function to call when payload needs to be sent.
	 * This can be NULL in which case the @ref http_client_request::payload
	 * field is used.
	 */
	http_client_payload_cb_t payload_cb;

	/** Payload, may be NULL */
	const uint8_t *payload;

	/** Length of the payload provided in @ref http_client_request::payload */
	size_t payload_len;

	/** User supplied callback function to call when optional headers need
	 * to be sent. Its header fields follow the ones given in
	 * @ref http_client_request::header_fields. May be NULL.
	 */
	http_client_headers_cb_t headers_cb;

	/** The HTTP header fields (application specific). Should point to an
	 * array of application-specific HTTP headers. May include Content-Type
	 * header, but in such case @ref http_client_request::content_type_value
	 * field should be NULL.
	 */
	const struct http_client_header *header_fields;

	/** Number of headers provided in @ref http_client_request::header_fields */
	size_t headers_count;
};

/** HTTP stream context */
struct http_client_stream {
	/** Whether the stream is in use. */
	bool is_used;

	/** Protocol stream identifier, as assigned by the protocol layer. */
	uint64_t stream_id;

	/** Transport stream carrying this stream, NULL on a single-stream
	 *  transport.
	 */
	void *transport_stream;

	/** Protocol-specific stream state. */
	union {
		/** HTTP/2 stream state. */
		struct http2_client_stream http2;
	} proto;

	/** Buffer backing the response header array and strings delivered to the
	 *  application. Aligned so that a @ref http_client_header array can be
	 *  carved from its start.
	 */
	uint8_t headers_buf[HTTP_CLIENT_HEADERS_BUFFER_SIZE] __aligned(sizeof(void *));

	/** Request the stream carries, NULL when none. */
	const struct http_client_request *request;

	/** Response data associated with the stream */
	struct http_client_response response;

	/** Response timeout for the stream (absolute time in milliseconds) */
	int64_t timeout;

	/** Result of the completed transaction (0, or a negative errno). */
	int result;

	/** Whether the transaction has completed. */
	bool completed;

	/** Whether the request is still being sent. */
	bool sending;

	/** Thread that took the stream in http_client_send_request(). NULL
	 *  once the request is on its way out on an asynchronous session;
	 *  kept until the stream is released on a synchronous one.
	 */
	k_tid_t caller;

	/** Whether the headers of the final response were received. */
	bool headers_received;

	/** User data */
	void *user_data;
};

/** HTTP client session state */
enum http_client_session_state {
	HTTP_CLIENT_SESSION_CLOSED, /**< Session closed */
	HTTP_CLIENT_SESSION_OPEN, /**< Session open */
	HTTP_CLIENT_SESSION_CLOSING, /**< Session being torn down */
};

/** HTTP session context */
struct http_client_session {
	/** HTTP session state */
	enum http_client_session_state state;

	/** Server address */
	struct net_sockaddr_storage server_addr;

	/** Server address length */
	net_socklen_t addrlen;

	/** Server host name, NULL if not configured. */
	const char *host;

	/** Whether session is synchronous or asynchronous. */
	bool async;

	/** Whether the transport of the ended session is still to be closed,
	 *  by the thread that ended it once it dropped the client lock, or by
	 *  the sender still on it. Nothing is received from it any more.
	 */
	bool close_pending;

	/** HTTP streams for this session */
	struct http_client_stream streams[HTTP_CLIENT_MAX_STREAMS];

	/** Receive buffer used to buffer incoming data from the transport */
	uint8_t rx_buf[HTTP_CLIENT_RX_BUFFER_SIZE];

	/** Number of valid bytes currently stored in @ref rx_buf. */
	size_t rx_data_len;

	/** Transport stream the data in @ref rx_buf came from. */
	void *rx_stream;

	/** Transmit buffer used when sending HTTP frames */
	uint8_t tx_buf[HTTP_CLIENT_TX_BUFFER_SIZE];

	/** Protocol-specific incremental parser context. */
	union {
		/** HTTP/2 parser context. */
		struct http2_client_parser http2;
	} parser;
};

/** HTTP session configuration */
struct http_client_session_config {
	/** Server address */
	struct net_sockaddr_storage server_addr;

	/** Server address length */
	net_socklen_t addrlen;

	/** Server host name, IPv4 literal or bracketed IPv6 literal, without
	 *  a port, used for the :authority pseudo-header (HTTP/2) and the Host
	 *  header (HTTP/1.1). The port is taken from the server address. Leave
	 *  NULL to use the server address literal instead. Must remain valid
	 *  for the lifetime of the session.
	 */
	const char *host;

	/** Whether session should be synchronous or asynchronous. */
	bool async;
};

/** HTTP client context */
struct http_client {
	/** Transport API in use. */
	const struct http_client_transport_ops *transport_ops;

	/** Context data for the transport layer. */
	void *transport_ctx;

	/** HTTP version in use */
	enum http_client_version version;

	/** HTTP session context */
	struct http_client_session session;

	/** Client lock */
	struct k_mutex lock;

	/** Signaled when a thread blocked sending a request may be able to proceed. */
	struct k_condvar tx_cond;

	/** Threads inside http_client_send_request() that own a stream, which
	 *  http_client_disconnect() waits for.
	 */
	uint8_t callers;

	/** Serializes the senders on the transport, so that a request frame or
	 *  header block goes out in one piece. Taken without @ref lock held;
	 *  the receive context only ever tries it without waiting.
	 */
	struct k_mutex tx_lock;

	/** Deadline enforcement for asynchronous transactions. */
	struct k_work_delayable timeout_work;

	/** Opaque user data */
	void *user_data;
};

/** @brief Initialize HTTP client with a custom transport
 *
 * A client that was initialized before has to be disconnected with
 * http_client_disconnect() first, even when its session ended on its own,
 * so that no deadline handler is still running on it.
 *
 * @param client HTTP client instance
 * @param version HTTP version to use
 * @param transport_ops Transport layer API to use
 * @param transport_ctx A pointer to the context data for the transport layer
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid argument
 * @retval -ENOTSUP No protocol layer for the HTTP version
 */
int http_client_init(struct http_client *client, enum http_client_version version,
		     const struct http_client_transport_ops *transport_ops,
		     void *transport_ctx);

/** @brief Initialize HTTP client with TCP transport
 *
 * See http_client_init() for reinitializing a client.
 *
 * @param client HTTP client instance
 * @param version HTTP version to use
 * @param transport_ctx A pointer to the TCP transport context
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid argument
 * @retval -ENOTSUP No protocol layer for the HTTP version
 */
int http_client_init_tcp(struct http_client *client, enum http_client_version version,
			 struct http_client_transport_tcp_ctx *transport_ctx);

/** @brief Initialize HTTP client with TLS transport
 *
 * Available when @kconfig{CONFIG_HTTP_CLIENT_TRANSPORT_TLS} is enabled. The
 * transport creates a TLS socket, applies the configuration from the context
 * and advertises the HTTP version in use via ALPN. See http_client_init()
 * for reinitializing a client.
 *
 * @param client HTTP client instance
 * @param version HTTP version to use
 * @param transport_ctx A pointer to the TLS transport context
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid argument
 * @retval -ENOTSUP No protocol layer for the HTTP version
 */
int http_client_init_tls(struct http_client *client, enum http_client_version version,
			 struct http_client_transport_tls_ctx *transport_ctx);

/** @brief Connect HTTP client to a HTTP server
 *
 * Opens the transport connection and the HTTP session on it. A failure of
 * the transport is reported with its own error code.
 *
 * @param client HTTP client instance
 * @param config HTTP session configuration
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid argument, such as a server address of the wrong
 *                 length or an invalid host name
 * @retval -EALREADY A session is open already
 * @retval -EBUSY A request of the previous session is still being sent
 * @retval -ETIMEDOUT The server did not answer in time
 * @retval -ECONNREFUSED The server refused the connection
 */
int http_client_connect(struct http_client *client,
			struct http_client_session_config *config);

/** @brief Send HTTP request to a HTTP server
 *
 * If the HTTP session was configured to be synchronous, the function will block
 * until the request/response exchange is complete.
 *
 * If the HTTP session was configured to be asynchronous, the function will exit
 * as soon as the entire request was sent, allowing to send multiple requests in
 * parallel from a single thread. Sending a request payload may still block, in
 * either mode, until the server grants the flow control credit for it or the
 * timeout expires.
 *
 * In both cases, the response is returned to the application via the callback
 * provided in the @p req structure.
 *
 * When the server shuts the connection down gracefully, requests it has not
 * processed fail with -ESHUTDOWN and can be repeated on a new connection. The
 * session closes by itself once the remaining transactions are complete.
 *
 * A failure of the transport while sending ends the session, and is reported
 * with the transport's own error code, a send that timed out as -ETIMEDOUT.
 * In a synchronous session the call also returns how the transaction ended.
 * In an asynchronous one it returns 0 once the request is sent, and the
 * outcome reaches the callback instead, as @ref http_client_response::error
 * on an aborted transaction; only a transaction that ends while the request
 * is still being sent, for example while waiting for flow control credit for
 * its body, is reported by the call as well. A callback that aborts the
 * transaction ends it with its own return value.
 *
 * A request the call refuses, with -EINVAL or -EBUSY for example, is reported
 * to the callback as aborted with the same error as well, so that every
 * request passed in ends with a callback.
 *
 * @param client HTTP client instance
 * @param request HTTP request context. In async mode, should remain valid
 *                throughout the exchange.
 * @param timeout Maximum time to wait for the response, in milliseconds,
 *                counted from the call. A negative value waits forever.
 * @param user_data User specified data that is passed to the callback.
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid argument, such as a header field or request target
 *                 that cannot be sent
 * @retval -ENOTSUP The request cannot be expressed in the protocol, such as
 *                  the CONNECT method
 * @retval -ENOTCONN No session is open
 * @retval -ENOMEM No stream is free for the request
 * @retval -EBUSY The server allows no more concurrent requests, or a request
 *                is in flight already on a synchronous session
 * @retval -ESHUTDOWN The server is closing the connection, or the connection
 *                    is used up; the request can be repeated on a new one
 * @retval -ENOBUFS The request does not fit the transmit buffer, or a response
 *                  header field does not fit the receive buffer
 * @retval -ETIMEDOUT The response did not arrive in time, or a send timed out
 * @retval -ECONNRESET The server reset the request
 * @retval -EPROTO The response was malformed
 * @retval -EBADMSG The server violated the protocol, which ended the session
 * @retval -ECONNABORTED The session ended while the request was in flight
 */
int http_client_send_request(struct http_client *client,
			     const struct http_client_request *request,
			     int32_t timeout, void *user_data);

/** @brief Disconnect HTTP client from a HTTP server
 *
 * Ends the session, notifying the server and aborting the requests in
 * flight, and closes the transport. Also waits for a deadline handler of the
 * client that may be running and for the other threads still inside
 * http_client_send_request() to return, so that the client can be
 * reinitialized or freed afterwards; call it before reinitializing a client
 * whose session ended on its own as well.
 *
 * The notifications to the server are a courtesy: the session is closed
 * whether they could be sent or not. A failure to close the transport is
 * reported with the transport's own error code.
 *
 * @param client HTTP client instance
 *
 * @retval 0 Success, also when no session was open
 * @retval -EINVAL Invalid argument
 */
int http_client_disconnect(struct http_client *client);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_HTTP_CLIENT_MULTIPROTOCOL_H_ */
