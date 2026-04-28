/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdbool.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/parser_url.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/dns_resolve.h>
#if defined(CONFIG_TLS_CREDENTIALS)
#include <zephyr/net/tls_credentials.h>
#endif

#include "net_shell_private.h"

#define DEFAULT_HTTPS_PORT 443
#define DEFAULT_HTTP_PORT  80

#define HTTP_SCHEMA_LEN   sizeof("https://")
#define HTTP_PROTOCOL_LEN sizeof("HTTP/1.1")

#if defined(CONFIG_HTTP_CLIENT)
static struct net_mgmt_event_callback l4_cb;
static K_SEM_DEFINE(l4_wait, 0, 1);
static K_SEM_DEFINE(dns_wait, 0, 1);

enum http_client_options {
	HTTP_CLIENT_HEADERS,
	HTTP_CLIENT_POSTFIELDS,
	HTTP_CLIENT_POSTFIELDS_SIZE,
	HTTP_CLIENT_PROTOCOL,
	HTTP_CLIENT_SSL_CERTIFICATE_TAG,
	HTTP_CLIENT_SSL_VERIFYHOST,
	HTTP_CLIENT_SSL_VERIFYPEER,
	HTTP_CLIENT_USERPWD,
	HTTP_CLIENT_WRITEFUNCTION,
};

struct http_client_url_fields {
	uint8_t schema[HTTP_SCHEMA_LEN];
	uint8_t hostname[ZSOCK_NI_MAXHOST];
	uint8_t uri[CONFIG_NET_SHELL_HTTP_CLIENT_URL_LEN];
	uint16_t port;
	bool is_ssl;
};

struct http_client_sh_ctx {
	int sockfd;
	struct net_sockaddr_storage sa;
	struct http_client_url_fields url_fields;
	enum http_method method;
	http_response_cb_t cb;
	uint8_t recv_buf[NET_IPV4_MTU];
	uint8_t payload[NET_IPV4_MTU];
	uint16_t payload_size;
	uint8_t protocol[HTTP_PROTOCOL_LEN];
	const char **headers;
	bool is_ssl_verifyhost;
	int is_ssl_verifypeer;
	int sec_tag;
	int err;
	const struct shell *sh;
};

static int http_client_url_fields_get(const uint8_t *url, const struct http_parser_url *purl,
				      enum http_parser_url_fields url_field, uint8_t *field,
				      uint16_t field_len)
{
	uint16_t off = purl->field_data[url_field].off;
	uint16_t len = purl->field_data[url_field].len;

	if (len == 0 || len >= field_len) {
		return -EINVAL;
	}

	if (url_field == UF_PATH) {
		memcpy(field, url + off, strlen(url));
		field[strlen(url)] = '\0';
	} else {
		memcpy(field, url + off, len);
		field[len] = '\0';
	}

	return 0;
}

static int http_client_url_parser(struct http_client_sh_ctx *ctx, const uint8_t *url)
{
	int ret;
	const struct shell *sh = ctx->sh;
	struct http_parser_url purl;
	uint8_t *schema = ctx->url_fields.schema;
	uint8_t *hostname = ctx->url_fields.hostname;
	uint16_t *port = &ctx->url_fields.port;
	uint8_t *uri = ctx->url_fields.uri;
	bool *is_ssl = &ctx->url_fields.is_ssl;
	uint8_t port_ascii[6];

	http_parser_url_init(&purl);

	ret = http_parser_parse_url(url, strlen(url), 0, &purl);
	if (ret < 0) {
		PR_ERROR("Failed to parse URL (%d)\n", ret);
		return ret;
	}

	ret = http_client_url_fields_get(url, &purl, UF_SCHEMA, schema,
					 sizeof(ctx->url_fields.schema));
	if (ret < 0) {
		PR_ERROR("Failed to parse schema (%d)\n", ret);
		return ret;
	}

	if (memcmp(schema, "https", sizeof("https")) == 0) {
		*is_ssl = true;
	}

	ret = http_client_url_fields_get(url, &purl, UF_HOST, hostname,
					 sizeof(ctx->url_fields.hostname));
	if (ret < 0) {
		PR_ERROR("Failed to parse hostname (%d)\n", ret);
		return ret;
	}

	ret = http_client_url_fields_get(url, &purl, UF_PORT, port_ascii, sizeof(port_ascii));
	if (ret < 0) {
		if (*is_ssl) {
			*port = DEFAULT_HTTPS_PORT;
		} else {
			*port = DEFAULT_HTTP_PORT;
		}
	} else {
		*port = (uint16_t)shell_strtol(port_ascii, 10, &ret);
		if (ret < 0) {
			PR_ERROR("Failed to parse port\n");
			return ret;
		}
	}

	ret = http_client_url_fields_get(url, &purl, UF_PATH, uri, sizeof(ctx->url_fields.uri));
	if (ret < 0) {
		memcpy(uri, "/", sizeof("/"));
	}

	PR("Hostname: %s, Port: %d, URI: %s\n", hostname, *port, uri);

	return 0;
}

