/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_WEBSOCKET)
#define SYS_LOG_DOMAIN "ws"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <version.h>

#include <net/net_ip.h>
#include <net/websocket.h>

#include <base64.h>
#include <mbedtls/sha1.h>

#define BUF_ALLOC_TIMEOUT 100

#define HTTP_CRLF "\r\n"

/* From RFC 6455 chapter 4.2.2 */
#define WS_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

static void ws_mask_payload(u8_t *payload, size_t payload_len,
			    u32_t masking_value)
{
	int i;

	for (i = 0; i < payload_len; i++) {
		payload[i] ^= masking_value >> (8 * (3 - i % 4));
	}
}

void ws_mask_pkt(struct net_pkt *pkt, u32_t masking_value, u32_t *data_read)
{
	struct net_buf *frag;
	u16_t pos;
	int i;

	frag = net_frag_get_pos(pkt,
				net_pkt_get_len(pkt) - net_pkt_appdatalen(pkt),
				&pos);
	if (!frag) {
		return;
	}

	NET_ASSERT(net_pkt_appdata(pkt) == frag->data + pos);

	while (frag) {
		for (i = pos; i < frag->len; i++, (*data_read)++) {
			frag->data[i] ^=
				masking_value >> (8 * (3 - (*data_read) % 4));
		}

		pos = 0;
		frag = frag->frags;
	}
}

int ws_send_msg(struct http_ctx *ctx, u8_t *payload, size_t payload_len,
		enum ws_opcode opcode, bool mask, bool final,
		const struct sockaddr *dst,
		void *user_send_data)
{
	u8_t header[14], hdr_len = 2;
	int ret;

	if (ctx->state != HTTP_STATE_OPEN) {
		return -ENOTCONN;
	}

	if (opcode != WS_OPCODE_DATA_TEXT && opcode != WS_OPCODE_DATA_BINARY &&
	    opcode != WS_OPCODE_CONTINUE && opcode != WS_OPCODE_CLOSE &&
	    opcode != WS_OPCODE_PING && opcode != WS_OPCODE_PONG) {
		return -EINVAL;
	}

	(void)memset(header, 0, sizeof(header));

	/* Is this the last packet? */
	header[0] = final ? BIT(7) : 0;

	/* Text, binary, ping, pong or close ? */
	header[0] |= opcode;

	/* Masking */
	header[1] = mask ? BIT(7) : 0;

	if (payload_len < 126) {
		header[1] |= payload_len;
	} else if (payload_len < 65536) {
		header[1] |= 126;
		header[2] = payload_len >> 8;
		header[3] = payload_len;
		hdr_len += 2;
	} else {
		header[1] |= 127;
		header[2] = 0;
		header[3] = 0;
		header[4] = 0;
		header[5] = 0;
		header[6] = payload_len >> 24;
		header[7] = payload_len >> 16;
		header[8] = payload_len >> 8;
		header[9] = payload_len;
		hdr_len += 8;
	}

	/* Add masking value if needed */
	if (mask) {
		u32_t masking_value;

		masking_value = sys_rand32_get();

		header[hdr_len++] |= masking_value >> 24;
		header[hdr_len++] |= masking_value >> 16;
		header[hdr_len++] |= masking_value >> 8;
		header[hdr_len++] |= masking_value;

		ws_mask_payload(payload, payload_len, masking_value);
	}

	ret = http_prepare_and_send(ctx, header, hdr_len, dst, user_send_data);
	if (ret < 0) {
		NET_DBG("Cannot add ws header (%d)", ret);
		goto quit;
	}

	if (payload) {
		ret = http_prepare_and_send(ctx, payload, payload_len,
					    dst, user_send_data);
		if (ret < 0) {
			NET_DBG("Cannot send %zd bytes message (%d)",
				payload_len, ret);
			goto quit;
		}
	}

	ret = http_send_flush(ctx, user_send_data);

quit:
	return ret;
}

