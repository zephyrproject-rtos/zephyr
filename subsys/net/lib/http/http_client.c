/*
 * Copyright (c) 2019 Unisoc Technologies INC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_http_client, CONFIG_HTTP_CLIENT_LOG_LEVEL);

#include <zephyr.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <version.h>

#include <sys/time.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/http_parser.h>
#include <kernel.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <net/tls_credentials.h>
#include "ca_certificate.h"
#endif

#include "../../ip/net_private.h"
#include <net/http_client.h>

#define HTTP_HOST          "Host"
#define HTTP_CONTENT_TYPE  "Content-Type"
#define HTTP_CONTENT_LEN   "Content-Length"

#define MAX_NUM_DIGITS	16


static int on_status(struct http_parser *parser,
					const char *at, size_t length)
{
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    parser);

	if (length > (HTTP_STATUS_STR_SIZE - 1)) {
		NET_ERR("HTTP response status error, status=%s", at);
		return -1;
	}
	memcpy(ctx->rsp.http_status, at, length);
	ctx->rsp.http_status[length] = 0;

	NET_DBG("HTTP response status %s",
		log_strdup(ctx->rsp.http_status));

	return 0;
}

static int on_body(struct http_parser *parser,
					const char *at, size_t length)
{
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    parser);

	ctx->rsp.body_found = 1;
	ctx->rsp.processed += length;

	NET_DBG("%s: Processed %zd length %zd",
		__func__, ctx->rsp.processed, length);

	if (ctx->rsp_cb) {
		ctx->rsp_cb(ctx,
				 at,
				 length,
				 HTTP_DATA_MORE);
	}

	return 0;
}

static int on_header_field(struct http_parser *parser, const char *at,
			   size_t length)
{
	const char *content_len = HTTP_CONTENT_LEN;
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    parser);
	u16_t len;

	len = strlen(content_len);
	if (length >= len && memcmp(at, content_len, len) == 0) {
		ctx->rsp.cl_present = true;
	}

	if (ctx->fv_cb) {
		ctx->fv_cb(ctx, at, length, HTTP_HEADER_FIELD);
	}

	return 0;
}


static int on_header_value(struct http_parser *parser, const char *at,
			   size_t length)
{
	char str[MAX_NUM_DIGITS];
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    parser);

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
			NET_DBG("http response content_len=%d\n",
				ctx->rsp.content_length);
		}

		ctx->rsp.cl_present = false;
	}

	if (ctx->fv_cb) {
		ctx->fv_cb(ctx, at, length, HTTP_HEADER_VALUE);
	}

	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    parser);

	ctx->rsp.message_complete = 1;

	if (ctx->rsp_cb) {
		ctx->rsp_cb(ctx,
				 NULL,
				 0,
				 HTTP_DATA_FINAL);
	}

	return 0;
}

static void dump_addrinfo(const struct addrinfo *ai)
{
	NET_DBG("addrinfo @%p: ai_family=%d, ai_socktype=%d, ai_protocol=%d, "
	       "sa_family=%d, sin_port=%x\n",
	       ai, ai->ai_family, ai->ai_socktype, ai->ai_protocol,
	       ai->ai_addr->sa_family,
	       ((struct sockaddr_in *)ai->ai_addr)->sin_port);
}

int
http_client_init(struct http_ctx *ctx,
				 char *host,
				 char *port,
				 bool tls)
{
	static struct addrinfo hints;
	struct addrinfo *res, *cur;
	int ret;
	char m_ipaddr[16];
	struct sockaddr_in *addr;

	if (tls) {
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		tls_credential_add(CA_CERTIFICATE_TAG,
					TLS_CREDENTIAL_CA_CERTIFICATE,
					ca_certificate, sizeof(ca_certificate));
#else
		NET_ERR("Please Check CONFIG_HTTPS was Enable or Not!\n");
#endif
	}

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(host, port, &hints, &res);
	NET_DBG("getaddrinfo status: %d\n", ret);

	dump_addrinfo(res);
	if (ret < 0) {
		return ret;
	}

	for (cur = res; cur != NULL; cur = cur->ai_next) {
		addr = (struct sockaddr_in *)cur->ai_addr;
		sprintf(m_ipaddr, "%d.%d.%d.%d",
		(*addr).sin_addr.s4_addr[0],
		(*addr).sin_addr.s4_addr[1],
		(*addr).sin_addr.s4_addr[2],
		(*addr).sin_addr.s4_addr[3]);
		NET_DBG("Get Ip Addr:%s\n", m_ipaddr);
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	if (tls) {
		ctx->sock_id = socket(res->ai_family,
						res->ai_socktype,
						IPPROTO_TLS_1_2);
			sec_tag_t sec_tag_opt[] = {
			CA_CERTIFICATE_TAG,
		};
		setsockopt(ctx->sock_id, SOL_TLS, TLS_SEC_TAG_LIST,
				 sec_tag_opt, sizeof(sec_tag_opt));

		setsockopt(ctx->sock_id, SOL_TLS, TLS_HOSTNAME,
				 ctx->host, strlen(ctx->host));
	} else {
		ctx->sock_id = socket(res->ai_family,
					res->ai_socktype,
					res->ai_protocol);
	}
#else
	{
		ctx->sock_id = socket(res->ai_family,
					res->ai_socktype,
					res->ai_protocol);
	}
#endif

	NET_DBG("http client do connect: ctx->sock_id = %d\n", ctx->sock_id);
	ret = connect(ctx->sock_id, res->ai_addr, res->ai_addrlen);
	NET_DBG("http connect status: %d\n", ret);
	if (ret < 0) {
		ctx->sock_id = 0;
		return ret;
	}

	freeaddrinfo(res);
	strcpy(ctx->host, host);
	return 0;
}

static int http_client_request(struct http_ctx *ctx)
{
	int ret;
	int size = strlen(ctx->req_buf);
	int sent, recved;
	int parsered;

	if (ctx->sock_id <= 0)  {
		NET_ERR("http request, socket error, sock_id=%d", ctx->sock_id);
		return ctx->sock_id;
	}

	sent = 0;
	NET_DBG("http client request start\n");
	while (sent < size) {
		ret = send(ctx->sock_id, ctx->req_buf+sent, size-sent, 0);
		if (ret < 0) {
			NET_ERR("http send error, ret=%d", ret);
			return ret;
		}
		sent += ret;
	}
	/** init the http parse*/
	http_parser_init(&ctx->parser, HTTP_RESPONSE);
	memset(&ctx->parser_settings, 0, sizeof(struct http_parser_settings));
	ctx->parser_settings.on_body = on_body;
	ctx->parser_settings.on_header_field = on_header_field;
	ctx->parser_settings.on_header_value = on_header_value;
	ctx->parser_settings.on_message_complete = on_message_complete;
	ctx->parser_settings.on_status = on_status;

	ctx->rsp_buf = malloc(HTTP_RESPONSE_MAX_SIZE);
	if (ctx->rsp_buf < 0) {
		NET_DBG("Cannot malloc http request buf[ctx->rsp_buf]\n");
		return -2;
	}
	memset(ctx->rsp_buf, 0, HTTP_RESPONSE_MAX_SIZE);
	memset(&ctx->rsp, 0, sizeof(struct http_resp));

	do {
		memset(ctx->rsp_buf, 0, HTTP_RESPONSE_MAX_SIZE);
		recved = recv(ctx->sock_id, ctx->rsp_buf,
					HTTP_RESPONSE_MAX_SIZE-1, 0);
		if (recved < 0) {
			NET_ERR("http recv error, recved=%d", recved);
			ret = recved;
			goto FREE;
		}
		NET_DBG("http client recv response, recved=%d\n", recved);

		parsered = http_parser_execute(&ctx->parser,
						    &ctx->parser_settings,
						    ctx->rsp_buf,
						    recved);

		if (!strcmp(ctx->rsp.http_status, HTTP_STATUS_OK) ||
			!strcmp(ctx->rsp.http_status, HTTP_PARTIAL_CONTENT)) {
			if (ctx->rsp.message_complete != 1) {
				NET_DBG("http recv response continue\n");
				continue;
			} else {
				NET_DBG("http recv response complete\n");
				recved = 0;
			}
		} else {
			NET_ERR("http resp err, http_status=%s",
				ctx->rsp.http_status);
			ret = -1;
			goto FREE;
		}

	} while (recved);

