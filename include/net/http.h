/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief HTTP server and client implementation for Zephyr.
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_H_
#define ZEPHYR_INCLUDE_NET_HTTP_H_

#include <net/net_app.h>
#include <net/http_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(ZEPHYR_USER_AGENT)
#define ZEPHYR_USER_AGENT "Zephyr"
#endif

#if !defined(CONFIG_HTTP_SERVER_NUM_URLS)
#define CONFIG_HTTP_SERVER_NUM_URLS 1
#endif

#if !defined(CONFIG_HTTP_HEADERS)
#define CONFIG_HTTP_HEADERS 1
#endif

#if !defined(HTTP_PROTOCOL)
#define HTTP_PROTOCOL	   "HTTP/1.1"
#endif

/**
 * @brief HTTP client and server library
 * @defgroup http HTTP Library
 * @ingroup networking
 * @{
 */

#define HTTP_CRLF "\r\n"

struct http_ctx;

enum http_state {
	  HTTP_STATE_CLOSED,
	  HTTP_STATE_WAITING_HEADER,
	  HTTP_STATE_RECEIVING_HEADER,
	  HTTP_STATE_HEADER_RECEIVED,
	  HTTP_STATE_OPEN,
} __packed;

enum http_url_flags {
	HTTP_URL_STANDARD = 0,
	HTTP_URL_WEBSOCKET,
} __packed;

enum http_connection_type {
	HTTP_CONNECTION = 1,
	WS_CONNECTION,
};

/* HTTP header fields struct */
struct http_field_value {
	/** Field name, this variable will point to the beginning of the string
	 *  containing the HTTP field name
	 */
	const char *key;

	/** Value, this variable will point to the beginning of the string
	 *  containing the field value
	 */
	const char *value;

	/** Length of the field name */
	u16_t key_len;

	/** Length of the field value */
	u16_t value_len;
};

/* HTTP root URL struct, used for pattern matching */
struct http_root_url {
	/** URL */
	const char *root;

	/** URL specific user data */
	u8_t *user_data;

	/** Length of the URL */
	u16_t root_len;

	/** Flags for this URL (values are from enum http_url_flags) */
	u8_t flags;

	/** Is this URL resource used or not */
	u8_t is_used;
};

enum http_verdict {
	HTTP_VERDICT_DROP,
	HTTP_VERDICT_ACCEPT,
};

/**
 * @typedef http_url_cb_t
 * @brief Default URL callback.
 *
 * @details This callback is called if there is a connection to unknown URL.
 *
 * @param ctx The context to use.
 * @param type Connection type (websocket or HTTP)
 * @param dst Remote socket address
 *
 * @return HTTP_VERDICT_DROP if connection is to be dropped,
 * HTTP_VERDICT_ACCEPT if the application wants to accept the unknown URL.
 */
typedef enum http_verdict (*http_url_cb_t)(struct http_ctx *ctx,
					   enum http_connection_type type,
					   const struct sockaddr *dst);

/* Collection of URLs that this server will handle */
struct http_server_urls {
	/* First item is the default handler and it is always there.
	 */
	struct http_root_url default_url;

	/** Callback that is called when unknown (default) URL is received */
	http_url_cb_t default_cb;

	struct http_root_url urls[CONFIG_HTTP_SERVER_NUM_URLS];
};

/**
 * @typedef http_recv_cb_t
 * @brief Network data receive callback.
 *
 * @details The recv callback is called after a network data is
 * received.
 *
 * @param ctx The context to use.
 * @param pkt Network buffer that is received. If the pkt is not NULL,
 * then the callback will own the buffer and it needs to to unref the pkt
 * as soon as it has finished working with it.  On EOF, pkt will be NULL.
 * @param status Value is set to 0 if some data or the connection is
 * at EOF, <0 if there was an error receiving data, in this case the
 * pkt parameter is set to NULL.
 * @param flags Flags related to http. For example contains information
 * if the data is text or binary etc.
 * @param dst Remote socket address from where HTTP packet is received.
 * @param user_data The user data given in init call.
 */
