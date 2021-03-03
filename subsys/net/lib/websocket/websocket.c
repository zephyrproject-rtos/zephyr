/** @file
 * @brief Websocket client API
 *
 * An API for applications to setup a websocket connections.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_websocket, CONFIG_NET_WEBSOCKET_LOG_LEVEL);

#include <kernel.h>
#include <strings.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/fdtable.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/http_client.h>
#include <net/websocket.h>

#include <random/rand32.h>
#include <sys/byteorder.h>
#include <sys/base64.h>
#include <mbedtls/sha1.h>

#include "net_private.h"
#include "sockets_internal.h"
#include "websocket_internal.h"

/* If you want to see the data that is being sent or received,
 * then you can enable debugging and set the following variables to 1.
 * This will print a lot of data so is not enabled by default.
 */
#define HEXDUMP_SENT_PACKETS 0
#define HEXDUMP_RECV_PACKETS 0

static struct websocket_context contexts[CONFIG_WEBSOCKET_MAX_CONTEXTS];

static struct k_sem contexts_lock;

extern const struct socket_op_vtable sock_fd_op_vtable;
static const struct socket_op_vtable websocket_fd_op_vtable;

#if defined(CONFIG_NET_TEST)
int verify_sent_and_received_msg(struct msghdr *msg, bool split_msg);
#endif

static const char *opcode2str(enum websocket_opcode opcode)
{
	switch (opcode) {
	case WEBSOCKET_OPCODE_DATA_TEXT:
		return "TEXT";
	case WEBSOCKET_OPCODE_DATA_BINARY:
		return "BIN";
	case WEBSOCKET_OPCODE_CONTINUE:
		return "CONT";
	case WEBSOCKET_OPCODE_CLOSE:
		return "CLOSE";
	case WEBSOCKET_OPCODE_PING:
		return "PING";
	case WEBSOCKET_OPCODE_PONG:
		return "PONG";
	default:
		break;
	}

	return NULL;
}

static int websocket_context_ref(struct websocket_context *ctx)
{
	int old_rc = atomic_inc(&ctx->refcount);

	return old_rc + 1;
}

static int websocket_context_unref(struct websocket_context *ctx)
{
	int old_rc = atomic_dec(&ctx->refcount);

	if (old_rc != 1) {
		return old_rc - 1;
	}

	return 0;
}

static inline bool websocket_context_is_used(struct websocket_context *ctx)
{
	NET_ASSERT(ctx);

	return !!atomic_get(&ctx->refcount);
}

static struct websocket_context *websocket_get(void)
{
	struct websocket_context *ctx = NULL;
	int i;

	k_sem_take(&contexts_lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		if (websocket_context_is_used(&contexts[i])) {
			continue;
		}

		websocket_context_ref(&contexts[i]);
		ctx = &contexts[i];
		break;
	}

	k_sem_give(&contexts_lock);

	return ctx;
}

static struct websocket_context *websocket_find(int real_sock)
{
	struct websocket_context *ctx = NULL;
	int i;

	k_sem_take(&contexts_lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		if (!websocket_context_is_used(&contexts[i])) {
			continue;
		}

		if (contexts[i].real_sock != real_sock) {
			continue;
		}

		ctx = &contexts[i];
		break;
	}

	k_sem_give(&contexts_lock);

	return ctx;
}

static void response_cb(struct http_response *rsp,
			enum http_final_call final_data,
			void *user_data)
{
	struct websocket_context *ctx = user_data;

	if (final_data == HTTP_DATA_MORE) {
		NET_DBG("[%p] Partial data received (%zd bytes)", ctx,
			rsp->data_len);
		ctx->all_received = false;
	} else if (final_data == HTTP_DATA_FINAL) {
		NET_DBG("[%p] All the data received (%zd bytes)", ctx,
			rsp->data_len);
		ctx->all_received = true;
	}
}

