/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define ROMABLE_REGION RAM
#define RAMABLE_REGION RAM

#ifdef CONFIG_X86_MMU
	#define MMU_PAGE_SIZE	KB(4)
	#define MMU_PAGE_ALIGN	. = ALIGN(MMU_PAGE_SIZE);
#else
	#define MMU_PAGE_ALIGN
#endif

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{
	/*
	 * The "locore" must be in the 64K of RAM, so that 16-bit code (with
	 * segment registers == 0x0000) and 32/64-bit code agree on addresses.
	 * ... there is no 16-bit code yet, but there will be when we add SMP.
	 */

	.locore 0x8000 : ALIGN(16)
	{
	_locore_start = .;
	*(.locore)
	*(.locore.*)
	MMU_PAGE_ALIGN
	_locore_end = .;

	_lorodata_start = .;
	*(.lorodata)
	MMU_PAGE_ALIGN
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
	MMU_PAGE_ALIGN
	z_shared_kernel_page_start = .;
#endif /* CONFIG_X86_KPTI */

	*(.tss)
	*(.gdt)

#ifdef CONFIG_X86_KPTI
	*(.trampolines)
	MMU_PAGE_ALIGN
	z_shared_kernel_page_end = .;

	ASSERT(z_shared_kernel_page_end - z_shared_kernel_page_start == 4096,
	       "shared kernel area is not one memory page");
#endif /* CONFIG_X86_KPTI */

	MMU_PAGE_ALIGN
	_lodata_end = .;
	}

	_locore_size = _lorodata_start - _locore_start;
	_lorodata_size = _lodata_start - _lorodata_start;
	_lodata_size = _lodata_end - _lodata_start;

	/*
	 * The rest of the system is loaded in "normal" memory (typically
	 * placed above 1MB to avoid the by memory hole at 0x90000-0xFFFFF).
	 */

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,ALIGN(16))
	{
	_image_rom_start = .;
	_image_text_start = .;
	*(.text)
	*(.text.*)

	#include <linker/kobject-text.ld>

	MMU_PAGE_ALIGN
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;
	_image_text_size = _image_text_end - _image_text_start;
	_image_rodata_start = .;

	#include <linker/common-rom.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,ALIGN(16))
	{
	*(.rodata)
	*(.rodata.*)

	#include <snippets-rodata.ld>

#ifdef CONFIG_X86_MMU
	. = ALIGN(8);
	_mmu_region_list_start = .;
	KEEP(*("._mmu_region.static.*"))
	_mmu_region_list_end = .;
#endif /* CONFIG_X86_MMU */

	#include <linker/kobject-rom.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/cplusplus-rom.ld>

	MMU_PAGE_ALIGN
	_image_rodata_end = .;
	_image_rodata_size = _image_rodata_end - _image_rodata_start;
	_image_rom_end = .;

#ifdef CONFIG_USERSPACE
	/* APP SHARED MEMORY REGION */
#define SMEM_PARTITION_ALIGN(size) MMU_PAGE_ALIGN
#define APP_SHARED_ALIGN  MMU_PAGE_ALIGN

#include <app_smem.ld>

	_image_ram_start = _app_smem_start;
	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_num_words = _app_smem_size >> 2;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
	_app_smem_num_words = _app_smem_size >> 2;
#endif /* CONFIG_USERSPACE */

	SECTION_PROLOGUE(_BSS_SECTION_NAME, (NOLOAD), ALIGN(16))
	{
	MMU_PAGE_ALIGN
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
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	__bss_num_dwords = (__bss_end - __bss_start) >> 2;

#include <linker/common-noinit.ld>

#include <snippets-sections.ld>

	SECTION_PROLOGUE(_DATA_SECTION_NAME,,ALIGN(16))
	{
	*(.data)
	*(.data.*)
	#include <snippets-rwdata.ld>
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <snippets-ram-sections.ld>
#include <linker/common-ram.ld>
#include <linker/cplusplus-ram.ld>
#include <linker/kobject.ld>

	. = ALIGN(8);
	MMU_PAGE_ALIGN
	_image_ram_end = .;
	_end = .;

	/* All unused memory also owned by the kernel for heaps */
	__kernel_ram_end = KERNEL_BASE_ADDR + KERNEL_RAM_SIZE;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

#include <linker/debug-sections.ld>

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
}
