/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_HTTP)
#if defined(CONFIG_HTTPS)
#define SYS_LOG_DOMAIN "https/client"
#else
#define SYS_LOG_DOMAIN "http/client"
#endif
#define NET_LOG_ENABLED 1
#endif

#define RX_EXTRA_DEBUG 0

#include <stdlib.h>
#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/dns_resolve.h>

#include <net/http.h>

#if defined(CONFIG_HTTPS)
#if defined(MBEDTLS_DEBUG_C)
#include <mbedtls/debug.h>
/* - Debug levels (from ext/lib/crypto/mbedtls/include/mbedtls/debug.h)
 *    - 0 No debug
 *    - 1 Error
 *    - 2 State change
 *    - 3 Informational
 *    - 4 Verbose
 */
#define DEBUG_THRESHOLD 0
#endif
#endif /* CONFIG_HTTPS */

#define BUF_ALLOC_TIMEOUT 100

#define HTTPS_STARTUP_TIMEOUT K_SECONDS(5)

/* HTTP client defines */
#define HTTP_EOF           "\r\n\r\n"

#define HTTP_CONTENT_TYPE  "Content-Type: "
#define HTTP_CONT_LEN_SIZE 64

struct waiter {
	struct http_client_ctx *ctx;
	struct k_sem wait;
};

int http_request(struct http_client_ctx *ctx,
		 struct http_client_request *req,
		 s32_t timeout)
{
	const char *method = http_method_str(req->method);
	struct net_pkt *pkt;
	int ret = -ENOMEM;

	pkt = net_pkt_get_tx(ctx->tcp.ctx, timeout);
	if (!pkt) {
		return -ENOMEM;
	}

	if (!net_pkt_append_all(pkt, strlen(method), (u8_t *)method,
				timeout)) {
		goto out;
	}

	/* Space after method string. */
	if (!net_pkt_append_all(pkt, 1, (u8_t *)" ", timeout)) {
		goto out;
	}

	if (!net_pkt_append_all(pkt, strlen(req->url), (u8_t *)req->url,
				timeout)) {
		goto out;
	}

	if (!net_pkt_append_all(pkt, strlen(req->protocol),
				(u8_t *)req->protocol, timeout)) {
		goto out;
	}

	if (req->host) {
		if (!net_pkt_append_all(pkt, strlen(req->host),
					(u8_t *)req->host, timeout)) {
			goto out;
		}

		if (!net_pkt_append_all(pkt, strlen(HTTP_CRLF),
					(u8_t *)HTTP_CRLF, timeout)) {
			goto out;
		}
	}

	if (req->header_fields) {
		if (!net_pkt_append_all(pkt, strlen(req->header_fields),
					(u8_t *)req->header_fields,
					timeout)) {
			goto out;
		}
	}

	if (req->content_type_value) {
		if (!net_pkt_append_all(pkt, strlen(HTTP_CONTENT_TYPE),
					(u8_t *)HTTP_CONTENT_TYPE,
					timeout)) {
			goto out;
		}

		if (!net_pkt_append_all(pkt, strlen(req->content_type_value),
					(u8_t *)req->content_type_value,
					timeout)) {
			goto out;
		}
	}

	if (req->payload && req->payload_size) {
		char content_len_str[HTTP_CONT_LEN_SIZE];

		ret = snprintk(content_len_str, HTTP_CONT_LEN_SIZE,
			       HTTP_CRLF "Content-Length: %u"
			       HTTP_CRLF HTTP_CRLF,
			       req->payload_size);
		if (ret <= 0 || ret >= HTTP_CONT_LEN_SIZE) {
			ret = -ENOMEM;
			goto out;
		}

		if (!net_pkt_append_all(pkt, ret, (u8_t *)content_len_str,
					timeout)) {
			ret = -ENOMEM;
			goto out;
		}

		if (!net_pkt_append_all(pkt, req->payload_size,
					(u8_t *)req->payload,
					timeout)) {
			ret = -ENOMEM;
			goto out;
		}
	} else {
		if (!net_pkt_append_all(pkt, strlen(HTTP_EOF),
					(u8_t *)HTTP_EOF,
					timeout)) {
			goto out;
		}
	}

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		net_pkt_set_appdatalen(pkt, net_pkt_get_len(pkt) -
				       net_pkt_ip_hdr_len(pkt) -
				       net_pkt_ipv6_ext_opt_len(pkt));
	} else
#endif
	{
		net_pkt_set_appdatalen(pkt, net_pkt_get_len(pkt) -
				       net_pkt_ip_hdr_len(pkt));
	}

	ret = ctx->tcp.send_data(pkt, NULL, timeout, NULL, ctx);
	if (ret == 0) {
		return 0;
	}

out:
	net_pkt_unref(pkt);

	return ret;
}

static void print_header_field(size_t len, const char *str)
{
#if defined(CONFIG_NET_DEBUG_HTTP)
#define MAX_OUTPUT_LEN 128
	char output[MAX_OUTPUT_LEN];

	/* The value of len does not count \0 so we need to increase it
	 * by one.
	 */
	if ((len + 1) > sizeof(output)) {
		len = sizeof(output) - 1;
	}

	snprintk(output, len + 1, "%s", str);

	NET_DBG("[%zd] %s", len, output);
#endif
}

static int on_url(struct http_parser *parser, const char *at, size_t length)
{
	ARG_UNUSED(parser);

	print_header_field(length, at);

	return 0;
}

static int on_status(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx;
	u16_t len;

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);
	len = min(length, sizeof(ctx->rsp.http_status) - 1);
	memcpy(ctx->rsp.http_status, at, len);
	ctx->rsp.http_status[len] = 0;

	NET_DBG("HTTP response status %s", ctx->rsp.http_status);

	return 0;
}

static int on_header_field(struct http_parser *parser, const char *at,
			   size_t length)
{
	char *content_len = "Content-Length";
	struct http_client_ctx *ctx;
	u16_t len;

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);

	len = strlen(content_len);
	if (length >= len && memcmp(at, content_len, len) == 0) {
		ctx->rsp.cl_present = true;
	}

	print_header_field(length, at);

	return 0;
}

