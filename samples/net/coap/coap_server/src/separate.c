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

static int separate_get(struct coap_resource *resource,
			struct coap_packet *request,
			struct sockaddr *addr, socklen_t addr_len)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t payload[40];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	bool suppress = false;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	if (type == COAP_TYPE_ACK) {
		return 0;
	}

	/* Check if response should be suppressed per RFC 7967 */
	r = coap_no_response_check(request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	if (r < 0 && r != -ENOENT) {
		/* Invalid No-Response option - do not suppress */
		suppress = false;
	}

	/* Always send empty ACK for separate response */
	r = coap_ack_init(&response, request, data, sizeof(data), 0);
	if (r < 0) {
		return r;
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);
	if (r < 0) {
		return r;
	}

	/* If response is suppressed, don't send the separate response */
	if (suppress) {
		return 0;
	}

	/* Response not suppressed, send separate response */
	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_CON;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	/* Re-use the buffer */
	r = coap_packet_init(&response, data, sizeof(data),
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, coap_next_id());
	if (r < 0) {
		return r;
	}

	r = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (r < 0) {
		return r;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		return r;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		return r;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		return r;
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static const char * const separate_path[] = { "separate", NULL };
COAP_RESOURCE_DEFINE(separate, coap_server,
{
	.get = separate_get,
	.path = separate_path,
});
