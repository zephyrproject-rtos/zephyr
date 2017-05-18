/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_HTTP)
#if defined(CONFIG_HTTPS)
#define SYS_LOG_DOMAIN "https/server"
#else
#define SYS_LOG_DOMAIN "http/server"
#endif
#define NET_LOG_ENABLED 1
#endif

#define RX_EXTRA_DEBUG 0

#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_context.h>

#include <net/http.h>

#if defined(CONFIG_HTTPS)
static void https_enable(struct http_server_ctx *ctx);
static void https_disable(struct http_server_ctx *ctx);

#if defined(MBEDTLS_DEBUG_C)
#include <mbedtls/debug.h>
#define DEBUG_THRESHOLD 0
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include <mbedtls/memory_buffer_alloc.h>
static unsigned char heap[CONFIG_HTTPS_HEAP_SIZE];
#endif
#endif

#define HTTP_DEFAULT_PORT  80
#define HTTPS_DEFAULT_PORT 443

#define RC_STR(rc)	(rc == 0 ? "OK" : "ERROR")

#define HTTP_STATUS_200_OK	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"\r\n"

#define HTTP_STATUS_400_BR	"HTTP/1.1 400 Bad Request\r\n" \
				"\r\n"

#define HTTP_STATUS_403_FBD	"HTTP/1.1 403 Forbidden\r\n" \
				"\r\n"

#define HTTP_STATUS_404_NF	"HTTP/1.1 404 Not Found\r\n" \
				"\r\n"

static inline u16_t http_strlen(const char *str)
{
	if (str) {
		return strlen(str);
	}

	return 0;
}

static int http_add_header(struct net_pkt *pkt, s32_t timeout, const char *str)
{
	if (net_pkt_append_all(pkt, strlen(str), (u8_t *)str, timeout)) {
		return 0;
	}

	return -ENOMEM;
}

static int http_add_chunk(struct net_pkt *pkt, s32_t timeout, const char *str)
{
	char chunk_header[16];
	u16_t str_len;
	bool ret;

	str_len = http_strlen(str);

	snprintk(chunk_header, sizeof(chunk_header), "%x\r\n", str_len);

	ret = net_pkt_append_all(pkt, strlen(chunk_header), chunk_header,
				 timeout);
	if (!ret) {
		return -ENOMEM;
	}

	if (str_len > 0) {
		ret = net_pkt_append_all(pkt, str_len, (u8_t *)str, timeout);
		if (!ret) {
			return -ENOMEM;
		}
	}

	ret = net_pkt_append_all(pkt, sizeof(HTTP_CRLF) - 1, HTTP_CRLF,
				 timeout);
	if (!ret) {
		return -ENOMEM;
	}

	return 0;
}

static void req_timer_cancel(struct http_server_ctx *ctx)
{
	ctx->req.timer_cancelled = true;
	k_delayed_work_cancel(&ctx->req.timer);

	NET_DBG("Context %p request timer cancelled", ctx);
}

static void req_timeout(struct k_work *work)
{
	struct http_server_ctx *ctx = CONTAINER_OF(work,
						   struct http_server_ctx,
						   req.timer);

	if (ctx->req.timer_cancelled) {
		return;
	}

	NET_DBG("Context %p request timeout", ctx);

	net_context_unref(ctx->req.net_ctx);
}

static void pkt_sent(struct net_context *context,
		     int status,
		     void *token,
		     void *user_data)
{
	s32_t timeout = POINTER_TO_INT(token);
	struct http_server_ctx *ctx = user_data;

	req_timer_cancel(ctx);

	if (timeout == K_NO_WAIT) {
		/* We can just close the context after the packet is sent. */
		net_context_unref(context);
	} else if (timeout > 0) {
		NET_DBG("Context %p starting timer", ctx);

		k_delayed_work_submit(&ctx->req.timer, timeout);

		ctx->req.timer_cancelled = false;
	}

	/* Note that if the timeout is K_FOREVER, we do not close
	 * the connection.
	 */
}

int http_response_wait(struct http_server_ctx *ctx, const char *http_header,
		       const char *html_payload, s32_t timeout)
{
	struct net_pkt *pkt;
	int ret = -EINVAL;

	pkt = net_pkt_get_tx(ctx->req.net_ctx, ctx->timeout);
	if (!pkt) {
		return ret;
	}

	ret = http_add_header(pkt, ctx->timeout, http_header);
	if (ret != 0) {
		goto exit_routine;
	}

	if (html_payload) {
		ret = http_add_chunk(pkt, ctx->timeout, html_payload);
		if (ret != 0) {
			goto exit_routine;
		}

		/* like EOF */
		ret = http_add_chunk(pkt, ctx->timeout, NULL);
		if (ret != 0) {
			goto exit_routine;
		}
	}

	net_pkt_set_appdatalen(pkt, net_buf_frags_len(pkt->frags));

	ret = ctx->send_data(pkt, pkt_sent, 0, INT_TO_POINTER(timeout), ctx);
	if (ret != 0) {
		goto exit_routine;
	}

	pkt = NULL;

exit_routine:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	return ret;
}