#define MAX_NUM_DIGITS	16

static int on_header_value(struct http_parser *parser, const char *at,
			   size_t length)
{
	struct http_client_ctx *ctx;
	char str[MAX_NUM_DIGITS];

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);

	if (ctx->rsp.cl_present) {
		if (length <= MAX_NUM_DIGITS - 1) {
			long int num;

			memcpy(str, at, length);
			str[length] = 0;
			num = strtol(str, NULL, 10);
			if (num == LONG_MIN || num == LONG_MAX) {
				return -EINVAL;
			}

			ctx->rsp.content_length = num;
		}

		ctx->rsp.cl_present = false;
	}

	print_header_field(length, at);

	return 0;
}

static int on_body(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	ctx->rsp.body_found = 1;
	ctx->rsp.processed += length;

	NET_DBG("Processed %zd length %zd", ctx->rsp.processed, length);

	if (!ctx->rsp.body_start) {
		ctx->rsp.body_start = (u8_t *)at;
	}

	if (ctx->rsp.cb) {
		NET_DBG("Calling callback for partitioned %zd len data",
			ctx->rsp.data_len);

		ctx->rsp.cb(ctx,
			    ctx->rsp.response_buf,
			    ctx->rsp.response_buf_len,
			    ctx->rsp.data_len,
			    HTTP_DATA_MORE,
			    ctx->req.user_data);

		/* Re-use the result buffer and start to fill it again */
		ctx->rsp.data_len = 0;
	}

	return 0;
}

static int on_headers_complete(struct http_parser *parser)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	if (parser->status_code >= 500 && parser->status_code < 600) {
		NET_DBG("Status %d, skipping body", parser->status_code);

		return 1;
	}

	if ((ctx->req.method == HTTP_HEAD || ctx->req.method == HTTP_OPTIONS)
	    && ctx->rsp.content_length > 0) {
		NET_DBG("No body expected");
		return 1;
	}

	NET_DBG("Headers complete");

	return 0;
}

static int on_message_begin(struct http_parser *parser)
{
#if defined(CONFIG_NET_DEBUG_HTTP) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	NET_DBG("-- HTTP %s response (headers) --",
		http_method_str(ctx->req.method));
#else
	ARG_UNUSED(parser);
#endif
	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	NET_DBG("-- HTTP %s response (complete) --",
		http_method_str(ctx->req.method));

	if (ctx->rsp.cb) {
		ctx->rsp.cb(ctx,
			    ctx->rsp.response_buf,
			    ctx->rsp.response_buf_len,
			    ctx->rsp.data_len,
			    HTTP_DATA_FINAL,
			    ctx->req.user_data);
	}

	k_sem_give(&ctx->req.wait);

	return 0;
}

static int on_chunk_header(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

static int on_chunk_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

static void http_receive_cb(struct http_client_ctx *ctx,
			    struct net_pkt *pkt)
{
	size_t start = ctx->rsp.data_len;
	size_t len = 0;
	struct net_buf *frag;
	int header_len;

	if (!pkt) {
		return;
	}

	/* Get rid of possible IP headers in the first fragment. */
	frag = pkt->frags;

	header_len = net_pkt_appdata(pkt) - frag->data;

	NET_DBG("Received %d bytes data", net_pkt_appdatalen(pkt));

	/* After this pull, the frag->data points directly to application data.
	 */
	net_buf_pull(frag, header_len);

	while (frag) {
		/* If this fragment cannot be copied to result buf,
		 * then parse what we have which will cause the callback to be
		 * called in function on_body(), and continue copying.
		 */
		if (ctx->rsp.data_len + frag->len > ctx->rsp.response_buf_len) {

			/* If the caller has not supplied a callback, then
			 * we cannot really continue if the response buffer
			 * overflows. Set the data_len to mark how many bytes
			 * should be needed in the response_buf.
			 */
			if (!ctx->rsp.cb) {
				ctx->rsp.data_len = net_pkt_get_len(pkt);
				goto out;
			}

			http_parser_execute(&ctx->parser,
					    &ctx->settings,
					    ctx->rsp.response_buf + start,
					    len);

			ctx->rsp.data_len = 0;
			len = 0;
			start = 0;
		}

		memcpy(ctx->rsp.response_buf + ctx->rsp.data_len,
		       frag->data, frag->len);

		ctx->rsp.data_len += frag->len;
		len += frag->len;
		frag = frag->frags;
	}

out:
	/* The parser's error can be catched outside, reading the
	 * http_errno struct member
	 */
	http_parser_execute(&ctx->parser, &ctx->settings,
			    ctx->rsp.response_buf + start, len);

	net_pkt_unref(pkt);
}

int client_reset(struct http_client_ctx *ctx)
{
	http_parser_init(&ctx->parser, HTTP_RESPONSE);

	memset(ctx->rsp.http_status, 0, sizeof(ctx->rsp.http_status));

	ctx->rsp.cl_present = 0;
	ctx->rsp.content_length = 0;
	ctx->rsp.processed = 0;
	ctx->rsp.body_found = 0;
	ctx->rsp.body_start = NULL;

	memset(ctx->rsp.response_buf, 0, ctx->rsp.response_buf_len);
	ctx->rsp.data_len = 0;

	return 0;
}

static void tcp_disconnect(struct http_client_ctx *ctx)
{
	if (ctx->tcp.ctx) {
		net_context_put(ctx->tcp.ctx);
		ctx->tcp.ctx = NULL;
	}
}

static void recv_cb(struct net_context *net_ctx, struct net_pkt *pkt,
		    int status, void *data)
{
	struct http_client_ctx *ctx = data;

	ARG_UNUSED(net_ctx);

	if (status) {
		return;
	}

	if (!pkt || net_pkt_appdatalen(pkt) == 0) {
		goto out;
	}

	/* receive_cb must take ownership of the received packet */
	if (ctx->tcp.receive_cb) {
		ctx->tcp.receive_cb(ctx, pkt);
		return;
	}

out:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

static int get_local_addr(struct http_client_ctx *ctx)
{
	if (ctx->tcp.local.family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		struct in6_addr *dst = &net_sin6(&ctx->tcp.remote)->sin6_addr;

		net_ipaddr_copy(&net_sin6(&ctx->tcp.local)->sin6_addr,
				net_if_ipv6_select_src_addr(NULL, dst));
#else
		return -EPFNOSUPPORT;
#endif
	} else if (ctx->tcp.local.family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_if_get_default();

		/* For IPv4 we take the first address in the interface */
		net_ipaddr_copy(&net_sin(&ctx->tcp.local)->sin_addr,
				&iface->ipv4.unicast[0].address.in_addr);
#else
		return -EPFNOSUPPORT;
#endif
	}

	return 0;
}

static int tcp_connect(struct http_client_ctx *ctx)
{
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int ret;

	if (ctx->tcp.ctx && net_context_is_used(ctx->tcp.ctx) &&
	    net_context_get_state(ctx->tcp.ctx) == NET_CONTEXT_CONNECTED) {
		/* If we are already connected, then just return */
		return -EALREADY;
	}

	if (ctx->tcp.remote.family == AF_INET6) {
		addrlen = sizeof(struct sockaddr_in6);

		/* If we are reconnecting, then make sure the source port
		 * is re-calculated so that the peer will not get confused
		 * which connection the connection is related to.
		 * This was seen in Linux which dropped packets when the same
		 * source port was for a new connection after the old connection
		 * was terminated.
		 */
		net_sin6(&ctx->tcp.local)->sin6_port = 0;
	} else {
		net_sin(&ctx->tcp.local)->sin_port = 0;
	}

	ret = get_local_addr(ctx);
	if (ret < 0) {
		NET_DBG("Cannot get local address (%d)", ret);
		return ret;
	}

	ret = net_context_get(ctx->tcp.remote.family, SOCK_STREAM,
			      IPPROTO_TCP, &ctx->tcp.ctx);
	if (ret) {
		NET_DBG("Get context error (%d)", ret);
		return ret;
	}

	ret = net_context_bind(ctx->tcp.ctx, &ctx->tcp.local,
			       addrlen);
	if (ret) {
		NET_DBG("Bind error (%d)", ret);
		goto out;
	}

	ret = net_context_connect(ctx->tcp.ctx,
				  &ctx->tcp.remote, addrlen,
				  NULL, ctx->tcp.timeout, NULL);
	if (ret) {
		NET_DBG("Connect error (%d)", ret);
		goto out;
	}

	return net_context_recv(ctx->tcp.ctx, ctx->tcp.recv_cb, K_NO_WAIT, ctx);

out:
	net_context_put(ctx->tcp.ctx);
	ctx->tcp.ctx = NULL;

	return ret;
}

#if defined(CONFIG_NET_DEBUG_HTTP)
static void sprint_addr(char *buf, int len,
			sa_family_t family,
			struct sockaddr *addr)
{
	if (family == AF_INET6) {
		net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf, len);
	} else if (family == AF_INET) {
		net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr, buf, len);
	} else {
		NET_DBG("Invalid protocol family");
	}
}
#endif

