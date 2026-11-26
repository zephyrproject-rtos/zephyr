/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/url.h>
#include <zephyr/ztest.h>

static void expect_field(const struct net_url *u, const struct net_url_field *field,
			 const char *expected)
{
	zexpect_equal(field->len, strlen(expected), "length %u, expected \"%s\"", field->len,
		      expected);
	zexpect_mem_equal(u->base + field->off, expected, field->len);
}

ZTEST(net_url, test_parse_full)
{
	static const char url[] = "https://user:pw@example.com:8443/a/b?x=1&y=2#top";
	struct net_url u;

	zassert_ok(net_url_parse(url, &u));
	expect_field(&u, &u.scheme, "https");
	expect_field(&u, &u.userinfo, "user:pw");
	expect_field(&u, &u.host, "example.com");
	expect_field(&u, &u.path, "/a/b");
	expect_field(&u, &u.query, "x=1&y=2");
	expect_field(&u, &u.fragment, "top");
	zexpect_equal(u.port, 8443);
}

ZTEST(net_url, test_parse_minimal)
{
	struct net_url u;

	zassert_ok(net_url_parse("http://example.com", &u));
	expect_field(&u, &u.scheme, "http");
	expect_field(&u, &u.userinfo, "");
	expect_field(&u, &u.host, "example.com");
	expect_field(&u, &u.path, "");
	expect_field(&u, &u.query, "");
	expect_field(&u, &u.fragment, "");
	zexpect_equal(u.port, 0);
}

ZTEST(net_url, test_parse_ipv6_literal)
{
	struct net_url u;

	zassert_ok(net_url_parse("coaps://[2001:db8::1]:5684/fw", &u));
	expect_field(&u, &u.host, "2001:db8::1");
	expect_field(&u, &u.path, "/fw");
	zexpect_equal(u.port, 5684);
}

ZTEST(net_url, test_parse_invalid)
{
	struct net_url u;

	zexpect_equal(net_url_parse(NULL, &u), -EINVAL);
	zexpect_equal(net_url_parse("http://example.com", NULL), -EINVAL);
	zexpect_equal(net_url_parse("not a url", &u), -EINVAL);
	zexpect_equal(net_url_parse("http://example.com:port/", &u), -EINVAL);
}

ZTEST(net_url, test_scheme_is)
{
	struct net_url u;

	zassert_ok(net_url_parse("HTTPS://example.com/", &u));
	zexpect_true(net_url_scheme_is(&u, "https"));
	zexpect_false(net_url_scheme_is(&u, "http"));
	zexpect_false(net_url_scheme_is(&u, "httpss"));
}

ZTEST(net_url, test_field_copy)
{
	struct net_url u;
	char buf[12];

	zassert_ok(net_url_parse("http://example.com/p", &u));
	zexpect_ok(net_url_field_copy(&u, &u.host, buf, sizeof(buf)));
	zexpect_str_equal(buf, "example.com");
	zexpect_equal(net_url_field_copy(&u, &u.host, buf, sizeof("example.com") - 1), -ENOBUFS);
	zexpect_ok(net_url_field_copy(&u, &u.query, buf, sizeof(buf)));
	zexpect_str_equal(buf, "");
	zexpect_equal(net_url_field_copy(&u, &u.query, buf, 0), -ENOBUFS);
}

ZTEST(net_url, test_target_copy)
{
	struct net_url u;
	char buf[16];

	zassert_ok(net_url_parse("http://h/a/b?x=1#f", &u));
	zexpect_ok(net_url_target_copy(&u, buf, sizeof(buf)));
	zexpect_str_equal(buf, "/a/b?x=1");

	zassert_ok(net_url_parse("http://h?x=1", &u));
	zexpect_ok(net_url_target_copy(&u, buf, sizeof(buf)));
	zexpect_str_equal(buf, "/?x=1");

	zassert_ok(net_url_parse("http://h/#f", &u));
	zexpect_ok(net_url_target_copy(&u, buf, sizeof(buf)));
	zexpect_str_equal(buf, "/");

	zassert_ok(net_url_parse("http://h/abcdefgh", &u));
	zexpect_equal(net_url_target_copy(&u, buf, sizeof("/abcdefgh") - 1), -ENOBUFS);
	zexpect_ok(net_url_target_copy(&u, buf, sizeof("/abcdefgh")));
	zexpect_str_equal(buf, "/abcdefgh");
}

ZTEST_SUITE(net_url, NULL, NULL, NULL, NULL, NULL);
