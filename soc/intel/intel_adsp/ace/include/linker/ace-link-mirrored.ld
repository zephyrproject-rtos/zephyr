/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the intel_apl_adsp platform
 */

OUTPUT_ARCH(xtensa)

#include <zephyr/devicetree.h>
#include <xtensa/config/core-isa.h>
#include <zephyr/linker/sections.h>
#include <adsp_memory.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

ENTRY(rom_entry);

/* DSP RAM regions (all of them) are mapped twice on the DSP.  One
 * mapping is set up to bypass the L1 cache, so it must be used when
 * multiprocessor coherence is desired, where the latter mapping is
 * best used for processor-local data (e.g. stacks) or shared data
 * that is managed with explicit cache flush/invalidate operations.
 *
 * These macros will set up a segment start address correctly,
 * including alignment to a cache line.  Be sure to also emit the
 * section to ">ram" or ">ucram" as appropriate, to prevent the linker
 * from filling in 512MB of sparse zeros.
 */
#define RPO_SET(addr, reg) ((addr & 0x1fffffff) | (reg << 29))

#ifdef CONFIG_KERNEL_COHERENCE
#ifdef CONFIG_MMU
#define SEGSTART_CACHED   RPO_SET(ALIGN(phys_dot, CONFIG_MMU_PAGE_SIZE), CONFIG_INTEL_ADSP_CACHED_REGION)
#define SEGSTART_UNCACHED RPO_SET(ALIGN(phys_dot, CONFIG_MMU_PAGE_SIZE), CONFIG_INTEL_ADSP_UNCACHED_REGION)
#else
#define SEGSTART_CACHED   RPO_SET(ALIGN(phys_dot, 64), CONFIG_INTEL_ADSP_CACHED_REGION)
#define SEGSTART_UNCACHED RPO_SET(ALIGN(phys_dot, 64), CONFIG_INTEL_ADSP_UNCACHED_REGION)
#endif
#else
#ifdef CONFIG_MMU
#define SEGSTART_CACHED   ALIGN(phys_dot, CONFIG_MMU_PAGE_SIZE)
#define SEGSTART_UNCACHED ALIGN(phys_dot, CONFIG_MMU_PAGE_SIZE)
#else
#define SEGSTART_CACHED   phys_dot
#define SEGSTART_UNCACHED phys_dot
#endif
#define ucram ram
#endif

#ifdef CONFIG_KERNEL_COHERENCE
#ifdef CONFIG_MMU
#define ADDR_CACHED(sym)   RPO_SET(ALIGN(sym, CONFIG_MMU_PAGE_SIZE), CONFIG_INTEL_ADSP_CACHED_REGION)
#define ADDR_UNCACHED(sym) RPO_SET(ALIGN(sym, CONFIG_MMU_PAGE_SIZE), CONFIG_INTEL_ADSP_UNCACHED_REGION)
#else
#define ADDR_CACHED(sym)   RPO_SET(ALIGN(sym, 64), CONFIG_INTEL_ADSP_CACHED_REGION)
#define ADDR_UNCACHED(sym) RPO_SET(ALIGN(sym, 64), CONFIG_INTEL_ADSP_UNCACHED_REGION)
#endif
#else
#ifdef CONFIG_MMU
#define ADDR_CACHED(sym)   ALIGN(sym, CONFIG_MMU_PAGE_SIZE)
#define ADDR_UNCACHED(sym) ALIGN(sym, CONFIG_MMU_PAGE_SIZE)
#else
#define ADDR_CACHED(sym)   sym
#define ADDR_UNCACHED(sym) sym
#endif
#endif

/* intlist.ld needs an IDT_LIST memory region */
#define IDT_BASE 0xe0000000
#define IDT_SIZE 0x2000

/* rimage module sections are C struct data, and thus flagged ALLOC.
 * The xcc linker demands they be in a declared memory region even if
 * the enclosing output section is (NOLOAD).  Put them here.
 */
#define NOLOAD_BASE 0x20000
#define NOLOAD_SIZE 0x100000

#ifdef CONFIG_MMU
#define MMU_PAGE_ALIGN . = ALIGN(CONFIG_MMU_PAGE_SIZE);
#define HDR_MMU_PAGE_ALIGN ALIGN(CONFIG_MMU_PAGE_SIZE)
#define HDR_4K_OR_MMU_PAGE_ALIGN ALIGN(CONFIG_MMU_PAGE_SIZE)
#else
#define MMU_PAGE_ALIGN
#define HDR_MMU_PAGE_ALIGN
#define HDR_4K_OR_MMU_PAGE_ALIGN ALIGN(4096)
#endif

