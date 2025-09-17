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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_websocket, CONFIG_NET_WEBSOCKET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <strings.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <zephyr/net/http/client.h>
#include <zephyr/net/websocket.h>

#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/base64.h>

#ifdef CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT
#include <psa/crypto.h>
#else
#include <mbedtls/sha1.h>
#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT */

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

static int response_cb(struct http_response *rsp,
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

	return 0;
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
#ifdef CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT
	psa_status_t psa_status;
	size_t hash_length;
#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT */

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
	ctx->recv_buf.buf = wreq->tmp_buf;
	ctx->recv_buf.size = wreq->tmp_buf_len;
	ctx->sec_accept_key = sec_accept_key;
	ctx->http_cb = wreq->http_cb;
	ctx->is_client = 1;

#ifdef CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT
	psa_status = psa_hash_compute(PSA_ALG_SHA_1, (const uint8_t *)&rnd_value, sizeof(rnd_value),
				      sec_accept_key, sizeof(sec_accept_key), &hash_length);
	if (psa_status != PSA_SUCCESS) {
		NET_DBG("[%p] Cannot calculate sha1 (%d)", ctx, psa_status);
		ret = -EPROTO;
		goto out;
	}
#else
	ret = mbedtls_sha1((const unsigned char *)&rnd_value, sizeof(rnd_value), sec_accept_key);
	if (ret != 0) {
		NET_DBG("[%p] Cannot calculate sha1 (%d)", ctx, ret);
		ret = -EPROTO;
		goto out;
	}
#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT */


	ret = base64_encode(sec_ws_key + sizeof("Sec-Websocket-Key: ") - 1,
			    sizeof(sec_ws_key) -
					sizeof("Sec-Websocket-Key: "),
			    &olen, sec_accept_key,
			    /* We are only interested in 16 first bytes so
			     * subtract 4 from the SHA-1 length
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
#ifdef CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT
	psa_status = psa_hash_compute(PSA_ALG_SHA_1, (const uint8_t *)key_accept, olen + key_len,
				      sec_accept_key, sizeof(sec_accept_key), &hash_length);
	if (psa_status != PSA_SUCCESS) {
		NET_DBG("[%p] Cannot calculate sha1 (%d)", ctx, psa_status);
		ret = -EPROTO;
		goto out;
	}
#else
	ret = mbedtls_sha1(key_accept, olen + key_len, sec_accept_key);
	if (ret != 0) {
		NET_DBG("[%p] Cannot calculate sha1 (%d)", ctx, ret);
		ret = -EPROTO;
		goto out;
	}
#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT */

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

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		ret = -ENOSPC;
		goto out;
	}

	ctx->sock = fd;
	zvfs_finalize_typed_fd(fd, ctx, (const struct fd_op_vtable *)&websocket_fd_op_vtable,
			    ZVFS_MODE_IFSOCK);

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

	/* We will re-use the temp buffer in receive function. If there were
	 * any leftover data from HTTP headers processing, we need to reflect
	 * this in the count variable.
	 */
	ctx->recv_buf.count = req.data_len;

	/* Init parser FSM */
	ctx->parser_state = WEBSOCKET_PARSER_STATE_OPCODE;

	(void)sock_obj_core_alloc_find(ctx->real_sock, fd, SOCK_STREAM);

	return fd;

out:
	if (fd >= 0) {
		(void)zsock_close(fd);
	}

	websocket_context_unref(ctx);
	return ret;
}

int websocket_disconnect(int ws_sock)
{
	return zsock_close(ws_sock);
}

static int websocket_interal_disconnect(struct websocket_context *ctx)
{
	int ret;

	if (ctx == NULL) {
		return -ENOENT;
	}

	NET_DBG("[%p] Disconnecting", ctx);

	ret = websocket_send_msg(ctx->sock, NULL, 0, WEBSOCKET_OPCODE_CLOSE,
				 ctx->is_client, true, SYS_FOREVER_MS);
	if (ret < 0) {
		NET_DBG("[%p] Failed to send close message (err %d).", ctx, ret);
	}

	(void)sock_obj_core_dealloc(ctx->sock);

	websocket_context_unref(ctx);

	return ret;
}

