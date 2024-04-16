/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log links
 *
 */


#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include "mock_log_link.h"

LOG_MODULE_REGISTER(test);

static void process(const struct log_backend *const backend,
			union log_msg_generic *msg)
{

}

static void panic(const struct log_backend *const backend)
{

}

const struct log_backend_api log_backend_test_api = {
	.process = process,
	.panic = panic
};

LOG_BACKEND_DEFINE(backend1, log_backend_test_api, false);

LOG_BACKEND_DEFINE(backend2, log_backend_test_api, false);

static void log_setup(bool backend2_enable)
{
	uint8_t offset = 0;

	log_init();
	(void)z_log_links_activate(0xFFFFFFFF, &offset);

	log_backend_enable(&backend1, NULL, LOG_LEVEL_DBG);

	if (backend2_enable) {
		log_backend_enable(&backend2, NULL, LOG_LEVEL_INF);
	} else {
		log_backend_disable(&backend2);
	}
}


struct mock_log_link_source domain1_sources[] = {
	{ .source = "abc", .clevel = 4, .rlevel = 4},
	{ .source = "xxx", .clevel = 3, .rlevel = 3},
	{ .source = "yyy", .clevel = 2, .rlevel = 2},
	{ .source = "zzz", .clevel = 4, .rlevel = 1},
};

struct mock_log_link_source domain2_sources[] = {
	{ .source = "abc2", .clevel = 2, .rlevel = 1},
	{ .source = "xxx2", .clevel = 2, .rlevel = 2},
	{ .source = "yyy2", .clevel = 3, .rlevel = 3},
	{ .source = "zzz2", .clevel = 4, .rlevel = 4},
};

struct mock_log_link_source domain3_sources[] = {
	{ .source = "abc", .clevel = 4, .rlevel = 4},
	{ .source = "xxx", .clevel = 3, .rlevel = 3},
	{ .source = "yyy", .clevel = 2, .rlevel = 2},
	{ .source = "zzz", .clevel = 2, .rlevel = 1},
};

struct mock_log_link_domain domain1 = {
	.source_cnt = ARRAY_SIZE(domain1_sources),
	.sources = domain1_sources,
	.name = "domain1"
};

struct mock_log_link_domain domain2 = {
	.source_cnt = ARRAY_SIZE(domain2_sources),
	.sources = domain2_sources,
	.name = "domain2"
};

struct mock_log_link_domain domain3 = {
	.source_cnt = ARRAY_SIZE(domain3_sources),
	.sources = domain3_sources,
	.name = "domain3"
};

struct mock_log_link_domain *domains_a[] = {&domain1, &domain2};
struct mock_log_link mock_link_a = {
	.domain_cnt = ARRAY_SIZE(domains_a),
	.domains = domains_a
};

struct mock_log_link_domain *domains_b[] = {&domain3};
struct mock_log_link mock_link_b = {
	.domain_cnt = ARRAY_SIZE(domains_b),
	.domains = domains_b
};

extern struct log_link_api mock_log_link_api;

LOG_LINK_DEF(mock_link1, mock_log_link_api, 0, &mock_link_a);
LOG_LINK_DEF(mock_link2, mock_log_link_api, 0, &mock_link_b);

ZTEST(log_links, test_log_domain_count)
{
	uint8_t domains_cnt;
	uint8_t exp_domains_cnt;

	log_setup(false);

	exp_domains_cnt = 1 + mock_link_a.domain_cnt + mock_link_b.domain_cnt;
	domains_cnt = log_domains_count();
	zassert_equal(domains_cnt, exp_domains_cnt,
			"Unexpected number of domains (%d)", domains_cnt);
}

ZTEST(log_links, test_log_source_count)
{
	uint8_t exp_source_cnt[] = {
		log_const_source_id(TYPE_SECTION_END(log_const)),
		/*link1*/
		domains_a[0]->source_cnt,
		domains_a[1]->source_cnt,
		domains_b[0]->source_cnt,
	};

	log_setup(false);


	for (uint8_t d = 0; d < log_domains_count(); d++) {
		uint16_t source_cnt = log_src_cnt_get(d);

		zassert_equal(source_cnt, exp_source_cnt[d],
			      "Unexpected source count (%d:%d)", d, source_cnt);
	}
}

