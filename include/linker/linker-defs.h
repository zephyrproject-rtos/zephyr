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

#ifndef _LINKERDEFS_H
#define _LINKERDEFS_H

#include <toolchain.h>
#include <linker/sections.h>
#include <misc/util.h>

/* include platform dependent linker-defs */
#ifdef CONFIG_X86
/* Nothing yet to include */
#elif defined(CONFIG_ARM)
/* Nothing yet to include */
#elif defined(CONFIG_ARC)
/* Nothing yet to include */
#elif defined(CONFIG_NIOS2)
/* Nothing yet to include */
#elif defined(CONFIG_RISCV32)
/* Nothing yet to include */
#elif defined(CONFIG_XTENSA)
/* Nothing yet to include */
#else
#error Arch not supported.
#endif

#ifdef _LINKER


/*
 * Space for storing per device busy bitmap. Since we do not know beforehand
 * the number of devices, we go through the below mechanism to allocate the
 * required space.
 */
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#define DEVICE_COUNT \
	((__device_init_end - __device_init_start) / _DEVICE_STRUCT_SIZE)
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

#define	SHELL_INIT_SECTIONS()		\
		__shell_cmd_start = .;		\
		KEEP(*(".shell_*"));		\
		__shell_cmd_end = .;

#ifdef CONFIG_APPLICATION_MEMORY

#ifndef NUM_KERNEL_OBJECT_FILES
#error "Expected NUM_KERNEL_OBJECT_FILES to be defined"
#elif NUM_KERNEL_OBJECT_FILES > 19
#error "Max supported kernel objects is 19."
/* TODO: Using the preprocessor to do this was a mistake. Rewrite to
   scale better. e.g. by aggregating the kernel objects into two
   archives like KBuild did.*/
#endif

#define X(i, j)  KERNEL_OBJECT_FILE_##i (j)
#define Y(i, j) *KERNEL_OBJECT_FILE_##i

#define KERNEL_INPUT_SECTION(sect) \
    UTIL_LISTIFY(NUM_KERNEL_OBJECT_FILES, X, sect)
#define APP_INPUT_SECTION(sect)	\
    *(EXCLUDE_FILE (UTIL_LISTIFY(NUM_KERNEL_OBJECT_FILES, Y, ~)) sect)

#else
#define KERNEL_INPUT_SECTION(sect)	*(sect)
#define APP_INPUT_SECTION(sect)		*(sect)
#endif


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

#ifdef CONFIG_APPLICATION_MEMORY
/* Memory owned by the application. Start and end will be aligned for memory
 * management/protection hardware for the target architecture.

 * The policy for this memory will be to configure all of it as user thread
 * accessible. It consists of all non-kernel globals.
 */
extern char __app_ram_start[];
extern char __app_ram_end[];
extern char __app_ram_size[];
#endif

/* Memory owned by the kernel. Start and end will be aligned for memory
 * management/protection hardware for the target architecture..
 *
 * Consists of all kernel-side globals, all kernel objects, all thread stacks,
 * and all currently unused RAM.  If CONFIG_APPLICATION_MEMORY is not enabled,
 * has all globals, not just kernel side.
 *
 * Except for the stack of the currently executing thread, none of this memory
 * is normally accessible to user threads unless specifically granted at
 * runtime.
 */
extern char __kernel_ram_start[];
extern char __kernel_ram_end[];
extern char __kernel_ram_size[];

/* Used by _bss_zero or arch-specific implementation */
extern char __bss_start[];
extern char __bss_end[];
#ifdef CONFIG_APPLICATION_MEMORY
extern char __app_bss_start[];
extern char __app_bss_end[];
#endif

/* Used by _data_copy() or arch-specific implementation */
#ifdef CONFIG_XIP
extern char __data_rom_start[];
extern char __data_ram_start[];
extern char __data_ram_end[];
#ifdef CONFIG_APPLICATION_MEMORY
extern char __app_data_rom_start[];
extern char __app_data_ram_start[];
extern char __app_data_ram_end[];
#endif /* CONFIG_APPLICATION_MEMORY */
#endif /* CONFIG_XIP */

/* Includes text and rodata */
extern char _image_rom_start[];
extern char _image_rom_end[];
extern char _image_rom_size[];

/* datas, bss, noinit */
extern char _image_ram_start[];
extern char _image_ram_end[];

extern char _image_text_start[];
extern char _image_text_end[];

extern char _image_rodata_start[];
extern char _image_rodata_end[];

extern char _vector_start[];
extern char _vector_end[];

/* end address of image, used by newlib for the heap */
extern char _end[];

#endif /* ! _ASMLANGUAGE */

#endif /* _LINKERDEFS_H */
