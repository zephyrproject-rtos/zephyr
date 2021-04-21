/*
 * Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(websocket_server_handlers, LOG_LEVEL_DBG);

#include "websocket_server_handlers.h"

#define BOARD_REPLY_PREFIX		CONFIG_BOARD" says: "
#define BOARD_REPLY_PREFIX_LEN		sizeof(BOARD_REPLY_PREFIX)

#define BOARD_REPLY_SUFFIX		" too!"
#define BOARD_REPLY_SUFFIX_LEN		sizeof(BOARD_REPLY_SUFFIX)

#define BOARD_REPLY_TOAL_LEN		(BOARD_REPLY_PREFIX_LEN +\
					 BOARD_REPLY_SUFFIX_LEN)

#define FIN_SHIFT		7u
#define RSV1_SHIFT		6u
#define RSV2_SHIFT		5u
#define RSV3_SHIFT		4u
#define OPCODE_SHIFT		0u

#define BOOL_MASK		0x1  /* boolean value mask */
#define HALF_BYTE_MASK		0xF  /* half byte value mask */

#define WS_URL			"/ws"  /* WebSocket server main url */

#define __code_decl	/* static */
#define __data_decl	static

/* Websocket server handlers: */
__code_decl int this_connect_handler(const struct mg_connection *conn,
				     void *cbdata);
__code_decl void this_ready_handler(struct mg_connection *conn, void *cbdata);
__code_decl int this_data_handler(struct mg_connection *conn, int bits,
				  char *data, size_t data_len, void *cbdata);
__code_decl void this_close_handler(const struct mg_connection *conn,
				    void *cbdata);

void init_websocket_server_handlers(struct mg_context *ctx)
{
	mg_set_websocket_handler(ctx, WS_URL,
				this_connect_handler,
				this_ready_handler,
				this_data_handler,
				this_close_handler,
				NULL);
}

__code_decl int this_connect_handler(const struct mg_connection *conn,
				     void *cbdata)
{
	int ret_val = 0;
	return ret_val;
}

__code_decl void this_ready_handler(struct mg_connection *conn, void *cbdata)
{
}

__code_decl int this_data_handler(struct mg_connection *conn, int bits,
				  char *data, size_t data_len, void *cbdata)
{
	int ret_state = 1;

	/* Encode bits as by https://tools.ietf.org/html/rfc6455#section-5.2: */
	const bool FIN = (bits >> FIN_SHIFT) & BOOL_MASK;
	const bool RSV1 = (bits >> RSV1_SHIFT) & BOOL_MASK;
	const bool RSV2 = (bits >> RSV2_SHIFT) & BOOL_MASK;
	const bool RSV3 = (bits >> RSV2_SHIFT) & BOOL_MASK;

	uint8_t OPCODE = (bits >> OPCODE_SHIFT) & HALF_BYTE_MASK;

	(void)FIN;
	(void)RSV1;
	(void)RSV2;
	(void)RSV3;

	LOG_DBG("got bits: %d", bits);
	LOG_DBG("\t\twith OPCODE: %d", OPCODE);

	/* TODO: Protect resp_data with semaphore */
	const size_t resp_data_len = BOARD_REPLY_TOAL_LEN + data_len;

	if (resp_data_len > CONFIG_MAIN_STACK_SIZE) {
		/* Close connection due to no memory */
		OPCODE = MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE;
	}

	char resp_data[resp_data_len];

	/* Process depending of opcode: */
	switch (OPCODE) {
	case MG_WEBSOCKET_OPCODE_CONTINUATION:
		break;
	case MG_WEBSOCKET_OPCODE_TEXT:
		memcpy(resp_data,
			BOARD_REPLY_PREFIX, BOARD_REPLY_PREFIX_LEN);
		memcpy(resp_data + BOARD_REPLY_PREFIX_LEN,
			data, data_len);
		memcpy(resp_data + BOARD_REPLY_PREFIX_LEN + data_len,
			BOARD_REPLY_SUFFIX, BOARD_REPLY_SUFFIX_LEN);

		ret_state = mg_websocket_write(conn, OPCODE, resp_data,
						resp_data_len);
		break;
	case MG_WEBSOCKET_OPCODE_BINARY:
		ret_state = 0;
		mg_websocket_write(conn,
			MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE, NULL, 0);

		LOG_ERR("Binary data not supported currently: "
			"close connection");
		break;
	case MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE:
		ret_state = 0;
		mg_websocket_write(conn, OPCODE, NULL, 0);
		break;
	case MG_WEBSOCKET_OPCODE_PING:
		break;
	case MG_WEBSOCKET_OPCODE_PONG:
		break;
	default:
		ret_state = 0;
		mg_websocket_write(conn,
			MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE, NULL, 0);

		LOG_ERR("Unknown OPCODE: close connection");
		break;
	}

	if (ret_state < 0) {
		/* TODO: Maybe need we close WS connection here?! */
		LOG_ERR("Unknown ERROR: ret_state = %d", ret_state);
	} else if (ret_state == 0) {
		LOG_DBG("Close WS sonnection: ret_state = %d", ret_state);
	}

	return ret_state;
}

__code_decl void this_close_handler(const struct mg_connection *conn,
				    void *cbdata)
{
}
