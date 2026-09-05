/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/safety_test/safety_test.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/crc.h>
#include <zephyr/toolchain.h>

#include "sample_common.h"

/*
 * Operands are volatile so the compiler cannot fold these expressions into
 * their answers at build time, which would leave the test checking nothing.
 */
static int cpu_core_run(const struct safety_test_context *ctx)
{
	volatile uint32_t a = 0xA5A5A5A5U;
	volatile uint32_t b = 0x5A5A5A5AU;
	volatile uint32_t m = 0x00010001U;
	volatile int32_t s = -12345;

	ARG_UNUSED(ctx);

	if ((a | b) != 0xFFFFFFFFU) {
		return -EIO;
	}

	if ((a & b) != 0U) {
		return -EIO;
	}

	if ((a ^ b) != 0xFFFFFFFFU) {
		return -EIO;
	}

	if ((uint32_t)~a != 0x5A5A5A5AU) {
		return -EIO;
	}

	if ((a + b) != 0xFFFFFFFFU) {
		return -EIO;
	}

	if ((b - a) != 0xB4B4B4B5U) {
		return -EIO;
	}

	if ((a >> 4) != 0x0A5A5A5AU) {
		return -EIO;
	}

	if ((a << 4) != 0x5A5A5A50U) {
		return -EIO;
	}

	if ((m * m) != 0x00020001U) {
		return -EIO;
	}

	if ((a / 3U) != 0x37373737U) {
		return -EIO;
	}

	if ((s / 100) != -123) {
		return -EIO;
	}

	if ((s % 100) != -45) {
		return -EIO;
	}

	return 0;
}

SAFETY_TEST_DEFINE(cpu_core, SAFETY_TEST_CAT_CPU, SAFETY_TEST_LEVEL_EARLY, 10,
		   SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_RUNTIME_OK |
			   SAFETY_TEST_FLAG_CRITICAL,
		   cpu_core_run, "CPU data path check");

#define SAMPLE_SCRATCH_WORDS 256U

struct sample_scratch {
	int32_t history_mc[SAMPLE_HISTORY_LEN];
	uint32_t history_count;
	uint32_t filler[SAMPLE_SCRATCH_WORDS - SAMPLE_HISTORY_LEN - 1U];
};

/*
 * __noinit so the march is not fighting the C runtime over the same memory, and
 * so it can run at PRE_KERNEL_2 without destroying initialised data.
 */
static __noinit struct sample_scratch scratch;

BUILD_ASSERT(sizeof(scratch) == SAMPLE_SCRATCH_WORDS * sizeof(uint32_t),
	     "scratch block must be a whole number of words");

void sample_history_append(int32_t temp_mc)
{
	scratch.history_mc[scratch.history_count % SAMPLE_HISTORY_LEN] = temp_mc;
	scratch.history_count++;
}

uint32_t sample_history_count(void)
{
	return scratch.history_count;
}

void sample_history_reset(void)
{
	scratch.history_count = 0U;
}

/*
 * March C-: w0 / r0w1 / r1w0 up, r0w1 / r1w0 down, r0 up. Detects stuck-at,
 * transition and coupling faults. The pointer is volatile so the read-backs
 * survive optimisation; without it the compiler removes the entire test.
 */
static int march_c_minus(volatile uint32_t *base, size_t words)
{
	size_t i;

	for (i = 0U; i < words; i++) {
		base[i] = 0U;
	}

	for (i = 0U; i < words; i++) {
		if (base[i] != 0U) {
			return -EIO;
		}
		base[i] = UINT32_MAX;
	}

	for (i = 0U; i < words; i++) {
		if (base[i] != UINT32_MAX) {
			return -EIO;
		}
		base[i] = 0U;
	}

	for (i = words; i-- > 0U;) {
		if (base[i] != 0U) {
			return -EIO;
		}
		base[i] = UINT32_MAX;
	}

	for (i = words; i-- > 0U;) {
		if (base[i] != UINT32_MAX) {
			return -EIO;
		}
		base[i] = 0U;
	}

	for (i = 0U; i < words; i++) {
		if (base[i] != 0U) {
			return -EIO;
		}
	}

	return 0;
}

static int ram_march_run(const struct safety_test_context *ctx)
{
	ARG_UNUSED(ctx);

	return march_c_minus((volatile uint32_t *)&scratch, SAMPLE_SCRATCH_WORDS);
}

SAFETY_TEST_DEFINE_EX(ram_march, SAFETY_TEST_CAT_RAM, SAFETY_TEST_LEVEL_PRE_KERNEL_2, 10,
		      SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_RUNTIME_OK |
			      SAFETY_TEST_FLAG_DESTRUCTIVE,
		      ram_march_run, "March C- over the sample's scratch RAM", NULL, 2000);