static inline void print_info(struct http_client_ctx *ctx,
				enum http_method method)
{
#if defined(CONFIG_NET_DEBUG_HTTP)
	char local[NET_IPV6_ADDR_LEN];
	char remote[NET_IPV6_ADDR_LEN];

	sprint_addr(local, NET_IPV6_ADDR_LEN, ctx->tcp.local.family,
		    &ctx->tcp.local);

	sprint_addr(remote, NET_IPV6_ADDR_LEN, ctx->tcp.remote.family,
		    &ctx->tcp.remote);

	NET_DBG("HTTP %s (%s) %s -> %s port %d",
		http_method_str(method), ctx->req.host, local, remote,
		ntohs(net_sin(&ctx->tcp.remote)->sin_port));
#endif
}

#if defined(CONFIG_HTTPS)
#if defined(MBEDTLS_ERROR_C)
#define print_error(fmt, ret)						\
	do {								\
		char error[64];						\
									\
		mbedtls_strerror(ret, error, sizeof(error));		\
									\
		NET_ERR(fmt " (%s)", -ret, error);			\
	} while (0)
#else
#define print_error(fmt, ret) NET_ERR(fmt, -ret)
#endif /* MBEDTLS_ERROR_C */

static void ssl_sent(struct net_context *context,
		     int status, void *token, void *user_data)
{
	struct http_client_ctx *http_ctx = user_data;

	k_sem_give(&http_ctx->https.mbedtls.ssl_ctx.tx_sem);
}

/* Send encrypted data */
static int ssl_tx(void *context, const unsigned char *buf, size_t size)
{
	struct http_client_ctx *ctx = context;
	struct net_pkt *send_pkt;
	int ret, len;

	send_pkt = net_pkt_get_tx(ctx->tcp.ctx, BUF_ALLOC_TIMEOUT);
	if (!send_pkt) {
		return MBEDTLS_ERR_SSL_ALLOC_FAILED;
	}

	ret = net_pkt_append_all(send_pkt, size, (u8_t *)buf,
				 BUF_ALLOC_TIMEOUT);
	if (!ret) {
		/* Cannot append data */
		net_pkt_unref(send_pkt);
		return MBEDTLS_ERR_SSL_ALLOC_FAILED;
	}

	len = size;

	ret = net_context_send(send_pkt, ssl_sent, K_NO_WAIT, NULL, ctx);
	if (ret < 0) {
		net_pkt_unref(send_pkt);
		return ret;
	}

	k_sem_take(&ctx->https.mbedtls.ssl_ctx.tx_sem, K_FOREVER);

	return len;
}

struct rx_fifo_block {
	sys_snode_t snode;
	struct k_mem_block block;
	struct net_pkt *pkt;
};

struct tx_fifo_block {
	struct k_mem_block block;
	struct http_client_request *req;
};

/* Receive encrypted data from network. Put that data into fifo
 * that will be read by https thread.
 */