typedef void (*http_recv_cb_t)(struct http_ctx *ctx,
			       struct net_pkt *pkt,
			       int status,
			       u32_t flags,
			       const struct sockaddr *dst,
			       void *user_data);

/**
 * @typedef http_connect_cb_t
 * @brief Connection callback.
 *
 * @details The connect callback is called after there was a connection to
 * non-default URL.
 *
 * @param ctx The context to use.
 * @param type Connection type (websocket or HTTP)
 * @param dst Remote socket address where the connection is established.
 * @param user_data The user data given in init call.
 */
typedef void (*http_connect_cb_t)(struct http_ctx *ctx,
				  enum http_connection_type type,
				  const struct sockaddr *dst,
				  void *user_data);

/**
 * @typedef http_send_cb_t
 * @brief Network data send callback.
 *
 * @details The send callback is called after a network data is
 * sent.
 *
 * @param ctx The context to use.
 * @param status Value is set to 0 if all data was sent ok, <0 if
 * there was an error sending data. >0 amount of data that was
 * sent when not all data was sent ok.
 * @param user_data_send The user data given in http_send() call.
 * @param user_data The user data given in init call.
 */
typedef void (*http_send_cb_t)(struct http_ctx *ctx,
			       int status,
			       void *user_data_send,
			       void *user_data);

/**
 * @typedef http_close_cb_t
 * @brief Close callback.
 *
 * @details The close callback is called after a connection was shutdown.
 *
 * @param ctx The context to use.
 * @param status Error code for the closing.
 * @param user_data The user data given in init call.
 */
typedef void (*http_close_cb_t)(struct http_ctx *ctx,
				int status,
				void *user_data);

/** Websocket and HTTP callbacks */
struct http_cb {
	/** Function that is called when a connection is established.
	 */
	http_connect_cb_t connect;

	/** Function that is called when data is received from network.
	 */
	http_recv_cb_t recv;

	/** Function that is called when net_pkt is sent.
	 */
	http_send_cb_t send;

	/** Function that is called when connection is shutdown.
	 */
	http_close_cb_t close;
};

#if defined(CONFIG_HTTP_CLIENT)
/* Is there more data to come */
enum http_final_call {
	HTTP_DATA_MORE = 0,
	HTTP_DATA_FINAL = 1,
};

/* Some generic configuration options, these can be overridden if needed. */
#if !defined(HTTP_STATUS_STR_SIZE)
#define HTTP_STATUS_STR_SIZE	32
#endif

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
typedef void (*http_response_cb_t)(struct http_ctx *ctx,
				   u8_t *data,
				   size_t buflen,
				   size_t datalen,
				   enum http_final_call final_data,
				   void *user_data);

/**
 * HTTP client request. This contains all the data that is needed when doing
 * a HTTP request.
 */
struct http_request {
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
#endif /* CONFIG_HTTP_CLIENT */

/**
 * Http context information. This contains all the data that is
 * needed when working with http API.
 */
struct http_ctx {
	/** Net app context. The http connection is handled via
	 * the net app API.
	 */
	struct net_app_ctx app_ctx;

	/** Local endpoint IP address */
	struct sockaddr local;

	/** Original server address */
	struct sockaddr *server_addr;

	/** Pending data to be sent */
	struct net_pkt *pending;

#if defined(CONFIG_HTTP_CLIENT)
	/** Server name */
	const char *server;
#endif /* CONFIG_HTTP_CLIENT */

	struct {
#if defined(CONFIG_HTTP_CLIENT)
		/** Semaphore to signal HTTP connection creation. */
		struct k_sem connect_wait;

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
			/** User provided HTTP response callback which is
			 * called when a response is received to a sent HTTP
			 * request.
			 */
			http_response_cb_t cb;

			/** Where the body starts.
			 */
			u8_t *body_start;

