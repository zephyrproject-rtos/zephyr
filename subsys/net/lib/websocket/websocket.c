/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_WS)
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

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/websocket.h>

#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>

#define BUF_ALLOC_TIMEOUT 100

#define HTTP_DEFAULT_PORT  80
#define HTTPS_DEFAULT_PORT 443

#if !defined(ZEPHYR_USER_AGENT)
#define ZEPHYR_USER_AGENT "Zephyr OS v"KERNEL_VERSION_STRING
#endif

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

#define HTTP_CRLF "\r\n"

/* From RFC 6455 chapter 4.2.2 */
#define WS_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#if defined(CONFIG_NET_DEBUG_WS_CONN)
/** List of ws connections */
static sys_slist_t ws_conn;

static ws_server_cb_t ctx_mon;
static void *mon_user_data;

static void ws_server_conn_add(struct ws_ctx *ctx)
{
	sys_slist_prepend(&ws_conn, &ctx->node);

	if (ctx_mon) {
		ctx_mon(ctx, mon_user_data);
	}
}

static void ws_server_conn_del(struct ws_ctx *ctx)
{
	sys_slist_find_and_remove(&ws_conn, &ctx->node);
}

void ws_server_conn_foreach(ws_server_cb_t cb, void *user_data)
{
	struct ws_ctx *ctx;

	SYS_SLIST_FOR_EACH_CONTAINER(&ws_conn, ctx, node) {
		cb(ctx, user_data);
	}
}

void ws_server_conn_monitor(ws_server_cb_t cb, void *user_data)
{
	ctx_mon = cb;
	mon_user_data = user_data;
}

#else
static inline void ws_server_conn_add(struct ws_ctx *ctx) {}
static inline void ws_server_conn_del(struct ws_ctx *ctx) {}
#endif /* CONFIG_NET_DEBUG_WS_CONN */

const char * const ws_state_str(enum ws_state state)
{
#if defined(CONFIG_NET_DEBUG_WS)
	switch (state) {
	case WS_STATE_CLOSED:
		return "CLOSED";
	case WS_STATE_WAITING_HEADER:
		return "WAITING_HEADER";
	case WS_STATE_RECEIVING_HEADER:
		return "RECEIVING HEADER";
	case WS_STATE_HEADER_RECEIVED:
		return "HEADER_RECEIVED";
	case WS_STATE_OPEN:
		return "OPEN";
	}
#else /* CONFIG_NET_DEBUG_WS */
	ARG_UNUSED(state);
#endif /* CONFIG_NET_DEBUG_WS */

	return "";
}

#if defined(CONFIG_NET_DEBUG_WS)
static void validate_state_transition(struct ws_ctx *ctx,
				      enum ws_state current,
				      enum ws_state new)
{
	static const u16_t valid_transitions[] = {
		[WS_STATE_CLOSED] = 1 << WS_STATE_WAITING_HEADER,
		[WS_STATE_WAITING_HEADER] = 1 << WS_STATE_RECEIVING_HEADER |
							1 << WS_STATE_CLOSED,
		[WS_STATE_RECEIVING_HEADER] = 1 << WS_STATE_HEADER_RECEIVED |
							1 << WS_STATE_CLOSED |
							1 << WS_STATE_OPEN,
		[WS_STATE_HEADER_RECEIVED] = 1 << WS_STATE_OPEN |
							1 << WS_STATE_CLOSED,
		[WS_STATE_OPEN] = 1 << WS_STATE_CLOSED,
	};

	if (!(valid_transitions[current] & 1 << new)) {
		NET_DBG("[%p] Invalid state transition: %s (%d) => %s (%d)",
			ctx, ws_state_str(current), current,
			ws_state_str(new), new);
	}
}
#endif /* CONFIG_NET_DEBUG_WS */

#define ws_change_state(ctx, new_state)				\
	_ws_change_state(ctx, new_state, __func__, __LINE__)

static void _ws_change_state(struct ws_ctx *ctx,
			     enum ws_state new_state,
			     const char *func, int line)
{
	if (ctx->state == new_state) {
		return;
	}

	NET_ASSERT(new_state >= WS_STATE_CLOSED &&
		   new_state <= WS_STATE_OPEN);

	NET_DBG("[%p] state %s (%d) => %s (%d) [%s():%d]",
		ctx, ws_state_str(ctx->state), ctx->state,
		ws_state_str(new_state), new_state,
		func, line);

#if defined(CONFIG_NET_DEBUG_WS)
	validate_state_transition(ctx, ctx->state, new_state);
#endif /* CONFIG_NET_DEBUG_WS */

	ctx->state = new_state;
}