static void ssl_received(struct net_context *context,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	struct http_client_ctx *http_ctx = user_data;
	struct rx_fifo_block *rx_data = NULL;
	struct k_mem_block block;
	int ret;

	ARG_UNUSED(context);
	ARG_UNUSED(status);

	if (pkt && !net_pkt_appdatalen(pkt)) {
		net_pkt_unref(pkt);
		return;
	}

	ret = k_mem_pool_alloc(http_ctx->https.pool, &block,
			       sizeof(struct rx_fifo_block),
			       BUF_ALLOC_TIMEOUT);
	if (ret < 0) {
		if (pkt) {
			net_pkt_unref(pkt);
		}

		return;
	}

	rx_data = block.data;
	rx_data->pkt = pkt;

	/* For freeing memory later */
	memcpy(&rx_data->block, &block, sizeof(struct k_mem_block));

	k_fifo_put(&http_ctx->https.mbedtls.ssl_ctx.rx_fifo, (void *)rx_data);

	/* Let the ssl_rx() to run */
	k_yield();
}

int ssl_rx(void *context, unsigned char *buf, size_t size)
{
	struct http_client_ctx *ctx = context;
	struct rx_fifo_block *rx_data;
	u16_t read_bytes;
	u8_t *ptr;
	int pos;
	int len;
	int ret = 0;

	if (!ctx->https.mbedtls.ssl_ctx.frag) {
		rx_data = k_fifo_get(&ctx->https.mbedtls.ssl_ctx.rx_fifo,
				     K_FOREVER);
		if (!rx_data || !rx_data->pkt) {
			NET_DBG("Closing %p connection", ctx);

			if (rx_data) {
				k_mem_pool_free(&rx_data->block);
			}

			return MBEDTLS_ERR_SSL_CONN_EOF;
		}

		ctx->https.mbedtls.ssl_ctx.rx_pkt = rx_data->pkt;

		k_mem_pool_free(&rx_data->block);

		read_bytes = net_pkt_appdatalen(
					ctx->https.mbedtls.ssl_ctx.rx_pkt);

		ctx->https.mbedtls.ssl_ctx.remaining = read_bytes;
		ctx->https.mbedtls.ssl_ctx.frag =
			ctx->https.mbedtls.ssl_ctx.rx_pkt->frags;

		ptr = net_pkt_appdata(ctx->https.mbedtls.ssl_ctx.rx_pkt);
		len = ptr - ctx->https.mbedtls.ssl_ctx.frag->data;

		if (len > ctx->https.mbedtls.ssl_ctx.frag->size) {
			NET_ERR("Buf overflow (%d > %u)", len,
				ctx->https.mbedtls.ssl_ctx.frag->size);
			return -EINVAL;
		}

		/* This will get rid of IP header */
		net_buf_pull(ctx->https.mbedtls.ssl_ctx.frag, len);
	} else {
		read_bytes = ctx->https.mbedtls.ssl_ctx.remaining;
		ptr = ctx->https.mbedtls.ssl_ctx.frag->data;
	}

	len = ctx->https.mbedtls.ssl_ctx.frag->len;
	pos = 0;
	if (read_bytes > size) {
		while (ctx->https.mbedtls.ssl_ctx.frag) {
			read_bytes = len < (size - pos) ? len : (size - pos);

#if RX_EXTRA_DEBUG == 1
			NET_DBG("Copying %d bytes", read_bytes);
#endif

			memcpy(buf + pos, ptr, read_bytes);

			pos += read_bytes;
			if (pos < size) {
				ctx->https.mbedtls.ssl_ctx.frag =
					ctx->https.mbedtls.ssl_ctx.frag->frags;
				ptr = ctx->https.mbedtls.ssl_ctx.frag->data;
				len = ctx->https.mbedtls.ssl_ctx.frag->len;
			} else {
				if (read_bytes == len) {
					ctx->https.mbedtls.ssl_ctx.frag =
					ctx->https.mbedtls.ssl_ctx.frag->frags;
				} else {
					net_buf_pull(
					       ctx->https.mbedtls.ssl_ctx.frag,
					       read_bytes);
				}

				ctx->https.mbedtls.ssl_ctx.remaining -= size;
				return size;
			}
		}
	} else {
		while (ctx->https.mbedtls.ssl_ctx.frag) {
#if RX_EXTRA_DEBUG == 1
			NET_DBG("Copying all %d bytes", len);
#endif

			memcpy(buf + pos, ptr, len);

			pos += len;
			ctx->https.mbedtls.ssl_ctx.frag =
				ctx->https.mbedtls.ssl_ctx.frag->frags;
			if (!ctx->https.mbedtls.ssl_ctx.frag) {
				break;
			}

			ptr = ctx->https.mbedtls.ssl_ctx.frag->data;
			len = ctx->https.mbedtls.ssl_ctx.frag->len;
		}

		net_pkt_unref(ctx->https.mbedtls.ssl_ctx.rx_pkt);
		ctx->https.mbedtls.ssl_ctx.rx_pkt = NULL;
		ctx->https.mbedtls.ssl_ctx.frag = NULL;
		ctx->https.mbedtls.ssl_ctx.remaining = 0;

		if (read_bytes != pos) {
			return -EIO;
		}

		ret = read_bytes;
	}

	return ret;
}

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_HTTP)
static void my_debug(void *ctx, int level,
		     const char *file, int line, const char *str)
{
	const char *p, *basename;
	int len;

	ARG_UNUSED(ctx);

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}

	}

	/* Avoid printing double newlines */
	len = strlen(str);
	if (str[len - 1] == '\n') {
		((char *)str)[len - 1] = '\0';
	}

	NET_DBG("%s:%04d: |%d| %s", basename, line, level, str);
}
#endif /* MBEDTLS_DEBUG_C && CONFIG_NET_DEBUG_HTTP */

static int entropy_source(void *data, unsigned char *output, size_t len,
			  size_t *olen)
{
	u32_t seed;

	ARG_UNUSED(data);

	seed = sys_rand32_get();

	if (len > sizeof(seed)) {
		len = sizeof(seed);
	}

	memcpy(output, &seed, len);

	*olen = len;
	return 0;
}

