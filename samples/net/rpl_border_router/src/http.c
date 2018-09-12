/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "rpl-br/http"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>

/* For Basic auth, we need base64 decoder which can be found
 * in mbedtls library.
 */
#include <mbedtls/base64.h>

#if defined(CONFIG_MBEDTLS)
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif
#endif

#include <net/net_context.h>
#include <net/websocket.h>
#include <net/net_mgmt.h>
#include <net/net_event.h>
#include <json.h>

#include "ipv6.h"
#include "route.h"
#include "rpl.h"
#include "net_private.h"

#include "config.h"

#define HTTP_CRLF "\r\n"
#define MAX_BUF_LEN 128
#define ALLOC_TIMEOUT 100

struct user_data {
	char buf[MAX_BUF_LEN];
	struct http_ctx *ctx;
	struct net_if *iface;
	const struct sockaddr *dst;
	int msg_count;
	int iface_count;
	int nbr_count;
	int route_count;
	int failure;
};

static struct {
	struct sockaddr auth_addr;
	bool auth_ok;
} rpl;

static const struct sockaddr *ws_dst;

static int http_serve_index_html(struct http_ctx *ctx,
				 const struct sockaddr *dst);

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(http_srv_tx, 64);
NET_PKT_DATA_POOL_DEFINE(http_srv_data, 64);

static struct k_mem_slab *tx_slab(void)
{
	return &http_srv_tx;
}

static struct net_buf_pool *data_pool(void)
{
	return &http_srv_data;
}
#else
#if defined(CONFIG_NET_L2_BLUETOOTH)
#error "TCP connections over Bluetooth need CONFIG_NET_CONTEXT_NET_PKT_POOL "\
	"defined."
#endif /* CONFIG_NET_L2_BLUETOOTH */

#define tx_slab NULL
#define data_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

/* Note that this should fit largest compressed file (br.js) */
#define RESULT_BUF_SIZE 2500
static u8_t result[RESULT_BUF_SIZE];

#if defined(CONFIG_HTTPS)
#if !defined(CONFIG_NET_APP_TLS_STACK_SIZE)
#define CONFIG_NET_APP_TLS_STACK_SIZE		8196
#endif /* CONFIG_NET_APP_TLS_STACK_SIZE */

#define APP_BANNER "Zephyr HTTP Server for border router"
#define INSTANCE_INFO "Zephyr border router example #1"

/* Note that each HTTPS context needs its own stack as there will be
 * a separate thread for each HTTPS context.
 */
NET_STACK_DEFINE(HTTPS, https_stack, CONFIG_NET_APP_TLS_STACK_SIZE,
		 CONFIG_NET_APP_TLS_STACK_SIZE);

#define RX_FIFO_DEPTH 4
K_MEM_POOL_DEFINE(ssl_rx_pool, 4, 64, RX_FIFO_DEPTH, 4);

#else /* CONFIG_HTTPS */
#define APP_BANNER "Zephyr HTTP server for border router"
#endif /* CONFIG_HTTPS */

static K_SEM_DEFINE(ws_reply, 0, UINT_MAX);

/*
 * Note that the http_ctx and http_server_urls are quite large so be
 * careful if those are allocated from stack.
 */
static struct http_ctx http_ctx;
static struct http_server_urls http_urls;

#define HTTP_STATUS_200_OK	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n"

#define HTTP_STATUS_200_OK_GZ	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"Content-Encoding: gzip\r\n"

#define HTTP_STATUS_200_OK_GZ_CSS \
				"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/css\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"Content-Encoding: gzip\r\n"

#define HTTP_STATUS_301_RE	"HTTP/1.1 301 Redirect\r\n" \
				"Content-Type: text/html\r\n" \
				"Location: /index.html\r\n" \
				"\r\n"

#define HTTP_401_STATUS_US	"HTTP/1.1 401 Unauthorized status\r\n" \
				"WWW-Authenticate: Basic realm=" \
				"\""HTTP_AUTH_REALM"\"\r\n\r\n"

#define HTML_HEADER		"<html><head>" \
				"<title>Zephyr RPL Border Router</title>" \
				"</head><body><h1>" \
				"<center>Zephyr RPL Border Router</center>" \
				"</h1>"

#define HTML_FOOTER		"</body></html>\r\n"

#if defined(CONFIG_HTTPS)
static bool check_file_size(const char *file, size_t size)
{
	if (size > MBEDTLS_SSL_MAX_CONTENT_LEN) {
		NET_ERR("The MBEDTLS_SSL_MAX_CONTENT_LEN (%d) is too small.",
			MBEDTLS_SSL_MAX_CONTENT_LEN);
		NET_ERR("Cannot send %s (len %zd)", file, size);

		return false;
	}

	if (size > RESULT_BUF_SIZE) {
		NET_ERR("The RESULT_BUF_SIZE (%d) is too small.",
			RESULT_BUF_SIZE);
		NET_ERR("Cannot send %s (len %zd)", file, size);

		return false;
	}

	return true;
}

/* Load the certificates and private RSA key. */

static const char echo_apps_cert_der[] = {
#include "echo-apps-cert.der.inc"
};

static const char echo_apps_key_der[] = {
#include "echo-apps-key.der.inc"
};

static int setup_cert(struct net_app_ctx *ctx,
		      mbedtls_x509_crt *cert,
		      mbedtls_pk_context *pkey)
{
	int ret;

	ret = mbedtls_x509_crt_parse(cert, echo_apps_cert_der,
				     sizeof(echo_apps_cert_der));
	if (ret != 0) {
		NET_ERR("mbedtls_x509_crt_parse returned %d", ret);
		return ret;
	}

	ret = mbedtls_pk_parse_key(pkey, echo_apps_key_der,
				   sizeof(echo_apps_key_der),
				   NULL, 0);
	if (ret != 0) {
		NET_ERR("mbedtls_pk_parse_key returned %d", ret);
		return ret;
	}

	return 0;
}
#else /* CONFIG_HTTPS */
static bool check_file_size(const char *file, size_t size)
{
	return true;
}
#endif /* CONFIG_HTTPS */

static int http_response(struct http_ctx *ctx, const char *header,
			 const char *payload, size_t payload_len,
			 const struct sockaddr *dst)
{
	int ret;

	ret = http_add_header(ctx, header, dst, NULL);
	if (ret < 0) {
		NET_ERR("Cannot add HTTP header (%d)", ret);
		return ret;
	}

	ret = http_add_header(ctx, HTTP_CRLF, dst, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = http_send_chunk(ctx, payload, payload_len, dst, NULL);
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		return ret;
	}

	return http_send_flush(ctx, NULL);
}

