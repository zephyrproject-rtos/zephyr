/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