static void test_single_compile_level(uint8_t d, uint16_t s, uint32_t exp_level)
{
	uint32_t level = log_filter_get(NULL, d, s, false);

	zassert_equal(level, exp_level,
			"%d:%d Unexpected compiled level (%d vs %d)",
			d, s, level, exp_level);
}

ZTEST(log_links, test_log_compiled_level_get)
{
	log_setup(false);

	test_single_compile_level(1, 0, domains_a[0]->sources[0].clevel);
	test_single_compile_level(1, 1, domains_a[0]->sources[1].clevel);
	test_single_compile_level(1, 3, domains_a[0]->sources[3].clevel);
	test_single_compile_level(2, 2, domains_a[1]->sources[2].clevel);
	test_single_compile_level(3, 2, domains_b[0]->sources[2].clevel);
}

static void test_single_runtime_level(uint8_t d, uint16_t s, uint8_t *link_level)
{
	uint32_t level1;
	uint32_t level2;

	level1 = log_filter_get(&backend1, d, s, true);
	level2 = log_filter_get(&backend2, d, s, true);
	zassert_equal(level1, *link_level,
			"%d:%d Unexpected compiled level (%d vs %d)",
			d, s, level1, *link_level);
	zassert_equal(level2, MIN(*link_level, LOG_LEVEL_INF),
			"%d:%d Unexpected compiled level (%d vs %d)",
			d, s, level2, MIN(*link_level, LOG_LEVEL_INF));

	log_filter_set(&backend1, d, s, 1);
	log_filter_set(&backend2, d, s, 2);
	level1 = log_filter_get(&backend1, d, s, true);
	level2 = log_filter_get(&backend2, d, s, true);
	zassert_equal(level1, 1, "%d:%d Unexpected compiled level (%d vs %d)",
			d, s, level1, 1);
	zassert_equal(level2, 2, "%d:%d Unexpected compiled level (%d vs %d)",
			d, s, level2, 2);

	/* level set in link should be the max of both level set */
	zassert_equal(*link_level, 2,
			"%d:%d Unexpected compiled level (got:%d exp:%d)",
			d, s, *link_level, 2);

}

ZTEST(log_links, test_log_runtime_level_set)
{
	log_setup(true);

	test_single_runtime_level(1, 0, &domains_a[0]->sources[0].rlevel);
	test_single_runtime_level(1, 3, &domains_a[0]->sources[3].rlevel);
	test_single_runtime_level(2, 1, &domains_a[1]->sources[1].rlevel);
	test_single_runtime_level(3, 1, &domains_b[0]->sources[1].rlevel);
}

ZTEST(log_links, test_log_domain_name_get)
{
	zassert_equal(strcmp(log_domain_name_get(0), ""), 0,
			"Unexpected domain name");
	zassert_equal(strcmp(log_domain_name_get(1), "domain1"), 0,
			"Unexpected domain name (%s)", log_domain_name_get(1));
	zassert_equal(strcmp(log_domain_name_get(2), "domain2"), 0,
			"Unexpected domain name (%s)", log_domain_name_get(2));
	zassert_equal(strcmp(log_domain_name_get(3), "domain3"), 0,
			"Unexpected domain name (%s)", log_domain_name_get(3));
}

static void test_single_log_source_name_get(uint8_t d, uint16_t s,
						const char *exp_name)
{
	const char *name = log_source_name_get(d, s);

	zassert_equal(strcmp(name, exp_name), 0, "%d:%d Unexpected source name",
			d, s);
}

ZTEST(log_links, test_log_source_name_get)
{
	log_setup(false);

	test_single_log_source_name_get(1, 0, domains_a[0]->sources[0].source);
	test_single_log_source_name_get(1, 1, domains_a[0]->sources[1].source);
	test_single_log_source_name_get(2, 2, domains_a[1]->sources[2].source);
	test_single_log_source_name_get(3, 3, domains_b[0]->sources[3].source);
}

ZTEST_SUITE(log_links, NULL, NULL, NULL, NULL, NULL);
