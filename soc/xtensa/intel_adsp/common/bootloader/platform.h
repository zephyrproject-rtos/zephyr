/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2021 Intel Corporation.  All rights reserved.
 */
#ifndef ZEPHYR_INCLUDE_INTEL_ADSP_PLATFORM_H_
#define ZEPHYR_INCLUDE_INTEL_ADSP_PLATFORM_H_

/* Various cAVS platform dependencies needed by the bootloader code.
 * These probably want to migrate to devicetree.
 */

#if defined(CONFIG_SOC_SERIES_INTEL_CAVS_V25)
#define PLATFORM_RESET_MHE_AT_BOOT
#define PLATFORM_MEM_INIT_AT_BOOT
#define PLATFORM_HPSRAM_EBB_COUNT 30
#define EBB_SEGMENT_SIZE          32

#elif defined(CONFIG_SOC_SERIES_INTEL_CAVS_V20)
#define PLATFORM_RESET_MHE_AT_BOOT
#define PLATFORM_MEM_INIT_AT_BOOT
#define PLATFORM_HPSRAM_EBB_COUNT 47
#define EBB_SEGMENT_SIZE          32

#elif defined(CONFIG_SOC_SERIES_INTEL_CAVS_V18)
#define PLATFORM_RESET_MHE_AT_BOOT
#define PLATFORM_MEM_INIT_AT_BOOT
#define PLATFORM_HPSRAM_EBB_COUNT 47
#define EBB_SEGMENT_SIZE          32

#elif defined(CONFIG_SOC_SERIES_INTEL_CAVS_V15)
#define PLATFORM_RESET_MHE_AT_BOOT
#define PLATFORM_DISABLE_L2CACHE_AT_BOOT

#endif

#endif /* ZEPHYR_INCLUDE_INTEL_ADSP_PLATFORM_H_ */
