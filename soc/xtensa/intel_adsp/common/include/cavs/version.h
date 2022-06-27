/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * Author: Bartosz Kokoszko <bartoszx.kokoszko@linux.intel.com>
 */

#ifndef __CAVS_VERSION_H__
#define __CAVS_VERSION_H__

/* Note: this is not a Zephyr API and Zephyr doesn't use it
 * (preferring the kconfig/devicetree mechanisms from which this is
 * derived).  SOF uses the CAVS_VERSION symbol internally and needs it
 * defined, and apparently that definition ended up here.  Really this
 * should be moved to SOF proper, but until then let's make sure it
 * doesn't spread:
 */
#ifdef CONFIG_SOF

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

#endif /* CONFIG_SOF */

#endif /* __CAVS_VERSION_H__ */