static int websocket_close_vmeth(void *obj)
{
	struct websocket_context *ctx = obj;
	int ret;

	ret = websocket_interal_disconnect(ctx);
	if (ret < 0) {
		/* Ignore error if we are not connected */
		if (ret != -ENOTCONN) {
			NET_DBG("[%p] Cannot close (%d)", obj, ret);

			errno = -ret;
			return -1;
		}

		ret = 0;
	}

	return ret;
}

static inline int websocket_poll_offload(struct zsock_pollfd *fds, int nfds,
					 int timeout)
{
	int fd_backup[CONFIG_ZVFS_POLL_MAX];
	const struct fd_op_vtable *vtable;
	void *ctx;
	int ret = 0;
	int i;

	/* Overwrite websocket file descriptors with underlying ones. */
	for (i = 0; i < nfds; i++) {
		fd_backup[i] = fds[i].fd;

		ctx = zvfs_get_fd_obj(fds[i].fd,
				   (const struct fd_op_vtable *)
						     &websocket_fd_op_vtable,
				   0);
		if (ctx == NULL) {
			continue;
		}

		fds[i].fd = ((struct websocket_context *)ctx)->real_sock;
	}

	/* Get offloaded sockets vtable. */
	ctx = zvfs_get_fd_obj_and_vtable(fds[0].fd,
				      (const struct fd_op_vtable **)&vtable,
				      NULL);
	if (ctx == NULL) {
		errno = EINVAL;
		ret = -1;
		goto exit;
	}

	ret = zvfs_fdtable_call_ioctl(vtable, ctx, ZFD_IOCTL_POLL_OFFLOAD,
				   fds, nfds, timeout);

exit:
	/* Restore original fds. */
	for (i = 0; i < nfds; i++) {
		fds[i].fd = fd_backup[i];
	}

	return ret;
}

static int websocket_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	struct websocket_context *ctx = obj;

	switch (request) {
	case ZFD_IOCTL_POLL_OFFLOAD: {
		struct zsock_pollfd *fds;
		int nfds;
		int timeout;

		fds = va_arg(args, struct zsock_pollfd *);
		nfds = va_arg(args, int);
		timeout = va_arg(args, int);

		return websocket_poll_offload(fds, nfds, timeout);
	}

	case ZFD_IOCTL_SET_LOCK:
		/* Ignore, don't want to overwrite underlying socket lock. */
		return 0;

	default: {
		const struct fd_op_vtable *vtable;
		void *core_obj;

		core_obj = zvfs_get_fd_obj_and_vtable(
				ctx->real_sock,
				(const struct fd_op_vtable **)&vtable,
				NULL);
		if (core_obj == NULL) {
			errno = EBADF;
			return -1;
		}

		/* Pass the call to the core socket implementation. */
		return vtable->ioctl(core_obj, request, args);
	}
	}

	return 0;
}

