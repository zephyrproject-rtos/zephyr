/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the CoAP conformance suite.
 *
 * The ETSI derived test cases the suite runs address three resources. /test
 * answers the four methods with Content, Created, Changed and Deleted. /large
 * is bigger than one block, so a GET of it is a block-wise transfer. /obs can
 * be observed, and changes every couple of seconds so that an observer has
 * something to be told. That is the whole of what is needed here, so rather
 * than pulling in the CoAP server sample and its resources this defines just
 * those three.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/coap.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(coap_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/net/net_if.h>

static uint16_t coap_port = 5683;

/* Bound to the IPv4 address rather than to the wildcard. A service given no
 * host prefers IPv6 whenever both families are built, and then only sees IPv4
 * traffic if v4 mapped addresses happen to be turned on. Naming the address
 * makes it deterministic, and the suite addresses the server over IPv4.
 */
COAP_SERVICE_DEFINE(conformance_server, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &coap_port,
		    COAP_SERVICE_AUTOSTART);

/* A response echoes the request's token and message id, and is acknowledged
 * only if the request asked to be. A non confirmable request is answered with
 * a non confirmable response, which is what RFC 7252 4.3 calls for and what
 * the ETSI cases check.
 */
static int respond(struct coap_resource *resource, struct coap_packet *request,
		   struct net_sockaddr *addr, net_socklen_t addr_len,
		   uint8_t response_code, const char *payload)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	struct coap_packet response;
	uint16_t id;
	uint8_t type;
	uint8_t tkl;
	int ret;

	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	type = (type == COAP_TYPE_CON) ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1,
			       type, tkl, token, response_code, id);
	if (ret < 0) {
		return ret;
	}

	if (payload != NULL) {
		ret = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
					     COAP_CONTENT_FORMAT_TEXT_PLAIN);
		if (ret < 0) {
			return ret;
		}

		ret = coap_packet_append_payload_marker(&response);
		if (ret < 0) {
			return ret;
		}

		ret = coap_packet_append_payload(&response, (const uint8_t *)payload,
						 strlen(payload));
		if (ret < 0) {
			return ret;
		}
	}

	return coap_resource_send(resource, &response, addr, addr_len, NULL);
}

static int test_get(struct coap_resource *resource, struct coap_packet *request,
		    struct net_sockaddr *addr, net_socklen_t addr_len)
{
	return respond(resource, request, addr, addr_len,
		       COAP_RESPONSE_CODE_CONTENT, "conformance");
}

static int test_post(struct coap_resource *resource, struct coap_packet *request,
		     struct net_sockaddr *addr, net_socklen_t addr_len)
{
	return respond(resource, request, addr, addr_len,
		       COAP_RESPONSE_CODE_CREATED, NULL);
}

static int test_put(struct coap_resource *resource, struct coap_packet *request,
		    struct net_sockaddr *addr, net_socklen_t addr_len)
{
	return respond(resource, request, addr, addr_len,
		       COAP_RESPONSE_CODE_CHANGED, NULL);
}

static int test_delete(struct coap_resource *resource, struct coap_packet *request,
		       struct net_sockaddr *addr, net_socklen_t addr_len)
{
	return respond(resource, request, addr, addr_len,
		       COAP_RESPONSE_CODE_DELETED, NULL);
}

static const char * const test_path[] = { "test", NULL };

COAP_RESOURCE_DEFINE(test_resource, conformance_server, {
	.get = test_get,
	.post = test_post,
	.put = test_put,
	.del = test_delete,
	.path = test_path,
});

/* Bigger than any block the server sends, so that a GET is a transfer of
 * several blocks rather than one message. Five of the 64 byte blocks the
 * suite asks for; the content is a repeating pattern, so that any block can
 * be checked against its offset.
 */
#define LARGE_SIZE 320

static uint8_t large_body[LARGE_SIZE];