#define SMEM_PARTITION_ALIGN(size) MMU_PAGE_ALIGN

MEMORY {
  vectors :
	org = VECBASE_RESET_PADDR_SRAM,
	len = VECTOR_TBL_SIZE
  imr :
	org = IMR_BOOT_LDR_TEXT_ENTRY_BASE,
	len = 0x100000
  ram :
	org = RAM_BASE,
	len = RAM_SIZE
#ifdef CONFIG_KERNEL_COHERENCE
  ucram :
	org = RPO_SET(RAM_BASE, CONFIG_INTEL_ADSP_UNCACHED_REGION),
	len = RAM_SIZE
#endif
#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST :
	org = IDT_BASE,
	len = IDT_SIZE
#endif
  lpram :
	org = LP_SRAM_BASE,
	len = LP_SRAM_SIZE
  noload :
	org = NOLOAD_BASE,
	len = NOLOAD_SIZE
}

SECTIONS {
  phys_dot = 0;

  /* Boot loader code in IMR memory */
  .imr : {
    _imr_start = .;
    /* Entry point MUST be here per external configuration */
    KEEP (*(.boot_entry.text))
    *(.imr .imr.*)
  } >imr

  /* Boot loader data.  Note that rimage seems to want this
   * page-aligned or it will throw an error, not sure why since all
   * the ROM cares about is a contiguous region.  And it's
   * particularly infuriating as it precludes linker .rodata next to
   * .text.
   */
  .imrdata : ALIGN(4096) {
    __imr_data_start = .;
    *(.imrdata .imrdata.*)
    __imr_data_end = .;
  } >imr

  /* Cold code in IMR memory */
  .cold : ALIGN(4096) {
    __cold_start = .;
    *(.cold .cold.* )
    __cold_end = .;
  } >imr

  /* Cold data. */
  .coldrodata : ALIGN(4096) {
    __coldrodata_start = .;
    *(.coldrodata .coldrodata.*)
    _imr_end = .;
  } >imr

/* Auto-generated vector linkage, to "vectors" memory region */
#include <xtensa_vectors.ld>
 >vectors

#ifdef CONFIG_XTENSA_MMU
  .xtensa_vector_code : {
    /* There is not much space between .DoubleExceptionVector
     * and the beginning of .text. So need to be careful on
     * what to put in this section. It would mostly be things
     * that are required to handle exceptions and interrupts.
     * Also things we want to avoid TLB misses during exception
     * handling.
     */

    *libarch__xtensa__core.a:xtensa_asm2_util.S.obj(.literal)
    *libarch__xtensa__core.a:xtensa_asm2_util.S.obj(.iram.text)
    *libarch__xtensa__core.a:xtensa_asm2_util.S.obj(.text)

    *libarch__xtensa__core.a:window_vectors.S.obj(.iram.text)

    /* For _Level1Vector */
    *libarch__xtensa__core.a:vector_handlers_util.S.obj(.iram0.text)

    *libarch__xtensa__core.a:vector_handlers.c.obj(.literal.xtensa_int?_c)
    *libarch__xtensa__core.a:vector_handlers.c.obj(.text.xtensa_int?_c)

    *libarch__xtensa__core.a:userspace.S.obj(.literal.xtensa_do_syscall)
    *libarch__xtensa__core.a:userspace.S.obj(.text.xtensa_do_syscall)
    *libarch__xtensa__core.a:ptables.c.obj(.literal.xtensa_swap_update_page_tables)
    *libarch__xtensa__core.a:ptables.c.obj(.text.xtensa_swap_update_page_tables)

#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER
    *libarch__xtensa__core.a:syscall_helper.c.obj(.text.xtensa_syscall_helper*)
#endif /* CONFIG_XTENSA_SYSCALL_USE_HELPER */

  } > vectors
#endif /* CONFIG_XTENSA_MMU */

  .text : HDR_MMU_PAGE_ALIGN
  {
    __text_region_start = .;
    _image_ram_start = .;
    z_mapped_start = .;
    *(.iram1 .iram1.*)
    *(.entry.text)
    *(.init.literal)
    *(.iram0.text)

#ifdef CONFIG_XTENSA_MMU
    /*
     * The C handle blows up in size significantly when logging is enabled.
     * So we can't keep this in .xtensa_vector_code above. Though we try to
     * leep these close enough that, hopefully, we can avoid TLB misses.
     */
    *libarch__xtensa__core.a:vector_handlers.c.obj(.literal.xtensa_excint1_c)
    *libarch__xtensa__core.a:vector_handlers.c.obj(.text.xtensa_excint1_c)

    /*
     * Experiment shows that system would not boot if this is away from
     * VECBASE more than a few pages. So workaround the issue by putting
     * xtensa_mmu_init() here.
     */
    *libarch__xtensa__core.a:ptables.c.obj(.literal.xtensa_mmu_init)
    *libarch__xtensa__core.a:ptables.c.obj(.text.xtensa_mmu_init)
#endif

    KEEP(*(.init))
    KEEP(*(.lps_vector))
    *(.gnu.linkonce.literal.* .gnu.linkonce.t.*.literal)
    MMU_PAGE_ALIGN
    *(.literal)
    *(.literal.*)
    *(.text)
    *(.text.* .stub .gnu.warning .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)

#include <snippets-text-sections.ld>

#include <zephyr/linker/kobject-text.ld>
    MMU_PAGE_ALIGN
    __text_region_end = .;
  } >ram

  .rodata : HDR_4K_OR_MMU_PAGE_ALIGN
  {
    __rodata_region_start = .;
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)

    . = ALIGN(4);
    #include <snippets-rodata.ld>
    #include <zephyr/linker/kobject-rom.ld>

    __XT_EXCEPTION_TABLE__ = .;
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table .gcc_except_table.*))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    __XT_EXCEPTION_DESCS__ = .;
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = .;
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    MMU_PAGE_ALIGN
  } >ram

  .module_init : HDR_MMU_PAGE_ALIGN {
    _module_init_start = .;
    *(*.initcall)
    _module_init_end = .;
  } >ram
  . = _module_init_end;
  phys_dot = . & 0x1fffffff;

