/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <offsets.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel/mm.h>


/* Bounds of physical RAM from DTS */
#define PHYS_RAM_ADDR			DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define PHYS_RAM_SIZE			DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

/* Bounds of flash from DTS */
#define FLASH_ROM_ADDR			DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define FLASH_ROM_SIZE			DT_REG_SIZE(DT_CHOSEN(zephyr_flash))

/* Virtual base address for the kernel; with CONFIG_MMU this is not necessarily
 * the same as its physical location, although an identity mapping for RAM
 * is still supported by setting CONFIG_KERNEL_VM_BASE=CONFIG_SRAM_BASE_ADDRESS.
 */
#ifdef Z_VM_KERNEL

#define KERNEL_BASE_ADDR \
	(CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_OFFSET)

#define KERNEL_RAM_SIZE \
	(CONFIG_KERNEL_VM_SIZE - CONFIG_KERNEL_VM_OFFSET)

#define PHYS_RAM_AVAIL \
	(PHYS_RAM_SIZE - CONFIG_SRAM_OFFSET)

#else

#define KERNEL_BASE_ADDR		(PHYS_RAM_ADDR + CONFIG_SRAM_OFFSET)
#define KERNEL_RAM_SIZE			(PHYS_RAM_SIZE - CONFIG_SRAM_OFFSET)

#endif

/* "kernel RAM" for linker VMA allocations starts at the offset */

/* Physical RAM location where the kernel image is loaded */
#define PHYS_LOAD_ADDR			(PHYS_RAM_ADDR + CONFIG_SRAM_OFFSET)

#ifdef CONFIG_USERSPACE
#define SMEM_PARTITION_ALIGN(size)	MMU_PAGE_ALIGN_PERM
#define APP_SHARED_ALIGN		MMU_PAGE_ALIGN_PERM
#endif