int http_response(struct http_server_ctx *ctx, const char *http_header,
		  const char *html_payload)
{
	return http_response_wait(ctx, http_header, html_payload, K_NO_WAIT);
}

int http_response_400(struct http_server_ctx *ctx, const char *html_payload)
{
	return http_response(ctx, HTTP_STATUS_400_BR, html_payload);
}

int http_response_403(struct http_server_ctx *ctx, const char *html_payload)
{
	return http_response(ctx, HTTP_STATUS_403_FBD, html_payload);
}

int http_response_404(struct http_server_ctx *ctx, const char *html_payload)
{
	return http_response(ctx, HTTP_STATUS_404_NF, html_payload);
}

int http_server_set_local_addr(struct sockaddr *addr, const char *myaddr,
			       u16_t port)
{
	if (!addr) {
		return -EINVAL;
	}

	if (myaddr) {
		void *inaddr;

		if (addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			inaddr = &net_sin(addr)->sin_addr;
			net_sin(addr)->sin_port = htons(port);
#else
			return -EPFNOSUPPORT;
#endif
		} else if (addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			inaddr = &net_sin6(addr)->sin6_addr;
			net_sin6(addr)->sin6_port = htons(port);
#else
			return -EPFNOSUPPORT;
#endif
		} else {
			return -EAFNOSUPPORT;
		}

		return net_addr_pton(addr->family, myaddr, inaddr);
	}

	/* If the caller did not supply the address where to bind, then
	 * try to figure it out ourselves.
	 */
	if (addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		net_ipaddr_copy(&net_sin6(addr)->sin6_addr,
				net_if_ipv6_select_src_addr(NULL,
					(struct in6_addr *)
					net_ipv6_unspecified_address()));
#else
		return -EPFNOSUPPORT;
#endif
	} else if (addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_if_get_default();

		/* For IPv4 we take the first address in the interface */
		net_ipaddr_copy(&net_sin(addr)->sin_addr,
				&iface->ipv4.unicast[0].address.in_addr);
#else
		return -EPFNOSUPPORT;
#endif
	}

	return 0;
}

struct http_root_url *http_server_add_url(struct http_server_urls *my,
					  const char *url, u8_t flags,
					  http_url_cb_t write_cb)
{
	int i;

	for (i = 0; i < CONFIG_HTTP_SERVER_NUM_URLS; i++) {
		if (my->urls[i].is_used) {
			continue;
		}

		my->urls[i].is_used = true;
		my->urls[i].root = url;

		/* This will speed-up some future operations */
		my->urls[i].root_len = strlen(url);
		my->urls[i].flags = flags;
		my->urls[i].write_cb = write_cb;

		return &my->urls[i];
	}

	return NULL;
}

int http_server_del_url(struct http_server_urls *my, const char *url)
{
	int i;

	for (i = 0; i < CONFIG_HTTP_SERVER_NUM_URLS; i++) {
		if (!my->urls[i].is_used) {
			continue;
		}

		if (strncmp(my->urls[i].root, url, my->urls[i].root_len)) {
			continue;
		}

		my->urls[i].is_used = false;
		my->urls[i].root = NULL;

		return 0;
	}

	return -ENOENT;
}

struct http_root_url *http_server_add_default(struct http_server_urls *my,
					      http_url_cb_t write_cb)
{
	if (my->default_url.is_used) {
		return NULL;
	}

	my->default_url.is_used = true;
	my->default_url.root = NULL;
	my->default_url.root_len = 0;
	my->default_url.flags = 0;
	my->default_url.write_cb = write_cb;

	return &my->default_url;
}

int http_server_del_default(struct http_server_urls *my)
{
	if (!my->default_url.is_used) {
		return -ENOENT;
	}

	my->default_url.is_used = false;

	return 0;
}

