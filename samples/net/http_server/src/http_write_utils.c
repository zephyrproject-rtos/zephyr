/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_write_utils.h"
#include "http_types.h"
#include "http_utils.h"

#include <net/nbuf.h>
#include <stdio.h>

uint16_t http_strlen(const char *str)
{
	if (str) {
		return strlen(str);
	}

	return 0;
}

int http_add_header(struct net_buf *tx, const char *str)
{
	net_nbuf_append(tx, strlen(str), (uint8_t *)str, K_FOREVER);

	return 0;
}

int http_add_chunk(struct net_buf *tx, const char *str)
{
	char chunk_header[16];
	char *rn = "\r\n";
	uint16_t str_len;

	str_len = http_strlen(str);

	snprintf(chunk_header, sizeof(chunk_header), "%x\r\n", str_len);
	net_nbuf_append(tx, strlen(chunk_header), chunk_header, K_FOREVER);

	if (str_len) {
		net_nbuf_append(tx, str_len, (uint8_t *)str, K_FOREVER);
	}

	net_nbuf_append(tx, strlen(rn), rn, K_FOREVER);

	return 0;
}

int http_write(struct http_server_ctx *ctx, const char *http_header,
	       const char *html_header, const char *html_body,
	       const char *html_footer)
{
	struct net_buf *tx;
	int rc = -EINVAL;

	tx = net_nbuf_get_tx(ctx->net_ctx, K_FOREVER);
	printf("[%s:%d] net_nbuf_get_tx, rc: %d <%s>\n",
	       __func__, __LINE__, !tx, RC_STR(!tx));
	if (tx == NULL) {
		goto exit_routine;
	}

	http_add_header(tx, http_header);
	if (html_header) {
		http_add_chunk(tx, html_header);
	}

	if (html_body) {
		http_add_chunk(tx, html_body);
	}

	if (html_footer) {
		http_add_chunk(tx, html_footer);
	}

	/* like EOF */
	http_add_chunk(tx, NULL);

	rc = net_context_send(tx, NULL, 0, NULL, NULL);
	printf("[%s:%d] net_context_send: %d <%s>\n",
	       __func__, __LINE__, rc, RC_STR(rc));
	if (rc != 0) {
		goto exit_routine;
	}

	tx = NULL;

exit_routine:
	/* unref can handle NULL buffers, so we are covered */
	net_nbuf_unref(tx);

	return rc;
}

/* Prints the received HTTP header fields as an HTML list */
static void print_http_headers(struct http_server_ctx *ctx,
			       char *str, uint16_t size)
{
	struct http_parser *parser = &ctx->parser;
	uint16_t offset = 0;

	offset = snprintf(str, size,
			  "<body><h1>Zephyr HTTP server</h1>"
			  "<h2>HTTP Header Fields</h2>\r\n<ul>\r\n");
	if (offset >= size) {
		return;
	}

	for (uint8_t i = 0; i < ctx->field_values_ctr; i++) {
		struct http_field_value *kv = &ctx->field_values[i];

		offset += snprintf(str + offset, size - offset,
				   "<li>%.*s: %.*s</li>\r\n",
				   kv->key_len, kv->key,
				   kv->value_len, kv->value);
		if (offset >= size) {
			return;
		}
	}

	offset += snprintf(str + offset, size - offset, "</ul>\r\n");
	if (offset >= size) {
		return;
	}

	offset += snprintf(str + offset, size - offset,
			   "<h2>HTTP Method: %s</h2>\r\n",
			   http_method_str(parser->method));
	if (offset >= size) {
		return;
	}

	offset += snprintf(str + offset, size - offset,
			   "<h2>URL: %.*s</h2>\r\n",
			   ctx->url_len, ctx->url);
	if (offset >= size) {
		return;
	}

	snprintf(str + offset, size - offset,
		 "<h2>Server: %s</h2></body>\r\n", CONFIG_ARCH);
}

#define HTTP_MAX_BODY_STR_SIZE		512
static char html_body[HTTP_MAX_BODY_STR_SIZE];

int http_write_header_fields(struct http_server_ctx *ctx)
{
	print_http_headers(ctx, html_body, HTTP_MAX_BODY_STR_SIZE);

	return http_write(ctx, HTTP_STATUS_200_OK, HTML_STR_HEADER,
			  html_body, HTML_STR_FOOTER);
}

int http_write_it_works(struct http_server_ctx *ctx)
{
	return http_write(ctx, HTTP_STATUS_200_OK, HTML_STR_HEADER,
			  "<body><h1><center>It Works!</center></h1>"
			  "</body>\r\n", HTML_STR_FOOTER);
}

int http_write_400_bad_request(struct http_server_ctx *ctx)
{
	return http_write(ctx, HTTP_STATUS_400_BR, NULL, NULL, NULL);
}

int http_write_403_forbidden(struct http_server_ctx *ctx)
{
	return http_write(ctx, HTTP_STATUS_403_FBD, NULL, NULL, NULL);
}

int http_write_404_not_found(struct http_server_ctx *ctx)
{
	return http_write(ctx, HTTP_STATUS_404_NF, NULL, NULL, NULL);
}

int http_write_soft_404_not_found(struct http_server_ctx *ctx)
{
	return http_write(ctx, HTTP_STATUS_200_OK, HTML_STR_HEADER,
			  "<body><h1><center>404 Not Found</center></h1>"
			  "</body>\r\n", HTML_STR_FOOTER);
}