static inline u16_t http_strlen(const char *str)
{
	if (str) {
		return strlen(str);
	}

	return 0;
}

static int _ws_add_header(struct net_pkt *pkt, s32_t timeout, const char *str)
{
	if (net_pkt_append_all(pkt, strlen(str), (u8_t *)str, timeout)) {
		return 0;
	}

	return -ENOMEM;
}

static int _ws_add_chunk(struct net_pkt *pkt, s32_t timeout,
			 const u8_t *buf, size_t len)
{
	char chunk_header[16];
	bool ret;

	if (!buf) {
		len = 0;
	}

	snprintk(chunk_header, sizeof(chunk_header), "%x" HTTP_CRLF,
		 (unsigned int)len);

	ret = net_pkt_append_all(pkt, strlen(chunk_header), chunk_header,
				 timeout);
	if (!ret) {
		return -ENOMEM;
	}

	if (len) {
		ret = net_pkt_append_all(pkt, len, buf, timeout);
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

static void ws_data_sent(struct net_app_ctx *app_ctx,
			 int status,
			 void *user_data_send,
			 void *user_data)
{
	struct ws_ctx *ctx = user_data;

	if (!user_data_send) {
		/* This is the token field in the net_context_send().
		 * If this is not set, then it is probably TCP ACK messages
		 * that are generated by the stack. We just ignore those.
		 */
		return;
	}

	if (ctx->state == WS_STATE_OPEN && ctx->cb.send) {
		ctx->cb.send(ctx, status, user_data_send, ctx->user_data);
	}
}

int ws_send_http_error(struct ws_ctx *ctx, int code, u8_t *html_payload,
		       size_t html_len)
{
	struct net_pkt *pkt = NULL;
	const char *msg;
	int ret;

	switch (code) {
	case 400:
		msg = HTTP_STATUS_400_BR;
		break;
	}

	ret = ws_add_http_header(ctx, &pkt, msg);
	if (!ret) {
		return ret;
	}

	if (html_payload) {
		ret = ws_add_http_data(ctx, &pkt, html_payload, html_len);
		if (!ret) {
			net_pkt_unref(pkt);
			return ret;
		}
	}

	return ws_send_data_http(ctx, pkt, NULL);
}

static void mask_pkt(struct net_pkt *pkt, u32_t masking_value)
{
	struct net_buf *frag;
	int i, count = 0;
	u16_t pos;

	frag = net_frag_get_pos(pkt,
				net_pkt_get_len(pkt) - net_pkt_appdatalen(pkt),
				&pos);
	if (!frag) {
		return;
	}

	NET_ASSERT(net_pkt_appdata(pkt) == frag->data + pos);

	while (frag) {
		for (i = pos; i < frag->len && count < net_pkt_appdatalen(pkt);
		     i++, count++) {
			frag->data[i] ^= masking_value >> (8 * (3 - count % 4));
		}

		pos = 0;
		frag = frag->frags;
	}
}

int ws_send_data_raw(struct ws_ctx *ctx, struct net_pkt *pkt,
		     void *user_send_data)
{
	int ret;

	NET_DBG("[%p] Sending %zd bytes data", ctx, net_pkt_get_len(pkt));

	ret = net_app_send_pkt(&ctx->app_ctx, pkt, NULL, 0, 0,
			       user_send_data);
	if (!ret) {
		/* We must let the system to send the packet, otherwise TCP
		 * might timeout before the packet is actually sent. This is
		 * easily seen if the application calls this functions many
		 * times in a row.
		 */
		k_yield();
	}

	return ret;
}

int ws_send_data(struct ws_ctx *ctx, struct net_pkt *pkt,
		 enum ws_opcode opcode, bool mask, bool final,
		 void *user_send_data)
{
	u8_t header[10], hdr_len = 2;
	int len, ret;
	bool alloc_here = false;

	if (ctx->state != WS_STATE_OPEN) {
		return -ENOTCONN;
	}

	if (opcode != WS_OPCODE_DATA_TEXT && opcode != WS_OPCODE_DATA_BINARY &&
	    opcode != WS_OPCODE_CONTINUE && opcode != WS_OPCODE_CLOSE &&
	    opcode != WS_OPCODE_PING && opcode != WS_OPCODE_PONG) {
		return -EINVAL;
	}

	if (!pkt) {
		pkt = net_app_get_net_pkt(&ctx->app_ctx,
					  AF_UNSPEC,
					  ctx->timeout);
		if (!pkt) {
			return -ENOMEM;
		}

		if (!net_app_get_net_buf(&ctx->app_ctx, pkt,
					 ctx->timeout)) {
			net_pkt_unref(pkt);
			return -ENOMEM;
		}

		alloc_here = true;
	}

	/* If there is IP + other headers in front, then get rid of them
	 * here.
	 */
	len = net_pkt_get_len(pkt);
	if (len > net_pkt_appdatalen(pkt) && net_pkt_appdatalen(pkt) > 0) {
		/* Make sure that appdata pointer is valid one */
		if ((net_pkt_appdata(pkt) < pkt->frags->data) ||
		    (net_pkt_appdata(pkt) > (pkt->frags->data +
					     pkt->frags->len))) {
			NET_DBG("appdata %p (%d len) is not [%p, %p], "
				"msg (%d bytes) discarded",
				net_pkt_appdata(pkt), net_pkt_appdatalen(pkt),
				pkt->frags->data, pkt->frags->data +
				pkt->frags->len, len);
			ret = -EINVAL;
			goto quit;
		}

		NET_DBG("Stripping %zd bytes from pkt %p",
			net_pkt_appdata(pkt) - pkt->frags->data, pkt);

		net_buf_pull(pkt->frags,
			     net_pkt_appdata(pkt) - pkt->frags->data);
	} else {
		net_pkt_set_appdatalen(pkt, len);
		net_pkt_set_appdata(pkt, pkt->frags->data);
	}

	len = net_pkt_appdatalen(pkt);

	memset(header, 0, sizeof(header));

	/* Is this the last packet? */
	header[0] = final ? BIT(7) : 0;

	/* Text, binary, ping, pong or close ? */
	header[0] |= opcode;

	/* Masking */
	header[1] = mask ? BIT(7) : 0;

	if (len < 126) {
		header[1] |= len;
	} else if (len == 126) {
		header[1] |= 126;
		header[2] |= htons(len) >> 8;
		header[3] |= htons(len);
		hdr_len += 2;
	} else {
		header[1] |= 127;
		header[2] |= htonl(len) >> 24;
		header[3] |= htonl(len) >> 16;
		header[4] |= htonl(len) >> 8;
		header[5] |= htonl(len);
		hdr_len += 4;
	}

	/* Add masking value if needed */
	if (mask) {
		u32_t masking_value;

		masking_value = sys_rand32_get();

		header[hdr_len++] |= masking_value >> 24;
		header[hdr_len++] |= masking_value >> 16;
		header[hdr_len++] |= masking_value >> 8;
		header[hdr_len++] |= masking_value;

		mask_pkt(pkt, masking_value);
	}

	if (!net_pkt_insert(pkt, pkt->frags, 0, hdr_len, header,
			    ctx->timeout)) {
		ret = -ENOMEM;
		goto quit;
	}

	net_pkt_set_appdatalen(pkt, net_pkt_appdatalen(pkt) + hdr_len);

	ret = ws_send_data_raw(ctx, pkt, user_send_data);
	if (ret < 0) {
		NET_DBG("Cannot send %zd bytes message (%d)",
			net_pkt_get_len(pkt), ret);
		goto quit;
	}

quit:
	if (alloc_here) {
		net_pkt_unref(pkt);
	}

	return ret;
}

int ws_add_http_header(struct ws_ctx *ctx, struct net_pkt **pkt,
		       const char *http_header)
{
	if (!pkt) {
		return -EINVAL;
	}

	if (!*pkt) {
		*pkt = net_app_get_net_pkt(&ctx->app_ctx, AF_UNSPEC,
					   ctx->timeout);
		if (!*pkt) {
			return -ENOMEM;
		}
	}

	return _ws_add_header(*pkt, ctx->timeout, http_header);
}

int ws_add_http_data(struct ws_ctx *ctx, struct net_pkt **pkt,
		     const u8_t *buf, size_t len)
{
	if (!pkt) {
		return -EINVAL;
	}

	if (!*pkt) {
		*pkt = net_app_get_net_pkt(&ctx->app_ctx, AF_UNSPEC,
					   ctx->timeout);
		if (!*pkt) {
			return -ENOMEM;
		}
	}

	return _ws_add_chunk(*pkt, ctx->timeout, buf, len);
}

#if defined(CONFIG_NET_DEBUG_WS) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
static char *sprint_ipaddr(char *buf, int buflen, const struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		char ipaddr[NET_IPV6_ADDR_LEN];

		net_addr_ntop(addr->sa_family,
			      &net_sin6(addr)->sin6_addr,
			      ipaddr, sizeof(ipaddr));
		snprintk(buf, buflen, "[%s]:%u", ipaddr,
			 ntohs(net_sin6(addr)->sin6_port));
#endif
	} else if (addr->sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		char ipaddr[NET_IPV4_ADDR_LEN];

		net_addr_ntop(addr->sa_family,
			      &net_sin(addr)->sin_addr,
			      ipaddr, sizeof(ipaddr));
		snprintk(buf, buflen, "%s:%u", ipaddr,
			 ntohs(net_sin(addr)->sin_port));
#endif
	}

	return buf;
}
#endif /* CONFIG_NET_DEBUG_HTTP */

static inline void new_client(struct ws_ctx *ctx,
			      enum ws_connection_type type,
			      struct net_app_ctx *app_ctx)
{
#if defined(CONFIG_NET_DEBUG_WS) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
#if defined(CONFIG_NET_IPV6)
#define PORT_LEN sizeof("[]:xxxxx")
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4)
#define PORT_LEN sizeof(":xxxxx")
#define ADDR_LEN NET_IPV4_ADDR_LEN
#endif
	char buf[ADDR_LEN + PORT_LEN];
	const char *type_str;

	if (type == WS_CONNECT) {
		type_str = "WS";
	} else {
		type_str = "HTTP";
	}

	NET_INFO("[%p] %s connection from %s (%p)", ctx, type_str,
		 sprint_ipaddr(buf, sizeof(buf),
			       &app_ctx->server.net_ctx->remote),
		 app_ctx->server.net_ctx);
#endif /* CONFIG_NET_DEBUG_HTTP */
}

