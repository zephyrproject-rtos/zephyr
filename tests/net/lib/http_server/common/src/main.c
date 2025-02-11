/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/server.h>

static struct http_resource_detail detail[] = {
	{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	{
			.type = HTTP_RESOURCE_TYPE_WEBSOCKET,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
};

#define DETAIL(n) &detail[n]
#define RES(n) &detail[n]

/*
 * Two separate HTTP server instances (A and B), each with different static
 * resources, listening on different ports. Static resources could be, for
 * example, gzip compressed html, javascript, css, or image files which
 * have fixed paths known at build time.
 *
 * REST endpoints could be considered as static resources, as long as
 * the paths (and implementation-specific details) are known at compile time.
 */
static const uint16_t service_A_port = 4242;
HTTP_SERVICE_DEFINE(service_A, "a.service.com", &service_A_port, 4, 2, DETAIL(0));
HTTP_RESOURCE_DEFINE(resource_0, service_A, "/", RES(0));
HTTP_RESOURCE_DEFINE(resource_1, service_A, "/index.html", RES(1));

/* ephemeral port of 0 */
static uint16_t service_B_port;
HTTP_SERVICE_DEFINE(service_B, "b.service.com", &service_B_port, 7, 3, DETAIL(1));
HTTP_RESOURCE_DEFINE(resource_2, service_B, "/foo.htm", RES(2));
HTTP_RESOURCE_DEFINE(resource_3, service_B, "/bar/baz.php", RES(3));

/*
 * An "empty" HTTP service is one without static resources. For example, a
 * service which loads resources from a filesystem that are determined at
 * runtime.
 */
static const uint16_t service_C_port = 5959;
HTTP_SERVICE_DEFINE_EMPTY(service_C, "192.168.1.1", &service_C_port, 5, 9, DETAIL(2));

/* Wildcard resources */
static uint16_t service_D_port = service_A_port + 1;
HTTP_SERVICE_DEFINE(service_D, "2001:db8::1", &service_D_port, 7, 3, DETAIL(3));
HTTP_RESOURCE_DEFINE(resource_4, service_D, "/foo1.htm*", RES(0));
HTTP_RESOURCE_DEFINE(resource_5, service_D, "/fo*", RES(1));
HTTP_RESOURCE_DEFINE(resource_6, service_D, "/f[ob]o3.html", RES(1));
HTTP_RESOURCE_DEFINE(resource_7, service_D, "/fb?3.htm", RES(0));
HTTP_RESOURCE_DEFINE(resource_8, service_D, "/f*4.html", RES(3));


ZTEST(http_service, test_HTTP_SERVICE_DEFINE)
{
	zassert_ok(strcmp(service_A.host, "a.service.com"));
	zassert_equal(service_A.port, &service_A_port);
	zassert_equal(*service_A.port, 4242);
	zassert_equal(service_A.detail, DETAIL(0));
	zassert_equal(service_A.concurrent, 4);
	zassert_equal(service_A.backlog, 2);

	zassert_ok(strcmp(service_B.host, "b.service.com"));
	zassert_equal(service_B.port, &service_B_port);
	zassert_equal(*service_B.port, 0);
	zassert_equal(service_B.detail, DETAIL(1));
	zassert_equal(service_B.concurrent, 7);
	zassert_equal(service_B.backlog, 3);

	zassert_ok(strcmp(service_C.host, "192.168.1.1"));
	zassert_equal(service_C.port, &service_C_port);
	zassert_equal(*service_C.port, 5959);
	zassert_equal(service_C.detail, DETAIL(2));
	zassert_equal(service_C.concurrent, 5);
	zassert_equal(service_C.backlog, 9);
	zassert_equal(service_C.res_begin, NULL);
	zassert_equal(service_C.res_end, NULL);
}

ZTEST(http_service, test_HTTP_SERVICE_COUNT)
{
	size_t n_svc;

	n_svc = 4273;
	HTTP_SERVICE_COUNT(&n_svc);
	zassert_equal(n_svc, 4);
}

ZTEST(http_service, test_HTTP_SERVICE_RESOURCE_COUNT)
{
	zassert_equal(HTTP_SERVICE_RESOURCE_COUNT(&service_A), 2);
	zassert_equal(HTTP_SERVICE_RESOURCE_COUNT(&service_B), 2);
	zassert_equal(HTTP_SERVICE_RESOURCE_COUNT(&service_C), 0);
}

ZTEST(http_service, test_HTTP_SERVICE_FOREACH)
{
	size_t n_svc = 0;
	size_t have_service_A = 0;
	size_t have_service_B = 0;
	size_t have_service_C = 0;
	size_t have_service_D = 0;

	HTTP_SERVICE_FOREACH(svc) {
		if (svc == &service_A) {
			have_service_A = 1;
		} else if (svc == &service_B) {
			have_service_B = 1;
		} else if (svc == &service_C) {
			have_service_C = 1;
		} else if (svc == &service_D) {
			have_service_D = 1;
		} else {
			zassert_unreachable("svc (%p) not equal to &service_A (%p), &service_B "
					    "(%p), or &service_C (%p)",
					    svc, &service_A, &service_B, &service_C);
		}

		n_svc++;
	}

	zassert_equal(n_svc, 4);
	zassert_equal(have_service_A + have_service_B + have_service_C + have_service_D, n_svc);
}

ZTEST(http_service, test_HTTP_RESOURCE_FOREACH)
{
	size_t first_res, second_res, n_res;

	n_res = 0;
	first_res = 0;
	second_res = 0;
	HTTP_RESOURCE_FOREACH(service_A, res) {
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
	HTTP_RESOURCE_FOREACH(service_B, res) {
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
	HTTP_SERVICE_FOREACH_RESOURCE(&service_C, res) {
		zassert_unreachable("service_C does not have any resources");
		n_res++;
	}

	zassert_equal(n_res, 0);
}

ZTEST(http_service, test_HTTP_SERVICE_FOREACH_RESOURCE)
{
	size_t first_res, second_res, n_res;

	n_res = 0;
	first_res = 0;
	second_res = 0;
	HTTP_SERVICE_FOREACH_RESOURCE(&service_A, res) {
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
	HTTP_SERVICE_FOREACH_RESOURCE(&service_B, res) {
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
	HTTP_SERVICE_FOREACH_RESOURCE(&service_C, res) {
		zassert_unreachable("service_C does not have any resources");
		n_res++;
	}

	zassert_equal(n_res, 0);
}

ZTEST(http_service, test_HTTP_RESOURCE_DEFINE)
{
	HTTP_SERVICE_FOREACH_RESOURCE(&service_A, res) {
		if (res == &resource_0) {
			zassert_ok(strcmp(res->resource, "/"));
			zassert_equal(res->detail, RES(0));
		} else if (res == &resource_1) {
			zassert_ok(strcmp(res->resource, "/index.html"));
			zassert_equal(res->detail, RES(1));
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_0 (%p) or &resource_1 (%p)", res,
				&resource_0, &resource_1);
		}
	}

	HTTP_SERVICE_FOREACH_RESOURCE(&service_B, res) {
		if (res == &resource_2) {
			zassert_ok(strcmp(res->resource, "/foo.htm"));
			zassert_equal(res->detail, RES(2));
		} else if (res == &resource_3) {
			zassert_ok(strcmp(res->resource, "/bar/baz.php"));
			zassert_equal(res->detail, RES(3));
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_2 (%p) or &resource_3 (%p)", res,
				&resource_2, &resource_3);
		}
	}
}

extern struct http_resource_detail *get_resource_detail(const char *path,
							int *path_len,
							bool is_websocket);

#define CHECK_PATH(path, len) ({ *len = 0; get_resource_detail(path, len, false); })

ZTEST(http_service, test_HTTP_RESOURCE_WILDCARD)
{
	struct http_resource_detail *res;
	int len;

	res = CHECK_PATH("/", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(0), "Resource mismatch");

	res = CHECK_PATH("/f", &len);
	zassert_is_null(res, "Resource found");
	zassert_equal(len, 0, "Length set");

	res = CHECK_PATH("/foo1.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(0), "Resource mismatch");

	res = CHECK_PATH("/foo2222.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(1), "Resource mismatch");

	res = CHECK_PATH("/fbo3.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(1), "Resource mismatch");

	res = CHECK_PATH("/fbo3.htm", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(0), "Resource mismatch");

	res = CHECK_PATH("/fbo4.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(3), "Resource mismatch");
}

ZTEST_SUITE(http_service, NULL, NULL, NULL, NULL, NULL);
