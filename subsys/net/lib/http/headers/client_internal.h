/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP_CLIENT_INTERNAL_H_
#define HTTP_CLIENT_INTERNAL_H_

#include <zephyr/net/http/client_multiprotocol.h>

/* Protocol layer interface, one implementation per HTTP version. The core
 * calls every operation with the client lock held, except the two request
 * senders, which run without it and take it for the session state as they
 * need; they serialize on the client's TX lock. The sends return 0 or a
 * negative errno from the transport; a protocol layer that finds the session
 * unusable ends it with http_client_session_terminate(), which the core
 * notices by the session state. Operations marked optional may be NULL.
 */
struct http_client_ops {
	/* Open the protocol session on the connected transport: send the
	 * preface, if any. A failure closes the transport again.
	 */
	int (*session_open)(struct http_client *client);

	/* Tell the peer that the session ends, with reason 0 for a disconnect
	 * by the application and a negative errno otherwise. Called while the
	 * transport is still usable, before it is closed. Optional.
	 */
	int (*session_close)(struct http_client *client, int reason);

	/* Set a fresh stream up for a request, e. g. assign its identifier,
	 * before anything is sent. Returns -EBUSY when the peer allows no more
	 * concurrent streams and -ESHUTDOWN when the session takes no new
	 * streams at all. Optional.
	 */
	int (*stream_open)(struct http_client *client, struct http_client_stream *stream);

	/* Release what the protocol layer holds for a stream, once its
	 * transaction is over and before the slot is reused. Optional.
	 */
	int (*stream_close)(struct http_client *client, struct http_client_stream *stream);

	/* Tell the peer that a stream whose request went out is abandoned:
	 * on a timeout, an error while sending or a disconnect. Optional.
	 */
	int (*stream_reset)(struct http_client *client, struct http_client_stream *stream);

	/* Send the request line and header fields, taken from the request
	 * builder (http_client_request_fields_foreach()). end_stream is set
	 * when no body follows. Called without the client lock.
	 */
	int (*stream_send_headers)(struct http_client *client, struct http_client_stream *stream,
				   const struct http_client_request *req, bool end_stream);

	/* Send a chunk of the request body, waiting for the peer's flow
	 * control through http_client_stream_wait_tx() where the protocol has
	 * any. end_stream is set with the last chunk, which may be empty.
	 * Called without the client lock.
	 */
	int (*stream_send_data)(struct http_client *client, struct http_client_stream *stream,
				const uint8_t *data, size_t len, bool end_stream);

	/* Process received data. transport_stream is the transport stream the
	 * data came from, NULL on a single-stream transport, and end_stream is
	 * set when no more data follows the len bytes on that stream; it is
	 * then called even with len 0. Returns the number of bytes consumed,
	 * -EAGAIN when more data is needed (not with end_stream set, as none
	 * will come), or a negative errno on error.
	 */
	int (*process_data)(struct http_client *client, void *transport_stream, uint8_t *data,
			    size_t len, bool end_stream);
};

#if defined(CONFIG_HTTP_CLIENT_VERSION_2)
const struct http_client_ops *http_client_get_http2_ops(void);
#else
static inline const struct http_client_ops *http_client_get_http2_ops(void) { return NULL; }
#endif

/* Look up an active stream by its protocol stream identifier. A completed
 * stream is not found, even while it waits to be released. Returns NULL if no
 * matching stream exists. Must be called with the client lock held.
 */
struct http_client_stream *http_client_stream_lookup(struct http_client *client,
						     uint64_t stream_id);

/* Deliver a chunk of the response, set up in stream->response, to the
 * application. Must be called with the client lock held (e. g. from a
 * process_data() implementation). The headers and body of the chunk are
 * cleared afterwards. On the final chunk the transaction is completed, with
 * the callback's result if it is negative, and the stream may be released on
 * return in asynchronous mode; the caller must not touch it then.
 *
 * Returns the callback's result on an intermediate chunk, so that a negative
 * value aborts the transaction, and 0 on the final one.
 */
int http_client_stream_deliver(struct http_client *client, struct http_client_stream *stream,
			       bool final);

/* Notify the client that a stream transaction was aborted with the negative
 * errno result. Must be called with the client lock held (e. g. from a
 * process_data() implementation). The application is notified via its
 * response callback. In asynchronous mode the stream is closed and released
 * here, unless its request is still being sent; otherwise the thread in
 * http_client_send_request() does that. Does nothing on a stream whose
 * transaction is over already.
 */
