/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/bit_rev.h>
#include <zephyr/ztest.h>

struct bit_rev32_case {
	uint32_t input;
	uint32_t expected;
};

struct bit_rev16_case {
	uint16_t input;
	uint16_t expected;
};

struct bit_rev8_case {
	uint8_t input;
	uint8_t expected;
};

static const struct bit_rev32_case bit_rev32_cases[] = {
	{ 0x00000000U, 0x00000000U },
	{ 0x00000001U, 0x80000000U },
	{ 0x80000000U, 0x00000001U },
	{ 0xAAAAAAAAU, 0x55555555U },
	{ 0x12345678U, 0x1E6A2C48U },
	{ 0x0F0F00FFU, 0xFF00F0F0U },
};

static const struct bit_rev16_case bit_rev16_cases[] = {
	{ 0x0000U, 0x0000U },
	{ 0x0001U, 0x8000U },
	{ 0x8000U, 0x0001U },
	{ 0x55AAU, 0x55AAU },
	{ 0x1234U, 0x2C48U },
	{ 0x00F1U, 0x8F00U },
};

static const struct bit_rev8_case bit_rev8_cases[] = {
	{ 0x00U, 0x00U },
	{ 0x01U, 0x80U },
	{ 0x80U, 0x01U },
	{ 0x55U, 0xAAU },
	{ 0x3CU, 0x3CU },
	{ 0x96U, 0x69U },
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(bit_rev32_params, bit_rev32_cases);
ZTEST_DEFINE_PARAM_VALUES_ARRAY(bit_rev16_params, bit_rev16_cases);
ZTEST_DEFINE_PARAM_VALUES_ARRAY(bit_rev8_params, bit_rev8_cases);

ZTEST_P(bit_rev, test_sys_bit_rev32)
{
	const struct bit_rev32_case *tc = ZTEST_GET_PARAM_PTR(struct bit_rev32_case);

	zexpect_equal(sys_bit_rev32(tc->input), tc->expected,
		      "Unexpected 32-bit reversal for 0x%08x", tc->input);
	zexpect_equal(sys_bit_rev32(tc->expected), tc->input,
		      "32-bit reversal should be symmetric for 0x%08x", tc->input);
}

ZTEST_P(bit_rev, test_sys_bit_rev16)
{
	const struct bit_rev16_case *tc = ZTEST_GET_PARAM_PTR(struct bit_rev16_case);

	zexpect_equal(sys_bit_rev16(tc->input), tc->expected,
		      "Unexpected 16-bit reversal for 0x%04x", tc->input);
	zexpect_equal(sys_bit_rev16(tc->expected), tc->input,
		      "16-bit reversal should be symmetric for 0x%04x", tc->input);
}

ZTEST_P(bit_rev, test_sys_bit_rev8)
{
	const struct bit_rev8_case *tc = ZTEST_GET_PARAM_PTR(struct bit_rev8_case);

	zexpect_equal(sys_bit_rev8(tc->input), tc->expected,
		      "Unexpected 8-bit reversal for 0x%02x", tc->input);
	zexpect_equal(sys_bit_rev8(tc->expected), tc->input,
		      "8-bit reversal should be symmetric for 0x%02x", tc->input);
}

ZTEST_SUITE(bit_rev, NULL, NULL, NULL, NULL, NULL);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, bit_rev, test_sys_bit_rev32, bit_rev32_params);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, bit_rev, test_sys_bit_rev16, bit_rev16_params);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, bit_rev, test_sys_bit_rev8, bit_rev8_params);
