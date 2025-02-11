/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common linker sections
 *
 * This script defines the memory location of the various sections that make up
 * a Zephyr Kernel image. This file is used by the linker.
 *
 * This script places the various sections of the image according to what
 * features are enabled by the kernel's configuration options.
 *
 * For a build that does not use the execute in place (XIP) feature, the script
 * generates an image suitable for loading into and executing from RAMABLE_REGION by
 * placing all the sections adjacent to each other.  There is also no separate
 * load address for the DATA section which means it doesn't have to be copied
 * into RAMABLE_REGION.
 *
 * For builds using XIP, there is a different load memory address (LMA) and
 * virtual memory address (VMA) for the DATA section.  In this case the DATA
 * section is copied into RAMABLE_REGION at runtime.
 *
 * When building an XIP image the data section is placed into ROMABLE_REGION.  In this
 * case, the LMA is set to __data_rom_start so the data section is concatenated
 * at the end of the RODATA section.  At runtime, the DATA section is copied
 * into the RAMABLE_REGION region so it can be accessed with read and write permission.
 *
 * Most symbols defined in the sections below are subject to be referenced in
 * the Zephyr Kernel image. If a symbol is used but not defined the linker will
 * emit an undefined symbol error.
 *
 * Please do not change the order of the section as the kernel expects this
 * order when programming the MMU.
 */

#include <zephyr/linker/linker-defs.h>
#include <zephyr/offsets.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel/mm.h>

#include <zephyr/linker/linker-tool.h>

#if defined(CONFIG_XIP) || defined(K_MEM_IS_VM_KERNEL)
	#define ROMABLE_REGION ROM
	#define RAMABLE_REGION RAM
#else
	#define ROMABLE_REGION RAM
	#define RAMABLE_REGION RAM
#endif

#ifdef CONFIG_MMU
	#define MMU_PAGE_ALIGN		. = ALIGN(CONFIG_MMU_PAGE_SIZE);
#else
	#define MMU_PAGE_ALIGN
#endif

/* Used to align areas with separate memory permission characteristics
 * so that the page permissions can be set in the MMU. Without this,
 * the kernel is just one blob with the same RWX permissions on all RAM
 */
#ifdef CONFIG_SRAM_REGION_PERMISSIONS
	#define MMU_PAGE_ALIGN_PERM	MMU_PAGE_ALIGN
#else
	#define MMU_PAGE_ALIGN_PERM
#endif

epoint = K_MEM_PHYS_ADDR(CONFIG_KERNEL_ENTRY);
ENTRY(epoint)