static int http_response_soft_404(struct http_ctx *ctx,
				  const struct sockaddr *dst)
{
	static const char *not_found =
		HTML_HEADER
		"<h2><center>404 Not Found</center></h2>"
		HTML_FOOTER;

	return http_response(ctx, HTTP_STATUS_200_OK, not_found,
			     strlen(not_found), dst);
}

int http_response_401(struct http_ctx *ctx,
		      const struct sockaddr *dst)
{
	return http_response(ctx, HTTP_401_STATUS_US, "", 0, dst);
}

static int http_basic_auth(struct http_ctx *ctx,
			   enum http_connection_type type,
			   const struct sockaddr *dst)
{
	const char auth_str[] = HTTP_CRLF "Authorization: Basic ";
	int ret = 0;
	char *ptr;

	NET_DBG("");

	ptr = strstr(ctx->http.field_values[0].key, auth_str);
	if (ptr) {
		char output[sizeof(HTTP_AUTH_USERNAME) +
			    sizeof(":") +
			    sizeof(HTTP_AUTH_PASSWORD)];
		size_t olen, ilen, alen;
		char *end, *colon;
		int ret;

		(void)memset(output, 0, sizeof(output));

		end = strstr(ptr + 2, HTTP_CRLF);
		if (!end) {
			ret = http_response_401(ctx, dst);
			goto close;
		}

		alen = sizeof(auth_str) - 1;
		ilen = end - (ptr + alen);

		ret = mbedtls_base64_decode(output, sizeof(output) - 1,
					    &olen, ptr + alen, ilen);
		if (ret) {
			ret = http_response_401(ctx, dst);
			goto close;
		}

		colon = memchr(output, ':', olen);

		if (colon && colon > output && colon < (output + olen) &&
		    memcmp(output, HTTP_AUTH_USERNAME, colon - output) == 0 &&
		    memcmp(colon + 1, HTTP_AUTH_PASSWORD,
			   output + olen - colon) == 0) {
			rpl.auth_ok = true;
			memcpy(&rpl.auth_addr,
			       &ctx->app_ctx.default_ctx->remote,
			       sizeof(struct sockaddr));
			if (type == WS_CONNECTION) {
				return 0;
			}

			ret = http_serve_index_html(ctx, dst);
			goto close;
		}

		ret = http_response_401(ctx, dst);
		goto close;
	}

	ret = http_response_401(ctx, dst);

close:
	http_close(ctx);
	return ret;
}

static int http_serve_index_html(struct http_ctx *ctx,
				 const struct sockaddr *dst)
{
	static const char index_html_gz[] = {
#include "index.html.gz.inc"
	};

	check_file_size("index.html", sizeof(index_html_gz));

	NET_DBG("Sending index.html (%zd bytes) to client",
		sizeof(index_html_gz));
	return http_response(ctx, HTTP_STATUS_200_OK_GZ, index_html_gz,
			     sizeof(index_html_gz), dst);
}

static int http_serve_style_css(struct http_ctx *ctx,
				const struct sockaddr *dst)
{
	static const char style_css_gz[] = {
#include "style.css.gz.inc"
	};

	check_file_size("style.css", sizeof(style_css_gz));

	NET_DBG("Sending style.css (%zd bytes) to client",
		sizeof(style_css_gz));
	return http_response(ctx, HTTP_STATUS_200_OK_GZ_CSS,
			     style_css_gz,
			     sizeof(style_css_gz),
			     dst);
}

static int http_serve_br_js(struct http_ctx *ctx,
			    const struct sockaddr *dst)
{
	static const char br_js_gz[] = {
#include "br.js.gz.inc"
	};

	check_file_size("br.js", sizeof(br_js_gz));

	NET_DBG("Sending br.js (%zd bytes) to client",
		sizeof(br_js_gz));
	return http_response(ctx, HTTP_STATUS_200_OK_GZ_CSS,
			     br_js_gz,
			     sizeof(br_js_gz), dst);
}

static int http_serve_favicon_ico(struct http_ctx *ctx,
				  const struct sockaddr *dst)
{
	static const char favicon_ico_gz[] = {
#include "favicon.ico.gz.inc"
	};

	check_file_size("favicon.ico", sizeof(favicon_ico_gz));

	NET_DBG("Sending favicon.ico (%zd bytes) to client",
		sizeof(favicon_ico_gz));
	return http_response(ctx, HTTP_STATUS_200_OK_GZ, favicon_ico_gz,
			     sizeof(favicon_ico_gz), dst);
}

static bool check_addr(struct http_ctx *ctx)
{
#if defined(CONFIG_NET_IPV4)
	if (ctx->app_ctx.ipv6.ctx->remote.sa_family == AF_INET6) {
		return memcmp(
			&net_sin6(&rpl.auth_addr)->sin6_addr,
			&net_sin6(&ctx->app_ctx.ipv6.ctx->remote)->sin6_addr,
			sizeof(struct in6_addr)) == 0;
	}
#endif
#if defined(CONFIG_NET_IPV4)
	if (ctx->app_ctx.ipv4.ctx->remote.sa_family == AF_INET) {
		return memcmp(
			&net_sin(&rpl.auth_addr)->sin_addr,
			&net_sin(&ctx->app_ctx.ipv4.ctx->remote)->sin_addr,
			sizeof(struct in_addr)) == 0;
	}
#endif

	return false;
}

static const char *addrtype2str(enum net_addr_type addr_type)
{
	switch (addr_type) {
	case NET_ADDR_ANY:
		return "unknown";
	case NET_ADDR_AUTOCONF:
		return "autoconf";
	case NET_ADDR_DHCP:
		return "DHCP";
	case NET_ADDR_MANUAL:
		return "manual";
	case NET_ADDR_OVERRIDABLE:
		return "overridable";
	}

	return "invalid";
}

static const char *addrstate2str(enum net_addr_state addr_state)
{
	switch (addr_state) {
	case NET_ADDR_ANY_STATE:
		return "unknown";
	case NET_ADDR_TENTATIVE:
		return "tentative";
	case NET_ADDR_PREFERRED:
		return "preferred";
	case NET_ADDR_DEPRECATED:
		return "deprecated";
	}

	return "invalid";
}

