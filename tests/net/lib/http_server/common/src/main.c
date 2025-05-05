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
	{
		.type = HTTP_RESOURCE_TYPE_STATIC_FS,
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
HTTP_SERVICE_DEFINE(service_A, "a.service.com", &service_A_port, 4, 2, DETAIL(0), NULL);
HTTP_RESOURCE_DEFINE(resource_0, service_A, "/", RES(0));
HTTP_RESOURCE_DEFINE(resource_1, service_A, "/index.html", RES(1));
HTTP_RESOURCE_DEFINE(resource_2, service_A, "/fs/*", RES(5));

/* ephemeral port of 0 */
static uint16_t service_B_port;
HTTP_SERVICE_DEFINE(service_B, "b.service.com", &service_B_port, 7, 3, DETAIL(1), NULL);
HTTP_RESOURCE_DEFINE(resource_3, service_B, "/foo.htm", RES(2));
HTTP_RESOURCE_DEFINE(resource_4, service_B, "/bar/baz.php", RES(3));

/*
 * An "empty" HTTP service is one without static resources. For example, a
 * service which loads resources from a filesystem that are determined at
 * runtime.
 */
static const uint16_t service_C_port = 5959;
HTTP_SERVICE_DEFINE_EMPTY(service_C, "192.168.1.1", &service_C_port, 5, 9, DETAIL(2), NULL);

/* Wildcard resources */
static uint16_t service_D_port = service_A_port + 1;
HTTP_SERVICE_DEFINE(service_D, "2001:db8::1", &service_D_port, 7, 3, DETAIL(3), NULL);
HTTP_RESOURCE_DEFINE(resource_5, service_D, "/foo1.htm*", RES(0));
HTTP_RESOURCE_DEFINE(resource_6, service_D, "/fo*", RES(1));
HTTP_RESOURCE_DEFINE(resource_7, service_D, "/f[ob]o3.html", RES(1));
HTTP_RESOURCE_DEFINE(resource_8, service_D, "/fb?3.htm", RES(0));
HTTP_RESOURCE_DEFINE(resource_9, service_D, "/f*4.html", RES(3));
HTTP_RESOURCE_DEFINE(resource_11, service_D, "/foo/*", RES(3));
HTTP_RESOURCE_DEFINE(resource_12, service_D, "/foo/b?r", RES(3));

/* Default resource in case of no match */
static uint16_t service_E_port = 8080;
HTTP_SERVICE_DEFINE(service_E, "192.0.2.1", &service_E_port, 1, 1, NULL, DETAIL(0));
HTTP_RESOURCE_DEFINE(resource_10, service_E, "/index.html", RES(4));

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
	zassert_equal(n_svc, 5);
}

ZTEST(http_service, test_HTTP_SERVICE_RESOURCE_COUNT)
{
	zassert_equal(HTTP_SERVICE_RESOURCE_COUNT(&service_A), 3);
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
	size_t have_service_E = 0;

	HTTP_SERVICE_FOREACH(svc) {
		if (svc == &service_A) {
			have_service_A = 1;
		} else if (svc == &service_B) {
			have_service_B = 1;
		} else if (svc == &service_C) {
			have_service_C = 1;
		} else if (svc == &service_D) {
			have_service_D = 1;
		} else if (svc == &service_E) {
			have_service_E = 1;
		} else {
			zassert_unreachable("svc (%p) not equal to any defined service", svc);
		}

		n_svc++;
	}

	zassert_equal(n_svc, 5);
	zassert_equal(have_service_A, 1);
	zassert_equal(have_service_B, 1);
	zassert_equal(have_service_C, 1);
	zassert_equal(have_service_D, 1);
	zassert_equal(have_service_E, 1);
}

