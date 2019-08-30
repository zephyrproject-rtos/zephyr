/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *         Keyon Jie <yang.jie@linux.intel.com>
 */

#ifndef __PLATFORM_CLOCK__
#define __PLATFORM_CLOCK__

#include <sof/cpu.h>
#include <sof/io.h>
#include <platform/shim.h>

/** \brief Core(s) settings, up to PLATFORM_CORE_COUNT */
#define CLK_CPU(x)      (x)

/** \brief SSP clock r-t settings are after the core(s) settings */
#define CLK_SSP         PLATFORM_CORE_COUNT

/* SSP clock run-time data is the last one, so total number is ssp idx +1 */

/** \brief Total number of clocks */
#define NUM_CLOCKS      (CLK_SSP + 1)

static inline int clock_platform_set_cpu_freq(uint32_t cpu_freq_enc)
{
        /* set CPU frequency request for CCU */
#if CAVS_VERSION == CAVS_VERSION_1_5
        io_reg_update_bits(SHIM_BASE + SHIM_CLKCTL, SHIM_CLKCTL_HDCS, 0);
#endif
        io_reg_update_bits(SHIM_BASE + SHIM_CLKCTL,
                           SHIM_CLKCTL_DPCS_MASK(cpu_get_id()),
                           cpu_freq_enc);

        return 0;
}

static inline int clock_platform_set_ssp_freq(uint32_t ssp_freq_enc)
{
        return 0;
}

#endif
