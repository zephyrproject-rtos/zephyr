/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Giovanni Piccari <giopiccari@outlook.com>
 * SPDX-FileCopyrightText: Copyright 2026 M31 srl <info@m31.com>
 */

#include <zephyr/ztest.h>
#include <string.h>
#include "bc66x_utils.h"

/* Normal */
ZTEST(bc66x_parsing_suite, test_strip_quotes_standard)
{
	char input[] = "\"OK\"";

	bc66x_strip_quotes(input);
	zassert_str_equal(input, "OK", "Failed to strip standard quotes");
}

/* NULL pointer */
ZTEST(bc66x_parsing_suite, test_strip_quotes_null)
{
	bc66x_strip_quotes(NULL);
	zassert_true(true, "Survived NULL pointer injection");
}

/* Eempty string */
ZTEST(bc66x_parsing_suite, test_strip_quotes_empty_string)
{
	char input[] = "";

	bc66x_strip_quotes(input);
	zassert_str_equal(input, "", "Should not modify an empty string");
}

/* Empty quotes */
ZTEST(bc66x_parsing_suite, test_strip_quotes_empty_quotes)
{
	char input[] = "\"\"";

	bc66x_strip_quotes(input);
	zassert_str_equal(input, "", "Failed to process empty quotes");
}

/* Missing trailing quote */
ZTEST(bc66x_parsing_suite, test_strip_quotes_missing_trailing)
{
	char input[] = "\"OK";

	bc66x_strip_quotes(input);
	zassert_str_equal(input, "OK", "Failed on missing trailing quote");
}

/* Missing leading quote */
ZTEST(bc66x_parsing_suite, test_strip_quotes_missing_leading)
{
	char input[] = "OK\"";

	bc66x_strip_quotes(input);
	zassert_str_equal(input, "OK\"", "Should not modify if leading quote is missing");
}

/* Single lone quote */
ZTEST(bc66x_parsing_suite, test_strip_quotes_single_quote)
{
	char input[] = "\"";

	bc66x_strip_quotes(input);
	zassert_str_equal(input, "", "Failed on single lone quote");
}

/* No quotes at all */
ZTEST(bc66x_parsing_suite, test_strip_quotes_no_quotes)
{
	char input[] = "NORMAL_TEXT";

	bc66x_strip_quotes(input);
	zassert_str_equal(input, "NORMAL_TEXT", "Modified a string that had no quotes");
}

/* Internal quotes */
ZTEST(bc66x_parsing_suite, test_strip_quotes_internal_quotes)
{
	char input[] = "\"SOME\"TEXT\"";

	bc66x_strip_quotes(input);
	zassert_str_equal(input, "SOME\"TEXT", "Failed to preserve internal quotes");
}

ZTEST_SUITE(bc66x_parsing_suite, NULL, NULL, NULL, NULL, NULL);