static int on_header_field(struct http_parser *parser, const char *at,
			   size_t length)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);
	struct websocket_context *ctx = req->internal.user_data;
	const char *ws_accept_str = "Sec-WebSocket-Accept";
	uint16_t len;

	len = strlen(ws_accept_str);
	if (length >= len && strncasecmp(at, ws_accept_str, len) == 0) {
		ctx->sec_accept_present = true;
	}

	if (ctx->http_cb && ctx->http_cb->on_header_field) {
		ctx->http_cb->on_header_field(parser, at, length);
	}

	return 0;
}

#define MAX_SEC_ACCEPT_LEN 32

static int on_header_value(struct http_parser *parser, const char *at,
			   size_t length)
{
	struct http_request *req = CONTAINER_OF(parser,
						struct http_request,
						internal.parser);
	struct websocket_context *ctx = req->internal.user_data;
	char str[MAX_SEC_ACCEPT_LEN];

	if (ctx->sec_accept_present) {
		int ret;
		size_t olen;

		ctx->sec_accept_ok = false;
		ctx->sec_accept_present = false;

		ret = base64_encode(str, sizeof(str) - 1, &olen,
				    ctx->sec_accept_key,
				    WS_SHA1_OUTPUT_LEN);
		if (ret == 0) {
			if (strncmp(at, str, length)) {
				NET_DBG("[%p] Security keys do not match "
					"%s vs %s", ctx, str, at);
			} else {
				ctx->sec_accept_ok = true;
			}
		}
	}

	if (ctx->http_cb && ctx->http_cb->on_header_value) {
		ctx->http_cb->on_header_value(parser, at, length);
	}

	return 0;
}