static void url_connected(struct ws_ctx *ctx,
			  enum ws_connection_type type)
{
	new_client(ctx, type, &ctx->app_ctx);

	if (ctx->cb.connect) {
		ctx->cb.connect(ctx, type, ctx->user_data);
	}
}

struct ws_root_url *ws_server_add_url(struct ws_server_urls *my,
				      const char *url, u8_t flags)
{
	int i;

	for (i = 0; i < CONFIG_WS_HTTP_SERVER_NUM_URLS; i++) {
		if (my->urls[i].is_used) {
			continue;
		}

		my->urls[i].is_used = true;
		my->urls[i].root = url;

		/* This will speed-up some future operations */
		my->urls[i].root_len = strlen(url);
		my->urls[i].flags = flags;

		NET_DBG("[%d] %s URL %s", i,
			flags == HTTP_URL_WS ? "WS" : "HTTP", url);

		return &my->urls[i];
	}

	return NULL;
}

int ws_server_del_url(struct ws_server_urls *my, const char *url)
{
	int i;

	for (i = 0; i < CONFIG_WS_HTTP_SERVER_NUM_URLS; i++) {
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

struct ws_root_url *ws_server_add_default(struct ws_server_urls *my,
					  ws_url_cb_t cb)
{
	if (my->default_url.is_used) {
		return NULL;
	}

	my->default_url.is_used = true;
	my->default_url.root = NULL;
	my->default_url.root_len = 0;
	my->default_url.flags = 0;
	my->default_cb = cb;

	return &my->default_url;
}

int ws_server_del_default(struct ws_server_urls *my)
{
	if (!my->default_url.is_used) {
		return -ENOENT;
	}

	my->default_url.is_used = false;

	return 0;
}

static int ws_url_cmp(const char *url, u16_t url_len,
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

static struct ws_root_url *ws_url_find(struct ws_ctx *ctx,
				       enum ws_url_flags flags)
{
	u16_t url_len = ctx->http.url_len;
	const char *url = ctx->http.url;
	struct ws_root_url *root_url;
	u8_t i;
	int ret;

	for (i = 0; i < CONFIG_WS_HTTP_SERVER_NUM_URLS; i++) {
		if (!ctx->http.urls) {
			continue;
		}

		root_url = &ctx->http.urls->urls[i];
		if (!root_url || !root_url->is_used) {
			continue;
		}

		ret = ws_url_cmp(url, url_len,
				 root_url->root, root_url->root_len);
		if (!ret && flags == root_url->flags) {
			return root_url;
		}
	}

	return NULL;
}

static int ws_process_recv(struct ws_ctx *ctx)
{
	struct ws_root_url *root_url;
	int ret;

	root_url = ws_url_find(ctx, HTTP_URL_STANDARD);
	if (!root_url) {
		if (!ctx->http.urls) {
			NET_DBG("[%p] No URL handlers found", ctx);
			ret = -ENOENT;
			goto out;
		}

		root_url = &ctx->http.urls->default_url;
		if (!root_url || !root_url->is_used) {
			NET_DBG("[%p] No default handler found", ctx);
			ret = -ENOENT;
			goto out;
		}

		if (ctx->http.urls->default_cb) {
			ret = ctx->http.urls->default_cb(ctx,
							 WS_HTTP_CONNECT);
			if (ret != WS_VERDICT_ACCEPT) {
				ret = -ECONNREFUSED;
				goto out;
			}
		}
	}

	ws_change_state(ctx, WS_STATE_OPEN);
	url_connected(ctx, WS_HTTP_CONNECT);

	ret = 0;

out:
	return ret;
}

static void ws_closed(struct net_app_ctx *app_ctx,
		      int status,
		      void *user_data)
{
	struct ws_ctx *ctx = user_data;

	ARG_UNUSED(app_ctx);
	ARG_UNUSED(status);

	ws_change_state(ctx, WS_STATE_CLOSED);

	NET_DBG("[%p] ws closed", ctx);

	ws_server_conn_del(ctx);

	if (ctx->cb.close) {
		ctx->cb.close(ctx, 0, ctx->user_data);
	}
}

static u32_t ws_strip_header(struct net_pkt *pkt, bool *masked,
			     u32_t *mask_value)
{
	struct net_buf *frag;
	u32_t flag = 0;
	u16_t value;
	u16_t pos;
	u8_t len, skip;

	/* We can get the ws header like this because it is in first
	 * fragment
	 */
	frag = net_frag_read_be16(pkt->frags,
				  net_pkt_appdata(pkt) - pkt->frags->data,
				  &pos,
				  &value);
	if (!frag && pos == 0xffff) {
		return 0;
	}

	if (value & 0x8000) {
		flag |= WS_FLAG_FINAL;
	}

	switch (value & 0x0f00) {
	case 0x0100:
		flag |= WS_FLAG_TEXT;
		break;
	case 0x0200:
		flag |= WS_FLAG_BINARY;
		break;
	case 0x0800:
		flag |= WS_FLAG_CLOSE;
		break;
	case 0x0900:
		flag |= WS_FLAG_PING;
		break;
	case 0x0A00:
		flag |= WS_FLAG_PONG;
		break;
	}

	if (value & 0x0080) {
		*masked = true;

		frag = net_frag_read_be32(pkt->frags, pos, &pos, mask_value);
		if (!frag && pos == 0xffff) {
			return 0;
		}
	} else {
		*masked = false;
	}

	len = value & 0x007f;
	if (len < 126) {
		skip = 0;
	} else if (len == 126) {
		skip = 1;
	} else {
		skip = 2;
	}

	frag = net_frag_get_pos(pkt, pos + skip, &pos);
	if (!frag && pos == 0xffff) {
		return 0;
	}

	net_pkt_set_appdatalen(pkt, net_pkt_appdatalen(pkt) - 6 - skip);
	net_pkt_set_appdata(pkt, frag->data + pos);

	return flag;
}

static void ws_received(struct net_app_ctx *app_ctx,
			struct net_pkt *pkt,
			int status,
			void *user_data)
{
	struct ws_ctx *ctx = user_data;
	size_t start = ctx->http.data_len;
	u16_t len = 0;
	struct net_buf *frag;
	int parsed_len;
	size_t recv_len;
	size_t pkt_len;

	recv_len = net_pkt_appdatalen(pkt);
	if (recv_len == 0) {
		/* don't print info about zero-length app data buffers */
		goto quit;
	}

	if (status) {
		NET_DBG("[%p] Status %d <%s>", ctx, status, RC_STR(status));
		goto out;
	}

	/* Get rid of possible IP headers in the first fragment. */
	frag = pkt->frags;

	pkt_len = net_pkt_get_len(pkt);

	if (recv_len < pkt_len) {
		net_buf_pull(frag, pkt_len - recv_len);
		net_pkt_set_appdata(pkt, frag->data);
	}

	NET_DBG("[%p] Received %zd bytes ws data", ctx, recv_len);

	if (ctx->state == WS_STATE_OPEN) {
		/* We have active websocket session and there is no longer
		 * any HTTP traffic in the connection. Give the data to
		 * application to send.
		 */
		goto ws_only;
	}

	while (frag) {
		/* If this fragment cannot be copied to result buf,
		 * then parse what we have which will cause the callback to be
		 * called in function on_body(), and continue copying.
		 */
		if ((ctx->http.data_len + frag->len) >
		    ctx->request_buf_len) {

			if (ctx->state == WS_STATE_HEADER_RECEIVED) {
				goto ws_ready;
			}

			/* If the caller has not supplied a callback, then
			 * we cannot really continue if the request buffer
			 * overflows. Set the data_len to mark how many bytes
			 * should be needed in the response_buf.
			 */
			if (ws_process_recv(ctx) < 0) {
				ctx->http.data_len = recv_len;
				goto out;
			}

			parsed_len =
				http_parser_execute(&ctx->http.parser,
						    &ctx->http.parser_settings,
						    ctx->request_buf +
						    start,
						    len);
			if (parsed_len <= 0) {
				goto fail;
			}

			ctx->http.data_len = 0;
			len = 0;
			start = 0;
		}

		memcpy(ctx->request_buf + ctx->http.data_len,
		       frag->data, frag->len);

		ctx->http.data_len += frag->len;
		len += frag->len;
		frag = frag->frags;
	}

out:
	parsed_len = http_parser_execute(&ctx->http.parser,
					 &ctx->http.parser_settings,
					 ctx->request_buf + start,
					 len);
	if (parsed_len < 0) {
fail:
		NET_DBG("[%p] Received %zd bytes, only parsed %d bytes (%s %s)",
			ctx, recv_len, parsed_len,
			http_errno_name(ctx->http.parser.http_errno),
			http_errno_description(
				ctx->http.parser.http_errno));
	}

	if (ctx->http.parser.http_errno != HPE_OK) {
		ws_send_http_error(ctx, 400, NULL, 0);
	} else {
		if (ctx->state == WS_STATE_HEADER_RECEIVED) {
			goto ws_ready;
		}

		ws_process_recv(ctx);
	}

quit:
	http_parser_init(&ctx->http.parser, HTTP_REQUEST);
	ctx->http.data_len = 0;
	ctx->http.field_values_ctr = 0;
	net_pkt_unref(pkt);

	return;

ws_only:
	if (ctx->cb.recv) {
		u32_t flags, mask_value = 0;
		bool masked = false;

		flags = ws_strip_header(pkt, &masked, &mask_value);

		if (flags & WS_FLAG_CLOSE) {
			NET_DBG("[%p] Close request from peer", ctx);
			ws_close(ctx);
			return;
		}

		if (flags & WS_FLAG_PING) {
			NET_DBG("[%p] Ping request from peer", ctx);
			ws_send_data(ctx, NULL, WS_OPCODE_PONG, false, true,
				     NULL);
			return;
		}

		NET_DBG("Forward the data to application for processing.");

		NET_DBG("Masked %s mask 0x%04x", masked ? "yes" : "no",
			mask_value);

		if (masked) {
			/* Always deliver unmasked data to the application */
			mask_pkt(pkt, mask_value);
		}

		ctx->cb.recv(ctx, pkt, 0, flags, ctx->user_data);
	}

	return;

ws_ready:
	ws_change_state(ctx, WS_STATE_OPEN);
	url_connected(ctx, WS_CONNECT);
	net_pkt_unref(pkt);
}

#if defined(CONFIG_WS_TLS)
int ws_server_set_tls(struct ws_ctx *ctx,
		      const char *server_banner,
		      u8_t *personalization_data,
		      size_t personalization_data_len,
		      net_app_cert_cb_t cert_cb,
		      net_app_entropy_src_cb_t entropy_src_cb,
		      struct k_mem_pool *pool,
		      k_thread_stack_t stack,
		      size_t stack_len)
{
	int ret;

	if (!ctx->is_tls) {
		/* Change the default port if user did not set it */
		if (!ctx->server_addr) {
			net_sin(&ctx->local)->sin_port =
				htons(HTTPS_DEFAULT_PORT);

#if defined(CONFIG_NET_IPV6)
			net_sin6(&ctx->app_ctx.ipv6.local)->sin6_port =
				htons(HTTPS_DEFAULT_PORT);
#endif
#if defined(CONFIG_NET_IPV4)
			net_sin(&ctx->app_ctx.ipv4.local)->sin_port =
				htons(HTTPS_DEFAULT_PORT);
#endif
		}

		ret = net_app_server_tls(&ctx->app_ctx,
					 ctx->request_buf,
					 ctx->request_buf_len,
					 server_banner,
					 personalization_data,
					 personalization_data_len,
					 cert_cb,
					 entropy_src_cb,
					 pool,
					 stack,
					 stack_len);
		if (ret < 0) {
			NET_ERR("Cannot init TLS (%d)", ret);
			goto quit;
		}

		ctx->is_tls = true;
		return 0;
	}

	return -EALREADY;

quit:
	net_app_release(&ctx->app_ctx);
	return ret;
}
#endif

static int on_header_field(struct http_parser *parser,
			   const char *at, size_t length)
{
	struct ws_ctx *ctx = parser->data;

	if (ctx->http.field_values_ctr >=
				CONFIG_WS_HTTP_HEADER_FIELD_ITEMS) {
		return 0;
	}

	ws_change_state(ctx, WS_STATE_RECEIVING_HEADER);

	ctx->http.field_values[ctx->http.field_values_ctr].key = at;
	ctx->http.field_values[ctx->http.field_values_ctr].key_len = length;

	return 0;
}

static int on_header_value(struct http_parser *parser,
			   const char *at, size_t length)
{
	struct ws_ctx *ctx = parser->data;

	if (ctx->http.field_values_ctr >=
				CONFIG_WS_HTTP_HEADER_FIELD_ITEMS) {
		return 0;
	}

	ctx->http.field_values[ctx->http.field_values_ctr].value = at;
	ctx->http.field_values[ctx->http.field_values_ctr].value_len = length;

	ctx->http.field_values_ctr++;

	return 0;
}

static int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct ws_ctx *ctx = parser->data;

	ctx->http.url = at;
	ctx->http.url_len = length;

	ws_change_state(ctx, WS_STATE_WAITING_HEADER);

	ws_server_conn_add(ctx);

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

static bool check_ws_headers(struct ws_ctx *ctx, struct http_parser *parser,
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

static struct net_pkt *prepare_reply(struct ws_ctx *ctx,
				     int ws_sec_key, int host, int subprotocol)
{
	char key_accept[32 + sizeof(WS_MAGIC) - 1];
	char accept[20];
	struct net_pkt *pkt;
	char tmp[64];
	int ret;
	size_t olen;

	pkt = net_app_get_net_pkt(&ctx->app_ctx, AF_UNSPEC, ctx->timeout);
	if (!pkt) {
		return NULL;
	}

	snprintk(tmp, sizeof(tmp), "HTTP/1.1 101 OK\r\n");
	if (!net_pkt_append_all(pkt, strlen(tmp), (u8_t *)tmp, ctx->timeout)) {
		goto fail;
	}

	snprintk(tmp, sizeof(tmp), "User-Agent: %s\r\n", ZEPHYR_USER_AGENT);
	if (!net_pkt_append_all(pkt, strlen(tmp), (u8_t *)tmp, ctx->timeout)) {
		goto fail;
	}

	snprintk(tmp, sizeof(tmp), "Upgrade: websocket\r\n");
	if (!net_pkt_append_all(pkt, strlen(tmp), (u8_t *)tmp, ctx->timeout)) {
		goto fail;
	}

	snprintk(tmp, sizeof(tmp), "Connection: Upgrade\r\n");
	if (!net_pkt_append_all(pkt, strlen(tmp), (u8_t *)tmp, ctx->timeout)) {
		goto fail;
	}

	olen = min(sizeof(key_accept),
		   ctx->http.field_values[ws_sec_key].value_len);
	strncpy(key_accept, ctx->http.field_values[ws_sec_key].value, olen);

	olen = min(sizeof(key_accept) -
		   ctx->http.field_values[ws_sec_key].value_len,
		   sizeof(WS_MAGIC) - 1);
	strncpy(key_accept + ctx->http.field_values[ws_sec_key].value_len,
		WS_MAGIC, olen);

	olen = ctx->http.field_values[ws_sec_key].value_len +
		sizeof(WS_MAGIC) - 1;

	mbedtls_sha1(key_accept, olen, accept);

	snprintk(tmp, sizeof(tmp), "Sec-WebSocket-Accept: ");

	ret = mbedtls_base64_encode(tmp + sizeof("Sec-WebSocket-Accept: ") - 1,
				    sizeof(tmp) -
				    (sizeof("Sec-WebSocket-Accept: ") - 1),
				    &olen, accept, sizeof(accept));
	if (ret) {
		if (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
			NET_DBG("[%p] Too short buffer olen %zd", ctx, olen);
		}

		goto fail;
	}

	snprintk(tmp + sizeof("Sec-WebSocket-Accept: ") - 1 + olen,
		 sizeof(tmp) - (sizeof("Sec-WebSocket-Accept: ") - 1) - olen,
		 "\r\n\r\n");

	if (!net_pkt_append_all(pkt, strlen(tmp), (u8_t *)tmp, ctx->timeout)) {
		goto fail;
	}

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

static int on_headers_complete(struct http_parser *parser)
{
	struct ws_ctx *ctx = parser->data;
	int ws_sec_key = -1, host = -1, subprotocol = -1;

	if (check_ws_headers(ctx, parser, &ws_sec_key, &host,
			     &subprotocol)) {
		struct net_pkt *pkt;
		struct ws_root_url *url;
		int ret;

		url = ws_url_find(ctx, HTTP_URL_WS);
		if (!url) {
			url = ws_url_find(ctx, HTTP_URL_STANDARD);
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
								 WS_CONNECT);
				if (ret == WS_VERDICT_ACCEPT) {
					goto accept;
				}
			}

			if (url->flags == HTTP_URL_WS) {
				goto fail;
			}
		}

		if (url->flags != HTTP_URL_WS) {
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

		ws_change_state(ctx, WS_STATE_HEADER_RECEIVED);

		/* We do not expect any HTTP data after this */
		return 2;

	fail:
		ws_change_state(ctx, WS_STATE_CLOSED);
	}

	return 0;
}

static int init_http_parser(struct ws_ctx *ctx)
{
	memset(ctx->http.field_values, 0, sizeof(ctx->http.field_values));

	ctx->http.parser_settings.on_header_field = on_header_field;
	ctx->http.parser_settings.on_header_value = on_header_value;
	ctx->http.parser_settings.on_url = on_url;
	ctx->http.parser_settings.on_headers_complete = on_headers_complete;

	http_parser_init(&ctx->http.parser, HTTP_REQUEST);

	ctx->http.parser.data = ctx;

	return 0;
}

static inline void new_server(struct ws_ctx *ctx,
			      const char *server_banner,
			      const struct sockaddr *addr)
{
#if defined(CONFIG_NET_DEBUG_WS) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
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

static void init_net(struct ws_ctx *ctx,
		     struct sockaddr *server_addr,
		     u16_t port)
{
	memset(&ctx->local, 0, sizeof(ctx->local));

	if (server_addr) {
		memcpy(&ctx->local, server_addr, sizeof(ctx->local));
	} else {
		ctx->local.sa_family = AF_UNSPEC;
		net_sin(&ctx->local)->sin_port = htons(port);
	}
}

int ws_server_init(struct ws_ctx *ctx,
		   struct ws_server_urls *urls,
		   struct sockaddr *server_addr,
		   u8_t *request_buf,
		   size_t request_buf_len,
		   const char *server_banner,
		   void *user_data)
{
	int ret = 0;

	if (!ctx) {
		return -EINVAL;
	}

	if (ctx->is_init) {
		return -EALREADY;
	}

	if (!request_buf || request_buf_len == 0) {
		NET_ERR("Request buf must be set");
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(*ctx));

	init_net(ctx, server_addr, HTTP_DEFAULT_PORT);

	if (server_banner) {
		new_server(ctx, server_banner, server_addr);
	}

	/* Timeout for network buffer allocations */
	ctx->timeout = BUF_ALLOC_TIMEOUT;

	ctx->request_buf = request_buf;
	ctx->request_buf_len = request_buf_len;
	ctx->http.urls = urls;
	ctx->http.data_len = 0;
	ctx->user_data = user_data;
	ctx->server_addr = server_addr;

	ret = net_app_init_tcp_server(&ctx->app_ctx,
				      &ctx->local,
				      HTTP_DEFAULT_PORT,
				      ctx);
	if (ret < 0) {
		NET_ERR("Cannot create ws server (%d)", ret);
		return ret;
	}

	ret = net_app_set_cb(&ctx->app_ctx, NULL, ws_received,
			     ws_data_sent, ws_closed);
	if (ret < 0) {
		NET_ERR("Cannot set callbacks (%d)", ret);
		goto quit;
	}

	init_http_parser(ctx);

	ctx->is_init = true;
	return 0;

quit:
	net_app_release(&ctx->app_ctx);
	return ret;
}

int ws_close(struct ws_ctx *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	ws_server_conn_del(ctx);

	return net_app_close(&ctx->app_ctx);
}

int ws_release(struct ws_ctx *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	ws_server_conn_del(ctx);

	ctx->is_tls = false;

	ws_server_disable(ctx);

	return net_app_release(&ctx->app_ctx);
}

int ws_server_enable(struct ws_ctx *ctx)
{
	int ret;

	NET_ASSERT(ctx);

	ret = net_app_listen(&ctx->app_ctx);
	if (ret < 0) {
		NET_ERR("Cannot wait connection (%d)", ret);
		return false;
	}

	net_app_server_enable(&ctx->app_ctx);

	return 0;
}

int ws_server_disable(struct ws_ctx *ctx)
{
	NET_ASSERT(ctx);

	net_app_server_disable(&ctx->app_ctx);

	return 0;
}

int ws_set_cb(struct ws_ctx *ctx,
	      ws_connect_cb_t connect_cb,
	      ws_recv_cb_t recv_cb,
	      ws_send_cb_t send_cb,
	      ws_close_cb_t close_cb)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	ctx->cb.connect = connect_cb;
	ctx->cb.recv = recv_cb;
	ctx->cb.send = send_cb;
	ctx->cb.close = close_cb;

	return 0;
}
