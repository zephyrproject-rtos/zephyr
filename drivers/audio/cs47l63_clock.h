/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_clock.h
 * @brief CS47L63 FLL1 solver and apply path (internal)
 *
 * Two halves. cs47l63_clock_solve() is pure computation - no bus access and no
 * @ref device pointer - so it is callable and testable with nothing attached.
 * cs47l63_clock_apply() writes what it produced.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_CLOCK_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_CLOCK_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FLL1 output this driver locks to, in Hz.
 *
 * A multiple of 6.144 MHz, which is what SYSCLK_FRAC = 0 means, and the value
 * the board's 48 kHz family is built on.
 */
#define CS47L63_FLL_FOUT_HZ 49152000U

/**
 * @brief A complete FLL1 and SYSCLK configuration.
 *
 * cs47l63_clock_solve() never leaves this half-written: on success every field
 * is ready to write unmodified, on error none of them mean anything. There is
 * nothing here for the caller to fix up.
 */
struct cs47l63_fll_solution {
	/** FLL1_REFCLK_SRC code - which pin or net the reference arrives on. */
	uint8_t refclk_src;
	/** FLL1_REFCLK_DIV: the reference is divided by 2^refclk_div. */
	uint8_t refclk_div;
	/** FLL1_LOCKDET_THR. */
	uint8_t lockdet_thr;
	/** FLL1_HP: loop performance mode. */
	uint8_t hp;
	/** FLL1_N: the integer part of the multiplier. */
	uint16_t n;
	/** FLL1_FB_DIV: feedback divider. */
	uint16_t fb_div;
	/** FLL1_LAMBDA and FLL1_THETA: the fractional part, as theta/lambda. */
	uint16_t lambda;
	uint16_t theta;
	/** The 16-bit loop-gain word written to FLL1_CONTROL4. */
	uint16_t gains;
	/** SYSCLK_FREQ code matching @ref CS47L63_FLL_FOUT_HZ. */
	uint8_t sysclk_freq;
};

/**
 * @brief Solve FLL1 for a reference and an output frequency.
 *
 * An unreachable configuration is rejected, never approximated. Locking to a
 * neighbouring frequency yields audio that is slightly fast and that nobody
 * notices, which is worse than a refusal.
 *
 * @param fref_hz Reference frequency at the FLL input, in Hz
 * @param fout_hz Requested FLL output frequency, in Hz
 * @param refclk_src FLL1_REFCLK_SRC code for the pin the reference arrives on
 * @param[out] out Solved terms
 * @retval 0 on success
 * @retval -EINVAL if @p out is NULL or either frequency is zero
 * @retval -ENOTSUP if no set of terms reaches @p fout_hz exactly from
 *         @p fref_hz - including a reference the input divider cannot bring
 *         into range, and an output above the FLL's maximum
 */
int cs47l63_clock_solve(uint32_t fref_hz, uint32_t fout_hz, uint8_t refclk_src,
			struct cs47l63_fll_solution *out);

/**
 * @brief Milliseconds to allow the loop before its lock is worth reading.
 *
 * Generous: the loop needs single-digit milliseconds once its reference runs,
 * and this also has to cover the gap between the codec being told to play and
 * the SoC actually starting the I2S transfer that produces MCLK1.
 */
#define CS47L63_FLL_LOCK_SETTLE_MS 500

/**
 * @brief Write a solved configuration and bring SYSCLK up on it.
 *
 * The vendor order, which this follows exactly: hold the FLL, write the
 * control registers, select the reference source, enable, latch the update,
 * release the hold. The hold is asserted before any other FLL register is
 * written and released only after the update is latched; a bus error partway
 * through therefore leaves the FLL held rather than half-configured and
 * running.
 *
 * This does not wait for lock and does not fail when the loop is unlocked. The
 * reference is the SoC's I2S master clock, which runs only while an I2S
 * transfer does, so at configure time there is nothing for the loop to lock to
 * and the wait could only ever time out. SYSCLK is enabled on the running FLL
 * regardless; it starts for real when the reference appears.
 *
 * @param dev Codec device
 * @param sol Solution from @ref cs47l63_clock_solve
 * @retval 0 on success
 * @retval negative errno propagated from the failing bus transaction
 */
int cs47l63_clock_apply(const struct device *dev, const struct cs47l63_fll_solution *sol);

/**
 * @brief Read whether FLL1 currently reports lock.
 *
 * A live status bit, not a latch: it answers for the moment of the read, so it
 * is only meaningful once the reference has had time to run. See
 * @ref CS47L63_FLL_LOCK_SETTLE_MS.
 *
 * @param dev Codec device
 * @param[out] locked true when FLL1 is locked. Untouched on failure.
 * @retval 0 on success
 * @retval -EINVAL if @p locked is NULL
 * @retval other negative errno propagated from the failing bus transaction
 */
int cs47l63_clock_locked(const struct device *dev, bool *locked);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_CLOCK_H_ */