MEMORY
    {
#if defined(Z_VM_KERNEL)
    ROM (rx)        : ORIGIN = PHYS_LOAD_ADDR, LENGTH = PHYS_RAM_AVAIL
#endif
    RAM (wx)        : ORIGIN = KERNEL_BASE_ADDR, LENGTH = KERNEL_RAM_SIZE

#if defined(CONFIG_DEMAND_PAGING) && !defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
    FLASH (rw)      : ORIGIN = FLASH_ROM_ADDR, LENGTH = FLASH_ROM_SIZE
#endif

    /*
     * On 32-bit x86, fake memory area for build-time IDT generation data.
     *
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 0xFFFF1000, LENGTH = 2K
    }

#if defined(Z_VM_KERNEL)
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

#if defined(CONFIG_DEMAND_PAGING) && !defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)

#undef SECTION_PROLOGUE
#define SECTION_PROLOGUE(name, options, align) \
        name options : ALIGN_WITH_INPUT align

#undef SECTION_DATA_PROLOGUE
#define SECTION_DATA_PROLOGUE(name, options, align) \
        name options : ALIGN_WITH_INPUT align

#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > vregion AT > lregion

#undef GROUP_DATA_LINK_IN
#define GROUP_DATA_LINK_IN(vregion, lregion) > vregion AT > lregion

#undef GROUP_NOLOAD_LINK_IN
#define GROUP_NOLOAD_LINK_IN(vregion, lregion) > vregion AT > lregion

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

/* For all source files under arch/x86/. */
#define LIB_ARCH_X86_IN_SECT(lsect)					\
	*libarch__x86__core.a.a:(.##lsect)				\
	*libarch__x86__core.a:(.##lsect##.*)

#ifdef CONFIG_MINIMAL_LIBC
/* For all source files under lib/libc/minimal/.
 * These files includes, for example, math and string functions.
 */
#define LIB_C_IN_SECT(lsect)						\
	*liblib__libc__minimal.a:(.##lsect)				\
	*liblib__libc__minimal.a:(.##lsect##.*)

#endif /* CONFIG_MINIMAL_LIBC */

#ifdef CONFIG_NEWLIB_LIBC
/* For Newlib libc-hook.c. */
#define LIB_C_IN_SECT(lsect)						\
	*liblib__libc__newlib.a:libc-hooks.c.obj(.##lsect)		\
	*liblib__libc__newlib.a:libc-hooks.c.obj(.##lsect##.*)

#endif /* CONFIG_NEWLIB_LIBC */

#ifdef CONFIG_PICOLIBC
/* For Picolibc libc-hook.c. */
#define LIB_C_IN_SECT(lsect)						\
	*liblib__libc__picolibc.a:libc-hooks.c.obj(.##lsect)		\
	*liblib__libc__picolibc.a:libc-hooks.c.obj(.##lsect##.*)

#endif /* CONFIG_PICOLIBC */

/*
 * For drivers that are usually used (e.g. serial)
 */
#define LIB_DRIVERS_IN_SECT(lsect)					\
	*libdrivers__serial.a:(.##lsect)				\
	*libdrivers__serial.a:(.##lsect##.*)				\
	*hpet.c.obj(.##lsect)						\
	*hpet.c.obj(.##lsect##.*)					\
	*intc_ioapic.c.obj(.##lsect)					\
	*intc_ioapic.c.obj(.##lsect##.*)				\
	*intc_loapic.c.obj(.##lsect)					\
	*intc_loapic.c.obj(.##lsect##.*)				\
	*intc_system_apic.c.obj(.##lsect)				\
	*intc_system_apic.c.obj(.##lsect##.*)				\
	*random_timer.c.obj(.##lsect)					\
	*random_timer.c.obj(.##lsect##.*)				\
	*uart_console.c.obj(.##lsect)					\
	*uart_console.c.obj(.##lsect##.*)

/* For all source files under kernel/. and kernel related files */
#define LIB_KERNEL_IN_SECT(lsect)					\
	*libkernel.a:(.##lsect)						\
	*libkernel.a:(.##lsect##.*)					\
	*libsubsys__demand_paging__*.a:(.##lsect)			\
	*libsubsys__demand_paging__*.a:(.##lsect##.*)

/* For particular file packaged in libzephyr.a. */
#define LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, objfile)			\
	*libzephyr.a:objfile.c.obj(.##lsect)				\
	*libzephyr.a:objfile.c.obj(.##lsect##.*)

/* For source files under lib/os/ with commonly used functions. */
#define LIB_ZEPHYR_IN_SECT(lsect)					\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, assert)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, bitarray)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, cbprintf_complete)	\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, cbprintf_nano)		\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, configs)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, coverage)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, heap)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, heap-validate)		\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, mutex)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, notify)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, printk)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, sem)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, stdout_console)		\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, sys_clock_init)		\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, rb)			\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, tracing_none)		\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, tracing_tracking)		\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, thread_entry)		\
	LIB_ZEPHYR_OBJECT_FILE_IN_SECT(lsect, work_q)

/*
 * Catch all for all internal/external kernel functions
 * as they are usually defined as "static inline" where
 * they are attached to the source which uses them.
 * Hence the need to specify them here so they can be pinned.
 */
#define ZEPHYR_KERNEL_FUNCS_IN_SECT					\
	*(.text.atomic_*)						\
	*(.text.k_*)							\
	*(.text.sys_*_bit)						\
	*(.text.sys_bitfield_*)						\
	*(.text.sys_clock_hw_cycles_per_sec)				\
	*(.text.sys_cache_*)						\
	*(.text.sys_dcache_*)						\
	*(.text.sys_icache_*)						\
	*(.text.sys_mutex_*)						\
	*(.text.sys_notify_*)						\
	*(.text.sys_dlist_*)						\
	*(.text.sys_slist_*)						\
	*(.text.sys_sflist_*)						\
	*(.text.sys_sfnode_*)						\
	*(.text.sys_io_*)						\
	*(.text.sys_in*)						\
	*(.text.sys_out*)						\
	*(.text.sys_read*)						\
	*(.text.sys_write*)						\
	*(.text.sys_get_be*)						\
	*(.text.sys_get_le*)						\
	*(.text.sys_put_be*)						\
	*(.text.sys_put_le*)						\
	*(.text.sys_mem_swap)						\
	*(.text.sys_memcpy_swap)					\
	*(.text.z_*)

/* For logging subsys */
#define LIB_SUBSYS_LOGGING_IN_SECT(lsect)				\
	*log_*.c.obj(.##lsect)						\
	*log_*.c.obj(.##lsect.*)					\
	*mpsc_pbuf.c.obj(.##lsect)					\
	*mpsc_pbuf.c.obj(.##lsect.*)

epoint = Z_MEM_PHYS_ADDR(CONFIG_KERNEL_ENTRY);
ENTRY(epoint)

/* SECTIONS definitions */
SECTIONS
	{

#include <zephyr/linker/rel-sections.ld>

    /DISCARD/ :
	{
	*(.plt)
	}

    /DISCARD/ :
	{
	*(.iplt)
	}

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

		*(.text.k_mem_paging_backing_store_init)
		*(.text.k_mem_paging_eviction_init)

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

		LIB_KERNEL_IN_SECT(text)
		LIB_ARCH_X86_IN_SECT(text)
		*(.text._OffsetAbsSyms)

		*(.pinned_text)
		*(.pinned_text.*)

		LIB_ZEPHYR_IN_SECT(text)
		LIB_C_IN_SECT(text)
		LIB_DRIVERS_IN_SECT(text)
		LIB_SUBSYS_LOGGING_IN_SECT(text)

		*_divdi3.o(.text)
		*_udivdi3.o(.text)
		*_udivmoddi4.o(.text)
		*_umoddi3.o(.text)
		*_popcountsi2.o(.text)

		*(.gnu.linkonce.t.exc_*)

		*(.text.*.constprop)
		*(.text.*.constprop.*)

#if defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_PICOLIBC)
		*libc*.a:(.text)
		*libc*.a:(.text.*)
#endif

#ifdef CONFIG_COVERAGE
		*(.text._sub_I_00100_0)
#endif

		ZEPHYR_KERNEL_FUNCS_IN_SECT

#include <zephyr/linker/kobject-text.ld>

		MMU_PAGE_ALIGN_PERM

		lnkr_pinned_text_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	MMU_PAGE_ALIGN_PERM

	lnkr_pinned_rodata_start = .;

#include <zephyr/linker/common-rom.ld>
#include <zephyr/linker/thread-local-storage.ld>

	SECTION_PROLOGUE(pinned.rodata,,)
	{
#include <zephyr/arch/x86/ia32/scripts/static_intr.ld>

		LIB_KERNEL_IN_SECT(rodata)
		LIB_ARCH_X86_IN_SECT(rodata)

		*(.pinned_rodata)
		*(.pinned_rodata.*)

		LIB_ZEPHYR_IN_SECT(rodata)
		LIB_C_IN_SECT(rodata)
		LIB_DRIVERS_IN_SECT(rodata)
		LIB_SUBSYS_LOGGING_IN_SECT(rodata)

		/* Static strings */
		*(.rodata.str*.*)
		*(.rodata.*.str*.*)

#if defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_PICOLIBC)
		*libc*.a:(.rodata)
		*libc*.a:(.rodata.*)
#endif

#include <snippets-rodata.ld>
#include <snippets-pinned-rodata.ld>

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

		LIB_KERNEL_IN_SECT(data)
		LIB_ARCH_X86_IN_SECT(data)

		*(.pinned_data)
		*(.pinned_data.*)

		LIB_ZEPHYR_IN_SECT(data)
		LIB_C_IN_SECT(data)
		LIB_DRIVERS_IN_SECT(data)
		LIB_SUBSYS_LOGGING_IN_SECT(data)

#ifdef CONFIG_COVERAGE
		*(.data.__gcov_.*)

		/*
		 * This is for the struct gcov_info for
		 * various functions.
		 */
		*(.data..LPBX*)
#endif

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/common-ram.ld>
#include <zephyr/arch/x86/pagetables.ld>
#include <zephyr/linker/kobject-data.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-pinned-data-sections.ld>

	lnkr_pinned_data_end = .;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-pinned-ram-sections.ld>

	SECTION_PROLOGUE(pinned.bss, (NOLOAD),)
	{
		. = ALIGN(4);

		lnkr_pinned_bss_start = .;

		LIB_KERNEL_IN_SECT(bss)
		LIB_ARCH_X86_IN_SECT(bss)

		*(.pinned_bss)
		*(.pinned_bss.*)

		LIB_ZEPHYR_IN_SECT(bss)
		LIB_C_IN_SECT(bss)
		LIB_DRIVERS_IN_SECT(bss)
		LIB_SUBSYS_LOGGING_IN_SECT(bss)

#ifdef CONFIG_COVERAGE
		*(.bss.__gcov0.*)
#endif

		lnkr_pinned_bss_end = .;
	} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_USERSPACE
	/* PINNED APP SHARED MEMORY REGION */
#include <app_smem_pinned.ld>

	_app_smem_pinned_size = _app_smem_pinned_end - _app_smem_pinned_start;
	_app_smem_pinned_num_words = _app_smem_pinned_size >> 2;
#endif /* CONFIG_USERSPACE */

	SECTION_PROLOGUE(pinned.noinit, (NOLOAD),)
	{
		. = ALIGN(4);

		lnkr_pinned_noinit_start = .;

		LIB_KERNEL_IN_SECT(noinit)
		LIB_ARCH_X86_IN_SECT(noinit)

		*(.pinned_noinit)
		*(.pinned_noinit.*)

		LIB_ZEPHYR_IN_SECT(noinit)
		LIB_C_IN_SECT(noinit)
		LIB_DRIVERS_IN_SECT(noinit)
		LIB_SUBSYS_LOGGING_IN_SECT(noinit)

#ifdef CONFIG_ZTEST
		/* For tests/kernel/mem_slab/ tests */
		*(.noinit.*.k_mem_slab_buf_*)

		/* For tests/kernel/mem_heap tests */
		*(.noinit.*.kheap_buf_*)

		/* Pin ztest_thread_stack.
		 * This must be done or else double fault
		 * may arise: testing exceptions while
		 * page fault for the ztest stack.
		 */
		*libsubsys__testsuite__ztest.a:(.noinit.*)
#endif

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

#if defined(CONFIG_DEMAND_PAGING) && !defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
/* From now on, put symbols into FLASH */
#undef ROMABLE_REGION
#define ROMABLE_REGION	FLASH

	/* This is to align the following sections in flash
	 * to their corresponding virtual addresses.
	 * In other words, the offset from start of flash
	 * for these sections would be the same from start of
	 * their virtual addresses. This provides a simple and
	 * direct mapping from backing store.
	 */
	flash_load_offset :
	{
		. = FLASH_ROM_ADDR + (lnkr_pinned_end - KERNEL_BASE_ADDR);
	} > FLASH AT > FLASH

#endif

	GROUP_START(ROMABLE_REGION)

	. = ALIGN(8);

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	__text_region_start = .;

#if !defined(CONFIG_LINKER_USE_BOOT_SECTION) || \
    !defined(CONFIG_LINKER_USE_PINNED_SECTION)
	z_mapped_start = .;
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

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#ifdef CONFIG_USERSPACE
	/* APP SHARED MEMORY REGION */
#include <app_smem.ld>

	_image_ram_start = _app_smem_start;
	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_num_words = _app_smem_size >> 2;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#endif /* CONFIG_USERSPACE */

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

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{

	__data_ram_start = .;

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
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__data_rom_start = LOADADDR(_DATA_SECTION_NAME);

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
	__data_ram_end = .;

	/* All unused memory also owned by the kernel for heaps */
	__kernel_ram_end = KERNEL_BASE_ADDR + KERNEL_RAM_SIZE;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

	_image_ram_all = (KERNEL_BASE_ADDR + KERNEL_RAM_SIZE) - _image_ram_start;

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

#define LAST_RAM_ALIGN MMU_PAGE_ALIGN

#include <zephyr/linker/ram-end.ld>

    GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

	/DISCARD/ : { *(.note.GNU-stack) }

	}
