/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MISELECT atomicity stress tests for the RISC-V IMSIC driver.
 *
 * The indirect CSR access mechanism (MISELECT+MIREG) is a two-step sequence
 * that is not inherently atomic. If an interrupt fires between writing
 * MISELECT and accessing MIREG, and the ISR also uses MISELECT, the
 * interrupted code's MIREG operation will target the wrong register.
 *
 * These tests demonstrate the vulnerability by:
 *   1. Running a timer ISR that changes MISELECT (by reading EITHRESH)
 *   2. Performing unprotected MISELECT+MIREG sequences in the main thread
 *      with an artificial delay to widen the race window
 *   3. Checking if the MIREG write landed on the wrong register (EITHRESH
 *      instead of EIE0)
 *
 * The protected variant wraps the sequence with irq_lock/irq_unlock and
 * verifies that no corruption occurs.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#include <zephyr/arch/riscv/csr.h>

#define TEST_ITERATIONS  50000
#define DELAY_LOOP_COUNT 100

ZTEST_SUITE(imsic_miselect, NULL, NULL, NULL, NULL, NULL);

/*
 * Timer ISR callback: changes MISELECT by reading EITHRESH.
 *
 * When this fires between the main thread's MISELECT write and MIREG access,
 * it leaves MISELECT pointing to ICSR_EITHRESH (0x72) instead of ICSR_EIE0
 * (0xC0). The main thread's subsequent MIREG operation then corrupts
 * EITHRESH.
 */
static void miselect_race_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	(void)micsr_read(ICSR_EITHRESH);
}

K_TIMER_DEFINE(race_timer, miselect_race_timer_handler, NULL);

/*
 * Test: unprotected MISELECT+MIREG sequence is vulnerable to corruption.
 *
 * Performs raw CSR writes with an artificial delay between MISELECT and MIREG
 * while a timer ISR aggressively changes MISELECT. The delay widens the race
 * window so the timer is likely to fire inside it.
 *
 * Expected: corruption detected (BIT(1) written to EITHRESH instead of EIE0).
 */
ZTEST(imsic_miselect, test_miselect_race_unprotected)
{
	int corruption_count = 0;

	/* Clean state */
	micsr_write(ICSR_EIE0, 0);
	micsr_write(ICSR_EITHRESH, 0);

	/* Start timer at minimum period for maximum preemption */
	k_timer_start(&race_timer, K_TICKS(1), K_TICKS(1));

	for (int i = 0; i < TEST_ITERATIONS; i++) {
		/*
		 * Unprotected sequence: no irq_lock.
		 * The timer can fire between MISELECT write and MIREG access.
		 */
		csr_write(MISELECT, ICSR_EIE0);

		/* Artificial delay to widen the race window */
		for (volatile int j = 0; j < DELAY_LOOP_COUNT; j++) {
		}

		csr_set(MIREG, BIT(1));

		/* Check for corruption: did BIT(1) land in EITHRESH? */
		unsigned long eithresh = micsr_read(ICSR_EITHRESH);

		if (eithresh != 0) {
			corruption_count++;
			/* Restore EITHRESH to 0 so system stays functional */
			micsr_write(ICSR_EITHRESH, 0);
		}

		/* Clean up EIE0 for next iteration */
		micsr_clear(ICSR_EIE0, BIT(1));
	}

	k_timer_stop(&race_timer);

	/* Final cleanup */
	micsr_write(ICSR_EIE0, 0);
	micsr_write(ICSR_EITHRESH, 0);

	TC_PRINT("Unprotected: %d corruptions in %d iterations\n", corruption_count,
		 TEST_ITERATIONS);

	zassert_true(corruption_count > 0, "Expected MISELECT corruption but none detected. "
					   "Race window may be too small for this platform.");
}

/*
 * Test: irq_lock-protected MISELECT+MIREG sequence prevents corruption.
 *
 * Same test as above, but wraps the MISELECT+MIREG sequence with
 * irq_lock/irq_unlock. The timer ISR cannot fire while interrupts are
 * disabled, so MISELECT cannot be changed between the write and the access.
 *
 * Expected: zero corruptions.
 */
ZTEST(imsic_miselect, test_miselect_race_protected)
{
	int corruption_count = 0;

	/* Clean state */
	micsr_write(ICSR_EIE0, 0);
	micsr_write(ICSR_EITHRESH, 0);

	/* Start timer at minimum period */
	k_timer_start(&race_timer, K_TICKS(1), K_TICKS(1));

	for (int i = 0; i < TEST_ITERATIONS; i++) {
		/*
		 * Protected sequence: irq_lock prevents the timer ISR
		 * from firing between MISELECT write and MIREG access.
		 */
		unsigned int key = irq_lock();

		csr_write(MISELECT, ICSR_EIE0);

		/* Same artificial delay */
		for (volatile int j = 0; j < DELAY_LOOP_COUNT; j++) {
		}

		csr_set(MIREG, BIT(1));

		irq_unlock(key);

		/* Check for corruption */
		unsigned long eithresh = micsr_read(ICSR_EITHRESH);

		if (eithresh != 0) {
			corruption_count++;
			micsr_write(ICSR_EITHRESH, 0);
		}

		/* Clean up EIE0 */
		micsr_clear(ICSR_EIE0, BIT(1));
	}

	k_timer_stop(&race_timer);

	/* Final cleanup */
	micsr_write(ICSR_EIE0, 0);
	micsr_write(ICSR_EITHRESH, 0);

	TC_PRINT("Protected: %d corruptions in %d iterations\n", corruption_count, TEST_ITERATIONS);

	zassert_equal(corruption_count, 0,
		      "MISELECT corruption detected despite irq_lock protection");
}