			/** Where the response is stored, this is to be
			 * provided by the user.
			 */
			u8_t *response_buf;

			/** Response buffer maximum length */
			size_t response_buf_len;

			/** Length of the data in the result buf. If the value
			 * is larger than response_buf_len, then it means that
			 * the data is truncated and could not be fully copied
			 * into response_buf. This can only happen if the user
			 * did not set the response callback. If the callback
			 * is set, then the HTTP client API will call response
			 * callback many times so that all the data is
			 * delivered to the user.
			 */
			size_t data_len;

			/** HTTP Content-Length field value */
			size_t content_length;

			/** Content length parsed. This should be the same as
			 * the content_length field if parsing was ok.
			 */
			size_t processed;

			/* https://tools.ietf.org/html/rfc7230#section-3.1.2
			 * The status-code element is a 3-digit integer code
			 *
			 * The reason-phrase element exists for the sole
			 * purpose of providing a textual description
			 * associated with the numeric status code. A client
			 * SHOULD ignore the reason-phrase content.
			 */
			char http_status[HTTP_STATUS_STR_SIZE];

			u8_t cl_present:1;
			u8_t body_found:1;
			u8_t message_complete:1;
		} rsp;
#endif /* CONFIG_HTTP_CLIENT */

		/** HTTP parser for parsing the initial request */
		struct http_parser parser;

		/** HTTP parser settings */
		struct http_parser_settings parser_settings;

#if defined(CONFIG_HTTP_SERVER)
		/** Collection of HTTP header fields */
		struct http_field_value field_values[CONFIG_HTTP_HEADERS];

		/** Collection of HTTP URLs that this context will handle. */
		struct http_server_urls *urls;

		/** Where the request is stored, this needs to be provided by
		 * the user.
		 */
		u8_t *request_buf;

		/** Request buffer maximum length */
		size_t request_buf_len;

		/** Length of the data in the request buf. */
		size_t data_len;

		/** Number of header field elements */
		u16_t field_values_ctr;
#endif /* CONFIG_HTTP_SERVER */

		/** HTTP Request URL */
		const char *url;

		/** URL's length */
		u16_t url_len;
	} http;

#if defined(CONFIG_WEBSOCKET)
	struct {
		/** Pending data that is not yet ready for processing */
		struct net_pkt *pending;

		/** Amount of data that needs to be read still */
		u32_t data_waiting;

		/** Websocket connection masking value */
		u32_t masking_value;

		/** How many bytes we have read */
		u32_t data_read;

		/** Message type flag. Value is one of WS_FLAG_XXX flag values
		 * defined in weboscket.h
		 */
		u32_t msg_type_flag;
	} websocket;
#endif /* CONFIG_WEBSOCKET */

#if defined(CONFIG_NET_DEBUG_HTTP_CONN)
	sys_snode_t node;
#endif /* CONFIG_HTTP_DEBUG_HTTP_CONN */

	/** HTTP callbacks */
	struct http_cb cb;

	/** User specified data that is passed in callbacks. */
	u8_t *user_data;

	/** State of the websocket */
	enum http_state state;

	/** Network buffer allocation timeout */
	s32_t timeout;

	/** Is this context setup or not */
	u8_t is_init : 1;

	/** Is this context setup for client or server */
	u8_t is_client : 1;

	/** Is this instance supporting TLS or not. */
	u8_t is_tls : 1;

	/** Are we connected or not (only used in client) */
	u8_t is_connected : 1;
};

