/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Bartosz Kokoszko <bartoszx.kokoszko@linux.intel.com>
 */

/**
 * DSP parameters, common for cAVS platforms.
 */

#ifndef __CAVS_CPU_H__
#define __CAVS_CPU_H__

/** Number of available DSP cores (conf. by kconfig) */
#if defined(CONFIG_SMP)
#define PLATFORM_CORE_COUNT CONFIG_MP_NUM_CPUS
#else
#define PLATFORM_CORE_COUNT 1
#endif

/** Id of master DSP core */
#define PLATFORM_PRIMARY_CORE_ID	0

#endif /* __CAVS_CPU_H__ */
