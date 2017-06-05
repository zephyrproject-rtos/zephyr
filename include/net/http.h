/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#include <net/net_context.h>

#if defined(CONFIG_HTTPS)
#if defined(CONFIG_MBEDTLS)
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#define mbedtls_time_t       time_t
#define MBEDTLS_EXIT_SUCCESS EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#include <mbedtls/ssl_cookie.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>
#endif /* CONFIG_MBEDTLS */
#endif /* CONFIG_HTTPS */

#define HTTP_CRLF "\r\n"

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
void http_heap_init(void);
#else
#define http_heap_init()
#endif /* MBEDTLS_MEMORY_BUFFER_ALLOC_C */

typedef int (*http_send_data_t)(struct net_pkt *pkt,
				net_context_send_cb_t cb,
				s32_t timeout,
				void *token,
				void *user_data);

#if defined(CONFIG_HTTPS)
/* Internal information for managing HTTPS data */
struct https_context {
	struct net_pkt *rx_pkt;
	struct net_buf *frag;
	struct k_sem tx_sem;
	struct k_fifo rx_fifo;
	struct k_fifo tx_fifo;
	int remaining;
};

/**
 * @typedef https_entropy_src_cb_t
 * @brief Callback used when the API user needs to setup the entropy source.
 * @detail This is the same as mbedtls_entropy_f_source_ptr callback.
 *
 * @param data Callback-specific data pointer
 * @param output Data to fill
 * @param len Maximum size to provide
 * @param olen The actual amount of bytes put into the buffer (Can be 0)
 */
typedef int (*https_entropy_src_cb_t)(void *data, unsigned char *output,
				      size_t len, size_t *olen);
#endif /* CONFIG_HTTPS */

#if defined(CONFIG_HTTP_CLIENT)

#include <net/http_parser.h>
#include <net/net_context.h>

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

#if defined(CONFIG_HTTPS)
/**
 * @typedef https_ca_cert_cb_t
 * @brief Callback used when the API user needs to setup the
 * HTTPS certs.
 *
 * @param ctx HTTPS client context.
 * @param ca_cert MBEDTLS certificate. This is of type mbedtls_x509_crt
 * if MBEDTLS_X509_CRT_PARSE_C is defined.
 *
 * @return 0 if ok, <0 if there is an error
 */
typedef int (*https_ca_cert_cb_t)(struct http_client_ctx *ctx,
				  void *ca_cert);
#endif /* CONFIG_HTTPS */

/**
 * HTTP client context information. This contains all the data that is
 * needed when doing HTTP requests.
 */
struct http_client_ctx {
	struct http_parser parser;
	struct http_parser_settings settings;

	/** Server name */
	const char *server;

	/** Is this instance HTTPS or not.
	 */
	bool is_https;

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
		 * special handling of the received raw data. Note that this
		 * callback is not called for HTTPS data.
		 */
		http_receive_cb_t receive_cb;

		/** Internal function that is called when HTTP data is
		 * received from network.
		 */
		net_context_recv_cb_t recv_cb;

		/** Internal function that is called when HTTP data is sent to
		 * network.
		 */
		http_send_data_t send_data;
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

#if defined(CONFIG_HTTPS)
	struct {
		/** HTTPS stack for mbedtls library. */
		u8_t *stack;

		/** HTTPS stack size. */
		int stack_size;

		/** HTTPS thread id */
		k_tid_t tid;

		/** HTTPS thread */
		struct k_thread thread;

		/** Memory pool for RX data */
		struct k_mem_pool *pool;

		/** Hostname to be used in the certificate verification */
		const char *cert_host;

