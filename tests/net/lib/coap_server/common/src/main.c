/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

static int coap_method1(struct coap_resource *resource, struct coap_packet *request,
			struct net_sockaddr *addr, net_socklen_t addr_len)
{
	ARG_UNUSED(resource);
	ARG_UNUSED(request);
	ARG_UNUSED(addr);
	ARG_UNUSED(addr_len);

	return -ENOSYS;
}

static int coap_method2(struct coap_resource *resource, struct coap_packet *request,
			struct net_sockaddr *addr, net_socklen_t addr_len)
{
	ARG_UNUSED(resource);
	ARG_UNUSED(request);
	ARG_UNUSED(addr);
	ARG_UNUSED(addr_len);

	return -ENOSYS;
}

static const uint16_t service_A_port = 4242;
COAP_SERVICE_DEFINE(service_A, "a.service.com", &service_A_port, COAP_SERVICE_AUTOSTART);

static const char * const resource_0_path[] = { "res0", NULL };
COAP_RESOURCE_DEFINE(resource_0, service_A, {
	.path = resource_0_path,
	.get = coap_method1,
	.put = coap_method2,
});

static const char * const resource_1_path[] = { "res1", NULL };
COAP_RESOURCE_DEFINE(resource_1, service_A, {
	.path = resource_1_path,
	.post = coap_method1,
});

static uint16_t service_B_port;
COAP_SERVICE_DEFINE(service_B, "b.service.com", &service_B_port, 0);

static const char * const resource_2_path[] = { "res2", "sub", NULL };
COAP_RESOURCE_DEFINE(resource_2, service_B, {
	.path = resource_2_path,
	.get = coap_method2,
	.put = coap_method1,
});

static const char * const resource_3_path[] = { "res3", "+", NULL };
COAP_RESOURCE_DEFINE(resource_3, service_B, {
	.path = resource_3_path,
	.post = coap_method2,
});

static const uint16_t service_C_port = 5959;
#ifdef CONFIG_NET_SOCKETS_ENABLE_DTLS
static const sec_tag_t sec_tag_list[] = { 1 };

COAPS_SERVICE_DEFINE(service_C, "192.168.1.1", &service_C_port, 0, sec_tag_list,
		     sizeof(sec_tag_list));

#else

COAP_SERVICE_DEFINE(service_C, "192.168.1.1", &service_C_port, 0);
#endif /* CONFIG_NET_SOCKETS_ENABLE_DTLS */

static const char * const resource_4_path[] = { "res4", "*", NULL };
COAP_RESOURCE_DEFINE(resource_4, service_C, {
	.path = resource_4_path,
	.get = coap_method1,
});

/* Replies to a GET with a piggybacked 2.05 response. */
static int coap_ping_get(struct coap_resource *resource, struct coap_packet *request,
			 struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t data[32];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	struct coap_packet response;
	int ret;

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, COAP_TYPE_ACK,
			       tkl, token, COAP_RESPONSE_CODE_CONTENT,
			       coap_header_get_id(request));
	if (ret < 0) {
		return ret;
	}

	return coap_resource_send(resource, &response, addr, addr_len, NULL);
}

static const uint16_t service_D_port = 5700;
COAP_SERVICE_DEFINE(service_D, "127.0.0.1", &service_D_port, COAP_SERVICE_AUTOSTART);

static const char * const resource_ping_path[] = { "ping", NULL };
COAP_RESOURCE_DEFINE(resource_ping, service_D, {
	.path = resource_ping_path,
	.get = coap_ping_get,
});

ZTEST(coap_service, test_COAP_SERVICE_DEFINE)
{
	zassert_ok(strcmp(service_A.host, "a.service.com"));
	zassert_equal(service_A.port, &service_A_port);
	zassert_equal(*service_A.port, 4242);
	zassert_equal(service_A.sec_tag_list, NULL);
	zassert_equal(service_A.sec_tag_list_size, 0);

	zassert_ok(strcmp(service_B.host, "b.service.com"));
	zassert_equal(service_B.port, &service_B_port);
	zassert_equal(*service_B.port, 0);
	zassert_equal(service_B.sec_tag_list, NULL);
	zassert_equal(service_B.sec_tag_list_size, 0);

	zassert_ok(strcmp(service_C.host, "192.168.1.1"));
	zassert_equal(service_C.port, &service_C_port);
	zassert_equal(*service_C.port, 5959);

#ifdef CONFIG_NET_SOCKETS_ENABLE_DTLS
	zassert_equal(service_C.sec_tag_list, sec_tag_list);
	zassert_equal(service_C.sec_tag_list_size, sizeof(sec_tag_list));
#else
	zassert_equal(service_C.sec_tag_list, NULL);
	zassert_equal(service_C.sec_tag_list_size, 0);
#endif
}

