/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define AON_C_OBJECT_FILE_IN_SECT(lsect, objfile)			\
        KEEP(*_intel_hal.a:objfile.c.obj(.##lsect))		\
        KEEP(*_intel_hal.a:objfile.c.obj(.##lsect##.*))

#define AON_S_OBJECT_FILE_IN_SECT(lsect, objfile)			\
        KEEP(*_intel_hal.a:objfile.S.obj(.##lsect))		\
        KEEP(*_intel_hal.a:objfile.S.obj(.##lsect##.*))

#define AON_IN_SECT(lsect)						\
        AON_C_OBJECT_FILE_IN_SECT(lsect, aon_task)			\
        AON_C_OBJECT_FILE_IN_SECT(lsect, ish_dma)			\
        AON_S_OBJECT_FILE_IN_SECT(lsect, ipapg)

GROUP_START(AON)

	SECTION_PROLOGUE(aon,,)
	{
		aon_start = .;
		KEEP(*(.data.aon_share))
		AON_IN_SECT(data)
		AON_IN_SECT(text)
		AON_IN_SECT(bss)
		aon_end = .;
	} GROUP_LINK_IN(AON)

GROUP_END(AON)
