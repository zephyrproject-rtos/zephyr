/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wasm_app.h"
#include "wa-inc/request.h"

static void my_response_handler(response_t *response, void *user_data)
{
	char *tag = (char *) user_data;

	if (response == NULL) {
		printf("[req] request timeout!\n");
		return;
	}

	printf("[req] response handler called mid:%d, status:%d, "
		   "fmt:%d, payload:%p, len:%d, tag:%s\n",
		   response->mid, response->status,
		   response->fmt, response->payload,
		   response->payload_len, tag);

	if (response->payload != NULL
		&& response->payload_len > 0
		&& response->fmt == FMT_ATTR_CONTAINER) {
		printf("[req] dump the response payload:\n");
		attr_container_dump((attr_container_t *) response->payload);
	}
}

static void test_send_request(char *url, char *tag)
{
	request_t request[1];

	init_request(request, url, COAP_PUT, 0, NULL, 0);
	api_send_request(request, my_response_handler, tag);
}

void on_init(void)
{
	test_send_request("/app/request_handler/url1",
					  "a request to target app");
	test_send_request("url1", "a general request");
}

void on_destroy(void)
{
	/**
	 * real destroy work including killing timer and closing sensor is
	 * accomplished in wasm app library version of on_destroy()
	 */
}
