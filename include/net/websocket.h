/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WS_H__
#define __WS_H__

#include <net/net_app.h>
#include <net/http_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(CONFIG_WS_HTTP_SERVER_NUM_URLS)
#define CONFIG_WS_HTTP_SERVER_NUM_URLS 1
#endif

#if !defined(CONFIG_WS_HTTP_HEADER_FIELD_ITEMS)
#define CONFIG_WS_HTTP_HEADER_FIELD_ITEMS 1
#endif

/**
 * @brief Websocket client and server library
 * @defgroup ws Websocket Library
 * @{
 */

struct ws_ctx;

/** Values for flag variable in receive callback */
#define WS_FLAG_FINAL  0x00000001
#define WS_FLAG_TEXT   0x00000002
#define WS_FLAG_BINARY 0x00000004
#define WS_FLAG_CLOSE  0x00000008
#define WS_FLAG_PING   0x00000010
#define WS_FLAG_PONG   0x00000011

enum ws_state {
	  WS_STATE_CLOSED,
	  WS_STATE_WAITING_HEADER,
	  WS_STATE_RECEIVING_HEADER,
	  WS_STATE_HEADER_RECEIVED,
	  WS_STATE_OPEN,
};

enum ws_url_flags {
#if defined(CONFIG_WS)
	HTTP_URL_STANDARD = 0,
#endif
	HTTP_URL_WS = 1,
};

enum ws_connection_type {
	WS_HTTP_CONNECT = 1,
	WS_CONNECT = 2,
};

enum ws_opcode  {
	WS_OPCODE_CONTINUE     = 0x00,
	WS_OPCODE_DATA_TEXT    = 0x01,
	WS_OPCODE_DATA_BINARY  = 0x02,
	WS_OPCODE_CLOSE        = 0x08,
	WS_OPCODE_PING         = 0x09,
	WS_OPCODE_PONG         = 0x0A,
};

/* HTTP header fields struct */
#if defined(CONFIG_WS)
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
#endif

/* HTTP root URL struct, used for pattern matching */
struct ws_root_url {
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

enum ws_verdict {
	WS_VERDICT_DROP,
	WS_VERDICT_ACCEPT,
};

/**
 * @typedef ws_url_cb_t
 * @brief Call default URL callback.
 *
 * @param ctx The context to use.
 * @param type Connection type (websocket or HTTP)
 *
 * @return WS_VERDICT_DROP if connection is to be dropped, WS_VERDICT_ACCEPT
 * if the application wants to accept the unknown URL.
 */
typedef enum ws_verdict (*ws_url_cb_t)(struct ws_ctx *ctx,
				       enum ws_connection_type type);

/* Collection of URLs that this server will handle */
struct ws_server_urls {
	/* First item is the default handler and it is always there.
	 */
	struct ws_root_url default_url;

	/** Callback that is called when unknown (default) URL is received */
	ws_url_cb_t default_cb;

	struct ws_root_url urls[CONFIG_WS_HTTP_SERVER_NUM_URLS];
};

/**
 * @typedef ws_recv_cb_t
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
 * @param flags Flags related to websocket. For example contains information
 * if the data is text or binary etc.
 * @param user_data The user data given in init call.
 */
typedef void (*ws_recv_cb_t)(struct ws_ctx *ctx,
			     struct net_pkt *pkt,
			     int status,
			     u32_t flags,
			     void *user_data);

/**
 * @typedef ws_connect_cb_t
 * @brief Connection callback.
 *
 * @details The connect callback is called after some valid URL is
 * connected.
 *
 * @param ctx The context to use.
 * @param type Connection type (websocket or HTTP)
 * @param user_data The user data given in init call.
 */
typedef void (*ws_connect_cb_t)(struct ws_ctx *ctx,
				enum ws_connection_type type,
				void *user_data);

/**
 * @typedef ws_send_cb_t
 * @brief Network data send callback.
 *
 * @details The send callback is called after a network data is
 * sent.
 *
 * @param ctx The context to use.
 * @param status Value is set to 0 if all data was sent ok, <0 if
 * there was an error sending data. >0 amount of data that was
 * sent when not all data was sent ok.
 * @param user_data_send The user data given in ws_send() call.
 * @param user_data The user data given in init call.
 */
typedef void (*ws_send_cb_t)(struct ws_ctx *ctx,
			     int status,
			     void *user_data_send,
			     void *user_data);

/**
 * @typedef ws_close_cb_t
 * @brief Close callback.
 *
 * @details The close callback is called after a connection is being
 * shutdown.
 *
 * @param ctx The context to use.
 * @param status Error code for the closing.
 * @param user_data The user data given in init call.
 */
typedef void (*ws_close_cb_t)(struct ws_ctx *ctx,
			      int status,
			      void *user_data);

/** Websocket and HTTP callbacks */
struct ws_cb {
	/** Function that is called when a connection is established.
	 */
	ws_connect_cb_t connect;