static int append_and_send_data(struct user_data *data,
				bool final,
				const char *fmt, ...)
{
	enum ws_opcode opcode = WS_OPCODE_CONTINUE;
	int ret, pos, len;
	va_list ap;

	va_start(ap, fmt);
	pos = vsnprintk(data->buf, MAX_BUF_LEN, fmt, ap);
	va_end(ap);

	len = strlen(data->buf);

	if (final) {
		if (data->msg_count == 0) {
			opcode = WS_OPCODE_DATA_TEXT;
		}

		ret = ws_send_msg_to_client(data->ctx, data->buf, len,
					    opcode, final, data->dst, NULL);
		if (ret < 0) {
			NET_DBG("Could not send %d bytes data to client", len);
			goto out;
		} else {
			NET_DBG("Sent %d bytes to client", len);
		}

		data->msg_count = 0;

		return ret;
	}

	if (data->msg_count == 0) {
		opcode = WS_OPCODE_DATA_TEXT;
	}

	ret = ws_send_msg_to_client(data->ctx, data->buf, len,
				    opcode, final, data->dst, NULL);
	if (ret < 0) {
		NET_DBG("Could not send %d bytes data to client", len);
		goto out;
	} else {
		NET_DBG("Sent %d bytes to client", len);
	}

	data->msg_count++;

out:
	return ret;
}

static inline u32_t remaining_lifetime(struct net_if_addr *ifaddr)
{
	s32_t remaining;

	if (ifaddr->lifetime.timer_timeout == 0) {
		return 0;
	}

	remaining = k_uptime_get() - ifaddr->lifetime.timer_start;

	return abs(remaining) / K_MSEC(1000);
}

static void append_unicast_addr(struct net_if *iface, struct user_data *data)
{
	char addr[NET_IPV6_ADDR_LEN], lifetime[10];
	struct net_if_addr *unicast;
	int i, ret = 0;
	int printed, count;

	for (i = 0, printed = 0, count = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		unicast = &iface->config.ip.ipv6->unicast[i];

		if (!unicast->is_used) {
			continue;
		}

		net_addr_ntop(AF_INET6, &unicast->address.in6_addr, addr,
			      NET_IPV6_ADDR_LEN);

		if (!printed) {
			ret = append_and_send_data(data, false,
						   "{\"IPv6\":[");
			if (ret < 0) {
				goto out;
			}
		}

		if (unicast->is_infinite) {
			snprintk(lifetime, sizeof(lifetime), "%s", "infinite");
		} else {
			snprintk(lifetime, sizeof(lifetime), "%u",
				 remaining_lifetime(unicast));
		}

		ret = append_and_send_data(data, false,
			       "%s{\"%s\":{"
			       "\"State\":\"%s\","
			       "\"Type\":\"%s\","
			       "\"Lifetime\":\"%s\""
			       "}}",
			       count > 0 ? "," : "", addr,
			       addrstate2str(unicast->addr_state),
			       addrtype2str(unicast->addr_type),
			       lifetime);
		if (ret < 0) {
			goto out;
		}

		count++;
		printed++;
	}

	if (printed > 0) {
		ret = append_and_send_data(data, false, "]}");
		if (ret < 0) {
			goto out;
		}
	}

out:
	if (ret < 0) {
		NET_ERR("Out of mem");
	}
}

static const char *iface2str(struct net_if *iface)
{
#ifdef CONFIG_NET_L2_IEEE802154
	if (iface->if_dev->l2 == &NET_L2_GET_NAME(IEEE802154)) {
		return "IEEE 802.15.4";
	}
#endif

#ifdef CONFIG_NET_L2_ETHERNET
	if (iface->if_dev->l2 == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}
#endif

#ifdef CONFIG_NET_L2_DUMMY
	if (iface->if_dev->l2 == &NET_L2_GET_NAME(DUMMY)) {
		return "Dummy";
	}
#endif

#ifdef CONFIG_NET_L2_BT
	if (iface->if_dev->l2 == &NET_L2_GET_NAME(BLUETOOTH)) {
		return "Bluetooth";
	}
#endif

#ifdef CONFIG_NET_L2_OFFLOAD
	if (iface->if_dev->l2 == &NET_L2_GET_NAME(OFFLOAD_IP)) {
		return "IP Offload";
	}
#endif

	return "unknown";
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct user_data *data = user_data;
	int ret;

	if (!net_if_is_up(iface)) {
		NET_DBG("Interface %p is down", iface);
		return;
	}

	ret = append_and_send_data(data, false, "%s{\"%p\":[",
				   data->iface_count > 0 ? "," : "", iface);
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(data, false, "{\"Type\":\"%s\"},",
				   iface2str(iface));
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(data, false, "{\"Link address\":\"%s\"},",
			   net_sprint_ll_addr(iface->if_dev->link_addr.addr,
					      iface->if_dev->link_addr.len));
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(data, false, "{\"MTU\":\"%d\"},",
				   iface->if_dev->mtu);
	if (ret < 0) {
		goto out;
	}

	append_unicast_addr(iface, data);

	/* Add more data here.... */

	data->iface_count++;

	ret = append_and_send_data(data, false, "]}");
	if (ret < 0) {
		goto out;
	}

out:
	if (ret < 0) {
		NET_ERR("Out of mem");
	}
}

static int send_iface_configuration(struct http_ctx *ctx,
				    const struct sockaddr *dst)
{
	struct user_data data;
	int ret;

	data.ctx = ctx;
	data.iface_count = 0;
	data.msg_count = 0;
	data.dst = dst;

	ret = append_and_send_data(&data, false,
				   "{\"interface_configuration\":[");
	if (ret < 0) {
		goto out;
	}

	net_if_foreach(iface_cb, &data);

	ret = append_and_send_data(&data, true, "]}");
	if (ret < 0) {
		goto out;
	}

	return ret;

out:
	return ret;
}

static const char *mode2str(enum net_rpl_mode mode)
{
	if (mode == NET_RPL_MODE_MESH) {
		return "mesh";
	} else if (mode == NET_RPL_MODE_FEATHER) {
		return "feather";
	} else if (mode == NET_RPL_MODE_LEAF) {
		return "leaf";
	}

	return "<unknown>";
}

