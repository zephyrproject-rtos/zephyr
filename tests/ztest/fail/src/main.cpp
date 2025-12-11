/* Copyright (c) 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <iostream>
#include <regex>
#include <string>
#include <zephyr/ztest.h>

ZTEST_SUITE(fail, nullptr, nullptr, nullptr, nullptr, nullptr);

ZTEST(fail, test_framework)
{
	auto found_error_string = false;
	char buffer[sizeof(CONFIG_TEST_ERROR_STRING)] = {0};
	std::string result;

	/* Start running the target binary. This binary is expected to fail. */
	auto pipe = popen(FAIL_TARGET_BINARY, "r");

	zassert_not_null(pipe, "Failed to execute '" FAIL_TARGET_BINARY "'");

	/* Wait for the binary to finish running and grab the output */
	while (!feof(pipe)) {
		if (fgets(buffer, ARRAY_SIZE(buffer), pipe) != nullptr) {
			if (found_error_string) {
				/* Already found the error string, no need to do any more string
				 * manipulation.
				 */
				continue;
			}

			/* Append the buffer to the result string */
			result += buffer;

			/* Check if result contains the right error string */
			found_error_string |=
				(result.find(CONFIG_TEST_ERROR_STRING) != std::string::npos);

			/* If the result string is longer than the expected string,
			 * we can prune it
			 */
			auto prune_length = static_cast<int>(result.length()) -
					    static_cast<int>(sizeof(CONFIG_TEST_ERROR_STRING));
			if (prune_length > 0) {
				result.erase(0, prune_length);
			}
		}
	}
	auto rc = WEXITSTATUS(pclose(pipe));

	zassert_equal(1, rc, "Test binary expected to fail with return code 1, but got %d", rc);
	zassert_true(found_error_string, "Test binary did not produce the expected error string \""
		     CONFIG_TEST_ERROR_STRING "\"");
}