#if !defined(CONFIG_NET_TEST)
static int sendmsg_all(int sock, const struct msghdr *message, int flags,
			const k_timepoint_t req_end_timepoint)
{
	int ret, i;
	size_t offset = 0;
	size_t total_len = 0;

	for (i = 0; i < message->msg_iovlen; i++) {
		total_len += message->msg_iov[i].iov_len;
	}

	while (offset < total_len) {
		ret = zsock_sendmsg(sock, message, flags);

		if ((ret == 0) || (ret < 0 && errno == EAGAIN)) {
			struct zsock_pollfd pfd;
			int pollres;
			k_ticks_t req_timeout_ticks =
				sys_timepoint_timeout(req_end_timepoint).ticks;
			int req_timeout_ms = k_ticks_to_ms_floor32(req_timeout_ticks);

			pfd.fd = sock;
			pfd.events = ZSOCK_POLLOUT;
			pollres = zsock_poll(&pfd, 1, req_timeout_ms);
			if (pollres == 0) {
				return -ETIMEDOUT;
			} else if (pollres > 0) {
				continue;
			} else {
				return -errno;
			}
		} else if (ret < 0) {
			return -errno;
		}

		offset += ret;
		if (offset >= total_len) {
			break;
		}

		/* Update msghdr for the next iteration. */
		for (i = 0; i < message->msg_iovlen; i++) {
			if (ret < message->msg_iov[i].iov_len) {
				message->msg_iov[i].iov_len -= ret;
				message->msg_iov[i].iov_base =
					(uint8_t *)message->msg_iov[i].iov_base + ret;
				break;
			}

			ret -= message->msg_iov[i].iov_len;
			message->msg_iov[i].iov_len = 0;
		}
	}

	return total_len;
}
#endif /* !defined(CONFIG_NET_TEST) */

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
		if ((payload != NULL) && (payload_len > 0)) {
			LOG_HEXDUMP_DBG(payload, payload_len, "Payload");
		} else {
			LOG_DBG("No payload");
		}
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

	k_timeout_t req_timeout = K_MSEC(timeout);
	k_timepoint_t req_end_timepoint = sys_timepoint_calc(req_timeout);

	return sendmsg_all(ctx->real_sock, &msg,
			   K_TIMEOUT_EQ(tout, K_NO_WAIT) ? ZSOCK_MSG_DONTWAIT : 0,
			   req_end_timepoint);
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

	ctx = zvfs_get_fd_obj(ws_sock, NULL, 0);
	if (ctx == NULL) {
		return -EBADF;
	}

#if !defined(CONFIG_NET_TEST)
	/* Websocket unit test does not use context from pool but allocates
	 * its own, hence skip the check.
	 */

	if (!PART_OF_ARRAY(contexts, ctx)) {
		return -ENOENT;
	}
#endif /* !defined(CONFIG_NET_TEST) */

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

		if ((payload != NULL) && (payload_len > 0)) {
			data_to_send = k_malloc(payload_len);
			if (!data_to_send) {
				return -ENOMEM;
			}

			memcpy(data_to_send, payload, payload_len);

			for (i = 0; i < payload_len; i++) {
				data_to_send[i] ^= ctx->masking_value >> (8 * (3 - i % 4));
			}
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

	/* Do no math with 0 and error codes */
	if (ret <= 0) {
		return ret;
	}

	return ret - hdr_len;
}

static uint32_t websocket_opcode2flag(uint8_t data)
{
	switch (data & 0x0f) {
	case WEBSOCKET_OPCODE_DATA_TEXT:
		return WEBSOCKET_FLAG_TEXT;
	case WEBSOCKET_OPCODE_DATA_BINARY:
		return WEBSOCKET_FLAG_BINARY;
	case WEBSOCKET_OPCODE_CLOSE:
		return WEBSOCKET_FLAG_CLOSE;
	case WEBSOCKET_OPCODE_PING:
		return WEBSOCKET_FLAG_PING;
	case WEBSOCKET_OPCODE_PONG:
		return WEBSOCKET_FLAG_PONG;
	default:
		break;
	}
	return 0;
}

