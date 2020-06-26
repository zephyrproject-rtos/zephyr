/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * Author: Bartosz Kokoszko <bartoszx.kokoszko@linux.intel.com>
 */

#ifndef __CAVS_VERSION_H__
#define __CAVS_VERSION_H__

#include <autoconf.h>

#define CAVS_VERSION_1_5 0x10500
#define CAVS_VERSION_1_8 0x10800
#define CAVS_VERSION_2_0 0x20000
#define CAVS_VERSION_2_5 0x20500

/* CAVS version defined by CONFIG_CAVS_VER_*/
#if CONFIG_SOC_SERIES_INTEL_CAVS_V15
#define CAVS_VERSION CAVS_VERSION_1_5
#elif CONFIG_SOC_SERIES_INTEL_CAVS_V18
#define CAVS_VERSION CAVS_VERSION_1_8
#elif CONFIG_SOC_SERIES_INTEL_CAVS_V20
#define CAVS_VERSION CAVS_VERSION_2_0
#elif CONFIG_SOC_SERIES_INTEL_CAVS_V25
#define CAVS_VERSION CAVS_VERSION_2_5
#endif

#endif /* __CAVS_VERSION_H__ */