static int _add_string(struct user_data *data,
		       const char *name,
		       const char *value,
		       bool first,
		       bool add_block)
{
	int ret;

	ret = append_and_send_data(data, false, "%s%s\"%s\":\"%s\"%s",
				   first ? "" : ",",
				   add_block ? "{" : "",
				   name, value,
				   add_block ? "}" : "");
	if (ret < 0) {
		goto out;
	}

out:
	return ret;
}

static int _add_int(struct user_data *data,
		    const char *name,
		    int value,
		    bool first,
		    bool add_block)
{
	int ret;

	ret = append_and_send_data(data, false, "%s%s\"%s\":\"%d\"%s",
				   first ? "" : ",",
				   add_block ? "{" : "",
				   name, value,
				   add_block ? "}" : "");
	if (ret < 0) {
		goto out;
	}

out:
	return ret;
}

static int add_string(struct user_data *data,
		      const char *name,
		      const char *value,
		      bool first)
{
	return _add_string(data, name, value, first, false);
}

static int add_int(struct user_data *data,
		   const char *name,
		   int value,
		   bool first)
{
	return _add_int(data, name, value, first, false);
}

static int add_string_block(struct user_data *data,
			    const char *name,
			    const char *value,
			    bool first)
{
	return _add_string(data, name, value, first, true);
}

static int add_int_block(struct user_data *data,
			 const char *name,
			 int value,
			 bool first)
{
	return _add_int(data, name, value, first, true);
}

static int add_rpl_config(struct user_data *data)
{
	int ret;

	ret = add_string_block(data, "RPL mode", mode2str(net_rpl_get_mode()),
			       true);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "Objective function",
			       IS_ENABLED(CONFIG_NET_RPL_MRHOF) ? "MRHOF" :
			       (IS_ENABLED(CONFIG_NET_RPL_OF0) ? "OF0" :
				"<unknown>"), false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "Routing metric",
			       IS_ENABLED(CONFIG_NET_RPL_MC_NONE) ? "none" :
			       (IS_ENABLED(CONFIG_NET_RPL_MC_ETX) ?
				"estimated num of TX" :
				(IS_ENABLED(CONFIG_NET_RPL_MC_ENERGY) ?
				 "energy based" :
				 "<unknown>")), false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "Mode of operation",
			       IS_ENABLED(CONFIG_NET_RPL_MOP2) ?
			       "Storing, no mcast (MOP2)" :
			       (IS_ENABLED(CONFIG_NET_RPL_MOP3) ?
				"Storing (MOP3)" :
				"<unknown>"), false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "Send probes to nodes",
			       IS_ENABLED(CONFIG_NET_RPL_PROBING) ?
			       "true" : "false", false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "Max instances",
			    CONFIG_NET_RPL_MAX_INSTANCES,
			    false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "Max DAG / instance",
			    CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE,
			    false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "Min hop rank increment",
			    CONFIG_NET_RPL_MIN_HOP_RANK_INC,
			    false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "Initial link metric",
			    CONFIG_NET_RPL_INIT_LINK_METRIC,
			    false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "RPL preference value",
			    CONFIG_NET_RPL_PREFERENCE, false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "DAG grounded by default",
			       IS_ENABLED(CONFIG_NET_RPL_GROUNDED) ?
			       "true" : "false", false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "Default instance id",
			    CONFIG_NET_RPL_DEFAULT_INSTANCE,
			    false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "Insert hop-by-hop option",
			       IS_ENABLED(CONFIG_NET_RPL_INSERT_HBH_OPTION) ?
			       "true" : "false", false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "Specify DAG when sending DAO",
			       IS_ENABLED(CONFIG_NET_RPL_DAO_SPECIFY_DAG) ?
			       "true" : "false", false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "DIO min interval",
			    CONFIG_NET_RPL_DIO_INTERVAL_MIN, false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "DIO doublings interval",
			    CONFIG_NET_RPL_DIO_INTERVAL_DOUBLINGS, false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "DIO redundancy value",
			    CONFIG_NET_RPL_DIO_REDUNDANCY,
			    false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "DAO send timer",
			    CONFIG_NET_RPL_DAO_TIMER, false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "DAO max retransmissions",
			    CONFIG_NET_RPL_DAO_MAX_RETRANSMISSIONS, false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "DAO ack expected",
			       IS_ENABLED(CONFIG_NET_RPL_DAO_ACK) ?
			       "true" : "false", false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string_block(data, "DIS send periodically",
			       IS_ENABLED(CONFIG_NET_RPL_DIS_SEND) ?
			       "true" : "false", false);
	if (ret < 0) {
		goto out;
	}

#if defined(CONFIG_NET_RPL_DIS_SEND)
	ret = add_int_block(data, "DIS interval", CONFIG_NET_RPL_DIS_INTERVAL,
			    false);
	if (ret < 0) {
		goto out;
	}
#endif

	ret = add_int_block(data, "Default route lifetime unit",
			    CONFIG_NET_RPL_DEFAULT_LIFETIME_UNIT, false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int_block(data, "Default route lifetime",
			    CONFIG_NET_RPL_DEFAULT_LIFETIME, false);
	if (ret < 0) {
		goto out;
	}

#if defined(CONFIG_NET_RPL_MOP3)
	ret = add_int_block(data, "Multicast MOP3 route lifetime",
			    CONFIG_NET_RPL_MCAST_LIFETIME, false);
	if (ret < 0) {
		goto out;
	}
#endif

out:
	return ret;
}

static int send_rpl_configuration(struct http_ctx *ctx,
				  const struct sockaddr *dst)
{
	struct user_data data;
	int ret;

	data.ctx = ctx;
	data.msg_count = 0;
	data.dst = dst;

	ret = append_and_send_data(&data, false, "{\"rpl_configuration\":[");
	if (ret < 0) {
		goto out;
	}

	ret = add_rpl_config(&data);
	if (ret < 0) {
		NET_ERR("Could not send RPL configuration");
		goto out;
	}

	ret = append_and_send_data(&data, true, "]}");
	if (ret < 0) {
		goto out;
	}

	return ret;

out:
	return ret;
}

static void append_nbr(struct net_nbr *nbr, struct user_data *data)
{
	int ret;

	if (data->iface != nbr->iface) {
		ret = append_and_send_data(data, false, "%s{\"%p\":[",
					   data->iface_count > 0 ? "]}," : "",
					   nbr->iface);
		if (ret < 0) {
			goto out;
		}

		data->iface_count++;
		data->nbr_count = 0;
		data->iface = nbr->iface;
	}

	ret = append_and_send_data(data, false, "%s{",
				   data->nbr_count > 0 ? "," : "");
	if (ret < 0) {
		goto out;
	}

	ret = add_string(data, "Link address",
			 nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
			 net_sprint_ll_addr(
				 net_nbr_get_lladdr(nbr->idx)->addr,
				 net_nbr_get_lladdr(nbr->idx)->len),
			 true);
	if (ret < 0) {
		goto out;
	}

	ret = add_string(data, "IPv6 address",
			 net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr),
			 false);
	if (ret < 0) {
		goto out;
	}

	ret = add_int(data, "Link metric", net_ipv6_nbr_data(nbr)->link_metric,
		      false);
	if (ret < 0) {
		goto out;
	}

	ret = add_string(data, "Is router",
			 net_ipv6_nbr_data(nbr)->is_router ? "true" : "false",
			 false);
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(data, false, "}");
	if (ret < 0) {
		goto out;
	}

	data->nbr_count++;

out:
	data->failure = ret;
}

