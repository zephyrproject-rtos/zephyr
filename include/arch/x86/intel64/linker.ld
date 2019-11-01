/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#define _LINKER
#define _ASMLANGUAGE

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
	_lorodata_start = .;
	*(.lorodata)
	MMU_PAGE_ALIGN
	_lodata_start = .;
	*(.tss)
	*(.lodata)
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

	#ifdef CONFIG_CUSTOM_RODATA_LD
	#include <custom-rodata.ld>
	#endif /* CONFIG_CUSTOM_RODATA_LD */

#ifdef CONFIG_X86_MMU
	. = ALIGN(8);
	_mmu_region_list_start = .;
	KEEP(*("._mmu_region.static.*"))
	_mmu_region_list_end = .;
#endif /* CONFIG_X86_MMU */

	} GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/cplusplus-rom.ld>

	MMU_PAGE_ALIGN
	_image_rodata_end = .;
	_image_rodata_size = _image_rodata_end - _image_rodata_start;
	_image_rom_end = .;

	SECTION_PROLOGUE(_DATA_SECTION_NAME,,ALIGN(16))
	{
	MMU_PAGE_ALIGN
	_image_ram_start = .;
	__kernel_ram_start = .;
	*(.data)
	*(.data.*)
	#include <snippets-rwdata.ld>
	#ifdef CONFIG_CUSTOM_RWDATA_LD
	#include <custom-rwdata.ld>
	#endif /* CONFIG_CUSTOM_RWDATA_LD */
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <snippets-ram-sections.ld>
#include <linker/common-ram.ld>
#include <linker/cplusplus-ram.ld>

	SECTION_PROLOGUE(_BSS_SECTION_NAME, (NOLOAD), ALIGN(16))
	{
	__bss_start = .;
	*(.bss)
	*(.bss.*)
	*(COMMON)
	. = ALIGN(4);	/* so __bss_num_dwords is exact */
	__bss_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	__bss_num_dwords = (__bss_end - __bss_start) >> 2;

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD), ALIGN(16))
	{
	*(.noinit)
	*(.noinit.*)
	#include <snippets-noinit.ld>
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <snippets-sections.ld>
#ifdef CONFIG_CUSTOM_SECTIONS_LD
#include <custom-sections.ld>
#endif /* CONFIG_CUSTOM_SECTIONS_LD */

	. = ALIGN(8);
	_image_ram_end = .;
	_end = .;

	/* All unused memory also owned by the kernel for heaps */
	__kernel_ram_end = PHYS_RAM_ADDR + KB(DT_RAM_SIZE);
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