#if defined(CONFIG_NET_DEBUG_HTTP) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
static char *sprint_ipaddr(char *buf, int buflen, const struct sockaddr *addr)
{
	if (addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		char ipaddr[NET_IPV6_ADDR_LEN];

		net_addr_ntop(addr->family,
			      &net_sin6(addr)->sin6_addr,
			      ipaddr, sizeof(ipaddr));
		snprintk(buf, buflen, "[%s]:%u", ipaddr,
			 ntohs(net_sin6(addr)->sin6_port));
#endif
	} else if (addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		char ipaddr[NET_IPV4_ADDR_LEN];

		net_addr_ntop(addr->family,
			      &net_sin(addr)->sin_addr,
			      ipaddr, sizeof(ipaddr));
		snprintk(buf, buflen, "%s:%u", ipaddr,
			 ntohs(net_sin(addr)->sin_port));
#endif
	}

	return buf;
}
#endif /* CONFIG_NET_DEBUG_HTTP */

static inline void new_client(struct http_server_ctx *http_ctx,
			      struct net_context *net_ctx,
			      const struct sockaddr *addr)
{
#if defined(CONFIG_NET_DEBUG_HTTP) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
#if defined(CONFIG_NET_IPV6)
#define PORT_STR sizeof("[]:xxxxx")
	char buf[NET_IPV6_ADDR_LEN + PORT_STR];
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define PORT_STR sizeof(":xxxxx")
	char buf[NET_IPV4_ADDR_LEN + PORT_STR];
#endif

	NET_INFO("%s connection from %s (%p)",
		 http_ctx->is_https ? "HTTPS" : "HTTP",
		 sprint_ipaddr(buf, sizeof(buf), addr),
		 net_ctx);
#endif /* CONFIG_NET_DEBUG_HTTP */
}

static inline void new_server(struct http_server_ctx *ctx,
			      const char *server_banner,
			      const struct sockaddr *addr)
{
#if defined(CONFIG_NET_DEBUG_HTTP) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
#if defined(CONFIG_NET_IPV6)
#define PORT_STR sizeof("[]:xxxxx")
	char buf[NET_IPV6_ADDR_LEN + PORT_STR];
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define PORT_STR sizeof(":xxxxx")
	char buf[NET_IPV4_ADDR_LEN + PORT_STR];
#endif

	if (addr) {
		NET_INFO("%s %s (%p)", server_banner,
			 sprint_ipaddr(buf, sizeof(buf), addr), ctx);
	} else {
		NET_INFO("%s (%p)", server_banner, ctx);
	}
#endif /* CONFIG_NET_DEBUG_HTTP */
}

static int on_header_field(struct http_parser *parser,
			   const char *at, size_t length)
{
	struct http_server_ctx *ctx = parser->data;

	if (ctx->req.field_values_ctr >= CONFIG_HTTP_HEADER_FIELD_ITEMS) {
		return 0;
	}

	ctx->req.field_values[ctx->req.field_values_ctr].key = at;
	ctx->req.field_values[ctx->req.field_values_ctr].key_len = length;

	return 0;
}

static int on_header_value(struct http_parser *parser,
			   const char *at, size_t length)
{
	struct http_server_ctx *ctx = parser->data;

	if (ctx->req.field_values_ctr >= CONFIG_HTTP_HEADER_FIELD_ITEMS) {
		return 0;
	}

	ctx->req.field_values[ctx->req.field_values_ctr].value = at;
	ctx->req.field_values[ctx->req.field_values_ctr].value_len = length;

	ctx->req.field_values_ctr++;

	return 0;
}

static int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct http_server_ctx *ctx = parser->data;

	ctx->req.url = at;
	ctx->req.url_len = length;

	return 0;
}

static int parser_init(struct http_server_ctx *ctx)
{
	memset(ctx->req.field_values, 0, sizeof(ctx->req.field_values));

	ctx->req.settings.on_header_field = on_header_field;
	ctx->req.settings.on_header_value = on_header_value;
	ctx->req.settings.on_url = on_url;

	http_parser_init(&ctx->req.parser, HTTP_REQUEST);

	ctx->req.parser.data = ctx;

	return 0;
}

static int http_url_cmp(const char *url, u16_t url_len,
			const char *root_url, u16_t root_url_len)
{
	if (url_len < root_url_len) {
		return -EINVAL;
	}

	if (memcmp(url, root_url, root_url_len) == 0) {
		if (url_len == root_url_len) {
			return 0;
		}

		/* Here we evaluate the following conditions:
		 * root_url = /images, url = /images/ -> OK
		 * root_url = /images/, url = /images/img.png -> OK
		 * root_url = /images/, url = /images_and_docs -> ERROR
		 */
		if (url_len > root_url_len) {
			if (root_url[root_url_len - 1] == '/') {
				return 0;
			}

			if (url[root_url_len] == '/') {
				return 0;
			}
		}
	}

	return -EINVAL;
}

static struct http_root_url *http_url_find(struct http_server_ctx *http_ctx)
{
	u16_t url_len = http_ctx->req.url_len;
	const char *url = http_ctx->req.url;
	struct http_root_url *root_url;
	u8_t i;
	int ret;

