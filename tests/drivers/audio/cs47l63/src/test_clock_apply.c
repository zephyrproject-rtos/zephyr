/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_clock_apply.c
 * @brief The register writes cs47l63_clock_apply() produces
 *
 * The nominal case throughout is the board's own: a 6.144 MHz reference on
 * MCLK1 taken to the 49.152 MHz the part requires of an FLL that feeds SYSCLK
 * for the 48 kHz sample-rate family (DS1249F2 section 4.10.7.4). The values
 * asserted are hand-derived from that ratio and from the field tables of
 * DS1249F2 table 4-48, not read back out of the solver.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "cs47l63_clock.h"
#include "cs47l63_regs.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_clock))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_clock))

/** The board's reference: the SoC's I2S master clock. */
#define FREF_HZ 6144000U

/*
 * The solution 6.144 MHz to 49.152 MHz has to produce, term by term. The ratio
 * is exactly 8, so the loop runs in its integer mode: N = 8, FB_DIV = 1, and
 * the fractional term is 1/1 with nothing left over.
 */
#define EXPECT_N           8U
#define EXPECT_FB_DIV      1U
#define EXPECT_LAMBDA      1U
#define EXPECT_THETA       0U
#define EXPECT_LOCKDET_THR 8U
#define EXPECT_HP          1U
#define EXPECT_GAINS       0x21F0U

#define EXPECT_CONTROL2                                                                            \
	((EXPECT_LOCKDET_THR << CS47L63_FLL1_LOCKDET_THR_SHIFT) | CS47L63_FLL1_LOCKDET |           \
	 CS47L63_FLL1_PHASEDET | CS47L63_FLL1_REFDET |                                             \
	 ((uint32_t)CS47L63_FLL_SRC_MCLK1 << CS47L63_FLL1_REFCLK_SRC_SHIFT) |                      \
	 (EXPECT_N << CS47L63_FLL1_N_SHIFT))

#define EXPECT_CONTROL3                                                                            \
	((EXPECT_LAMBDA << CS47L63_FLL1_LAMBDA_SHIFT) | (EXPECT_THETA << CS47L63_FLL1_THETA_SHIFT))

#define EXPECT_CONTROL4                                                                            \
	((EXPECT_GAINS << CS47L63_FLL1_GAIN_SHIFT) | (EXPECT_HP << CS47L63_FLL1_HP_SHIFT) |        \
	 (EXPECT_FB_DIV << CS47L63_FLL1_FB_DIV_SHIFT))

/** SYSCLK_SRC 0x0C is FLL1 at 45 to 50 MHz; SYSCLK_FREQ 011 names 49.152 MHz. */
#define EXPECT_SYSTEM_CLOCK1                                                                       \
	(((uint32_t)CS47L63_SYSCLK_FREQ_49M152 << CS47L63_SYSCLK_FREQ_SHIFT) | CS47L63_SYSCLK_EN | \
	 CS47L63_SYSCLK_SRC_FLL1)

static struct cs47l63_fll_solution s_sol;

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	zassert_ok(
		cs47l63_clock_solve(FREF_HZ, CS47L63_FLL_FOUT_HZ, CS47L63_FLL_SRC_MCLK1, &s_sol));
}

ZTEST_SUITE(cs47l63_clock_apply, NULL, NULL, before_each, NULL, NULL);

ZTEST(cs47l63_clock_apply, test_control_registers_carry_the_solved_terms)
{
	uint32_t val;

	zassert_ok(cs47l63_clock_apply(CODEC, &s_sol));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_FLL1_CONTROL2), EXPECT_CONTROL2,
		      "FLL1_CONTROL2 must carry LOCKDET, PHASEDET, REFDET, the threshold, the "
		      "reference divider and N in one word");
	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_FLL1_CONTROL3, 0, &val),
		     "FLL1_CONTROL3 must be written");
	zassert_equal(val, EXPECT_CONTROL3, "LAMBDA in the upper half, THETA in the lower");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_FLL1_CONTROL4), EXPECT_CONTROL4,
		      "FLL1_CONTROL4 must carry the gain word, the loop mode and FB_DIV");
}

ZTEST(cs47l63_clock_apply, test_system_clock1_names_fll1_and_49m152_in_one_word)
{
	uint32_t val = cs47l63_emul_get_reg(EMUL, CS47L63_SYSTEM_CLOCK1);

	zassert_ok(cs47l63_clock_apply(CODEC, &s_sol));

	val = cs47l63_emul_get_reg(EMUL, CS47L63_SYSTEM_CLOCK1);

	zassert_equal(val & CS47L63_SYSCLK_SRC_MASK, CS47L63_SYSCLK_SRC_FLL1,
		      "SYSCLK_SRC must be 0x0C, FLL1 at 45 to 50 MHz");
	zassert_equal((val & CS47L63_SYSCLK_FREQ_MASK) >> CS47L63_SYSCLK_FREQ_SHIFT,
		      CS47L63_SYSCLK_FREQ_49M152,
		      "SYSCLK_FREQ must be 011, and it must name the rate that same source "
		      "delivers");
	zassert_equal(val & CS47L63_SYSCLK_FRAC, 0,
		      "SYSCLK_FRAC selects the 6.144 MHz family this driver uses");
	zassert_not_equal(val & CS47L63_SYSCLK_EN, 0, "SYSCLK must be enabled");
	zassert_equal(val, EXPECT_SYSTEM_CLOCK1, "source and frequency go out in one write");
}

