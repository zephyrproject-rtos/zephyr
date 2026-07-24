/* Copyright (c) 2022, 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-defs.h>
#include <xtensa/config/core-isa.h>
#include <zephyr/arch/xtensa/xtensa_ptr.h>

bool xtensa_soc_ptr_executable(const void *p)
{
	bool in_text = ((p >= (void *)__text_region_start) &&
			(p <= (void *)__text_region_end));
	bool in_vecbase = ((p >= (void *)XCHAL_VECBASE_RESET_VADDR) &&
			   (p < (void *)CONFIG_SRAM_OFFSET));

	return (in_text || in_vecbase);
}