	for (i = 0; i < CONFIG_HTTP_SERVER_NUM_URLS; i++) {
		root_url = &http_ctx->urls->urls[i];
		if (!root_url->is_used) {
			continue;
		}

		ret = http_url_cmp(url, url_len,
				   root_url->root, root_url->root_len);
		if (!ret) {
			return root_url;
		}
	}

	return NULL;
}

static int http_process_recv(struct http_server_ctx *http_ctx)
{
	struct http_root_url *root_url;
	int ret;

	root_url = http_url_find(http_ctx);
	if (!root_url) {
		root_url = &http_ctx->urls->default_url;
		if (!root_url->is_used) {
			NET_DBG("No default handler found (%p)", http_ctx);
			ret = -ENOENT;
			goto out;
		}
	}

	if (root_url->write_cb) {
		NET_DBG("Calling handler %p context %p",
			root_url->write_cb, http_ctx);
		ret = root_url->write_cb(http_ctx);
	} else {
		NET_ERR("No handler for %s", http_ctx->req.url);
		ret = -ENOENT;
	}

out:
	return ret;
}

static void http_recv(struct net_context *net_ctx,
		      struct net_pkt *pkt, int status,
		      void *user_data)
{
	struct http_server_ctx *http_ctx = user_data;
	struct net_buf *frag;
	size_t start = http_ctx->req.data_len;
	int parsed_len;
	int header_len;
	u16_t len = 0, recv_len;

	if (!pkt) {
		NET_DBG("Connection closed by peer");
		return;
	}

	if (!http_ctx->enabled) {
		goto quit;
	}

	recv_len = net_pkt_appdatalen(pkt);
	if (recv_len == 0) {
		/* don't print info about zero-length app data buffers */
		goto quit;
	}

	if (status) {
		NET_DBG("Status %d <%s>", status, RC_STR(status));
		goto out;
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
		if (http_ctx->req.data_len + frag->len >
		    http_ctx->req.request_buf_len) {

			/* If the caller has not supplied a callback, then
			 * we cannot really continue if the request buffer
			 * overflows. Set the data_len to mark how many bytes
			 * should be needed in the response_buf.
			 */
			if (http_process_recv(http_ctx) < 0) {
				http_ctx->req.data_len = net_pkt_get_len(pkt);
				goto out;
			}

			parsed_len =
				http_parser_execute(&http_ctx->req.parser,
						    &http_ctx->req.settings,
						    http_ctx->req.request_buf +
						    start,
						    len);
			if (parsed_len <= 0) {
				goto fail;
			}

			http_ctx->req.data_len = 0;
			len = 0;
			start = 0;
		}

		memcpy(http_ctx->req.request_buf + http_ctx->req.data_len,
		       frag->data, frag->len);

		http_ctx->req.data_len += frag->len;
		len += frag->len;
		frag = frag->frags;
	}

out:
	parsed_len = http_parser_execute(&http_ctx->req.parser,
					 &http_ctx->req.settings,
					 http_ctx->req.request_buf + start,
					 len);
	if (parsed_len < 0) {
fail:
		NET_DBG("Received %u bytes, only parsed %d bytes (%s %s)",
			recv_len, parsed_len,
			http_errno_name(http_ctx->req.parser.http_errno),
			http_errno_description(
				http_ctx->req.parser.http_errno));
	}

	if (http_ctx->req.parser.http_errno != HPE_OK) {
		http_response_400(http_ctx, NULL);
	} else {
		http_process_recv(http_ctx);
	}

quit:
	http_ctx->req.data_len = 0;
	net_pkt_unref(pkt);
}

static void accept_cb(struct net_context *net_ctx,
		      struct sockaddr *addr, socklen_t addrlen,
		      int status, void *data)
{
	struct http_server_ctx *http_ctx = data;

	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	if (status != 0) {
		net_context_put(net_ctx);
		return;
	}

	http_ctx->req.net_ctx = net_ctx;

	new_client(http_ctx, net_ctx, addr);

	net_context_recv(net_ctx, http_ctx->recv_cb, K_NO_WAIT, http_ctx);
}

static int set_net_ctx(struct http_server_ctx *http_ctx,
		       struct net_context *ctx,
		       struct sockaddr *addr,
		       socklen_t socklen)
{
	int ret;

	ret = net_context_bind(ctx, addr, socklen);
	if (ret < 0) {
		NET_ERR("Cannot bind context (%d)", ret);
		goto out;
	}

	ret = net_context_listen(ctx, 0);
	if (ret < 0) {
		NET_ERR("Cannot listen context (%d)", ret);
		goto out;
	}

	ret = net_context_accept(ctx, accept_cb, 0, http_ctx);
	if (ret < 0) {
		NET_ERR("Cannot accept context (%d)", ret);
		goto out;
	}

out:
	return ret;
}