static int websocket_parse(struct websocket_context *ctx, struct websocket_buffer *payload)
{
	int len;
	uint8_t data;
	size_t parsed_count = 0;

	do {
		if (parsed_count >= ctx->recv_buf.count) {
			return parsed_count;
		}
		if (ctx->parser_state != WEBSOCKET_PARSER_STATE_PAYLOAD) {
			data = ctx->recv_buf.buf[parsed_count++];

			switch (ctx->parser_state) {
			case WEBSOCKET_PARSER_STATE_OPCODE:
				ctx->message_type = websocket_opcode2flag(data);
				if ((data & 0x80) != 0) {
					ctx->message_type |= WEBSOCKET_FLAG_FINAL;
				}
				ctx->parser_state = WEBSOCKET_PARSER_STATE_LENGTH;
				break;
			case WEBSOCKET_PARSER_STATE_LENGTH:
				ctx->masked = (data & 0x80) != 0;
				len = data & 0x7f;
				if (len < 126) {
					ctx->message_len = len;
					if (ctx->masked) {
						ctx->masking_value = 0;
						ctx->parser_remaining = 4;
						ctx->parser_state = WEBSOCKET_PARSER_STATE_MASK;
					} else {
						ctx->parser_remaining = ctx->message_len;
						ctx->parser_state =
							(ctx->parser_remaining == 0)
								? WEBSOCKET_PARSER_STATE_OPCODE
								: WEBSOCKET_PARSER_STATE_PAYLOAD;
					}
				} else {
					ctx->message_len = 0;
					ctx->parser_remaining = (len < 127) ? 2 : 8;
					ctx->parser_state = WEBSOCKET_PARSER_STATE_EXT_LEN;
				}
				break;
			case WEBSOCKET_PARSER_STATE_EXT_LEN:
				ctx->parser_remaining--;
				ctx->message_len |= ((uint64_t)data << (ctx->parser_remaining * 8));
				if (ctx->parser_remaining == 0) {
					if (ctx->masked) {
						ctx->masking_value = 0;
						ctx->parser_remaining = 4;
						ctx->parser_state = WEBSOCKET_PARSER_STATE_MASK;
					} else {
						ctx->parser_remaining = ctx->message_len;
						ctx->parser_state = WEBSOCKET_PARSER_STATE_PAYLOAD;
					}
				}
				break;
			case WEBSOCKET_PARSER_STATE_MASK:
				ctx->parser_remaining--;
				ctx->masking_value |=
					(uint32_t)((uint64_t)data << (ctx->parser_remaining * 8));
				if (ctx->parser_remaining == 0) {
					if (ctx->message_len == 0) {
						ctx->parser_remaining = 0;
						ctx->parser_state = WEBSOCKET_PARSER_STATE_OPCODE;
					} else {
						ctx->parser_remaining = ctx->message_len;
						ctx->parser_state = WEBSOCKET_PARSER_STATE_PAYLOAD;
					}
				}
				break;
			default:
				return -EFAULT;
			}

#if (LOG_LEVEL >= LOG_LEVEL_DBG)
			if ((ctx->parser_state == WEBSOCKET_PARSER_STATE_PAYLOAD) ||
			    ((ctx->parser_state == WEBSOCKET_PARSER_STATE_OPCODE) &&
			     (ctx->message_len == 0))) {
				NET_DBG("[%p] %smasked, mask 0x%08x, type 0x%02x, msg %zd", ctx,
					ctx->masked ? "" : "un",
					ctx->masked ? ctx->masking_value : 0, ctx->message_type,
					(size_t)ctx->message_len);
			}
#endif
		} else {
			size_t remaining_in_recv_buf = ctx->recv_buf.count - parsed_count;
			size_t payload_in_recv_buf =
				MIN(remaining_in_recv_buf, ctx->parser_remaining);
			size_t free_in_payload_buf = payload->size - payload->count;
			size_t ready_to_copy = MIN(payload_in_recv_buf, free_in_payload_buf);

			if (free_in_payload_buf == 0) {
				break;
			}

			memcpy(&payload->buf[payload->count], &ctx->recv_buf.buf[parsed_count],
			       ready_to_copy);
			parsed_count += ready_to_copy;
			payload->count += ready_to_copy;
			ctx->parser_remaining -= ready_to_copy;
			if (ctx->parser_remaining == 0) {
				ctx->parser_remaining = 0;
				ctx->parser_state = WEBSOCKET_PARSER_STATE_OPCODE;
			}
		}

	} while (ctx->parser_state != WEBSOCKET_PARSER_STATE_OPCODE);

	return parsed_count;
}