int ws_strip_header(struct net_pkt *pkt, bool *masked, u32_t *mask_value,
		    u32_t *message_length, u32_t *message_type_flag,
		    u32_t *header_len)
{
	struct net_buf *frag;
	u16_t value, pos, appdata_pos;
	u8_t len; /* message length byte */
	u8_t len_len; /* length of the length field in header */

	frag = net_frag_read_be16(pkt->frags, 0, &pos, &value);
	if (!frag && pos == 0xffff) {
		return -ENOMSG;
	}

	if (value & 0x8000) {
		*message_type_flag |= WS_FLAG_FINAL;
	}

	switch (value & 0x0f00) {
	case 0x0100:
		*message_type_flag |= WS_FLAG_TEXT;
		break;
	case 0x0200:
		*message_type_flag |= WS_FLAG_BINARY;
		break;
	case 0x0800:
		*message_type_flag |= WS_FLAG_CLOSE;
		break;
	case 0x0900:
		*message_type_flag |= WS_FLAG_PING;
		break;
	case 0x0A00:
		*message_type_flag |= WS_FLAG_PONG;
		break;
	}

	len = value & 0x007f;
	if (len < 126) {
		len_len = 0;
		*message_length = len;
	} else if (len == 126) {
		u16_t msg_len;

		len_len = 2;

		frag = net_frag_read_be16(frag, pos, &pos, &msg_len);
		if (!frag && pos == 0xffff) {
			return -ENOMSG;
		}

		*message_length = msg_len;
	} else {
		len_len = 4;

		frag = net_frag_read_be32(frag, pos, &pos, message_length);
		if (!frag && pos == 0xffff) {
			return -ENOMSG;
		}
	}

	if (value & 0x0080) {
		*masked = true;
		appdata_pos = 0;

		frag = net_frag_read_be32(frag, pos, &pos, mask_value);
		if (!frag && pos == 0xffff) {
			return -ENOMSG;
		}
	} else {
		*masked = false;
		appdata_pos = len_len;
	}

	frag = net_frag_get_pos(pkt, pos + appdata_pos, &pos);
	if (!frag && pos == 0xffff) {
		return -ENOMSG;
	}

	/* Minimum websocket header is 6 bytes, header length might be
	 * bigger depending on length field len.
	 */
	*header_len = 6 + len_len;

	return 0;
}

static bool field_contains(const char *field, int field_len,
			   const char *str, int str_len)
{
	bool found = false;
	char c, skip;

	c = *str++;
	if (c == '\0') {
		return false;
	}

	str_len--;

	do {
		do {
			skip = *field++;
			field_len--;
			if (skip == '\0' || field_len == 0) {
				return false;
			}
		} while (skip != c);

		if (field_len < str_len) {
			return false;
		}

		if (strncasecmp(field, str, str_len) == 0) {
			found = true;
			break;
		}

	} while (field_len >= str_len);

	return found;
}

static bool check_ws_headers(struct http_ctx *ctx, struct http_parser *parser,
			     int *ws_sec_key, int *host, int *subprotocol)
{
	int i, count, connection = -1;
	int ws_sec_version = -1;

	if (!parser->upgrade || parser->method != HTTP_GET ||
	    parser->http_major != 1 || parser->http_minor != 1) {
		return false;
	}

	for (i = 0, count = 0; i < ctx->http.field_values_ctr; i++) {
		if (ctx->http.field_values[i].key_len == 0) {
			continue;
		}

		if (strncasecmp(ctx->http.field_values[i].key,
				"Sec-WebSocket-Key",
				sizeof("Sec-WebSocket-Key") - 1) == 0) {
			*ws_sec_key = i;
			continue;
		}

		if (strncasecmp(ctx->http.field_values[i].key,
				"Sec-WebSocket-Version",
				sizeof("Sec-WebSocket-Version") - 1) == 0) {
			if (strncmp(ctx->http.field_values[i].value,
				    "13", sizeof("13") - 1) == 0) {
				ws_sec_version = i;
			}

			continue;
		}

		if (strncasecmp(ctx->http.field_values[i].key,
				"Connection", sizeof("Connection") - 1) == 0) {
			if (field_contains(
				    ctx->http.field_values[i].value,
				    ctx->http.field_values[i].value_len,
				    "Upgrade", sizeof("Upgrade") - 1)) {
				connection = i;
			}

			continue;
		}

		if (strncasecmp(ctx->http.field_values[i].key, "Host",
				sizeof("Host") - 1) == 0) {
			*host = i;
			continue;
		}

		if (strncasecmp(ctx->http.field_values[i].key,
				"Sec-WebSocket-Protocol",
				sizeof("Sec-WebSocket-Protocol") - 1) == 0) {
			*subprotocol = i;
			continue;
		}
	}

	if (connection >= 0 && *ws_sec_key >= 0 && ws_sec_version >= 0 &&
	    *host >= 0) {
		return true;
	}

	return false;
}

