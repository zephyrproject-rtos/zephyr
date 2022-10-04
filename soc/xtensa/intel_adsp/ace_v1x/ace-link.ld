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
#include <cavs-vectors.h>
#include <cavs-mem.h>

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
#define SEGSTART_CACHED   RPO_SET(ALIGN(64), CONFIG_XTENSA_CACHED_REGION)
#define SEGSTART_UNCACHED RPO_SET(ALIGN(64), CONFIG_XTENSA_UNCACHED_REGION)
#else
#define SEGSTART_CACHED   .
#define SEGSTART_UNCACHED .
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

MEMORY {
  vector_base_text :
	org = XCHAL_VECBASE_RESET_PADDR_SRAM,
        len = MEM_VECBASE_LIT_SIZE
  vector_int2_lit :
	org = XCHAL_INTLEVEL2_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int2_text :
        org = XCHAL_INTLEVEL2_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_int3_lit :
	org = XCHAL_INTLEVEL3_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int3_text :
	org = XCHAL_INTLEVEL3_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_int4_lit :
	org = XCHAL_INTLEVEL4_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int4_text :
	org = XCHAL_INTLEVEL4_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_int5_lit :
	org = XCHAL_INTLEVEL5_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int5_text :
	org = XCHAL_INTLEVEL5_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_int6_lit :
	org = XCHAL_INTLEVEL6_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int6_text :
	org = XCHAL_INTLEVEL6_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_int7_lit :
	org = XCHAL_INTLEVEL7_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_int7_text :
	org = XCHAL_INTLEVEL7_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_kernel_lit :
	org = XCHAL_KERNEL_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_kernel_text :
	org = XCHAL_KERNEL_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_user_lit :
	org = XCHAL_USER_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_user_text :
	org = XCHAL_USER_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
  vector_double_lit :
	org = XCHAL_DOUBLEEXC_VECTOR_PADDR_SRAM - MEM_VECT_LIT_SIZE,
        len = MEM_VECT_LIT_SIZE
  vector_double_text :
	org = XCHAL_DOUBLEEXC_VECTOR_PADDR_SRAM,
        len = MEM_VECT_TEXT_SIZE
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
  .Level5InterruptVector.literal : {
    _Level5InterruptVector_literal_start = .;
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = .;
  } >vector_int5_lit
  .Level5InterruptVector.text : {
    _Level5InterruptVector_text_start = .;
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = .;
  } >vector_int5_text
  .DebugExceptionVector.literal : {
    _DebugExceptionVector_literal_start = .;
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = .;
  } >vector_int6_lit
  .DebugExceptionVector.text : {
    _DebugExceptionVector_text_start = .;
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = .;
  } >vector_int6_text
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

  .text : {
    _text_start = .;
    *(.iram1 .iram1.*)
    *(.entry.text)
    *(.init.literal)
    *(.iram0.text)
    KEEP(*(.init))
    KEEP(*(.lps_vector))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    _text_end = .;
  } >ram

  .rodata : ALIGN(4096)
  {
    _rodata_start = .;
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)

    . = ALIGN(4);
    #include <snippets-rodata.ld>

    __XT_EXCEPTION_TABLE__ = .;
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table .gcc_except_table.*))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
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
    _rodata_end = .;
  } >ram

  .module_init : {
    _module_init_start = .;
    *(*.initcall)
    _module_init_end = .;
  } >ram

#define RAMABLE_REGION ram
#define ROMABLE_REGION ram
#include <zephyr/linker/common-rom.ld>

  .fw_ready : {
    KEEP(*(".fw_ready"));
    KEEP (*(.fw_ready_metadata))
  } >ram

  .noinit SEGSTART_UNCACHED : {
    *(.noinit)
    *(.noinit.*)
  } >ucram

  .data SEGSTART_UNCACHED : {
    _data_start = .;
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
    _data_end = .;
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

#include <zephyr/linker/common-ram.ld>

  .tm_clone_table : {
    *(.tm_clone_table)
  } >ram

  /* This section is cached.  By default it contains only declared
  * thread stacks, but applications can put symbols here too.
  */
  .cached SEGSTART_CACHED : {
     _cached_start = .;
    *(.cached .cached.*)
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
    _bss_end = .;
  } >ucram

  . = SEGSTART_UNCACHED;
  _end = ALIGN(8);

  /* Heap start and end markers. Used to reserve system heap memory. */
  .heap_mem SEGSTART_UNCACHED (NOLOAD) :
  {
    _heap_start = .;
    *(.heap_mem)
    _heap_end = .;
  } >ucram

  .unused_ram_start_marker SEGSTART_CACHED (NOLOAD) :
  {
    . = ALIGN(4096);
    _unused_ram_start_marker = .;
    *(.unused_ram_start_marker)
    *(.unused_ram_start_marker.*)
  } >ram

  . = L2_SRAM_BASE + L2_SRAM_SIZE;
  . = SEGSTART_UNCACHED;
  _heap_sentry = .;

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

#include <zephyr/linker/debug-sections.ld>

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
}