#if defined(CONFIG_NET_IPV4)
static int setup_ipv4_ctx(struct http_server_ctx *http_ctx,
			  struct sockaddr *addr)
{
	socklen_t socklen;
	int ret;

	socklen = sizeof(struct sockaddr_in);

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			      &http_ctx->net_ipv4_ctx);
	if (ret < 0) {
		NET_ERR("Cannot get network context (%d)", ret);
		http_ctx->net_ipv4_ctx = NULL;
		return ret;
	}

	if (addr->family == AF_UNSPEC) {
		addr->family = AF_INET;

		http_server_set_local_addr(addr, NULL,
					   net_sin(addr)->sin_port);
	}

	ret = set_net_ctx(http_ctx, http_ctx->net_ipv4_ctx,
			  addr, socklen);
	if (ret < 0) {
		net_context_put(http_ctx->net_ipv4_ctx);
		http_ctx->net_ipv4_ctx = NULL;
	}

	return ret;
}
#endif

#if defined(CONFIG_NET_IPV6)
int setup_ipv6_ctx(struct http_server_ctx *http_ctx, struct sockaddr *addr)
{
	socklen_t socklen;
	int ret;

	socklen = sizeof(struct sockaddr_in6);

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
			      &http_ctx->net_ipv6_ctx);
	if (ret < 0) {
		NET_ERR("Cannot get network context (%d)", ret);
		http_ctx->net_ipv6_ctx = NULL;
		return ret;
	}

	if (addr->family == AF_UNSPEC) {
		addr->family = AF_INET6;

		http_server_set_local_addr(addr, NULL,
					   net_sin6(addr)->sin6_port);
	}

	ret = set_net_ctx(http_ctx, http_ctx->net_ipv6_ctx,
			  addr, socklen);
	if (ret < 0) {
		net_context_put(http_ctx->net_ipv6_ctx);
		http_ctx->net_ipv6_ctx = NULL;
	}

	return ret;
}
#endif

static int init_net(struct http_server_ctx *ctx,
		    struct sockaddr *server_addr,
		    u16_t port)
{
	struct sockaddr addr;
	int ret;

	memset(&addr, 0, sizeof(addr));

	if (server_addr) {
		memcpy(&addr, server_addr, sizeof(addr));
	} else {
		addr.family = AF_UNSPEC;
		net_sin(&addr)->sin_port = htons(port);
	}

	if (addr.family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		ret = setup_ipv6_ctx(ctx, &addr);
#else
		return -EPFNOSUPPORT;
#endif
	} else if (addr.family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		ret = setup_ipv4_ctx(ctx, &addr);
#else
		return -EPFNOSUPPORT;
#endif
	} else if (addr.family == AF_UNSPEC) {
#if defined(CONFIG_NET_IPV4)
		ret = setup_ipv4_ctx(ctx, &addr);
#endif
		/* We ignore the IPv4 error if IPv6 is enabled */
#if defined(CONFIG_NET_IPV6)
		memset(&addr, 0, sizeof(addr));
		addr.family = AF_UNSPEC;
		net_sin6(&addr)->sin6_port = htons(port);

		ret = setup_ipv6_ctx(ctx, &addr);
#endif
	} else {
		return -EINVAL;
	}

	return ret;
}

bool http_server_enable(struct http_server_ctx *http_ctx)
{
	bool old;

	NET_ASSERT(http_ctx);

	old = http_ctx->enabled;

	http_ctx->enabled = true;

#if defined(CONFIG_HTTPS)
	if (http_ctx->is_https) {
		https_enable(http_ctx);
	}
#endif
	return old;
}

bool http_server_disable(struct http_server_ctx *http_ctx)
{
	bool old;

	NET_ASSERT(http_ctx);

	req_timer_cancel(http_ctx);

	old = http_ctx->enabled;

	http_ctx->enabled = false;

#if defined(CONFIG_HTTPS)
	if (http_ctx->is_https) {
		https_disable(http_ctx);
	}
#endif
	return old;
}

