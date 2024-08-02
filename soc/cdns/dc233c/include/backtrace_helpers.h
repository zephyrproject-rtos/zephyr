/* Copyright (c) 2022, 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_SOC_XTENSA_DC233C_BACKTRACE_HELPERS_H_
#define ZEPHYR_SOC_XTENSA_DC233C_BACKTRACE_HELPERS_H_

#include <zephyr/linker/linker-defs.h>

#include <xtensa/config/core-isa.h>

static inline bool xtensa_dc233c_ptr_executable(const void *p)
{
	bool in_text = ((p >= (void *)__text_region_start) &&
			(p <= (void *)__text_region_end));
	bool in_vecbase = ((p >= (void *)XCHAL_VECBASE_RESET_VADDR) &&
			   (p < (void *)CONFIG_SRAM_OFFSET));

	return (in_text || in_vecbase);
}

#endif /* ZEPHYR_SOC_XTENSA_DC233C_BACKTRACE_HELPERS_H_ */
