/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <zephyr.h>
#include <logging/log.h>
#include "mock_backend.h"
#include <sys/printk.h>
#include <logging/log_backend.h>
#include <logging/log_backend_std.h>
#include <logging/log_ctrl.h>
#include <logging/log_output.h>
#include <tc_util.h>
#include <stdbool.h>
#include <ztest.h>
#include <stdlib.h>

#define TEST_MSG_0 "0 args"

MOCK_LOG_BACKEND_DEFINE(backend1, false);

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

/**
 * @brief Testcase to validate that the mock backend would use the expected
 * processing function from the function pointer table format_table
 */
void test_log_syst_format_table_selection(void)
{

#ifdef CONFIG_LOG_MIPI_SYST_ENABLE
	uint32_t test_log_type_syst = LOG_OUTPUT_SYST;

	log_format_func_t test_log_output_func = log_format_func_t_get(test_log_type_syst);

	zassert_equal_ptr(test_log_output_func, log_output_msg2_syst_process,
						"Function pointer for SYST log format was not selected");

#elif CONFIG_LOG_BACKEND_MOCK_OUTPUT_DEFAULT == LOG_OUTPUT_TEXT

	uint32_t test_log_type_text = LOG_OUTPUT_TEXT;
	log_format_func_t test_log_output_func = log_format_func_t_get(test_log_type_text);

	zassert_equal_ptr(test_log_output_func, log_output_msg2_process,
						"Function pointer for TEXT log format was not selected");

#endif

}

#ifdef CONFIG_LOG_MIPI_SYST_ENABLE

/* Testcase to validate the SYST output of log data */
void test_log_syst_data(void)
{
	LOG_DBG("Debug message example, %d", 1);

	/** Split the expected_str into 3 parts and concatenate them
	 *  to address the checkpatch warnings for very long strings
	 */
	const char *prefix = "720A000B1E000000000000000000";
	const char *body =
	"4465627567206D657373616765206578616D706C652C202564000";
	const char *postfix = "1000000";
	char *expected_str = malloc(strlen(prefix)+strlen(body)+strlen(postfix)+1);


	strcpy(expected_str, "");
	strcat(expected_str, prefix);
	strcat(expected_str, body);
	strcat(expected_str, postfix);

	validate_msg(expected_str);

	free(expected_str);
}

/* Testcase to validate the SYST output of data with multiple arguments */
void test_log_syst_data_multiple_args(void)
{
	LOG_DBG("Debug message example, %d, %d, %d", 1, 2, 3);

	const char *prefix = "720A000B2E000000000000000000";
	const char *body =
	"4465627567206D657373616765206578616D706C652C2025642C2025642C202564000";
	const char *postfix = "10000000200000003000000";
	char *expected_str = malloc(strlen(prefix)+strlen(body)+strlen(postfix)+1);

	strcpy(expected_str, "");
	strcat(expected_str, prefix);
	strcat(expected_str, body);
	strcat(expected_str, postfix);

	validate_msg(expected_str);

	free(expected_str);
}

/* Testcase to validate the SYST output of float data */
void test_log_syst_float_data(void)
{
	LOG_DBG("Debug message example, %f", 1.223);

	const char *prefix = "720A000B22000000000000000000";
	const char *body =
	"4465627567206D657373616765206578616D706C652C2025";
	const char *postfix = "6600C520B0726891F33F";
	char *expected_str = malloc(strlen(prefix)+strlen(body)+strlen(postfix)+1);

	strcpy(expected_str, "");
	strcat(expected_str, prefix);
	strcat(expected_str, body);
	strcat(expected_str, postfix);

	validate_msg(expected_str);

	free(expected_str);
}

#else
void test_log_syst_data(void)
{
	ztest_test_skip();
}

void test_log_syst_float_data(void)
{
	ztest_test_skip();
}

void test_log_syst_data_multiple_args(void)
{
	ztest_test_skip();
}

#endif
/* test case main entry */
void test_main(void)
{
	ztest_test_suite(test_log_syst,
		ztest_unit_test(test_log_syst_format_table_selection),
		ztest_unit_test(test_log_syst_data),
		ztest_unit_test(test_log_syst_float_data),
		ztest_unit_test(test_log_syst_data_multiple_args)
		);
	ztest_run_test_suite(test_log_syst);
}