void http_client_stream_set_complete(struct http_client *client,
				     struct http_client_stream *stream, int result);

/* Let the receive path make progress while a request cannot be sent further,
 * e. g. for lack of flow control credit. Receives on the calling thread in
 * synchronous mode, waits for http_client_tx_wakeup() in asynchronous mode. The
 * wait is bounded, so the caller re-checks its condition and calls again.
 * Must be called with the client lock held, which is released while waiting.
 *
 * Returns 0 when the caller should re-check, -ETIMEDOUT when the stream
 * deadline has passed, -EALREADY when the transaction completed successfully
 * in the meantime, or the negative result it was aborted with.
 */
int http_client_stream_wait_tx(struct http_client *client, struct http_client_stream *stream);

/* Wake threads waiting in http_client_stream_wait_tx(). Must be called with
 * the client lock held.
 */
void http_client_tx_wakeup(struct http_client *client);

/* Take and give up the TX lock, which serializes the senders of request
 * frames on the transport. Taken without the client lock held, as the
 * receive context only ever tries it without waiting. Giving it up also
 * closes the transport when the session ended meanwhile and left that to the
 * sender. Both are called with the client lock released.
 */
void http_client_tx_acquire(struct http_client *client);
void http_client_tx_release(struct http_client *client);

/* Request building, shared by the protocol layers. */

/* Longest field name, path or authority the client handles. */
#define HTTP_CLIENT_REQUEST_FIELD_MAX_LEN 256U

/* The request line in protocol-neutral form: what HTTP/2 and HTTP/3 send as
 * pseudo-headers and HTTP/1.1 as its request line and Host header. The
 * strings point into the request, the session or the buffer given to
 * http_client_request_line_get().
 */
struct http_client_request_line {
	const char *method;
	const char *scheme;
	const char *authority;
	const char *path;
};

/* Fill in the request line of req. buf backs the authority, host[:port] from
 * the application Host header or the server address, and the path, which
 * always starts with a slash.
 */
int http_client_request_line_get(struct http_client *client,
				 const struct http_client_request *req, char *buf, size_t buflen,
				 struct http_client_request_line *line);

/* Receives one header field. A negative return stops the walk with that
 * error. The strings are only valid during the call.
 */
typedef int (*http_client_field_cb_t)(const char *name, const char *value, void *user_data);

/* Walk the header fields of the stream's request in wire order: the request
 * line as :method, :scheme, :path and :authority when pseudo_headers is set,
 * the fields the library adds (content-type, and content-length when the
 * payload length is known up front), then the application fields including
 * those from headers_cb. Names are lowercase. Fields the library sends
 * itself are left out: Host, as the request line carries the authority, an
 * application Content-Type when the request sets content_type_value, and
 * Content-Length when the library adds one. With pseudo_headers set the
 * connection-specific fields of HTTP/1.1 are dropped as well. A field whose
 * name is not a token or starts with a colon, or whose value carries control
 * characters other than tab or leading or trailing whitespace, fails with
 * -EINVAL; the fields given up front are checked before anything is emitted.
 * The value of an application Host header must be a valid authority.
 */
int http_client_request_fields_foreach(struct http_client *client,
				       struct http_client_stream *stream, bool pseudo_headers,
				       http_client_field_cb_t cb, void *user_data);

/* End the session from the protocol layer, e. g. once the peer has shut the
 * connection down: remaining streams are aborted, the peer is notified when
 * notify_peer is set and the transport closed. A reason of 0 stands for a
 * regular shutdown. Must be called with the client lock held.
 */
void http_client_session_terminate(struct http_client *client, int reason, bool notify_peer);

/* Whether a field name is one of the connection-specific fields of HTTP/1.1,
 * which have no place in an HTTP/2 or HTTP/3 message (RFC 9113,
 * Section 8.2.2). The name need not be NUL-terminated.
 */
bool http_client_field_is_connection_specific(const char *name, size_t name_len);

/* Whether a session host is a valid reg-name, IPv4 address or bracketed IPv6
 * address, with no port.
 */
bool http_client_host_is_valid(const char *host);

#endif /* HTTP_CLIENT_INTERNAL_H_ */