ZTEST(http_service, test_HTTP_RESOURCE_FOREACH)
{
	size_t first_res, second_res, third_res, n_res;

	n_res = 0;
	first_res = 0;
	second_res = 0;
	third_res = 0;
	HTTP_RESOURCE_FOREACH(service_A, res) {
		if (res == &resource_0) {
			first_res = 1;
		} else if (res == &resource_1) {
			second_res = 1;
		} else if (res == &resource_2) {
			third_res = 1;
		} else {
			zassert_unreachable("res (%p) not equal to &resource_0 (%p), &resource_1 "
					    "(%p) or &resource_2 (%p)",
					    res, &resource_0, &resource_1, &resource_2);
		}

		n_res++;
	}

	zassert_equal(n_res, 3);
	zassert_equal(first_res + second_res + third_res, n_res);

	n_res = 0;
	first_res = 0;
	second_res = 0;
	HTTP_RESOURCE_FOREACH(service_B, res) {
		if (res == &resource_3) {
			first_res = 1;
		} else if (res == &resource_4) {
			second_res = 1;
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_3 (%p) or &resource_4 (%p)", res,
				&resource_3, &resource_4);
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
	size_t first_res, second_res, third_res, n_res;

	n_res = 0;
	first_res = 0;
	second_res = 0;
	third_res = 0;
	HTTP_SERVICE_FOREACH_RESOURCE(&service_A, res) {
		if (res == &resource_0) {
			first_res = 1;
		} else if (res == &resource_1) {
			second_res = 1;
		} else if (res == &resource_2) {
			third_res = 1;
		} else {
			zassert_unreachable("res (%p) not equal to &resource_0 (%p), &resource_1 "
					    "(%p) or &resource_2 (%p)",
					    res, &resource_0, &resource_1, &resource_2);
		}

		n_res++;
	}

	zassert_equal(n_res, 3);
	zassert_equal(first_res + second_res + third_res, n_res);

	n_res = 0;
	first_res = 0;
	second_res = 0;
	HTTP_SERVICE_FOREACH_RESOURCE(&service_B, res) {
		if (res == &resource_3) {
			first_res = 1;
		} else if (res == &resource_4) {
			second_res = 1;
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_3 (%p) or &resource_4 (%p)", res,
				&resource_3, &resource_4);
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
		} else if (res == &resource_2) {
			zassert_ok(strcmp(res->resource, "/fs/*"));
			zassert_equal(res->detail, RES(5));
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_0 (%p) or &resource_1 (%p)", res,
				&resource_0, &resource_1);
		}
	}

	HTTP_SERVICE_FOREACH_RESOURCE(&service_B, res) {
		if (res == &resource_3) {
			zassert_ok(strcmp(res->resource, "/foo.htm"));
			zassert_equal(res->detail, RES(2));
		} else if (res == &resource_4) {
			zassert_ok(strcmp(res->resource, "/bar/baz.php"));
			zassert_equal(res->detail, RES(3));
		} else {
			zassert_unreachable(
				"res (%p) not equal to &resource_3 (%p) or &resource_4 (%p)", res,
				&resource_3, &resource_4);
		}
	}
}

extern struct http_resource_detail *get_resource_detail(const struct http_service_desc *service,
							const char *path,
							int *path_len,
							bool is_websocket);

#define CHECK_PATH(svc, path, len) ({ *len = 0; get_resource_detail(&svc, path, len, false); })