static void nbr_cb(struct net_nbr *nbr, void *user_data)
{
	struct user_data *data = user_data;

	append_nbr(nbr, data);
}

static int send_ipv6_neighbors(struct http_ctx *ctx,
			       const struct sockaddr *dst,
			       struct net_nbr *nbr)
{
	struct user_data data;
	int ret;

	data.ctx = ctx;
	data.iface = NULL;
	data.msg_count = 0;
	data.dst = dst;

	ret = append_and_send_data(&data, false, "{\"neighbors\":[");
	if (ret < 0) {
		goto out;
	}

	data.nbr_count = 0;
	data.iface_count = 0;

	if (!nbr) {
		net_ipv6_nbr_foreach(nbr_cb, &data);
	} else {
		append_nbr(nbr, &data);
	}

	if (data.failure < 0) {
		ret = data.failure;
		goto out;
	}

	if (data.iface_count > 0) {
		ret = append_and_send_data(&data, false, "]}");
		if (ret < 0) {
			goto out;
		}
	}

	ret = append_and_send_data(&data, true, "]}");
	if (ret < 0) {
		ret = -ENOMEM;
		goto out;
	}

	return ret;

out:
	NET_DBG("Cannot send neighbor information");

	return ret;
}

static int send_ipv6_neighbor_deletion(struct http_ctx *ctx,
				       const struct sockaddr *dst,
				       struct net_if *iface,
				       struct in6_addr *addr)
{
	struct user_data data;
	int ret;

	data.ctx = ctx;
	data.iface = NULL;
	data.dst = dst;

	ret = append_and_send_data(&data, false, "{\"neighbors\":[");
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(&data, false, "{\"%p\":[{", iface);
	if (ret < 0) {
		goto out;
	}

	ret = add_string(&data, "Operation", "delete", true);
	if (ret < 0) {
		goto out;
	}

	ret = add_string(&data, "IPv6 address",
			 net_sprint_ipv6_addr(addr), false);
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(&data, true, "}]}]}");
	if (ret < 0) {
		goto out;
	}

	return ret;

out:
	NET_DBG("Cannot send neighbor information");

	return ret;
}

static void append_route(struct net_route_entry *entry, struct user_data *data)
{
	struct net_route_nexthop *nexthop_route;
	int ret = 0;

	if (entry->iface != data->iface) {
		return;
	}

	ret = append_and_send_data(data, false,
				   "%s{\"IPv6 prefix\":\"%s/%d\"",
				   data->route_count > 0 ? "," : "",
				   net_sprint_ipv6_addr(&entry->addr),
				   entry->prefix_len);
	if (ret < 0) {
		goto out;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&entry->nexthop, nexthop_route, node) {
		struct net_linkaddr_storage *lladdr;

		if (!nexthop_route->nbr) {
			continue;
		}

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			ret = add_string(data, "Link address", "unknown",
					 false);
			if (ret < 0) {
				ret = -ENOMEM;
				goto out;
			}
		} else {
			lladdr = net_nbr_get_lladdr(nexthop_route->nbr->idx);

			ret = add_string(data, "Link address",
					 net_sprint_ll_addr(lladdr->addr,
							    lladdr->len),
					 false);
			if (ret < 0) {
				ret = -ENOMEM;
				goto out;
			}
		}
	}

	ret = append_and_send_data(data, false, "}");
	if (ret < 0) {
		goto out;
	}

	data->route_count++;
	return;

out:
	data->failure = ret;
}

static void route_cb(struct net_route_entry *entry, void *user_data)
{
	struct user_data *data = user_data;

	append_route(entry, data);
}

static void append_route_iface(struct net_if *iface,
			       struct net_route_entry *route,
			       void *user_data)
{
	struct user_data *data = user_data;
	int ret;

	if (!net_if_is_up(iface)) {
		NET_DBG("Interface %p is down", iface);
		return;
	}

	ret = append_and_send_data(data, false, "%s{\"%p\":[",
				   data->iface_count > 0 ? "," : "", iface);
	if (ret < 0) {
		goto out;
	}

	data->iface = iface;
	data->route_count = 0;

	if (!route) {
		net_route_foreach(route_cb, data);
	} else {
		append_route(route, data);
	}

	data->iface_count++;

	ret = append_and_send_data(data, false, "]}");
	if (ret < 0) {
		ret = -ENOMEM;
		goto out;
	}

out:
	if (ret < 0) {
		NET_ERR("Out of mem");
	}
}

static void iface_cb_for_routes(struct net_if *iface, void *user_data)
{
	append_route_iface(iface, NULL, user_data);
}

static int send_ipv6_routes(struct http_ctx *ctx,
			    const struct sockaddr *dst,
			    struct net_if *iface,
			    struct net_route_entry *route)
{
	struct user_data data;
	int ret;

	data.ctx = ctx;
	data.iface_count = 0;
	data.msg_count = 0;
	data.dst = dst;

	ret = append_and_send_data(&data, false, "{\"routes\":[");
	if (ret < 0) {
		goto out;
	}

	if (!iface && !route) {
		net_if_foreach(iface_cb_for_routes, &data);
	} else {
		append_route_iface(iface, route, &data);
	}

	ret = append_and_send_data(&data, true, "]}");
	if (ret < 0) {
		goto out;
	}

	return ret;

out:
	return ret;
}

static int send_ipv6_route_deletion(struct http_ctx *ctx,
				    const struct sockaddr *dst,
				    struct net_if *iface,
				    struct net_event_ipv6_route *info)
{
	struct user_data data;
	int ret;

