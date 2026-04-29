/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* Please keep in sync with cmake/linker_script/common/common-noinit.cmake */

SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
{
        /*
         * This section is used for non-initialized objects that
         * will not be cleared during the boot process.
         */

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#ifdef CONFIG_NOINIT_SNIPPET_FIRST
#include <snippets-noinit.ld>
#endif

        *(.noinit)
        *(".noinit.*")
#if defined(CONFIG_LLEXT) && !defined(CONFIG_LLEXT_CUSTOM_HEAP_PLACEMENT)
        *(.llext_heap)
        *(.llext_ext_heap)
        *(.llext_metadata_heap)
#endif /* CONFIG_LLEXT && !CONFIG_LLEXT_CUSTOM_HEAP_PLACEMENT */
#ifdef CONFIG_USERSPACE
	z_user_stacks_start = .;
	*(.user_stacks*)
	z_user_stacks_end = .;
#endif /* CONFIG_USERSPACE */

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#ifndef CONFIG_NOINIT_SNIPPET_FIRST
#include <snippets-noinit.ld>
#endif

} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_DCACHE_LINE_SIZE) && CONFIG_DCACHE_LINE_SIZE > 0
SECTION_PROLOGUE(noinit.__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME,(NOLOAD),
		SUBALIGN(CONFIG_DCACHE_LINE_SIZE))
{
	PLACE_SYMBOL_HERE(__dcacheline_exclusive_noinit_start);
	KEEP(*(SORT_BY_NAME(.__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME.*)))
	. = ALIGN(CONFIG_DCACHE_LINE_SIZE);
	PLACE_SYMBOL_HERE(__dcacheline_exclusive_noinit_end);
} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
#endif /* CONFIG_DCACHE_LINE_SIZE */

#include "kobject-priv-stacks.ld"