/*
 * Expected CRC-32 of the text region, supplied by the second build pass (see
 * README). It lives in .rodata, which sits outside the region it describes, so
 * writing the value back cannot change the bytes being checked (that is what
 * makes the two passes converge with no iteration and no binary patching).
 */
static volatile const uint32_t sample_text_crc = CONFIG_SAMPLE_IMAGE_CRC;

static int flash_integrity_run(const struct safety_test_context *ctx)
{
	const uint8_t *start = (const uint8_t *)__text_region_start;
	const uint8_t *end = (const uint8_t *)__text_region_end;
	uint32_t crc;

	ARG_UNUSED(ctx);

	/* First pass: nothing to compare against yet. Distinct from a mismatch. */
	if (sample_text_crc == 0U) {
		return -ENODATA;
	}

	/*
	 * One contiguous span. The image is XIP, so this is a plain memory read
	 * needing no flash driver, which is what makes PRE_KERNEL_1 viable.
	 */
	crc = crc32_ieee(start, (size_t)(end - start));

	if (crc != sample_text_crc) {
		return -EILSEQ;
	}

	return 0;
}

SAFETY_TEST_DEFINE(flash_integrity, SAFETY_TEST_CAT_FLASH, SAFETY_TEST_LEVEL_PRE_KERNEL_1, 10,
		   SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_RUNTIME_OK,
		   flash_integrity_run, "CRC-32 over the text region");

static const struct device *const clock_ref = DEVICE_DT_GET(DT_ALIAS(clock_ref));

/*
 * The POST_KERNEL sweep runs at CONFIG_SAFETY_TEST_INIT_PRIORITY; the counter
 * driver initialises at CONFIG_COUNTER_INIT_PRIORITY. The whole test depends on
 * the second happening first, so break the build if anyone changes either.
 */
BUILD_ASSERT(CONFIG_SAFETY_TEST_INIT_PRIORITY > CONFIG_COUNTER_INIT_PRIORITY,
	     "the safety sweep must run after the counter driver initialises");

/*
 * Compares the counter's own oscillator against the main clock.
 *
 * The reference is not precise, so this catches gross faults only and is not
 * frequency calibration.
 */
static int clock_xcheck_run(const struct safety_test_context *ctx)
{
	uint32_t t0, t1, ticks, top, nominal_hz, observed_hz, delta;
	uint64_t c0, c1, elapsed_us;
	int ret;

	ARG_UNUSED(ctx);

	if (!device_is_ready(clock_ref)) {
		return -ENODEV;
	}

	if (!counter_is_counting_up(clock_ref)) {
		return -ENOTSUP;
	}

	ret = counter_start(clock_ref);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	ret = counter_get_value(clock_ref, &t0);
	if (ret < 0) {
		return ret;
	}
	c0 = k_cycle_get_64();

	k_msleep(CONFIG_SAMPLE_CLOCK_WINDOW_MS);

	ret = counter_get_value(clock_ref, &t1);
	if (ret < 0) {
		return ret;
	}
	c1 = k_cycle_get_64();

	elapsed_us = k_cyc_to_us_floor64(c1 - c0);
	if (elapsed_us == 0U) {
		return -EIO;
	}

	top = counter_get_top_value(clock_ref);
	ticks = (t1 >= t0) ? (t1 - t0) : (top - t0 + t1 + 1U);
	if (ticks == 0U) {
		/* A reference that never moved is exactly what this test is for. */
		return -ENOTCONN;
	}

	nominal_hz = counter_get_frequency(clock_ref);
	if (nominal_hz == 0U) {
		return -EIO;
	}

	observed_hz = (uint32_t)(((uint64_t)ticks * USEC_PER_SEC) / elapsed_us);
	delta = (observed_hz > nominal_hz) ? (observed_hz - nominal_hz)
					  : (nominal_hz - observed_hz);

	if ((uint64_t)delta * 100U >
	    (uint64_t)nominal_hz * (uint64_t)CONFIG_SAMPLE_CLOCK_TOLERANCE_PCT) {
		return -ERANGE;
	}

	return 0;
}

/*
 * No duration budget: this test sleeps for its measurement window on purpose,
 * so any budget would flag the thing it is supposed to do.
 */
SAFETY_TEST_DEFINE(clock_xcheck, SAFETY_TEST_CAT_CLOCK, SAFETY_TEST_LEVEL_POST_KERNEL, 10,
		   SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_RUNTIME_OK |
			   SAFETY_TEST_FLAG_CRITICAL,
		   clock_xcheck_run, "LPTMR cross-check against the system clock");
