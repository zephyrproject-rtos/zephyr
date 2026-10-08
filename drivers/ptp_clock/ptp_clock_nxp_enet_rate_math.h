/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PTP_CLOCK_PTP_CLOCK_NXP_ENET_RATE_MATH_H_
#define ZEPHYR_DRIVERS_PTP_CLOCK_PTP_CLOCK_NXP_ENET_RATE_MATH_H_

#include <stdint.h>

/**
 * @brief Find the ATINC.INC_CORR and ATCOR values for a wanted average tick.
 *
 * The ENET 1588 timer adds ATINC.INC nanoseconds on every tick of its clock, except for one
 * tick in every (ATCOR + 1), which adds ATINC.INC_CORR instead. ATCOR = 0 turns the correction
 * off. The average tick is therefore
 *
 *     avg = inc + (inc_corr - inc) / (cor + 1)      (cor != 0)
 *     avg = inc                                     (cor == 0)
 *
 * The reference manual does not state the length of the correction period. It was measured
 * on an i.MX RT1170 by running the timer against the CPU cycle counter: INC = 10, INC_CORR = 9
 * and ATCOR = 9 gives a rate of 0.990000, which is 1 - 1 / (10 * (9 + 1)), not 1 - 1 / (10 * 9).
 *
 * Each corrected tick moves the time by (inc_corr - inc) ns at once, so the time has a
 * sawtooth of that size around its average. The function tries INC_CORR - INC = 1, 2, ... and,
 * for each, the two periods around the exact one. It stops at the first pair within
 * @p tolerance_ns, or else keeps the pair with the smallest error. Since ATCOR = 0 means "no
 * correction", the shortest period is 2 ticks (ATCOR = 1).
 *
 * The correction is switched off (ATCOR = 0, INC_CORR = @p inc) when @p frac_ns is 0, or when
 * it is too small for any period that fits in ATCOR.
 *
 * @param inc          Whole nanoseconds per tick programmed in ATINC.INC.
 * @param frac_ns      Wanted average nanoseconds per tick minus @p inc. Passing the difference
 *                     rather than the average keeps the precision of a small value.
 * @param tolerance_ns Largest accepted error of the average tick, in nanoseconds.
 * @param inc_corr_max Largest value of the INC_CORR field.
 * @param cor_max      Largest value of the ATCOR field.
 * @param out_inc_corr Chosen ATINC.INC_CORR value.
 * @param out_cor      Chosen ATCOR value, which is the correction period in ticks minus one.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p frac_ns cannot be reached: it needs an INC_CORR outside 0 to
 *                 @p inc_corr_max, or a period shorter than 2 ticks, or it is not a number.
 *                 The output values are not modified.
 */
int ptp_clock_nxp_enet_find_correction(int inc, double frac_ns, double tolerance_ns,
				       int inc_corr_max, uint32_t cor_max, int *out_inc_corr,
				       uint32_t *out_cor);

#endif /* ZEPHYR_DRIVERS_PTP_CLOCK_PTP_CLOCK_NXP_ENET_RATE_MATH_H_ */
