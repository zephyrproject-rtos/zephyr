/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_
#define ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_

#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/kernel.h>

/**
 * @brief Application memory domain APIs
 * @defgroup mem_domain_apis_app Application memory domain APIs
 * @ingroup mem_domain_apis
 * @{
 */

#ifdef CONFIG_USERSPACE

/**
 * @brief Name of the data section for a particular partition
 *
 * Useful for defining memory pools, or any other macro that takes a
 * section name as a parameter.
 *
 * @param id Partition name
 */
#define K_APP_DMEM_SECTION(id) data_smem_##id##_data

/**
 * @brief Name of the bss section for a particular partition
 *
 * Useful for defining memory pools, or any other macro that takes a
 * section name as a parameter.
 *
 * @param id Partition name
 */
#define K_APP_BMEM_SECTION(id) data_smem_##id##_bss

/**
 * @brief Place data in a partition's data section
 *
 * Globals tagged with this will end up in the data section for the
 * specified memory partition. This data should be initialized to some
 * desired value.
 *
 * @param id Name of the memory partition to associate this data
 */
#define K_APP_DMEM(id) Z_GENERIC_SECTION(K_APP_DMEM_SECTION(id))

/**
 * @brief Place data in a partition's bss section
 *
 * Globals tagged with this will end up in the bss section for the
 * specified memory partition. This data will be zeroed at boot.
 *
 * @param id Name of the memory partition to associate this data
 */
#define K_APP_BMEM(id) Z_GENERIC_SECTION(K_APP_BMEM_SECTION(id))

struct z_app_region {
	void *bss_start;
	size_t bss_size;
};

#define Z_APP_START(id) z_data_smem_##id##_part_start
#define Z_APP_SIZE(id) z_data_smem_##id##_part_size
#define Z_APP_BSS_START(id) z_data_smem_##id##_bss_start
#define Z_APP_BSS_SIZE(id) z_data_smem_##id##_bss_size

/* If a partition is declared with K_APPMEM_PARTITION, but never has any
 * data assigned to its contents, then no symbols with its prefix will end
 * up in the symbol table. This prevents gen_app_partitions.py from detecting
 * that the partition exists, and the linker symbols which specify partition
 * bounds will not be generated, resulting in build errors.
 *
 * What this inline assembly code does is define a symbol with no data.
 * This should work for all arches that produce ELF binaries, see
 * https://sourceware.org/binutils/docs/as/Section.html
 *
 * We don't know what active flags/type of the pushed section were, so we are
 * specific: "aw" indicates section is allocatable and writable,
 * and "@progbits" indicates the section has data.
 */
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
/* ARM has a quirk in that '@' denotes a comment, so we have to send
 * %progbits to the assembler instead.
 */
#define Z_PROGBITS_SYM	"%"
#else
#define Z_PROGBITS_SYM "@"
#endif

#if defined(CONFIG_ARC) && defined(__CCAC__)
/* ARC MWDT assembler has slightly different pushsection/popsection directives
 * names.
 */
#define Z_PUSHSECTION_DIRECTIV		".pushsect"
#define Z_POPSECTION_DIRECTIVE		".popsect"
#else
#define Z_PUSHSECTION_DIRECTIV		".pushsection"
#define Z_POPSECTION_DIRECTIVE		".popsection"
#endif

#define Z_APPMEM_PLACEHOLDER(name) \
	__asm__ ( \
		Z_PUSHSECTION_DIRECTIV " " STRINGIFY(K_APP_DMEM_SECTION(name)) \
			",\"aw\"," Z_PROGBITS_SYM "progbits\n\t" \
		".global " STRINGIFY(name) "_placeholder\n\t" \
		STRINGIFY(name) "_placeholder:\n\t" \
		Z_POPSECTION_DIRECTIVE "\n\t")

/**
 * @brief Define an application memory partition with linker support
 *
 * Defines a k_mem_paritition with the provided name.
 * This name may be used with the K_APP_DMEM and K_APP_BMEM macros to
 * place globals automatically in this partition.
 *
 * NOTE: placeholder char variable is defined here to prevent build errors
 * if a partition is defined but nothing ever placed in it.
 *
 * @param name Name of the k_mem_partition to declare
 */
#define K_APPMEM_PARTITION_DEFINE(name) \
	extern char Z_APP_START(name)[]; \
	extern char Z_APP_SIZE(name)[]; \
	struct k_mem_partition name = { \
		.start = (uintptr_t) &Z_APP_START(name), \
		.size = (size_t) &Z_APP_SIZE(name), \
		.attr = K_MEM_PARTITION_P_RW_U_RW \
	}; \
	extern char Z_APP_BSS_START(name)[]; \
	extern char Z_APP_BSS_SIZE(name)[]; \
	Z_GENERIC_SECTION(.app_regions.name) \
	const struct z_app_region name##_region = { \
		.bss_start = &Z_APP_BSS_START(name), \
		.bss_size = (size_t) &Z_APP_BSS_SIZE(name) \
	}; \
	Z_APPMEM_PLACEHOLDER(name)
#else

#define K_APP_BMEM(ptn)
#define K_APP_DMEM(ptn)
#define K_APP_DMEM_SECTION(ptn) .data
#define K_APP_BMEM_SECTION(ptn) .bss
#define K_APPMEM_PARTITION_DEFINE(name)

#endif /* CONFIG_USERSPACE */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_APP_MEMORY_APP_MEMDOMAIN_H_ */
