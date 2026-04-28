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

#include "kobject-priv-stacks.ld"