int websocket_connect(int sock, struct websocket_request *wreq,
		      int32_t timeout, void *user_data)
{
	/* This is the expected Sec-WebSocket-Accept key. We are storing a
	 * pointer to this in ctx but the value is only used for the duration
	 * of this function call so there is no issue even if this variable
	 * is allocated from stack.
	 */
	uint8_t sec_accept_key[WS_SHA1_OUTPUT_LEN];
	struct http_parser_settings http_parser_settings;
	struct websocket_context *ctx;
	struct http_request req;
	int ret, fd, key_len;
	size_t olen;
	char key_accept[MAX_SEC_ACCEPT_LEN + sizeof(WS_MAGIC)];
	uint32_t rnd_value = sys_rand32_get();
	char sec_ws_key[] =
		"Sec-WebSocket-Key: 0123456789012345678901==\r\n";
	char *headers[] = {
		sec_ws_key,
		"Upgrade: websocket\r\n",
		"Connection: Upgrade\r\n",
		"Sec-WebSocket-Version: 13\r\n",
		NULL
	};

	fd = -1;

	if (sock < 0 || wreq == NULL || wreq->host == NULL ||
	    wreq->url == NULL) {
		return -EINVAL;
	}

	ctx = websocket_find(sock);
	if (ctx) {
		NET_DBG("[%p] Websocket for sock %d already exists!", ctx,
			sock);
		return -EEXIST;
	}

	ctx = websocket_get();
	if (!ctx) {
		return -ENOENT;
	}

	ctx->real_sock = sock;
	ctx->tmp_buf = wreq->tmp_buf;
	ctx->tmp_buf_len = wreq->tmp_buf_len;
	ctx->sec_accept_key = sec_accept_key;
	ctx->http_cb = wreq->http_cb;

	mbedtls_sha1_ret((const unsigned char *)&rnd_value, sizeof(rnd_value),
			 sec_accept_key);

	ret = base64_encode(sec_ws_key + sizeof("Sec-Websocket-Key: ") - 1,
			    sizeof(sec_ws_key) -
					sizeof("Sec-Websocket-Key: "),
			    &olen, sec_accept_key,
			    /* We are only interested in 16 first bytes so
			     * substract 4 from the SHA-1 length
			     */
			    sizeof(sec_accept_key) - 4);
	if (ret) {
		NET_DBG("[%p] Cannot encode base64 (%d)", ctx, ret);
		goto out;
	}

	if ((olen + sizeof("Sec-Websocket-Key: ") + 2) > sizeof(sec_ws_key)) {
		NET_DBG("[%p] Too long message (%zd > %zd)", ctx,
			olen + sizeof("Sec-Websocket-Key: ") + 2,
			sizeof(sec_ws_key));
		ret = -EMSGSIZE;
		goto out;
	}

	memcpy(sec_ws_key + sizeof("Sec-Websocket-Key: ") - 1 + olen,
	       HTTP_CRLF, sizeof(HTTP_CRLF));

	memset(&req, 0, sizeof(req));

	req.method = HTTP_GET;
	req.url = wreq->url;
	req.host = wreq->host;
	req.protocol = "HTTP/1.1";
	req.header_fields = (const char **)headers;
	req.optional_headers_cb = wreq->optional_headers_cb;
	req.optional_headers = wreq->optional_headers;
	req.response = response_cb;
	req.http_cb = &http_parser_settings;
	req.recv_buf = wreq->tmp_buf;
	req.recv_buf_len = wreq->tmp_buf_len;

	/* We need to catch the Sec-WebSocket-Accept field in order to verify
	 * that it contains the stuff that we sent in Sec-WebSocket-Key field
	 * so setup HTTP callbacks so that we will get the needed fields.
	 */
	if (ctx->http_cb) {
		memcpy(&http_parser_settings, ctx->http_cb,
		       sizeof(http_parser_settings));
	} else {
		memset(&http_parser_settings, 0, sizeof(http_parser_settings));
	}

	http_parser_settings.on_header_field = on_header_field;
	http_parser_settings.on_header_value = on_header_value;

	/* Pre-calculate the expected Sec-Websocket-Accept field */
	key_len = MIN(sizeof(key_accept) - 1, olen);
	strncpy(key_accept, sec_ws_key + sizeof("Sec-Websocket-Key: ") - 1,
		key_len);

	olen = MIN(sizeof(key_accept) - 1 - key_len, sizeof(WS_MAGIC) - 1);
	strncpy(key_accept + key_len, WS_MAGIC, olen);

	/* This SHA-1 value is then checked when we receive the response */
	mbedtls_sha1_ret(key_accept, olen + key_len, sec_accept_key);

	ret = http_client_req(sock, &req, timeout, ctx);
	if (ret < 0) {
		NET_DBG("[%p] Cannot connect to Websocket host %s", ctx,
			wreq->host);
		ret = -ECONNABORTED;
		goto out;
	}

	if (!(ctx->all_received && ctx->sec_accept_ok)) {
		NET_DBG("[%p] WS handshake failed (%d/%d)", ctx,
			ctx->all_received, ctx->sec_accept_ok);
		ret = -ECONNABORTED;
		goto out;
	}

	ctx->user_data = user_data;

	fd = z_reserve_fd();
	if (fd < 0) {
		ret = -ENOSPC;
		goto out;
	}

	ctx->sock = fd;
	z_finalize_fd(fd, ctx,
		      (const struct fd_op_vtable *)&websocket_fd_op_vtable);

	/* Call the user specified callback and if it accepts the connection
	 * then continue.
	 */
	if (wreq->cb) {
		ret = wreq->cb(fd, &req, user_data);
		if (ret < 0) {
			NET_DBG("[%p] Connection aborted (%d)", ctx, ret);
			goto out;
		}
	}

	NET_DBG("[%p] WS connection to peer established (fd %d)", ctx, fd);

	/* We will re-use the temp buffer in receive function if needed but
	 * in order that to work the amount of data in buffer must be set to 0
	 */
	ctx->tmp_buf_pos = 0;

	return fd;

out:
	if (fd >= 0) {
		(void)close(fd);
	}

	websocket_context_unref(ctx);
	return ret;
}

int websocket_disconnect(int ws_sock)
{
	return close(ws_sock);
}

static int websocket_interal_disconnect(struct websocket_context *ctx)
{
	int ret;

	if (ctx == NULL) {
		return -ENOENT;
	}

	NET_DBG("[%p] Disconnecting", ctx);

	ret = close(ctx->real_sock);

	websocket_context_unref(ctx);

	return ret;
}

