/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "mock_backend.h"
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/ztest.h>
#include <stdlib.h>

/** Hex string corresponding to "Debug message example, %d, %d, %d", 1, 2, 3.
 * PAYLOAD_MULTIPLE_ARGS defines the expected payload
 * in SYST format when multiple arguments are passed to log API.
 */
#define PAYLOAD_MULTIPLE_ARGS "4465627567206D657373616765206578616D706C652C2025642C2025642C20256"\
	"400010000000200000003000000"

LOG_BACKEND_DEFINE(log_backend_mock, mock_log_backend_api, false);

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

/**
 * @brief Verify the correct format processing function is selected for the active output format.
 *
 * @details
 * The log output subsystem resolves the rendering function for a given output
 * format from an internal function-pointer table. This test confirms that when
 * MIPI SyS-T output is enabled the SyS-T processing function is selected, and
 * otherwise the plain text processing function is selected, ensuring log
 * messages are rendered into the configured machine-parseable format.
 *
 * Test steps:
 * - Query the format function for the active output type via log_format_func_t_get().
 * - Compare the returned pointer against the expected SyS-T or text processor.
 *
 * Expected result:
 * - The returned function pointer matches the processor for the active format.
 *
 * @see log_format_func_t_get()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 */
ZTEST(log_syst, test_log_syst_format_table_selection)
{
#ifdef CONFIG_LOG_MIPI_SYST_ENABLE
	uint32_t test_log_type_syst = LOG_OUTPUT_SYST;

	log_format_func_t test_log_output_func = log_format_func_t_get(test_log_type_syst);

	zassert_equal_ptr(test_log_output_func, log_output_msg_syst_process,
					"Correct Function pointer for SYST log\n"
					"format was not selected %p vs %p",
					test_log_output_func, log_output_msg_syst_process);

#elif CONFIG_LOG_BACKEND_MOCK_OUTPUT_DEFAULT == LOG_OUTPUT_TEXT

	uint32_t test_log_type_text = LOG_OUTPUT_TEXT;
	log_format_func_t test_log_output_func = log_format_func_t_get(test_log_type_text);

	zassert_equal_ptr(test_log_output_func, log_output_msg_process,
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

/**
 * @brief Verify a single-argument log message is rendered into the expected SyS-T payload.
 *
 * @details
 * Emits a debug message with one integer argument and validates that the mock
 * backend captures a MIPI SyS-T encoded record whose type, flags, module id,
 * sub-type and payload match the expected hex sequence. This confirms log
 * content is rendered into the structured, machine-parseable SyS-T format for
 * post processing.
 *
 * Test steps:
 * - Emit a debug log with a formatted string and one integer argument.
 * - Validate the captured SyS-T message fields and payload against expected values.
 *
 * Expected result:
 * - The captured SyS-T record matches the expected encoded message and argument.
 *
 * @see validate_msg()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 */
ZTEST(log_syst, test_log_syst_data)
{
	LOG_DBG("Debug message example, %d", 1);

	const char *sub_type = SUB_TYPE;
	const char *payload =
	"4465627567206D657373616765206578616D706C652C2025640001000000";

	validate_msg(type, optional_flags, module_id, sub_type, payload);
}

/**
 * @brief Verify a multi-argument log message is rendered into the expected SyS-T payload.
 *
 * @details
 * Emits a debug message with several integer arguments and validates that the
 * mock backend captures a MIPI SyS-T encoded record with the expected payload.
 * This exercises printf-style rendering of multiple arguments into the
 * machine-parseable SyS-T output used for post processing.
 *
 * Test steps:
 * - Emit a debug log with a formatted string and multiple integer arguments.
 * - Validate the captured SyS-T message fields and payload against expected values.
 *
 * Expected result:
 * - The captured SyS-T record matches the expected encoded message and arguments.
 *
 * @see validate_msg()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 */
ZTEST(log_syst, test_log_syst_data_multiple_args)
{
	LOG_DBG("Debug message example, %d, %d, %d", 1, 2, 3);

	const char *sub_type = SUB_TYPE;
	char *payload = PAYLOAD_MULTIPLE_ARGS;

	validate_msg(type, optional_flags, module_id, sub_type, payload);
}

/**
 * @brief Verify a floating-point log message is rendered into the expected SyS-T payload.
 *
 * @details
 * Emits a debug message with a floating-point argument and validates that the
 * mock backend captures a MIPI SyS-T encoded record with the expected payload.
 * This confirms printf-style rendering of floating-point arguments into the
 * machine-parseable SyS-T output used for post processing.
 *
 * Test steps:
 * - Emit a debug log with a formatted string and a floating-point argument.
 * - Validate the captured SyS-T message fields and payload against expected values.
 *
 * Expected result:
 * - The captured SyS-T record matches the expected encoded message and float value.
 *
 * @see validate_msg()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-3
 */
ZTEST(log_syst, test_log_syst_float_data)
{
	LOG_DBG("Debug message example, %f", 1.223);

	const char *sub_type = SUB_TYPE;
	const char *payload =
	"4465627567206D657373616765206578616D706C652C20256600C520B0726891F33F";

	validate_msg(type, optional_flags, module_id, sub_type, payload);
}

#else

/**
 * @brief Verify single-argument SyS-T rendering is skipped when SyS-T support is disabled.
 *
 * @details
 * When MIPI SyS-T output is not enabled in the build the SyS-T payload cannot
 * be produced, so this test is reported as skipped rather than failing. This
 * keeps the SyS-T post-processing checks contingent on the configured format.
 *
 * Test steps:
 * - Mark the test as skipped because SyS-T support is not enabled.
 *
 * Expected result:
 * - The test is reported as skipped.
 *
 * @see ztest_test_skip()
 * @ingroup logging_tests
 */
ZTEST(log_syst, test_log_syst_data)
{
	ztest_test_skip();
}

/**
 * @brief Verify multi-argument SyS-T rendering is skipped when SyS-T support is disabled.
 *
 * @details
 * When MIPI SyS-T output is not enabled in the build the SyS-T payload cannot
 * be produced, so this test is reported as skipped rather than failing. This
 * keeps the SyS-T post-processing checks contingent on the configured format.
 *
 * Test steps:
 * - Mark the test as skipped because SyS-T support is not enabled.
 *
 * Expected result:
 * - The test is reported as skipped.
 *
 * @see ztest_test_skip()
 * @ingroup logging_tests
 */
ZTEST(log_syst, test_log_syst_data_multiple_args)
{
	ztest_test_skip();
}

/**
 * @brief Verify floating-point SyS-T rendering is skipped when SyS-T support is disabled.
 *
 * @details
 * When MIPI SyS-T output is not enabled in the build the SyS-T payload cannot
 * be produced, so this test is reported as skipped rather than failing. This
 * keeps the SyS-T post-processing checks contingent on the configured format.
 *
 * Test steps:
 * - Mark the test as skipped because SyS-T support is not enabled.
 *
 * Expected result:
 * - The test is reported as skipped.
 *
 * @see ztest_test_skip()
 * @ingroup logging_tests
 */
ZTEST(log_syst, test_log_syst_float_data)
{
	ztest_test_skip();
}

#endif
/* test case main entry */

static void before(void *unused)
{
	const struct log_backend *backend;

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);
		if (backend == &log_backend_mock) {
			log_backend_enable(backend, NULL, 4);
		} else {
			log_backend_disable(backend);
		}
	}
}

static void after(void *unused)
{
	const struct log_backend *backend;

	for (int i = 0; i < log_backend_count_get(); i++) {
		backend = log_backend_get(i);
		if (backend == &log_backend_mock) {
			log_backend_disable(backend);
		} else {
			log_backend_enable(backend, backend->cb->ctx, 4);
		}
	}
}

ZTEST_SUITE(log_syst, NULL, NULL, before, after, NULL);