static int https_init(struct http_client_ctx *ctx)
{
	int ret = 0;

	k_sem_init(&ctx->https.mbedtls.ssl_ctx.tx_sem, 0, UINT_MAX);
	k_fifo_init(&ctx->https.mbedtls.ssl_ctx.rx_fifo);
	k_fifo_init(&ctx->https.mbedtls.ssl_ctx.tx_fifo);

	mbedtls_platform_set_printf(printk);

	http_heap_init();

	mbedtls_ssl_init(&ctx->https.mbedtls.ssl);
	mbedtls_ssl_config_init(&ctx->https.mbedtls.conf);
	mbedtls_entropy_init(&ctx->https.mbedtls.entropy);
	mbedtls_ctr_drbg_init(&ctx->https.mbedtls.ctr_drbg);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_init(&ctx->https.mbedtls.ca_cert);
#endif

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_HTTP)
	mbedtls_debug_set_threshold(DEBUG_THRESHOLD);
	mbedtls_ssl_conf_dbg(&ctx->https.mbedtls.conf, my_debug, NULL);
#endif

	/* Seed the RNG */
	mbedtls_entropy_add_source(&ctx->https.mbedtls.entropy,
				   ctx->https.mbedtls.entropy_src_cb,
				   NULL,
				   MBEDTLS_ENTROPY_MAX_GATHER,
				   MBEDTLS_ENTROPY_SOURCE_STRONG);

	ret = mbedtls_ctr_drbg_seed(
		&ctx->https.mbedtls.ctr_drbg,
		mbedtls_entropy_func,
		&ctx->https.mbedtls.entropy,
		(const unsigned char *)ctx->https.mbedtls.personalization_data,
		ctx->https.mbedtls.personalization_data_len);
	if (ret != 0) {
		print_error("mbedtls_ctr_drbg_seed returned -0x%x", ret);
		goto exit;
	}

	/* Setup SSL defaults etc. */
	ret = mbedtls_ssl_config_defaults(&ctx->https.mbedtls.conf,
					  MBEDTLS_SSL_IS_CLIENT,
					  MBEDTLS_SSL_TRANSPORT_STREAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		print_error("mbedtls_ssl_config_defaults returned -0x%x", ret);
		goto exit;
	}

	mbedtls_ssl_conf_rng(&ctx->https.mbedtls.conf,
			     mbedtls_ctr_drbg_random,
			     &ctx->https.mbedtls.ctr_drbg);

	/* Load the certificates and other related stuff. This needs to be done
	 * by the user so we call a callback that user must have provided.
	 */
	ret = ctx->https.mbedtls.cert_cb(ctx, &ctx->https.mbedtls.ca_cert);
	if (ret != 0) {
		goto exit;
	}

	ret = mbedtls_ssl_setup(&ctx->https.mbedtls.ssl,
				&ctx->https.mbedtls.conf);
	if (ret != 0) {
		NET_ERR("mbedtls_ssl_setup returned -0x%x", ret);
		goto exit;
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (ctx->https.cert_host) {
		ret = mbedtls_ssl_set_hostname(&ctx->https.mbedtls.ssl,
					       ctx->https.cert_host);
		if (ret != 0) {
			print_error("mbedtls_ssl_set_hostname returned -0x%x",
				    ret);
			goto exit;
		}
	}
#endif

	NET_DBG("SSL setup done");

	/* The HTTPS thread is started do initiate HTTPS handshake etc when
	 * the first HTTP request is being done.
	 */

exit:
	return ret;
}

static void https_handler(struct http_client_ctx *ctx,
			  struct k_sem *startup_sync)
{
	struct tx_fifo_block *tx_data;
	struct http_client_request req;
	size_t len;
	int ret;

	/* First mbedtls specific initialization */
	ret = https_init(ctx);

	k_sem_give(startup_sync);

	if (ret < 0) {
		return;
	}

reset:
	http_parser_init(&ctx->parser, HTTP_RESPONSE);
	ctx->rsp.data_len = 0;

	/* Wait that the sender sends the data, and the peer to respond to.
	 */
	tx_data = k_fifo_get(&ctx->https.mbedtls.ssl_ctx.tx_fifo, K_FOREVER);
	if (tx_data) {
		/* Because the req pointer might disappear as it is controlled
		 * by application, copy the data here.
		 */
		memcpy(&req, tx_data->req, sizeof(req));
	} else {
		NET_ASSERT(tx_data);
		goto reset;
	}

	print_info(ctx, ctx->req.method);

	/* If the connection is not active, then re-connect */
	ret = tcp_connect(ctx);
	if (ret < 0 && ret != -EALREADY) {
		k_sem_give(&ctx->req.wait);
		goto reset;
	}

	mbedtls_ssl_session_reset(&ctx->https.mbedtls.ssl);
	mbedtls_ssl_set_bio(&ctx->https.mbedtls.ssl, ctx, ssl_tx,
			    ssl_rx, NULL);

	/* SSL handshake. The ssl_rx() function will be called next by
	 * mbedtls library. The ssl_rx() will block and wait that data is
	 * received by ssl_received() and passed to it via fifo. After
	 * receiving the data, this function will then proceed with secure
	 * connection establishment.
	 */
	/* Waiting SSL handshake */
	do {
		ret = mbedtls_ssl_handshake(&ctx->https.mbedtls.ssl);
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret == MBEDTLS_ERR_SSL_CONN_EOF) {
				goto close;
			}

			if (ret < 0) {
				print_error("mbedtls_ssl_handshake returned "
					    "-0x%x", ret);
				goto close;
			}
		}
	} while (ret != 0);

	ret = http_request(ctx, &req, BUF_ALLOC_TIMEOUT);

	k_mem_pool_free(&tx_data->block);

	if (ret < 0) {
		NET_DBG("Send error (%d)", ret);
		goto close;
	}

	NET_DBG("Read HTTPS response");

	do {
		len = ctx->rsp.response_buf_len - 1;
		memset(ctx->rsp.response_buf, 0, ctx->rsp.response_buf_len);

		ret = mbedtls_ssl_read(&ctx->https.mbedtls.ssl,
				       ctx->rsp.response_buf, len);
		if (ret == 0) {
			goto close;
		}

		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}

		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			NET_DBG("Connection was closed gracefully");
			goto close;
		}

		if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
			NET_DBG("Connection was reset by peer");
			goto close;
		}

		if (ret == -EIO) {
			NET_DBG("Response received, waiting another ctx %p",
				ctx);
			goto next;
		}

		if (ret < 0) {
			print_error("mbedtls_ssl_read returned -0x%x", ret);
			goto close;
		}

		/* The data_len will count how many bytes we have read,
		 * this value is passed to user supplied response callback
		 * by on_body() and on_message_complete() functions.
		 */
		ctx->rsp.data_len += ret;

		ret = http_parser_execute(&ctx->parser,
					  &ctx->settings,
					  ctx->rsp.response_buf,
					  ret);
		if (!ret) {
			goto close;
		}

		ctx->rsp.data_len = 0;

		if (ret > 0) {
			/* Get more data */
			ret = MBEDTLS_ERR_SSL_WANT_READ;
		}
	} while (ret < 0);