#if !defined(CONFIG_NET_TEST)
static int wait_rx(int sock, int timeout)
{
	struct zsock_pollfd fds = {
		.fd = sock,
		.events = ZSOCK_POLLIN,
	};
	int ret;

	ret = zsock_poll(&fds, 1, timeout);
	if (ret < 0) {
		return ret;
	}

	if (ret == 0) {
		/* Timeout */
		return -EAGAIN;
	}

	if (fds.revents & ZSOCK_POLLNVAL) {
		return -EBADF;
	}

	if (fds.revents & ZSOCK_POLLERR) {
		return -EIO;
	}

	return 0;
}

static int timeout_to_ms(k_timeout_t *timeout)
{
	if (K_TIMEOUT_EQ(*timeout, K_NO_WAIT)) {
		return 0;
	} else if (K_TIMEOUT_EQ(*timeout, K_FOREVER)) {
		return SYS_FOREVER_MS;
	} else {
		return k_ticks_to_ms_floor32(timeout->ticks);
	}
}

#endif /* !defined(CONFIG_NET_TEST) */

int websocket_recv_msg(int ws_sock, uint8_t *buf, size_t buf_len,
		       uint32_t *message_type, uint64_t *remaining, int32_t timeout)
{
	struct websocket_context *ctx;
	int ret;
	k_timepoint_t end;
	k_timeout_t tout = K_FOREVER;
	struct websocket_buffer payload = {.buf = buf, .size = buf_len, .count = 0};

	if (timeout != SYS_FOREVER_MS) {
		tout = K_MSEC(timeout);
	}

	if ((buf == NULL) || (buf_len == 0)) {
		return -EINVAL;
	}

	end = sys_timepoint_calc(tout);

#if defined(CONFIG_NET_TEST)
	struct test_data *test_data = zvfs_get_fd_obj(ws_sock, NULL, 0);

	if (test_data == NULL) {
		return -EBADF;
	}

	ctx = test_data->ctx;
#else
	ctx = zvfs_get_fd_obj(ws_sock, NULL, 0);
	if (ctx == NULL) {
		return -EBADF;
	}

	if (!PART_OF_ARRAY(contexts, ctx)) {
		return -ENOENT;
	}
#endif /* CONFIG_NET_TEST */

	do {
		size_t parsed_count;

		if (ctx->recv_buf.count == 0) {
#if defined(CONFIG_NET_TEST)
			size_t input_len = MIN(ctx->recv_buf.size,
					       test_data->input_len - test_data->input_pos);

			if (input_len > 0) {
				memcpy(ctx->recv_buf.buf,
				       &test_data->input_buf[test_data->input_pos], input_len);
				test_data->input_pos += input_len;
				ret = input_len;
			} else {
				/* emulate timeout */
				ret = -EAGAIN;
			}
#else
			tout = sys_timepoint_timeout(end);

			ret = wait_rx(ctx->real_sock, timeout_to_ms(&tout));
			if (ret == 0) {
				ret = zsock_recv(ctx->real_sock, ctx->recv_buf.buf,
						 ctx->recv_buf.size, ZSOCK_MSG_DONTWAIT);
				if (ret < 0) {
					ret = -errno;
				}
			}
#endif /* CONFIG_NET_TEST */

			if (ret < 0) {
				if ((ret == -EAGAIN) && (payload.count > 0)) {
					/* go to unmasking */
					break;
				}
				return ret;
			}

			if (ret == 0) {
				/* Socket closed */
				return -ENOTCONN;
			}

			ctx->recv_buf.count = ret;

			NET_DBG("[%p] Received %d bytes", ctx, ret);
		}

		ret = websocket_parse(ctx, &payload);
		if (ret < 0) {
			return ret;
		}
		parsed_count = ret;

		if ((ctx->parser_state == WEBSOCKET_PARSER_STATE_OPCODE) ||
		    (payload.count >= payload.size)) {
			if (remaining != NULL) {
				*remaining = ctx->parser_remaining;
			}
			if (message_type != NULL) {
				*message_type = ctx->message_type;
			}

			size_t left = ctx->recv_buf.count - parsed_count;

			if (left > 0) {
				memmove(ctx->recv_buf.buf, &ctx->recv_buf.buf[parsed_count], left);
			}
			ctx->recv_buf.count = left;
			break;
		}

		ctx->recv_buf.count -= parsed_count;

	} while (true);

	/* Unmask the data */
	if (ctx->masked) {
		uint8_t *mask_as_bytes = (uint8_t *)&ctx->masking_value;
		size_t data_buf_offset = ctx->message_len - ctx->parser_remaining - payload.count;

		for (size_t i = 0; i < payload.count; i++) {
			size_t m = data_buf_offset % 4;

			payload.buf[i] ^= mask_as_bytes[3 - m];
			data_buf_offset++;
		}
	}

	return payload.count;
}