static void net_event_l4_handler(struct net_mgmt_event_callback *cb, uint64_t status,
				 struct net_if *iface)
{
	switch (status) {
	case NET_EVENT_L4_CONNECTED:
		k_sem_give(&l4_wait);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		break;
	default:
		break;
	}
}

static void http_client_dns_cb(enum dns_resolve_status status, struct dns_addrinfo *info,
			       void *user_data)
{
	struct http_client_sh_ctx *ctx = (struct http_client_sh_ctx *)user_data;
	const struct shell *sh = ctx->sh;

	switch (status) {
	case DNS_EAI_CANCELED:
	case DNS_EAI_FAIL:
	case DNS_EAI_NODATA:
		break;

	case DNS_EAI_ALLDONE:
		k_sem_give(&dns_wait);
		break;

	default:
		break;
	}

	if (info == NULL) {
		return;
	}

	if (info->ai_family == NET_AF_INET) {
		PR("Host IPv4 address: %s\n",
		   net_sprint_ipv4_addr(&net_sin(&info->ai_addr)->sin_addr));
	} else if (info->ai_family == NET_AF_INET6) {
		PR("Host IPv6 address: %s\n",
		   net_sprint_ipv6_addr(&net_sin6(&info->ai_addr)->sin6_addr));
	} else {
		PR_ERROR("Invalid IP address family (%d)\n", info->ai_family);
		ctx->err = -EINVAL;
		return;
	}

	ctx->sa = *(net_sas(&info->ai_addr));
	ctx->sa.ss_family = info->ai_family;
	ctx->err = 0;
}

