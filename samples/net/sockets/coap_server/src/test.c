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

#include "net_private.h"

static int piggyback_get(struct coap_resource *resource,
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
			     COAP_RESPONSE_CODE_CONTENT, id);
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

static int test_del(struct coap_resource *resource,
		    struct coap_packet *request,
		    struct sockaddr *addr, socklen_t addr_len)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;
	uint8_t code;
	uint8_t type;
	uint16_t id;
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
			     COAP_RESPONSE_CODE_DELETED, id);
	if (r < 0) {
		return r;
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static int test_put(struct coap_resource *resource,
		    struct coap_packet *request,
		    struct sockaddr *addr, socklen_t addr_len)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	const uint8_t *payload;
	uint16_t payload_len;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	uint16_t id;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	payload = coap_packet_get_payload(request, &payload_len);
	if (payload) {
		net_hexdump("PUT Payload", payload, payload_len);
	}

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	r = coap_packet_init(&response, data, sizeof(data),
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CHANGED, id);
	if (r < 0) {
		return r;
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static int test_post(struct coap_resource *resource,
		     struct coap_packet *request,
		     struct sockaddr *addr, socklen_t addr_len)
{
	static const char * const location_path[] = { "location1",
						      "location2",
						      "location3",
						      NULL };
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	const char * const *p;
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	const uint8_t *payload;
	uint16_t payload_len;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	uint16_t id;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	payload = coap_packet_get_payload(request, &payload_len);
	if (payload) {
		net_hexdump("POST Payload", payload, payload_len);
	}

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

	for (p = location_path; *p; p++) {
		r = coap_packet_append_option(&response,
					      COAP_OPTION_LOCATION_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			return r;
		}
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static const char * const test_path[] = { "test", NULL };
COAP_RESOURCE_DEFINE(test, coap_server,
{
	.get = piggyback_get,
	.post = test_post,
	.del = test_del,
	.put = test_put,
	.path = test_path,
});

static const char * const segments_path[] = { "seg1", "seg2", "seg3", NULL };
COAP_RESOURCE_DEFINE(segments, coap_server,
{
	.get = piggyback_get,
	.path = segments_path,
});

#if defined(CONFIG_COAP_URI_WILDCARD)

static const char * const wildcard1_path[] = { "wild1", "+", "wild3", NULL };
COAP_RESOURCE_DEFINE(wildcard1, coap_server,
{
	.get = piggyback_get,
	.path = wildcard1_path,
});

static const char * const wildcard2_path[] = { "wild2", "#", NULL };
COAP_RESOURCE_DEFINE(wildcard2, coap_server,
{
	.get = piggyback_get,
	.path = wildcard2_path,
});

#endif
