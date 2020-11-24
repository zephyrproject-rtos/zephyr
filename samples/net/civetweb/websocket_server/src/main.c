/*
 * Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <posix/pthread.h>

#include "civetweb.h"

#include "http_server_handlers.h"
#include "websocket_server_handlers.h"

#define HTTP_PORT	8080
#define HTTPS_PORT	4443

#define CIVETWEB_MAIN_THREAD_STACK_SIZE		CONFIG_MAIN_STACK_SIZE

/* Use smallest possible value of 1024 (see the line 18619 of civetweb.c) */
#define MAX_REQUEST_SIZE_BYTES			1024

K_THREAD_STACK_DEFINE(civetweb_stack, CIVETWEB_MAIN_THREAD_STACK_SIZE);

void *main_pthread(void *arg)
{
	static const char * const options[] = {
		"listening_ports", STRINGIFY(HTTP_PORT),
		"num_threads", "1",
		"max_request_size", STRINGIFY(MAX_REQUEST_SIZE_BYTES),
		NULL
	};

	struct mg_callbacks callbacks;
	struct mg_context *ctx;

	(void)arg;

	memset(&callbacks, 0, sizeof(callbacks));
	ctx = mg_start(&callbacks, NULL, (const char **)options);

	if (ctx == NULL) {
		LOG_ERR("Unable to start the server\n");
		return 0;
	}

	init_http_server_handlers(ctx);
	init_websocket_server_handlers(ctx);

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

	LOG_INF("WebSocket Server was started!");
}