int http_server_init(struct http_server_ctx *http_ctx,
		     struct http_server_urls *urls,
		     struct sockaddr *server_addr,
		     u8_t *request_buf,
		     size_t request_buf_len,
		     const char *server_banner)
{
	int ret;

	if (http_ctx->urls) {
		NET_ERR("Server context %p already initialized", http_ctx);
		return -EINVAL;
	}

	if (!request_buf || request_buf_len == 0) {
		NET_ERR("Request buf must be set");
		return -EINVAL;
	}

	ret = init_net(http_ctx, server_addr, HTTP_DEFAULT_PORT);
	if (ret < 0) {
		return ret;
	}

	if (server_banner) {
		new_server(http_ctx, server_banner, server_addr);
	}

	http_ctx->req.request_buf = request_buf;
	http_ctx->req.request_buf_len = request_buf_len;
	http_ctx->req.data_len = 0;
	http_ctx->urls = urls;
	http_ctx->recv_cb = http_recv;
	http_ctx->send_data = net_context_send;

	k_delayed_work_init(&http_ctx->req.timer, req_timeout);

	parser_init(http_ctx);

	return 0;
}

void http_server_release(struct http_server_ctx *http_ctx)
{
	if (!http_ctx->urls) {
		return;
	}

	http_server_disable(http_ctx);

#if defined(CONFIG_NET_IPV4)
	if (http_ctx->net_ipv4_ctx) {
		net_context_put(http_ctx->net_ipv4_ctx);
		http_ctx->net_ipv4_ctx = NULL;
	}
#endif
#if defined(CONFIG_NET_IPV6)
	if (http_ctx->net_ipv6_ctx) {
		net_context_put(http_ctx->net_ipv6_ctx);
		http_ctx->net_ipv6_ctx = NULL;
	}
#endif

	http_ctx->req.net_ctx = NULL;
	http_ctx->urls = NULL;
}

#if defined(CONFIG_HTTPS)
struct rx_fifo_block {
	sys_snode_t snode;
	struct k_mem_block block;
	struct net_pkt *pkt;
};

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_HTTP)
static void my_debug(void *ctx, int level,
		     const char *file, int line, const char *str)
{
	const char *p, *basename;

	ARG_UNUSED(ctx);

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}

	}

	NET_DBG("%s:%04d: |%d| %s", basename, line, level, str);
}
#endif /* MBEDTLS_DEBUG_C && CONFIG_NET_DEBUG_HTTP */

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
#define print_error(fmt, ret)						\
	do {								\
		NET_ERR(fmt, -ret);					\
	} while (0)
#endif

#define BUF_ALLOC_TIMEOUT 100

/* Receive encrypted data from network. Put that data into fifo
 * that will be read by https thread.
 */
static void ssl_received(struct net_context *context,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	struct http_server_ctx *http_ctx = user_data;
	struct rx_fifo_block *rx_data = NULL;
	struct k_mem_block block;
	int ret;

	ARG_UNUSED(context);
	ARG_UNUSED(status);

	if (!pkt) {
		return;
	}

	if (!net_pkt_appdatalen(pkt)) {
		net_pkt_unref(pkt);
		return;
	}

	ret = k_mem_pool_alloc(http_ctx->https.pool, &block,
			       sizeof(struct rx_fifo_block),
			       BUF_ALLOC_TIMEOUT);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return;
	}

	rx_data = block.data;
	rx_data->pkt = pkt;

	/* For freeing memory later */
	memcpy(&rx_data->block, &block, sizeof(struct k_mem_block));

	k_fifo_put(&http_ctx->https.mbedtls.ssl_ctx.rx_fifo, (void *)rx_data);
}

/* This will copy data from received net_pkt buf into mbedtls internal buffers.
 */
static int ssl_rx(void *context, unsigned char *buf, size_t size)
{
	struct http_server_ctx *ctx = context;
	struct rx_fifo_block *rx_data;
	u16_t read_bytes;
	u8_t *ptr;
	int pos;
	int len;
	int ret = 0;

	if (!ctx->https.mbedtls.ssl_ctx.frag) {
		rx_data = k_fifo_get(&ctx->https.mbedtls.ssl_ctx.rx_fifo,
				     K_FOREVER);

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
		} else {
			/* This will get rid of IP header */
			net_buf_pull(ctx->https.mbedtls.ssl_ctx.frag, len);
		}
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

static void ssl_sent(struct net_context *context,
		     int status, void *token, void *user_data)
{
	struct http_server_ctx *http_ctx = user_data;

	k_sem_give(&http_ctx->https.mbedtls.ssl_ctx.tx_sem);
}

/* Send encrypted data */
static int ssl_tx(void *context, const unsigned char *buf, size_t size)
{
	struct http_server_ctx *ctx = context;
	struct net_pkt *send_buf;
	int ret, len;

	send_buf = net_pkt_get_tx(ctx->req.net_ctx, BUF_ALLOC_TIMEOUT);
	if (!send_buf) {
		return MBEDTLS_ERR_SSL_ALLOC_FAILED;
	}

	ret = net_pkt_append_all(send_buf, size, (u8_t *)buf,
				 BUF_ALLOC_TIMEOUT);
	if (!ret) {
		/* Cannot append data */
		net_pkt_unref(send_buf);
		return 0;
	}

	len = size;

	ret = net_context_send(send_buf, ssl_sent, K_NO_WAIT, NULL, ctx);
	if (ret < 0) {
		net_pkt_unref(send_buf);
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}

	k_sem_take(&ctx->https.mbedtls.ssl_ctx.tx_sem, K_FOREVER);

	return len;
}

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

/* This gets plain data and it sends encrypted one to peer */
static int https_send(struct net_pkt *pkt,
		      net_context_send_cb_t cb,
		      s32_t timeout,
		      void *token,
		      void *user_data)
{
	struct http_server_ctx *ctx = user_data;
	int ret;
	u16_t len;

	len = net_pkt_appdatalen(pkt);

	ret = net_frag_linearize(ctx->req.request_buf,
				 ctx->req.request_buf_len,
				 pkt, net_pkt_ip_hdr_len(pkt),
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
					ctx->req.request_buf, len);
		if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
			print_error("peer closed the connection -0x%x", ret);
			goto out;
		}

		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret < 0) {
				print_error("mbedtls_ssl_write returned "
					    "-0x%x", ret);
				goto out;
			}
		}
	} while (ret <= 0);

