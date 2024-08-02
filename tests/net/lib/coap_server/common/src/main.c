/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net/coap_service.h>

static int coap_method1(struct coap_resource *resource, struct coap_packet *request,
			struct sockaddr *addr, socklen_t addr_len)
{
	ARG_UNUSED(resource);
	ARG_UNUSED(request);
	ARG_UNUSED(addr);
	ARG_UNUSED(addr_len);

	return -ENOSYS;
}

static int coap_method2(struct coap_resource *resource, struct coap_packet *request,
			struct sockaddr *addr, socklen_t addr_len)
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
COAP_SERVICE_DEFINE(service_C, "192.168.1.1", &service_C_port, 0);

static const char * const resource_4_path[] = { "res4", "*", NULL };
COAP_RESOURCE_DEFINE(resource_4, service_C, {
	.path = resource_4_path,
	.get = coap_method1,
});

ZTEST(coap_service, test_COAP_SERVICE_DEFINE)
{
	zassert_ok(strcmp(service_A.host, "a.service.com"));
	zassert_equal(service_A.port, &service_A_port);
	zassert_equal(*service_A.port, 4242);

	zassert_ok(strcmp(service_B.host, "b.service.com"));
	zassert_equal(service_B.port, &service_B_port);
	zassert_equal(*service_B.port, 0);

	zassert_ok(strcmp(service_C.host, "192.168.1.1"));
	zassert_equal(service_C.port, &service_C_port);
	zassert_equal(*service_C.port, 5959);
}

ZTEST(coap_service, test_COAP_SERVICE_COUNT)
{
	size_t n_svc;

	n_svc = 4273;
	COAP_SERVICE_COUNT(&n_svc);
	zassert_equal(n_svc, 3);
}

ZTEST(coap_service, test_COAP_SERVICE_RESOURCE_COUNT)
{
	zassert_equal(COAP_SERVICE_RESOURCE_COUNT(&service_A), 2);
	zassert_equal(COAP_SERVICE_RESOURCE_COUNT(&service_B), 2);
	zassert_equal(COAP_SERVICE_RESOURCE_COUNT(&service_C), 1);
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
		} else {
			zassert_unreachable("svc (%p) not equal to &service_A (%p), &service_B "
					    "(%p), or &service_C (%p)",
					    svc, &service_A, &service_B, &service_C);
		}

		n_svc++;
	}

	zassert_equal(n_svc, 3);
	zassert_equal(have_service_A + have_service_B + have_service_C, n_svc);
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

ZTEST_SUITE(coap_service, NULL, NULL, NULL, NULL, NULL);