	data.ctx = ctx;
	data.dst = dst;

	ret = append_and_send_data(&data, false, "{\"routes\":[");
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(&data, false, "{\"%p\":[", iface);
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(&data, false, "{\"Operation\":\"delete\",");
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(&data, false,
				   "\"IPv6 prefix\":\"%s/%d\"",
				   net_sprint_ipv6_addr(&info->addr),
				   info->prefix_len);
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(&data, true, "}]}]}");
	if (ret < 0) {
		goto out;
	}

	return ret;

out:
	return ret;
}

static void calculate_edges(void)
{
	u8_t i, j, k;

	k = 0;

	for (i = 1; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		if (!topology.nodes[i].used) {
			continue;
		}

		for (j = 0; j < CONFIG_NET_IPV6_MAX_NEIGHBORS; j++) {
			if (!topology.nodes[j].used) {
				continue;
			}

			if (!net_ipv6_addr_cmp(&topology.nodes[i].parent,
					       &topology.nodes[j].addr)) {
				continue;
			}

			topology.edges[k].from = topology.nodes[i].id;
			topology.edges[k].to = topology.nodes[j].id;
			topology.edges[k].used = true;

			k++;
			break;

		}
	}
}

static int send_topology_information(struct http_ctx *ctx,
				     const struct sockaddr *dst)
{
	struct user_data data;
	u8_t i;
	int ret;

	data.ctx = ctx;
	data.iface_count = 0;
	data.msg_count = 0;
	data.dst = dst;

	calculate_edges();

	ret = append_and_send_data(&data, false, "{\"topology\":{");
	if (ret < 0) {
		goto out;
	}

	ret = append_and_send_data(&data, false, "\"nodes\":[");
	if (ret < 0) {
		goto out;
	}

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		if (!topology.nodes[i].used) {
			continue;
		}

		ret = append_and_send_data(&data, false, i == 0 ? "{" : ",{");
		if (ret < 0) {
			goto out;
		}

		ret = add_int(&data, "id", topology.nodes[i].id, true);
		if (ret < 0) {
			goto out;
		}

		ret = add_string(&data, "label",
				 topology.nodes[i].label, false);
		if (ret < 0) {
			goto out;
		}

		ret = add_string(&data, "title",
				 net_sprint_ipv6_addr(&topology.nodes[i].addr),
				 false);
		if (ret < 0) {
			goto out;
		}

		ret = append_and_send_data(&data, false, "}");
		if (ret < 0) {
			goto out;
		}
	}

	ret = append_and_send_data(&data, false, "],\"edges\":[");
	if (ret < 0) {
		goto out;
	}

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		if (!topology.edges[i].used) {
			continue;
		}

		ret = append_and_send_data(&data, false, i == 0 ? "{" : ",{");
		if (ret < 0) {
			goto out;
		}

		ret = add_int(&data, "from", topology.edges[i].from, true);
		if (ret < 0) {
			goto out;
		}

		ret = add_int(&data, "to", topology.edges[i].to, false);
		if (ret < 0) {
			goto out;
		}

		ret = append_and_send_data(&data, false, "}");
		if (ret < 0) {
			goto out;
		}
	}

	ret = append_and_send_data(&data, true, "]}}");
	if (ret < 0) {
		goto out;
	}

	return ret;

out:
	return ret;
}

static void ws_send_info(struct http_ctx *ctx,
			 const struct sockaddr *dst)
{
	int ret;

	ret = send_iface_configuration(ctx, dst);
	if (ret < 0) {
		NET_ERR("Cannot send interface configuration (%d)", ret);
	}

	ret = send_rpl_configuration(ctx, dst);
	if (ret < 0) {
		NET_ERR("Cannot send RPL configuration (%d)", ret);
	}

	ret = send_ipv6_neighbors(ctx, dst, NULL);
	if (ret < 0) {
		NET_ERR("Cannot send neighbor information (%d)", ret);
		return;
	}

	ret = send_ipv6_routes(ctx, dst, NULL, NULL);
	if (ret < 0) {
		NET_ERR("Cannot send route information (%d)", ret);
		return;
	}

	ret = send_topology_information(ctx, dst);
	if (ret < 0) {
		NET_ERR("Cannot send topology information (%d)", ret);
	}
}

struct ws_http_ctx {
	struct http_ctx *ctx;
	const struct sockaddr *dst;
	bool data_set;
};

static struct ws_http_ctx ws_ctx;

static void ws_serve_replies(void)
{
	ws_send_info(ws_ctx.ctx, ws_ctx.dst);

	ws_ctx.data_set = false;
}

static void http_connected(struct http_ctx *ctx,
			   enum http_connection_type type,
			   const struct sockaddr *dst,
			   void *user_data)
{
	char url[32];
	size_t len = min(sizeof(url) - 1, ctx->http.url_len);

	NET_DBG("");

	if (0 && (!rpl.auth_ok || !check_addr(ctx))) {
		rpl.auth_ok = false;
		http_basic_auth(ctx, type, dst);
		return;
	}

	memcpy(url, ctx->http.url, len);
	url[len] = '\0';

	NET_DBG("%s connect attempt URL %s",
		type == HTTP_CONNECTION ? "HTTP" :
		(type == WS_CONNECTION ? "WS" : "<unknown>"), url);

	if (type == HTTP_CONNECTION) {
		if (strncmp(ctx->http.url, "/",
			    ctx->http.url_len) == 0) {
			http_serve_index_html(ctx, dst);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/index.html",
			    ctx->http.url_len) == 0) {
			http_serve_index_html(ctx, dst);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/br.js",
			    ctx->http.url_len) == 0) {
			http_serve_br_js(ctx, dst);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/style.css",
			    ctx->http.url_len) == 0) {
			http_serve_style_css(ctx, dst);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/favicon.ico",
			    ctx->http.url_len) == 0) {
			http_serve_favicon_ico(ctx, dst);
			http_close(ctx);
			return;
		}
	} else if (type == WS_CONNECTION) {
		if (strncmp(ctx->http.url, "/ws", ctx->http.url_len) == 0) {
			ws_ctx.ctx = ctx;
			ws_ctx.dst = dst;
			ws_dst = dst;
			ws_ctx.data_set = true;
			k_sem_give(&ws_reply);
		}
	}
}