static int websocket_close_vmeth(void *obj)
{
	struct websocket_context *ctx = obj;
	int ret;

	ret = websocket_interal_disconnect(ctx);
	if (ret < 0) {
		NET_DBG("[%p] Cannot close (%d)", obj, ret);

		errno = -ret;
		return -1;
	}

	return ret;
}

static int websocket_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	return sock_fd_op_vtable.fd_vtable.ioctl(obj, request, args);
}

static int websocket_prepare_and_send(struct websocket_context *ctx,
				      uint8_t *header, size_t header_len,
				      uint8_t *payload, size_t payload_len,
				      int32_t timeout)
{
	struct iovec io_vector[2];
	struct msghdr msg;

	io_vector[0].iov_base = header;
	io_vector[0].iov_len = header_len;
	io_vector[1].iov_base = payload;
	io_vector[1].iov_len = payload_len;

	memset(&msg, 0, sizeof(msg));

	msg.msg_iov = io_vector;
	msg.msg_iovlen = ARRAY_SIZE(io_vector);

	if (HEXDUMP_SENT_PACKETS) {
		LOG_HEXDUMP_DBG(header, header_len, "Header");
		LOG_HEXDUMP_DBG(payload, payload_len, "Payload");
	}

#if defined(CONFIG_NET_TEST)
	/* Simulate a case where the payload is split to two. The unit test
	 * does not set mask bit in this case.
	 */
	return verify_sent_and_received_msg(&msg, !(header[1] & BIT(7)));
#else
	k_timeout_t tout = K_FOREVER;

	if (timeout != SYS_FOREVER_MS) {
		tout = K_MSEC(timeout);
	}

	return sendmsg(ctx->real_sock, &msg,
		       K_TIMEOUT_EQ(tout, K_NO_WAIT) ? MSG_DONTWAIT : 0);
#endif /* CONFIG_NET_TEST */
}

int websocket_send_msg(int ws_sock, const uint8_t *payload, size_t payload_len,
		       enum websocket_opcode opcode, bool mask, bool final,
		       int32_t timeout)
{
	struct websocket_context *ctx;
	uint8_t header[MAX_HEADER_LEN], hdr_len = 2;
	uint8_t *data_to_send = (uint8_t *)payload;
	int ret;

	if (opcode != WEBSOCKET_OPCODE_DATA_TEXT &&
	    opcode != WEBSOCKET_OPCODE_DATA_BINARY &&
	    opcode != WEBSOCKET_OPCODE_CONTINUE &&
	    opcode != WEBSOCKET_OPCODE_CLOSE &&
	    opcode != WEBSOCKET_OPCODE_PING &&
	    opcode != WEBSOCKET_OPCODE_PONG) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_TEST)
	/* Websocket unit test does not use socket layer but feeds
	 * the data directly here when testing this function.
	 */
	ctx = UINT_TO_POINTER((unsigned int) ws_sock);
#else
	ctx = z_get_fd_obj(ws_sock, NULL, 0);
	if (ctx == NULL) {
		return -EBADF;
	}

	if (!PART_OF_ARRAY(contexts, ctx)) {
		return -ENOENT;
	}
#endif /* CONFIG_NET_TEST */

	NET_DBG("[%p] Len %zd %s/%d/%s", ctx, payload_len, opcode2str(opcode),
		mask, final ? "final" : "more");

	memset(header, 0, sizeof(header));

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
		int i;

		ctx->masking_value = sys_rand32_get();

		header[hdr_len++] |= ctx->masking_value >> 24;
		header[hdr_len++] |= ctx->masking_value >> 16;
		header[hdr_len++] |= ctx->masking_value >> 8;
		header[hdr_len++] |= ctx->masking_value;

		data_to_send = k_malloc(payload_len);
		if (!data_to_send) {
			return -ENOMEM;
		}

		memcpy(data_to_send, payload, payload_len);

		for (i = 0; i < payload_len; i++) {
			data_to_send[i] ^=
				ctx->masking_value >> (8 * (3 - i % 4));
		}
	}

	ret = websocket_prepare_and_send(ctx, header, hdr_len,
					 data_to_send, payload_len, timeout);
	if (ret < 0) {
		NET_DBG("Cannot send ws msg (%d)", -errno);
		goto quit;
	}

