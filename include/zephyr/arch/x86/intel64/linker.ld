/*
 * Copyright (c) 2019-2021 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define ROMABLE_REGION RAM
#define RAMABLE_REGION RAM

#define MMU_PAGE_ALIGN		. = ALIGN(CONFIG_MMU_PAGE_SIZE);

/* Used to align areas with separate memory permission characteristics
 * so that the page permissions can be set in the MMU. Without this,
 * the kernel is just one blob with the same RWX permissions on all RAM
 */
#ifdef CONFIG_SRAM_REGION_PERMISSIONS
	#define MMU_PAGE_ALIGN_PERM	MMU_PAGE_ALIGN
#else
	#define MMU_PAGE_ALIGN_PERM
#endif

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{
	/*
	 * The "locore" must be in the 64K of RAM, so that 16-bit code (with
	 * segment registers == 0x0000) and 32/64-bit code agree on addresses.
	 * ... there is no 16-bit code yet, but there will be when we add SMP.
	 */

	SECTION_PROLOGUE(.locore,,)
	{
	_locore_start = .;
	*(.locore)
	*(.locore.*)
	MMU_PAGE_ALIGN_PERM
	_locore_end = .;

	_lorodata_start = .;
	*(.lorodata)
	MMU_PAGE_ALIGN_PERM
	_lodata_start = .;

	*(.lodata)

#ifdef CONFIG_X86_KPTI
	/* Special page containing supervisor data that is still mapped in
	 * user mode page tables. GDT, TSSes, trampoline stack, and
	 * any LDT must go here as they always must live in a page that is
	 * marked 'present'. Still not directly user accessible, but
	 * no sensitive data should be here as Meltdown exploits may read it.
	 *
	 * On x86-64 the IDT is in rodata and doesn't need to be in the
	 * trampoline page.
	 */
	MMU_PAGE_ALIGN_PERM
	z_shared_kernel_page_start = .;
#endif /* CONFIG_X86_KPTI */

        *(.boot_arg)
	*(.tss)
	*(.gdt)

#ifdef CONFIG_X86_KPTI
	*(.trampolines)
	MMU_PAGE_ALIGN_PERM
	z_shared_kernel_page_end = .;

	ASSERT(z_shared_kernel_page_end - z_shared_kernel_page_start == 4096,
	       "shared kernel area is not one memory page");
#endif /* CONFIG_X86_KPTI */

	. = ALIGN(CONFIG_MMU_PAGE_SIZE);
	_lodata_end = .;
	} > LOCORE

	_locore_size = _lorodata_start - _locore_start;
	_lorodata_size = _lodata_start - _lorodata_start;
	_lodata_size = _lodata_end - _lodata_start;

	/*
	 * The rest of the system is loaded in "normal" memory (typically
	 * placed above 1MB to avoid the by memory hole at 0x90000-0xFFFFF).
	 */

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	. = ALIGN(16);
	__rom_region_start = .;
	__text_region_start = .;
	z_mapped_start = .;
	*(.text)
	*(.text.*)

	#include <zephyr/linker/kobject-text.ld>

	MMU_PAGE_ALIGN_PERM
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__text_region_end = .;
	__text_region_size = __text_region_end - __text_region_start;
	__rodata_region_start = .;

	#include <zephyr/linker/common-rom.ld>
	/* Located in generated directory. This file is populated by calling
	 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
	 */
	#include <snippets-rom-sections.ld>
	#include <zephyr/linker/thread-local-storage.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
	. = ALIGN(16);
	*(.rodata)
	*(.rodata.*)

	MMU_PAGE_ALIGN
	#include <snippets-rodata.ld>

#ifdef CONFIG_X86_MMU
	. = ALIGN(8);
	_mmu_region_list_start = .;
	KEEP(*("._mmu_region.static.*"))
	_mmu_region_list_end = .;
#endif /* CONFIG_X86_MMU */

	#include <zephyr/linker/kobject-rom.ld>
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>

	MMU_PAGE_ALIGN_PERM
	__rodata_region_end = .;
	__rodata_region_size = __rodata_region_end - __rodata_region_start;
	__rom_region_end = .;

#ifdef CONFIG_USERSPACE
	/* APP SHARED MEMORY REGION */
#define SMEM_PARTITION_ALIGN(size) MMU_PAGE_ALIGN_PERM
#define APP_SHARED_ALIGN  MMU_PAGE_ALIGN_PERM

#include <app_smem.ld>

	_image_ram_start = _app_smem_start;
	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_num_words = _app_smem_size >> 2;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
	_app_smem_num_words = _app_smem_size >> 2;
#endif /* CONFIG_USERSPACE */

/* This should be put here before BSS section, otherwise the .bss.__gcov will
 * be put in BSS section. That causes gcov not work properly */
#include <snippets-ram-sections.ld>

	SECTION_PROLOGUE(_BSS_SECTION_NAME, (NOLOAD),)
	{
	. = ALIGN(16);
	MMU_PAGE_ALIGN_PERM
#ifndef CONFIG_USERSPACE
	_image_ram_start = .;
#endif
	__kernel_ram_start = .;
	__bss_start = .;
	*(.bss)
	*(.bss.*)
	*(COMMON)
	. = ALIGN(4);	/* so __bss_num_dwords is exact */
	__bss_end = .;
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__bss_num_dwords = (__bss_end - __bss_start) >> 2;

#include <zephyr/linker/common-noinit.ld>

#include <snippets-sections.ld>

	SECTION_PROLOGUE(_DATA_SECTION_NAME,,)
	{
	. = ALIGN(16);
	*(.data)
	*(.data.*)
	#include <snippets-rwdata.ld>
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/cplusplus-ram.ld>
#include <zephyr/arch/x86/pagetables.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

/* Must be last in RAM */
#include <zephyr/linker/kobject-data.ld>

#define LAST_RAM_ALIGN MMU_PAGE_ALIGN

#include <zephyr/linker/ram-end.ld>

    GROUP_END(RAMABLE_REGION)

	/* All unused memory also owned by the kernel for heaps */
	__kernel_ram_end = KERNEL_BASE_ADDR + KERNEL_RAM_SIZE;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

	z_mapped_size = z_mapped_end - z_mapped_start;

#include <zephyr/linker/debug-sections.ld>

	/DISCARD/ :
	{
	*(.got)
	*(.got.plt)
	*(.igot)
	*(.igot.plt)
	*(.iplt)
	*(.plt)
	*(.note.GNU-stack)
	*(.rel.*)
	*(.rela.*)
	}
/*
 * eh_frame section won't be removed even with "--gc-sections" by LLVM lld.
 */
#if !defined(CONFIG_CPP_EXCEPTIONS)
	/DISCARD/ : { *(.eh_frame) }
#endif

/*
 * The sections below are still treated as warnings
 * with "--orphan-handling=warn" by LLVM lld.
 */
#if !defined(CONFIG_LLVM_USE_LD)
	.symtab 0 : { *(.symtab) }
	.strtab 0 : { *(.strtab) }
	.shstrtab 0 : { *(.shstrtab) }
#endif

#ifdef CONFIG_LLEXT
	#include <zephyr/linker/llext-sections.ld>
#endif
}