ZTEST(coap_service, test_COAP_SERVICE_COUNT)
{
	size_t n_svc;

	n_svc = 4273;
	COAP_SERVICE_COUNT(&n_svc);
	zassert_equal(n_svc, 4);
}

ZTEST(coap_service, test_COAP_SERVICE_RESOURCE_COUNT)
{
	zassert_equal(COAP_SERVICE_RESOURCE_COUNT(&service_A), 2);
	zassert_equal(COAP_SERVICE_RESOURCE_COUNT(&service_B), 2);
	zassert_equal(COAP_SERVICE_RESOURCE_COUNT(&service_C), 1);
	zassert_equal(COAP_SERVICE_RESOURCE_COUNT(&service_D), 1);
}

ZTEST(coap_service, test_COAP_SERVICE_HAS_RESOURCE)
{
	zassert_true(COAP_SERVICE_HAS_RESOURCE(&service_A, &resource_0));
	zassert_true(COAP_SERVICE_HAS_RESOURCE(&service_A, &resource_1));
	zassert_false(COAP_SERVICE_HAS_RESOURCE(&service_A, &resource_2));
	zassert_false(COAP_SERVICE_HAS_RESOURCE(&service_A, &resource_3));

	zassert_false(COAP_SERVICE_HAS_RESOURCE(&service_B, &resource_0));
	zassert_false(COAP_SERVICE_HAS_RESOURCE(&service_B, &resource_1));
	zassert_true(COAP_SERVICE_HAS_RESOURCE(&service_B, &resource_2));
	zassert_true(COAP_SERVICE_HAS_RESOURCE(&service_B, &resource_3));

	zassert_false(COAP_SERVICE_HAS_RESOURCE(&service_C, &resource_0));
	zassert_true(COAP_SERVICE_HAS_RESOURCE(&service_C, &resource_4));
}

ZTEST(coap_service, test_COAP_SERVICE_FOREACH)
{
	size_t n_svc = 0;
	size_t have_service_A = 0;
	size_t have_service_B = 0;
	size_t have_service_C = 0;
	size_t have_service_D = 0;

	COAP_SERVICE_FOREACH(svc) {
		if (svc == &service_A) {
			have_service_A = 1;
			zassert_equal(svc->flags & COAP_SERVICE_AUTOSTART, COAP_SERVICE_AUTOSTART);
		} else if (svc == &service_B) {
			have_service_B = 1;
			zassert_equal(svc->flags & COAP_SERVICE_AUTOSTART, 0);
		} else if (svc == &service_C) {
			have_service_C = 1;
			zassert_equal(svc->flags & COAP_SERVICE_AUTOSTART, 0);
		} else if (svc == &service_D) {
			have_service_D = 1;
			zassert_equal(svc->flags & COAP_SERVICE_AUTOSTART, COAP_SERVICE_AUTOSTART);
		} else {
			zassert_unreachable("svc (%p) not equal to &service_A (%p), &service_B "
					    "(%p), &service_C (%p), or &service_D (%p)",
					    svc, &service_A, &service_B, &service_C, &service_D);
		}

		n_svc++;
	}

	zassert_equal(n_svc, 4);
	zassert_equal(have_service_A + have_service_B + have_service_C + have_service_D, n_svc);
}

ZTEST(coap_service, test_COAP_RESOURCE_FOREACH)
{
	size_t first_res, second_res, n_res;

	n_res = 0;
	first_res = 0;
	second_res = 0;
	COAP_RESOURCE_FOREACH(service_A, res) {
		if (res == &resource_0) {
			first_res = 1;
		} else if (res == &resource_1) {
			second_res = 1;
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_0 (%p) or &resource_1 (%p)", res,
				&resource_0, &resource_1);
		}

		n_res++;
	}

	zassert_equal(n_res, 2);
	zassert_equal(first_res + second_res, n_res);

	n_res = 0;
	first_res = 0;
	second_res = 0;
	COAP_RESOURCE_FOREACH(service_B, res) {
		if (res == &resource_2) {
			first_res = 1;
		} else if (res == &resource_3) {
			second_res = 1;
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_2 (%p) or &resource_3 (%p)", res,
				&resource_2, &resource_3);
		}

		n_res++;
	}

	zassert_equal(n_res, 2);
	zassert_equal(first_res + second_res, n_res);

	n_res = 0;
	first_res = 0;
	second_res = 0;
	COAP_RESOURCE_FOREACH(service_C, res) {
		if (res == &resource_4) {
			first_res = 1;
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_4 (%p)", res, &resource_4);
		}
		n_res++;
	}

	zassert_equal(n_res, 1);
	zassert_equal(first_res + second_res, n_res);
}