quit:
	if (data_to_send != payload) {
		k_free(data_to_send);
	}

	return ret - hdr_len;
}

static bool websocket_parse_header(uint8_t *buf, size_t buf_len, bool *masked,
				   uint32_t *mask_value, uint64_t *message_length,
				   uint32_t *message_type_flag,
				   size_t *header_len)
{
	uint8_t len_len; /* length of the length field in header */
	uint8_t len;     /* message length byte */
	uint16_t value;

	value = sys_get_be16(&buf[0]);
	if (value & 0x8000) {
		*message_type_flag |= WEBSOCKET_FLAG_FINAL;
	}

	switch (value & 0x0f00) {
	case 0x0100:
		*message_type_flag |= WEBSOCKET_FLAG_TEXT;
		break;
	case 0x0200:
		*message_type_flag |= WEBSOCKET_FLAG_BINARY;
		break;
	case 0x0800:
		*message_type_flag |= WEBSOCKET_FLAG_CLOSE;
		break;
	case 0x0900:
		*message_type_flag |= WEBSOCKET_FLAG_PING;
		break;
	case 0x0A00:
		*message_type_flag |= WEBSOCKET_FLAG_PONG;
		break;
	}

	len = value & 0x007f;
	if (len < 126) {
		len_len = 0;
		*message_length = len;
	} else if (len == 126) {
		len_len = 2;
		*message_length = sys_get_be16(&buf[2]);
	} else {
		len_len = 8;
		*message_length = sys_get_be64(&buf[2]);
	}

	/* Minimum websocket header is 2 bytes, header length might be
	 * bigger depending on length field len.
	 */
	*header_len = MIN_HEADER_LEN + len_len;

	if (buf_len >= *header_len) {
		if (value & 0x0080) {
			*masked = true;
			*mask_value = sys_get_be32(&buf[2 + len_len]);
			*header_len += 4;
		} else {
			*masked = false;
		}

		return true;
	}

	return false;
}

