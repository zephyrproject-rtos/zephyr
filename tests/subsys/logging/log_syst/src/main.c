/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>
#include "mock_backend.h"
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <tc_util.h>
#include <stdbool.h>
#include <ztest.h>
#include <stdlib.h>

/** Hex string corresponding to "Debug message example, %d, %d, %d", 1, 2, 3.
 * PAYLOAD_MULTIPLE_ARGS defines the expected payload
 * in SYST format when multiple arguments are passed to log API.
 */
#define PAYLOAD_MULTIPLE_ARGS "4465627567206D657373616765206578616D706C652C2025642C2025642C20256"\
	"400010000000200000003000000"

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
					"Correct Function pointer for SYST log\n"
					"format was not selected %p vs %p",
					test_log_output_func, log_output_msg2_syst_process);

#elif CONFIG_LOG_BACKEND_MOCK_OUTPUT_DEFAULT == LOG_OUTPUT_TEXT

	uint32_t test_log_type_text = LOG_OUTPUT_TEXT;
	log_format_func_t test_log_output_func = log_format_func_t_get(test_log_type_text);

	zassert_equal_ptr(test_log_output_func, log_output_msg2_process,
						"Function pointer for TEXT log format was not selected");

#endif

}

#ifdef CONFIG_LOG_MIPI_SYST_ENABLE

const char *type = "72";
const char *optional_flags = "0A";
const char *module_id = "00";

#ifdef CONFIG_64BIT
#define SUB_TYPE "0C"
#else
#define SUB_TYPE "0B"
#endif

/* Testcase to validate the SYST output of log data */
void test_log_syst_data(void)
{
	LOG_DBG("Debug message example, %d", 1);

	const char *sub_type = SUB_TYPE;
	const char *payload =
	"4465627567206D657373616765206578616D706C652C2025640001000000";

	validate_msg(type, optional_flags, module_id, sub_type, payload);
}

/* Testcase to validate the SYST output of data with multiple arguments */
void test_log_syst_data_multiple_args(void)
{
	LOG_DBG("Debug message example, %d, %d, %d", 1, 2, 3);

	const char *sub_type = SUB_TYPE;
	char *payload = PAYLOAD_MULTIPLE_ARGS;

	validate_msg(type, optional_flags, module_id, sub_type, payload);
}

/* Testcase to validate the SYST output of float data */
void test_log_syst_float_data(void)
{
	LOG_DBG("Debug message example, %f", 1.223);

	const char *sub_type = SUB_TYPE;
	const char *payload =
	"4465627567206D657373616765206578616D706C652C20256600C520B0726891F33F";

	validate_msg(type, optional_flags, module_id, sub_type, payload);
}

#else

void test_log_syst_data(void)
{
	ztest_test_skip();
}

void test_log_syst_data_multiple_args(void)
{
	ztest_test_skip();
}

void test_log_syst_float_data(void)
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