static int websocket_send(struct websocket_context *ctx, const uint8_t *buf,
			  size_t buf_len, int32_t timeout)
{
	int ret;

	NET_DBG("[%p] Sending %zd bytes", ctx, buf_len);

	ret = websocket_send_msg(ctx->sock, buf, buf_len, WEBSOCKET_OPCODE_DATA_TEXT,
				 ctx->is_client, true, timeout);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	NET_DBG("[%p] Sent %d bytes", ctx, ret);

	sock_obj_core_update_send_stats(ctx->sock, ret);

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
		if (ret == -ENOTCONN) {
			ret = 0;
		} else {
			errno = -ret;
			return -1;
		}
	}

	NET_DBG("[%p] Received %d bytes", ctx, ret);

	sock_obj_core_update_recv_stats(ctx->sock, ret);

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

int websocket_register(int sock, uint8_t *recv_buf, size_t recv_buf_len)
{
	struct websocket_context *ctx;
	int ret, fd;

	if (sock < 0) {
		return -EINVAL;
	}

	ctx = websocket_find(sock);
	if (ctx) {
		NET_DBG("[%p] Websocket for sock %d already exists!", ctx, sock);
		return -EEXIST;
	}

	ctx = websocket_get();
	if (!ctx) {
		return -ENOENT;
	}

	ctx->real_sock = sock;
	ctx->recv_buf.buf = recv_buf;
	ctx->recv_buf.size = recv_buf_len;
	ctx->is_client = 0;

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		ret = -ENOSPC;
		goto out;
	}

	ctx->sock = fd;
	zvfs_finalize_typed_fd(fd, ctx, (const struct fd_op_vtable *)&websocket_fd_op_vtable,
			    ZVFS_MODE_IFSOCK);

	NET_DBG("[%p] WS connection to peer established (fd %d)", ctx, fd);

	ctx->recv_buf.count = 0;
	ctx->parser_state = WEBSOCKET_PARSER_STATE_OPCODE;

	(void)sock_obj_core_alloc_find(ctx->real_sock, fd, SOCK_STREAM);

	return fd;

out:
	websocket_context_unref(ctx);

	return ret;
}

static struct websocket_context *websocket_search(int sock)
{
	struct websocket_context *ctx = NULL;
	int i;

	k_sem_take(&contexts_lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		if (!websocket_context_is_used(&contexts[i])) {
			continue;
		}

		if (contexts[i].sock != sock) {
			continue;
		}

		ctx = &contexts[i];
		break;
	}

	k_sem_give(&contexts_lock);

	return ctx;
}

int websocket_unregister(int sock)
{
	struct websocket_context *ctx;

	if (sock < 0) {
		return -EINVAL;
	}

	ctx = websocket_search(sock);
	if (ctx == NULL) {
		NET_DBG("[%p] Real socket for websocket sock %d not found!", ctx, sock);
		return -ENOENT;
	}

	if (ctx->real_sock < 0) {
		return -EALREADY;
	}

	(void)zsock_close(sock);
	(void)zsock_close(ctx->real_sock);

	ctx->real_sock = -1;
	ctx->sock = -1;

	return 0;
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
