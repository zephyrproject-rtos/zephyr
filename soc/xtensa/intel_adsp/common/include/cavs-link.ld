/*
 * Copyright (c) 2019 Intel Corporation
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

#include <devicetree.h>
#include <xtensa/config/core-isa.h>
#include <autoconf.h>
#include <linker/sections.h>
#include <cavs-vectors.h>
#include <cavs-mem.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

ENTRY(rom_entry);

/* DSP RAM regions (all of them) are mapped twice on the DSP: once in
 * a 512MB region from 0x80000000-0x9fffffff and again from
 * 0xa0000000-0xbfffffff.  The first mapping is set up to bypass the
 * L1 cache, so it must be used when multiprocessor coherence is
 * desired, where the latter mapping is best used for processor-local
 * data (e.g. stacks) or shared data that is managed with explicit
 * cache flush/invalidate operations.
 *
 * These macros will set up a segment start address correctly,
 * including alignment to a cache line.  Be sure to also emit the
 * section to ">ram :ram_phdr" or ">ucram :ucram_phdr" as
 * appropriate. (Forgetting the correct PHDR will actually work, as
 * the output tooling ignores it, but it will cause the linker to emit
 * 512MB of unused data into the output file!)
 *
 * (Note clumsy syntax because XCC doesn't understand the "~" operator)
 */
#ifdef CONFIG_KERNEL_COHERENCE
#define SEGSTART_CACHED   (ALIGN(64) | 0x20000000)
#define SEGSTART_UNCACHED (ALIGN(64) & 0xdfffffff) /* == ~0x20000000 */
#else
#define SEGSTART_CACHED   /**/
#define SEGSTART_UNCACHED /**/
#define ucram ram
#define ucram_phdr ram_phdr
#endif

/* intlist.ld needs an IDT_LIST memory region */
#define IDT_BASE 0xe0000000
#define IDT_SIZE 0x2000

MEMORY
{
  vector_memory_lit :
	org = XCHAL_MEMERROR_VECTOR_PADDR + MEM_ERROR_LIT_SIZE,
        len = MEM_ERROR_LIT_SIZE
  vector_memory_text :
	org = XCHAL_MEMERROR_VECTOR_PADDR,
        len = MEM_ERROR_TEXT_SIZE
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
	org = RAM_BASE - 0x20000000,
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
}

_EXT_MAN_ALIGN_ = 16;

SECTIONS
{
  /* Boot loader code in IMR memory */
  .imr : {
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

 .MemoryExceptionVector.literal : ALIGN(4)
  {
    _MemoryExceptionVector_literal_start = ABSOLUTE(.);
    KEEP (*(.MemoryExceptionVector.literal))
    _MemoryExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_memory_lit
  .MemoryExceptionVector.text : ALIGN(4)
  {
    _MemoryExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.MemoryExceptionVector.text))
    _MemoryExceptionVector_text_end = ABSOLUTE(.);
  } >vector_memory_text
   .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
  } >vector_base_text
  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
  } >vector_int2_lit
  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
  } >vector_int2_text
  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
  } >vector_int3_lit
  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
  } >vector_int3_text
  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
  } >vector_int4_lit
  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
  } >vector_int4_text
  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
  } >vector_int5_lit
  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
  } >vector_int5_text
  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_int6_lit
  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
  } >vector_int6_text
  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_int7_lit
  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
  } >vector_int7_text
  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_kernel_lit
  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
  } >vector_kernel_text
  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_user_lit
  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
  } >vector_user_text
  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_double_lit
  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
  } >vector_double_text

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    *(.iram0.text)
    KEEP(*(.init))
    KEEP(*(.lps_vector))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >ram

  .rodata : ALIGN(4096)
  {
    _rodata_start = ABSOLUTE(.);
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
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
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);
    _rodata_end = ABSOLUTE(.);
  } >ram

  .module_init : ALIGN(4)
  {
    _module_init_start = ABSOLUTE(.);
    *(*.module_init)
    _module_init_end = ABSOLUTE(.);
  } >ram

#define RAMABLE_REGION ram
#define ROMABLE_REGION ram
#include <linker/common-rom.ld>

  .fw_ready : ALIGN(4)
  {
    KEEP(*(".fw_ready"));
    KEEP (*(.fw_ready_metadata))
  } >ram

  .noinit SEGSTART_UNCACHED : ALIGN(4)
  {
    *(.noinit)
    *(.noinit.*)
  } >ucram

  .data SEGSTART_UNCACHED : ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
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
    . = ALIGN(4096);
    *(.gna_model)
    _data_end = ABSOLUTE(.);
    . = ALIGN(4096);
  } >ucram

  .lit4 SEGSTART_CACHED : ALIGN(4)
  {
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
  } >ram

