/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE
	z_kobject_data_begin = .;
	/* Constraints:
         *
         * - changes to the size of this section between build phases
         *   *must not* shift the memory address of any kernel obejcts,
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
	 * that can be predicted without actually building and compling it.
         */
	SECTION_DATA_PROLOGUE(kobject_data,,)
	{
	*(".kobject_data.data*")

#ifndef LINKER_PASS2
#ifdef CONFIG_DYNAMIC_OBJECTS
	PROVIDE(_thread_idx_map = .);
	. += CONFIG_MAX_THREAD_BYTES;
#endif
#endif

        /* This is also unpredictable in size, and has the same constraints.
         * On XIP systems this will get put at the very end of ROM.
         */
#ifndef CONFIG_XIP
	*(".kobject_data.rodata*")
#endif
        } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_GEN_PRIV_STACKS
	SECTION_DATA_PROLOGUE(priv_stacks_noinit,,)
	{
	z_priv_stacks_ram_start = .;
	*(".priv_stacks.noinit")
	z_priv_stacks_ram_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_GEN_PRIV_STACKS */
#endif /* CONFIG_USERSPACE */
