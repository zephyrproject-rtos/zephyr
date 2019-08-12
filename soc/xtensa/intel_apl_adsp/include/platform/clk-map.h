/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * Author: Tomasz Lauda <tomasz.lauda@linux.intel.com>
 */

#ifndef __INCLUDE_CLOCK_MAP__
#define __INCLUDE_CLOCK_MAP__

#include <sof/clk.h>

#define CLK_MAX_CPU_HZ		400000000

static const struct freq_table cpu_freq[] = {
	{100000000, 100000, 0x3},
	{200000000, 200000, 0x1},
	{CLK_MAX_CPU_HZ, 400000, 0x0}, /* the default one */
};

#define CPU_DEFAULT_IDX	2

/* IMPORTANT: array should be filled in increasing order
 * (regarding to .freq field)
 */
static const struct freq_table ssp_freq[] = {
	{ 19200000, 19200, CLOCK_SSP_XTAL_OSCILLATOR }, /* the default one */
	{ 24576000, 24576, CLOCK_SSP_AUDIO_CARDINAL },
	{ 96000000, 96000, CLOCK_SSP_PLL_FIXED },
};

#define SSP_DEFAULT_IDX	0

#endif