ZTEST(cs47l63_clock_apply, test_loop_is_held_before_any_other_fll_register_is_written)
{
	int hold_idx;
	uint32_t val;

	zassert_ok(cs47l63_clock_apply(CODEC, &s_sol));

	hold_idx = cs47l63_emul_write_index(EMUL, CS47L63_FLL1_CONTROL1, 0);
	zassert_true(hold_idx >= 0, "FLL1_CONTROL1 must be written first of all");

	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_FLL1_CONTROL1, 0, &val));
	zassert_equal(val, CS47L63_FLL1_HOLD,
		      "the first write must assert the hold and nothing else");

	zassert_true(hold_idx < cs47l63_emul_write_index(EMUL, CS47L63_FLL1_CONTROL2, 0),
		     "FLL1_CONTROL2 must be written under the hold");
	zassert_true(hold_idx < cs47l63_emul_write_index(EMUL, CS47L63_FLL1_CONTROL3, 0),
		     "FLL1_CONTROL3 must be written under the hold");
	zassert_true(hold_idx < cs47l63_emul_write_index(EMUL, CS47L63_FLL1_CONTROL4, 0),
		     "FLL1_CONTROL4 must be written under the hold");
}

ZTEST(cs47l63_clock_apply, test_enable_then_latch_then_release_in_that_order)
{
	uint32_t val;

	zassert_ok(cs47l63_clock_apply(CODEC, &s_sol));

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_FLL1_CONTROL1), 4,
		      "hold, enable, latch, release: four writes and no more");

	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_FLL1_CONTROL1, 1, &val));
	zassert_equal(val, CS47L63_FLL1_HOLD | CS47L63_FLL1_EN,
		      "the loop is enabled while still held");

	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_FLL1_CONTROL1, 2, &val));
	zassert_equal(val, CS47L63_FLL1_HOLD | CS47L63_FLL1_EN | CS47L63_FLL1_CTRL_UPD,
		      "the whole set is latched before the hold goes away");

	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_FLL1_CONTROL1, 3, &val));
	zassert_equal(val, CS47L63_FLL1_EN | CS47L63_FLL1_CTRL_UPD,
		      "the hold is released last, leaving the loop enabled");
}

ZTEST(cs47l63_clock_apply, test_sysclk_is_brought_up_after_the_loop_is_released)
{
	int release_idx;
	int sysclk_idx;

	zassert_ok(cs47l63_clock_apply(CODEC, &s_sol));

	release_idx = cs47l63_emul_write_index(EMUL, CS47L63_FLL1_CONTROL1, 3);
	sysclk_idx = cs47l63_emul_write_index(EMUL, CS47L63_SYSTEM_CLOCK1, 0);

	zassert_true(release_idx >= 0 && sysclk_idx > release_idx,
		     "SYSCLK is selected onto a loop that is already running");
}

ZTEST(cs47l63_clock_apply, test_does_not_wait_for_lock)
{
	/* The reference is the SoC's I2S master clock, which is not running at
	 * configure time, so the loop cannot be locked here. Applying must
	 * still succeed, and must not read the lock status at all.
	 */
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, 0);

	zassert_ok(cs47l63_clock_apply(CODEC, &s_sol));

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_STS_6), 0,
		      "apply must not consult a lock bit that cannot yet be meaningful");
}

ZTEST(cs47l63_clock_apply, test_lock_status_reads_the_live_bit)
{
	bool locked = true;

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, 0);
	zassert_ok(cs47l63_clock_locked(CODEC, &locked));
	zassert_false(locked, "FLL1_LOCK_STS1 clear must read as unlocked");

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	zassert_ok(cs47l63_clock_locked(CODEC, &locked));
	zassert_true(locked, "FLL1_LOCK_STS1 set must read as locked");

	zassert_equal(-EINVAL, cs47l63_clock_locked(CODEC, NULL));
}

ZTEST(cs47l63_clock_apply, test_null_solution_rejected)
{
	zassert_equal(-EINVAL, cs47l63_clock_apply(CODEC, NULL));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), 0, "a rejected apply writes nothing");
}

ZTEST(cs47l63_clock_apply, test_bus_failure_leaves_the_loop_held)
{
	uint32_t val;

	cs47l63_emul_fail_at(EMUL, CS47L63_FLL1_CONTROL3);

	zassert_equal(-EIO, cs47l63_clock_apply(CODEC, &s_sol),
		      "the failing transaction's errno must reach the caller");

	/* The injection took effect: the register the fault was armed on was
	 * never written, so the failure below is the one that was injected.
	 */
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_FLL1_CONTROL3), 0,
		      "the armed write must not have landed");

	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_FLL1_CONTROL1), 1,
		      "only the hold was written before the failure");
	zassert_true(cs47l63_emul_nth_write(EMUL, CS47L63_FLL1_CONTROL1, 0, &val));
	zassert_equal(val & CS47L63_FLL1_HOLD, CS47L63_FLL1_HOLD,
		      "a half-configured loop must be left held, not left running");
	zassert_equal(val & CS47L63_FLL1_EN, 0, "and must not have been enabled");
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_SYSTEM_CLOCK1), 0,
		      "SYSCLK must not be brought up on a loop that was never configured");
}
