/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/timeutil.h>
#include <zephyr/ztest.h>

/* Counts the macro is exercised with, including 0 and 1. */
#define DELAY_CYCLES_SMALL  1
#define DELAY_CYCLES_MEDIUM 8
#define DELAY_CYCLES_LARGE  64

/* Widest NOP encoding to consider when searching for the repeating unit. */
#define MAX_NOP_BYTES 8

/* Longest function prologue to skip before the first NOP. */
#define MAX_PROLOGUE_BYTES 16

/*
 * The unrolled loop must emit the same instructions as the equivalent run of
 * arch_nop() calls. Both functions are kept out of line so that their code can
 * be compared without anything else being folded into them.
 */
__attribute__((noinline)) static void delay_cycles_subject(void)
{
	timeutil_delay_cycles(DELAY_CYCLES_MEDIUM);
}

__attribute__((noinline)) static void delay_cycles_reference(void)
{
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
}

ZTEST(timeutil_delay, test_delay_cycles_zero)
{
	/* A zero count must execute no NOPs and must still compile. */
	timeutil_delay_cycles(0);
}

ZTEST(timeutil_delay, test_delay_cycles_counts)
{
	timeutil_delay_cycles(DELAY_CYCLES_SMALL);
	timeutil_delay_cycles(DELAY_CYCLES_MEDIUM);
	timeutil_delay_cycles(DELAY_CYCLES_LARGE);
}

ZTEST(timeutil_delay, test_delay_cycles_constant_expression)
{
	/* The count may be any compile-time constant expression. */
	timeutil_delay_cycles(DELAY_CYCLES_MEDIUM - DELAY_CYCLES_SMALL);
	timeutil_delay_cycles(2 * DELAY_CYCLES_SMALL);
}

/*
 * Clear the tag bit a function pointer may carry, such as the Arm Thumb bit,
 * to reach the first instruction.
 */
static const uint8_t *nop_run_base(const void *fn)
{
	return (const uint8_t *)((uintptr_t)fn & ~(uintptr_t)1);
}

/*
 * Find a run of @p count identical NOPs in @p code, reporting where the run
 * starts and how many bytes one NOP occupies. The run is the first stretch
 * built from a single repeating unit.
 */
static bool find_nop_run(const uint8_t *code, unsigned int count, size_t *offset, size_t *width)
{
	for (size_t off = 0; off <= MAX_PROLOGUE_BYTES; off++) {
		for (size_t w = 1; w <= MAX_NOP_BYTES; w++) {
			bool repeats = true;

			for (unsigned int i = 1; i < count; i++) {
				if (memcmp(&code[off], &code[off + (i * w)], w) != 0) {
					repeats = false;
					break;
				}
			}

			if (repeats) {
				*offset = off;
				*width = w;
				return true;
			}
		}
	}

	return false;
}

ZTEST(timeutil_delay, test_delay_cycles_matches_arch_nop)
{
	const uint8_t *subject = nop_run_base(&delay_cycles_subject);
	const uint8_t *reference = nop_run_base(&delay_cycles_reference);
	size_t subject_offset;
	size_t subject_width;
	size_t reference_offset;
	size_t reference_width;

	/*
	 * NOP encodings are one byte on some architectures and up to four on
	 * others, and a function may begin with a prologue such as the Xtensa
	 * entry instruction, so locate the run in each function separately.
	 */
	zassert_true(find_nop_run(subject, DELAY_CYCLES_MEDIUM, &subject_offset,
				  &subject_width),
		     "could not find the run of NOPs in the unrolled delay");
	zassert_true(find_nop_run(reference, DELAY_CYCLES_MEDIUM, &reference_offset,
				  &reference_width),
		     "could not find the run of NOPs in the reference");

	zexpect_equal(subject_width, reference_width,
		      "unrolled delay uses a %zu byte NOP, reference uses %zu",
		      subject_width, reference_width);

	zexpect_mem_equal(&subject[subject_offset], &reference[reference_offset],
			  reference_width * DELAY_CYCLES_MEDIUM,
			  "unrolled delay does not match %d arch_nop() calls",
			  DELAY_CYCLES_MEDIUM);
}

ZTEST_SUITE(timeutil_delay, NULL, NULL, NULL, NULL, NULL);