#if defined(CONFIG_HTTP_SERVER)
/**
 * @brief Create a HTTP listener.
 *
 * @details Note that the context must be valid for the whole duration of the
 * http life cycle. This usually means that it cannot be allocated from
 * stack.
 *
 * @param ctx Http context. This init function will initialize it.
 * @param urls Array of URLs that the server instance will serve. If the
 * server receives a HTTP request into one of the URLs, it will call user
 * supplied callback. If no such URL is registered, a default handler will
 * be called (if set by the user). If no data handler is found, the request
 * is dropped.
 * @param server_addr Socket address of the local network interface and TCP
 * port where the data is being waited. If the socket family is set to
 * AF_UNSPEC, then both IPv4 and IPv6 is started to be listened. If the
 * address is set to be INADDR_ANY (for IPv4) or unspecified address (all bits
 * zeros for IPv6), then the HTTP server will select proper IP address to bind
 * to. If caller has not specified HTTP listening port, then port 80 is being
 * listened. The parameter can be left NULL in which case a listener to port 80
 * using IPv4 and IPv6 is created. Note that if IPv4 or IPv6 is disabled, then
 * the corresponding disabled service listener is not created.
 * @param request_buf Caller-supplied buffer where the HTTP request will be
 * stored
 * @param request_buf_len Length of the caller-supplied buffer.
 * @param server_banner Print information about started service. This is only
 * printed if HTTP debugging is activated. The parameter can be set to NULL if
 * no extra prints are needed.
 * @param user_data User specific data that is passed as is to the connection
 * callbacks.
 *
 * @return 0 if ok, <0 if error.
 */
int http_server_init(struct http_ctx *ctx,
		     struct http_server_urls *urls,
		     struct sockaddr *server_addr,
		     u8_t *request_buf,
		     size_t request_buf_len,
		     const char *server_banner,
		     void *user_data);

#if defined(CONFIG_HTTPS)
/**
 * @brief Initialize TLS support for this http context
 *
 * @param ctx Http context
 * @param server_banner Print information about started service. This is only
 * printed if net_app debugging is activated. The parameter can be set to NULL
 * if no extra prints are needed.
 * @param personalization_data Personalization data (Device specific
 * identifiers) for random number generator. (Can be NULL).
 * @param personalization_data_len Length of the personalization data.
 * @param cert_cb User supplied callback that setups the certificates.
 * @param entropy_src_cb User supplied callback that setup the entropy. This
 * can be set to NULL, in which case default entropy source is used.
 * @param pool Memory pool for RX data reads.
 * @param stack TLS thread stack.
 * @param stack_len TLS thread stack size.
 *
 * @return Return 0 if ok, <0 if error.
 */
int http_server_set_tls(struct http_ctx *ctx,
			const char *server_banner,
			u8_t *personalization_data,
			size_t personalization_data_len,
			net_app_cert_cb_t cert_cb,
			net_app_entropy_src_cb_t entropy_src_cb,
			struct k_mem_pool *pool,
			k_thread_stack_t *stack,
			size_t stack_len);

#endif /* CONFIG_HTTPS */

/**
 * @brief Enable HTTP server that is related to this context.
 *
 * @detail The HTTP server will start to serve request after this.
 *
 * @param ctx Http context.
 *
 * @return 0 if server is enabled, <0 otherwise
 */
int http_server_enable(struct http_ctx *ctx);

/**
 * @brief Disable HTTP server that is related to this context.
 *
 * @detail The HTTP server will stop to serve request after this.
 *
 * @param ctx Http context.
 *
 * @return 0 if server is disabled, <0 if there was an error
 */
int http_server_disable(struct http_ctx *ctx);

/**
 * @brief Add an URL to a list of URLs that are tied to certain webcontext.
 *
 * @param urls URL struct that will contain all the URLs the user wants to
 * register.
 * @param url URL string.
 * @param flags Flags for the URL.
 *
 * @return NULL if the URL is already registered, pointer to  URL if
 * registering was ok.
 */
struct http_root_url *http_server_add_url(struct http_server_urls *urls,
					  const char *url, u8_t flags);

/**
 * @brief Delete the URL from list of URLs that are tied to certain
 * webcontext.
 *
 * @param urls URL struct that will contain all the URLs the user has
 * registered.
 * @param url URL string.
 *
 * @return 0 if ok, <0 if error.
 */
