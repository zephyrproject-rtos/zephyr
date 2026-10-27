/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/timer/system_timer.h>

#ifdef CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE
ZTEST(sys_clock_hw_cycles_update, test_update_no_change_is_noop)
{
	uint32_t old_hz = sys_clock_hw_cycles_per_sec();

	z_sys_clock_hw_cycles_per_sec_update(old_hz);
	zassert_equal(sys_clock_hw_cycles_per_sec(), old_hz, "frequency changed unexpectedly");
}

ZTEST(sys_clock_hw_cycles_update, test_update_zero_is_ignored)
{
	uint32_t old_hz = sys_clock_hw_cycles_per_sec();

	z_sys_clock_hw_cycles_per_sec_update(0U);
	zassert_equal(sys_clock_hw_cycles_per_sec(), old_hz, "frequency changed unexpectedly");
}

ZTEST(sys_clock_hw_cycles_update, test_update_changes_value_is_visible_via_getter)
{
	uint32_t old_hz = sys_clock_hw_cycles_per_sec();
	uint32_t new_hz = (old_hz == 1000000U) ? 1000001U : 1000000U;

	z_sys_clock_hw_cycles_per_sec_update(new_hz);
	zassert_equal(sys_clock_hw_cycles_per_sec(), new_hz, "frequency not updated");
}
#endif /* CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE */

/* t * to / from with the given rounding offset, as z_tmcvt_gen_64_slow() */
static uint64_t ref_conv(uint64_t t, uint32_t from, uint32_t to, uint32_t off)
{
	return ((t / from) * to) + ((((t % from) * to) + off) / from);
}

static bool same(const char *conv, uint64_t c, uint64_t got, uint64_t want)
{
	if (got != want) {
		TC_PRINT("%s(%llu) at %u Hz: %llu, expected %llu\n", conv, c,
			 sys_clock_hw_cycles_per_sec(), got, want);
		return false;
	}
	return true;
}

/* Conversions from cycles at the current rate match plain division. Prints
 * each mismatch.
 */
static bool from_cycles_ok(void)
{
	static const uint64_t values[] = {
		0U, 1U, 999U, 123456789U, UINT32_MAX, 0x123456789abcULL,
	};
	uint32_t hz = sys_clock_hw_cycles_per_sec();
	bool ok = true;

	ARRAY_FOR_EACH(values, i) {
		uint64_t c = values[i];

		ok = same("k_cyc_to_ms_floor64", c, k_cyc_to_ms_floor64(c),
			  ref_conv(c, hz, 1000U, 0U)) && ok;
		ok = same("k_cyc_to_us_ceil64", c, k_cyc_to_us_ceil64(c),
			  ref_conv(c, hz, 1000000U, hz - 1U)) && ok;
		ok = same("k_cyc_to_ns_near64", c, k_cyc_to_ns_near64(c),
			  ref_conv(c, hz, 1000000000U, hz / 2U)) && ok;
		ok = same("k_cyc_to_ticks_floor64", c, k_cyc_to_ticks_floor64(c),
			  ref_conv(c, hz, CONFIG_SYS_CLOCK_TICKS_PER_SEC, 0U)) && ok;
		if (c <= UINT32_MAX) {
			ok = same("k_cyc_to_ms_floor32", c, k_cyc_to_ms_floor32(c),
				  (uint32_t)((c * 1000U) / hz)) && ok;
			ok = same("k_cyc_to_ms_near32", c, k_cyc_to_ms_near32(c),
				  (uint32_t)(((c * 1000U) + (hz / 2U)) / hz)) && ok;
			ok = same("k_cyc_to_us_ceil32", c, k_cyc_to_us_ceil32(c),
				  (uint32_t)(((c * 1000000U) + (hz - 1U)) / hz)) && ok;
		}
	}
	return ok;
}

static ZTEST_BMEM bool user_ok;

static void user_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	user_ok = from_cycles_ok();
}

/* from_cycles_ok() in a user thread. On 32-bit targets with CONFIG_USERSPACE,
 * the conversions there get the reciprocal of the rate through a system call.
 */
static bool user_from_cycles_ok(void)
{
	static K_THREAD_STACK_DEFINE(stack, 2048);
	static struct k_thread thread;

	user_ok = false;
	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), user_entry, NULL, NULL,
			NULL, k_thread_priority_get(k_current_get()), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);
	k_thread_join(&thread, K_FOREVER);
	return user_ok;
}

