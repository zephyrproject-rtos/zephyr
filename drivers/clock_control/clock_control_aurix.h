/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZYPHER_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_AURIX_H
#define ZYPHER_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_AURIX_H

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#define FOSC_SOURCES sys_pll, per_pll, fmcan, fasclins

/* Clock source*/
#define CLOCK(clock)        DT_NODELABEL(clock)
#define CLOCK_SOURCE(clock) DT_CLOCKS_CTLR(DT_NODELABEL(clock))
#define CLOCK_SOURCE_IS(clock, source)                                                             \
	DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(clock)), DT_NODELABEL(source))
#define CLOCK_SOURCES_EQUAL(clock, ...)

/**/
#define __CLOCK_SOURCE_ONE_OF(source, clock) CLOCK_SOURCE_IS(clock, source)

/** Check if source of clock is one of the arguments */
#define CLOCK_SOURCE_ONE_OF(clock, ...)                                                            \
	(FOR_EACH_FIXED_ARG(__CLOCK_SOURCE_ONE_OF, (||), clock, __VA_ARGS__))
/** Check if clock is clock source of one of the arguments */
#define CLOCK_IS_CLOCK_SOURCE(clock, ...)                                                          \
	(FOR_EACH_FIXED_ARG(CLOCK_SOURCE_IS, (||), clock, __VA_ARGS__))

/* Checks */
#define CHECK_CLOCK_SOURCE_ONE_OF(clock, ...)                                                      \
	BUILD_ASSERT(CLOCK_SOURCE_ONE_OF(clock, __VA_ARGS__), "Invalid " #clock " sources")
#define CHECK_CLOCK_IN_USE(clock, ...)                                                             \
	BUILD_ASSERT(!CLOCK_IS_CLOCK_SOURCE(clock, __VA_ARGS__) ||                                 \
			     DT_NODE_HAS_STATUS(DT_NODELABEL(clock), okay),                        \
		     "Clock " #clock " is not enabled")

/* Frequencies*/
#define CLOCK_FREQ(clock)        DT_PROP(CLOCK(clock), clock_frequency)
#define CLOCK_SOURCE_FREQ(clock) DT_PROP(CLOCK_SOURCE(clock), clock_frequency)
#define PLL_P_DIV(pll)           DT_PROP(DT_NODELABEL(pll), p_div)
#define PLL_N_DIV(pll)           DT_PROP(DT_NODELABEL(pll), n_div)
#define PLL_K_DIV(pll, id)       DT_PROP(DT_CHILD(DT_NODELABEL(pll), DT_CAT3(pll, _, id)), k_div)
#define PLL_K_PRE_DIV(pll, id)                                                                     \
	COND_CODE_1(                                                                               \
		DT_NODE_HAS_PROP(DT_CHILD(DT_NODELABEL(pll), DT_CAT3(pll, _, id)), k_pre_div),     \
		(DT_STRING_UNQUOTED(DT_CHILD(DT_NODELABEL(pll), DT_CAT3(pll, _, id)), k_pre_div)), \
		(1.0))
#define PLL_K_PRE_DIV_REG(pll, id)                                                                 \
	(id == 3 ? (uint32_t)(PLL_K_PRE_DIV(pll, id) * 10.0 - 10)                                  \
		 : (PLL_K_PRE_DIV(pll, id) == 1.0   ? 0                                            \
		    : PLL_K_PRE_DIV(pll, id) == 2.0 ? 1                                            \
		    : PLL_K_PRE_DIV(pll, id) == 1.2 ? 2                                            \
						    : 3))
#define PLL_DCO_FREQ(pll) ((double)CLOCK_SOURCE_FREQ(pll) / PLL_P_DIV(pll) * PLL_N_DIV(pll))
#define PLL_FREQ(pll, id) (PLL_DCO_FREQ(pll) / PLL_K_PRE_DIV(pll, id) / PLL_K_DIV(pll, id))

/* Dividers & Select*/
#define CLOCK_DIV_WITH_INST(clock, compat) ((DT_HAS_COMPAT_STATUS_OKAY(compat)) ? clock##_div : 0)
#define CLOCK_DIV_WITH_STATUS(clock)	\
					(DT_NODE_HAS_STATUS(CLOCK(clock), okay) ? clock##_div : 0)
#define CLOCK_SEL_WITH_INST(clock, source0, source1, compat)                                       \
	(IS_ENABLED(DT_CAT3(DT_N_INST_, compat, _NUM_OKAY))                                        \
		 ? CLOCK_SOURCE_IS(clock, source0) ? 1 : 0x2                                       \
		 : 0x0)

enum base_clock {
	fback_freq = (uint32_t)CLOCK_FREQ(fback),
	fosc_freq = (uint32_t)CLOCK_FREQ(fosc),
	fpll0_freq = (uint32_t)PLL_FREQ(sys_pll, 2),
	fpll1_freq = (uint32_t)PLL_FREQ(per_pll, 2),
	fpll2_freq = (uint32_t)PLL_FREQ(per_pll, 3),
	fsource0_freq = CLOCK_SOURCE_IS(fsource0, fpll0) ? fpll0_freq : fback_freq,
	fsource1_freq = CLOCK_SOURCE_IS(fsource1, fpll1) ? fpll1_freq : fback_freq,
	fsource2_freq = CLOCK_SOURCE_IS(fsource2, fpll2) ? fpll2_freq : fback_freq,
#if CONFIG_SOC_SERIES_TC4X
#if DT_NODE_EXISTS(DT_NODELABEL(fpllppu))
	fpllppu_freq = (uint32_t)PLL_FREQ(sys_pll, 3),
	fsourceppu_freq = CLOCK_SOURCE_IS(fsourceppu, fpllppu) ? fpllppu_freq : fback_freq,
#endif
	fpll3_freq = (uint32_t)PLL_FREQ(per_pll, 4),
	fsource3_freq = CLOCK_SOURCE_IS(fsource3, fpll3) ? fpll3_freq : fback_freq,
#endif
};

enum ccu_div {
	fspb_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fspb)),
	fsri_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fsri)),
	ffsi_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(ffsi)),
	fstm_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fstm)),
	fmcanh_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fmcanh)),
	fmcani_div = DIV_ROUND_UP(fsource1_freq, CLOCK_FREQ(fmcani)),
	fqspi_div =
		DIV_ROUND_UP(CLOCK_SOURCE_IS(fsourceqspi, fsource1) ? fsource1_freq : fsource2_freq,
			     CLOCK_FREQ(fqspi)),
	fasclinsi_div = DIV_ROUND_UP(fsource1_freq, CLOCK_FREQ(fasclinsi)),
	fasclinf_div = DIV_ROUND_UP(fsource2_freq, CLOCK_FREQ(fasclinf)),
	fi2c_div = DIV_ROUND_UP(fsource2_freq, CLOCK_FREQ(fi2c)),
