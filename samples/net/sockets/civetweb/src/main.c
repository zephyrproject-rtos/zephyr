/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <posix/pthread.h>
#include <data/json.h>

#include "civetweb.h"

#define HTTP_TEXT_JS		"text/javascript"
#define HTTP_EOFL		"\r\n"  /* http end of line */

#define HTTP_CONTEND_ENCODING	"Content-Encoding: "
#define HTTP_ENCODING_GZ	"gzip"

#define JQUERY_URI	"/jquery-3.5.1.slim.min.js"

#define TX_CHUNK_SIZE_BYTES	CONFIG_NET_TX_STACK_SIZE

#define CIVETWEB_MAIN_THREAD_STACK_SIZE 4096

K_THREAD_STACK_DEFINE(civetweb_stack, CIVETWEB_MAIN_THREAD_STACK_SIZE);

struct civetweb_info {
	const char *version;
	const char *os;
	uint32_t features;
	const char *feature_list;
	const char *build;
	const char *compiler;
	const char *data_model;
};

#define FIELD(struct_, member_, type_) { \
	.field_name = #member_, \
	.field_name_len = sizeof(#member_) - 1, \
	.offset = offsetof(struct_, member_), \
	.type = type_ \
}

int this_send_buffer_chunked(struct mg_connection *conn, const char *mime_type,
			     const char *buff, const size_t buff_len)
{
	/* TODO: Add ret after each function call! */
	int ret = 0;

	ret = mg_send_http_ok(conn, mime_type, -1);
	if (ret < 0) {
		goto error_this_send_buffer_chunked;
	}

	long long left_bytes = buff_len;
	char *itr = (char *)buff;  /* buffer iterator */

	LOG_DBG("Transferring:");
	LOG_DBG("itr: 0x%08X ret: %d left_bytes: %lld chunk_size: %zd B",
		(unsigned int)itr, ret, left_bytes, TX_CHUNK_SIZE_BYTES);

	while (left_bytes > TX_CHUNK_SIZE_BYTES) {
		ret = mg_send_chunk(conn, itr, TX_CHUNK_SIZE_BYTES);
		itr += TX_CHUNK_SIZE_BYTES;
		left_bytes -= TX_CHUNK_SIZE_BYTES;

		LOG_DBG("itr: 0x%08X ret: %d left_bytes: %lld",
			(unsigned int)itr, ret, left_bytes);

		if (ret < 0) {
			goto error_this_send_buffer_chunked;
		}
	}

	if (left_bytes > 0) {
		ret = mg_send_chunk(conn, itr, left_bytes);
		itr += left_bytes;
		left_bytes = 0;

		LOG_DBG("itr: 0x%08X ret: %d left_bytes: %lld",
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

void send_ok(struct mg_connection *conn)
{
	mg_printf(conn,
		  "HTTP/1.1 200 OK\r\n"
		  "Content-Type: text/html\r\n"
		  "Connection: close\r\n\r\n");
}

int hello_world_handler(struct mg_connection *conn, void *cbdata)
{
	send_ok(conn);
	mg_printf(conn, "<html><body>");
	mg_printf(conn, "<h3>Hello World from Zephyr!</h3>");
	mg_printf(conn, "See also:\n");
	mg_printf(conn, "<ul>\n");
	mg_printf(conn, "<li><a href=/info>system info</a></li>\n");
	mg_printf(conn, "<li><a href=/history>cookie demo</a></li>\n");
	mg_printf(conn, "</ul>\n");
	mg_printf(conn, "</body></html>\n");

	return 200;
}

int system_info_handler(struct mg_connection *conn, void *cbdata)
{
	static const struct json_obj_descr descr[] = {
		FIELD(struct civetweb_info, version, JSON_TOK_STRING),
		FIELD(struct civetweb_info, os, JSON_TOK_STRING),
		FIELD(struct civetweb_info, feature_list, JSON_TOK_STRING),
		FIELD(struct civetweb_info, build, JSON_TOK_STRING),
		FIELD(struct civetweb_info, compiler, JSON_TOK_STRING),
		FIELD(struct civetweb_info, data_model, JSON_TOK_STRING),
	};

	struct civetweb_info info = {};
	char info_str[1024] = {};
	int ret;
	int size;

	size = mg_get_system_info(info_str, sizeof(info_str));

	ret = json_obj_parse(info_str, size, descr, ARRAY_SIZE(descr), &info);

	send_ok(conn);

	if (ret < 0) {
		mg_printf(conn, "Could not retrieve: %d\n", ret);
		return 500;
	}


	mg_printf(conn, "<html><body>");

	mg_printf(conn, "<h3>Server info</h3>");
	mg_printf(conn, "<ul>\n");
	mg_printf(conn, "<li>host os - %s</li>\n", info.os);
	mg_printf(conn, "<li>server - civetweb %s</li>\n", info.version);
	mg_printf(conn, "<li>compiler - %s</li>\n", info.compiler);
	mg_printf(conn, "<li>board - %s</li>\n", CONFIG_BOARD);
	mg_printf(conn, "</ul>\n");

	mg_printf(conn, "</body></html>\n");

	return 200;
}

int history_handler(struct mg_connection *conn, void *cbdata)
{
	const struct mg_request_info *req_info = mg_get_request_info(conn);
	const char *cookie = mg_get_header(conn, "Cookie");
	char history_str[64];

	mg_get_cookie(cookie, "history", history_str, sizeof(history_str));

	mg_printf(conn, "HTTP/1.1 200 OK\r\n");
	mg_printf(conn, "Connection: close\r\n");
	mg_printf(conn, "Set-Cookie: history='%s'\r\n", req_info->local_uri);
	mg_printf(conn, "Content-Type: text/html\r\n\r\n");

	mg_printf(conn, "<html><body>");

	mg_printf(conn, "<h3>Your URI is: %s<h3>\n", req_info->local_uri);

	if (history_str[0] == 0) {
		mg_printf(conn, "<h5>This is your first visit.</h5>\n");
	} else {
		mg_printf(conn, "<h5>your last /history visit was: %s</h5>\n",
			  history_str);
	}

	mg_printf(conn, "Some cookie-saving links to try:\n");
	mg_printf(conn, "<ul>\n");
	mg_printf(conn, "<li><a href=/history/first>first</a></li>\n");
	mg_printf(conn, "<li><a href=/history/second>second</a></li>\n");
	mg_printf(conn, "<li><a href=/history/third>third</a></li>\n");
	mg_printf(conn, "<li><a href=/history/fourth>fourth</a></li>\n");
	mg_printf(conn, "<li><a href=/history/fifth>fifth</a></li>\n");
	mg_printf(conn, "</ul>\n");

	mg_printf(conn, "</body></html>\n");

	return 200;
}

void this_set_return_value(int *ret_val)
{
	if (*ret_val < 0) {
		*ret_val = 404;  /* 404 - HTTP FAIL or 0 - handler fail */
	} else {
		*ret_val = 200;  /* 200 - HTTP OK*/
	}
}

int jquery_handler(struct mg_connection *conn, void *cbdata)
{
	static const char jquery_js[] = {
#include "web_page/jquery-3.5.1.slim.min.js.gz.inc"
	};

	int ret = 0;

	ret = this_send_buffer_chunked(conn, HTTP_TEXT_JS
					     HTTP_EOFL
					     HTTP_CONTEND_ENCODING
					     HTTP_ENCODING_GZ,
					     jquery_js, sizeof(jquery_js));
	this_set_return_value(&ret);
	return ret;
}

void *main_pthread(void *arg)
{
	static const char * const options[] = {
		"listening_ports",
		"8080",
		"num_threads",
		"1",
		"max_request_size",
		"2048",
		0
	};

	struct mg_callbacks callbacks;
	struct mg_context *ctx;

	(void)arg;

	memset(&callbacks, 0, sizeof(callbacks));
	ctx = mg_start(&callbacks, 0, (const char **)options);

	if (ctx == NULL) {
		printf("Unable to start the server.");
		return 0;
	}

	mg_set_request_handler(ctx, "/$", hello_world_handler, 0);
	mg_set_request_handler(ctx, "/info$", system_info_handler, 0);
	mg_set_request_handler(ctx, "/history", history_handler, 0);
	mg_set_request_handler(ctx, JQUERY_URI, jquery_handler, 0);

	return 0;
}

void main(void)
{
	pthread_attr_t civetweb_attr;
	pthread_t civetweb_thread;

	(void)pthread_attr_init(&civetweb_attr);
	(void)pthread_attr_setstack(&civetweb_attr, &civetweb_stack,
				    CIVETWEB_MAIN_THREAD_STACK_SIZE);

	(void)pthread_create(&civetweb_thread, &civetweb_attr,
			     &main_pthread, 0);
}
