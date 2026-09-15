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

static struct mock_log_link_source link1_sources[] = {
	{ .source = "abc", .clevel = 4, .rlevel = 4},
	{ .source = "xxx", .clevel = 3, .rlevel = 3},
	{ .source = "yyy", .clevel = 2, .rlevel = 2},
	{ .source = "zzz", .clevel = 4, .rlevel = 1},
};

static struct mock_log_link_source link2_sources[] = {
	{ .source = "abc2", .clevel = 2, .rlevel = 1},
	{ .source = "xxx2", .clevel = 2, .rlevel = 2},
	{ .source = "yyy2", .clevel = 3, .rlevel = 3},
	{ .source = "zzz2", .clevel = 4, .rlevel = 4},
};

static struct mock_log_link_source link3_sources[] = {
	{ .source = "abc", .clevel = 4, .rlevel = 4},
	{ .source = "xxx", .clevel = 3, .rlevel = 3},
	{ .source = "yyy", .clevel = 2, .rlevel = 2},
	{ .source = "zzz", .clevel = 2, .rlevel = 1},
};

static struct mock_log_link mock_link1_ctx = {
	.source_cnt = ARRAY_SIZE(link1_sources),
	.sources = link1_sources
};

static struct mock_log_link mock_link2_ctx = {
	.source_cnt = ARRAY_SIZE(link2_sources),
	.sources = link2_sources
};

static struct mock_log_link mock_link3_ctx = {
	.source_cnt = ARRAY_SIZE(link3_sources),
	.sources = link3_sources
};

MOCK_LOG_LINK_DEFINE(mock_link1, &mock_link1_ctx);
MOCK_LOG_LINK_DEFINE(mock_link2, &mock_link2_ctx);
MOCK_LOG_LINK_DEFINE(mock_link3, &mock_link3_ctx);

/* Each link provides a single domain. Domain IDs are assigned in the order in
 * which links are activated, starting from 1 (0 is the local domain).
 */
static struct mock_log_link *links[] = {
	&mock_link1_ctx, &mock_link2_ctx, &mock_link3_ctx
};

#define LINK_DOMAIN(_domain_id) links[(_domain_id) - 1]

ZTEST(log_links, test_log_domain_count)
{
	uint8_t domains_cnt;
	uint8_t exp_domains_cnt;

	log_setup(false);

	exp_domains_cnt = 1 + ARRAY_SIZE(links);
	domains_cnt = log_domains_count();
	zassert_equal(domains_cnt, exp_domains_cnt,
			"Unexpected number of domains (%d)", domains_cnt);
}

ZTEST(log_links, test_log_source_count)
{
	uint32_t exp_source_cnt[] = {
		log_const_source_id(TYPE_SECTION_END(log_const)),
		LINK_DOMAIN(1)->source_cnt,
		LINK_DOMAIN(2)->source_cnt,
		LINK_DOMAIN(3)->source_cnt,
	};

	log_setup(false);


	for (uint8_t d = 0; d < log_domains_count(); d++) {
		uint32_t source_cnt = log_src_cnt_get(d);

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

	test_single_compile_level(1, 0, LINK_DOMAIN(1)->sources[0].clevel);
	test_single_compile_level(1, 1, LINK_DOMAIN(1)->sources[1].clevel);
	test_single_compile_level(1, 3, LINK_DOMAIN(1)->sources[3].clevel);
	test_single_compile_level(2, 2, LINK_DOMAIN(2)->sources[2].clevel);
	test_single_compile_level(3, 2, LINK_DOMAIN(3)->sources[2].clevel);
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

	test_single_runtime_level(1, 0, &LINK_DOMAIN(1)->sources[0].rlevel);
	test_single_runtime_level(1, 3, &LINK_DOMAIN(1)->sources[3].rlevel);
	test_single_runtime_level(2, 1, &LINK_DOMAIN(2)->sources[1].rlevel);
	test_single_runtime_level(3, 1, &LINK_DOMAIN(3)->sources[1].rlevel);
}

ZTEST(log_links, test_log_domain_name_get)
{
	/* Remote domain is named after the link which provides it. */
	zassert_str_equal(log_domain_name_get(0), CONFIG_LOG_DOMAIN_NAME,
			  "Unexpected domain name");
	zassert_str_equal(log_domain_name_get(1), "mock_link1",
			  "Unexpected domain name (%s)", log_domain_name_get(1));
	zassert_str_equal(log_domain_name_get(2), "mock_link2",
			  "Unexpected domain name (%s)", log_domain_name_get(2));
	zassert_str_equal(log_domain_name_get(3), "mock_link3",
			  "Unexpected domain name (%s)", log_domain_name_get(3));
}

static void test_single_log_source_name_get(uint8_t d, uint16_t s,
						const char *exp_name)
{
	const char *name = log_source_name_get(d, s);

	if (exp_name == NULL) {
		zassert_equal(name, exp_name);
		return;
	}

	zassert_equal(strcmp(name, exp_name), 0, "%d:%d Unexpected source name",
			d, s);
}

ZTEST(log_links, test_log_source_name_get)
{
	const char *exp_name = LINK_DOMAIN(1)->sources[0].source;

	log_setup(false);

	test_single_log_source_name_get(1, 0, exp_name);
	test_single_log_source_name_get(1, 1, LINK_DOMAIN(1)->sources[1].source);
	test_single_log_source_name_get(2, 2, LINK_DOMAIN(2)->sources[2].source);
	test_single_log_source_name_get(3, 3, LINK_DOMAIN(3)->sources[3].source);

	/* Try fetching invalid sources, it should not change the cached name */
	for (uint16_t s = 0; s < CONFIG_LOG_SOURCE_NAME_CACHE_ENTRY_COUNT; s++) {
		test_single_log_source_name_get(1, s + 100, NULL);
	}

	LINK_DOMAIN(1)->sources[0].source = "new_name";
	/* Even though source name changed, the cached name should not change. */
	test_single_log_source_name_get(1, 0, exp_name);
}

ZTEST_SUITE(log_links, NULL, NULL, NULL, NULL, NULL);
