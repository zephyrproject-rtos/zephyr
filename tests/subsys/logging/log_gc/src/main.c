/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Mohammed El Amrani
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_internal.h>
#include <zephyr/ztest.h>

#include <string.h>

#include "test_modules.h"

#define USED_MODULE_NAME "log_gc_used"

ZTEST(log_gc, test_unused_module_metadata_is_collected)
{
	const uint32_t source_count = log_src_cnt_get(Z_LOG_LOCAL_DOMAIN_ID);
	const int source_id = log_source_id_get(USED_MODULE_NAME);
	const char *source_name;

	log_gc_used_module_run();

	zassert_equal(source_count, 1U, "Expected one log source, got %u", source_count);
	zassert_equal(source_id, 0, "Unexpected source ID: %d", source_id);

	source_name = log_source_name_get(Z_LOG_LOCAL_DOMAIN_ID, source_id);
	zassert_not_null(source_name, "Used source has no name");
	zassert_equal(strcmp(source_name, USED_MODULE_NAME), 0, "Unexpected source name");
	zassert_equal(log_source_id_get("log_gc_unused"), -1,
		      "Unused source is still registered");

#if defined(CONFIG_LOG_RUNTIME_FILTERING)
	{
		const struct log_source_const_data *const_data = TYPE_SECTION_START(log_const);
		struct log_source_dynamic_data *dynamic_data = TYPE_SECTION_START(log_dynamic);

		z_log_runtime_filters_init();

		zassert_equal(log_const_source_id(const_data), source_id,
			      "Incorrect constant source ID");
		zassert_equal(log_dynamic_source_id(dynamic_data), source_id,
			      "Constant and dynamic source IDs differ");
		zassert_equal(LOG_FILTER_AGGR_SLOT_GET(&dynamic_data->filters),
			      CONFIG_LOG_RUNTIME_DEFAULT_LEVEL,
			      "Incorrect initial runtime filter");
	}
#endif
}

ZTEST_SUITE(log_gc, NULL, NULL, NULL, NULL, NULL);
