/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#if defined(CONFIG_HTTP_CLIENT)

#include <net/http_parser.h>
#include <net/net_context.h>

#define HTTP_CRLF "\r\n"

/* Is there more data to come */
enum http_final_call {
	HTTP_DATA_MORE = 0,
	HTTP_DATA_FINAL = 1,
};

#ifndef HTTP_PROTOCOL
#define HTTP_PROTOCOL	   "HTTP/1.1"
#endif

/* Some generic configuration options, these can be overriden if needed. */
#ifndef HTTP_STATUS_STR_SIZE
#define HTTP_STATUS_STR_SIZE	32
#endif

/* Default network activity timeout in seconds */
#define HTTP_NETWORK_TIMEOUT	K_SECONDS(20)

/* It seems enough to hold 'Content-Length' and its value */
#define HTTP_CONTENT_LEN_SIZE	48

/* Default HTTP Header Field values for HTTP Requests if using the
 * HTTP_HEADER_FIELDS define.
 */
#ifndef HTTP_ACCEPT
#define HTTP_ACCEPT		"text/plain"
#endif

#ifndef HTTP_ACCEPT_ENC
#define HTTP_ACCEPT_ENC		"identity"
#endif

#ifndef HTTP_ACCEPT_LANG
#define HTTP_ACCEPT_LANG	"en-US"
#endif

#ifndef HTTP_CONNECTION
#define HTTP_CONNECTION		"Close"
#endif

#ifndef HTTP_USER_AGENT
#define HTTP_USER_AGENT	"Zephyr-HTTP-Client/1.8"
#endif

/* This can be used in http_client_send_get_req() when supplying
 * extra_header_fields parameter.
 */
#ifndef HTTP_HEADER_FIELDS
#define HTTP_HEADER_FIELDS				     \
	"Accept: " HTTP_ACCEPT HTTP_CRLF		     \
	"Accept-Encoding: " HTTP_ACCEPT_ENC HTTP_CRLF	     \
	"Accept-Language: " HTTP_ACCEPT_LANG HTTP_CRLF	     \
	"User-Agent: " HTTP_USER_AGENT HTTP_CRLF	     \
	"Connection: " HTTP_CONNECTION HTTP_CRLF
#endif

struct http_client_ctx;

/**
 * @typedef http_receive_cb_t
 * @brief Callback used when TCP data has been received from peer.
 *
 * @param ctx HTTP context.
 * @param pkt Network packet.
 */
typedef void (*http_receive_cb_t)(struct http_client_ctx *ctx,
				  struct net_pkt *pkt);

/**
 * @typedef http_response_cb_t
 * @brief Callback used when a response has been received from peer.
 *
 * @param ctx HTTP context.
 * @param data Received data buffer
 * @param buflen Data buffer len (as specified by user)
 * @param datalen Received data len, if this is larger than buflen,
 * then some data was skipped.
 * @param final_data Does this data buffer contain all the data or
 * is there still more data to come.
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*http_response_cb_t)(struct http_client_ctx *ctx,
				   u8_t *data, size_t buflen,
				   size_t datalen,
				   enum http_final_call final_data,
				   void *user_data);

/**
 * HTTP client context information. This contains all the data that is
 * needed when doing HTTP requests.
 */
struct http_client_ctx {
	struct http_parser parser;
	struct http_parser_settings settings;

	/** Server name */
	const char *server;

#if defined(CONFIG_DNS_RESOLVER)
	/** Remember the DNS query id so that it can be cancelled
	 * if the HTTP context is released and the query is active
	 * at that time.
	 */
	u16_t dns_id;
#endif

	struct {
		/** Local socket address */
		struct sockaddr local;

		/** Remote (server) socket address */
		struct sockaddr remote;

		/** IP stack network context */
		struct net_context *ctx;

		/** Network timeout */
		s32_t timeout;

		/** User can define this callback if it wants to have
		 * special handling of the received raw data.
		 */
		http_receive_cb_t receive_cb;
	} tcp;

	/** HTTP request information */
	struct {
		/**
		 * Semaphore to signal HTTP request completion
		 */
		struct k_sem wait;

		/** Hostname to be used in the request */
		const char *host;

		/** User provided data */
		void *user_data;

		/** What method we used here (GET, POST, HEAD etc.)
		 */
		enum http_method method;
	} req;

	/** HTTP response information */
	struct {
		/** User provided HTTP response callback which is called
		 * when a response is received to a sent HTTP request.
		 */
		http_response_cb_t cb;

		/** Where the response is stored, this is to be provided
		 * by the user.
		 */
		u8_t *response_buf;