int http_server_del_url(struct http_server_urls *urls, const char *url);

/**
 * @brief Add default URL handler.
 *
 * @detail If no URL handler is found, then call this handler. There can
 * be only one default handler in the URL struct. The callback can decide
 * if the connection request is dropped or passed.
 *
 * @param urls URL struct that will contain all the URLs the user has
 * registered.
 * @param cb Callback that is called when non-registered URL is requested.
 *
 * @return NULL if default URL is already registered, pointer to default
 * URL if registering was ok.
 */
struct http_root_url *http_server_add_default(struct http_server_urls *urls,
					      http_url_cb_t cb);

/**
 * @brief Delete the default URL handler.
 *
 * @detail Unregister the previously registered default URL handler.
 *
 * @param urls URL struct that will contain all the URLs the user has
 * registered.
 *
 * @return 0 if ok, <0 if error.
 */
int http_server_del_default(struct http_server_urls *urls);

#else /* CONFIG_HTTP_SERVER */

static inline int http_server_init(struct http_ctx *ctx,
				   struct http_server_urls *urls,
				   struct sockaddr *server_addr,
				   u8_t *request_buf,
				   size_t request_buf_len,
				   const char *server_banner)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(urls);
	ARG_UNUSED(server_addr);
	ARG_UNUSED(request_buf);
	ARG_UNUSED(request_buf_len);
	ARG_UNUSED(server_banner);

	return -ENOTSUP;
}

#if defined(CONFIG_HTTPS) && defined(CONFIG_NET_APP_SERVER)
static inline int http_server_set_tls(struct http_ctx *ctx,
				      const char *server_banner,
				      u8_t *personalization_data,
				      size_t personalization_data_len,
				      net_app_cert_cb_t cert_cb,
				      net_app_entropy_src_cb_t entropy_src_cb,
				      struct k_mem_pool *pool,
				      k_thread_stack_t *stack,
				      size_t stack_len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(server_banner);
	ARG_UNUSED(personalization_data);
	ARG_UNUSED(personalization_data_len);
	ARG_UNUSED(cert_cb);
	ARG_UNUSED(entropy_src_cb);
	ARG_UNUSED(pool);
	ARG_UNUSED(stack);
	ARG_UNUSED(stack_len);

	return -ENOTSUP;
}
#endif /* CONFIG_HTTPS && CONFIG_NET_APP_SERVER */

static inline int http_server_enable(struct http_ctx *ctx)
{
	ARG_UNUSED(ctx);
	return -ENOTSUP;
}

static inline int http_server_disable(struct http_ctx *ctx)
{
	ARG_UNUSED(ctx);
	return -ENOTSUP;
}

static inline
struct http_root_url *http_server_add_url(struct http_server_urls *urls,
					  const char *url, u8_t flags)
{
	ARG_UNUSED(urls);
	ARG_UNUSED(url);
	ARG_UNUSED(flags);

	return NULL;
}
#endif /* CONFIG_HTTP_SERVER */

#if defined(CONFIG_HTTP_CLIENT)
/**
 * @brief Generic function to send a HTTP request to the network. Normally
 * applications would not need to use this function.
 *
 * @param ctx HTTP context.
 * @param req HTTP request to perform.
 * @param timeout Timeout when doing net_buf allocations.
 * @param user_data User data related to this request. This is passed
 * to send callback if user has specified that.
 *
 * @return Return 0 if ok, and <0 if error.
 */
int http_request(struct http_ctx *ctx,
		 struct http_request *req,
		 s32_t timeout,
		 void *user_data);

/**
 * @brief Cancel a pending request.
 *
 * @param ctx HTTP context.
 *
 * @return Return 0 if ok, and <0 if error.
 */
int http_request_cancel(struct http_ctx *ctx);