/**
 * The sample coap JSON command from WebUI looks like this.
 * {"coap":{"command":"led_on","ipv6_addr":"fe80::212:4b00:0:2"}}
 */

#define JSON_COAP_PREFIX "{\"coap\":"
#define MAX_PAYLOAD_LEN	100
struct coap_command {
	const char *command;
	const char *ipv6_addr;
};

struct rpl_coap_req {
	struct coap_command coap;
};

static const struct json_obj_descr command_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct coap_command, command, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct coap_command, ipv6_addr, JSON_TOK_STRING),
};

static const struct json_obj_descr coap_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct rpl_coap_req, coap, command_descr),
};

static void handle_coap_request(struct http_ctx *ctx,
				struct net_pkt *pkt,
				void *user_data)
{
	struct rpl_coap_req req;
	struct net_buf *frag;
	struct in6_addr peer_addr;
	enum coap_request_type type;
	char payload[MAX_PAYLOAD_LEN];
	u8_t *ptr;
	u8_t len;
	u16_t pos;
	int ret;

	len = net_pkt_appdatalen(pkt);
	if (len > MAX_PAYLOAD_LEN - 1) {
		NET_ERR("Can't handle payload more than %d(%d)",
			MAX_PAYLOAD_LEN, len);
		return;
	}

	frag = pkt->frags;
	ptr = net_pkt_appdata(pkt);

	pos = (u16_t)(ptr - frag->data);

	frag = net_frag_read(frag, pos, &pos, len, &payload[0]);
	if (!frag && pos == 0xffff) {
		NET_WARN("Failed to read payload");
		return;
	}

	payload[len] = '\0';

	ret = json_obj_parse((char *)payload, len, coap_descr,
			     ARRAY_SIZE(coap_descr), &req);
	if (ret < 0) {
		NET_ERR("Failed to parse JSON string %d", ret);
		return;
	}

	ret = net_addr_pton(AF_INET6, req.coap.ipv6_addr, &peer_addr);
	if (ret < 0) {
		NET_WARN("Invalid peer address %s", req.coap.ipv6_addr);
		return;
	}

	if (strcmp(req.coap.command, "toggle") == 0) {
		type = COAP_REQ_TOGGLE_LED;
	} else {
		NET_WARN("Invalid coap command %s", req.coap.command);
		return;
	}

	coap_send_request(&peer_addr, type, NULL, NULL);
	NET_DBG("Send CoAP request '%s'-'%s'", req.coap.command,
		req.coap.ipv6_addr);
}

static void http_received(struct http_ctx *ctx,
			  struct net_pkt *pkt,
			  int status,
			  u32_t flags,
			  const struct sockaddr *dst,
			  void *user_data)
{
	if (!pkt) {
		NET_DBG("Received NULL packet for unknown reason");
		return;
	}

	if (!status) {
		NET_DBG("Received %d bytes data", net_pkt_appdatalen(pkt));

		if (!strncmp((char *)net_pkt_appdata(pkt), JSON_COAP_PREFIX,
			     sizeof(JSON_COAP_PREFIX) - 1)) {
			handle_coap_request(ctx, pkt, user_data);
		}
	} else {
		NET_ERR("Receive error (%d)", status);
	}

	net_pkt_unref(pkt);
}

static void http_sent(struct http_ctx *ctx,
		      int status,
		      void *user_data_send,
		      void *user_data)
{
	NET_DBG("Data sent status %d", status);
}

static void http_closed(struct http_ctx *ctx,
			int status,
			void *user_data)
{
	NET_DBG("Connection %p closed", ctx);
}

static const char *get_string(int str_len, const char *str)
{
	static char buf[64];
	int len = min(str_len, sizeof(buf) - 1);

	memcpy(buf, str, len);
	buf[len] = '\0';

	return buf;
}

static enum http_verdict default_handler(struct http_ctx *ctx,
					 enum http_connection_type type,
					 const struct sockaddr *dst)
{
	NET_DBG("No handler for %s URL %s",
		type == HTTP_CONNECTION ? "HTTP" : "WS",
		get_string(ctx->http.url_len, ctx->http.url));

	if (type == HTTP_CONNECTION) {
		http_response_soft_404(ctx, dst);
	}

	return HTTP_VERDICT_DROP;
}

static void coap_obs_cb(struct coap_packet *response, void *user_data)
{
	int ret;

	ret = send_topology_information(&http_ctx, ws_dst);
	if (ret < 0) {
		NET_ERR("Cannot send topology (%d)", ret);
	}
}

