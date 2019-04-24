/*
 * Copyright (c) 2019 Unisoc Technologies INC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief HTTP client high-level api implementation for Zephyr.
 */

#ifndef __ZEPHYR_HTTP_CLIENT_H__
#define __ZEPHYR_HTTP_CLIENT_H__

#include <net/http_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Http client library
 * @defgroup http client Library
 * @ingroup networking
 * @{
 */

#ifdef CONFIG_HTTPS
	#define CONFIG_NET_SOCKETS_SOCKOPT_TLS
#endif

#if !defined(ZEPHYR_USER_AGENT)
#define ZEPHYR_USER_AGENT "Zephyr"
#endif

#if !defined(HTTP_PROTOCOL)
#define HTTP_PROTOCOL	   "HTTP/1.1"
#endif

#define HTTP_CRLF "\r\n"
#define HTTP_STATUS_OK			"OK"
#define HTTP_PARTIAL_CONTENT	"Partial Content"

#define HOST_NAME_MAX_SIZE		32
#define HTTP_REQUEST_MAX_SIZE	1024
#define HTTP_RESPONSE_MAX_SIZE	4096
#define HTTP_STATUS_STR_SIZE	16

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	#define HTTP_PORT "443"
#else
	#define HTTP_PORT "80"
#endif

struct http_ctx;

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

/* Is there more data to come */
enum http_final_call {
	HTTP_DATA_MORE = 0,
	HTTP_DATA_FINAL = 1,
};

/* http header filed and value */
enum http_header_field_val {
	HTTP_HEADER_FIELD = 0,
	HTTP_HEADER_VALUE = 1,
};

/**
 * @typedef http_response_cb_t
 * @brief Callback used when a response has been received from peer.
 *
 * @param ctx HTTP context.
 * @param data Received body point
 * @param received body length
 * @param final_data Does this data buffer contain all the data or
 * is there still more data to come.
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*http_response_cb_t)(struct http_ctx *ctx,
					const char *body,
					int body_len,
					enum http_final_call final_data);

/**
 * @typedef http_get_filed_value_cb_ts
 * @brief Callback used when got http header filed and value.
 *
 * @param ctx HTTP context.
 * @param data of header
 * @param header field or value length
 * @param http_header_field_val indicate it's filed or value
 */
typedef void (*http_get_filed_value_cb_t)(struct http_ctx *ctx,
					const char *at,
					int at_len,
					enum http_header_field_val f_v);
/**
 * HTTP client request. This contains all the data that is needed when doing
 * a HTTP request.
 */
struct http_request {
	/** The HTTP method: GET, HEAD, OPTIONS, POST, ... */
	const char *method_str;

	/** The URL for this request, for example: /index.html */
	const char *path;

	/** The HTTP header with optional*/
	const char *header_fields;

	/** The size of HTTP header fields */
	const u16_t header_fields_size;

	/** http header keep-alive*/
	const bool keep_alive;

};

struct http_resp {
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

	u8_t cl_present;
	u8_t body_found;
	u8_t message_complete;
};

/**
 * Http context information. This contains all the data that is
 * needed when working with http API.
 */
struct http_ctx {

	/** socket id with http conenct*/
	u16_t sock_id;

	/** Server socket addr */
	struct addrinfo *addr_info;
	char host[HOST_NAME_MAX_SIZE];

	/** HTTP request information */
	struct http_request req;
	char *req_buf;

	/** HTTP response information */
	struct http_resp rsp;
	char *rsp_buf;

	/** User provided HTTP response callback which is
	 * called when a response is received to a sent HTTP
	 * request.
	 */
	http_response_cb_t rsp_cb;

	/** User provided to parse field-value what user care about
	 * it, such as Content-Range to get the resource file size
	 */
	http_get_filed_value_cb_t fv_cb;

	/** HTTP parser for parsing the initial request */
	struct http_parser parser;

	/** HTTP parser settings */
	struct http_parser_settings parser_settings;
};

/**
 * @brief Initialize user-supplied HTTP context.
 *
 * @detail Caller can set the various fields in http_ctx after this call
 * if needed.
 *
 * @param http_ctx HTTP context.
 * @param host HTTP request host name.
 * @param port http server port number.
 * @param tls http connect is https or not
 *
 * @return Return 0 if ok, and <0 if error.
 */
int
http_client_init(struct http_ctx *ctx,
				 char *host,
				 char *port,
				 bool tls);
/**
 *@brief Generic function to get a HTTP request to the network.
 *
 * @param ctx HTTP context.
 * @param path http request path, like /index.html.
 * @param keep_alive http connect is alive or not
 * @param user_data User data related to this request. This is passed
 * to send callback if user has specified that.
 *
 * @return Return 0 if ok, and <0 if error.
 */
int http_client_get(struct http_ctx *ctx,
		char *path,
		bool keep_alive,
		void *user_data);

/**
 * @brief Generic function to post a HTTP request to the network.
 *
 * @param ctx HTTP context.
 * @param path http request path, like /index.html.
 * @param keep_alive http connect is alive or not
 * @param user_data User data related to this request. This is passed
 * to send callback if user has specified that.
 *
 * @return Return 0 if ok, and <0 if error.
 */
int http_client_post(struct http_ctx *ctx,
		char *path,
		bool keep_alive,
		void *user_data);

/**
 * @brief Close a network connection to peer.
 *
 * @param ctx Http context.
 *
 * @return 0 if ok, <0 if error.
 */
int http_client_close(struct http_ctx *ctx);

/**
 * @brief Add HTTP header field to the message.
 *
 * @details This can be called multiple times to add pieces of HTTP header
 * fields into the message. Caller must put the "\r\n" characters to the
 * input http_header_field variable.
 *
 * @param ctx Http context.
 * @param http_header_field All or part of HTTP header to be added.
 *
 * @return void
 */
void http_add_header_field(struct http_ctx *ctx,
			char *header_fields);

/**
 * @brief set http response callback.
 *
 * @details user can call this function to set http response callback,
 * then it will got the response body data.
 *
 * @param ctx Http context.
 * @param http_response_cb_t function of response callback.
 *
 * @return void
 */
void http_set_resp_callback(struct http_ctx *ctx,
			http_response_cb_t resp_cb);
/**
 * @brief set parse http response header field and value.
 *
 * @details user can call this function to set parse filed and value of
 * response header, such as Content-Range to get resource size,
 * and ETag to get the resource brief of resource...
 *
 * @param ctx Http context.
 * @param http_response_cb_t function of response callback.
 *
 * @return void
 */
void http_set_fv_callback(struct http_ctx *ctx,
			http_get_filed_value_cb_t fv_cb);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ZEPHYR_HTTP_CLIENT_H__ */
