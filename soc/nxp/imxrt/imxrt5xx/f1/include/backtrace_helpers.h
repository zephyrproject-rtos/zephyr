/* Copyright (c) 2025 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_SOC_NXP_IMXRT_IMXRT5XX_F1_BACKTRACE_HELPERS_H_
#define ZEPHYR_SOC_NXP_IMXRT_IMXRT5XX_F1_BACKTRACE_HELPERS_H_

#include <zephyr/linker/linker-defs.h>

#include <xtensa/config/core-isa.h>

static inline bool xtensa_mimxrt595s_f1_ptr_executable(const void *p)
{
	return ((p >= (void *)__text_region_start) &&
		(p <= (void *)__text_region_end));
}

#endif /* ZEPHYR_SOC_NXP_IMXRT_IMXRT5XX_F1_BACKTRACE_HELPERS_H_ */