	/** Function that is called when data is received from network.
	 */
	ws_recv_cb_t recv;

	/** Function that is called when net_pkt is sent.
	 */
	ws_send_cb_t send;

	/** Function that is called when connection is shutdown.
	 */
	ws_close_cb_t close;
};

/**
 * Websocket context information. This contains all the data that is
 * needed when working with websocket API.
 */
struct ws_ctx {
	/** Net app context. The websocket connection is handled via
	 * the net app API.
	 */
	struct net_app_ctx app_ctx;

	/** Local endpoint IP address */
	struct sockaddr local;

	/** Original server address */
	struct sockaddr *server_addr;

	struct {
		/** Collection of HTTP URLs that this context will handle. */
		struct ws_server_urls *urls;

		/** HTTP URL parser */
		struct http_parser_url parsed_uri;

		/** HTTP parser for parsing the initial request */
		struct http_parser parser;

		/** HTTP parser settings */
		struct http_parser_settings parser_settings;

		/** Collection of HTTP header fields */
#if defined(CONFIG_WS)
		struct http_field_value
			field_values[CONFIG_WS_HTTP_HEADER_FIELD_ITEMS];
#endif

		/** HTTP Request URL */
		const char *url;

		/** Length of the data in the request buf. */
		size_t data_len;

		/** URL's length */
		u16_t url_len;

		/** Number of header field elements */
		u16_t field_values_ctr;
	} http;

#if defined(CONFIG_NET_DEBUG_WS_CONN)
	sys_snode_t node;
#endif

	/** Websocket and HTTP callbacks */
	struct ws_cb cb;

	/** User specified data that is passed in callbacks. */
	u8_t *user_data;

	/** Where the request is stored, this is to be provided by the user.
	 */
	u8_t *request_buf;

	/** Request buffer maximum length */
	size_t request_buf_len;

	/** State of the websocket */
	enum ws_state state;

	/** Network buffer allocation timeout */
	s32_t timeout;

	/** Websocket endpoint address */
	struct sockaddr *addr;

	/** Websocket endpoint port */
	u16_t port;

	/** Is this context setup or not */
	u8_t is_init : 1;

