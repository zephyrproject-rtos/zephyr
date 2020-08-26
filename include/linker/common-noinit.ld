/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
{
        /*
         * This section is used for non-initialized objects that
         * will not be cleared during the boot process.
         */
        *(.noinit)
        *(".noinit.*")
#ifdef CONFIG_USERSPACE
	z_user_stacks_start = .;
	*(.user_stacks)
	z_user_stacks_end = .;
#endif /* CONFIG_USERSPACE */

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-noinit.ld>

} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
