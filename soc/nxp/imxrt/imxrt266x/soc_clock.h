/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_CLOCK_H_
#define _SOC_CLOCK_H_

/**
 * @brief Bring the SoC clock tree up to the High Performance Run operating point.
 *
 * Programs the analog sources through the power-mode transition a frequency
 * change has to be bracketed by. Call once before any driver: the per-peripheral
 * roots in devicetree are applied later by the clock controller and assume these
 * sources.
 */
void soc_clock_init(void);

/**
 * @brief Switch the CPU, NPU and MEDIA roots to the High Performance Run rates.
 *
 * The rates soc_clock_init() programs: the CPU at the cpu0 devicetree rate. A
 * clock callback for POWER_EnterHpRun(), which orders the core voltage around
 * it. Not reentrant; call only after soc_clock_init().
 */
void soc_clock_switch_to_high_performance_run(void);

/**
 * @brief Switch the CPU, NPU and MEDIA roots to the Normal Run rates.
 *
 * The CPU runs from the core PLL at its devicetree rate, the NPU from half
 * the main PLL's second fractional output (500 MHz on the EVK). A clock
 * callback for POWER_EnterNormalRun(). Not reentrant; call only after
 * soc_clock_init().
 */
void soc_clock_switch_to_normal_run(void);

/**
 * @brief Switch the CPU, NPU and MEDIA roots to the Low Leakage Run rates.
 *
 * The CPU runs from half the PLL_PFDX rate, the NPU from half the core PLL
 * rate (500 MHz and 396 MHz on the EVK). A clock callback for
 * POWER_EnterLpRun(). Not reentrant; call only after soc_clock_init().
 */
void soc_clock_switch_to_low_leakage_run(void);

#endif /* _SOC_CLOCK_H_ */
