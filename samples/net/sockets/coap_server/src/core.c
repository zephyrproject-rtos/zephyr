/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap_service_sample);

#include <zephyr/sys/printk.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/net/coap_link_format.h>

static int core_get(struct coap_resource *resource,
		    struct coap_packet *request,
		    struct sockaddr *addr, socklen_t addr_len)
{
	static const char dummy_str[] = "Just a test\n";
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t tkl;
	int r;

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	r = coap_packet_init(&response, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		return r;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		return r;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)dummy_str,
				       sizeof(dummy_str));
	if (r < 0) {
		return r;
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static const char * const core_1_path[] = { "core1", NULL };
static const char * const core_1_attributes[] = {
	"title=\"Core 1\"",
	"rt=core1",
	NULL,
};
COAP_RESOURCE_DEFINE(core_1, coap_server,
{
	.get = core_get,
	.path = core_1_path,
	.user_data = &((struct coap_core_metadata) {
		.attributes = core_1_attributes,
	}),
});

static const char * const core_2_path[] = { "core2", NULL };
static const char * const core_2_attributes[] = {
	"title=\"Core 2\"",
	"rt=core2",
	NULL,
};
COAP_RESOURCE_DEFINE(core_2, coap_server,
{
	.get = core_get,
	.path = core_2_path,
	.user_data = &((struct coap_core_metadata) {
		.attributes = core_2_attributes,
	}),
});