int websocket_recv_msg(int ws_sock, uint8_t *buf, size_t buf_len,
		       uint32_t *message_type, uint64_t *remaining, int32_t timeout)
{
	struct websocket_context *ctx;
	size_t header_len = 0;
	int recv_len = 0;
	size_t can_copy, left;
	int ret;
	k_timeout_t tout = K_FOREVER;

	if (timeout != SYS_FOREVER_MS) {
		tout = K_MSEC(timeout);
	}

#if defined(CONFIG_NET_TEST)
	/* Websocket unit test does not use socket layer but feeds
	 * the data directly here when testing this function.
	 */
	struct test_data {
		uint8_t *input_buf;
		size_t input_len;
		struct websocket_context *ctx;
	};

	struct test_data *test_data =
	    UINT_TO_POINTER((unsigned int) ws_sock);

	ctx = test_data->ctx;
#else
	ctx = z_get_fd_obj(ws_sock, NULL, 0);
	if (ctx == NULL) {
		return -EBADF;
	}

	if (!PART_OF_ARRAY(contexts, ctx)) {
		return -ENOENT;
	}
#endif /* CONFIG_NET_TEST */

	/* If we have not received the websocket header yet, read it first */
	if (!ctx->header_received) {
#if defined(CONFIG_NET_TEST)
		size_t input_len = MIN(ctx->tmp_buf_len - ctx->tmp_buf_pos,
				       test_data->input_len);

		memcpy(&ctx->tmp_buf[ctx->tmp_buf_pos], test_data->input_buf,
		       input_len);
		test_data->input_buf += input_len;
		ret = input_len;
#else
		ret = recv(ctx->real_sock, &ctx->tmp_buf[ctx->tmp_buf_pos],
			   ctx->tmp_buf_len - ctx->tmp_buf_pos,
			   K_TIMEOUT_EQ(tout, K_NO_WAIT) ? MSG_DONTWAIT : 0);
#endif /* CONFIG_NET_TEST */

		if (ret < 0) {
			return -errno;
		}

		if (ret == 0) {
			/* Socket closed */
			return 0;
		}

		ctx->tmp_buf_pos += ret;

		if (ctx->tmp_buf_pos >= MIN_HEADER_LEN) {
			bool masked;

			/* Now we will be able to figure out what is the
			 * actual size of the header.
			 */
			if (websocket_parse_header(&ctx->tmp_buf[0],
						   ctx->tmp_buf_pos,
						   &masked,
						   &ctx->masking_value,
						   &ctx->message_len,
						   &ctx->message_type,
						   &header_len)) {
				ctx->masked = masked;

				if (message_type) {
					*message_type = ctx->message_type;
				}
			} else {
				return -EAGAIN;
			}
		} else {
			return -EAGAIN;
		}

		if (ctx->tmp_buf_pos < header_len) {
			return -EAGAIN;
		}

		/* All of the header is now received, we can read the payload
		 * data next.
		 */
		ctx->header_received = true;

		if (HEXDUMP_RECV_PACKETS) {
			LOG_HEXDUMP_DBG(&ctx->tmp_buf[0], header_len,
					"Header");
			NET_DBG("[%p] masked %d mask 0x%04x hdr %zd msg %zd",
				ctx, ctx->masked,
				ctx->masked ? ctx->masking_value : 0,
				header_len, (size_t)ctx->message_len);
		}

		ctx->total_read = 0;

		memmove(ctx->tmp_buf, &ctx->tmp_buf[header_len],
			ctx->tmp_buf_len - header_len);
		ctx->tmp_buf_pos -= header_len;

		if (ctx->tmp_buf_pos == 0) {
			/* No data after the header, let the caller call
			 * this function again to get the payload.
			 */
			return -EAGAIN;
		}

		NET_DBG("There is %zd bytes of data", ctx->tmp_buf_pos);
	}

	/* Now read the whole payload or parts of it */

	if (ctx->tmp_buf_pos == 0) {
		/* Read more data into temp buffer */
#if defined(CONFIG_NET_TEST)
		size_t input_len = MIN(ctx->tmp_buf_len, test_data->input_len);

		memcpy(ctx->tmp_buf, test_data->input_buf, input_len);
		test_data->input_buf += input_len;

		ret = input_len;
#else
		ret = recv(ctx->real_sock, ctx->tmp_buf, ctx->tmp_buf_len,
			   K_TIMEOUT_EQ(tout, K_NO_WAIT) ? MSG_DONTWAIT : 0);
#endif /* CONFIG_NET_TEST */

		if (ret < 0) {
			return -errno;
		}

		if (ret == 0) {
			return 0;
		}

		ctx->tmp_buf_pos = ret;
	}

	if (ctx->tmp_buf_pos <= buf_len) {
		/* Is there already any data in the temp buffer? If yes,
		 * just return it to the caller.
		 */
		can_copy = MIN(ctx->message_len - ctx->total_read,
			       ctx->tmp_buf_pos);
	} else {
		/* We have more data in tmp buffer that will fit into
		 * user buffer.
		 */
		can_copy = MIN(ctx->message_len - ctx->total_read, buf_len);
	}

	left = ctx->tmp_buf_pos - can_copy;

	NET_ASSERT(ctx->tmp_buf_pos >= can_copy);

	memmove(buf, ctx->tmp_buf, can_copy);
	recv_len = can_copy;

	if (left > 0) {
		memmove(ctx->tmp_buf, &ctx->tmp_buf[can_copy], left);
	}

	ctx->tmp_buf_pos = left;
	ctx->total_read += recv_len;

	/* Unmask the data */
	if (ctx->masked) {
		/* As we might have less than 4 received bytes, we must select
		 * which byte from masking value to take. The mask_shift will
		 * tell that.
		 */
		int mask_shift = (ctx->total_read - recv_len) % sizeof(uint32_t);
		int i;

		for (i = 0; i < recv_len; i++) {
			buf[i] ^= ctx->masking_value >>
				(8 * (3 - (i + mask_shift) % 4));
		}
	}

#if HEXDUMP_RECV_PACKETS
	LOG_HEXDUMP_DBG(buf, recv_len, "Payload");
#endif

	if (remaining) {
		*remaining = ctx->message_len - ctx->total_read;
	}

	/* Start to read the header again if all the data has been received */
	if (ctx->message_len == ctx->total_read) {
		ctx->header_received = false;
		ctx->message_len = 0;
		ctx->message_type = 0;
		ctx->total_read = 0;
	}

	return recv_len;
}

