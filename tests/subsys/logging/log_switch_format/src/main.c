/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_output_dict.h>
#include <ztest.h>

#define LOG_MODULE_NAME log_switch_format
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_ERR);

extern size_t log_format_table_size(void);

void log_msgs(void)
{
	/* standard print */
	LOG_ERR("Error message example.");

	/* raw string */
	printk("hello sys-t on board %s\n", CONFIG_BOARD);

#if CONFIG_LOG_MODE_DEFERRED
	/*
	 * When deferred logging is enabled, the work is being performed by
	 * another thread. This k_sleep() gives that thread time to process
	 * those messages.
	 */

	k_sleep(K_TICKS(10));
#endif
}

void test_log_switch_format_success_case(void)
{
	const struct log_backend *backend;
	uint32_t log_type;

	log_msgs();

	log_type = LOG_OUTPUT_TEXT;
	backend = log_format_set_all_active_backends(log_type);

	zassert_is_null(backend, "Unexpected failure in switching log format");

	log_msgs();

	log_type = LOG_OUTPUT_SYST;
	backend = log_format_set_all_active_backends(log_type);

	zassert_is_null(backend, "Unexpected failure in switching log format");

	log_msgs();

	log_type = LOG_OUTPUT_TEXT;
	backend = log_format_set_all_active_backends(log_type);

	zassert_is_null(backend, "Unexpected failure in switching log format");

	/* raw string */
	printk("SYST Sample Execution Completed\n");

}

void test_log_switch_format_set(void)
{
	const char *backend_name;
	const struct log_backend *backend;
	int ret;
	uint32_t log_type;

	log_type = LOG_OUTPUT_TEXT;
	backend_name = "not_exists";

	backend = log_backend_get_by_name(backend_name);
	zassert_is_null(backend, "Backend unexpectedly found");

	ret = log_backend_format_set(backend, log_type);

	zassert_equal(ret, -EINVAL, "Expected -EINVAL, Got %d\n", ret);

	backend_name = CONFIG_LOG_BACKEND_DEFAULT;
	backend = log_backend_get_by_name(backend_name);

	zassert_not_null(backend, "Backend not found");

	log_type = log_format_table_size() + 1;
	ret = log_backend_format_set(backend, log_type);

	zassert_equal(ret, -EINVAL, "Log type not supported, Invalid value returned");

}

void test_log_switch_format_set_all_active_backends(void)
{
	size_t log_type = log_format_table_size() + 1;
	const struct log_backend *backend;

	backend = log_format_set_all_active_backends(log_type);

	zassert_not_null(backend, "Unexpectedly all active backends switched the logging format");

	log_type = LOG_OUTPUT_SYST;
	backend = log_format_set_all_active_backends(log_type);

	zassert_is_null(backend, "Not all active backends have switched logging formats");
}

/* Testcase to verify the entries in function pointer table */
void test_log_switch_format_func_t_get(void)
{
	const log_format_func_t expected_values[] = {
	[LOG_OUTPUT_TEXT] = log_output_msg2_process,
	[LOG_OUTPUT_SYST] = IS_ENABLED(CONFIG_LOG_MIPI_SYST_ENABLE) ?
						log_output_msg2_syst_process : NULL,
	[LOG_OUTPUT_DICT] = IS_ENABLED(CONFIG_LOG_DICTIONARY_SUPPORT) ?
						log_dict_output_msg2_process : NULL
	};

	zassert_equal(log_format_table_size(), ARRAY_SIZE(expected_values),
					       "Update test for expected_values table");

	for (int i = 0; i < ARRAY_SIZE(expected_values); i++) {
		zassert_equal(log_format_func_t_get(i), expected_values[i],
							"Log Format Not supported");
	}

}

void test_main(void)
{
	ztest_test_suite(test_log_switch_format,
		ztest_unit_test(test_log_switch_format_success_case),
		ztest_unit_test(test_log_switch_format_set),
		ztest_unit_test(test_log_switch_format_set_all_active_backends),
		ztest_unit_test(test_log_switch_format_func_t_get)
		);
	ztest_run_test_suite(test_log_switch_format);
}
