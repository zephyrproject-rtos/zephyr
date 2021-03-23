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
         *   since it contains a table of the memory addresses of those
         *   kernel objects
         *
         * - It is OK if this section itself is shifted in between builds; for
         *   example some arches may precede this section with generated MMU
         *    page tables which are also unpredictable in size.
         *
         * The size of the object list is a function of the number of kernel
         * objects. It is not something that can be predicted without actually
         * building and compling it.
         */
	SECTION_DATA_PROLOGUE(kobject_data,,)
	{
	*(".kobject_data.data*")

#if !defined(LINKER_ZEPHYR_PREBUILT) && !defined(LINKER_ZEPHYR_FINAL)
        /* The symbols here do not exist before gen_kobj_list.py
         * has a chance to run. So provide some dummies here.
         */
	PROVIDE(z_kobject_list = .);

#ifdef CONFIG_DYNAMIC_OBJECTS
	PROVIDE(_thread_idx_map = .);
	. += CONFIG_MAX_THREAD_BYTES;
#endif
#endif
        } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#endif /* CONFIG_USERSPACE */
