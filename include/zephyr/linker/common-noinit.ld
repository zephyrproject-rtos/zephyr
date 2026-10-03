/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* Please keep in sync with cmake/linker_script/common/common-noinit.cmake */

#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>

#define DCACHELINE_EXCLUSIVE_NOINIT_SECTION_START UTIL_CAT(__, UTIL_CAT(__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME, _start))
#define DCACHELINE_EXCLUSIVE_NOINIT_SECTION_END UTIL_CAT(__, UTIL_CAT(__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME, _end))

SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
{
        /*
         * This section is used for non-initialized objects that
         * will not be cleared during the boot process.
         */
	PLACE_SYMBOL_HERE(__noinit_start);

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
#if defined(CONFIG_DCACHE_LINE_SIZE)
	. = ALIGN(CONFIG_DCACHE_LINE_SIZE);
	PLACE_SYMBOL_HERE(DCACHELINE_EXCLUSIVE_NOINIT_SECTION_START);
	KEEP(*(SORT_BY_NAME(.__DCACHELINE_EXCLUSIVE_NOINIT_SECTION_NAME.*)))
	. = ALIGN(CONFIG_DCACHE_LINE_SIZE);
	PLACE_SYMBOL_HERE(DCACHELINE_EXCLUSIVE_NOINIT_SECTION_END);
#endif /* CONFIG_DCACHE_LINE_SIZE */

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#ifndef CONFIG_NOINIT_SNIPPET_FIRST
#include <snippets-noinit.ld>
#endif

	PLACE_SYMBOL_HERE(__noinit_end);
} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include "kobject-priv-stacks.ld"