#define RAMABLE_REGION ram
#define ROMABLE_REGION ram

  __common_rom_region_start = ADDR_CACHED(_module_init_end);
#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
   __rodata_region_end = .;
  __common_rom_region_end = .;
    __rodata_region_end = .;
  . = __common_rom_region_end;
  phys_dot = . & 0x1fffffff;

  .noinit ADDR_UNCACHED(__common_rom_region_end) (NOLOAD) : HDR_MMU_PAGE_ALIGN {
    __data_start = .;
    *(.noinit)
    *(.noinit.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() CMake function.
 */
#include <snippets-noinit.ld>

    __noinit_end = .;
  } >ucram
  . = __noinit_end;
  phys_dot = . & 0x1fffffff;

#include <zephyr/linker/kobject-priv-stacks.ld>
    . = ADDR_UNCACHED(__noinit_end);

  .data ADDR_UNCACHED(__noinit_end) : HDR_MMU_PAGE_ALIGN {
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    KEEP(*(.gnu.linkonce.d.*personality*))
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    KEEP(*(.jcr))
    _trace_ctx_start = ABSOLUTE(.);
    *(.trace_ctx)
    _trace_ctx_end = ABSOLUTE(.);
    *(.gna_model)
    __data_end = .;
    MMU_PAGE_ALIGN
  } >ucram
  . = __data_end;
  phys_dot = . & 0x1fffffff;

  .lit4 ADDR_CACHED(__data_end) : {
    _lit4_start = .;
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = .;
  } >ram
  . = _lit4_end;
  phys_dot = . & 0x1fffffff;

/* These values need to change in our scheme, where the common-ram
 * sections need to be linked in safe/uncached memory but common-rom
 * wants to use the cache
 */
. = ADDR_UNCACHED(_lit4_end);
#undef RAMABLE_REGION
#undef ROMABLE_REGION
#define RAMABLE_REGION ucram
#define ROMABLE_REGION ucram

  .fw_ready ADDR_UNCACHED(_lit4_end) : {
    KEEP(*(".fw_ready"));
    KEEP (*(.fw_ready_metadata))
    __fw_ready_end = .;
  } > ucram
  . = __fw_ready_end;
  phys_dot = . & 0x1fffffff;

  __common_ram_region_start = .;

#ifdef CONFIG_USERSPACE

#define APP_SHARED_ALIGN MMU_PAGE_ALIGN
#define EMPTY_APP_SHARED_ALIGN APP_SHARED_ALIGN

	/* APP SHARED MEMORY REGION */
#include <app_smem.ld>
	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_num_words = _app_smem_size >> 2;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#endif /* CONFIG_USERSPACE */

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>
  __common_ram_region_end = .;
  . = __common_ram_region_end;
  phys_dot = . & 0x1fffffff;

  .tm_clone_table ADDR_CACHED(__common_ram_region_end) : {
    *(.tm_clone_table)
    __tm_clone_table_end = .;
  } >ram
  . = __tm_clone_table_end;
  phys_dot = . & 0x1fffffff;

  /* This section is cached.  By default it contains only declared
  * thread stacks, but applications can put symbols here too.
  */
  .cached ADDR_CACHED(__tm_clone_table_end) : {
     _cached_start = .;
    *(.cached .cached.*)
#ifdef CONFIG_USERSPACE
    z_user_stacks_start = .;
    *(.user_stacks*)
    z_user_stacks_end = .;
#endif
    _cached_end = .;
  } >ram
  . = _cached_end;
  phys_dot = . & 0x1fffffff;

  /* Rimage requires 4k alignment between "DATA" and "BSS", can't do
   * this in the section declaration below because we're also changing
   * cacheability and that leaves a gap in the image large enough for
   * binutils to decide to warn about (no way to turn that off, it
   * seems, --warn-section-align is on by default)
   */
  . = ALIGN(4096);
  _bss_start_phys = .;

  .bss ADDR_UNCACHED(_bss_start_phys) (NOLOAD) :
  {
    __bss_start = .;

#if defined(CONFIG_XTENSA_MMU)
    /* Explicitly place these here and together instead of letting linker
     * decide where to put these. This keeps a large piece of memory
     * allocated continuously to avoid fragmenting the virtual address
     * space.
     */
    *:ptables.c.obj(.bss.l1_page_tables)
    *:ptables.c.obj(.bss.l2_page_tables)
    *:ptables.c.obj(.bss.l1_page_tables_track .bss.l2_page_tables_counter)
#endif /* CONFIG_XTENSA_MMU */

#if defined(CONFIG_ZTEST) && defined(CONFIG_SIMULATOR_XTENSA)
    /* For some weird unknown reasons, the simulator really do not
     * like these cpuhold_* variables to be tightly packed together.
     * This results in cpuhold_spawned not being updated, and we
     * will be stuck in the while loop for it to be set.
     * Workaround by explicitly aligning these variables.
     */
    . = ALIGN(16);
    *:ztest.c.obj(.bss.cpuhold_sem)
    . = ALIGN(16);
    *:ztest.c.obj(.bss.cpuhold_active)
    . = ALIGN(16);
    *:ztest.c.obj(.bss.cpuhold_spawned)
    . = ALIGN(16);
    *:ztest.c.obj(.bss.cpuhold_pool_items)
#endif /* CONFIG_ZTEST && CONFIG_SIMULATOR_XTENSA */

    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(8);
    MMU_PAGE_ALIGN
    __bss_end = .;
  } >ucram
  . = __bss_end;
  phys_dot = . & 0x1fffffff;

  . = ADDR_UNCACHED(__bss_end);
  _end = ALIGN(8);

  /* Heap start and end markers. Used to reserve system heap memory. */
  .heap_mem ADDR_UNCACHED(__bss_end) (NOLOAD) :
  {
    _shared_heap_start = .;
    *(.shared_heap_mem)
    _shared_heap_end = .;
    _heap_start = .;
    *(.heap_mem)
    _heap_end = .;
  } >ucram
  . = _heap_end;
  phys_dot = . & 0x1fffffff;

  .unused_ram_start_marker ADDR_CACHED(_heap_end) (NOLOAD) :
  {
    . = ALIGN(4096);
    _unused_ram_start_marker = .;
    *(.unused_ram_start_marker)
    *(.unused_ram_start_marker.*)
    z_mapped_end = .;
  } >ram
  . = z_mapped_end;
  phys_dot = . & 0x1fffffff;

  /* Heap start and end markers. Used with libc malloc code. */
  . = ADDR_UNCACHED(z_mapped_end);
  _end = ALIGN(8);
  . = ADDR_CACHED(z_mapped_end);
  z_mapped_end = .;
  . = L2_SRAM_BASE + L2_SRAM_SIZE;
  . = ADDR_UNCACHED(z_mapped_end);
  _heap_end = .;
  _heap_sentry = .;
  . = ADDR_CACHED(z_mapped_end);
  _image_ram_end = .;

  /* dma buffers */
  .lpbuf (NOLOAD): {
    _dma_buf_start = .;
    *(.dma_buffers)
    _dma_buf_end = .;
  } >lpram

  /* Non-loadable sections below.  Back to cached memory so
   * the cache remap script doesn't try to move them around needlessly.
   */
  . = ADDR_CACHED(z_mapped_end);

  /* rimage module manifest headers */
  .module.boot : { KEEP(*(.module.boot)) } >noload
  .module.cold : { KEEP(*(.module.cold)) } >noload
  .module.main : { KEEP(*(.module.main)) } >noload

 .static_uuid_entries : {
    *(*.static_uuids)
  } >noload

  .static_log_entries : {
    *(*.static_log*)
  } >noload

  /* This is the "extended manifest" data (mostly versioning stuff)
   * emitted by SOF and inspected by the kernel driver.  It doesn't
   * appear directly in the image, but rimage will parse and repack
   * this into the output file header, so requires this be present
   * even if empty.  Alignment and padding to 16 bytes is required,
   * otherwise rimage will complain about the size being wrong (which
   * sounds like a struct should be declared packed somewhere...)
   */
  .fw_metadata : ALIGN(16) {
    KEEP (*(.fw_metadata))
    . = ALIGN(16);
  } >noload

#include <snippets-sections.ld>

#include <zephyr/linker/debug-sections.ld>

   /DISCARD/ : { *(.note.GNU-stack) }

  .xtensa.info 0 : { *(.xtensa.info) }
  .xt.insn 0 : {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 : {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit 0 : {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .xt.profile_range 0 : {
    KEEP (*(.xt.profile_range))
    KEEP (*(.gnu.linkonce.profile_range.*))
  }
  .xt.profile_ranges 0 : {
    KEEP (*(.xt.profile_ranges))
    KEEP (*(.gnu.linkonce.xt.profile_ranges.*))
  }
  .xt.profile_files 0 : {
    KEEP (*(.xt.profile_files))
    KEEP (*(.gnu.linkonce.xt.profile_files.*))
  }

#ifdef CONFIG_GEN_ISR_TABLES
#include <zephyr/linker/intlist.ld>
#endif
#include <zephyr/linker/llext-sections.ld>

#define MEM_MAP_SYM_PROVIDE(sym)                                                                   \
	PROVIDE(sym##_c = RPO_SET(sym, CONFIG_INTEL_ADSP_CACHED_REGION));                          \
	PROVIDE(sym##_uc = RPO_SET(sym, CONFIG_INTEL_ADSP_UNCACHED_REGION));

	MEM_MAP_SYM_PROVIDE(_cached_start)
	MEM_MAP_SYM_PROVIDE(_cached_end)
	MEM_MAP_SYM_PROVIDE(__cold_start)
	MEM_MAP_SYM_PROVIDE(__cold_end)
	MEM_MAP_SYM_PROVIDE(__coldrodata_start)
	MEM_MAP_SYM_PROVIDE(_heap_start)
	MEM_MAP_SYM_PROVIDE(_heap_end)
	MEM_MAP_SYM_PROVIDE(_image_ram_start)
	MEM_MAP_SYM_PROVIDE(_image_ram_end)
	MEM_MAP_SYM_PROVIDE(__imr_data_start)
	MEM_MAP_SYM_PROVIDE(__imr_data_end)
	MEM_MAP_SYM_PROVIDE(_imr_end)
	MEM_MAP_SYM_PROVIDE(__common_ram_region_start)
	MEM_MAP_SYM_PROVIDE(__common_ram_region_end)
	MEM_MAP_SYM_PROVIDE(__rodata_region_start)
	MEM_MAP_SYM_PROVIDE(__rodata_region_end)
	MEM_MAP_SYM_PROVIDE(__text_region_start)
	MEM_MAP_SYM_PROVIDE(__text_region_end)
	MEM_MAP_SYM_PROVIDE(_shared_heap_start)
	MEM_MAP_SYM_PROVIDE(_shared_heap_end)
}
