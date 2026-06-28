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

/**
 * @brief Verify the total domain count aggregates the local domain and all linked domains.
 *
 * @details
 * Two mock log links expose several remote logging domains. After activating
 * the links, this test confirms the reported domain count equals the local
 * domain plus the domains contributed by each link, validating that logs from
 * multiple domains are aggregated through the link mechanism.
 *
 * Test steps:
 * - Set up logging and activate the configured links.
 * - Query the total domain count.
 * - Compare it against the local domain plus each link's domain count.
 *
 * Expected result:
 * - The domain count matches the sum of local and linked domains.
 *
 * @see log_domains_count()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-5
 */
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

/**
 * @brief Verify per-domain source counts reflect locally and link-registered sources.
 *
 * @details
 * Each logging domain registers an independent set of log sources, including
 * those contributed by remote domains over a link. This test confirms the
 * source count reported for every domain matches the expected number of
 * registered sources, validating source registration across multiple domains.
 *
 * Test steps:
 * - Set up logging and activate the configured links.
 * - For each domain query the registered source count.
 * - Compare each against the expected per-domain source count.
 *
 * Expected result:
 * - Each domain reports the expected number of registered sources.
 *
 * @see log_src_cnt_get()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-11
 */
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

/**
 * @brief Verify compile-time severity levels are retrieved for link-registered sources.
 *
 * @details
 * Sources exposed through a link carry their own compiled (build-time) severity
 * level. This test queries the compiled level for several sources across linked
 * domains and confirms each matches the level declared by the remote source,
 * validating compile-time severity filtering information across a link.
 *
 * Test steps:
 * - Set up logging and activate the configured links.
 * - Query the compiled level for selected sources in linked domains.
 * - Compare each against the source's declared compile level.
 *
 * Expected result:
 * - Each compiled level matches the declared level of the link source.
 *
 * @see log_filter_get()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-9
 */
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

/**
 * @brief Verify runtime severity filtering is set per backend and propagated across a link.
 *
 * @details
 * For sources reached through a link, this test reads the initial runtime
 * filter level per backend, sets distinct runtime levels for two backends, and
 * confirms the link's effective level becomes the maximum of the per-backend
 * settings. This validates per-source, per-backend runtime severity filtering
 * and its propagation to the remote domain over the link.
 *
 * Test steps:
 * - Set up logging with both backends enabled and activate the links.
 * - Read and then set distinct runtime filter levels per backend per source.
 * - Verify each backend's level and the link's aggregated (max) level.
 *
 * Expected result:
 * - Per-backend runtime levels are applied and the link level is their maximum.
 *
 * @see log_filter_set()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-10
 */
ZTEST(log_links, test_log_runtime_level_set)
{
	log_setup(true);

	test_single_runtime_level(1, 0, &domains_a[0]->sources[0].rlevel);
	test_single_runtime_level(1, 3, &domains_a[0]->sources[3].rlevel);
	test_single_runtime_level(2, 1, &domains_a[1]->sources[1].rlevel);
	test_single_runtime_level(3, 1, &domains_b[0]->sources[1].rlevel);
}

/**
 * @brief Verify domain names are reported correctly for the local and linked domains.
 *
 * @details
 * Each linked logging domain advertises a name that must be retrievable by
 * domain id. This test confirms the local domain reports an empty name and each
 * link-provided domain reports its configured name, validating identification of
 * domains aggregated through links.
 *
 * Test steps:
 * - Query the domain name for the local domain and each linked domain.
 * - Compare each returned name against the expected value.
 *
 * Expected result:
 * - Each domain id maps to its expected domain name.
 *
 * @see log_domain_name_get()
 * @ingroup logging_tests
 */
ZTEST(log_links, test_log_domain_name_get)
{
	zassert_str_equal(log_domain_name_get(0), "",
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

/**
 * @brief Verify source names are reported correctly for link-registered sources.
 *
 * @details
 * Sources registered through a link expose their names by domain and source id.
 * This test queries names for several sources across linked domains and confirms
 * each matches the configured remote source name, validating identification of
 * log sources registered across multiple domains.
 *
 * Test steps:
 * - Set up logging and activate the configured links.
 * - Query the source name for selected sources in linked domains.
 * - Compare each against the expected source name.
 *
 * Expected result:
 * - Each queried source reports its expected name.
 *
 * @see log_source_name_get()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-11
 */
ZTEST(log_links, test_log_source_name_get)
{
	log_setup(false);

	test_single_log_source_name_get(1, 0, domains_a[0]->sources[0].source);
	test_single_log_source_name_get(1, 1, domains_a[0]->sources[1].source);
	test_single_log_source_name_get(2, 2, domains_a[1]->sources[2].source);
	test_single_log_source_name_get(3, 3, domains_b[0]->sources[3].source);
}

ZTEST_SUITE(log_links, NULL, NULL, NULL, NULL, NULL);
