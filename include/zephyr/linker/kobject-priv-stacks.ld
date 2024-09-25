/*
 * Copyright (c) 2017,2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_GEN_PRIV_STACKS
	SECTION_DATA_PROLOGUE(priv_stacks_noinit,(NOLOAD),)
	{
	z_priv_stacks_ram_start = .;

	/* During LINKER_KOBJECT_PREBUILT and LINKER_ZEPHYR_PREBUILT,
	 * space needs to be reserved for the rodata that will be
	 * produced by gperf during the final stages of linking.
	 * The alignment and size are produced by
	 * scripts/build/gen_kobject_placeholders.py. These are here
	 * so the addresses to kobjects would remain the same
	 * during the final stages of linking (LINKER_ZEPHYR_FINAL).
	 */

#if defined(LINKER_ZEPHYR_PREBUILT)
#include <zephyr/linker-kobject-prebuilt-priv-stacks.h>
#ifdef KOBJECT_PRIV_STACKS_ALIGN
	. = ALIGN(KOBJECT_PRIV_STACKS_ALIGN);
	. = . + KOBJECT_PRIV_STACKS_SZ;
#endif
#endif /* LINKER_ZEPHYR_PREBUILT */

#if defined(LINKER_ZEPHYR_FINAL)
#include <zephyr/linker-kobject-prebuilt-priv-stacks.h>
#ifdef KOBJECT_PRIV_STACKS_ALIGN
	. = ALIGN(KOBJECT_PRIV_STACKS_ALIGN);
#endif
	*(".priv_stacks.noinit")
#endif /* LINKER_ZEPHYR_FINAL */

	z_priv_stacks_ram_end = .;

#if defined(LINKER_ZEPHYR_FINAL)
#ifdef KOBJECT_PRIV_STACKS_ALIGN
	z_priv_stacks_ram_used = z_priv_stacks_ram_end - z_priv_stacks_ram_start;

	ASSERT(z_priv_stacks_ram_used <= KOBJECT_PRIV_STACKS_SZ,
"scripts/build/gen_kobject_placeholders.py did not reserve enough space \
for privileged stacks."
	);

	/* Padding is needed to preserve kobject addresses
	 * if we have reserved more space than needed.
	 */
	. = MAX(., z_priv_stacks_ram_start + KOBJECT_PRIV_STACKS_SZ);
#endif /* KOBJECT_PRIV_STACKS_ALIGN */
#endif /* LINKER_ZEPHYR_FINAL */

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_GEN_PRIV_STACKS */
#endif /* CONFIG_USERSPACE */
