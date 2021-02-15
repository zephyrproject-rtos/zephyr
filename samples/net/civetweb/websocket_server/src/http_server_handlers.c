/*
 * Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(http_server_handlers, LOG_LEVEL_DBG);

#include "http_server_handlers.h"

#define TX_CHUNK_SIZE_BYTES	CONFIG_NET_TX_STACK_SIZE

#define URL_MAIN		"/$"
#define URL_INDEX_CSS		"/index.css"
#define URL_INDEX_HTML		"/index.html"
#define URL_FAVICON_ICO		"/favicon.ico"
#define URL_WS_JS		"/ws.js"

#define HTTP_TEXT_HTML		"text/html"
#define HTTP_TEXT_CSS		"text/css"
#define HTTP_TEXT_JS		"text/javascript"
#define HTTP_EOFL		"\r\n"  /* http end of line */

#define HTTP_CONTEND_ENCODING	"Content-Encoding: "
#define HTTP_ENCODING_GZ	"gzip"

#define __code_decl	/* static */
#define __data_decl	static

__code_decl void this_register_handlers(struct mg_context *ctx);

__code_decl int this_send_buffer_chunked(struct mg_connection *conn,
					 const char *mime_type,
					 const char *buff,
					 const size_t buff_len);

__code_decl int this_redirect_2_index_html(struct mg_connection *conn,
					   void *cbdata);
__code_decl int this_index_html_handler(struct mg_connection *conn,
					void *cbdata);
__code_decl int this_index_css_handler(struct mg_connection *conn,
					void *cbdata);
__code_decl int this_ws_js_handler(struct mg_connection *conn,
				   void *cbdata);
__code_decl int this_favicon_ico_handler(struct mg_connection *conn,
					 void *cbdata);

__code_decl void this_set_return_value(int *ret_val);

void init_http_server_handlers(struct mg_context *ctx)
{
	this_register_handlers(ctx);
}

__code_decl void this_register_handlers(struct mg_context *ctx)
{
	mg_set_request_handler(ctx, URL_MAIN,
				this_redirect_2_index_html, NULL);
	mg_set_request_handler(ctx, URL_INDEX_HTML,
				this_index_html_handler, NULL);
	mg_set_request_handler(ctx, URL_INDEX_CSS,
				this_index_css_handler, NULL);
	mg_set_request_handler(ctx, URL_WS_JS,
				this_ws_js_handler, NULL);
	mg_set_request_handler(ctx, URL_FAVICON_ICO,
				this_favicon_ico_handler, NULL);
}

__code_decl int this_send_buffer_chunked(struct mg_connection *conn,
					 const char *mime_type,
					 const char *buff,
					 const size_t buff_len)
{
	int ret = 0;

	ret = mg_send_http_ok(conn, mime_type, -1);
	if (ret < 0) {
		goto error_this_send_buffer_chunked;
	}

	long left_bytes = buff_len;
	char *itr = (char *)buff;  /* buffer iterator */

	LOG_DBG("Transferring:");
	LOG_DBG("itr: 0x%08X ret: %d left_bytes: %ld chunk_size: %zd B",
		(unsigned int)itr, ret, left_bytes, TX_CHUNK_SIZE_BYTES);

	while (left_bytes > TX_CHUNK_SIZE_BYTES) {
		ret = mg_send_chunk(conn, itr, TX_CHUNK_SIZE_BYTES);
		itr += TX_CHUNK_SIZE_BYTES;
		left_bytes -= TX_CHUNK_SIZE_BYTES;

		LOG_DBG("itr: 0x%08X ret: %d left_bytes: %ld",
			(unsigned int)itr, ret, left_bytes);

		if (ret < 0) {
			goto error_this_send_buffer_chunked;
		}
	}

	if (left_bytes > 0) {
		ret = mg_send_chunk(conn, itr, left_bytes);
		itr += left_bytes;
		left_bytes = 0;

		LOG_DBG("itr: 0x%08X ret: %d left_bytes: %ld",
			(unsigned int)itr, ret, left_bytes);

		if (ret < 0) {
			goto error_this_send_buffer_chunked;
		}
	}

	/* Must be sent at the end of the chuked sequence */
	ret = mg_send_chunk(conn, "", 0);

error_this_send_buffer_chunked:
	if (ret < 0) {
		LOG_ERR("aborted! ret: %d", ret);
	}

	return ret;
}

__code_decl int this_redirect_2_index_html(struct mg_connection *conn,
					   void *cbdata)
{
	int ret = 0;

	ret = mg_send_http_redirect(conn, URL_INDEX_HTML, 303);
	return 202;
}

__code_decl int this_index_html_handler(struct mg_connection *conn,
					void *cbdata)
{
	__data_decl const char index_html[] = {
#include "web_page/index.html.gz.inc"
	};

	int ret = 0;

	ret = this_send_buffer_chunked(conn, HTTP_TEXT_HTML
					     HTTP_EOFL
					     HTTP_CONTEND_ENCODING
					     HTTP_ENCODING_GZ,
					     index_html, sizeof(index_html));
	this_set_return_value(&ret);
	return ret;
}

__code_decl int this_index_css_handler(struct mg_connection *conn, void *cbdata)
{
	__data_decl const char index_css[] = {
#include "web_page/index.css.gz.inc"
	};

	int ret = 0;

	ret = this_send_buffer_chunked(conn, HTTP_TEXT_CSS
					     HTTP_EOFL
					     HTTP_CONTEND_ENCODING
					     HTTP_ENCODING_GZ,
					     index_css, sizeof(index_css));
	this_set_return_value(&ret);
	return ret;
}

__code_decl int this_ws_js_handler(struct mg_connection *conn, void *cbdata)
{
	__data_decl const char ws_js[] = {
#include "web_page/ws.js.gz.inc"
	};

	int ret = 0;

	ret = this_send_buffer_chunked(conn, HTTP_TEXT_JS
					     HTTP_EOFL
					     HTTP_CONTEND_ENCODING
					     HTTP_ENCODING_GZ,
					     ws_js, sizeof(ws_js));
	this_set_return_value(&ret);
	return ret;
}

__code_decl int this_favicon_ico_handler(struct mg_connection *conn,
					 void *cbdata)
{
	int ret = 404;
	return ret;  /* should fail */
}

__code_decl void this_set_return_value(int *ret_val)
{
	if (*ret_val < 0) {
		*ret_val = 404;  /* 404 - HTTP FAIL or 0 - handler fail */
	} else {
		*ret_val = 200;  /* 200 - HTTP OK*/
	}
}