/**
 * @brief Send a HTTP request to peer.
 *
 * @param http_ctx HTTP context.
 * @param req HTTP request to perform.
 * @param cb Callback to call when the response has been received from peer.
 * @param response_buf Caller-supplied buffer where the HTTP response will be
 * stored
 * @param response_buf_len Length of the caller-supplied buffer.
 * @param user_data A valid pointer on some user data or NULL
 * @param timeout Amount of time to wait for a reply. If the timeout is 0,
 * then we return immediately and the callback (if set) will be called later.
 *
 * @return Return 0 if ok, and <0 if error.
 */
int http_client_send_req(struct http_ctx *http_ctx,
			 struct http_request *req,
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
 * to add. This can be set to NULL. The format is "name: value\\r\\n"
 * Example: "Accept: text/plain\\r\\nConnection: Close\\r\\n"
 * @param cb Callback to call when the response has been received from peer.
 * @param response_buf Caller-supplied buffer where the HTTP request will be
 * stored
 * @param response_buf_len Length of the caller-supplied buffer.
 * @param user_data A valid pointer on some user data or NULL
 * @param timeout Amount of time to wait for a reply. If the timeout is 0,
 * then we return immediately and the callback (if set) will be called later.
 *
 * @return Return 0 if ok, and <0 if error.
 */
static inline int http_client_send_get_req(struct http_ctx *http_ctx,
					   const char *url,
					   const char *host,
					   const char *extra_header_fields,
					   http_response_cb_t cb,
					   u8_t *response_buf,
					   size_t response_buf_len,
					   void *user_data,
					   s32_t timeout)
{
	struct http_request req = {
		.method = HTTP_GET,
		.url = url,
		.host = host,
		.protocol = " " HTTP_PROTOCOL,
		.header_fields = extra_header_fields,
	};

	return http_client_send_req(http_ctx, &req, cb, response_buf,
				    response_buf_len, user_data, timeout);
}

/**
 * @brief Initialize user-supplied HTTP context.
 *
 * @detail Caller can set the various fields in http_ctx after this call
 * if needed.
 *
 * @param http_ctx HTTP context.
 * @param server HTTP server address or host name. If host name is given,
 * then DNS resolver support (CONFIG_DNS_RESOLVER) must be enabled. If caller
 * sets the server parameter as NULL, then the server_addr parameter must be
 * set by the user and it is used when connecting to the server.
 * @param server_port HTTP server TCP port. If server parameter is NULL, then
 * this value is ignored.
 * @param server_addr HTTP server IP address and port. This is only used if
 * server parameter is not set.
 * @param timeout If server address needs to be resolved, then this value is
 * used as a timeout.
 *
 * @return Return 0 if ok, <0 if error.
 */
int http_client_init(struct http_ctx *http_ctx,
		     const char *server,
		     u16_t server_port,
		     struct sockaddr *server_addr,
		     s32_t timeout);

#if defined(CONFIG_HTTPS)
/**
 * @brief Initialize user-supplied HTTP context when using HTTPS.
 *
 * @detail Caller can set the various fields in http_ctx after this call
 * if needed.
 *
 * @param ctx HTTPS context.
 * @param request_buf Caller-supplied buffer where the TLS request will be
 * stored
 * @param request_buf_len Length of the caller-supplied buffer.
 * @param personalization_data Personalization data (Device specific
 * identifiers) for random number generator. (Can be NULL).
 * @param personalization_data_len Length of the personalization data.
 * @param cert_cb User-supplied callback that setups the certificates.
 * @param cert_host Hostname that is used to verify the server certificate.
 * This value is used when HTTP client API calls mbedtls_ssl_set_hostname()
 * which sets the hostname to check against the received server certificate.
 * See https://tls.mbed.org/kb/how-to/use-sni for more details.
 * This can be left NULL in which case mbedtls will silently skip certificate
 * verification entirely. This option is only used if MBEDTLS_X509_CRT_PARSE_C
 * is enabled in mbedtls config file.
 * @param entropy_src_cb User-supplied callback that setup the entropy. This
 * can be set to NULL, in which case default entropy source is used.
 * @param pool Memory pool for RX data reads.
 * @param https_stack HTTPS thread stack.
 * @param https_stack_len HTTP thread stack size.
 *
 * @return Return 0 if ok, <0 if error.
 */
int http_client_set_tls(struct http_ctx *ctx,
			u8_t *request_buf,
			size_t request_buf_len,
			u8_t *personalization_data,
			size_t personalization_data_len,
			net_app_ca_cert_cb_t cert_cb,
			const char *cert_host,
			net_app_entropy_src_cb_t entropy_src_cb,
			struct k_mem_pool *pool,
			k_thread_stack_t *https_stack,
			size_t https_stack_size);
#endif /* CONFIG_HTTPS */
#endif /* CONFIG_HTTP_CLIENT */

/**
 * @brief Close a network connection to peer.
 *
 * @param ctx Http context.
 *
 * @return 0 if ok, <0 if error.
 */
int http_close(struct http_ctx *ctx);

/**
 * @brief Release this http context.
 *
 * @details No network data will be received via this context after this
 * call.
 *
 * @param ctx Http context.
 *
 * @return 0 if ok, <0 if error.
 */
int http_release(struct http_ctx *ctx);

/**
 * @brief Set various callbacks that are called at various stage of ws session.
 *
 * @param ctx Http context.
 * @param connect_cb Connect callback.
 * @param recv_cb Data receive callback.
 * @param send_cb Data sent callback.
 * @param close_cb Close callback.
 *
 * @return 0 if ok, <0 if error.
 */
int http_set_cb(struct http_ctx *ctx,
		http_connect_cb_t connect_cb,
		http_recv_cb_t recv_cb,
		http_send_cb_t send_cb,
		http_close_cb_t close_cb);

/**
 * @brief Send a message to peer. The data can be either HTTP or websocket
 * data.
 *
 * @details This does not modify the network packet but sends it as is.
 *
 * @param ctx Http context.
 * @param pkt Network packet to send
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
int http_send_msg_raw(struct http_ctx *ctx, struct net_pkt *pkt,
		      void *user_send_data);

/**
 * @brief Prepare HTTP data to be sent to peer. If there is enough data, the
 * function will then send it too.
 *
 * @param ctx Http context.
 * @param payload Application data to send
 * @param payload_len Length of application data
 * @param dst Remote socket address
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
int http_prepare_and_send(struct http_ctx *ctx, const char *payload,
			  size_t payload_len,
			  const struct sockaddr *dst,
			  void *user_send_data);

/**
 * @brief Send HTTP data chunk to peer.
 *
 * @param ctx Http context.
 * @param payload Application data to send
 * @param payload_len Length of application data
 * @param dst Remote socket address
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
int http_send_chunk(struct http_ctx *ctx, const char *payload,
		    size_t payload_len,
		    const struct sockaddr *dst,
		    void *user_send_data);

/**
 * @brief Send any pending data immediately.
 *
 * @param ctx Http context.
 * @param user_send_data Optional user_data for that is used as a parameter in
 * send callback if that is specified.
 *
 * @return 0 if ok, <0 if error.
 */
int http_send_flush(struct http_ctx *ctx, void *user_send_data);

/**
 * @brief Send HTTP error message to peer.
 *
 * @param ctx Http context.
 * @param code HTTP error code
 * @param html_payload Extra payload, can be null
 * @param html_len Payload length
 * @param dst Remote socket address
 *
 * @return 0 if ok, <0 if error.
 */
int http_send_error(struct http_ctx *ctx, int code,
		    u8_t *html_payload, size_t html_len,
		    const struct sockaddr *dst);

/**
 * @brief Add HTTP header field to the message.
 *
 * @details This can be called multiple times to add pieces of HTTP header
 * fields into the message. Caller must put the "\\r\\n" characters to the
 * input http_header_field variable.
 *
 * @param ctx Http context.
 * @param http_header_field All or part of HTTP header to be added.
 * @param dst Remote socket address.
 * @param user_send_data User data value that is used in send callback.
 * Note that this value is only used if this function call causes a HTTP
 * message to be sent.
 *
 * @return <0 if error, other value tells how many bytes were added
 */
int http_add_header(struct http_ctx *ctx, const char *http_header_field,
		    const struct sockaddr *dst,
		    void *user_send_data);

/**
 * @brief Add HTTP header field to the message.
 *
 * @details This can be called multiple times to add pieces of HTTP header
 * fields into the message. If name is "Foo" and value is "bar", then this
 * function will add "Foo: bar\\r\\n" to message.
 *
 * @param ctx Http context.
 * @param name Name of the header field
 * @param value Value of the header field
 * @param dst Remote socket address
 * @param user_send_data User data value that is used in send callback.
 * Note that this value is only used if this function call causes a HTTP
 * message to be sent.
 *
 * @return <0 if error, other value tells how many bytes were added
 */
int http_add_header_field(struct http_ctx *ctx, const char *name,
			  const char *value,
			  const struct sockaddr *dst,
			  void *user_send_data);

/**
 * @brief Find a handler function for a given URL.
 *
 * @details This is internal function, do not call this from application.
 *
 * @param ctx Http context.
 * @param flags Tells if the URL is either HTTP or websocket URL
 *
 * @return URL handler or NULL if no such handler was found.
 */
struct http_root_url *http_url_find(struct http_ctx *ctx,
				    enum http_url_flags flags);

#define http_change_state(ctx, new_state)			\
	_http_change_state(ctx, new_state, __func__, __LINE__)

/**
 * @brief Change the state of the HTTP engine
 *
 * @details This is internal function, do not call this from application.
 *
 * @param ctx HTTP context.
 * @param new_state New state of the context.
 * @param func Function that changed the state (for debugging)
 * @param line Line number of the function (for debugging)
 */
void _http_change_state(struct http_ctx *ctx,
			enum http_state new_state,
			const char *func, int line);

#if defined(CONFIG_HTTP_SERVER) && defined(CONFIG_NET_DEBUG_HTTP_CONN)
/**
 * @typedef http_server_cb_t
 * @brief HTTP connection monitoring callback.
 *
 * @details This callback is called by server if new HTTP connection is
 * established or disconnected. Requires that HTTP connection monitoring
 * is enabled.
 *
 * @param ctx The HTTP context to use.
 * @param user_data User specified data.
 */
typedef void (*http_server_cb_t)(struct http_ctx *entry,
				 void *user_data);

/**
 * @brief Go through all the HTTP connections in the server and call
 * user provided callback for each connection.
 *
 * @param cb Callback to call for each HTTP connection.
 * @param user_data User provided data that is passed to callback.
 */
void http_server_conn_foreach(http_server_cb_t cb, void *user_data);

/**
 * @brief Register a callback that is called if HTTP connection is
 * established or disconnected.
 *
 * @details This is normally called only by "net http monitor" shell command.
 *
 * @param cb Callback to call for each HTTP connection created or
 * deleted.
 * @param user_data User provided data that is passed to callback.
 */
void http_server_conn_monitor(http_server_cb_t cb, void *user_data);

/**
 * @brief HTTP connection was established.
 *
 * @details This is internal function, do not call this from application.
 *
 * @param ctx HTTP context.
 */
void http_server_conn_add(struct http_ctx *ctx);

/**
 * @brief HTTP connection was disconnected.
 *
 * @details This is internal function, do not call this from application.
 *
 * @param ctx HTTP context.
 */
void http_server_conn_del(struct http_ctx *ctx);
#else
#define http_server_conn_foreach(...)
#define http_server_conn_monitor(...)
#define http_server_conn_add(...)
#define http_server_conn_del(...)
#endif /* CONFIG_HTTP_SERVER && CONFIG_NET_DEBUG_HTTP_CONN */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_HTTP_H_ */