static struct net_mgmt_event_callback br_mgmt_cb;
static void mgmt_cb(struct net_mgmt_event_callback *cb,
		    u32_t mgmt_event, struct net_if *iface)
{
#if !defined(CONFIG_NET_L2_IEEE802154)
	NET_DBG("CONFIG_NET_L2_IEEE802154 not enabled");
	return;
#endif
	struct net_if *iface_802154 = net_if_get_ieee802154();
	struct net_event_ipv6_route *route_info;
	struct net_event_ipv6_nbr *nbr_info;
	struct net_route_entry *route;
	struct net_nbr *nbr;
	int ret;

	if (iface_802154 != iface) {
		return;
	}

	if (!cb->info) {
		return;
	}

	if (mgmt_event == NET_EVENT_IPV6_NBR_ADD) {
		nbr_info = (struct net_event_ipv6_nbr *)cb->info;
		if (!nbr_info) {
			NET_ERR("Invalid info received on event");
			return;
		}

		nbr = net_ipv6_nbr_lookup(iface, &nbr_info->addr);
		if (!nbr || !net_ipv6_nbr_data(nbr)) {
			NET_ERR("Invalid neighbor data received");
			return;
		}

		NET_DBG("NBR add %s", net_sprint_ipv6_addr(&nbr_info->addr));

		ret = send_ipv6_neighbors(&http_ctx, ws_dst, nbr);
		if (ret < 0) {
			NET_ERR("Cannot send neighbor information (%d)", ret);
			return;
		}
	} else if (mgmt_event == NET_EVENT_IPV6_NBR_DEL) {
		nbr_info = (struct net_event_ipv6_nbr *)cb->info;
		if (!nbr_info) {
			NET_ERR("Invalid info received on event");
			return;
		}

		NET_DBG("NBR del %s", net_sprint_ipv6_addr(&nbr_info->addr));

		ret = send_ipv6_neighbor_deletion(&http_ctx, ws_dst, iface,
						  &nbr_info->addr);
		if (ret < 0) {
			NET_ERR("Cannot send neighbor information (%d)", ret);
			return;
		}
	} else if (mgmt_event == NET_EVENT_IPV6_ROUTE_ADD) {
		route_info = (struct net_event_ipv6_route *)cb->info;
		if (!route_info) {
			NET_ERR("Invalid info received on event");
			return;
		}

		route = net_route_lookup(iface, &route_info->addr);
		if (!route) {
			NET_ERR("Invalid route entry received");
			return;
		}

		NET_DBG("ROUTE add addr %s/%d",
			net_sprint_ipv6_addr(&route_info->addr),
			route_info->prefix_len);
		{
			NET_DBG("ROUTE add nexthop %s",
				net_sprint_ipv6_addr(&route_info->nexthop));

		}

		coap_send_request(&route_info->nexthop,
				  COAP_REQ_RPL_OBS, coap_obs_cb, NULL);

		ret = send_ipv6_routes(&http_ctx, ws_dst, iface, route);
		if (ret < 0) {
			NET_ERR("Cannot send route information (%d)", ret);
			return;
		}
	} else if (mgmt_event == NET_EVENT_IPV6_ROUTE_DEL) {
		route_info = (struct net_event_ipv6_route *)cb->info;
		if (!route_info) {
			NET_ERR("Invalid info received on event");
			return;
		}

		NET_DBG("ROUTE del addr %s/%d",
			net_sprint_ipv6_addr(&route_info->addr),
			route_info->prefix_len);
		{
			NET_DBG("ROUTE del nexthop %s",
				net_sprint_ipv6_addr(&route_info->nexthop));

		}

		ret = send_ipv6_route_deletion(&http_ctx, ws_dst, iface,
					       route_info);
		if (ret < 0) {
			NET_ERR("Cannot send route information (%d)", ret);
			return;
		}

		coap_remove_node_from_topology(&route_info->nexthop);

		ret = send_topology_information(&http_ctx, ws_dst);
		if (ret < 0) {
			NET_ERR("Cannot send topology information (%d)", ret);
		}
	}
}

#define WS_HTTP_STACK_SIZE	2500
NET_STACK_DEFINE(WS_HTTP, ws_http_stack, WS_HTTP_STACK_SIZE,
		 WS_HTTP_STACK_SIZE);
static struct k_thread ws_http_thread_data;

static void ws_http_thread(void)
{
	while (1) {
		k_sem_take(&ws_reply, K_FOREVER);

		if (ws_ctx.data_set) {
			ws_serve_replies();
		}
	}
}

void start_http_server(struct net_if *iface)
{
	struct sockaddr addr, *server_addr;
	int ret;

	/*
	 * There are several options here for binding to local address.
	 * 1) The server address can be left empty in which case the
	 *    library will bind to both IPv4 and IPv6 addresses and to
	 *    port 80 which is the default, or 443 if TLS is enabled.
	 * 2) The server address can be partially filled, meaning that
	 *    the address can be left to 0 and port can be set if a value
	 *    other than 80 is desired. If the protocol family in sockaddr
	 *    is set to AF_UNSPEC, then both IPv4 and IPv6 sockets are bound.
	 * 3) The address can be set to some real value.
	 */
#define ADDR_OPTION 1

#if ADDR_OPTION == 1
	server_addr = NULL;

	ARG_UNUSED(addr);

#elif ADDR_OPTION == 2
	/* Accept any local listening address */
	(void)memset(&addr, 0, sizeof(addr));

	net_sin(&addr)->sin_port = htons(ZEPHYR_PORT);

	/* In this example, listen only IPv4 */
	addr.sa_family = AF_INET;

	server_addr = &addr;

#elif ADDR_OPTION == 3
	/* Set the bind address according to your configuration */
	(void)memset(&addr, 0, sizeof(addr));

	/* In this example, listen only IPv6 */
	addr.sa_family = AF_INET6;
	net_sin6(&addr)->sin6_port = htons(ZEPHYR_PORT);

	ret = net_ipaddr_parse(ZEPHYR_ADDR, strlen(ZEPHYR_ADDR), &addr);
	if (ret < 0) {
		NET_ERR("Cannot set local address (%d)", ret);
		panic(NULL);
	}

	server_addr = &addr;

#else
	server_addr = NULL;

	ARG_UNUSED(addr);
#endif

	http_server_add_default(&http_urls, default_handler);
	http_server_add_url(&http_urls, "/", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/index.html", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/style.css", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/br.js", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/favicon.ico", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/ws", HTTP_URL_WEBSOCKET);

	ret = http_server_init(&http_ctx, &http_urls, server_addr,
			       result, sizeof(result),
			       "Zephyr HTTP Server for border router", NULL);
	if (ret < 0) {
		NET_ERR("Cannot init web server (%d)", ret);
		return;
	}


	http_set_cb(&http_ctx, http_connected, http_received, http_sent,
		    http_closed);

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&http_ctx.app_ctx, tx_slab, data_pool);
#endif

#if defined(CONFIG_NET_APP_TLS)
	ret = http_server_set_tls(&http_ctx,
				  NULL,
				  INSTANCE_INFO,
				  strlen(INSTANCE_INFO),
				  setup_cert,
				  NULL,
				  &ssl_rx_pool,
				  https_stack,
				  K_THREAD_STACK_SIZEOF(https_stack));
	if (ret < 0) {
		NET_ERR("Cannot enable TLS support (%d)", ret);
	}
#endif

	http_server_enable(&http_ctx);

	net_mgmt_init_event_callback(&br_mgmt_cb, mgmt_cb,
				     NET_EVENT_IPV6_NBR_ADD |
				     NET_EVENT_IPV6_NBR_DEL |
				     NET_EVENT_IPV6_ROUTE_ADD |
				     NET_EVENT_IPV6_ROUTE_DEL);
	net_mgmt_add_event_callback(&br_mgmt_cb);

	/* Run http(WS) replies in separate thread */
	k_sem_init(&ws_reply, 0, UINT_MAX);

	k_thread_create(&ws_http_thread_data, ws_http_stack,
			K_THREAD_STACK_SIZEOF(ws_http_stack),
			(k_thread_entry_t)ws_http_thread, NULL, NULL, NULL,
			K_PRIO_COOP(10), 0, 0);
}

void stop_http_server(void)
{
	http_server_disable(&http_ctx);
	http_release(&http_ctx);
}
