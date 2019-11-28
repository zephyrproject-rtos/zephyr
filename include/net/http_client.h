/** @file
 * @brief HTTP client API
 *
 * An API for applications do HTTP requests
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_CLIENT_H_
#define ZEPHYR_INCLUDE_NET_HTTP_CLIENT_H_

/**
 * @brief HTTP client API
 * @defgroup http_client HTTP client API
 * @ingroup networking
 * @{
 */

#include <net/net_ip.h>
#include <net/http_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HTTP_CRLF)
#define HTTP_CRLF "\r\n"
#endif

#if !defined(HTTP_STATUS_STR_SIZE)
#define HTTP_STATUS_STR_SIZE	32
#endif

/* Is there more data to come */
enum http_final_call {
	HTTP_DATA_MORE = 0,
	HTTP_DATA_FINAL = 1,
};

struct http_request;
struct http_response;

/**
 * @typedef http_payload_cb_t
 * @brief Callback used when data needs to be sent to the server.
 *
 * @param sock Socket id of the connection
 * @param req HTTP request information
 * @param user_data User specified data specified in http_client_req()
 *
 * @return >=0 amount of data sent, in this case http_client_req() should
 *             continue sending data,
 *         <0  if http_client_req() should return the error code to the
 *             caller.
 */
typedef int (*http_payload_cb_t)(int sock,
				 struct http_request *req,
				 void *user_data);

/**
 * @typedef http_header_cb_t
 * @brief Callback can be used if application wants to construct additional
 * HTTP headers when the HTTP request is sent. Usage of this is optional.
 *
 * @param sock Socket id of the connection
 * @param req HTTP request information
 * @param user_data User specified data specified in http_client_req()
 *
 * @return >=0 amount of data sent, in this case http_client_req() should
 *             continue sending data,
 *         <0  if http_client_req() should return the error code to the
 *             caller.
 */
typedef int (*http_header_cb_t)(int sock,
				struct http_request *req,
				void *user_data);

/**
 * @typedef http_response_cb_t
 * @brief Callback used when data is received from the server.
 *
 * @param rsp HTTP response information
 * @param final_data Does this data buffer contain all the data or
 *        is there still more data to come.
 * @param user_data User specified data specified in http_client_req()
 */
typedef void (*http_response_cb_t)(struct http_response *rsp,
				   enum http_final_call final_data,
				   void *user_data);

/**
 * HTTP response from the server.
 */
struct http_response {
	/** HTTP parser settings for the application usage */
	const struct http_parser_settings *http_cb;

	/** User provided HTTP response callback which is
	 * called when a response is received to a sent HTTP
	 * request.
	 */
	http_response_cb_t cb;

	/** Where the body starts */
	u8_t *body_start;

	/** Where the response is stored, this is to be
	 * provided by the user.
	 */
	u8_t *recv_buf;

	/** Response buffer maximum length */
	size_t recv_buf_len;

	/** Length of the data in the result buf. If the value
	 * is larger than recv_buf_len, then it means that
	 * the data is truncated and could not be fully copied
	 * into recv_buf. This can only happen if the user
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

	u8_t cl_present : 1;
	u8_t body_found : 1;
	u8_t message_complete : 1;
};

/** HTTP client internal data that the application should not touch
 */
struct http_client_internal_data {
	/** Work for handling timeout */
	struct k_delayed_work work;

	/** HTTP parser context */
	struct http_parser parser;

	/** HTTP parser settings */
	struct http_parser_settings parser_settings;

	/** HTTP response specific data (filled by http_client_req() when
	 * data is received)
	 */
	struct http_response response;

	/** User data */
	void *user_data;

	/** HTTP socket */
	int sock;

	/** Request timeout */
	s32_t timeout;
};

/**
 * HTTP client request. This contains all the data that is needed when doing
 * a HTTP request.
 */
struct http_request {
	/** HTTP client request internal data */
	struct http_client_internal_data internal;

	/* User should fill in following parameters */

	/** The HTTP method: GET, HEAD, OPTIONS, POST, ... */
	enum http_method method;

	/** User supplied callback function to call when response is
	 * received.
	 */
	http_response_cb_t response;

	/** User supplied list of HTTP callback functions if the
	 * calling application wants to know the parsing status or the HTTP
	 * fields. This is optional and normally not needed.
	 */
	const struct http_parser_settings *http_cb;

	/** User supplied buffer where received data is stored */
	u8_t *recv_buf;

	/** Length of the user supplied receive buffer */
	size_t recv_buf_len;

	/** The URL for this request, for example: /index.html */
	const char *url;

	/** The HTTP protocol, for example "HTTP/1.1" */
	const char *protocol;

	/** The HTTP header fields (application specific)
	 * The Content-Type may be specified here or in the next field.
	 * Depending on your application, the Content-Type may vary, however
	 * some header fields may remain constant through the application's
	 * life cycle. This is a NULL terminated list of header fields.
	 */
	const char **header_fields;

	/** The value of the Content-Type header field, may be NULL */
	const char *content_type_value;

	/** Hostname to be used in the request */
	const char *host;

	/** User supplied callback function to call when payload
	 * needs to be sent. This can be NULL in which case the payload field
	 * in http_request is used. The idea of this payload callback is to
	 * allow user to send more data that is practical to store in allocated
	 * memory.
	 */
	http_payload_cb_t payload_cb;

	/** Payload, may be NULL */
	const char *payload;

	/** Payload length, may be 0. Only used if payload field is not NULL */
	size_t payload_len;

	/** User supplied callback function to call when optional headers need
	 * to be sent. This can be NULL, in which case the optional_headers
	 * field in http_request is used. The idea of this optional_headers
	 * callback is to allow user to send more HTTP header data that is
	 * practical to store in allocated memory.
	 */
	http_header_cb_t optional_headers_cb;

	/** A NULL terminated list of any optional headers that
	 * should be added to the HTTP request. May be NULL.
	 * If the optional_headers_cb is specified, then this field is ignored.
	 * Note that there are two similar fields that contain headers,
	 * the header_fields above and this optional_headers. This is done
	 * like this to support Websocket use case where Websocket will use
	 * header_fields variable and any optional application specific
	 * headers will be placed into this field.
	 */
	const char **optional_headers;
};

/**
 * @brief Do a HTTP request. The callback is called when data is received
 * from the HTTP server. The caller must have created a connection to the
 * server before calling this function so connect() call must have be done
 * successfully for the socket.
 *
 * @param sock Socket id of the connection.
 * @param req HTTP request information
 * @param timeout Max timeout to wait for the data. The timeout value cannot be
 *        0 as there would be no time to receive the data.
 * @param user_data User specified data that is passed to the callback.
 *
 * @return <0 if error, >=0 amount of data sent to the server
 */
int http_client_req(int sock, struct http_request *req,
		    s32_t timeout, void *user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_HTTP_CLIENT_H_ */
