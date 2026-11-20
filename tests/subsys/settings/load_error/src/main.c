/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/settings/settings.h>
#include <zephyr/ztest.h>

#include "settings_priv.h"

struct test_store {
	struct settings_store store;
	int result;
	uint32_t calls;
	uint32_t value;
};

static struct test_store stores[3];

static ssize_t read_value(void *cb_arg, void *data, size_t len)
{
	len = MIN(len, sizeof(uint32_t));
	memcpy(data, cb_arg, len);
	return len;
}

static int test_load(struct settings_store *store, const struct settings_load_arg *arg)
{
	struct test_store *source = CONTAINER_OF(store, struct test_store, store);

	zassert_str_equal(arg->subtree, "test");
	source->calls++;
	if (source->result == 0 && arg->cb != NULL) {
		return arg->cb("value", sizeof(source->value), read_value, &source->value,
			       arg->param);
	}
	return source->result;
}

static int load_value(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
		      void *param)
{
	zassert_str_equal(key, "value");
	zassert_equal(len, sizeof(uint32_t));
	zassert_equal(read_cb(cb_arg, param, len), len);
	return 0;
}

static const struct settings_store_itf test_store_itf = {
	.csi_load = test_load,
};

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	sys_slist_init(&settings_load_srcs);
	memset(stores, 0, sizeof(stores));
	for (size_t i = 0; i < ARRAY_SIZE(stores); i++) {
		stores[i].store.cs_itf = &test_store_itf;
		settings_src_register(&stores[i].store);
	}
}

/** @brief An empty source list is a successful scan. */
ZTEST(settings_load_error, test_no_sources)
{
	sys_slist_init(&settings_load_srcs);
	zassert_ok(settings_load_subtree_direct("test", NULL, NULL));
}

/** @brief Successful scans visit every registered source. */
ZTEST(settings_load_error, test_success)
{
	zassert_ok(settings_load_subtree_direct("test", NULL, NULL));
	for (size_t i = 0; i < ARRAY_SIZE(stores); i++) {
		zassert_equal(stores[i].calls, 1U);
	}
}

/** @brief A backend error is preserved without stopping subsequent scans. */
ZTEST(settings_load_error, test_first_source_error)
{
	stores[0].result = -EIO;
	zassert_equal(settings_load_subtree_direct("test", NULL, NULL), -EIO);
	zassert_equal(stores[0].calls, 1U);
	zassert_equal(stores[1].calls, 1U);
	zassert_equal(stores[2].calls, 1U);
}

/** @brief A successful source does not hide a later backend error. */
ZTEST(settings_load_error, test_later_source_error)
{
	stores[1].result = -EACCES;
	zassert_equal(settings_load_subtree_direct("test", NULL, NULL), -EACCES);
	zassert_equal(stores[0].calls, 1U);
	zassert_equal(stores[1].calls, 1U);
	zassert_equal(stores[2].calls, 1U);
}

/** @brief Subsequent errors and positive results do not hide the first error. */
ZTEST(settings_load_error, test_first_error_preserved)
{
	stores[0].result = -EIO;
	stores[1].result = -EACCES;
	stores[2].result = 1;
	zassert_equal(settings_load_subtree_direct("test", NULL, NULL), -EIO);
	for (size_t i = 0; i < ARRAY_SIZE(stores); i++) {
		zassert_equal(stores[i].calls, 1U);
	}
}

/** @brief Later sources still load and override values after a scan error. */
ZTEST(settings_load_error, test_values_after_error)
{
	uint32_t value = 0U;

	stores[0].result = -EIO;
	stores[1].value = 1U;
	stores[2].value = 2U;
	zassert_equal(settings_load_subtree_direct("test", load_value, &value), -EIO);
	zassert_equal(value, stores[2].value);
}

/** @brief Positive callback stop values retain the successful return contract. */
ZTEST(settings_load_error, test_positive_result)
{
	stores[0].result = 1;
	zassert_ok(settings_load_subtree_direct("test", NULL, NULL));
	zassert_equal(stores[1].calls, 1U);
	zassert_equal(stores[2].calls, 1U);

	stores[2].result = 1;
	zassert_ok(settings_load_subtree_direct("test", NULL, NULL));
}

ZTEST_SUITE(settings_load_error, NULL, NULL, before, NULL, NULL);
