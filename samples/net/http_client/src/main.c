/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <misc/printk.h>

#include "http_client_types.h"
#include "http_client.h"
#include "config.h"

#define POST_CONTENT_TYPE "application/x-www-form-urlencoded"
#define POST_PAYLOAD "os=ZephyrRTOS&arch="CONFIG_ARCH

#define MAX_ITERATIONS	100

static struct http_client_ctx http_ctx;

static void send_http_method(enum http_method method, const char *url,
			     const char *content_type, const char *payload);

void main(void)
{
	int i = MAX_ITERATIONS;
	int rc;

	printk("Wait for network device to settle...\n");
	k_sleep(K_SECONDS(6));
	http_init(&http_ctx);
	http_ctx.tcp_ctx.receive_cb = http_receive_cb;
	http_ctx.tcp_ctx.timeout = HTTP_NETWORK_TIMEOUT;

	rc = tcp_set_local_addr(&http_ctx.tcp_ctx, LOCAL_ADDR);
	if (rc) {
		printk("tcp_set_local_addr error\n");
		goto lb_exit;
	}

	while (i-- > 0) {
		send_http_method(HTTP_GET, "/index.html", NULL, NULL);
		k_sleep(APP_NAP_TIME);

		send_http_method(HTTP_HEAD, "/", NULL, NULL);
		k_sleep(APP_NAP_TIME);

		send_http_method(HTTP_OPTIONS, "/index.html", NULL, NULL);
		k_sleep(APP_NAP_TIME);

		send_http_method(HTTP_POST, "/post_test.php",
				 POST_CONTENT_TYPE, POST_PAYLOAD);
		k_sleep(APP_NAP_TIME);
	}

lb_exit:
	printk("\nBye!\n");
}

void print_banner(enum http_method method)
{
	printk("\n*******************************************\n"
	       "HTTP Client: %s\nConnecting to: %s port %d\n"
	       "Hostname: %s\nHTTP Request: %s\n",
	       LOCAL_ADDR, SERVER_ADDR, SERVER_PORT,
	       HOST_NAME, http_method_str(method));
}

static
void send_http_method(enum http_method method, const char *url,
		      const char *content_type, const char *payload)
{
	int rc;

	print_banner(method);

	http_reset_ctx(&http_ctx);

	rc = tcp_connect(&http_ctx.tcp_ctx, SERVER_ADDR, SERVER_PORT);
	if (rc) {
		printk("tcp_connect error\n");
		return;
	}

	switch (method) {
	case HTTP_GET:
		rc = http_send_get(&http_ctx, url);
		break;
	case HTTP_POST:
		rc = http_send_post(&http_ctx, url, content_type, payload);
		break;
	case HTTP_HEAD:
		rc = http_send_head(&http_ctx, url);
		break;
	case HTTP_OPTIONS:
		rc = http_send_options(&http_ctx, url, NULL, NULL);
		break;
	default:
		printk("Not yet implemented\n");
		goto lb_exit;
	}

	if (rc) {
		printk("Send error\n");
		goto lb_exit;
	}

	/* this is async, so we wait until the reception callback
	 * processes the server's response (if any)
	 */
	k_sleep(APP_NAP_TIME);

	printk("\nHTTP server response status: %s\n", http_ctx.http_status);

	printk("HTTP parser status: %s\n",
	       http_errno_description(http_ctx.parser.http_errno));

	if (method == HTTP_GET) {
		if (http_ctx.body_found) {
			printk("HTTP body: %u bytes, expected: %u bytes\n",
			       http_ctx.processed, http_ctx.content_length);
		} else {
			printk("Error detected during HTTP msg processing\n");
		}
	}

lb_exit:
	tcp_disconnect(&http_ctx.tcp_ctx);
}