static void set_rate(uint32_t hz)
{
#ifdef CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE
	z_sys_clock_hw_cycles_per_sec_update(hz);
#else
	/* What a driver that reads its frequency at run time does at init */
	z_sys_clock_hw_cycles_per_sec_set(hz);
#endif
}

ZTEST(sys_clock_hw_cycles_update, test_conversions_follow_update)
{
	static const uint32_t rates[] = {
		32768U, 1000000U, 19200000U, 48000000U, 0x80000001U, UINT32_MAX,
	};

	ARRAY_FOR_EACH(rates, i) {
		set_rate(rates[i]);
		zassert_equal(sys_clock_hw_cycles_per_sec(), rates[i], "frequency not updated");
		zassert_true(from_cycles_ok(), "%u Hz", rates[i]);
		zassert_true(user_from_cycles_ok(), "%u Hz in a user thread", rates[i]);
	}
}

/* A conversion from a variable rate other than the cycle rate divides by it */
ZTEST(sys_clock_hw_cycles_update, test_other_rate)
{
	volatile uint32_t hz = 32768U;

	zassert_not_equal(sys_clock_hw_cycles_per_sec(), hz);
	zassert_equal(z_tmcvt_64(98304U, hz, Z_HZ_ns, false, false, true), 3000000000ULL);
	zassert_equal(z_tmcvt_32(98304U, hz, Z_HZ_ms, false, false, true), 3000U);
}

/* Two rates whose bytes all differ, so a torn read gives neither */
#define TEAR_HZ_A 0x11111111U
#define TEAR_HZ_B 0x22EE22EEU

static void flip_rate(void *p1, void *p2, void *p3)
{
	atomic_t *done = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (uint32_t i = 0U; i < 1000000U; i++) {
		z_sys_clock_hw_cycles_per_sec_set(((i & 1U) != 0U) ? TEAR_HZ_A : TEAR_HZ_B);
	}
	atomic_set(done, 1);
}

/* With another CPU changing the rate, the rate matches one of the rates set.
 * So do the conversions from cycles on 32-bit targets, which take one copy of
 * the rate reciprocal. 64-bit targets may read the rate more than once per
 * conversion.
 */
ZTEST(sys_clock_hw_cycles_update, test_rate_not_torn)
{
	static K_THREAD_STACK_DEFINE(stack, 1024);
	static struct k_thread thread;
	const uint64_t c = 0x123456789abcULL;
	atomic_t done = ATOMIC_INIT(0);
	uint32_t torn_hz = 0U;
	uint32_t torn_conv = 0U;

	if (!IS_ENABLED(CONFIG_SMP) || (arch_num_cpus() < 2U)) {
		ztest_test_skip();
	}

	z_sys_clock_hw_cycles_per_sec_set(TEAR_HZ_A);
	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), flip_rate, &done, NULL,
			NULL, k_thread_priority_get(k_current_get()), 0, K_NO_WAIT);

	while (atomic_get(&done) == 0) {
		uint32_t hz = sys_clock_hw_cycles_per_sec();
		uint64_t ms = k_cyc_to_ms_floor64(c);

		if ((hz != TEAR_HZ_A) && (hz != TEAR_HZ_B)) {
			torn_hz++;
		}
		if (!IS_ENABLED(CONFIG_64BIT) &&
		    (ms != ref_conv(c, TEAR_HZ_A, 1000U, 0U)) &&
		    (ms != ref_conv(c, TEAR_HZ_B, 1000U, 0U))) {
			torn_conv++;
		}
	}
	k_thread_join(&thread, K_FOREVER);

	zassert_equal(torn_hz, 0U, "%u torn rates", torn_hz);
	zassert_equal(torn_conv, 0U, "%u torn conversions", torn_conv);
}

ZTEST_USER(sys_clock_hw_cycles_update, test_user_conversions)
{
	zassert_true(from_cycles_ok());
}

static uint32_t boot_hz;

static void *suite_setup(void)
{
	boot_hz = sys_clock_hw_cycles_per_sec();
	return NULL;
}

/* Restore the boot rate, also after a failed test */
static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	set_rate(boot_hz);
}

ZTEST_SUITE(sys_clock_hw_cycles_update, NULL, suite_setup, NULL, after, NULL);