static int large_get(struct coap_resource *resource, struct coap_packet *request,
		     struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	static struct coap_block_context ctx;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	struct coap_packet response;
	uint16_t size;
	uint16_t id;
	uint8_t type;
	uint8_t tkl;
	int ret;

	/* One transfer at a time. The context carries over from one block
	 * request to the next and is cleared by the last, so the next GET
	 * starts a new transfer. The block size is the smaller of the server's
	 * own and the one the client asks for.
	 */
	if (ctx.total_size == 0) {
		ret = coap_block_transfer_init(
			&ctx, coap_bytes_to_block_size(CONFIG_COAP_SERVER_BLOCK_SIZE),
			LARGE_SIZE);
		if (ret < 0) {
			return ret;
		}
	}

	ret = coap_update_from_block(request, &ctx);
	if (ret < 0) {
		return ret;
	}

	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	type = (type == COAP_TYPE_CON) ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, type, tkl,
			       token, COAP_RESPONSE_CODE_CONTENT, id);
	if (ret < 0) {
		return ret;
	}

	ret = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (ret < 0) {
		return ret;
	}

	ret = coap_append_block2_option(&response, &ctx);
	if (ret < 0) {
		return ret;
	}

	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		return ret;
	}

	size = MIN(coap_block_size_to_bytes(ctx.block_size), ctx.total_size - ctx.current);

	ret = coap_packet_append_payload(&response, &large_body[ctx.current], size);
	if (ret < 0) {
		return ret;
	}

	/* Zero means this was the last block. */
	if (coap_next_block(&response, &ctx) == 0) {
		memset(&ctx, 0, sizeof(ctx));
	}

	return coap_resource_send(resource, &response, addr, addr_len, NULL);
}

static const char * const large_path[] = { "large", NULL };

COAP_RESOURCE_DEFINE(large_resource, conformance_server, {
	.get = large_get,
	.path = large_path,
});

/* How often the observed resource changes. Shorter than the time the suite
 * waits for a notification, longer than a round trip.
 */
#define OBS_INTERVAL K_SECONDS(2)

static int obs_counter;

static void obs_update(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(obs_work, obs_update);

/* The initial response and every notification look the same apart from the
 * message type and id: the current state, with the Observe option carrying
 * the resource's sequence number when the peer is an observer.
 */
static int obs_send(struct coap_resource *resource, const struct net_sockaddr *addr,
		    net_socklen_t addr_len, uint8_t type, uint16_t id,
		    const uint8_t *token, uint8_t tkl, bool observed)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet response;
	char payload[32];
	int ret;

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, type, tkl,
			       token, COAP_RESPONSE_CODE_CONTENT, id);
	if (ret < 0) {
		return ret;
	}

	if (observed) {
		ret = coap_append_option_int(&response, COAP_OPTION_OBSERVE, resource->age);
		if (ret < 0) {
			return ret;
		}
	}

	ret = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (ret < 0) {
		return ret;
	}

	ret = coap_packet_append_payload_marker(&response);
	if (ret < 0) {
		return ret;
	}

	ret = snprintk(payload, sizeof(payload), "counter: %d", obs_counter);
	if (ret < 0) {
		return ret;
	}

	ret = coap_packet_append_payload(&response, (const uint8_t *)payload, strlen(payload));
	if (ret < 0) {
		return ret;
	}

	return coap_resource_send(resource, &response, addr, addr_len, NULL);
}

static int obs_get(struct coap_resource *resource, struct coap_packet *request,
		   struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t type;
	uint8_t tkl;
	int ret;

	/* Zero means the request registered an observer. Anything else is a
	 * plain GET, a deregistration, or a registration the server could not
	 * take, and the answer to all of those carries no Observe option.
	 * RFC 7641 4.1.
	 */
	ret = coap_resource_parse_observe(resource, request, addr);

	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	type = (type == COAP_TYPE_CON) ? COAP_TYPE_ACK : COAP_TYPE_NON_CON;

	return obs_send(resource, addr, addr_len, type, id, token, tkl, ret == 0);
}

/* A notification is confirmable, so that an observer that has gone away
 * answers with a reset and is removed. RFC 7641 4.5.
 */
static void obs_notify(struct coap_resource *resource, struct coap_observer *observer)
{
	int ret;

	ret = obs_send(resource, net_sad(&observer->addr),
		       net_family2size(observer->addr.ss_family), COAP_TYPE_CON,
		       coap_next_id(), observer->token, observer->tkl, true);
	if (ret < 0) {
		LOG_ERR("Cannot notify an observer (%d)", ret);
	}
}

static const char * const obs_path[] = { "obs", NULL };

COAP_RESOURCE_DEFINE(obs_resource, conformance_server, {
	.get = obs_get,
	.path = obs_path,
	.notify = obs_notify,
});

static void obs_update(struct k_work *work)
{
	ARG_UNUSED(work);

	obs_counter++;
	(void)coap_resource_notify(&obs_resource);

	k_work_reschedule(&obs_work, OBS_INTERVAL);
}

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	/* net_config has already brought the interface up and set its
	 * addresses: it waits for both before main() runs.
	 */

	for (size_t i = 0; i < sizeof(large_body); i++) {
		large_body[i] = 'a' + (i % 26);
	}

	k_work_schedule(&obs_work, OBS_INTERVAL);

	LOG_INF("CoAP server ready");

	while (true) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
