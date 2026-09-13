/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_clock.c
 * @brief CS47L63 FLL1 solver and apply path
 *
 * The solver reproduces the FLLHJ term selection of the Apache-2.0 vendor
 * driver (modules/hal/cirrus-logic/cs47l63/cs47l63.c, cs47l63_fll_do_config()):
 * its thresholds, gain words, feedback dividers and N/theta/lambda arithmetic
 * are properties of the loop hardware, not of any one implementation. The
 * register order in cs47l63_clock_apply() is that source's
 * cs47l63_fll_apply_config() / cs47l63_fll_enable().
 */

#include "cs47l63_clock.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include "cs47l63_bus.h"
#include "cs47l63_regs.h"

/* Reference-frequency bands. Each picks a lock-detect threshold, a loop-gain
 * word and a starting feedback divider.
 */
#define CS47L63_FLL_LOW_THRESH 192000U
#define CS47L63_FLL_MID_THRESH 1152000U
#define CS47L63_FLL_MAX_THRESH 13000000U
#define CS47L63_FLL_LOW_GAINS  0x23F0
#define CS47L63_FLL_MID_GAINS  0x22F2
#define CS47L63_FLL_HIGH_GAINS 0x21F0

/* Below this the loop runs in its low-power integer mode. */
#define CS47L63_FLL_LP_INT_THRESH 100000U

/* N range, per mode. */
#define CS47L63_FLL_INT_MIN_N  1U
#define CS47L63_FLL_INT_MAX_N  1023U
#define CS47L63_FLL_FRAC_MIN_N 2U
#define CS47L63_FLL_FRAC_MAX_N 255U

/** Largest output the loop can produce. */
#define CS47L63_FLL_MAX_FOUT 50000000U

/** The reference divider is a shift of 0 to 3. */
#define CS47L63_FLL_REFDIV_MAX_SHIFT 3U

static uint32_t gcd_u32(uint32_t a, uint32_t b)
{
	while (b != 0) {
		uint32_t t = a % b;

		a = b;
		b = t;
	}

	return a;
}

/**
 * @brief The SYSCLK_FREQ code for an FLL output, or -ENOTSUP.
 *
 * An FLL that feeds SYSCLK is not free to run at any of the frequencies the
 * SYSCLK_FREQ field can name: DS1249F2 section 4.10.7 requires exactly
 * 49.152 MHz for the 48 kHz sample-rate family, or 45.1584 MHz for the
 * 44.1 kHz one. SYSCLK_FRAC is pinned to 0 throughout this driver, which
 * selects the first of those two, so one output frequency is legal here and
 * anything else has to be refused rather than programmed.
 */
static int sysclk_freq_code(uint32_t fout_hz, uint8_t *code)
{
	if (fout_hz != CS47L63_FLL_FOUT_HZ) {
		return -ENOTSUP;
	}

	*code = CS47L63_SYSCLK_FREQ_49M152;

	return 0;
}

int cs47l63_clock_solve(uint32_t fref_hz, uint32_t fout_hz, uint8_t refclk_src,
			struct cs47l63_fll_solution *out)
{
	uint32_t refdiv;
	uint32_t fref;
	uint32_t fbdiv;
	uint32_t gains;
	uint32_t lockdet_thr;
	uint32_t hp;
	uint32_t min_n;
	uint32_t max_n;
	uint32_t ratio;
	uint32_t divisor;
	uint32_t num;
	uint32_t lambda;
	uint32_t n;
	uint32_t theta;
	uint8_t freq_code;
	bool frac;
	int ret;

	if (out == NULL) {
		return -EINVAL;
	}
	if (fref_hz == 0 || fout_hz == 0) {
		return -EINVAL;
	}
	if (fout_hz > CS47L63_FLL_MAX_FOUT) {
		return -ENOTSUP;
	}

	/* SYSCLK is part of the solution, not something the caller is left to
	 * remember: an fout the SYSCLK mux cannot name is unreachable, and
	 * saying so here keeps a failed solve from writing anything.
	 */
	ret = sysclk_freq_code(fout_hz, &freq_code);
	if (ret < 0) {
		return ret;
	}

