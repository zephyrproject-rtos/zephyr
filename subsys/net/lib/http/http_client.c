/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_HTTP)
#define SYS_LOG_DOMAIN "http/client"
#define NET_LOG_ENABLED 1
#endif

#include <stdlib.h>
#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/dns_resolve.h>

#include <net/http.h>

#define BUF_ALLOC_TIMEOUT K_SECONDS(1)

/* HTTP client defines */
#define HTTP_EOF           "\r\n\r\n"

#define HTTP_CONTENT_TYPE  "Content-Type: "
#define HTTP_CONT_LEN_SIZE 64

struct waiter {
	struct http_client_ctx *ctx;
	struct k_sem wait;
};

int http_request(struct net_context *net_ctx, struct http_client_request *req,
		 s32_t timeout)
{
	const char *method = http_method_str(req->method);
	struct net_pkt *pkt;
	int ret = -ENOMEM;

	pkt = net_pkt_get_tx(net_ctx, timeout);
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

	return net_context_send(pkt, NULL, timeout, NULL, NULL);

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
#if defined(CONFIG_NET_DEBUG_HTTP)
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

	return net_context_recv(ctx->tcp.ctx, recv_cb, K_NO_WAIT, ctx);

out:
	net_context_put(ctx->tcp.ctx);

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

	client_reset(ctx);

	ret = tcp_connect(ctx);
	if (ret) {
		NET_DBG("TCP connect error (%d)", ret);
		return ret;
	}

	if (!req->host) {
		req->host = ctx->server;
	}

	ctx->req.host = req->host;
	ctx->req.method = req->method;
	ctx->req.user_data = user_data;

	ctx->rsp.cb = cb;
	ctx->rsp.response_buf = response_buf;
	ctx->rsp.response_buf_len = response_buf_len;

	print_info(ctx, ctx->req.method);

	ret = http_request(ctx->tcp.ctx, req, BUF_ALLOC_TIMEOUT);
	if (ret) {
		NET_DBG("Send error (%d)", ret);
		goto out;
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

	k_sem_init(&ctx->req.wait, 0, 1);

	return 0;
}

void http_client_release(struct http_client_ctx *ctx)
{
	if (!ctx) {
		return;
	}

	net_context_put(ctx->tcp.ctx);
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
