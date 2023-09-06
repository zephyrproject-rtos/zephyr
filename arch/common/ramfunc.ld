/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

/* The ramfunc region must have an associated MPU entry added for it, so that
 * code can be executed in RAM. This means the start of the ramfunc region
 * must respect MPU alignment requirements. We include a padding section here
 * in order to make sure the region will be placed at the correct offset
 * in flash.
 */
SECTION_PROLOGUE(ramfunc_padding_section,,)
{
	MPU_ALIGN(__ramfunc_size);
#ifdef CONFIG_BUILD_ALIGN_LMA
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#else
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
#endif

SECTION_DATA_PROLOGUE(.ramfunc,,)
{
	__ramfunc_start = .;
	*(.ramfunc)
	*(".ramfunc.*")

#include <snippets-ramfunc-section.ld>

	MPU_ALIGN(__ramfunc_size);
	__ramfunc_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
__ramfunc_size = __ramfunc_end - __ramfunc_start;
__ramfunc_load_start = LOADADDR(.ramfunc);
