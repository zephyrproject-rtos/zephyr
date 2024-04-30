/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_SOC_INTEL_ADSP_DEBUG_HELPERS_H_
#define ZEPHYR_SOC_INTEL_ADSP_DEBUG_HELPERS_H_

#include <adsp_memory.h>
#include <zephyr/linker/linker-defs.h>

extern char _imr_start[];
extern char _imr_end[];
extern char _end[];
extern char _heap_sentry[];
extern char _cached_start[];
extern char _cached_end[];

static inline bool intel_adsp_ptr_executable(const void *p)
{
	return (p >= (void *)__text_region_start && p <= (void *)__text_region_end) ||
		(p >= (void *)_imr_start && p <= (void *)_imr_end);
}

static inline bool intel_adsp_ptr_is_sane(uint32_t sp)
{
	return ((char *)sp >= _end && (char *)sp <= _heap_sentry) ||
		((char *)sp >= _cached_start && (char *)sp <= _cached_end) ||
		(sp >= (IMR_BOOT_LDR_MANIFEST_BASE - CONFIG_ISR_STACK_SIZE)
		 && sp <= IMR_BOOT_LDR_MANIFEST_BASE);
}


#endif /* ZEPHYR_SOC_INTEL_ADSP_DEBUG_HELPERS_H_ */