ZTEST(coap_service, test_COAP_SERVICE_FOREACH_RESOURCE)
{
	size_t first_res, second_res, n_res;

	n_res = 0;
	first_res = 0;
	second_res = 0;
	COAP_SERVICE_FOREACH_RESOURCE(&service_A, res) {
		if (res == &resource_0) {
			first_res = 1;
		} else if (res == &resource_1) {
			second_res = 1;
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_0 (%p) or &resource_1 (%p)", res,
				&resource_0, &resource_1);
		}

		n_res++;
	}

	zassert_equal(n_res, 2);
	zassert_equal(first_res + second_res, n_res);

	n_res = 0;
	first_res = 0;
	second_res = 0;
	COAP_SERVICE_FOREACH_RESOURCE(&service_B, res) {
		if (res == &resource_2) {
			first_res = 1;
		} else if (res == &resource_3) {
			second_res = 1;
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_2 (%p) or &resource_3 (%p)", res,
				&resource_2, &resource_3);
		}

		n_res++;
	}

	zassert_equal(n_res, 2);
	zassert_equal(first_res + second_res, n_res);

	n_res = 0;
	first_res = 0;
	second_res = 0;
	COAP_SERVICE_FOREACH_RESOURCE(&service_C, res) {
		if (res == &resource_4) {
			first_res = 1;
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_4 (%p)", res, &resource_4);
		}
		n_res++;
	}

	zassert_equal(n_res, 1);
	zassert_equal(first_res + second_res, n_res);
}

ZTEST(coap_service, test_COAP_RESOURCE_DEFINE)
{
	COAP_SERVICE_FOREACH_RESOURCE(&service_A, res) {
		if (res == &resource_0) {
			zassert_ok(strcmp(res->path[0], "res0"));
			zassert_equal(res->get, coap_method1);
			zassert_equal(res->put, coap_method2);
		} else if (res == &resource_1) {
			zassert_ok(strcmp(res->path[0], "res1"));
			zassert_equal(res->post, coap_method1);
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_0 (%p) or &resource_1 (%p)", res,
				&resource_0, &resource_1);
		}
	}

	COAP_SERVICE_FOREACH_RESOURCE(&service_B, res) {
		if (res == &resource_2) {
			zassert_ok(strcmp(res->path[0], "res2"));
			zassert_ok(strcmp(res->path[1], "sub"));
			zassert_equal(res->get, coap_method2);
			zassert_equal(res->put, coap_method1);
		} else if (res == &resource_3) {
			zassert_ok(strcmp(res->path[0], "res3"));
			zassert_ok(strcmp(res->path[1], "+"));
			zassert_equal(res->post, coap_method2);
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_2 (%p) or &resource_3 (%p)", res,
				&resource_2, &resource_3);
		}
	}

	COAP_SERVICE_FOREACH_RESOURCE(&service_C, res) {
		if (res == &resource_4) {
			zassert_ok(strcmp(res->path[0], "res4"));
			zassert_ok(strcmp(res->path[1], "*"));
			zassert_equal(res->get, coap_method1);
			zassert_equal(res->put, NULL);
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_4 (%p)", res, &resource_4);
		}
	}
}

/*
 * Regression test: a confirmable message whose retransmission fails to send
 * (for example the network interface goes down while awaiting the ACK) must
 * not trip an assertion in the CoAP server thread; it used to kernel-panic
 * with ASSERTION FAIL [ret == pending->len] once zsock_sendto() returned
 * -ENETDOWN.
 */