close:
	/* If there is any pending data that have not been processed yet,
	 * we need to free it here.
	 */
	if (ctx->https.mbedtls.ssl_ctx.rx_pkt) {
		net_pkt_unref(ctx->https.mbedtls.ssl_ctx.rx_pkt);
		ctx->https.mbedtls.ssl_ctx.rx_pkt = NULL;
		ctx->https.mbedtls.ssl_ctx.frag = NULL;
	}

	NET_DBG("Resetting HTTPS connection %p", ctx);

	tcp_disconnect(ctx);

next:
	mbedtls_ssl_close_notify(&ctx->https.mbedtls.ssl);

	goto reset;
}

static void https_shutdown(struct http_client_ctx *ctx)
{
	if (!ctx->https.tid) {
		return;
	}

	/* Empty the fifo just in case there is any received packets
	 * still there.
	 */
	while (1) {
		struct rx_fifo_block *rx_data;

		rx_data = k_fifo_get(&ctx->https.mbedtls.ssl_ctx.rx_fifo,
				     K_NO_WAIT);
		if (!rx_data) {
			break;
		}

		net_pkt_unref(rx_data->pkt);

		k_mem_pool_free(&rx_data->block);
	}

	k_fifo_cancel_wait(&ctx->https.mbedtls.ssl_ctx.rx_fifo);

	/* Let the ssl_rx() run if there is anything there waiting */
	k_yield();

	mbedtls_ssl_close_notify(&ctx->https.mbedtls.ssl);
	mbedtls_ssl_free(&ctx->https.mbedtls.ssl);
	mbedtls_ssl_config_free(&ctx->https.mbedtls.conf);
	mbedtls_ctr_drbg_free(&ctx->https.mbedtls.ctr_drbg);
	mbedtls_entropy_free(&ctx->https.mbedtls.entropy);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_free(&ctx->https.mbedtls.ca_cert);
#endif

	tcp_disconnect(ctx);

	NET_DBG("HTTPS thread %p stopped for %p", ctx->https.tid, ctx);

	k_thread_abort(ctx->https.tid);
	ctx->https.tid = 0;
}

static int start_https(struct http_client_ctx *ctx)
{
	struct k_sem startup_sync;

	/* Start the thread that handles HTTPS traffic. */
	if (ctx->https.tid) {
		return -EALREADY;
	}

	NET_DBG("Starting HTTPS thread for %p", ctx);

	k_sem_init(&startup_sync, 0, 1);

	ctx->https.tid = k_thread_create(&ctx->https.thread,
					 ctx->https.stack,
					 ctx->https.stack_size,
					 (k_thread_entry_t)https_handler,
					 ctx, &startup_sync, 0,
					 K_PRIO_COOP(7), 0, 0);

	/* Wait until we know that the HTTPS thread startup was ok */
	if (k_sem_take(&startup_sync, HTTPS_STARTUP_TIMEOUT) < 0) {
		https_shutdown(ctx);
		return -ECANCELED;
	}

	NET_DBG("HTTPS thread %p started for %p", ctx->https.tid, ctx);

	return 0;
}
#else
#define start_https(...) 0
#endif /* CONFIG_HTTPS */

int http_client_send_req(struct http_client_ctx *ctx,
			 struct http_client_request *req,
			 http_response_cb_t cb,
			 u8_t *response_buf,
			 size_t response_buf_len,
			 void *user_data,
			 s32_t timeout)
{
	int ret;

	if (!response_buf || response_buf_len == 0) {
		return -EINVAL;
	}

	ctx->rsp.response_buf = response_buf;
	ctx->rsp.response_buf_len = response_buf_len;

	client_reset(ctx);

	/* HTTPS connection is established in https_handler() */
	if (!ctx->is_https) {
		ret = tcp_connect(ctx);
		if (ret < 0 && ret != -EALREADY) {
			NET_DBG("TCP connect error (%d)", ret);
			return ret;
		}
	}

	if (!req->host) {
		req->host = ctx->server;
	}

	ctx->req.host = req->host;
	ctx->req.method = req->method;
	ctx->req.user_data = user_data;

	ctx->rsp.cb = cb;

#if defined(CONFIG_HTTPS)
	if (ctx->is_https) {
		struct tx_fifo_block *tx_data;
		struct k_mem_block block;

		ret = start_https(ctx);
		if (ret != 0 && ret != -EALREADY) {
			NET_ERR("HTTPS init failed (%d)", ret);
			goto out;
		}

		ret = k_mem_pool_alloc(ctx->https.pool, &block,
				       sizeof(struct tx_fifo_block),
				       BUF_ALLOC_TIMEOUT);
		if (ret < 0) {
			goto out;
		}

		tx_data = block.data;
		tx_data->req = req;

		memcpy(&tx_data->block, &block, sizeof(struct k_mem_block));

		/* We need to pass the HTTPS request to HTTPS thread because
		 * of the mbedtls API stack size requirements.
		 */
		k_fifo_put(&ctx->https.mbedtls.ssl_ctx.tx_fifo,
			   (void *)tx_data);

		/* Let the https_handler() to start to process the message.
		 *
		 * Note that if the timeout > 0 or is K_FOREVER, then this
		 * yield is not really necessary as the k_sem_take() will
		 * let the https handler thread to run. But if the timeout
		 * is K_NO_WAIT, then we need to let the https handler to
		 * run now.
		 */
		k_yield();
	} else
#endif /* CONFIG_HTTPS */
	{
		print_info(ctx, ctx->req.method);

		ret = http_request(ctx, req, BUF_ALLOC_TIMEOUT);
		if (ret < 0) {
			NET_DBG("Send error (%d)", ret);
			goto out;
		}
	}

	if (timeout != 0 && k_sem_take(&ctx->req.wait, timeout)) {
		ret = -ETIMEDOUT;
		goto out;
	}

	if (timeout == 0) {
		return -EINPROGRESS;
	}

	return 0;

out:
	tcp_disconnect(ctx);

	return ret;
}

