/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/toolchain.h>

/**
 * @brief Test the __counted_by() attribute
 *
 * @defgroup test_counted_by __counted_by() tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
 */

struct counted {
	uint8_t count;
	uint32_t elems[] __counted_by(count);
};

/*
 * Kept out of line so that the compiler cannot size the array from the
 * storage it lives in: the only source left is the count member.
 */
static __noinline size_t elems_size(struct counted *c)
{
#if TOOLCHAIN_HAS_COUNTED_BY
	return __builtin_dynamic_object_size(c->elems, 1);
#else
	ARG_UNUSED(c);
	return 0U;
#endif
}

/**
 * @brief Verify that the array size follows the count member
 *
 * With the attribute in effect, __builtin_dynamic_object_size() returns
 * count * sizeof(element), read from the count member at the time of the
 * call.
 */
ZTEST(counted_by, test_dynamic_object_size)
{
	static uint32_t storage[1U + 8U];
	struct counted *c = (struct counted *)storage;

	if (TOOLCHAIN_HAS_COUNTED_BY == 0) {
		ztest_test_skip();
	}

	c->count = 3U;
	zassert_equal(elems_size(c), 3U * sizeof(uint32_t));

	c->count = 8U;
	zassert_equal(elems_size(c), 8U * sizeof(uint32_t));

	c->count = 0U;
	zassert_equal(elems_size(c), 0U);
}

extern void *common_setup(void);
ZTEST_SUITE(counted_by, NULL, common_setup, NULL, NULL, NULL);