ZTEST(coap_service, test_retransmit_send_failure_no_assert)
{
	struct net_if *iface = net_if_get_default();
	struct coap_transmission_parameters params = coap_get_transmission_parameters();
	struct net_sockaddr_in6 dst = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(5684),
	};
	struct coap_packet cpkt;
	uint8_t buf[64];

	zassert_equal(zsock_inet_pton(NET_AF_INET6, "::1", &dst.sin6_addr), 1);

	/* Wait for the CoAP service to start */
	for (int i = 0; i < 100 && coap_service_is_running(&service_A) != 1; i++) {
		k_sleep(K_MSEC(10));
	}
	zassert_equal(coap_service_is_running(&service_A), 1);

	/* Use short retransmission timeouts so the test finishes quickly */
	params.ack_timeout = 50;
	params.coap_backoff_percent = 200;
	params.max_retransmission = 2;

	zassert_ok(coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_CON, 0,
				    NULL, COAP_METHOD_GET, coap_next_id()));

	/* Sending a confirmable packet queues it for retransmission */
	zassert_ok(coap_service_send(&service_A, &cpkt, (struct net_sockaddr *)&dst,
				     sizeof(dst), &params));

	/* Take the interface down so the pending retransmissions fail to send */
	zassert_ok(net_if_down(iface));

	/* Let the whole retransmission schedule expire while the interface is
	 * down; each attempt fails with -ENETDOWN in the CoAP server thread.
	 */
	k_sleep(K_MSEC(500));

	zassert_ok(net_if_up(iface));
}

/* An observer is keyed by its endpoint and its token, and an empty token is
 * a token like any other: a client that registers with one has to be
 * notified, and has to be able to deregister again. RFC 7641 section 4.1.
 */
ZTEST(coap_service, test_observe_with_empty_token)
{
	struct net_sockaddr_in6 peer = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(5683),
	};
	struct coap_packet request;
	uint8_t buf[64];

	zassert_equal(zsock_inet_pton(NET_AF_INET6, "::1", &peer.sin6_addr), 1);

	/* A GET with the Observe option set to 0 and no token registers. */
	zassert_ok(coap_packet_init(&request, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_CON,
				    0, NULL, COAP_METHOD_GET, coap_next_id()));
	zassert_ok(coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0));

	zassert_equal(coap_resource_parse_observe(&resource_0, &request,
						  (struct net_sockaddr *)&peer),
		      0, "an observer with an empty token was not registered");

	/* The same again is a refresh, not a second observer. */
	zassert_equal(coap_resource_parse_observe(&resource_0, &request,
						  (struct net_sockaddr *)&peer),
		      0, "a refresh with an empty token was not accepted");

	/* The Observe option set to 1 deregisters that same observer. */
	zassert_ok(coap_packet_init(&request, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_CON,
				    0, NULL, COAP_METHOD_GET, coap_next_id()));
	zassert_ok(coap_append_option_int(&request, COAP_OPTION_OBSERVE, 1));

	zassert_equal(coap_resource_parse_observe(&resource_0, &request,
						  (struct net_sockaddr *)&peer),
		      1, "the observer with an empty token was not removed");

	zassert_equal(coap_resource_parse_observe(&resource_0, &request,
						  (struct net_sockaddr *)&peer),
		      -ENOENT, "a removed observer was found again");
}

/* A reply to an IPv4 request is sent with an IP_PKTINFO control message, which
 * is smaller than the control buffer the server reserves for IPV6_PKTINFO.
 * On 64-bit targets the server thread used to spin forever in the stack on the
 * unused tail of that buffer and never answered.
 */
ZTEST(coap_service, test_ipv4_request_gets_reply)
{
	struct net_sockaddr_in dst = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(5700),
	};
	struct zsock_pollfd pfd;
	struct coap_packet request;
	uint8_t buf[64];
	uint8_t rsp[64];
	int sock;
	ssize_t len;

	zassert_equal(zsock_inet_pton(NET_AF_INET, "127.0.0.1", &dst.sin_addr), 1);

	for (int i = 0; i < 100 && coap_service_is_running(&service_D) != 1; i++) {
		k_sleep(K_MSEC(10));
	}
	zassert_equal(coap_service_is_running(&service_D), 1);

	zassert_ok(coap_packet_init(&request, buf, sizeof(buf), COAP_VERSION_1, COAP_TYPE_CON,
				    0, NULL, COAP_METHOD_GET, coap_next_id()));
	zassert_ok(coap_packet_append_option(&request, COAP_OPTION_URI_PATH, "ping", 4));

	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0);

	len = zsock_sendto(sock, request.data, request.offset, 0,
			   (struct net_sockaddr *)&dst, sizeof(dst));
	zassert_equal(len, request.offset);

	pfd.fd = sock;
	pfd.events = ZSOCK_POLLIN;
	zassert_equal(zsock_poll(&pfd, 1, 1000), 1, "no reply to an IPv4 request");

	len = zsock_recv(sock, rsp, sizeof(rsp), 0);
	zassert_true(len >= 4, "short reply");
	zassert_equal(rsp[1], COAP_RESPONSE_CODE_CONTENT);

	zsock_close(sock);
}

ZTEST_SUITE(coap_service, NULL, NULL, NULL, NULL, NULL);
