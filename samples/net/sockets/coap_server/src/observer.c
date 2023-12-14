/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap_service_sample);

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/coap_service.h>

static int obs_counter;

static void update_counter(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(obs_work, update_counter);

static int send_notification_packet(struct coap_resource *resource,
				    const struct sockaddr *addr,
				    socklen_t addr_len,
				    uint16_t age, uint16_t id,
				    const uint8_t *token, uint8_t tkl,
				    bool is_response)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	char payload[14];
	uint8_t type;
	int r;

	if (is_response) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_CON;
	}

	if (!is_response) {
		id = coap_next_id();
	}

	r = coap_packet_init(&response, data, sizeof(data),
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		return r;
	}

	if (age >= 2U) {
		r = coap_append_option_int(&response, COAP_OPTION_OBSERVE, age);
		if (r < 0) {
			return r;
		}
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
		     "Counter: %d\n", obs_counter);
	if (r < 0) {
		return r;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		return r;
	}

	k_work_reschedule(&obs_work, K_SECONDS(5));

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static int obs_get(struct coap_resource *resource,
		   struct coap_packet *request,
		   struct sockaddr *addr, socklen_t addr_len)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;

	r = coap_resource_parse_observe(resource, request, addr);

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	return send_notification_packet(resource, addr, addr_len,
					r == 0 ? resource->age : 0,
					id, token, tkl, true);
}

static void obs_notify(struct coap_resource *resource,
		       struct coap_observer *observer)
{
	send_notification_packet(resource,
				 &observer->addr,
				 sizeof(observer->addr),
				 resource->age, 0,
				 observer->token, observer->tkl, false);
}

static const char * const obs_path[] = { "obs", NULL };
COAP_RESOURCE_DEFINE(obs, coap_server,
{
	.get = obs_get,
	.path = obs_path,
	.notify = obs_notify,
});

static void update_counter(struct k_work *work)
{
	obs_counter++;

	coap_resource_notify(&obs);

	k_work_reschedule(&obs_work, K_SECONDS(5));
}
