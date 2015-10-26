/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#include <sections.h>

/* include platform dependent linker-defs */
#ifdef CONFIG_X86
#include <arch/x86/linker-defs-arch.h>
#elif defined(CONFIG_ARM)
/* Nothing yet to include */
#elif defined(CONFIG_ARC)
/* Nothing yet to include */
#else
#error Arch not supported.
#endif

#ifdef _LINKER

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
		__device_init_start = .;		\
		DEVICE_INIT_LEVEL(PRIMARY)		\
		DEVICE_INIT_LEVEL(SECONDARY)	\
		DEVICE_INIT_LEVEL(NANOKERNEL)	\
		DEVICE_INIT_LEVEL(MICROKERNEL)	\
		DEVICE_INIT_LEVEL(APPLICATION)	\
		__device_init_end = .;			\


/* define a section for undefined device initialization levels */
#define DEVICE_INIT_UNDEFINED_SECTION()		\
		KEEP(*(SORT(.init_[_A-Z0-9]*)))	\

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

#include <stdint.h>
extern char __bss_start[];
extern int __bss_num_words[];
#ifdef CONFIG_XIP
extern char __data_rom_start[];
extern char __data_ram_start[];
extern int __data_num_words[];
#endif

extern char _image_rom_start[];
extern char _image_rom_end[];
extern char _image_ram_start[];
extern char _image_ram_end[];
extern char _image_text_start[];
extern char _image_text_end[];

/* end address of image. */
extern char _end[];

#endif /* ! _ASMLANGUAGE */

#endif /* _LINKERDEFS_H */