static struct net_pkt *prepare_reply(struct http_ctx *ctx,
				     int ws_sec_key, int host, int subprotocol)
{
	static const char basic_reply_headers[] =
		"HTTP/1.1 101 OK\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: ";
	char key_accept[32 + sizeof(WS_MAGIC)];
	char accept[20];
	struct net_pkt *pkt;
	char tmp[64];
	int ret;
	size_t key_len;
	size_t olen;

	pkt = net_app_get_net_pkt_with_dst(&ctx->app_ctx,
					   ctx->http.parser.addr,
					   ctx->timeout);
	if (!pkt) {
		return NULL;
	}

	if (!net_pkt_append_all(pkt, sizeof(basic_reply_headers) - 1,
				(u8_t *)basic_reply_headers, ctx->timeout)) {
		goto fail;
	}

	key_len = min(sizeof(key_accept) - 1,
		      ctx->http.field_values[ws_sec_key].value_len);
	strncpy(key_accept, ctx->http.field_values[ws_sec_key].value,
		key_len);

	olen = min(sizeof(key_accept) - 1 - key_len, sizeof(WS_MAGIC) - 1);
	strncpy(key_accept + key_len, WS_MAGIC, olen);

	mbedtls_sha1(key_accept, olen + key_len, accept);

	ret = base64_encode(tmp, sizeof(tmp) - 1, &olen, accept,
			    sizeof(accept));
	if (ret) {
		if (ret == -ENOMEM) {
			NET_DBG("[%p] Too short buffer olen %zd", ctx, olen);
		}

		goto fail;
	}
	if (!net_pkt_append_all(pkt, olen, (u8_t *)tmp, ctx->timeout)) {
		goto fail;
	}

	ret = snprintk(tmp, sizeof(tmp), "User-Agent: %s\r\n\r\n",
		       ZEPHYR_USER_AGENT);
	if (ret < 0 || ret >= sizeof(tmp)) {
		goto fail;
	}
	if (!net_pkt_append_all(pkt, ret, (u8_t *)tmp, ctx->timeout)) {
		goto fail;
	}

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

int ws_headers_complete(struct http_parser *parser)
{
	struct http_ctx *ctx = parser->data;
	int ws_sec_key = -1, host = -1, subprotocol = -1;

	if (check_ws_headers(ctx, parser, &ws_sec_key, &host,
			     &subprotocol)) {
		struct net_pkt *pkt;
		struct http_root_url *url;
		int ret;

		url = http_url_find(ctx, HTTP_URL_WEBSOCKET);
		if (!url) {
			url = http_url_find(ctx, HTTP_URL_STANDARD);
			if (url) {
				/* Normal HTTP URL was found */
				return 0;
			}

			/* If there is no URL to serve this websocket
			 * request, then just bail out.
			 */
			if (!ctx->http.urls) {
				NET_DBG("[%p] No URL handlers found", ctx);
				return 0;
			}

			url = &ctx->http.urls->default_url;
			if (url && url->is_used &&
			    ctx->http.urls->default_cb) {
				ret = ctx->http.urls->default_cb(ctx,
							WS_CONNECTION,
							ctx->http.parser.addr);
				if (ret == HTTP_VERDICT_ACCEPT) {
					goto accept;
				}
			}

			if (url->flags == HTTP_URL_WEBSOCKET) {
				goto fail;
			}
		}

		if (url->flags != HTTP_URL_WEBSOCKET) {
			return 0;
		}

accept:
		NET_DBG("[%p] ws header %d fields found", ctx,
			ctx->http.field_values_ctr + 1);

		pkt = prepare_reply(ctx, ws_sec_key, host, subprotocol);
		if (!pkt) {
			goto fail;
		}

		net_pkt_set_appdatalen(pkt, net_buf_frags_len(pkt->frags));

		ret = net_app_send_pkt(&ctx->app_ctx, pkt, NULL, 0, 0,
				       INT_TO_POINTER(K_FOREVER));
		if (ret) {
			goto fail;
		}

		http_change_state(ctx, HTTP_STATE_HEADER_RECEIVED);

		/* We do not expect any HTTP data after this */
		return 2;

fail:
		http_change_state(ctx, HTTP_STATE_CLOSED);
	}

	return 0;
}