#ifdef CONFIG_KERNEL_COHERENCE
/* These values need to change in our scheme, where the common-ram
 * sections need to be linked in safe/uncached memory but common-rom
 * wants to use the cache
 */
  . = SEGSTART_UNCACHED;
#endif

#undef RAMABLE_REGION
#undef ROMABLE_REGION
#define RAMABLE_REGION ucram
#define ROMABLE_REGION ucram

#include <linker/common-ram.ld>

  .tm_clone_table :
  {
    *(.tm_clone_table)
  } >ram

  /* This section is cached.  By default it contains only declared
  * thread stacks, but applications can put symbols here too.
  */
  .cached SEGSTART_CACHED :
  {
    *(.cached .cached.*)
  } >ram

  . = ALIGN(4096);

  .bss SEGSTART_UNCACHED (NOLOAD) : ALIGN(4096)
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

#ifdef CONFIG_KERNEL_COHERENCE
  . = SEGSTART_UNCACHED;
#endif
  _end = ALIGN(8);
  PROVIDE(end = ALIGN(8));

  /* Re-adjust to the upper mapping for the final symbols below */
#ifdef CONFIG_KERNEL_COHERENCE
  . = SEGSTART_CACHED;
#endif
  __stack = L2_SRAM_BASE + L2_SRAM_SIZE;

#ifdef CONFIG_KERNEL_COHERENCE
  . = SEGSTART_UNCACHED;
#endif

  /* dma buffers */
  .lpbuf (NOLOAD): ALIGN(4)
  {
    _dma_buf_start = ABSOLUTE(.);
    *(.dma_buffers)
    _dma_buf_end = ABSOLUTE(.);
  } >lpram

  . = L2_SRAM_BASE + L2_SRAM_SIZE;
#ifdef CONFIG_KERNEL_COHERENCE
  . = SEGSTART_UNCACHED;
#endif
  _heap_sentry = .;


  /* Non-loadable sections below.  Back to cached memory so
   * the cache remap script doesn't try to move them around needlessly.
   */
#ifdef CONFIG_KERNEL_COHERENCE
  . = SEGSTART_CACHED;
#endif

  /* rimage module manifest headers */
  .module.boot : { KEEP(*(.module.boot)) }
  .module.main : { KEEP(*(.module.main)) }

  .comment : { *(.comment) }
  .debug : { *(.debug) }
  .line : { *(.line) }
  .debug_srcinfo : { *(.debug_srcinfo) }
  .debug_sfnames : { *(.debug_sfnames) }
  .debug_aranges : { *(.debug_aranges) }
  .debug_pubnames : { *(.debug_pubnames) }
  .debug_info : { *(.debug_info) }
  .debug_abbrev : { *(.debug_abbrev) }
  .debug_line : { *(.debug_line) }
  .debug_frame : { *(.debug_frame) }
  .debug_str : { *(.debug_str) }
  .debug_loc : { *(.debug_loc) }
  .debug_macinfo : { *(.debug_macinfo) }
  .debug_weaknames : { *(.debug_weaknames) }
  .debug_funcnames : { *(.debug_funcnames) }
  .debug_typenames : { *(.debug_typenames) }
  .debug_varnames : { *(.debug_varnames) }
  .debug_ranges : { *(.debug_ranges) }
  .xtensa.info : { *(.xtensa.info) }
  .xt.insn :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop :
  {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .xt.profile_range :
  {
    KEEP (*(.xt.profile_range))
    KEEP (*(.gnu.linkonce.profile_range.*))
  }
  .xt.profile_ranges :
  {
    KEEP (*(.xt.profile_ranges))
    KEEP (*(.gnu.linkonce.xt.profile_ranges.*))
  }
  .xt.profile_files :
  {
    KEEP (*(.xt.profile_files))
    KEEP (*(.gnu.linkonce.xt.profile_files.*))
  }

 .static_uuid_entries : {
    *(*.static_uuids)
  }

  .static_log_entries : {
    *(*.static_log*)
  }

  /* This is the "extended manifest" data (mostly versioning stuff)
   * emitted by SOF and inspected by the kernel driver.  It doesn't
   * appear directly in the image, but rimage will parse and repack
   * this into the output file header, so requires this be present
   * even if empty.  Assignment to "." forces this section to be
   * created.  Alignment and padding to 16 bytes is required,
   * otherwise rimage will complain about the size being wrong (which
   * sounds like a struct should be declared packed somewhere...)
   */
  .fw_metadata : ALIGN(16) {
    . = .;
    KEEP (*(.fw_metadata))
    . = ALIGN(16);
  }

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif
}