		/** Where the body starts.
		 */
		u8_t *body_start;

		/** Response buffer maximum length */
		size_t response_buf_len;

		/** Length of the data in the result buf. If the value is
		 * larger than response_buf_len, then it means that the data
		 * is truncated and could not be fully copied into
		 * response_buf. This can only happen if the user did not
		 * set the response callback. If the callback is set, then
		 * the HTTP client API will call response callback many times
		 * so that all the data is delivered to the user.
		 */
		size_t data_len;

		/** HTTP Content-Length field value */
		size_t content_length;

		/** Content length parsed. This should be the same as the
		 * content_length field if parsing was ok.
		 */
		size_t processed;

		/* https://tools.ietf.org/html/rfc7230#section-3.1.2
		 * The status-code element is a 3-digit integer code
		 *
		 * The reason-phrase element exists for the sole purpose of
		 * providing a textual description associated with the
		 * numeric status code. A client SHOULD ignore the
		 * reason-phrase content.
		 */
		char http_status[HTTP_STATUS_STR_SIZE];

		u8_t cl_present:1;
		u8_t body_found:1;
	} rsp;
};

/**
 * HTTP client request. This contains all the data that is needed when doing
 * a HTTP request.
 */
struct http_client_request {
	/** The HTTP method: GET, HEAD, OPTIONS, POST, ... */
	enum http_method method;

	/** The URL for this request, for example: /index.html */
	const char *url;

	/** The HTTP protocol: HTTP/1.1 */
	const char *protocol;

	/** The HTTP header fields (application specific)
	 * The Content-Type may be specified here or in the next field.
	 * Depending on your application, the Content-Type may vary, however
	 * some header fields may remain constant through the application's
	 * life cycle.
	 */
	const char *header_fields;

	/** The value of the Content-Type header field, may be NULL */
	const char *content_type_value;

	/** Hostname to be used in the request */
	const char *host;

	/** Payload, may be NULL */
	const char *payload;

	/** Payload size, may be 0 */
	u16_t payload_size;
};

/**
 * @brief Generic function to send a HTTP request to the network. Normally
 * applications would not need to use this function.
 *
 * @param net_ctx Network context.
 * @param req HTTP request to perform.
 * @param timeout Timeout when doing net_buf allocations.
 *
 * @return Return 0 if ok, and <0 if error.
 */
int http_request(struct net_context *net_ctx, struct http_client_request *req,
		 s32_t timeout);

/**
 * @brief Send a HTTP request to peer.
 *
 * @param http_ctx HTTP context.
 * @param req HTTP request to perform.
 * @param cb Callback to call when the response has been received from peer.
 * @param response_buf Caller supplied buffer where the HTTP response will be
 * stored
 * @param response_buf_len Length of the caller suppied buffer.
 * @param user_data A valid pointer on some user data or NULL
 * @param timeout Amount of time to wait for a reply. If the timeout is 0,
 * then we return immediately and the callback (if set) will be called later.
 *
 * @return Return 0 if ok, and <0 if error.
 */
int http_client_send_req(struct http_client_ctx *http_ctx,
			 struct http_client_request *req,
			 http_response_cb_t cb,
			 u8_t *response_buf,
			 size_t response_buf_len,
			 void *user_data,
			 s32_t timeout);

/**
 * @brief Send a HTTP GET request to peer.
 *
 * @param http_ctx HTTP context.
 * @param url URL to use.
 * @param host Host field in HTTP header. If set to NULL, then server
 * name is used.
 * @param extra_header_fields Any extra header fields that caller wants
 * to add. This can be set to NULL. The format is "name: value\r\n"
 * Example: "Accept: text/plain\r\nConnection: Close\r\n"
 * @param cb Callback to call when the response has been received from peer.
 * @param response_buf Caller supplied buffer where the HTTP request will be
 * stored
 * @param response_buf_len Length of the caller suppied buffer.
 * @param user_data A valid pointer on some user data or NULL
 * @param timeout Amount of time to wait for a reply. If the timeout is 0,
 * then we return immediately and the callback (if set) will be called later.
 *
 * @return Return 0 if ok, and <0 if error.
 */
static inline int http_client_send_get_req(struct http_client_ctx *http_ctx,
					   const char *url,
					   const char *host,
					   const char *extra_header_fields,
					   http_response_cb_t cb,
					   u8_t *response_buf,
					   size_t response_buf_len,
					   void *user_data,
					   s32_t timeout)
{
	struct http_client_request req = {
				.method = HTTP_GET,
				.url = url,
				.host = host,
				.protocol = " " HTTP_PROTOCOL HTTP_CRLF,
				.header_fields = extra_header_fields,
	};

	return http_client_send_req(http_ctx, &req, cb, response_buf,
				    response_buf_len, user_data, timeout);
}

