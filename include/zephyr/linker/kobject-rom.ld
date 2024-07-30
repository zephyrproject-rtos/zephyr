/*
 * Copyright (c) 2017,2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE

	/* During LINKER_KOBJECT_PREBUILT and LINKER_ZEPHYR_PREBUILT,
	 * space needs to be reserved for the rodata that will be
	 * produced by gperf during the final stages of linking.
	 * The alignment and size are produced by
	 * scripts/build/gen_kobject_placeholders.py. These are here
	 * so the addresses to kobjects would remain the same
	 * during the final stages of linking (LINKER_ZEPHYR_FINAL).
	 */

#if defined(LINKER_ZEPHYR_PREBUILT)
#include <zephyr/linker-kobject-prebuilt-rodata.h>
#ifdef KOBJECT_RODATA_ALIGN
	. = ALIGN(KOBJECT_RODATA_ALIGN);

	_kobject_rodata_area_start = .;

	. = . + KOBJECT_RODATA_SZ;

	_kobject_rodata_area_end = .;
#endif
#endif /* LINKER_ZEPHYR_PREBUILT */

#if defined(LINKER_ZEPHYR_FINAL)
#include <zephyr/linker-kobject-prebuilt-rodata.h>
#ifdef KOBJECT_RODATA_ALIGN
	. = ALIGN(KOBJECT_RODATA_ALIGN);

	_kobject_rodata_area_start = .;
#endif

	*(".kobject_data.rodata*")

#ifdef KOBJECT_RODATA_ALIGN
	_kobject_rodata_area_end = .;
	_kobject_rodata_area_used = _kobject_rodata_area_end - _kobject_rodata_area_start;

	ASSERT(_kobject_rodata_area_used <= KOBJECT_RODATA_SZ,
"scripts/build/gen_kobject_placeholders.py did not reserve enough space \
for kobject rodata."
	);

	/* Padding is needed to preserve kobject addresses
	 * if we have reserved more space than needed.
	 */
	. = MAX(., _kobject_rodata_area_start + KOBJECT_RODATA_SZ);
#endif /* KOBJECT_RODATA_ALIGN */
#endif /* LINKER_ZEPHYR_FINAL */

#endif /* CONFIG_USERSPACE */
