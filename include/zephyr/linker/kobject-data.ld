/*
 * Copyright (c) 2017,2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE

	z_kobject_data_begin = .;
	SECTION_DATA_PROLOGUE(kobject_data,,)
	{

#if !defined(LINKER_ZEPHYR_PREBUILT) && \
    !defined(LINKER_ZEPHYR_FINAL)

#ifdef CONFIG_DYNAMIC_OBJECTS
	PROVIDE(_thread_idx_map = .);
	. = . + CONFIG_MAX_THREAD_BYTES;
#endif

#endif /* !LINKER_ZEPHYR_PREBUILT && !LINKER_ZEPHYR_FINAL */

	/* During LINKER_KOBJECT_PREBUILT and LINKER_ZEPHYR_PREBUILT,
	 * space needs to be reserved for the rodata that will be
	 * produced by gperf during the final stages of linking.
	 * The alignment and size are produced by
	 * scripts/build/gen_kobject_placeholders.py. These are here
	 * so the addresses to kobjects would remain the same
	 * during the final stages of linking (LINKER_ZEPHYR_FINAL).
	 */

#if defined(LINKER_ZEPHYR_PREBUILT)
#include <zephyr/linker-kobject-prebuilt-data.h>

#ifdef CONFIG_DYNAMIC_OBJECTS
	/* This is produced by gperf. Put a place holder here
	 * to avoid compilation error.
	 */
	PROVIDE(_thread_idx_map = .);
#endif
#ifdef KOBJECT_DATA_ALIGN
	. = ALIGN(KOBJECT_DATA_ALIGN);
	. = . + KOBJECT_DATA_SZ;
#endif
#endif /* LINKER_ZEPHYR_PREBUILT */

#if defined(LINKER_ZEPHYR_FINAL)
#include <zephyr/linker-kobject-prebuilt-data.h>
#ifdef KOBJECT_DATA_ALIGN
	. = ALIGN(KOBJECT_DATA_ALIGN);

	_kobject_data_area_start = .;
#endif

	*(".kobject_data.data*")
	*(".kobject_data.sdata*")

#ifdef KOBJECT_DATA_ALIGN
	_kobject_data_area_end = .;
	_kobject_data_area_used = _kobject_data_area_end - _kobject_data_area_start;

	ASSERT(_kobject_data_area_used <= KOBJECT_DATA_SZ,
"scripts/build/gen_kobject_placeholders.py did not reserve enough space \
for kobject data."
	);

	/* Padding is needed to preserve kobject addresses
	 * if we have reserved more space than needed.
	 */
	. = MAX(., _kobject_data_area_start + KOBJECT_DATA_SZ);
#endif /* KOBJECT_DATA_ALIGN */
#endif /* LINKER_ZEPHYR_FINAL */

        } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#endif /* CONFIG_USERSPACE */