FREE:
	free(ctx->rsp_buf);

	return 0;
}

int http_client_get(struct http_ctx *ctx,
		char *path,
		bool keep_alive,
		void *user_data)
{
	int ret;
	char temp[128];

	if (ctx == NULL) {
		return -1;
	}

	ctx->req_buf = malloc(HTTP_REQUEST_MAX_SIZE);
	if (ctx->req_buf < 0) {
		NET_DBG("Cannot malloc http request buff [ctx->req_buf]");
		return -2;
	}
	memset(ctx->req_buf, 0, HTTP_REQUEST_MAX_SIZE);
	memset(temp, 0, sizeof(temp));

	ctx->req.method_str = http_method_str(HTTP_GET);
	if (path != NULL) {
		if (user_data != NULL) {
			sprintf(temp, "%s?%s", path, (char *)user_data);
		} else {
			sprintf(temp, "%s", path);
		}
	} else {
		if (user_data != NULL) {
			sprintf(temp, "/?%s", (char *)user_data);
		} else {
			temp[0] = '/';
		}
	}

	sprintf(ctx->req_buf, "GET %s HTTP/1.1\r\nHOST: %s\r\n",
			temp, ctx->host);
	memset(temp, 0, sizeof(temp));
	if (keep_alive) {
		sprintf(temp, "Connection: Keep-alive\r\n");
	} else {
		sprintf(temp, "Connection: close\r\n");
	}
	strcat(ctx->req_buf, temp);

	/** add header fields*/
	if (ctx->req.header_fields) {
		strcat(ctx->req_buf, ctx->req.header_fields);
	}

	/**add http end */
	strcat(ctx->req_buf, "\r\n");
	printk("http request size=%d\n", strlen(ctx->req_buf));
	NET_DBG("http client send request:%s\n", ctx->req_buf);
	/**send http request*/
	ret = http_client_request(ctx);

	free(ctx->req_buf);
	return ret;
}