/**
 * @brief Send a HTTP POST request to peer.
 *
 * @param http_ctx HTTP context.
 * @param url URL to use.
 * @param host Host field in HTTP header. If set to NULL, then server
 * name is used.
 * @param extra_header_fields Any extra header fields that caller wants
 * to add. This can be set to NULL. The format is "name: value\r\n"
 * Example: "Accept: text/plain\r\nConnection: Close\r\n"
 * @param content_type Content type of the data.
 * @param payload Payload data.
 * @param cb Callback to call when the response has been received from peer.
 * @param response_buf Caller supplied buffer where the HTTP response will be
 * stored
 * @param response_buf_len Length of the caller suppied buffer.
 * @param user_data A valid pointer on some user data or NULL
 * @param timeout Amount of time to wait for a reply. If the timeout is 0,
 * then we return immediately and the callback (if set) will be called later.
 *
 * @return Return 0 if ok, and <0 if error.
 */
static inline int http_client_send_post_req(struct http_client_ctx *http_ctx,
					    const char *url,
					    const char *host,
					    const char *extra_header_fields,
					    const char *content_type,
					    const char *payload,
					    http_response_cb_t cb,
					    u8_t *response_buf,
					    size_t response_buf_len,
					    void *user_data,
					    s32_t timeout)
{
	struct http_client_request req = {
				.method = HTTP_POST,
				.url = url,
				.host = host,
				.protocol = " " HTTP_PROTOCOL HTTP_CRLF,
				.header_fields = extra_header_fields,
				.content_type_value = content_type,
				.payload = payload,
	};

	return http_client_send_req(http_ctx, &req, cb, response_buf,
				    response_buf_len, user_data, timeout);
}

/**
 * @brief Initialize user supplied HTTP context.
 *
 * @detail Caller can set the various fields in http_ctx after this call
 * if needed.
 *
 * @param http_ctx HTTP context.
 * @param server HTTP server address or host name. If host name is given,
 * then DNS resolver support (CONFIG_DNS_RESOLVER) must be enabled. If caller
 * sets the server parameter as NULL, then it no attempt is done to figure out
 * the remote address and caller must set the address in http_ctx.tcp.remote
 * itself.
 * @param server_port HTTP server TCP port.
 *
 * @return Return 0 if ok, <0 if error.
 */
int http_client_init(struct http_client_ctx *http_ctx,
		     const char *server, u16_t server_port);

/**
 * @brief Release all the resources allocated for HTTP context.
 *
 * @param http_ctx HTTP context.
 */
void http_client_release(struct http_client_ctx *http_ctx);
#endif

#if defined(CONFIG_HTTP_SERVER)

#include <net/net_context.h>

#if defined(CONFIG_HTTP_PARSER)
#include <net/http_parser.h>
#endif

/* HTTP server context state */
enum HTTP_CTX_STATE {
	HTTP_CTX_FREE = 0,
	HTTP_CTX_IN_USE
};

/* HTTP header fields struct */
struct http_field_value {
	/** Field name, this variable will point to the beginning of the string
	 *  containing the HTTP field name
	 */
	const char *key;
	/** Length of the field name */
	u16_t key_len;

	/** Value, this variable will point to the beginning of the string
	 *  containing the field value
	 */
	const char *value;
	/** Length of the field value */
	u16_t value_len;
};

/* The HTTP server context struct */
struct http_server_ctx {
	u8_t state;

	/** Collection of header fields */
	struct http_field_value field_values[CONFIG_HTTP_HEADER_FIELD_ITEMS];
	/** Number of header field elements */
	u16_t field_values_ctr;

	/** HTTP Request URL */
	const char *url;
	/** URL's length */
	u16_t url_len;

	/**IP stack network context */
	struct net_context *net_ctx;

	/** Network timeout */
	s32_t timeout;

#if defined(CONFIG_HTTP_PARSER)
	/** HTTP parser */
	struct http_parser parser;
	/** HTTP parser settings */
	struct http_parser_settings parser_settings;
#endif
};

int http_response(struct http_server_ctx *ctx, const char *http_header,
		  const char *html_payload);

int http_response_400(struct http_server_ctx *ctx, const char *html_payload);

int http_response_403(struct http_server_ctx *ctx, const char *html_payload);

int http_response_404(struct http_server_ctx *ctx, const char *html_payload);

#endif

#endif
