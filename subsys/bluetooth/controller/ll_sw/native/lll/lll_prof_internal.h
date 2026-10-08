/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_LLL_LLL_PROF_INTERNAL_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_LLL_LLL_PROF_INTERNAL_H_

/* ISR profiling measures CPU use, which a simulation does not model, hence
 * there is nothing to report.
 */
static inline uint16_t lll_prof_radio_get(void)
{
	return 0U;
}

static inline uint16_t lll_prof_lll_get(void)
{
	return 0U;
}

static inline uint16_t lll_prof_ull_high_get(void)
{
	return 0U;
}

static inline uint16_t lll_prof_ull_low_get(void)
{
	return 0U;
}

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_LLL_LLL_PROF_INTERNAL_H_ */
