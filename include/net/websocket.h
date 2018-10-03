/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WEBSOCKET_H_
#define ZEPHYR_INCLUDE_NET_WEBSOCKET_H_

#include <net/http.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Websocket library
 * @defgroup websocket Websocket Library
 * @{
 */

/** Values for flag variable in HTTP receive callback */
#define WS_FLAG_FINAL  0x00000001
#define WS_FLAG_TEXT   0x00000002
#define WS_FLAG_BINARY 0x00000004
#define WS_FLAG_CLOSE  0x00000008
#define WS_FLAG_PING   0x00000010
#define WS_FLAG_PONG   0x00000011

enum ws_opcode  {
	WS_OPCODE_CONTINUE     = 0x00,
	WS_OPCODE_DATA_TEXT    = 0x01,
	WS_OPCODE_DATA_BINARY  = 0x02,
	WS_OPCODE_CLOSE        = 0x08,
	WS_OPCODE_PING         = 0x09,
	WS_OPCODE_PONG         = 0x0A,
};

/**
 * @brief Send websocket msg to peer.
 *
 * @details The function will automatically add websocket header to the
 * message.
 *
 * @param ctx Websocket context.
 * @param payload Websocket data to send.
 * @param payload_len Length of the data to be sent.
 * @param opcode Operation code (text, binary, ping, pong, close)
 * @param mask Mask the data, see RFC 6455 for details
 * @param final Is this final message for this message send. If final == false,
 * then the first message must have valid opcode and subsequent messages must
 * have opcode WS_OPCODE_CONTINUE. If final == true and this is the only
 * message, then opcode should have proper opcode (text or binary) set.
 * @param dst Remote socket address
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
int ws_send_msg(struct http_ctx *ctx, u8_t *payload, size_t payload_len,
		enum ws_opcode opcode, bool mask, bool final,
		const struct sockaddr *dst,
		void *user_send_data);

/**
 * @brief Send message to client.
 *
 * @details The function will automatically add websocket header to the
 * message.
 *
 * @param ctx Websocket context.
 * @param payload Websocket data to send.
 * @param payload_len Length of the data to be sent.
 * @param opcode Operation code (text, binary, ping ,pong ,close)
 * @param final Is this final message for this data send
 * @param dst Remote socket address
 * @param user_send_data User specific data to this connection. This is passed
 * as a parameter to sent cb after the packet has been sent.
 *
 * @return 0 if ok, <0 if error.
 */
static inline int ws_send_msg_to_client(struct http_ctx *ctx,
					u8_t *payload,
					size_t payload_len,
					enum ws_opcode opcode,
					bool final,
					const struct sockaddr *dst,
					void *user_send_data)
{
	return ws_send_msg(ctx, payload, payload_len, opcode, false, final,
			   dst, user_send_data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __WS_H__ */
