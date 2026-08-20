/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the CoAP conformance suite.
 *
 * The ETSI derived test cases the suite runs all address one resource, /test,
 * and expect the four methods on it to answer with Content, Created, Changed
 * and Deleted. That is the whole of what is needed here, so rather than
 * pulling in the CoAP server sample and its resources this defines just that
 * resource.
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
#include <zephyr/net/net_mgmt.h>

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

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	/* The addresses are configured during initialisation, so the interface
	 * is usually up before main() runs. Only wait when it is not: waiting
	 * unconditionally would block until the timeout, because the event has
	 * already been and gone.
	 */
	if (!net_if_is_up(iface)) {
		(void)net_mgmt_event_wait_on_iface(iface, NET_EVENT_IF_UP, NULL,
						   NULL, NULL, K_SECONDS(10));
	}

	LOG_INF("CoAP server ready");

	while (true) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
