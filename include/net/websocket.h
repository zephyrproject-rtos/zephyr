/** @file
 * @brief Websocket API
 *
 * An API for applications to setup websocket connections
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_H_

#include <kernel.h>

#include <net/net_ip.h>
#include <net/http_parser.h>
#include <net/http_client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Websocket API
 * @defgroup websocket Websocket API
 * @ingroup networking
 * @{
 */

/** Message type values. Returned in websocket_recv_msg() */
#define WEBSOCKET_FLAG_FINAL  0x00000001 /**< Final frame        */
#define WEBSOCKET_FLAG_TEXT   0x00000002 /**< Textual data       */
#define WEBSOCKET_FLAG_BINARY 0x00000004 /**< Binary data        */
#define WEBSOCKET_FLAG_CLOSE  0x00000008 /**< Closing connection */
#define WEBSOCKET_FLAG_PING   0x00000010 /**< Ping message       */
#define WEBSOCKET_FLAG_PONG   0x00000020 /**< Pong message       */

enum websocket_opcode  {
	WEBSOCKET_OPCODE_CONTINUE     = 0x00,
	WEBSOCKET_OPCODE_DATA_TEXT    = 0x01,
	WEBSOCKET_OPCODE_DATA_BINARY  = 0x02,
	WEBSOCKET_OPCODE_CLOSE        = 0x08,
	WEBSOCKET_OPCODE_PING         = 0x09,
	WEBSOCKET_OPCODE_PONG         = 0x0A,
};

/**
 * @typedef websocket_connect_cb_t
 * @brief Callback called after Websocket connection is established.
 *
 * @param ws_sock Websocket id
 * @param req HTTP handshake request
 * @param user_data A valid pointer on some user data or NULL
 *
 * @return 0 if ok, <0 if there is an error and connection should be aborted
 */
typedef int (*websocket_connect_cb_t)(int ws_sock, struct http_request *req,
				      void *user_data);

/**
 * Websocket client connection request. This contains all the data that is
 * needed when doing a Websocket connection request.
 */
struct websocket_request {
	/** Host of the Websocket server when doing HTTP handshakes. */
	const char *host;

	/** URL of the Websocket. */
	const char *url;

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
	 */
	const char **optional_headers;

	/** User supplied callback function to call when a connection is
	 * established.
	 */
	websocket_connect_cb_t cb;

	/** User supplied list of callback functions if the calling application
	 * wants to know the parsing status or the HTTP fields during the
	 * handshake. This is optional parameter and normally not needed but
	 * is useful if the caller wants to know something about
	 * the fields that the server is sending.
	 */
	const struct http_parser_settings *http_cb;

	/** User supplied buffer where HTTP connection data is stored */
	uint8_t *tmp_buf;

	/** Length of the user supplied temp buffer */
	size_t tmp_buf_len;
};

/**
 * @brief Connect to a server that provides Websocket service. The callback is
 * called after connection is established. The returned value is a new socket
 * descriptor that can be used to send / receive data using the BSD socket API.
 *
 * @param http_sock Socket id to the server. Note that this socket is used to do
 *        HTTP handshakes etc. The actual Websocket connectivity is done via the
 *        returned websocket id. Note that the http_sock must not be closed
 *        after this function returns as it is used to deliver the Websocket
 *        packets to the Websocket server.
 * @param req Websocket request. User should allocate and fill the request
 *        data.
 * @param timeout Max timeout to wait for the connection. The timeout value is
 *        in milliseconds. Value SYS_FOREVER_MS means to wait forever.
 * @param user_data User specified data that is passed to the callback.
 *
 * @return Websocket id to be used when sending/receiving Websocket data.
 */
int websocket_connect(int http_sock, struct websocket_request *req,
		      int32_t timeout, void *user_data);

/**
 * @brief Send websocket msg to peer.
 *
 * @details The function will automatically add websocket header to the
 * message.
 *
 * @param ws_sock Websocket id returned by websocket_connect().
 * @param payload Websocket data to send.
 * @param payload_len Length of the data to be sent.
 * @param opcode Operation code (text, binary, ping, pong, close)
 * @param mask Mask the data, see RFC 6455 for details
 * @param final Is this final message for this message send. If final == false,
 *        then the first message must have valid opcode and subsequent messages
 *        must have opcode WEBSOCKET_OPCODE_CONTINUE. If final == true and this
 *        is the only message, then opcode should have proper opcode (text or
 *        binary) set.
 * @param timeout How long to try to send the message. The value is in
 *        milliseconds. Value SYS_FOREVER_MS means to wait forever.
 *
 * @return <0 if error, >=0 amount of bytes sent
 */
int websocket_send_msg(int ws_sock, const uint8_t *payload, size_t payload_len,
		       enum websocket_opcode opcode, bool mask, bool final,
		       int32_t timeout);

/**
 * @brief Receive websocket msg from peer.
 *
 * @details The function will automatically remove websocket header from the
 * message.
 *
 * @param ws_sock Websocket id returned by websocket_connect().
 * @param buf Buffer where websocket data is read.
 * @param buf_len Length of the data buffer.
 * @param message_type Type of the message.
 * @param remaining How much there is data left in the message after this read.
 * @param timeout How long to try to receive the message.
 *        The value is in milliseconds. Value SYS_FOREVER_MS means to wait
 *        forever.
 *
 * @return <0 if error, >=0 amount of bytes received
 */
int websocket_recv_msg(int ws_sock, uint8_t *buf, size_t buf_len,
		       uint32_t *message_type, uint64_t *remaining,
		       int32_t timeout);

/**
 * @brief Close websocket.
 *
 * @details One must call websocket_connect() after this call to re-establish
 * the connection.
 *
 * @param ws_sock Websocket id returned by websocket_connect().
 */
int websocket_disconnect(int ws_sock);

#if defined(CONFIG_WEBSOCKET_CLIENT)
void websocket_init(void);
#else
static inline void websocket_init(void)
{
}
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_WEBSOCKET_H_ */
