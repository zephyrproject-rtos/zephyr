/*
 * Copyright (c) 2024 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include "dns_cache.h"

#define TEST_DNS_CACHE_SIZE        12
#define TEST_DNS_CACHE_DEFAULT_TTL 1
DNS_CACHE_DEFINE(test_dns_cache, TEST_DNS_CACHE_SIZE);

void clear_cache(void *fixture)
{
	ARG_UNUSED(fixture);
	dns_cache_flush(&test_dns_cache);
}

ZTEST_SUITE(net_dns_cache_test, NULL, NULL, clear_cache, NULL, NULL);

ZTEST(net_dns_cache_test, test_simple_cache_entry)
{
	struct dns_addrinfo info_write = {.ai_family = AF_INET};
	struct dns_addrinfo info_read = {0};
	const char *query = "example.com";
	enum dns_query_type query_type = DNS_QUERY_TYPE_A;

	zassert_ok(dns_cache_add(&test_dns_cache, query, &info_write, TEST_DNS_CACHE_DEFAULT_TTL),
		   "Cache entry adding should work.");
	zassert_equal(1, dns_cache_find(&test_dns_cache, query, query_type, &info_read, 1));
	zassert_equal(AF_INET, info_read.ai_family);
}

ZTEST(net_dns_cache_test, test_not_cached)
{
	struct dns_addrinfo info_read = {0};
	const char *query = "example.com";
	enum dns_query_type query_type = DNS_QUERY_TYPE_A;

	zassert_equal(0, dns_cache_find(&test_dns_cache, query, query_type, &info_read, 1));
	zassert_equal(0, info_read.ai_family);
}

ZTEST(net_dns_cache_test, test_fill_cache)
{
	struct dns_addrinfo info_write = {.ai_family = AF_INET};
	struct dns_addrinfo info_read[TEST_DNS_CACHE_SIZE] = {0};
	const char *query = "example.com";
	enum dns_query_type query_type = DNS_QUERY_TYPE_A;

	for (size_t i = 0; i < TEST_DNS_CACHE_SIZE; i++) {
		zassert_ok(dns_cache_add(&test_dns_cache, query, &info_write,
					 TEST_DNS_CACHE_DEFAULT_TTL),
			   "Cache entry adding should work.");
	}
	zassert_equal(TEST_DNS_CACHE_SIZE, dns_cache_find(&test_dns_cache, query, query_type,
							  info_read, TEST_DNS_CACHE_SIZE));
	zassert_equal(AF_INET, info_read[TEST_DNS_CACHE_SIZE - 1].ai_family);
}

ZTEST(net_dns_cache_test, test_flush)
{
	struct dns_addrinfo info_write = {.ai_family = AF_INET};
	struct dns_addrinfo info_read[TEST_DNS_CACHE_SIZE] = {0};
	const char *query = "example.com";
	enum dns_query_type query_type = DNS_QUERY_TYPE_A;

	for (size_t i = 0; i < TEST_DNS_CACHE_SIZE; i++) {
		zassert_ok(dns_cache_add(&test_dns_cache, query, &info_write,
					 TEST_DNS_CACHE_DEFAULT_TTL),
			   "Cache entry adding should work.");
	}
	zassert_ok(dns_cache_flush(&test_dns_cache));
	zassert_equal(0, dns_cache_find(&test_dns_cache, query, query_type, info_read,
					TEST_DNS_CACHE_SIZE));
	zassert_equal(0, info_read[TEST_DNS_CACHE_SIZE - 1].ai_family);
}

ZTEST(net_dns_cache_test, test_fill_cache_to_small)
{
	struct dns_addrinfo info_write = {.ai_family = AF_INET};
	struct dns_addrinfo info_read[TEST_DNS_CACHE_SIZE - 1] = {0};
	const char *query = "example.com";
	enum dns_query_type query_type = DNS_QUERY_TYPE_A;

	for (size_t i = 0; i < TEST_DNS_CACHE_SIZE; i++) {
		zassert_ok(dns_cache_add(&test_dns_cache, query, &info_write,
					 TEST_DNS_CACHE_DEFAULT_TTL),
			   "Cache entry adding should work.");
	}
	zassert_equal(-ENOSR, dns_cache_find(&test_dns_cache, query, query_type, info_read,
					     TEST_DNS_CACHE_SIZE - 1));
	zassert_equal(AF_INET, info_read[TEST_DNS_CACHE_SIZE - 2].ai_family);
}

ZTEST(net_dns_cache_test, test_closest_expiry_removed)
{
	struct dns_addrinfo info_write = {.ai_family = AF_INET};
	struct dns_addrinfo info_read = {0};
	const char *closest_expiry = "example.com";
	enum dns_query_type query_type = DNS_QUERY_TYPE_A;

	zassert_ok(dns_cache_add(&test_dns_cache, closest_expiry, &info_write,
				 TEST_DNS_CACHE_DEFAULT_TTL),
		   "Cache entry adding should work.");
	k_sleep(K_MSEC(1));
	for (size_t i = 0; i < TEST_DNS_CACHE_SIZE; i++) {
		zassert_ok(dns_cache_add(&test_dns_cache, "example2.com", &info_write,
					 TEST_DNS_CACHE_DEFAULT_TTL),
			   "Cache entry adding should work.");
	}
	zassert_equal(0,
		      dns_cache_find(&test_dns_cache, closest_expiry, query_type, &info_read, 1));
	zassert_equal(0, info_read.ai_family);
}

ZTEST(net_dns_cache_test, test_expired_entries_removed)
{
	struct dns_addrinfo info_write = {.ai_family = AF_INET};
	struct dns_addrinfo info_read[3] = {0};
	const char *query = "example.com";
	enum dns_query_type query_type = DNS_QUERY_TYPE_A;

	zassert_ok(dns_cache_add(&test_dns_cache, query, &info_write, TEST_DNS_CACHE_DEFAULT_TTL),
		   "Cache entry adding should work.");
	zassert_ok(
		dns_cache_add(&test_dns_cache, query, &info_write, TEST_DNS_CACHE_DEFAULT_TTL * 2),
		"Cache entry adding should work.");
	zassert_ok(
		dns_cache_add(&test_dns_cache, query, &info_write, TEST_DNS_CACHE_DEFAULT_TTL * 3),
		"Cache entry adding should work.");
	zassert_equal(3, dns_cache_find(&test_dns_cache, query, query_type, info_read, 3));
	zassert_equal(AF_INET, info_read[0].ai_family);
	k_sleep(K_MSEC(TEST_DNS_CACHE_DEFAULT_TTL * 1000 + 1));
	zassert_equal(2, dns_cache_find(&test_dns_cache, query, query_type, info_read, 3));
	zassert_equal(AF_INET, info_read[0].ai_family);
	k_sleep(K_MSEC(TEST_DNS_CACHE_DEFAULT_TTL * 1000 + 1));
	zassert_equal(1, dns_cache_find(&test_dns_cache, query, query_type, info_read, 3));
	zassert_equal(AF_INET, info_read[0].ai_family);
	k_sleep(K_MSEC(1));
	zassert_equal(1, dns_cache_find(&test_dns_cache, query, query_type, info_read, 3));
	zassert_equal(AF_INET, info_read[0].ai_family);
}

ZTEST(net_dns_cache_test, test_different_type_not_returned)
{
	struct dns_addrinfo info_write = {.ai_family = AF_INET};
	struct dns_addrinfo info_read = {0};
	const char *query = "example.com";
	enum dns_query_type query_type = DNS_QUERY_TYPE_AAAA;

	zassert_ok(dns_cache_add(&test_dns_cache, query, &info_write, TEST_DNS_CACHE_DEFAULT_TTL),
		   "Cache entry adding should work.");
	zassert_equal(0, dns_cache_find(&test_dns_cache, query, query_type, &info_read, 1));
	zassert_equal(0, info_read.ai_family);
}

ZTEST(net_dns_cache_test, test_only_expected_type_returned)
{
	struct dns_addrinfo info_write_a = {.ai_family = AF_INET};
	struct dns_addrinfo info_write_b = {.ai_family = AF_INET6};
	struct dns_addrinfo info_read = {0};
	const char *query = "example.com";
	enum dns_query_type query_type_a = DNS_QUERY_TYPE_A;
	enum dns_query_type query_type_b = DNS_QUERY_TYPE_AAAA;

	zassert_ok(dns_cache_add(&test_dns_cache, query, &info_write_a, TEST_DNS_CACHE_DEFAULT_TTL),
		   "Cache entry adding should work.");
	zassert_ok(dns_cache_add(&test_dns_cache, query, &info_write_b, TEST_DNS_CACHE_DEFAULT_TTL),
		   "Cache entry adding should work.");
	zassert_equal(1, dns_cache_find(&test_dns_cache, query, query_type_a, &info_read, 1));
	zassert_equal(AF_INET, info_read.ai_family);
	zassert_equal(1, dns_cache_find(&test_dns_cache, query, query_type_b, &info_read, 1));
	zassert_equal(AF_INET6, info_read.ai_family);
}