	/* Bring the reference into the loop's input range with the /1 /2 /4 /8
	 * predivider. A reference still above the maximum at /8 is out of
	 * range and is refused rather than approximated.
	 */
	for (refdiv = 0; refdiv <= CS47L63_FLL_REFDIV_MAX_SHIFT; refdiv++) {
		if ((fref_hz >> refdiv) <= CS47L63_FLL_MAX_THRESH) {
			break;
		}
	}
	refdiv = MIN(refdiv, CS47L63_FLL_REFDIV_MAX_SHIFT);
	fref = fref_hz >> refdiv;
	if (fref > CS47L63_FLL_MAX_THRESH) {
		return -ENOTSUP;
	}
	if (fref > fout_hz) {
		return -ENOTSUP;
	}

	frac = (fout_hz % fref) != 0;

	if (fref < CS47L63_FLL_LOW_THRESH) {
		lockdet_thr = 2;
		gains = CS47L63_FLL_LOW_GAINS;
		fbdiv = frac ? 256 : 4;
	} else if (fref < CS47L63_FLL_MID_THRESH) {
		lockdet_thr = 8;
		gains = CS47L63_FLL_MID_GAINS;
		fbdiv = frac ? 16 : 2;
	} else {
		lockdet_thr = 8;
		gains = CS47L63_FLL_HIGH_GAINS;
		fbdiv = 1;
	}

	if (frac) {
		/* A fractional ratio needs the high-performance loop. */
		hp = 0x3;
		min_n = CS47L63_FLL_FRAC_MIN_N;
		max_n = CS47L63_FLL_FRAC_MAX_N;
	} else {
		hp = (fref < CS47L63_FLL_LP_INT_THRESH) ? 0x0 : 0x1;
		min_n = CS47L63_FLL_INT_MIN_N;
		max_n = CS47L63_FLL_INT_MAX_N;
	}

	/* Walk the feedback divider until N lands inside the range the mode
	 * allows. Either bound running out means the request is unreachable.
	 */
	ratio = fout_hz / fref;
	while (ratio / fbdiv < min_n) {
		fbdiv /= 2;
		if (fbdiv < min_n) {
			return -ENOTSUP;
		}
	}
	while (frac && (ratio / fbdiv > max_n)) {
		fbdiv *= 2;
		if (fbdiv >= 1024) {
			return -ENOTSUP;
		}
	}

	/* N + theta/lambda is fout/(fref * fbdiv) in lowest terms. */
	divisor = gcd_u32(fout_hz, fbdiv * fref);
	num = fout_hz / divisor;
	lambda = (fref * fbdiv) / divisor;
	if (lambda == 0) {
		return -ENOTSUP;
	}
	n = num / lambda;
	theta = num % lambda;

	if (n < min_n || n > max_n) {
		return -ENOTSUP;
	}
	if (fbdiv < 1 || (frac && fbdiv >= 1024) || (!frac && fbdiv >= 256)) {
		return -ENOTSUP;
	}
	if (lambda > UINT16_MAX || theta > UINT16_MAX) {
		return -ENOTSUP;
	}

	out->refclk_src = refclk_src;
	out->refclk_div = (uint8_t)refdiv;
	out->lockdet_thr = (uint8_t)lockdet_thr;
	out->hp = (uint8_t)hp;
	out->n = (uint16_t)n;
	out->fb_div = (uint16_t)fbdiv;
	out->lambda = (uint16_t)lambda;
	out->theta = (uint16_t)theta;
	out->gains = (uint16_t)gains;
	out->sysclk_freq = freq_code;

	return 0;
}

