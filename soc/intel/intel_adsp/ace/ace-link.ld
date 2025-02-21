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
#include <adsp-vectors.h>
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
#ifdef CONFIG_KERNEL_COHERENCE
#define RPO_SET(addr, reg) ((addr & 0x1fffffff) | (reg << 29))
#ifdef CONFIG_MMU
#define SEGSTART_CACHED   RPO_SET(ALIGN(CONFIG_MMU_PAGE_SIZE), CONFIG_XTENSA_CACHED_REGION)
#define SEGSTART_UNCACHED RPO_SET(ALIGN(CONFIG_MMU_PAGE_SIZE), CONFIG_XTENSA_UNCACHED_REGION)
#else
#define SEGSTART_CACHED   RPO_SET(ALIGN(64), CONFIG_XTENSA_CACHED_REGION)
#define SEGSTART_UNCACHED RPO_SET(ALIGN(64), CONFIG_XTENSA_UNCACHED_REGION)
#endif
#else
#ifdef CONFIG_MMU
#define SEGSTART_CACHED   ALIGN(CONFIG_MMU_PAGE_SIZE)
#define SEGSTART_UNCACHED ALIGN(CONFIG_MMU_PAGE_SIZE)
#else
#define SEGSTART_CACHED   .
#define SEGSTART_UNCACHED .
#endif
#define ucram ram
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
  vector_base_text :
	org = VECBASE_RESET_PADDR_SRAM,
        len = MEM_VECBASE_LIT_SIZE
  vector_int2_lit :
	org = INTLEVEL2_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int2_text :
        org = INTLEVEL2_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_int3_lit :
	org = INTLEVEL3_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int3_text :
	org = INTLEVEL3_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_int4_lit :
	org = INTLEVEL4_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int4_text :
	org = INTLEVEL4_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_int7_lit :
	org = INTLEVEL7_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int7_text :
	org = INTLEVEL7_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_kernel_lit :
	org = KERNEL_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_kernel_text :
	org = KERNEL_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_user_lit :
	org = USER_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_user_text :
	org = USER_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_double_lit :
	org = DOUBLEEXC_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_double_text :
	org = DOUBLEEXC_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
#ifdef CONFIG_XTENSA_MMU
  xtensa_vector_code :
	org = DOUBLEEXC_VECTOR_PADDR_SRAM + MEM_VECT_TEXT_SIZE,
	len = RAM_BASE - (DOUBLEEXC_VECTOR_PADDR_SRAM + MEM_VECT_TEXT_SIZE)
#endif
  imr :
        org = IMR_BOOT_LDR_TEXT_ENTRY_BASE,
        len = 0x100000
  ram :
	org = RAM_BASE,
	len = RAM_SIZE
#ifdef CONFIG_KERNEL_COHERENCE
  ucram :
	org = RPO_SET(RAM_BASE, CONFIG_XTENSA_UNCACHED_REGION),
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
    *(.imrdata .imrdata.*)
  } >imr

  /* Cold code in IMR memory */
  .cold : ALIGN(4096) {
    *(.cold .cold.* )
  } >imr

  /* Cold data. */
  .coldrodata : ALIGN(4096) {
    *(.coldrodata .coldrodata.*)
    _imr_end = .;
  } >imr

   .WindowVectors.text : {
    _WindowVectors_text_start = .;
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = .;
  } >vector_base_text
  .Level2InterruptVector.literal : {
    _Level2InterruptVector_literal_start = .;
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = .;
  } >vector_int2_lit
  .Level2InterruptVector.text : {
    _Level2InterruptVector_text_start = .;
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = .;
  } >vector_int2_text
  .Level3InterruptVector.literal : {
    _Level3InterruptVector_literal_start = .;
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = .;
  } >vector_int3_lit
  .Level3InterruptVector.text : {
    _Level3InterruptVector_text_start = .;
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = .;
  } >vector_int3_text
  .Level4InterruptVector.literal : {
    _Level4InterruptVector_literal_start = .;
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = .;
  } >vector_int4_lit
  .Level4InterruptVector.text : {
    _Level4InterruptVector_text_start = .;
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = .;
  } >vector_int4_text
  .DebugExceptionVector.literal : {
    _DebugExceptionVector_literal_start = .;
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = .;
  } >vector_int4_lit
  .DebugExceptionVector.text : {
    _DebugExceptionVector_text_start = .;
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = .;
  } >vector_int4_text
  .NMIExceptionVector.literal : {
    _NMIExceptionVector_literal_start = .;
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = .;
  } >vector_int7_lit
  .NMIExceptionVector.text : {
    _NMIExceptionVector_text_start = .;
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = .;
  } >vector_int7_text
  .KernelExceptionVector.literal : {
    _KernelExceptionVector_literal_start = .;
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = .;
  } >vector_kernel_lit
  .KernelExceptionVector.text : {
    _KernelExceptionVector_text_start = .;
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = .;
  } >vector_kernel_text
  .UserExceptionVector.literal : {
    _UserExceptionVector_literal_start = .;
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = .;
  } >vector_user_lit
  .UserExceptionVector.text : {
    _UserExceptionVector_text_start = .;
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = .;
  } >vector_user_text
  .DoubleExceptionVector.literal : {
    _DoubleExceptionVector_literal_start = .;
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = .;
  } >vector_double_lit
  .DoubleExceptionVector.text : {
    _DoubleExceptionVector_text_start = .;
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = .;
  } >vector_double_text

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

