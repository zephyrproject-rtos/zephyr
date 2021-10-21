/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wasm_app.h"
#include "wa-inc/request.h"

static void url1_request_handler(request_t *request)
{
	response_t response[1];
	attr_container_t *payload;

	printf("[resp] ### user resource 1 handler called\n");

	if (request->payload != NULL && request->fmt == FMT_ATTR_CONTAINER)
		attr_container_dump((attr_container_t *) request->payload);

	payload = attr_container_create("wasm app response payload");
	if (payload == NULL)
		return;

	attr_container_set_string(&payload, "key1", "value1");
	attr_container_set_string(&payload, "key2", "value2");

	make_response_for_request(request, response);
	set_response(response, CONTENT_2_05,
				 FMT_ATTR_CONTAINER,
				 (void *)payload,
				 attr_container_get_serialize_length(payload));
	api_response_send(response);

	attr_container_destroy(payload);
}

static void url2_request_handler(request_t *request)
{
	response_t response[1];

	make_response_for_request(request, response);
	set_response(response, DELETED_2_02, 0, NULL, 0);
	api_response_send(response);

	printf("### user resource 2 handler called\n");
}

void on_init(void)
{
	/* register resource uri */
	api_register_resource_handler("/url1", url1_request_handler);
	api_register_resource_handler("/url2", url2_request_handler);
}

void on_destroy(void)
{
	/**
	 * real destroy work including killing timer and closing sensor is
	 * accomplished in wasm app library version of on_destroy()
	 */
}