	/** Is this instance supporting TLS or not.
	 */
	u8_t is_tls : 1;
};

#if defined(CONFIG_WS_SERVER)
/**
 * @brief Create a websocket listener.
 *
 * @details Note that the context must be valid for the whole duration of the
 * websocket life cycle. This usually means that it cannot be allocated from
 * stack.
 *
 * @param ctx Websocket context. Ths init function will initialize it.
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
int ws_server_init(struct ws_ctx *ctx,
		   struct ws_server_urls *urls,
		   struct sockaddr *server_addr,
		   u8_t *request_buf,
		   size_t request_buf_len,
		   const char *server_banner,
		   void *user_data);

#if defined(CONFIG_WS_TLS)
/**
 * @brief Initialize TLS support for this websocket context
 *
 * @param ctx Websocket context
 * @param server_banner Print information about started service. This is only
 * printed if net_app debugging is activated. The parameter can be set to NULL
 * if no extra prints are needed.
 * @param personalization_data Personalization data (Device specific
 * identifiers) for random number generator. (Can be NULL).
 * @param personalization_data_len Length of the personalization data.
 * @param cert_cb User supplied callback that setups the certifacates.
 * @param entropy_src_cb User supplied callback that setup the entropy. This
 * can be set to NULL, in which case default entropy source is used.
 * @param pool Memory pool for RX data reads.
 * @param stack TLS thread stack.
 * @param stack_len TLS thread stack size.
 *
 * @return Return 0 if ok, <0 if error.
 */
int ws_server_set_tls(struct ws_ctx *ctx,
		      const char *server_banner,
		      u8_t *personalization_data,
		      size_t personalization_data_len,
		      net_app_cert_cb_t cert_cb,
		      net_app_entropy_src_cb_t entropy_src_cb,
		      struct k_mem_pool *pool,
		      k_thread_stack_t stack,
		      size_t stack_len);

#endif /* CONFIG_WS_TLS */

/**
 * @brief Enable HTTP server that is related to this context.
 *
 * @detail The HTTP server will start to serve request after this.
 *
 * @param ctx Websocket context.
 *
 * @return 0 if server is enabled, <0 otherwise
 */
int ws_server_enable(struct ws_ctx *ctx);

/**
 * @brief Disable HTTP server that is related to this context.
 *
 * @detail The HTTP server will stop to serve request after this.
 *
 * @param ctx Websocket context.
 *
 * @return 0 if server is disabled, <0 if there was an error
 */
int ws_server_disable(struct ws_ctx *ctx);

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
struct ws_root_url *ws_server_add_url(struct ws_server_urls *urls,
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
int ws_server_del_url(struct ws_server_urls *urls, const char *url);

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
struct ws_root_url *ws_server_add_default(struct ws_server_urls *urls,
					  ws_url_cb_t cb);

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
int ws_server_del_default(struct ws_server_urls *urls);

#else /* CONFIG_WS_SERVER */

static inline int ws_server_init(struct ws_ctx *ctx,
				 struct ws_server_urls *urls,
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

#if defined(CONFIG_WS_TLS)
static inline int ws_server_set_tls(struct ws_ctx *ctx,
				    const char *server_banner,
				    u8_t *personalization_data,
				    size_t personalization_data_len,
				    net_app_cert_cb_t cert_cb,
				    net_app_entropy_src_cb_t entropy_src_cb,
				    struct k_mem_pool *pool,
				    k_thread_stack_t stack,
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
#endif /* CONFIG_WS_TLS */

static inline int ws_server_enable(struct ws_ctx *ctx)
{
	ARG_UNUSED(ctx);
	return -ENOTSUP;
}

static inline int ws_server_disable(struct ws_ctx *ctx)
{
	ARG_UNUSED(ctx);
	return -ENOTSUP;
}

static inline
struct ws_root_url *ws_server_add_url(struct ws_server_urls *urls,
				      const char *url, u8_t flags,
				      ws_url_cb_t write_cb)
{
	ARG_UNUSED(urls);
	ARG_UNUSED(url);
	ARG_UNUSED(flags);
	ARG_UNUSED(write_cb);

	return NULL;
}

#endif /* CONFIG_WS_SERVER */

/**
 * @brief Close a network connection to peer.
 *
 * @param ctx Websocket context.
 *
 * @return 0 if ok, <0 if error.
 */
int ws_close(struct ws_ctx *ctx);

/**
 * @brief Release this websocket context.
 *
 * @details No network data will be received via this context after this
 * call.
 *
 * @param ctx Websocket context.
 *
 * @return 0 if ok, <0 if error.
 */
int ws_release(struct ws_ctx *ctx);

/**
 * @brief Set various callbacks that are called at various stage of ws session.
 *
 * @param ctx Websocket context.
 * @param connect_cb Connect callback.
 * @param recv_cb Data receive callback.
 * @param send_cb Data sent callback.
 * @param close_cb Close callback.
 *
 * @return 0 if ok, <0 if error.
 */
int ws_set_cb(struct ws_ctx *ctx,
	      ws_connect_cb_t connect_cb,
	      ws_recv_cb_t recv_cb,
	      ws_send_cb_t send_cb,
	      ws_close_cb_t close_cb);

/**
 * @brief Send data to peer. The data can be either HTTP or websocket data.
 *
 * @details This does not modify the network packet but sends it as is.
 *
 * @param ctx Websocket context.
 * @param pkt Network packet to send
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
int ws_send_data_raw(struct ws_ctx *ctx, struct net_pkt *pkt,
		     void *user_send_data);

/**
 * @brief Send data to peer.
 *
 * @details The function will automatically add websocket header to the
 * message.
 *
 * @param ctx Websocket context.
 * @param pkt Network packet to send. This can be left NULL if no user data
 * is to be sent.
 * @param opcode Operation code (text, binary, ping ,pong ,close)
 * @param mask Mask the data, see RFC 6455 for details
 * @param final Is this final message for this data send
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
int ws_send_data(struct ws_ctx *ctx, struct net_pkt *pkt,
		 enum ws_opcode opcode, bool mask, bool final,
		 void *user_send_data);

/**
 * @brief Send HTTP data to peer.
 *
 * @param ctx Websocket context.
 * @param pkt Network packet to send
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
static inline int ws_send_data_http(struct ws_ctx *ctx, struct net_pkt *pkt,
				    void *user_send_data)
{
	return ws_send_data_raw(ctx, pkt, user_send_data);
}

/**
 * @brief Send HTTP error message to peer.
 *
 * @param ctx Websocket context.
 * @param code HTTP error code
 * @param html_payload Extra payload, can be null
 * @param html_len Payload length
 *
 * @return 0 if ok, <0 if error.
 */
int ws_send_http_error(struct ws_ctx *ctx, int code, u8_t *html_payload,
		       size_t html_len);

/**
 * @brief Send data to client.
 *
 * @details The function will automatically add websocket header to the
 * message.
 *
 * @param ctx Websocket context.
 * @param pkt Network packet to send
 * @param opcode Operation code (text, binary, ping ,pong ,close)
 * @param final Is this final message for this data send
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
static inline int ws_send_data_to_client(struct ws_ctx *ctx,
					 struct net_pkt *pkt,
					 enum ws_opcode opcode,
					 bool final,
					 void *user_send_data)
{
	return ws_send_data(ctx, pkt, opcode, false, final, user_send_data);
}

/**
 * @brief Add HTTP header to the message.
 *
 * @details This can be called multiple times to add pieces of HTTP header into
 * the message.
 *
 * @param ctx Websocket context.
 * @param pkt Network packet to send. If *pkt is NULL, then the API
 * will allocate network packet and place data into it.
 * @param http_header All or part of HTTP header to be added.
 *
 * @return 0 if ok, <0 if error.
 */
int ws_add_http_header(struct ws_ctx *ctx, struct net_pkt **pkt,
		       const char *http_header);

/**
 * @brief Add HTTP data to the message.
 *
 * @details This can be called multiple times to add pieces of HTTP data into
 * the message.
 *
 * @param ctx Websocket context.
 * @param pkt Network packet to send. If *pkt is NULL, then the API
 * will allocate network packet and place data into it.
 * @param buf Buffer that contains the data
 * @param len Length of the buffer
 *
 * @return 0 if ok, <0 if error.
 */
int ws_add_http_data(struct ws_ctx *ctx, struct net_pkt **pkt,
		     const u8_t *buf, size_t len);

/**
 * @brief Add data to the message.
 *
 * @details This can be called multiple times to add pieces of ws data into
 * the message.
 *
 * @param ctx Websocket context.
 * @param pkt Network packet to send. If *pkt is NULL, then the API
 * will allocate network packet and place data into it.
 * @param buf Buffer that contains the data
 * @param len Length of the buffer
 *
 * @return 0 if ok, <0 if error.
 */
int ws_add_data(struct ws_ctx *ctx, struct net_pkt **pkt,
		const u8_t *buf, size_t len);

#if defined(CONFIG_NET_DEBUG_WS_CONN)
typedef void (*ws_server_cb_t)(struct ws_ctx *entry,
				      void *user_data);

void ws_server_conn_foreach(ws_server_cb_t cb, void *user_data);
void ws_server_conn_monitor(ws_server_cb_t cb, void *user_data);
#else
#define ws_server_conn_foreach(...)
#define ws_server_conn_monitor(...)
#endif /* CONFIG_NET_DEBUG_WS_CONN */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __WS_H__ */