int http_client_post(struct http_ctx *ctx,
		char *path,
		bool keep_alive,
		void *user_data)
{
	int ret;
	char temp[128];

	if (ctx == NULL) {
		return -1;
	}

	ctx->req_buf = malloc(HTTP_REQUEST_MAX_SIZE);
	if (ctx->req_buf < 0) {
		NET_DBG("Cannot malloc http request buff [ctx->req_buf]");
		return -2;
	}
	memset(ctx->req_buf, 0, HTTP_REQUEST_MAX_SIZE);
	memset(temp, 0, sizeof(temp));

	ctx->req.method_str = http_method_str(HTTP_POST);
	if (path != NULL) {
		sprintf(temp, "%s", path);
	} else {
		temp[0] = '/';
	}

	sprintf(ctx->req_buf, "POST %s HTTP/1.1\r\nHOST: %s\r\n",
			temp, ctx->host);
	memset(temp, 0, sizeof(temp));
	if (keep_alive) {
		sprintf(temp, "Connection: Keep-alive\r\n");
	} else {
		sprintf(temp, "Connection: close\r\n");
	}
	strcat(ctx->req_buf, temp);

	/** add header fields*/
	if (ctx->req.header_fields) {
		strcat(ctx->req_buf, ctx->req.header_fields);
	}

	/**add http end */
	strcat(ctx->req_buf, "\r\n");

	/**add post user data*/
	if (user_data != NULL) {
		strcat(ctx->req_buf, (char *)user_data);
	}

	NET_DBG("http client send request:%s\n", ctx->req_buf);
	/**send http request*/
	ret = http_client_request(ctx);

	free(ctx->req_buf);
	return ret;
}

int http_client_close(struct http_ctx *ctx)
{
	if (ctx == NULL) {
		return -1;
	}

	close(ctx->sock_id);

	return 0;
}

void http_add_header_field(struct http_ctx *ctx,
			char *header_fields)
{
	if (ctx == NULL) {
		return;
	}
	ctx->req.header_fields = header_fields;
}

void http_set_resp_callback(struct http_ctx *ctx,
			http_response_cb_t rsp_cb)
{
	if (ctx == NULL) {
		return;
	}

	ctx->rsp_cb = rsp_cb;
}

void http_set_fv_callback(struct http_ctx *ctx,
			http_get_filed_value_cb_t fv_cb)
{
	if (ctx == NULL) {
		return;
	}

	ctx->fv_cb = fv_cb;
}
