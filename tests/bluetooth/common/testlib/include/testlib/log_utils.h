/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_LOG_UTILS_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_LOG_UTILS_H_

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/__assert.h>

static inline void bt_testlib_log_level_set(char *module, uint32_t new_level)
{
	int source_id;
	uint32_t result_level;

	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING));

	source_id = log_source_id_get(module);
	__ASSERT(source_id >= 0, "%d", source_id);

	result_level = log_filter_set(NULL, Z_LOG_LOCAL_DOMAIN_ID, source_id, new_level);
	__ASSERT(result_level == new_level, "%u %u", result_level, new_level);
}

static inline void bt_testlib_log_level_set_all(uint32_t new_level)
{
	uint32_t source_count;

	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING));

	source_count = log_src_cnt_get(Z_LOG_LOCAL_DOMAIN_ID);

	for (uint32_t source_id = 0; source_id < source_count; source_id++) {
		uint32_t result_level;

		result_level = log_filter_set(NULL, Z_LOG_LOCAL_DOMAIN_ID, source_id, new_level);
		__ASSERT(result_level == new_level, "%u %u", result_level, new_level);
	}
}

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_LOG_UTILS_H_ */