#if CONFIG_SOC_SERIES_TC3X
	fgeth_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fgeth)),
	ffsi2_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fsri)),
	fbbb_div = 1,
	fgtm_div = 1,
#endif
#if CONFIG_SOC_SERIES_TC4X
	ftpb_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(ftpb)),
	fleth_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fleth)),
	fleth100_div = DIV_ROUND_UP(fsource2_freq, CLOCK_FREQ(fleth100)),
#if DT_NODE_EXISTS(DT_NODELABEL(fgeth))
	fgeth_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fgeth)),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fcpb))
	fcpb_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fcpb)),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fcanxl))
	fcanxl_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fcanxl)),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fppu))
	fppu_div = DIV_ROUND_UP(fsourceppu_freq, CLOCK_FREQ(fppu)),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fgtm))
	fgtm_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fgtm)),
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(fegtm))
	fegtm_div = DIV_ROUND_UP(fsource0_freq, CLOCK_FREQ(fegtm)),
#endif
#endif
};

/* Check Pll*/
CHECK_CLOCK_SOURCE_ONE_OF(fsource0, fpll0, fback);
CHECK_CLOCK_SOURCE_ONE_OF(fsource1, fpll1, fback);
CHECK_CLOCK_SOURCE_ONE_OF(fsource2, fpll2, fback);

#if CONFIG_SOC_SERIES_TC4X
CHECK_CLOCK_SOURCE_ONE_OF(fsource3, fpll3, fback);
#endif

CHECK_CLOCK_IN_USE(fosc, FOSC_SOURCES);

#define WAIT_FOR_CCUSTAT_UNLOCKED_OR_ERR(freq)                                                     \
	{                                                                                          \
		if (!WAIT_FOR(CLOCK_CCUSTAT.B.LCK == 0, 100, ccu_busy_wait_##freq(100))) {         \
			return -EIO;                                                               \
		}                                                                                  \
	}

#define WAIT_FOR_CCUCON_UNLOCKED_OR_ERR(id, freq)                                                  \
	{                                                                                          \
		if (!WAIT_FOR(MODULE_SCU.DT_CAT(CCUCON, id).B.LCK == 0, 100,                       \
			      ccu_busy_wait_##freq(100))) {                                        \
			return -EIO;                                                               \
		}                                                                                  \
	}

static inline void ccu_busy_wait_fback(uint32_t usec_to_wait)
{
	if (usec_to_wait == 0U) {
		return;
	}

	uint32_t start_cycles = k_cycle_get_32();
	uint32_t cycles_to_wait = z_tmcvt_32(usec_to_wait, Z_HZ_us, 50000000, true, true, false);

	for (;;) {
		uint32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
}

static inline void ccu_busy_wait_fback_div(uint32_t usec_to_wait)
{
	if (usec_to_wait == 0U) {
		return;
	}

	uint32_t start_cycles = k_cycle_get_32();
	uint32_t cycles_to_wait =
		z_tmcvt_32(usec_to_wait, Z_HZ_us, 100000000 / fstm_div, true, true, false);

	for (;;) {
		uint32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
}

static inline void ccu_busy_wait_framp(uint32_t usec_to_wait)
{
	if (usec_to_wait == 0U) {
		return;
	}

	uint32_t start_cycles = k_cycle_get_32();
	uint32_t cycles_to_wait = z_tmcvt_32(
		usec_to_wait, Z_HZ_us, (fsource0_freq - 100000000) / fstm_div, true, true, false);

	for (;;) {
		uint32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
}

#define CCU_BUSY_WAIT_FPLL(offset, _)                                                              \
	static inline void ccu_busy_wait_fpll_div##offset(uint32_t usec_to_wait)                   \
	{                                                                                          \
		if (usec_to_wait == 0U) {                                                          \
			return;                                                                    \
		}                                                                                  \
                                                                                                   \
		uint32_t start_cycles = k_cycle_get_32();                                          \
		uint32_t cycles_to_wait = z_tmcvt_32(usec_to_wait, Z_HZ_us,                        \
						     (uint32_t)((float)PLL_DCO_FREQ(sys_pll) /     \
								(PLL_K_DIV(sys_pll, 2) + offset)), \
						     true, true, false);                           \
                                                                                                   \
		for (;;) {                                                                         \
			uint32_t current_cycles = k_cycle_get_32();                                \
                                                                                                   \
			/* this handles the rollover on an unsigned 32-bit value */                \
			if ((current_cycles - start_cycles) >= cycles_to_wait) {                   \
				break;                                                             \
			}                                                                          \
		}                                                                                  \
	}
LISTIFY(4, CCU_BUSY_WAIT_FPLL, ())

#endif