		/** mbedtls related configuration. */
		struct {
			struct https_context ssl_ctx;
			https_ca_cert_cb_t cert_cb;
			https_entropy_src_cb_t entropy_src_cb;
			mbedtls_entropy_context entropy;
			mbedtls_ctr_drbg_context ctr_drbg;
			mbedtls_ssl_context ssl;
			mbedtls_ssl_config conf;
			mbedtls_x509_crt ca_cert;
			u8_t *personalization_data;
			size_t personalization_data_len;
		} mbedtls;
	} https;
#endif /* CONFIG_HTTPS */
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
 * @param ctx HTTP client context.
 * @param req HTTP request to perform.
 * @param timeout Timeout when doing net_buf allocations.
 *
 * @return Return 0 if ok, and <0 if error.
 */
int http_request(struct http_client_ctx *ctx,
		 struct http_client_request *req,
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
 * sets the server parameter as NULL, then no attempt is done to figure out
 * the remote address and caller must set the address in http_ctx.tcp.remote
 * itself.
 * @param server_port HTTP server TCP port.
 *
 * @return Return 0 if ok, <0 if error.
 */
int http_client_init(struct http_client_ctx *http_ctx,
		     const char *server, u16_t server_port);

#if defined(CONFIG_HTTPS)
/**
 * @brief Initialize user supplied HTTP context when using HTTPS.
 *
 * @detail Caller can set the various fields in http_ctx after this call
 * if needed.
 *
 * @param http_ctx HTTPS context.
 * @param server HTTPS server address or host name. If host name is given,
 * then DNS resolver support (CONFIG_DNS_RESOLVER) must be enabled. If caller
 * sets the server parameter as NULL, then no attempt is done to figure out
 * the remote address and caller must set the address in http_ctx.tcp.remote
 * itself.
 * @param server_port HTTPS server TCP port.
 * @param personalization_data Personalization data (Device specific
 * identifiers) for random number generator. (Can be NULL).
 * @param personalization_data_len Length of the personalization data.
 * @param cert_cb User supplied callback that setups the certifacates.
 * @param cert_host Hostname that is used to verify the server certificate.
 * This value is used when HTTP client API calls mbedtls_ssl_set_hostname()
 * which sets the hostname to check against the received server certificate.
 * See https://tls.mbed.org/kb/how-to/use-sni for more details.
 * This can be left NULL in which case mbedtls will silently skip certificate
 * verification entirely. This option is only used if MBEDTLS_X509_CRT_PARSE_C
 * is enabled in mbedtls config file.
 * @param entropy_src_cb User supplied callback that setup the entropy. This
 * can be set to NULL, in which case default entropy source is used.
 * @param pool Memory pool for RX data reads.
 * @param https_stack HTTPS thread stack.
 * @param https_stack_len HTTP thread stack size.
 *
 * @return Return 0 if ok, <0 if error.
 */
int https_client_init(struct http_client_ctx *http_ctx,
		      const char *server, u16_t server_port,
		      u8_t *personalization_data,
		      size_t personalization_data_len,
		      https_ca_cert_cb_t cert_cb,
		      const char *cert_host,
		      https_entropy_src_cb_t entropy_src_cb,
		      struct k_mem_pool *pool,
		      u8_t *https_stack,
		      size_t https_stack_size);
#endif /* CONFIG_HTTPS */

/**
 * @brief Release all the resources allocated for HTTP context.
 *
 * @param http_ctx HTTP context.
 */
void http_client_release(struct http_client_ctx *http_ctx);
#endif /* CONFIG_HTTP_CLIENT */

#if defined(CONFIG_HTTP_SERVER)

#include <net/net_context.h>
#include <net/http_parser.h>

struct http_server_ctx;

enum http_url_flags {
	HTTP_URL_STANDARD = 0,
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

typedef int (*http_url_cb_t)(struct http_server_ctx *ctx);

/* HTTP root URL struct, used for pattern matching */
struct http_root_url {
	/** URL */
	const char *root;

	/** Callback that is called when this URL is received */
	http_url_cb_t write_cb;

	/** Length of the URL */
	u16_t root_len;

	/** Flags for this URL (values are from enum http_url_flags) */
	u8_t flags;

	/** Is this URL resource used or not */
	u8_t is_used;
};

/* Collection of URLs that this server will handle */
struct http_server_urls {
	/* First item is the default handler and it is always there.
	 */
	struct http_root_url default_url;
	struct http_root_url urls[CONFIG_HTTP_SERVER_NUM_URLS];
};

#if defined(CONFIG_HTTPS)
/**
 * @typedef https_server_cert_cb_t
 * @brief Callback used when the API user needs to setup the
 * HTTPS certs.
 *
 * @param ctx HTTPS server context.
 * @param cert MBEDTLS certificate
 * @param pkey MBEDTLS private key
 *
 * @return 0 if ok, <0 if there is an error
 */
typedef int (*https_server_cert_cb_t)(struct http_server_ctx *ctx,
				      mbedtls_x509_crt *cert,
				      mbedtls_pk_context *pkey);
#endif /* CONFIG_HTTPS */

/* The HTTP server context struct */
struct http_server_ctx {
	/** Collection of URLs that this server context will handle */
	struct http_server_urls *urls;

#if defined(CONFIG_NET_IPV4)
	/** IPv4 stack network context for listening */
	struct net_context *net_ipv4_ctx;
#endif
#if defined(CONFIG_NET_IPV6)
	/** IPv6 stack network context for listening */
	struct net_context *net_ipv6_ctx;
#endif

	/** Function that is called when data is received from network. */
	net_context_recv_cb_t recv_cb;

	/** Function that is called when data is sent to network. */
	http_send_data_t send_data;

#if defined(CONFIG_NET_DEBUG_HTTP_CONN)
	sys_snode_t node;
#endif

	/** Network timeout */
	s32_t timeout;

	/** Running status of the server. If true, then the server is enabled.
	 * If false then it is disabled and will not serve clients.
	 * The server is disabled by default after initialization and will need
	 * to be enabled manually.
	 */
	bool enabled;

	/** Is this instance HTTPS or not.
	 */
	bool is_https;

	struct {
		/** From which net_context the request came from */
		struct net_context *net_ctx;

		/** HTTP request timer. After sending a response to the
		 * client, it is possible to wait for any request back via
		 * the same socket. If no response is received, then this
		 * timeout is activated and connection is tore down.
		 */
		struct k_delayed_work timer;

		/** HTTP parser */
		struct http_parser parser;

		/** HTTP parser settings */
		struct http_parser_settings settings;

		/** Collection of header fields */
		struct http_field_value
				field_values[CONFIG_HTTP_HEADER_FIELD_ITEMS];

		/** HTTP Request URL */
		const char *url;

		/** Where the request is stored, this is to be provided
		 * by the user.
		 */
		u8_t *request_buf;

		/** Request buffer maximum length */
		size_t request_buf_len;

		/** Length of the data in the request buf. */
		size_t data_len;

		/** Number of header field elements */
		u16_t field_values_ctr;

		/** URL's length */
		u16_t url_len;

		/** Has the request timer been cancelled. */
		u8_t timer_cancelled;
	} req;

#if defined(CONFIG_HTTPS)
	struct {
		/** HTTPS stack for mbedtls library. */
		u8_t *stack;

		/** HTTPS stack size. */
		int stack_size;

		/** HTTPS thread id */
		k_tid_t tid;

		/** HTTPS thread */
		struct k_thread thread;

		/** Memory pool for RX data */
		struct k_mem_pool *pool;

		/** mbedtls related configuration. */
		struct {
			struct https_context ssl_ctx;
			https_server_cert_cb_t cert_cb;
			https_entropy_src_cb_t entropy_src_cb;
			mbedtls_entropy_context entropy;
			mbedtls_ctr_drbg_context ctr_drbg;
			mbedtls_ssl_context ssl;
			mbedtls_ssl_config conf;
			mbedtls_x509_crt srvcert;
			mbedtls_pk_context pkey;
			u8_t *personalization_data;
			size_t personalization_data_len;
		} mbedtls;
	} https;
#endif /* CONFIG_HTTPS */
};

#if defined(CONFIG_NET_DEBUG_HTTP_CONN)
typedef void (*http_server_cb_t)(struct http_server_ctx *entry,
				 void *user_data);

void http_server_conn_foreach(http_server_cb_t cb, void *user_data);
void http_server_conn_monitor(http_server_cb_t cb, void *user_data);
#else
#define http_server_conn_foreach(...)
#define http_server_conn_monitor(...)
#endif /* CONFIG_NET_DEBUG_HTTP_CONN */

/**
 * @brief Initialize user supplied HTTP context.
 *
 * @detail Caller can set the various callback fields in http_ctx and
 * http_ctx.req.parser after this call if needed.
 *
 * @param http_ctx HTTP context.
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
 * @param request_buf Caller supplied buffer where the HTTP request will be
 * stored
 * @param request_buf_len Length of the caller suppied buffer.
 * @param server_banner Print information about started service. This is only
 * printed if HTTP debugging is activated. The parameter can be set to NULL if
 * no extra prints are needed.
 *
 * @return Return 0 if ok, <0 if error.
 */
int http_server_init(struct http_server_ctx *http_ctx,
		     struct http_server_urls *urls,
		     struct sockaddr *server_addr,
		     u8_t *request_buf,
		     size_t request_buf_len,
		     const char *server_banner);

#if defined(CONFIG_HTTPS)
/**
 * @brief Initialize user supplied HTTP context. This function must be
 * used if HTTPS server is created.
 *
 * @detail Caller can set the various callback fields in http_ctx and
 * http_ctx.req.parser after this call if needed.
 *
 * @param http_ctx HTTP context.
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
 * @param request_buf Caller supplied buffer where the HTTP request will be
 * stored
 * @param request_buf_len Length of the caller suppied buffer.
 * @param server_banner Print information about started service. This is only
 * printed if HTTP debugging is activated. The parameter can be set to NULL if
 * no extra prints are needed.
 * @param personalization_data Personalization data (Device specific
 * identifiers) for random number generator. (Can be NULL).
 * @param personalization_data_len Length of the personalization data.
 * @param cert_cb User supplied callback that setups the certifacates.
 * @param entropy_src_cb User supplied callback that setup the entropy. This
 * can be set to NULL, in which case default entropy source is used.
 * @param pool Memory pool for RX data reads.
 * @param https_stack HTTPS thread stack.
 * @param https_stack_len HTTP thread stack size.
 *
 * @return Return 0 if ok, <0 if error.
 */
int https_server_init(struct http_server_ctx *http_ctx,
		      struct http_server_urls *urls,
		      struct sockaddr *server_addr,
		      u8_t *request_buf,
		      size_t request_buf_len,
		      const char *server_banner,
		      u8_t *personalization_data,
		      size_t personalization_data_len,
		      https_server_cert_cb_t cert_cb,
		      https_entropy_src_cb_t entropy_src_cb,
		      struct k_mem_pool *pool,
		      u8_t *https_stack,
		      size_t https_stack_len);
#endif /* CONFIG_HTTPS */

/**
 * @brief Release all the resources allocated for HTTP context.
 *
 * @param http_ctx HTTP context.
 */
void http_server_release(struct http_server_ctx *http_ctx);

/**
 * @brief Enable HTTP server that is related to this context.
 *
 * @detail The HTTP server will start to serve request after this.
 *
 * @param http_ctx HTTP context.
 *
 * @return Previous status of the server.
 */
bool http_server_enable(struct http_server_ctx *http_ctx);

/**
 * @brief Disable HTTP server that is related to this context.
 *
 * @detail The HTTP server will stop to serve request after this.
 *
 * @param http_ctx HTTP context.
 *
 * @return Previous status of the server.
 */
bool http_server_disable(struct http_server_ctx *http_ctx);

/**
 * @brief Helper function that can be used to fill the server (local) sockaddr
 * struct.
 *
 * @param addr Sockaddr struct to be filled.
 * @param myaddr Address that the local IP address. If left NULL, then the
 * proper system address is used.
 * @param port TCP port to use.
 *
 * @return 0 if ok, <0 if error.
 */
int http_server_set_local_addr(struct sockaddr *addr, const char *myaddr,
			       u16_t port);

/**
 * @brief Add a handler for a given URL.
 *
 * @detail Register a handler which is called if the server receives a
 * request to a given URL.
 *
 * @param urls URL struct that will contain all the URLs the user wants to
 * register.
 * @param url URL string.
 * @param flags Flags for the URL.
 * @param write_cb Callback that is called when URL is requested.
 *
 * @return NULL if the URL is already registered, pointer to  URL if
 * registering was ok.
 */
struct http_root_url *http_server_add_url(struct http_server_urls *urls,
					  const char *url, u8_t flags,
					  http_url_cb_t write_cb);

/**
 * @brief Delete the handler for a given URL.
 *
 * @detail Unregister the previously registered URL handler.
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
 * be only one default handler in the URL struct.
 *
 * @param urls URL struct that will contain all the URLs the user has
 * registered.
 * @param write_cb Callback that is called when non-registered URL is
 * requested.
 *
 * @return NULL if default URL is already registered, pointer to default
 * URL if registering was ok.
 */
struct http_root_url *http_server_add_default(struct http_server_urls *urls,
					      http_url_cb_t write_cb);

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

/**
 * @brief Send HTTP response to client.
 *
 * @detail After sending a response, an optional timeout is started
 * which will wait for any new requests from the peer.
 *
 * @param ctx HTTP context.
 * @param http_header HTTP headers to send.
 * @param html_payload HTML payload to send.
 * @param timeout Timeout to wait until the connection is shutdown.
 *
 * @return 0 if ok, <0 if error.
 */
int http_response_wait(struct http_server_ctx *ctx, const char *http_header,
		       const char *html_payload, s32_t timeout);

/**
 * @brief Send HTTP response to client.
 *
 * @detail The connection to peer is torn down right after the response
 * is sent.
 *
 * @param ctx HTTP context.
 * @param http_header HTTP headers to send.
 * @param html_payload HTML payload to send.
 *
 * @return 0 if ok, <0 if error.
 */
int http_response(struct http_server_ctx *ctx, const char *http_header,
		  const char *html_payload);

/**
 * @brief Send HTTP 400 response to client.
 *
 * @detail HTTP 400 error code indicates a Bad Request
 *
 * @param ctx HTTP context.
 * @param html_payload HTML payload to send.
 *
 * @return 0 if ok, <0 if error.
 */
int http_response_400(struct http_server_ctx *ctx, const char *html_payload);

/**
 * @brief Send HTTP 403 response to client.
 *
 * @detail HTTP 403 error code indicates a Forbidden Request
 *
 * @param ctx HTTP context.
 * @param html_payload HTML payload to send.
 *
 * @return 0 if ok, <0 if error.
 */
int http_response_403(struct http_server_ctx *ctx, const char *html_payload);

/**
 * @brief Send HTTP 404 response to client.
 *
 * @detail HTTP 404 error code indicates a Not Found resource.
 *
 * @param ctx HTTP context.
 * @param html_payload HTML payload to send.
 *
 * @return 0 if ok, <0 if error.
 */
int http_response_404(struct http_server_ctx *ctx, const char *html_payload);

#endif /* CONFIG_HTTP_SERVER */

#endif /* __HTTP_H__ */
