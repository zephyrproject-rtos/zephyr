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

#include <zephyr/toolchain.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/offsets.h>

/* We need to dummy out DT_NODE_HAS_STATUS when building the unittests.
 * Including devicetree.h would require generating dummy header files
 * to match what gen_defines creates, so it's easier to just dummy out
 * DT_NODE_HAS_STATUS.
 */
#ifdef ZTEST_UNITTEST
#define DT_NODE_HAS_STATUS(node, status) 0
#define DT_NODE_HAS_STATUS_OKAY(node) 0
#else
#include <zephyr/devicetree.h>
#endif

#ifdef _LINKER
/*
 * generate a symbol to mark the start of the objects array for
 * the specified object and level, then link all of those objects
 * (sorted by priority). Ensure the objects aren't discarded if there is
 * no direct reference to them
 */
#define CREATE_OBJ_LEVEL(object, level)				\
		__##object##_##level##_start = .;		\
		KEEP(*(SORT(.z_##object##_##level?_*)));	\
		KEEP(*(SORT(.z_##object##_##level??_*)));

/*
 * link in shell initialization objects for all modules that use shell and
 * their shell commands are automatically initialized by the kernel.
 */

#elif defined(_ASMLANGUAGE)

/* Assembly FILES: declaration defined by the linker script */
GDATA(__bss_start)
GDATA(__bss_num_words)
#ifdef CONFIG_XIP
GDATA(__data_region_load_start)
GDATA(__data_region_start)
GDATA(__data_region_num_words)
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

#ifdef CONFIG_LINKER_USE_PINNED_SECTION
extern char _app_smem_pinned_start[];
extern char _app_smem_pinned_end[];
extern char _app_smem_pinned_size[];
extern char _app_smem_pinned_num_words[];
#endif

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
extern char __data_region_load_start[];
extern char __data_region_start[];
extern char __data_region_end[];
#endif /* CONFIG_XIP */

#ifdef CONFIG_MMU
/* Virtual addresses of page-aligned kernel image mapped into RAM at boot */
extern char z_mapped_start[];
extern char z_mapped_end[];
#endif /* CONFIG_MMU */

/* Includes text and rodata */
extern char __rom_region_start[];
extern char __rom_region_end[];
extern char __rom_region_size[];

/* Includes all ROMable data, i.e. the size of the output image file. */
extern char _flash_used[];

/* datas, bss, noinit */
extern char _image_ram_start[];
extern char _image_ram_end[];
extern char _image_ram_size[];

extern char __text_region_start[];
extern char __text_region_end[];
extern char __text_region_size[];

extern char __rodata_region_start[];
extern char __rodata_region_end[];
extern char __rodata_region_size[];

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

#if (DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_ccm)))
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

#if (DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_itcm)))
extern char __itcm_start[];
extern char __itcm_end[];
extern char __itcm_size[];
extern char __itcm_load_start[];
#endif

#if (DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm)))
extern char __dtcm_data_start[];
extern char __dtcm_data_end[];
extern char __dtcm_bss_start[];
extern char __dtcm_bss_end[];
extern char __dtcm_noinit_start[];
extern char __dtcm_noinit_end[];
extern char __dtcm_data_load_start[];
extern char __dtcm_start[];
extern char __dtcm_end[];
#endif

#if (DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_ocm)))
extern char __ocm_data_start[];
extern char __ocm_data_end[];
extern char __ocm_bss_start[];
extern char __ocm_bss_end[];
extern char __ocm_start[];
extern char __ocm_end[];
extern char __ocm_size[];
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
extern char __ramfunc_region_start[];
extern char __ramfunc_start[];
extern char __ramfunc_end[];
extern char __ramfunc_size[];
extern char __ramfunc_load_start[];
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
extern char z_kobject_data_begin[];
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_THREAD_LOCAL_STORAGE
extern char __tdata_start[];
extern char __tdata_end[];
extern char __tdata_size[];
extern char __tdata_align[];
extern char __tbss_start[];
extern char __tbss_end[];
extern char __tbss_size[];
extern char __tbss_align[];
extern char __tls_start[];
extern char __tls_end[];
extern char __tls_size[];
#endif /* CONFIG_THREAD_LOCAL_STORAGE */

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
/* lnkr_boot_start[] and lnkr_boot_end[]
 * must encapsulate all the boot sections.
 */
extern char lnkr_boot_start[];
extern char lnkr_boot_end[];

extern char lnkr_boot_text_start[];
extern char lnkr_boot_text_end[];
extern char lnkr_boot_text_size[];
extern char lnkr_boot_data_start[];
extern char lnkr_boot_data_end[];
extern char lnkr_boot_data_size[];
extern char lnkr_boot_rodata_start[];
extern char lnkr_boot_rodata_end[];
extern char lnkr_boot_rodata_size[];
extern char lnkr_boot_bss_start[];
extern char lnkr_boot_bss_end[];
extern char lnkr_boot_bss_size[];
extern char lnkr_boot_noinit_start[];
extern char lnkr_boot_noinit_end[];
extern char lnkr_boot_noinit_size[];
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#ifdef CONFIG_LINKER_USE_PINNED_SECTION
/* lnkr_pinned_start[] and lnkr_pinned_end[] must encapsulate
 * all the pinned sections as these are used by
 * the MMU code to mark the physical page frames with
 * K_MEM_PAGE_FRAME_PINNED.
 */
extern char lnkr_pinned_start[];
extern char lnkr_pinned_end[];

extern char lnkr_pinned_text_start[];
extern char lnkr_pinned_text_end[];
extern char lnkr_pinned_text_size[];
extern char lnkr_pinned_data_start[];
extern char lnkr_pinned_data_end[];
extern char lnkr_pinned_data_size[];
extern char lnkr_pinned_rodata_start[];
extern char lnkr_pinned_rodata_end[];
extern char lnkr_pinned_rodata_size[];
extern char lnkr_pinned_bss_start[];
extern char lnkr_pinned_bss_end[];
extern char lnkr_pinned_bss_size[];
extern char lnkr_pinned_noinit_start[];
extern char lnkr_pinned_noinit_end[];
extern char lnkr_pinned_noinit_size[];

__pinned_func
static inline bool lnkr_is_pinned(uint8_t *addr)
{
	if ((addr >= (uint8_t *)lnkr_pinned_start) &&
	    (addr < (uint8_t *)lnkr_pinned_end)) {
		return true;
	} else {
		return false;
	}
}

__pinned_func
static inline bool lnkr_is_region_pinned(uint8_t *addr, size_t sz)
{
	if ((addr >= (uint8_t *)lnkr_pinned_start) &&
	    ((addr + sz) < (uint8_t *)lnkr_pinned_end)) {
		return true;
	} else {
		return false;
	}
}

#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

#ifdef CONFIG_LINKER_USE_ONDEMAND_SECTION
/* lnkr_ondemand_start[] and lnkr_ondemand_end[] must encapsulate
 * all the on-demand sections as these are used by
 * the MMU code to mark the virtual pages with the appropriate backing store
 * location token to have them be paged in on demand.
 */
extern char lnkr_ondemand_start[];
extern char lnkr_ondemand_end[];
extern char lnkr_ondemand_load_start[];

extern char lnkr_ondemand_text_start[];
extern char lnkr_ondemand_text_end[];
extern char lnkr_ondemand_text_size[];
extern char lnkr_ondemand_rodata_start[];
extern char lnkr_ondemand_rodata_end[];
extern char lnkr_ondemand_rodata_size[];

#endif /* CONFIG_LINKER_USE_ONDEMAND_SECTION */
#endif /* ! _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_DEFS_H_ */
