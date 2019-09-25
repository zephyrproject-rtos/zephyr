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
#include <linker/sections.h>
#include <sys/util.h>
#include <offsets.h>

#ifdef _LINKER


/*
 * Space for storing per device busy bitmap. Since we do not know beforehand
 * the number of devices, we go through the below mechanism to allocate the
 * required space.
 */
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#define DEVICE_COUNT \
	((__device_init_end - __device_init_start) / _DEVICE_STRUCT_SIZEOF)
#define DEV_BUSY_SZ	(((DEVICE_COUNT + 31) / 32) * 4)
#define DEVICE_BUSY_BITFIELD()			\
		FILL(0x00) ;			\
		__device_busy_start = .;	\
		. = . + DEV_BUSY_SZ;		\
		__device_busy_end = .;
#else
#define DEVICE_BUSY_BITFIELD()
#endif

/*
 * generate a symbol to mark the start of the device initialization objects for
 * the specified level, then link all of those objects (sorted by priority);
 * ensure the objects aren't discarded if there is no direct reference to them
 */

#define DEVICE_INIT_LEVEL(level)				\
		__device_##level##_start = .;			\
		KEEP(*(SORT(.init_##level[0-9])));		\
		KEEP(*(SORT(.init_##level[1-9][0-9])));	\

/*
 * link in device initialization objects for all devices that are automatically
 * initialized by the kernel; the objects are sorted in the order they will be
 * initialized (i.e. ordered by level, sorted by priority within a level)
 */

#define	DEVICE_INIT_SECTIONS()			\
		__device_init_start = .;	\
		DEVICE_INIT_LEVEL(PRE_KERNEL_1)	\
		DEVICE_INIT_LEVEL(PRE_KERNEL_2)	\
		DEVICE_INIT_LEVEL(POST_KERNEL)	\
		DEVICE_INIT_LEVEL(APPLICATION)	\
		__device_init_end = .;		\
		DEVICE_BUSY_BITFIELD()		\


/* define a section for undefined device initialization levels */
#define DEVICE_INIT_UNDEFINED_SECTION()		\
		KEEP(*(SORT(.init_[_A-Z0-9]*)))	\

/*
 * link in shell initialization objects for all modules that use shell and
 * their shell commands are automatically initialized by the kernel.
 */

#define	SHELL_INIT_SECTIONS()				\
		__shell_module_start = .;		\
		KEEP(*(".shell_module_*"));		\
		__shell_module_end = .;			\
		__shell_cmd_start = .;			\
		KEEP(*(".shell_cmd_*"));		\
		__shell_cmd_end = .;			\

/*
 * link in shell initialization objects for all modules that use shell and
 * their shell commands are automatically initialized by the kernel.
 */

#define APP_SMEM_SECTION() KEEP(*(SORT("data_smem_*")))

#ifdef CONFIG_X86 /* LINKER FILES: defines used by linker script */
/* Should be moved to linker-common-defs.h */
#if defined(CONFIG_XIP)
#define ROMABLE_REGION ROM
#else
#define ROMABLE_REGION RAM
#endif
#endif

/*
 * If image is loaded via kexec Linux system call, then program
 * headers need to be page aligned.
 * This can be done by section page aligning.
 */
#ifdef CONFIG_BOOTLOADER_KEXEC
#define KEXEC_PGALIGN_PAD(x) . = ALIGN(x);
#else
#define KEXEC_PGALIGN_PAD(x)
#endif

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

#ifdef CONFIG_COVERAGE_GCOV
extern char __gcov_bss_start[];
extern char __gcov_bss_end[];
extern char __gcov_bss_size[];
#endif	/* CONFIG_COVERAGE_GCOV */

/* end address of image, used by newlib for the heap */
extern char _end[];

#ifdef DT_CCM_BASE_ADDRESS
extern char __ccm_data_rom_start[];
extern char __ccm_start[];
extern char __ccm_data_start[];
extern char __ccm_data_end[];
extern char __ccm_bss_start[];
extern char __ccm_bss_end[];
extern char __ccm_noinit_start[];
extern char __ccm_noinit_end[];
extern char __ccm_end[];
#endif /* DT_CCM_BASE_ADDRESS */

#ifdef DT_DTCM_BASE_ADDRESS
extern char __dtcm_data_start[];
extern char __dtcm_data_end[];
extern char __dtcm_bss_start[];
extern char __dtcm_bss_end[];
extern char __dtcm_noinit_start[];
extern char __dtcm_noinit_end[];
extern char __dtcm_data_rom_start[];
extern char __dtcm_start[];
extern char __dtcm_end[];
#endif /* DT_DTCM_BASE_ADDRESS */

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
#endif /* CONFIG_USERSPACE */

#endif /* ! _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_DEFS_H_ */