#if defined(CONFIG_DNS_RESOLVER)
static void dns_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	struct waiter *waiter = user_data;
	struct http_client_ctx *ctx = waiter->ctx;

	if (!(status == DNS_EAI_INPROGRESS && info)) {
		return;
	}

	if (info->ai_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		net_ipaddr_copy(&net_sin(&ctx->tcp.remote)->sin_addr,
				&net_sin(&info->ai_addr)->sin_addr);
#else
		goto out;
#endif
	} else if (info->ai_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		net_ipaddr_copy(&net_sin6(&ctx->tcp.remote)->sin6_addr,
				&net_sin6(&info->ai_addr)->sin6_addr);
#else
		goto out;
#endif
	} else {
		goto out;
	}

	ctx->tcp.remote.family = info->ai_family;

out:
	k_sem_give(&waiter->wait);
}

#define DNS_WAIT K_SECONDS(2)
#define DNS_WAIT_SEM (DNS_WAIT + K_SECONDS(1))

static int resolve_name(struct http_client_ctx *ctx,
			const char *server,
			enum dns_query_type type)
{
	struct waiter dns_waiter;
	int ret;

	dns_waiter.ctx = ctx;
	k_sem_init(&dns_waiter.wait, 0, 1);

	ret = dns_get_addr_info(server, type, &ctx->dns_id, dns_cb,
				&dns_waiter, DNS_WAIT);
	if (ret < 0) {
		NET_ERR("Cannot resolve %s (%d)", server, ret);
		ctx->dns_id = 0;
		return ret;
	}

	/* Wait a little longer for the DNS to finish so that
	 * the DNS will timeout before the semaphore.
	 */
	if (k_sem_take(&dns_waiter.wait, DNS_WAIT_SEM)) {
		NET_ERR("Timeout while resolving %s", server);
		ctx->dns_id = 0;
		return -ETIMEDOUT;
	}

	ctx->dns_id = 0;

	if (ctx->tcp.remote.family == AF_UNSPEC) {
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_DNS_RESOLVER */

static inline int set_remote_addr(struct http_client_ctx *ctx,
				  const char *server, u16_t server_port)
{
	int ret;

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
	ret = net_addr_pton(AF_INET6, server,
			    &net_sin6(&ctx->tcp.remote)->sin6_addr);
	if (ret < 0) {
		/* Could be hostname, try DNS if configured. */
#if !defined(CONFIG_DNS_RESOLVER)
		NET_ERR("Invalid IPv6 address %s", server);
		return -EINVAL;
#else
		ret = resolve_name(ctx, server, DNS_QUERY_TYPE_AAAA);
		if (ret < 0) {
			NET_ERR("Cannot resolve %s (%d)", server, ret);
			return ret;
		}
#endif
	}

	net_sin6(&ctx->tcp.remote)->sin6_port = htons(server_port);
	net_sin6(&ctx->tcp.remote)->sin6_family = AF_INET6;
#endif /* IPV6 && !IPV4 */

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	ret = net_addr_pton(AF_INET, server,
			    &net_sin(&ctx->tcp.remote)->sin_addr);
	if (ret < 0) {
		/* Could be hostname, try DNS if configured. */
#if !defined(CONFIG_DNS_RESOLVER)
		NET_ERR("Invalid IPv4 address %s", server);
		return -EINVAL;
#else
		ret = resolve_name(ctx, server, DNS_QUERY_TYPE_A);
		if (ret < 0) {
			NET_ERR("Cannot resolve %s (%d)", server, ret);
			return ret;
		}
#endif
	}

	net_sin(&ctx->tcp.remote)->sin_port = htons(server_port);
	net_sin(&ctx->tcp.remote)->sin_family = AF_INET;
#endif /* IPV6 && !IPV4 */

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	ret = net_addr_pton(AF_INET, server,
			    &net_sin(&ctx->tcp.remote)->sin_addr);
	if (ret < 0) {
		ret = net_addr_pton(AF_INET6, server,
				    &net_sin6(&ctx->tcp.remote)->sin6_addr);
		if (ret < 0) {
			/* Could be hostname, try DNS if configured. */
#if !defined(CONFIG_DNS_RESOLVER)
			NET_ERR("Invalid IPv4 or IPv6 address %s", server);
			return -EINVAL;
#else
			ret = resolve_name(ctx, server, DNS_QUERY_TYPE_A);
			if (ret < 0) {
				ret = resolve_name(ctx, server,
						   DNS_QUERY_TYPE_AAAA);
				if (ret < 0) {
					NET_ERR("Cannot resolve %s (%d)",
						server, ret);
					return ret;
				}

				goto ipv6;
			}

			goto ipv4;
#endif /* !CONFIG_DNS_RESOLVER */
		} else {
#if defined(CONFIG_DNS_RESOLVER)
		ipv6:
#endif
			net_sin6(&ctx->tcp.remote)->sin6_port =
				htons(server_port);
			net_sin6(&ctx->tcp.remote)->sin6_family = AF_INET6;
		}
	} else {
#if defined(CONFIG_DNS_RESOLVER)
	ipv4:
#endif
		net_sin(&ctx->tcp.remote)->sin_port = htons(server_port);
		net_sin(&ctx->tcp.remote)->sin_family = AF_INET;
	}
#endif /* IPV4 && IPV6 */

	/* If we have not yet figured out what is the protocol family,
	 * then we cannot continue.
	 */
	if (ctx->tcp.remote.family == AF_UNSPEC) {
		NET_ERR("Unknown protocol family.");
		return -EPFNOSUPPORT;
	}

	return 0;
}

int http_client_init(struct http_client_ctx *ctx,
		     const char *server, u16_t server_port)
{
	int ret;

	memset(ctx, 0, sizeof(*ctx));

	if (server) {
		ret = set_remote_addr(ctx, server, server_port);
		if (ret < 0) {
			return ret;
		}

		ctx->tcp.local.family = ctx->tcp.remote.family;
		ctx->server = server;
	}

	ctx->settings.on_body = on_body;
	ctx->settings.on_chunk_complete = on_chunk_complete;
	ctx->settings.on_chunk_header = on_chunk_header;
	ctx->settings.on_headers_complete = on_headers_complete;
	ctx->settings.on_header_field = on_header_field;
	ctx->settings.on_header_value = on_header_value;
	ctx->settings.on_message_begin = on_message_begin;
	ctx->settings.on_message_complete = on_message_complete;
	ctx->settings.on_status = on_status;
	ctx->settings.on_url = on_url;

	ctx->tcp.receive_cb = http_receive_cb;
	ctx->tcp.timeout = HTTP_NETWORK_TIMEOUT;
	ctx->tcp.send_data = net_context_send;
	ctx->tcp.recv_cb = recv_cb;

	k_sem_init(&ctx->req.wait, 0, 1);

	return 0;
}

#if defined(CONFIG_HTTPS)
/* This gets plain data and it sends encrypted one to peer */
static int https_send(struct net_pkt *pkt,
		      net_context_send_cb_t cb,
		      s32_t timeout,
		      void *token,
		      void *user_data)
{
	struct http_client_ctx *ctx = user_data;
	int ret;
	u16_t len;

	if (!ctx->rsp.response_buf || ctx->rsp.response_buf_len == 0) {
		NET_DBG("Response buf not setup");
		return -EINVAL;
	}

	len = net_pkt_appdatalen(pkt);
	if (len == 0) {
		NET_DBG("No application data to send");
		return -EINVAL;
	}

	ret = net_frag_linearize(ctx->rsp.response_buf,
				 ctx->rsp.response_buf_len,
				 pkt,
				 net_pkt_ip_hdr_len(pkt) +
				 net_pkt_ipv6_ext_opt_len(pkt),
				 len);
	if (ret < 0) {
		NET_DBG("Cannot linearize send data (%d)", ret);
		return ret;
	}

	if (ret != len) {
		NET_DBG("Linear copy error (%u vs %d)", len, ret);
		return -EINVAL;
	}

	do {
		ret = mbedtls_ssl_write(&ctx->https.mbedtls.ssl,
					ctx->rsp.response_buf, len);
		if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
			NET_ERR("peer closed the connection -0x%x", ret);
			goto out;
		}

		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret < 0) {
				print_error("mbedtls_ssl_write returned -0x%x",
					    ret);
				goto out;
			}
		}
	} while (ret <= 0);