static int websocket_send(struct websocket_context *ctx, const uint8_t *buf,
			  size_t buf_len, int32_t timeout)
{
	int ret;

	NET_DBG("[%p] Sending %zd bytes", ctx, buf_len);

	ret = websocket_send_msg(ctx->sock, buf, buf_len,
				 WEBSOCKET_OPCODE_DATA_TEXT,
				 true, true, timeout);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	NET_DBG("[%p] Sent %d bytes", ctx, ret);

	return ret;
}

static int websocket_recv(struct websocket_context *ctx, uint8_t *buf,
			  size_t buf_len, int32_t timeout)
{
	uint32_t message_type;
	uint64_t remaining;
	int ret;

	NET_DBG("[%p] Waiting data, buf len %zd bytes", ctx, buf_len);

	/* TODO: add support for recvmsg() so that we could return the
	 *       websocket specific information in ancillary data.
	 */
	ret = websocket_recv_msg(ctx->sock, buf, buf_len, &message_type,
				 &remaining, timeout);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	NET_DBG("[%p] Received %d bytes", ctx, ret);

	return ret;
}

static ssize_t websocket_read_vmeth(void *obj, void *buffer, size_t count)
{
	return (ssize_t)websocket_recv(obj, buffer, count, SYS_FOREVER_MS);
}

static ssize_t websocket_write_vmeth(void *obj, const void *buffer,
				     size_t count)
{
	return (ssize_t)websocket_send(obj, buffer, count, SYS_FOREVER_MS);
}

static ssize_t websocket_sendto_ctx(void *obj, const void *buf, size_t len,
				    int flags,
				    const struct sockaddr *dest_addr,
				    socklen_t addrlen)
{
	struct websocket_context *ctx = obj;
	int32_t timeout = SYS_FOREVER_MS;

	if (flags & ZSOCK_MSG_DONTWAIT) {
		timeout = 0;
	}

	ARG_UNUSED(dest_addr);
	ARG_UNUSED(addrlen);

	return (ssize_t)websocket_send(ctx, buf, len, timeout);
}

static ssize_t websocket_recvfrom_ctx(void *obj, void *buf, size_t max_len,
				      int flags, struct sockaddr *src_addr,
				      socklen_t *addrlen)
{
	struct websocket_context *ctx = obj;
	int32_t timeout = SYS_FOREVER_MS;

	if (flags & ZSOCK_MSG_DONTWAIT) {
		timeout = 0;
	}

	ARG_UNUSED(src_addr);
	ARG_UNUSED(addrlen);

	return (ssize_t)websocket_recv(ctx, buf, max_len, timeout);
}

static const struct socket_op_vtable websocket_fd_op_vtable = {
	.fd_vtable = {
		.read = websocket_read_vmeth,
		.write = websocket_write_vmeth,
		.close = websocket_close_vmeth,
		.ioctl = websocket_ioctl_vmeth,
	},
	.sendto = websocket_sendto_ctx,
	.recvfrom = websocket_recvfrom_ctx,
};

void websocket_context_foreach(websocket_context_cb_t cb, void *user_data)
{
	int i;

	k_sem_take(&contexts_lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		if (!websocket_context_is_used(&contexts[i])) {
			continue;
		}

		k_mutex_lock(&contexts[i].lock, K_FOREVER);

		cb(&contexts[i], user_data);

		k_mutex_unlock(&contexts[i].lock);
	}

	k_sem_give(&contexts_lock);
}

void websocket_init(void)
{
	k_sem_init(&contexts_lock, 1, K_SEM_MAX_LIMIT);
}
