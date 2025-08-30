/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

SECTION_DATA_PROLOGUE(.user_noinit,,)
{
	. = ALIGN(4);
	__user_noinit_start = .;
	*(.user_noinit)
	*(".user_noinit.*")

	. = ALIGN(4);
	__user_noinit_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
__user_noinit_size = __user_noinit_end - __user_noinit_start;
__user_noinit_load_start = LOADADDR(.user_noinit);