out:
	return ret;
}

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
static void heap_init(struct http_server_ctx *ctx)
{
	static bool heap_init;

	if (!heap_init) {
		mbedtls_memory_buffer_alloc_init(heap, sizeof(heap));
		heap_init = true;
	}
}
#else
#define heap_init(...)
#endif

static void https_handler(struct http_server_ctx *ctx)
{
	size_t len;
	int ret;

	NET_DBG("HTTPS handler starting");

	mbedtls_platform_set_printf(printk);

	heap_init(ctx);

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_HTTP)
	mbedtls_debug_set_threshold(DEBUG_THRESHOLD);
	mbedtls_ssl_conf_dbg(&ctx->https.mbedtls.conf, my_debug, NULL);
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_init(&ctx->https.mbedtls.srvcert);
#endif

	mbedtls_pk_init(&ctx->https.mbedtls.pkey);
	mbedtls_ssl_init(&ctx->https.mbedtls.ssl);
	mbedtls_ssl_config_init(&ctx->https.mbedtls.conf);
	mbedtls_entropy_init(&ctx->https.mbedtls.entropy);
	mbedtls_ctr_drbg_init(&ctx->https.mbedtls.ctr_drbg);

	/* Load the certificates and private RSA key. This needs to be done
	 * by the user so we call a callback that user must have provided.
	 */
	ret = ctx->https.mbedtls.cert_cb(ctx, &ctx->https.mbedtls.srvcert,
					 &ctx->https.mbedtls.pkey);
	if (ret != 0) {
		goto exit;
	}

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
					  MBEDTLS_SSL_IS_SERVER,
					  MBEDTLS_SSL_TRANSPORT_STREAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		print_error("mbedtls_ssl_config_defaults returned -0x%x", ret);
		goto exit;
	}

	mbedtls_ssl_conf_rng(&ctx->https.mbedtls.conf,
			     mbedtls_ctr_drbg_random,
			     &ctx->https.mbedtls.ctr_drbg);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_ssl_conf_ca_chain(&ctx->https.mbedtls.conf,
				  ctx->https.mbedtls.srvcert.next,
				  NULL);

	ret = mbedtls_ssl_conf_own_cert(&ctx->https.mbedtls.conf,
					&ctx->https.mbedtls.srvcert,
					&ctx->https.mbedtls.pkey);
	if (ret != 0) {
		print_error("mbedtls_ssl_conf_own_cert returned -0x%x", ret);
		goto exit;
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	ret = mbedtls_ssl_setup(&ctx->https.mbedtls.ssl,
				&ctx->https.mbedtls.conf);
	if (ret != 0) {
		print_error("mbedtls_ssl_setup returned -0x%x", ret);
		goto exit;
	}

reset:
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
			if (ret < 0) {
				print_error("mbedtls_ssl_handshake returned "
					    "-0x%x", ret);
				goto reset;
			}
		}
	} while (ret != 0);

	/* Read the HTTPS Request */
	NET_DBG("Read HTTPS request");
	do {
		len = ctx->req.request_buf_len - 1;
		memset(ctx->req.request_buf, 0, ctx->req.request_buf_len);

		ret = mbedtls_ssl_read(&ctx->https.mbedtls.ssl,
				       ctx->req.request_buf, len);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}

		if (ret <= 0) {
			switch (ret) {
			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				NET_DBG("Connection was closed gracefully");
				goto close;

			case MBEDTLS_ERR_NET_CONN_RESET:
				NET_DBG("Connection was reset by peer");
				break;

			default:
				print_error("mbedtls_ssl_read returned "
					    "-0x%x", ret);
				break;
			}

			goto close;
		}

		ret = http_parser_execute(&ctx->req.parser,
					  &ctx->req.settings,
					  ctx->req.request_buf,
					  ret);
	} while (ret < 0);

	/* Write the Response */
	NET_DBG("Write HTTPS response");

	if (ctx->req.parser.http_errno != HPE_OK) {
		http_response_400(ctx, NULL);
	} else {
		http_process_recv(ctx);
	}