ZTEST(http_service, test_HTTP_RESOURCE_WILDCARD)
{
	struct http_resource_detail *res;
	int len;

	res = CHECK_PATH(service_A, "/", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(0), "Resource mismatch");

	res = CHECK_PATH(service_D, "/f", &len);
	zassert_is_null(res, "Resource found");
	zassert_equal(len, 0, "Length set");

	res = CHECK_PATH(service_D, "/foo1.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(0), "Resource mismatch");

	res = CHECK_PATH(service_D, "/foo2222.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(1), "Resource mismatch");

	res = CHECK_PATH(service_D, "/fbo3.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(1), "Resource mismatch");

	res = CHECK_PATH(service_D, "/fbo3.htm", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(0), "Resource mismatch");

	res = CHECK_PATH(service_D, "/fbo4.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(3), "Resource mismatch");

	res = CHECK_PATH(service_D, "/fb", &len);
	zassert_is_null(res, "Resource found");
	zassert_equal(len, 0, "Length set");

	res = CHECK_PATH(service_A, "/fs/index.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(5), "Resource mismatch");

	/* Resources that only exist on one service should not be found on another */
	res = CHECK_PATH(service_A, "/foo1.htm", &len);
	zassert_is_null(res, "Resource found");
	zassert_equal(len, 0, "Length set");

	res = CHECK_PATH(service_A, "/foo2222.html", &len);
	zassert_is_null(res, "Resource found");
	zassert_equal(len, 0, "Length set");

	res = CHECK_PATH(service_A, "/fbo3.htm", &len);
	zassert_is_null(res, "Resource found");
	zassert_equal(len, 0, "Length set");

	res = CHECK_PATH(service_D, "/foo/bar", &len);
	zassert_not_null(res, "Resource not found");
	zassert_true(len == (sizeof("/foo/bar") - 1), "Length not set correctly");
	zassert_equal(res, RES(3), "Resource mismatch");

	res = CHECK_PATH(service_D, "/foo/bar?param=value", &len);
	zassert_not_null(res, "Resource not found");
	zassert_true(len == (sizeof("/foo/bar") - 1), "Length not set correctly");
	zassert_equal(res, RES(3), "Resource mismatch");

	res = CHECK_PATH(service_D, "/bar?foo=value", &len);
	zassert_is_null(res, "Resource found");
	zassert_equal(len, 0, "Length set");

	res = CHECK_PATH(service_D, "/foo/bar?param=value", &len);
	zassert_not_null(res, "Resource not found");
	zassert_true(len == (sizeof("/foo/bar") - 1), "Length not set correctly");
	zassert_equal(res, RES(3), "Resource mismatch");
}

ZTEST(http_service, test_HTTP_RESOURCE_DEFAULT)
{
#define NON_EXISTING_PATH "/this_path_is_not_registered"
	struct http_resource_detail *res;
	int len;

	/* For a path that does exist, the correct resource should be returned */
	res = CHECK_PATH(service_E, "/index.html", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_true(len > 0, "Length not set");
	zassert_equal(res, RES(4), "Resource mismatch");

	/* For a path that does not exist, the default resource should be returned */
	res = CHECK_PATH(service_E, NON_EXISTING_PATH, &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_equal(len, strlen(NON_EXISTING_PATH), "Length incorrect");
	zassert_equal(res, RES(0), "Resource mismatch");

	/* If query params are present, length should not include them */
	res = CHECK_PATH(service_E, NON_EXISTING_PATH "?param=value", &len);
	zassert_not_null(res, "Cannot find resource");
	zassert_equal(len, strlen(NON_EXISTING_PATH), "Length incorrect");
	zassert_equal(res, RES(0), "Resource mismatch");
}

extern void http_server_get_content_type_from_extension(char *url, char *content_type,
							size_t content_type_size);

/* add another content type */
HTTP_SERVER_CONTENT_TYPE(mpg, "video/mpeg")

ZTEST(http_service, test_HTTP_SERVER_CONTENT_TYPE)
{
	size_t n_content_types = 0;
	size_t have_html = 0;
	size_t have_css = 0;
	size_t have_js = 0;
	size_t have_jpg = 0;
	size_t have_png = 0;
	size_t have_svg = 0;
	size_t have_mpg = 0;

	HTTP_SERVER_CONTENT_TYPE_FOREACH(ct)
	{
		if (strncmp(ct->extension, "html", ct->extension_len) == 0) {
			have_html = 1;
		} else if (strncmp(ct->extension, "css", ct->extension_len) == 0) {
			have_css = 1;
		} else if (strncmp(ct->extension, "js", ct->extension_len) == 0) {
			have_js = 1;
		} else if (strncmp(ct->extension, "jpg", ct->extension_len) == 0) {
			have_jpg = 1;
		} else if (strncmp(ct->extension, "png", ct->extension_len) == 0) {
			have_png = 1;
		} else if (strncmp(ct->extension, "svg", ct->extension_len) == 0) {
			have_svg = 1;
		} else if (strncmp(ct->extension, "mpg", ct->extension_len) == 0) {
			have_mpg = 1;
		} else {
			zassert_unreachable("unknown extension (%s)", ct->extension);
		}

		n_content_types++;
	}

	zassert_equal(n_content_types, 7);
	zassert_equal(have_html & have_css & have_js & have_jpg & have_png & have_svg & have_mpg,
		      1);

	char *mp3 = "song.mp3";
	char *html = "page.html";
	char *mpg = "video.mpg";
	char content_type[64] = "unknown";

	http_server_get_content_type_from_extension(mp3, content_type, sizeof(content_type));
	zassert_str_equal(content_type, "unknown");

	http_server_get_content_type_from_extension(html, content_type, sizeof(content_type));
	zassert_str_equal(content_type, "text/html");

	http_server_get_content_type_from_extension(mpg, content_type, sizeof(content_type));
	zassert_str_equal(content_type, "video/mpeg");
}

ZTEST_SUITE(http_service, NULL, NULL, NULL, NULL, NULL);
