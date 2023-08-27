/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>

static uint16_t test_http_service_port = htons(CONFIG_NET_HTTP_SERVER_SERVICE_PORT);
HTTP_SERVICE_DEFINE(test_http_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &test_http_service_port, 1,
		    10, NULL);

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = GET,
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, test_http_service, "/",
		     &index_html_gz_resource_detail);

struct http_resource_detail_rest add_two_numbers_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_REST,
			.bitmask_of_supported_http_methods = POST,
		},
};

HTTP_RESOURCE_DEFINE(add_two_numbers, test_http_service, "/add", &add_two_numbers_detail);

int main(void)
{
	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	if (server_fd < 0) {
		printf("Failed to initialize HTTP2 server\n");
		return -1;
	}

	http_server_start(&ctx);

	return 0;
}