out:
	if (cb) {
		cb(net_pkt_context(pkt), ret, token, user_data);
	}

	return ret;
}

int https_client_init(struct http_client_ctx *ctx,
		      const char *server, u16_t server_port,
		      u8_t *personalization_data,
		      size_t personalization_data_len,
		      https_ca_cert_cb_t cert_cb,
		      const char *cert_host,
		      https_entropy_src_cb_t entropy_src_cb,
		      struct k_mem_pool *pool,
		      u8_t *https_stack,
		      size_t https_stack_size)
{
	int ret;

	if (!cert_cb) {
		NET_ERR("Cert callback must be set");
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(*ctx));

	if (server) {
		ret = set_remote_addr(ctx, server, server_port);
		if (ret < 0) {
			return ret;
		}

		ctx->tcp.local.family = ctx->tcp.remote.family;
		ctx->server = server;
	}

	k_sem_init(&ctx->req.wait, 0, 1);

	ctx->is_https = true;

	ctx->settings.on_body = on_body;
	ctx->settings.on_chunk_complete = on_chunk_complete;
	ctx->settings.on_chunk_header = on_chunk_header;
	ctx->settings.on_headers_complete = on_headers_complete;
	ctx->settings.on_header_field = on_header_field;
	ctx->settings.on_header_value = on_header_value;
	ctx->settings.on_message_begin = on_message_begin;
	ctx->settings.on_message_complete = on_message_complete;
	ctx->settings.on_status = on_status;
	ctx->settings.on_url = on_url;

	ctx->tcp.timeout = HTTP_NETWORK_TIMEOUT;
	ctx->tcp.send_data = https_send;
	ctx->tcp.recv_cb = ssl_received;

	ctx->https.cert_host = cert_host;
	ctx->https.stack = https_stack;
	ctx->https.stack_size = https_stack_size;
	ctx->https.mbedtls.cert_cb = cert_cb;
	ctx->https.pool = pool;
	ctx->https.mbedtls.personalization_data = personalization_data;
	ctx->https.mbedtls.personalization_data_len = personalization_data_len;

	if (entropy_src_cb) {
		ctx->https.mbedtls.entropy_src_cb = entropy_src_cb;
	} else {
		ctx->https.mbedtls.entropy_src_cb = entropy_source;
	}

	/* The mbedtls is initialized in HTTPS thread because of mbedtls stack
	 * requirements.
	 */
	return 0;
}
#endif /* CONFIG_HTTPS */

void http_client_release(struct http_client_ctx *ctx)
{
	if (!ctx) {
		return;
	}

#if defined(CONFIG_HTTPS)
	if (ctx->is_https) {
		https_shutdown(ctx);
	}
#endif /* CONFIG_HTTPS */

	/* https_shutdown() might have released the context already */
	if (ctx->tcp.ctx) {
		net_context_put(ctx->tcp.ctx);
		ctx->tcp.ctx = NULL;
	}

	ctx->tcp.receive_cb = NULL;
	ctx->rsp.cb = NULL;
	k_sem_give(&ctx->req.wait);

#if defined(CONFIG_DNS_RESOLVER)
	if (ctx->dns_id) {
		dns_cancel_addr_info(ctx->dns_id);
	}
#endif

	/* Let all the pending waiters run */
	k_yield();

	memset(ctx, 0, sizeof(*ctx));
}
