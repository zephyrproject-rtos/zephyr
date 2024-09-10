/* Copyright (c) 2024 Linaro LTD */
/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/kernel.h>

/* Rather than trying to get C enums to match in size with Rust ones, just use a known integet type.
 */
enum units {
	UNIT_FOREVER,
	UNIT_NO_WAIT,
	UNIT_DUR_MSEC,
	UNIT_INST_MSEC,
};

/* Data handed back from C containing processed time constant values.
 */
struct time_entry {
	const char *name;

	uint32_t units;

	/* Value in the given units. */
	int64_t uvalue;

	/* Value in ticks. */
	k_timeout_t value;
};

const struct time_entry time_entries[] = {
	/* For the constants, only the `.value` gets used by the test. */
	{
		.name = "K_FOREVER",
		.units = UNIT_FOREVER,
		.value = K_FOREVER,
	},
	{
		.name = "K_NO_WAIT",
		.units = UNIT_NO_WAIT,
		.value = K_NO_WAIT,
	},
#define DUR_TEST(unit, n) \
	{ \
		.name = "Duration " #unit " " #n, \
		.units = UNIT_DUR_ ## unit, \
		.uvalue = n, \
		.value = K_ ## unit(n), \
	}
	/* Test various values near typical clock boundaries. */
	DUR_TEST(MSEC, 1),
	DUR_TEST(MSEC, 2),
	DUR_TEST(MSEC, 99),
	DUR_TEST(MSEC, 100),
	DUR_TEST(MSEC, 101),
	DUR_TEST(MSEC, 999),
	DUR_TEST(MSEC, 1000),
	DUR_TEST(MSEC, 1001),
	DUR_TEST(MSEC, 32767),
	DUR_TEST(MSEC, 32768),
	DUR_TEST(MSEC, 32769),
	/* The Instance tests don't set the `.value` because it isn't constant, and the test code
	 * will calculate the value at runtime, using the conversion functions below.
	 */
#define INST_TEST(unit, n) \
	{ \
		.name = "Instant " #unit " " #n, \
		.units = UNIT_INST_ ## unit, \
		.uvalue = n, \
	}
	INST_TEST(MSEC, 1),
	INST_TEST(MSEC, 2),
	INST_TEST(MSEC, 99),
	INST_TEST(MSEC, 100),
	INST_TEST(MSEC, 101),
	INST_TEST(MSEC, 999),
	INST_TEST(MSEC, 1000),
	INST_TEST(MSEC, 1001),
	INST_TEST(MSEC, 32767),
	INST_TEST(MSEC, 32768),
	INST_TEST(MSEC, 32769),
	{
		.name = 0,
	},
};

/* Return the indexed time entry.  It is up to the Rust code to detect the null name, and handle it
 * properly.
 */
const struct time_entry *get_time_entry(uintptr_t index)
{
	return &time_entries[index];
}

/* The abs timeout is not constant, so provide this wrapper function.
 */
const k_timeout_t ms_to_abs_timeout(int64_t ms)
{
	return K_TIMEOUT_ABS_MS(ms);
}
