/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/http.h>

#include <net/net_pkt.h>
#include <misc/printk.h>

/* HTTP client defines */
#define HTTP_PROTOCOL	   "HTTP/1.1"
#define HTTP_EOF           "\r\n\r\n"

#define HTTP_CONTENT_TYPE  "Content-Type: "
#define HTTP_CONT_LEN_SIZE 64

int http_request(struct net_context *net_ctx, s32_t timeout,
		 struct http_client_request *req)
{
	struct net_pkt *tx;
	int rc = -ENOMEM;

	tx = net_pkt_get_tx(net_ctx, timeout);
	if (!tx) {
		return -ENOMEM;
	}

	if (!net_pkt_append(tx, strlen(req->method), (u8_t *)req->method,
			    timeout)) {
		goto lb_exit;
	}

	if (!net_pkt_append(tx, strlen(req->url), (u8_t *)req->url,
			    timeout)) {
		goto lb_exit;
	}

	if (!net_pkt_append(tx, strlen(req->protocol),
			    (u8_t *)req->protocol, timeout)) {
		goto lb_exit;
	}

	if (!net_pkt_append(tx, strlen(req->header_fields),
			    (u8_t *)req->header_fields,
			    timeout)) {
		goto lb_exit;
	}

	if (req->content_type_value) {
		if (!net_pkt_append(tx, strlen(HTTP_CONTENT_TYPE),
				    (u8_t *)HTTP_CONTENT_TYPE,
				    timeout)) {
			goto lb_exit;
		}

		if (!net_pkt_append(tx, strlen(req->content_type_value),
				    (u8_t *)req->content_type_value,
				    timeout)) {
			goto lb_exit;
		}
	}

	if (req->payload && req->payload_size) {
		char content_len_str[HTTP_CONT_LEN_SIZE];

		rc = snprintk(content_len_str, HTTP_CONT_LEN_SIZE,
			      "\r\nContent-Length: %u\r\n\r\n",
			      req->payload_size);
		if (rc <= 0 || rc >= HTTP_CONT_LEN_SIZE) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		if (!net_pkt_append(tx, rc, (u8_t *)content_len_str,
				    timeout)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		if (!net_pkt_append(tx, req->payload_size,
				    (u8_t *)req->payload,
				    timeout)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

	} else {
		if (!net_pkt_append(tx, strlen(HTTP_EOF),
				    (u8_t *)HTTP_EOF,
				    timeout)) {
			goto lb_exit;
		}
	}

	return net_context_send(tx, NULL, timeout, NULL, NULL);

lb_exit:
	net_pkt_unref(tx);

	return rc;
}

int http_request_get(struct net_context *net_ctx, s32_t timeout, char *url,
		     char *header_fields)
{
	struct http_client_request req = {
				.method = "GET ",
				.url = url,
				.protocol = " "HTTP_PROTOCOL"\r\n",
				.header_fields = header_fields };

	return http_request(net_ctx, timeout, &req);
}

int http_request_head(struct net_context *net_ctx, s32_t timeout, char *url,
		      char *header_fields)
{
	struct http_client_request req = {
				.method = "HEAD ",
				.url = url,
				.protocol = " "HTTP_PROTOCOL"\r\n",
				.header_fields = header_fields };

	return http_request(net_ctx, timeout, &req);
}

int http_request_options(struct net_context *net_ctx, s32_t timeout,
			 char *url, char *header_fields)
{
	struct http_client_request req = {
				.method = "OPTIONS ",
				.url = url,
				.protocol = " "HTTP_PROTOCOL"\r\n",
				.header_fields = header_fields };

	return http_request(net_ctx, timeout, &req);
}

int http_request_post(struct net_context *net_ctx, s32_t timeout, char *url,
		      char *header_fields, char *content_type_value,
		      char *payload)
{
	struct http_client_request req = {
				.method = "POST ",
				.url = url,
				.protocol = " "HTTP_PROTOCOL"\r\n",
				.header_fields = header_fields,
				.content_type_value = content_type_value,
				.payload = payload };

	return http_request(net_ctx, timeout, &req);
}
