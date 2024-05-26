/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/service.h>
#include <zephyr/sys/base64.h>
#include <mbedtls/sha1.h>

LOG_MODULE_DECLARE(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

#include "headers/server_internal.h"

#if !defined(ZEPHYR_USER_AGENT)
#define ZEPHYR_USER_AGENT "Zephyr"
#endif

/* From RFC 6455 chapter 4.2.2 */
#define WS_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* Handle upgrade from HTTP/1.1 to Websocket, see RFC 6455
 */
int handle_http1_to_websocket_upgrade(struct http_client_ctx *client)
{
	static const char switching_protocols[] =
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Connection: Upgrade\r\n"
		"Upgrade: websocket\r\n"
		"Sec-WebSocket-Accept: ";
	char key_accept[32 + sizeof(WS_MAGIC)];
	char accept[20];
	char tmp[64];
	size_t key_len;
	size_t olen;
	int ret;

	key_len = MIN(sizeof(key_accept) - 1, sizeof(client->ws_sec_key));
	strncpy(key_accept, client->ws_sec_key, key_len);
	key_len = strlen(key_accept);

	olen = MIN(sizeof(key_accept) - 1 - key_len, sizeof(WS_MAGIC) - 1);
	strncpy(key_accept + key_len, WS_MAGIC, olen);

	mbedtls_sha1(key_accept, olen + key_len, accept);

	ret = base64_encode(tmp, sizeof(tmp) - 1, &olen, accept, sizeof(accept));
	if (ret) {
		if (ret == -ENOMEM) {
			NET_DBG("[%p] Too short buffer olen %zd", client, olen);
		}

		goto error;
	}

	ret = http_server_sendall(client, switching_protocols,
				  sizeof(switching_protocols) - 1);
	if (ret < 0) {
		NET_DBG("Cannot write to socket (%d)", ret);
		goto error;
	}

	ret = http_server_sendall(client, tmp, strlen(tmp));
	if (ret < 0) {
		NET_DBG("Cannot write to socket (%d)", ret);
		goto error;
	}

	ret = snprintk(tmp, sizeof(tmp), "\r\nUser-Agent: %s\r\n\r\n",
		       ZEPHYR_USER_AGENT);
	if (ret < 0 || ret >= sizeof(tmp)) {
		goto error;
	}

	ret = http_server_sendall(client, tmp, strlen(tmp));
	if (ret < 0) {
		NET_DBG("Cannot write to socket (%d)", ret);
		goto error;
	}

	/* Only after the complete HTTP1 payload has been processed, switch
	 * to Websocket.
	 */
	if (client->parser_state == HTTP1_MESSAGE_COMPLETE_STATE) {
		client->current_detail = NULL;
		client->server_state = HTTP_SERVER_PREFACE_STATE;
		client->cursor += client->data_len;
		client->data_len = 0;
	}

	return 0;

error:
	return ret;
}