close:
	mbedtls_ssl_close_notify(&ctx->https.mbedtls.ssl);
	goto reset;

exit:
	return;
}

static void https_enable(struct http_server_ctx *ctx)
{
	/* Start the thread that handles HTTPS traffic. */
	if (ctx->https.tid) {
		return;
	}

	ctx->https.tid = k_thread_create(&ctx->https.thread,
					 ctx->https.stack,
					 ctx->https.stack_size,
					 (k_thread_entry_t)https_handler,
					 ctx, NULL, NULL,
					 K_PRIO_COOP(7), 0, 0);
}

static void https_disable(struct http_server_ctx *ctx)
{
	if (!ctx->https.tid) {
		return;
	}

	mbedtls_ssl_free(&ctx->https.mbedtls.ssl);
	mbedtls_ssl_config_free(&ctx->https.mbedtls.conf);
	mbedtls_ctr_drbg_free(&ctx->https.mbedtls.ctr_drbg);
	mbedtls_entropy_free(&ctx->https.mbedtls.entropy);

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

	NET_DBG("HTTPS thread %p stopped", ctx->https.tid);

	k_thread_abort(ctx->https.tid);
	ctx->https.tid = 0;
}

static int https_init(struct http_server_ctx *ctx)
{
	k_sem_init(&ctx->https.mbedtls.ssl_ctx.tx_sem, 0, UINT_MAX);
	k_fifo_init(&ctx->https.mbedtls.ssl_ctx.rx_fifo);

	/* Next we return to application which must then enable the HTTPS
	 * service. The enable function will then start the https thread and
	 * do what ever further configuration needed.
	 *
	 * We do the mbedtls initialization in its own thread because it uses
	 * of of stack and the main stack runs out of memory very easily.
	 *
	 * See https_handler() how the things proceed from now on.
	 */

	return 0;
}

int https_server_init(struct http_server_ctx *ctx,
		      struct http_server_urls *urls,
		      struct sockaddr *server_addr,
		      u8_t *request_buf,
		      size_t request_buf_len,
		      const char *server_banner,
		      u8_t *personalization_data,
		      size_t personalization_data_len,
		      https_server_cert_cb_t cert_cb,
		      https_entropy_src_cb_t entropy_src_cb,
		      struct k_mem_pool *pool,
		      u8_t *https_stack,
		      size_t https_stack_size)
{
	int ret;

	if (ctx->urls) {
		NET_ERR("Server context %p already initialized", ctx);
		return -EALREADY;
	}

	if (!request_buf || request_buf_len == 0) {
		NET_ERR("Request buf must be set");
		return -EINVAL;
	}

	if (!cert_cb) {
		NET_ERR("Cert callback must be set");
		return -EINVAL;
	}

	ret = init_net(ctx, server_addr, HTTPS_DEFAULT_PORT);
	if (ret < 0) {
		return ret;
	}

	if (server_banner) {
		new_server(ctx, server_banner, server_addr);
	}

	ctx->req.request_buf = request_buf;
	ctx->req.request_buf_len = request_buf_len;
	ctx->req.data_len = 0;
	ctx->urls = urls;
	ctx->is_https = true;
	ctx->https.stack = https_stack;
	ctx->https.stack_size = https_stack_size;
	ctx->https.mbedtls.cert_cb = cert_cb;
	ctx->https.pool = pool;

	if (entropy_src_cb) {
		ctx->https.mbedtls.entropy_src_cb = entropy_src_cb;
	} else {
		ctx->https.mbedtls.entropy_src_cb = entropy_source;
	}

	ctx->https.mbedtls.personalization_data = personalization_data;
	ctx->https.mbedtls.personalization_data_len = personalization_data_len;
	ctx->send_data = https_send;
	ctx->recv_cb = ssl_received;

	k_delayed_work_init(&ctx->req.timer, req_timeout);

	parser_init(ctx);

	/* Then mbedtls specific initialization */
	return https_init(ctx);
}
#endif /* CONFIG_HTTPS */
