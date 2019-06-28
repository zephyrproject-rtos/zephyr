/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE
	/* Constraints:
         *
         * - changes to the size of this section between build phases
         *   *must not* shift the memory address of any kernel objects,
         *   since it contains a hashtable of the memory addresses of those
         *   kernel objects
         *
         * - It is OK if this section itself is shifted in between builds; for
         *   example some arches may precede this section with generated MMU
         *    page tables which are also unpredictable in size.
         *
         * The size of the
	 * gperf tables is both a function of the number of kernel objects,
	 * *and* the specific memory addresses being hashed. It is not something
	 * that can be predicted without actually building and compiling it.
         */
	SECTION_DATA_PROLOGUE(priv_stacks,,)
	{
	*(".priv_stacks.data*")

        /* This is also unpredictable in size, and has the same constraints.
         * On XIP systems this will get put at the very end of ROM.
         */
#ifndef CONFIG_XIP
	*(".priv_stacks.rodata*")
#endif
        } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_USERSPACE */