#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER
    *libarch__xtensa__core.a:syscall_helper.c.obj(.text.xtensa_syscall_helper*)
#endif /* CONFIG_XTENSA_SYSCALL_USE_HELPER */

  } > xtensa_vector_code
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
    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    MMU_PAGE_ALIGN
    *(.literal .text)
    *(.literal.* .text.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
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
    _bss_table_start = .;
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = .;
    MMU_PAGE_ALIGN
  } >ram

  .module_init : HDR_MMU_PAGE_ALIGN {
    _module_init_start = .;
    *(*.initcall)
    _module_init_end = .;
  } >ram

#define RAMABLE_REGION ram
#define ROMABLE_REGION ram

  __common_rom_region_start = SEGSTART_CACHED;
#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
   __rodata_region_end = .;
  __common_rom_region_end = .;
    __rodata_region_end = .;

  .noinit SEGSTART_UNCACHED : HDR_MMU_PAGE_ALIGN {
    __data_start = .;
    *(.noinit)
    *(.noinit.*)
  } >ucram

#include <zephyr/linker/kobject-priv-stacks.ld>
    . = SEGSTART_UNCACHED;

  .data SEGSTART_UNCACHED : HDR_MMU_PAGE_ALIGN {
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

  .lit4 SEGSTART_CACHED : {
    _lit4_start = .;
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = .;
  } >ram

/* These values need to change in our scheme, where the common-ram
 * sections need to be linked in safe/uncached memory but common-rom
 * wants to use the cache
 */
. = SEGSTART_UNCACHED;
#undef RAMABLE_REGION
#undef ROMABLE_REGION
#define RAMABLE_REGION ucram
#define ROMABLE_REGION ucram

  .fw_ready SEGSTART_UNCACHED : {
    KEEP(*(".fw_ready"));
    KEEP (*(.fw_ready_metadata))
  } > ucram

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

  .tm_clone_table : {
    *(.tm_clone_table)
  } >ram

  /* This section is cached.  By default it contains only declared
  * thread stacks, but applications can put symbols here too.
  */
  .cached SEGSTART_CACHED : {
     _cached_start = .;
    *(.cached .cached.*)
#ifdef CONFIG_USERSPACE
    z_user_stacks_start = .;
    *(.user_stacks*)
    z_user_stacks_end = .;
#endif
    _cached_end = .;
  } >ram

  /* Rimage requires 4k alignment between "DATA" and "BSS", can't do
   * this in the section declaration below because we're also changing
   * cacheability and that leaves a gap in the image large enough for
   * binutils to decide to warn about (no way to turn that off, it
   * seems, --warn-section-align is on by default)
   */
  . = ALIGN(4096);

  .bss SEGSTART_UNCACHED (NOLOAD) :
  {
    _bss_start = .;
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
    _bss_end = .;
  } >ucram

  . = SEGSTART_UNCACHED;
  _end = ALIGN(8);

  /* Heap start and end markers. Used to reserve system heap memory. */
  .heap_mem SEGSTART_UNCACHED (NOLOAD) :
  {
    _heap_start = .;
    *(.heap_mem)
  } >ucram

  .unused_ram_start_marker SEGSTART_CACHED (NOLOAD) :
  {
    . = ALIGN(4096);
    _unused_ram_start_marker = .;
    *(.unused_ram_start_marker)
    *(.unused_ram_start_marker.*)
    z_mapped_end = .;
  } >ram

  /* Heap start and end markers. Used with libc malloc code. */
  . = SEGSTART_UNCACHED;
  _end = ALIGN(8);
  . = SEGSTART_CACHED;
  z_mapped_end = .;
  . = L2_SRAM_BASE + L2_SRAM_SIZE;
  . = SEGSTART_UNCACHED;
  _heap_end = .;
  _heap_sentry = .;
  . = SEGSTART_CACHED;
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
  . = SEGSTART_CACHED;

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

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif
}
