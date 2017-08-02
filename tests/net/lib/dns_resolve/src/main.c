/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
extern void test_init(void);
extern void dns_query_invalid_timeout(void);
extern void dns_query_invalid_context(void);
extern void dns_query_invalid_callback(void);
extern void dns_query_invalid_query(void);
extern void dns_query_too_many(void);
extern void dns_query_server_count(void);
extern void dns_query_ipv4_server_count(void);
extern void dns_query_ipv6_server_count(void);
extern void dns_query_ipv4_timeout(void);
extern void dns_query_ipv6_timeout(void);
extern void dns_query_ipv4_cancel(void);
extern void dns_query_ipv6_cancel(void);
extern void dns_query_ipv4(void);
extern void dns_query_ipv6(void);
void test_main(void)
{
	ztest_test_suite(dns_tests,
			 ztest_unit_test(test_init),
			 ztest_unit_test(dns_query_invalid_timeout),
			 ztest_unit_test(dns_query_invalid_context),
			 ztest_unit_test(dns_query_invalid_callback),
			 ztest_unit_test(dns_query_invalid_query),
			 ztest_unit_test(dns_query_too_many),
			 ztest_unit_test(dns_query_server_count),
			 ztest_unit_test(dns_query_ipv4_server_count),
			 ztest_unit_test(dns_query_ipv6_server_count),
			 ztest_unit_test(dns_query_ipv4_timeout),
			 ztest_unit_test(dns_query_ipv6_timeout),
			 ztest_unit_test(dns_query_ipv4_cancel),
			 ztest_unit_test(dns_query_ipv6_cancel),
			 ztest_unit_test(dns_query_ipv4),
			 ztest_unit_test(dns_query_ipv6));

	ztest_run_test_suite(dns_tests);
}
