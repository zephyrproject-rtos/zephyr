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

static int location_query_post(struct coap_resource *resource,
			       struct coap_packet *request,
			       struct sockaddr *addr, socklen_t addr_len)
{
	static const char *const location_query[] = { "first=1",
						      "second=2",
						      NULL };
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	const char * const *p;
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	r = coap_packet_init(&response, data, sizeof(data),
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CREATED, id);
	if (r < 0) {
		return r;
	}

	for (p = location_query; *p; p++) {
		r = coap_packet_append_option(&response,
					      COAP_OPTION_LOCATION_QUERY,
					      *p, strlen(*p));
		if (r < 0) {
			return r;
		}
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static const char * const location_query_path[] = { "location-query", NULL };
COAP_RESOURCE_DEFINE(location_query, coap_server,
{
	.post = location_query_post,
	.path = location_query_path,
});