/* SECTIONS definitions */
SECTIONS
	{

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

    /DISCARD/ :
	{
	*(.plt)
	}

    /DISCARD/ :
	{
	*(.iplt)
	}

#if defined(CONFIG_SOC_FAMILY_INTEL_ISH) && defined(CONFIG_PM)
#include <zephyr/arch/x86/ia32/scripts/ish_aon.ld>
#endif

#ifdef CONFIG_LINKER_USE_BOOT_SECTION

	SECTION_PROLOGUE(boot.text,,)
	{
#include <snippets-rom-start.ld>

		MMU_PAGE_ALIGN
		lnkr_boot_start = .;
		z_mapped_start = .;

		lnkr_boot_text_start = .;

		KEEP(*(.boot_text.__start))
		*(.boot_text)
		*(.boot_text.*)

		MMU_PAGE_ALIGN_PERM

		lnkr_boot_text_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(boot.rodata,,)
	{
		MMU_PAGE_ALIGN_PERM

		lnkr_boot_rodata_start = .;

		*(.boot_rodata)
		*(.boot_rodata.*)

		MMU_PAGE_ALIGN_PERM

		lnkr_boot_rodata_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(boot.data,,)
	{
		MMU_PAGE_ALIGN_PERM

		. = ALIGN(4);

		lnkr_boot_data_start = .;

		*(.boot_data)
		*(.boot_data.*)

		lnkr_boot_data_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(boot.bss, (NOLOAD),)
	{
		. = ALIGN(4);

		lnkr_boot_bss_start = .;

		*(.boot_bss)
		*(.boot_bss.*)

		lnkr_boot_bss_end = .;
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(boot.noinit, (NOLOAD),)
	{
		. = ALIGN(4);

		lnkr_boot_noinit_start = .;

		*(.boot_noinit)
		*(.boot_noinit.*)

		lnkr_boot_noinit_end = .;

		MMU_PAGE_ALIGN

		lnkr_boot_end = .;
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	lnkr_boot_text_size = lnkr_boot_text_end - lnkr_boot_text_start;
	lnkr_boot_rodata_size = lnkr_boot_rodata_end - lnkr_boot_rodata_start;
	lnkr_boot_data_size = lnkr_boot_data_end - lnkr_boot_data_start;
	lnkr_boot_bss_size = lnkr_boot_bss_end - lnkr_boot_bss_start;
	lnkr_boot_noinit_size = lnkr_boot_noinit_end - lnkr_boot_noinit_start;

#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#ifdef CONFIG_LINKER_USE_PINNED_SECTION

	SECTION_PROLOGUE(pinned.text,,)
	{
#ifndef CONFIG_LINKER_USE_BOOT_SECTION
#include <snippets-rom-start.ld>
#endif

		MMU_PAGE_ALIGN

		lnkr_pinned_start = .;

#ifndef CONFIG_LINKER_USE_BOOT_SECTION
		z_mapped_start = .;
#endif

		lnkr_pinned_text_start = .;

		*(.pinned_text)
		*(.pinned_text.*)

		*(.gnu.linkonce.t.exc_*)

#include <zephyr/linker/kobject-text.ld>

		MMU_PAGE_ALIGN_PERM

		lnkr_pinned_text_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	MMU_PAGE_ALIGN_PERM

	lnkr_pinned_rodata_start = .;

#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
#include <zephyr/linker/thread-local-storage.ld>

	SECTION_PROLOGUE(pinned.rodata,,)
	{
#include <zephyr/arch/x86/ia32/scripts/static_intr.ld>

		*(.pinned_rodata)
		*(.pinned_rodata.*)

#include <snippets-rodata.ld>

#include <zephyr/linker/kobject-rom.ld>

		MMU_PAGE_ALIGN_PERM

		lnkr_pinned_rodata_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(pinned.data,,)
	{
		MMU_PAGE_ALIGN_PERM

		lnkr_pinned_data_start = .;

		. = ALIGN(4);

#include <zephyr/arch/x86/ia32/scripts/shared_kernel_pages.ld>
#include <zephyr/arch/x86/ia32/scripts/dynamic_intr.ld>

		*(.pinned_data)
		*(.pinned_data.*)

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/common-ram.ld>
#include <zephyr/arch/x86/pagetables.ld>
#include <zephyr/linker/kobject-data.ld>

	lnkr_pinned_data_end = .;

	SECTION_PROLOGUE(pinned.bss, (NOLOAD),)
	{
		. = ALIGN(4);

		lnkr_pinned_bss_start = .;

		*(.pinned_bss)
		*(.pinned_bss.*)

		lnkr_pinned_bss_end = .;
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(pinned.noinit, (NOLOAD),)
	{
		. = ALIGN(4);

		lnkr_pinned_noinit_start = .;

		*(.pinned_noinit)
		*(.pinned_noinit.*)

		lnkr_pinned_noinit_end = .;

		MMU_PAGE_ALIGN

		lnkr_pinned_end = .;

	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	lnkr_pinned_text_size = lnkr_pinned_text_end - lnkr_pinned_text_start;
	lnkr_pinned_rodata_size = lnkr_pinned_rodata_end - lnkr_pinned_rodata_start;
	lnkr_pinned_data_size = lnkr_pinned_data_end - lnkr_pinned_data_start;
	lnkr_pinned_bss_size = lnkr_pinned_bss_end - lnkr_pinned_bss_start;
	lnkr_pinned_noinit_size = lnkr_pinned_noinit_end - lnkr_pinned_noinit_start;

#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

	GROUP_START(ROMABLE_REGION)

	. = ALIGN(8);

#ifdef CONFIG_XIP
	__rom_region_start = PHYS_LOAD_ADDR;
#endif
	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	__text_region_start = .;

#if !defined(CONFIG_LINKER_USE_BOOT_SECTION) || \
    !defined(CONFIG_LINKER_USE_PINNED_SECTION)
#ifndef CONFIG_XIP
	z_mapped_start = .;
#endif
#endif

#if !defined(CONFIG_LINKER_USE_BOOT_SECTION) || \
    !defined(CONFIG_LINKER_USE_PINNED_SECTION)
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_START ...). This typically contains the vector
 * table and debug information.
 */
#include <snippets-rom-start.ld>
#endif

	/* Needs KEEP() as ENTRY() is given a physical address */
	KEEP(*(.text.__start))
/*
 * We need these sections to extract interrupt information, but they
 * will be removed with "--gc-sections" by LLVM lld, so add keep
 * command to save them.
 */
#ifndef CONFIG_LLVM_USE_LD
	KEEP(*(.text.irqstubs))
	KEEP(*(".gnu.linkonce.t.exc_*_stub"))
#endif
	*(.text)
	*(".text.*")
	*(.gnu.linkonce.t.*)
	*(.init)
	*(.fini)
	*(.eini)

#ifndef CONFIG_LINKER_USE_PINNED_SECTION
#include <zephyr/linker/kobject-text.ld>
#endif

	MMU_PAGE_ALIGN_PERM
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__text_region_end = .;
	__text_region_size = __text_region_end - __text_region_start;
	__rodata_region_start = .;

#ifndef CONFIG_LINKER_USE_PINNED_SECTION
#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
#include <zephyr/linker/thread-local-storage.ld>
#endif

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
	*(.rodata)
	*(".rodata.*")
	*(.gnu.linkonce.r.*)

#ifndef CONFIG_DYNAMIC_INTERRUPTS
#ifndef CONFIG_LINKER_USE_PINNED_SECTION
#include <zephyr/arch/x86/ia32/scripts/static_intr.ld>
#endif /* !CONFIG_LINKER_USE_PINNED_SECTION */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

#ifndef CONFIG_LINKER_USE_PINNED_SECTION
/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <zephyr/linker/kobject-rom.ld>
#endif
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>

	MMU_PAGE_ALIGN_PERM
	/* ROM ends here, position counter will now be in RAM areas */
#ifdef CONFIG_XIP
	__rom_region_end = .;
	__rom_region_size = __rom_region_end - __rom_region_start;
#endif
	__rodata_region_end = .;
	__rodata_region_size = __rodata_region_end - __rodata_region_start;
	GROUP_END(ROMABLE_REGION)
	/*
	 * Needed for dynamic linking which we do not have, do discard
	 */
	/DISCARD/ : {
		*(.got.plt)
		*(.igot.plt)
		*(.got)
		*(.igot)
	}
	/* RAMABLE_REGION */
	GROUP_START(RAMABLE_REGION)

#ifdef CONFIG_XIP
	MMU_PAGE_ALIGN
	z_mapped_start = .;
#endif
/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

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

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{

	MMU_PAGE_ALIGN_PERM
	__data_region_start = .;
	__data_start = .;

	*(.data)
	*(".data.*")
	*(".kernel.*")

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#ifndef CONFIG_LINKER_USE_PINNED_SECTION
#include <zephyr/arch/x86/ia32/scripts/dynamic_intr.ld>
#endif /* !CONFIG_LINKER_USE_PINNED_SECTION */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

#ifndef CONFIG_LINKER_USE_PINNED_SECTION
#include <zephyr/arch/x86/ia32/scripts/shared_kernel_pages.ld>
#endif /* !CONFIG_LINKER_USE_PINNED_SECTION */

	. = ALIGN(4);
	__data_end = .;

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	__data_size = __data_end - __data_start;
	__data_load_start = LOADADDR(_DATA_SECTION_NAME);

	__data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

#include <zephyr/linker/cplusplus-ram.ld>

#ifndef CONFIG_LINKER_USE_PINNED_SECTION
#include <zephyr/linker/common-ram.ld>
#include <zephyr/arch/x86/pagetables.ld>

/* Must be last in RAM */
#include <zephyr/linker/kobject-data.ld>
#endif /* !CONFIG_LINKER_USE_PINNED_SECTION */

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

	MMU_PAGE_ALIGN
	__data_region_end = .;

	SECTION_PROLOGUE(_BSS_SECTION_NAME, (NOLOAD),)
	{
	MMU_PAGE_ALIGN_PERM
#if !defined(CONFIG_USERSPACE)
	_image_ram_start = .;
#endif
	/*
	 * For performance, BSS section is forced to be both 4 byte aligned and
	 * a multiple of 4 bytes.
	 */
	. = ALIGN(4);
	__kernel_ram_start = .;
	__bss_start = .;

	*(.bss)
	*(".bss.*")
	*(COMMON)
	*(".kernel_bss.*")

	/*
	 * As memory is cleared in words only, it is simpler to ensure the BSS
	 * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
	 */
	. = ALIGN(4);
	__bss_end = .;
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__bss_num_words	= (__bss_end - __bss_start) >> 2;

#include <zephyr/linker/common-noinit.ld>

	MMU_PAGE_ALIGN_PERM

	/* All unused memory also owned by the kernel for heaps */
	__kernel_ram_end = KERNEL_BASE_ADDR + KERNEL_RAM_SIZE;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

	_image_ram_all = (KERNEL_BASE_ADDR + KERNEL_RAM_SIZE) - _image_ram_start;

	z_mapped_end = .;
	z_mapped_size = z_mapped_end - z_mapped_start;

#ifndef LINKER_ZEPHYR_FINAL
	/* static interrupts */
	SECTION_PROLOGUE(intList,,)
	{
	KEEP(*(.spurIsr))
	KEEP(*(.spurNoErrIsr))
	KEEP(*(.intList))
	KEEP(*(.gnu.linkonce.intList.*))
	} > IDT_LIST
#else
	/DISCARD/ :
	{
	KEEP(*(.spurIsr))
	KEEP(*(.spurNoErrIsr))
	KEEP(*(.intList))
	KEEP(*(.gnu.linkonce.intList.*))
	}
#endif



/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#include <zephyr/linker/ram-end.ld>

    GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

	/DISCARD/ : { *(.note.GNU-stack) }
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
	}

#ifdef CONFIG_XIP
/*
 * Round up number of words for DATA section to ensure that XIP copies the
 * entire data section. XIP copy is done in words only, so there may be up
 * to 3 extra bytes copied in next section (BSS). At run time, the XIP copy
 * is done first followed by clearing the BSS section.
 */
__data_size = (__data_region_end - __data_region_start);
__data_num_words = (__data_size + 3) >> 2;
#endif