int cs47l63_clock_apply(const struct device *dev, const struct cs47l63_fll_solution *sol)
{
	int ret;

	if (sol == NULL) {
		return -EINVAL;
	}

	/* Hold first. Every write below happens with the loop held, which is
	 * also the state a failure partway through leaves behind.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_HOLD,
				     CS47L63_FLL1_HOLD);
	if (ret < 0) {
		return ret;
	}

	/* LOCKDET and REFDET are not part of the vendor driver's FLL path,
	 * because its BSP hands cs47l63_syscfg_regs[] to cs47l63_boot() and that
	 * block writes FLL1_CONTROL2 = 0x28601177 - both detectors enabled -
	 * before any FLL call runs. This driver applies no such block, so the
	 * two bits are set here instead. Without LOCKDET the lock detector is
	 * off and FLL1_LOCK_STS1 cannot rise however well the loop is running,
	 * which is indistinguishable from a loop that never locked.
	 */
	ret = cs47l63_bus_update_reg(
		dev, CS47L63_FLL1_CONTROL2,
		CS47L63_FLL1_LOCKDET_THR_MASK | CS47L63_FLL1_LOCKDET | CS47L63_FLL1_PHASEDET |
			CS47L63_FLL1_REFDET | CS47L63_FLL1_REFCLK_DIV_MASK | CS47L63_FLL1_N_MASK,
		((uint32_t)sol->lockdet_thr << CS47L63_FLL1_LOCKDET_THR_SHIFT) |
			CS47L63_FLL1_LOCKDET | CS47L63_FLL1_PHASEDET | CS47L63_FLL1_REFDET |
			((uint32_t)sol->refclk_div << CS47L63_FLL1_REFCLK_DIV_SHIFT) |
			((uint32_t)sol->n << CS47L63_FLL1_N_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_write_reg(dev, CS47L63_FLL1_CONTROL3,
				    ((uint32_t)sol->lambda << CS47L63_FLL1_LAMBDA_SHIFT) |
					    ((uint32_t)sol->theta << CS47L63_FLL1_THETA_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL4,
				     CS47L63_FLL1_GAIN_MASK | CS47L63_FLL1_HP_MASK |
					     CS47L63_FLL1_FB_DIV_MASK,
				     ((uint32_t)sol->gains << CS47L63_FLL1_GAIN_SHIFT) |
					     ((uint32_t)sol->hp << CS47L63_FLL1_HP_SHIFT) |
					     ((uint32_t)sol->fb_div << CS47L63_FLL1_FB_DIV_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL2, CS47L63_FLL1_REFCLK_SRC_MASK,
				     (uint32_t)sol->refclk_src << CS47L63_FLL1_REFCLK_SRC_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_EN, CS47L63_FLL1_EN);
	if (ret < 0) {
		return ret;
	}

	/* Latch the whole set, then let the loop run. */
	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_CTRL_UPD,
				     CS47L63_FLL1_CTRL_UPD);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_FLL1_CONTROL1, CS47L63_FLL1_HOLD, 0);
	if (ret < 0) {
		return ret;
	}

	/* No wait for lock here. The reference is the SoC's I2S master clock,
	 * and that pin only runs while an I2S transfer does - it is dead at
	 * configure time, so a loop that has not locked yet is the normal
	 * outcome of this function rather than a fault. Lock is observed later,
	 * once the stream is running (cs47l63_clock_locked()).
	 */
	return cs47l63_bus_update_reg(dev, CS47L63_SYSTEM_CLOCK1,
				      CS47L63_SYSCLK_FRAC | CS47L63_SYSCLK_FREQ_MASK |
					      CS47L63_SYSCLK_EN | CS47L63_SYSCLK_SRC_MASK,
				      ((uint32_t)sol->sysclk_freq << CS47L63_SYSCLK_FREQ_SHIFT) |
					      CS47L63_SYSCLK_EN | CS47L63_SYSCLK_SRC_FLL1);
}

int cs47l63_clock_locked(const struct device *dev, bool *locked)
{
	uint32_t sts;
	int ret;

	if (locked == NULL) {
		return -EINVAL;
	}

	ret = cs47l63_bus_read_reg(dev, CS47L63_IRQ1_STS_6, &sts);
	if (ret < 0) {
		return ret;
	}

	*locked = (sts & CS47L63_FLL1_LOCK_STS1) != 0;

	return 0;
}
