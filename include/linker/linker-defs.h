/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * Platform independent, commonly used macros and defines related to linker
 * script.
 *
 * This file may be included by:
 * - Linker script files: for linker section declarations
 * - C files: for external declaration of address or size of linker section
 * - Assembly files: for external declaration of address or size of linker
 *   section
 */

#ifndef ZEPHYR_INCLUDE_LINKER_LINKER_DEFS_H_
#define ZEPHYR_INCLUDE_LINKER_LINKER_DEFS_H_

#include <toolchain.h>
#include <toolchain/common.h>
#include <linker/sections.h>
#include <sys/util.h>
#include <offsets.h>

/* We need to dummy out DT_NODE_HAS_STATUS when building the unittests.
 * Including devicetree.h would require generating dummy header files
 * to match what gen_defines creates, so it's easier to just dummy out
 * DT_NODE_HAS_STATUS.
 */
#ifdef ZTEST_UNITTEST
#define DT_NODE_HAS_STATUS(node, status) 0
#else
#include <devicetree.h>
#endif

#ifdef _LINKER
#define Z_LINK_ITERABLE(struct_type) \
	_CONCAT(_##struct_type, _list_start) = .; \
	KEEP(*(SORT_BY_NAME(._##struct_type.static.*))); \
	_CONCAT(_##struct_type, _list_end) = .

/* Define an output section which will set up an iterable area
 * of equally-sized data structures. For use with Z_STRUCT_SECTION_ITERABLE.
 * Input sections will be sorted by name, per ld's SORT_BY_NAME.
 *
 * This macro should be used for read-only data.
 */
#define Z_ITERABLE_SECTION_ROM(struct_type, subalign) \
	SECTION_PROLOGUE(struct_type##_area,,SUBALIGN(subalign)) \
	{ \
		Z_LINK_ITERABLE(struct_type); \
	} GROUP_LINK_IN(ROMABLE_REGION)

/* Define an output section which will set up an iterable area
 * of equally-sized data structures. For use with Z_STRUCT_SECTION_ITERABLE.
 * Input sections will be sorted by name, per ld's SORT_BY_NAME.
 *
 * This macro should be used for read-write data that is modified at runtime.
 */
#define Z_ITERABLE_SECTION_RAM(struct_type, subalign) \
	SECTION_DATA_PROLOGUE(struct_type##_area,,SUBALIGN(subalign)) \
	{ \
		Z_LINK_ITERABLE(struct_type); \
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

/*
 * generate a symbol to mark the start of the objects array for
 * the specified object and level, then link all of those objects
 * (sorted by priority). Ensure the objects aren't discarded if there is
 * no direct reference to them
 */
#define CREATE_OBJ_LEVEL(object, level)				\
		__##object##_##level##_start = .;		\
		KEEP(*(SORT(.object##_##level[0-9]*)));		\
		KEEP(*(SORT(.object##_##level[1-9][0-9]*)));

/*
 * link in shell initialization objects for all modules that use shell and
 * their shell commands are automatically initialized by the kernel.
 */

#define APP_SMEM_SECTION() KEEP(*(SORT("data_smem_*")))

#elif defined(_ASMLANGUAGE)

/* Assembly FILES: declaration defined by the linker script */
GDATA(__bss_start)
GDATA(__bss_num_words)
#ifdef CONFIG_XIP
GDATA(__data_rom_start)
GDATA(__data_ram_start)
GDATA(__data_num_words)
#endif

#else /* ! _ASMLANGUAGE */

#include <zephyr/types.h>
/*
 * Memory owned by the kernel, to be used as shared memory between
 * application threads.
 *
 * The following are extern symbols from the linker. This enables
 * the dynamic k_mem_domain and k_mem_partition creation and alignment
 * to the section produced in the linker.

 * The policy for this memory will be to initially configure all of it as
 * kernel / supervisor thread accessible.
 */
extern char _app_smem_start[];
extern char _app_smem_end[];
extern char _app_smem_size[];
extern char _app_smem_rom_start[];
extern char _app_smem_num_words[];

/* Memory owned by the kernel. Start and end will be aligned for memory
 * management/protection hardware for the target architecture.
 *
 * Consists of all kernel-side globals, all kernel objects, all thread stacks,
 * and all currently unused RAM.
 *
 * Except for the stack of the currently executing thread, none of this memory
 * is normally accessible to user threads unless specifically granted at
 * runtime.
 */
extern char __kernel_ram_start[];
extern char __kernel_ram_end[];
extern char __kernel_ram_size[];

/* Used by z_bss_zero or arch-specific implementation */
extern char __bss_start[];
extern char __bss_end[];

/* Used by z_data_copy() or arch-specific implementation */
#ifdef CONFIG_XIP
extern char __data_rom_start[];
extern char __data_ram_start[];
extern char __data_ram_end[];
#endif /* CONFIG_XIP */

/* Includes text and rodata */
extern char _image_rom_start[];
extern char _image_rom_end[];
extern char _image_rom_size[];

/* Includes all ROMable data, i.e. the size of the output image file. */
extern char _flash_used[];

/* datas, bss, noinit */
extern char _image_ram_start[];
extern char _image_ram_end[];

extern char _image_text_start[];
extern char _image_text_end[];
extern char _image_text_size[];

extern char _image_rodata_start[];
extern char _image_rodata_end[];
extern char _image_rodata_size[];

extern char _vector_start[];
extern char _vector_end[];

#ifdef CONFIG_SW_VECTOR_RELAY
extern char __vector_relay_table[];
#endif

#ifdef CONFIG_COVERAGE_GCOV
extern char __gcov_bss_start[];
extern char __gcov_bss_end[];
extern char __gcov_bss_size[];
#endif	/* CONFIG_COVERAGE_GCOV */

/* end address of image, used by newlib for the heap */
extern char _end[];

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ccm), okay)
extern char __ccm_data_rom_start[];
extern char __ccm_start[];
extern char __ccm_data_start[];
extern char __ccm_data_end[];
extern char __ccm_bss_start[];
extern char __ccm_bss_end[];
extern char __ccm_noinit_start[];
extern char __ccm_noinit_end[];
extern char __ccm_end[];
#endif

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
extern char __dtcm_data_start[];
extern char __dtcm_data_end[];
extern char __dtcm_bss_start[];
extern char __dtcm_bss_end[];
extern char __dtcm_noinit_start[];
extern char __dtcm_noinit_end[];
extern char __dtcm_data_rom_start[];
extern char __dtcm_start[];
extern char __dtcm_end[];
#endif

/* Used by the Security Attribution Unit to configure the
 * Non-Secure Callable region.
 */
#ifdef CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS
extern char __sg_start[];
extern char __sg_end[];
extern char __sg_size[];
#endif /* CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS */

/*
 * Non-cached kernel memory region, currently only available on ARM Cortex-M7
 * with a MPU. Start and end will be aligned for memory management/protection
 * hardware for the target architecture.
 *
 * All the functions with '__nocache' keyword will be placed into this
 * section.
 */
#ifdef CONFIG_NOCACHE_MEMORY
extern char _nocache_ram_start[];
extern char _nocache_ram_end[];
extern char _nocache_ram_size[];
#endif /* CONFIG_NOCACHE_MEMORY */

/* Memory owned by the kernel. Start and end will be aligned for memory
 * management/protection hardware for the target architecture.
 *
 * All the functions with '__ramfunc' keyword will be placed into this
 * section, stored in RAM instead of FLASH.
 */
#ifdef CONFIG_ARCH_HAS_RAMFUNC_SUPPORT
extern char _ramfunc_ram_start[];
extern char _ramfunc_ram_end[];
extern char _ramfunc_ram_size[];
extern char _ramfunc_rom_start[];
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */

/* Memory owned by the kernel. Memory region for thread privilege stack buffers,
 * currently only applicable on ARM Cortex-M architecture when building with
 * support for User Mode.
 *
 * All thread privilege stack buffers will be placed into this section.
 */
#ifdef CONFIG_USERSPACE
extern char z_priv_stacks_ram_start[];
extern char z_priv_stacks_ram_end[];
extern char z_user_stacks_start[];
extern char z_user_stacks_end[];
#endif /* CONFIG_USERSPACE */

#endif /* ! _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_DEFS_H_ */