static int http_client_dns_get_info(struct http_client_sh_ctx *ctx, enum dns_query_type type)
{
	int ret;
	uint16_t dns_id;

	ret = dns_get_addr_info(ctx->url_fields.hostname, type, &dns_id, http_client_dns_cb,
				(void *)ctx, CONFIG_NET_SOCKETS_DNS_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = k_sem_take(&dns_wait, K_MSEC(CONFIG_NET_SOCKETS_DNS_TIMEOUT));
	if (ret < 0) {
		return ret;
	}

	return ctx->err;
}

static int http_client_dns_lookup(struct http_client_sh_ctx *ctx)
{
	int ret;
	const struct shell *sh = ctx->sh;

	if (!ctx->url_fields.hostname[0]) {
		PR_ERROR("Invalid hostname\n");
		return -EINVAL;
	}

	net_mgmt_init_event_callback(&l4_cb, net_event_l4_handler,
				     NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&l4_cb);
	conn_mgr_mon_resend_status();

	ret = k_sem_take(&l4_wait, K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT));
	if (ret < 0) {
		PR_ERROR("Failed to resolve IP address (%d)\n", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = http_client_dns_get_info(ctx, DNS_QUERY_TYPE_A);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && ctx->sa.ss_family == NET_AF_UNSPEC) {
		ret = http_client_dns_get_info(ctx, DNS_QUERY_TYPE_AAAA);
	}

	return ret;
}

static int http_client_connect_setup(struct http_client_sh_ctx *ctx)
{
	const struct shell *sh = ctx->sh;
	bool is_ssl = ctx->url_fields.is_ssl;

	ctx->sockfd = zsock_socket(ctx->sa.ss_family, NET_SOCK_STREAM,
				   is_ssl ? NET_IPPROTO_TLS_1_2 : NET_IPPROTO_TCP);
	if (ctx->sockfd < 0) {
		PR_ERROR("Failed to create socket (%d)\n", -ECONNABORTED);
		return -ECONNABORTED;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	int ret;

	if (ctx->is_ssl_verifyhost && is_ssl) {
		ret = zsock_setsockopt(ctx->sockfd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME,
				       ctx->url_fields.hostname, strlen(ctx->url_fields.hostname));
		if (ret < 0) {
			PR_ERROR("Failed to set TLS_HOSTNAME %s option (%d)\n",
				 ctx->url_fields.hostname, ret);
			return ret;
		}
	}

	if (ctx->is_ssl_verifyhost == 0) {
		PR_WARNING("Hostname verification is disabled\n");

		ret = zsock_setsockopt(ctx->sockfd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, NULL, 0);
		if (ret < 0) {
			PR_ERROR("Failed to set TLS_HOSTNAME %s option (%d)\n",
				 ctx->url_fields.hostname, ret);
			return ret;
		}
	}

	if (ctx->is_ssl_verifypeer && is_ssl) {
		ret = zsock_setsockopt(ctx->sockfd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
				       &ctx->sec_tag, sizeof(int));
		if (ret < 0) {
			PR_ERROR("Failed to set TLS_SEC_TAG_LIST option (%d)\n", ret);
			return ret;
		}
	}

	if (ctx->is_ssl_verifypeer == 0) {
		PR_WARNING("TLS verification is disabled\n");

		ret = zsock_setsockopt(ctx->sockfd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
				       &ctx->is_ssl_verifypeer, sizeof(int));
		if (ret < 0) {
			PR_ERROR("Failed to set TLS_PEER_VERIFY option (%d)\n", ret);
			return ret;
		}
	}
#endif

	return 0;
}

static int http_client_connect(struct http_client_sh_ctx *ctx)
{
	int ret;
	const struct shell *sh = ctx->sh;
	struct net_sockaddr_in *sa_in;
	struct net_sockaddr_in6 *sa_in6;

	ret = http_client_connect_setup(ctx);
	if (ret < 0) {
		PR_ERROR("Failed to setup connection (%d)\n", -ECONNABORTED);
		return -ECONNABORTED;
	}

	if (ctx->sa.ss_family == NET_AF_INET) {
		sa_in = (struct net_sockaddr_in *)(&ctx->sa);
		sa_in->sin_port = net_htons(ctx->url_fields.port);
	} else if (ctx->sa.ss_family == NET_AF_INET6) {
		sa_in6 = (struct net_sockaddr_in6 *)(&ctx->sa);
		sa_in6->sin6_port = net_htons(ctx->url_fields.port);
	} else {
		PR_ERROR("Invalid IP address family (%d)\n", ctx->sa.ss_family);
		return -EINVAL;
	}

	ret = zsock_connect(ctx->sockfd, net_sad(&ctx->sa), net_family2size(ctx->sa.ss_family));
	if (ret < 0) {
		PR_ERROR("Failed to connect to remote (%d)\n", -ECONNABORTED);
		zsock_close(ctx->sockfd);
		ctx->sockfd = -1;
		return -ECONNABORTED;
	}

	return 0;
}

#if defined(CONFIG_NET_SHELL_HTTP_CLIENT_HAS_CERTIFICATE)
static const uint8_t ca_certificate[] = {
#include "certs.der.inc"
};

static int http_client_certificate(void)
{
	int ret;

	ret = tls_credential_add(CONFIG_NET_SHELL_HTTP_CLIENT_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE, ca_certificate,
				 sizeof(ca_certificate));
	if (ret < 0) {
		LOG_ERR("Failed to register public certificate (%d)", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(http_client_certificate, APPLICATION, 0);
#endif /* CONFIG_NET_SHELL_HTTP_CLIENT_HAS_CERTIFICATE */

static int http_client_init(struct http_client_sh_ctx *ctx, const uint8_t *url)
{
	int ret;
	const struct shell *sh = ctx->sh;

	memset(ctx, 0, sizeof(struct http_client_sh_ctx));

	ctx->sh = sh;
	ctx->is_ssl_verifyhost = true;
	ctx->is_ssl_verifypeer = true;

#if defined(CONFIG_TLS_CREDENTIALS)
	ctx->sec_tag = CONFIG_NET_SHELL_HTTP_CLIENT_CERTIFICATE_TAG;
#endif

	ret = http_client_url_parser(ctx, url);
	if (ret < 0) {
		PR_ERROR("Failed to parse URL (%d)\n", ret);
		return ret;
	}

	ret = http_client_dns_lookup(ctx);
	if (ret < 0) {
		PR_ERROR("Failed to resolve DNS address (%d)\n", ret);
		return ret;
	}

	return 0;
}

static void http_client_setopt(struct http_client_sh_ctx *ctx, enum http_client_options option,
			       void *parameter)
{
	switch (option) {
	case HTTP_CLIENT_HEADERS:
		ctx->headers = (const char **)parameter;
		break;
	case HTTP_CLIENT_POSTFIELDS:
		memcpy(ctx->payload, (uint8_t *)parameter, sizeof(ctx->payload));
		break;
	case HTTP_CLIENT_POSTFIELDS_SIZE:
		ctx->payload_size = *((uint16_t *)parameter);
		break;
	case HTTP_CLIENT_PROTOCOL:
		memcpy(ctx->protocol, (uint8_t *)parameter, sizeof(ctx->protocol));
		break;
	case HTTP_CLIENT_SSL_CERTIFICATE_TAG:
		ctx->sec_tag = *((int *)parameter);
		break;
	case HTTP_CLIENT_SSL_VERIFYHOST:
		ctx->is_ssl_verifyhost = *((bool *)parameter);
		break;
	case HTTP_CLIENT_SSL_VERIFYPEER:
		ctx->is_ssl_verifypeer = *((int *)parameter);
		break;
	case HTTP_CLIENT_WRITEFUNCTION:
		ctx->cb = (http_response_cb_t)parameter;
		break;
	default:
		break;
	}
}

static int http_client_perform(struct http_client_sh_ctx *ctx, enum http_method method)
{
	int ret;
	struct http_request req;

	memset(&req, 0, sizeof(struct http_request));

	ret = http_client_connect(ctx);
	if (ret < 0) {
		return ret;
	}

	req.method = method;
	req.url = ctx->url_fields.uri;
	req.host = ctx->url_fields.hostname;
	req.header_fields = ctx->headers;
	req.protocol = ctx->protocol;
	req.response = ctx->cb;
	req.recv_buf = ctx->recv_buf;
	req.recv_buf_len = sizeof(ctx->recv_buf);

	if (method == HTTP_POST || method == HTTP_PUT) {
		req.payload = ctx->payload;
		req.payload_len = ctx->payload_size;
	}

	ret = http_client_req(ctx->sockfd, &req, CONFIG_NET_SOCKETS_CONNECT_TIMEOUT, ctx);
	zsock_close(ctx->sockfd);

	return ret;
}

static int http_client_handler(struct http_response *rsp, enum http_final_call final_data,
			       void *user_data)
{
	struct http_client_sh_ctx *ctx = (struct http_client_sh_ctx *)user_data;
	const struct shell *sh = ctx->sh;

	if (rsp->http_status_code == 0) {
		PR_WARNING("No HTTP response received\n");
		return -ENODATA;
	}

	if (rsp->body_frag_len > 0) {
		shell_print(sh, "%.*s", rsp->body_frag_len, rsp->body_frag_start);
	}

	return 0;
}
#endif /* CONFIG_HTTP_CLIENT */

static int cmd_http_get(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_HTTP_CLIENT)
	int ret;
	struct http_client_sh_ctx ctx;

	ctx.sh = sh;
	ret = http_client_init(&ctx, argv[1]);
	if (ret < 0) {
		PR_ERROR("Failed to initialize http_client (%d)\n", ret);
		return ret;
	}

	http_client_setopt(&ctx, HTTP_CLIENT_PROTOCOL, "HTTP/1.1");
	http_client_setopt(&ctx, HTTP_CLIENT_WRITEFUNCTION, http_client_handler);

	ret = http_client_perform(&ctx, HTTP_GET);
	if (ret < 0) {
		PR_ERROR("HTTP GET request failed (%d)\n", ret);
		return ret;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_HTTP_CLIENT", "HTTP client");
#endif

	return 0;
}

static int cmd_http_post(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_HTTP_CLIENT)
	int ret;
	uint16_t payload_size = strlen(argv[2]) + 1;
	struct http_client_sh_ctx ctx;

	ctx.sh = sh;
	ret = http_client_init(&ctx, argv[1]);
	if (ret < 0) {
		PR_ERROR("Failed to initialize http_client (%d)\n", ret);
		return ret;
	}

	http_client_setopt(&ctx, HTTP_CLIENT_PROTOCOL, "HTTP/1.1");
	http_client_setopt(&ctx, HTTP_CLIENT_WRITEFUNCTION, http_client_handler);
	http_client_setopt(&ctx, HTTP_CLIENT_POSTFIELDS, argv[2]);
	http_client_setopt(&ctx, HTTP_CLIENT_POSTFIELDS_SIZE, &payload_size);

	ret = http_client_perform(&ctx, HTTP_POST);
	if (ret < 0) {
		PR_ERROR("HTTP POST request failed (%d)\n", ret);
		return ret;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_HTTP_CLIENT", "HTTP client");
#endif

	return 0;
}

static int cmd_http_put(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_HTTP_CLIENT)
	int ret;
	uint16_t payload_size = strlen(argv[2]) + 1;
	struct http_client_sh_ctx ctx;

	ctx.sh = sh;
	ret = http_client_init(&ctx, argv[1]);
	if (ret < 0) {
		PR_ERROR("Failed to initialize http_client (%d)\n", ret);
		return ret;
	}

	http_client_setopt(&ctx, HTTP_CLIENT_PROTOCOL, "HTTP/1.1");
	http_client_setopt(&ctx, HTTP_CLIENT_WRITEFUNCTION, http_client_handler);
	http_client_setopt(&ctx, HTTP_CLIENT_POSTFIELDS, argv[2]);
	http_client_setopt(&ctx, HTTP_CLIENT_POSTFIELDS_SIZE, &payload_size);

	ret = http_client_perform(&ctx, HTTP_PUT);
	if (ret < 0) {
		PR_ERROR("HTTP PUT request failed (%d)\n", ret);
		return ret;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_HTTP_CLIENT", "HTTP client");
#endif

	return 0;
}

static int cmd_http_delete(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_HTTP_CLIENT)
	int ret;
	struct http_client_sh_ctx ctx;

	ctx.sh = sh;
	ret = http_client_init(&ctx, argv[1]);
	if (ret < 0) {
		PR_ERROR("Failed to initialize http client (%d)\n", ret);
		return ret;
	}

	http_client_setopt(&ctx, HTTP_CLIENT_PROTOCOL, "HTTP/1.1");
	http_client_setopt(&ctx, HTTP_CLIENT_WRITEFUNCTION, http_client_handler);

	ret = http_client_perform(&ctx, HTTP_DELETE);
	if (ret < 0) {
		PR_ERROR("HTTP DELETE request failed (%d)\n", ret);
		return ret;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_HTTP_CLIENT", "HTTP client");
#endif

	return 0;
}

/* clang-format off */
SHELL_SUBCMD_ADD((net, http), get, NULL,
		 SHELL_HELP("Perform HTTP GET request", "<url>"),
		 cmd_http_get, 2, 0);

SHELL_SUBCMD_ADD((net, http), post, NULL,
		 SHELL_HELP("Perform HTTP POST request", "<url> <body>"),
		 cmd_http_post, 3, 0);

SHELL_SUBCMD_ADD((net, http), put, NULL,
		 SHELL_HELP("Perform HTTP PUT request", "<url> <body>"),
		 cmd_http_put, 3, 0);

SHELL_SUBCMD_ADD((net, http), delete, NULL,
		 SHELL_HELP("Perform HTTP DELETE request", "<url>"),
		 cmd_http_delete, 2, 0);
/* clang-format on */
