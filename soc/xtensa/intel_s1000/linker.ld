/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the intel_s1000_crb platform
 */

OUTPUT_ARCH(xtensa)
#define _LINKER
#define _ASMLANGUAGE

#include <generated_dts_board.h>
#include "memory.h"
#include <autoconf.h>
#include <linker/sections.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define RAMABLE_REGION ram :ram_phdr
#define ROMABLE_REGION ram :ram_phdr
#define LPRAM_REGION lpram

MEMORY
{
  vector_reset_text :
        org = XCHAL_RESET_VECTOR0_PADDR_SRAM,
        len = MEM_RESET_TEXT_SIZE
  vector_reset_lit :
        org = XCHAL_RESET_VECTOR0_PADDR_SRAM + MEM_RESET_TEXT_SIZE,
        len = MEM_RESET_LIT_SIZE
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
  ram :
	org = RAM_BASE,
	len = RAM_SIZE
#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST :
	org = IDT_BASE,
	len = IDT_SIZE
#endif
  lpram :
	org = LPRAM_BASE,
	len = LPRAM_SIZE
}

PHDRS
{
  vector_reset_text_phdr PT_LOAD;
  vector_reset_lit_phdr PT_LOAD;
  vector_memory_lit_phdr PT_LOAD;
  vector_memory_text_phdr PT_LOAD;
  vector_base_text_phdr PT_LOAD;
  vector_int2_lit_phdr PT_LOAD;
  vector_int2_text_phdr PT_LOAD;
  vector_int3_lit_phdr PT_LOAD;
  vector_int3_text_phdr PT_LOAD;
  vector_int4_lit_phdr PT_LOAD;
  vector_int4_text_phdr PT_LOAD;
  vector_int5_lit_phdr PT_LOAD;
  vector_int5_text_phdr PT_LOAD;
  vector_int6_lit_phdr PT_LOAD;
  vector_int6_text_phdr PT_LOAD;
  vector_int7_lit_phdr PT_LOAD;
  vector_int7_text_phdr PT_LOAD;
  vector_kernel_lit_phdr PT_LOAD;
  vector_kernel_text_phdr PT_LOAD;
  vector_user_lit_phdr PT_LOAD;
  vector_user_text_phdr PT_LOAD;
  vector_double_lit_phdr PT_LOAD;
  vector_double_text_phdr PT_LOAD;
  ram_phdr PT_LOAD;
}
_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = XCHAL_VECBASE_RESET_PADDR_SRAM);

ENTRY(CONFIG_KERNEL_ENTRY)

/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x44024000;
_memmap_cacheattr_wt_base = 0x11021000;
_memmap_cacheattr_bp_base = 0x22022000;
_memmap_cacheattr_unused_mask = 0x00F00FFF;
_memmap_cacheattr_wb_trapnull = 0x4422422F;
_memmap_cacheattr_wba_trapnull = 0x4422422F;
_memmap_cacheattr_wbna_trapnull = 0x25222222;
_memmap_cacheattr_wt_trapnull = 0x1122122F;
_memmap_cacheattr_bp_trapnull = 0x2222222F;
_memmap_cacheattr_wb_strict = 0x44F24FFF;
_memmap_cacheattr_wt_strict = 0x11F21FFF;
_memmap_cacheattr_bp_strict = 0x22F22FFF;
_memmap_cacheattr_wb_allvalid = 0x44224222;
_memmap_cacheattr_wt_allvalid = 0x11221222;
_memmap_cacheattr_bp_allvalid = 0x22222222;

/*
 * Cache attributes for memory addresses:
 * (Each 4 bits correspond to 512MB of memory)
 *
 * 0x00000000 - 0x1FFFFFFF (bit  0 -  3) : Bypass cache
 *                                         (region for hardware registers)
 * 0x20000000 - 0x3FFFFFFF (bit  4 -  7) : No access
 * 0x40000000 - 0x5FFFFFFF (bit  8 - 11) : No access
 * 0x60000000 - 0x7FFFFFFF (bit 12 - 15) : No access
 * 0x80000000 - 0x9FFFFFFF (bit 16 - 19) : Bypass cache
 * 0xA0000000 - 0xBFFFFFFF (bit 20 - 23) : Cached, write-through
 * 0xC0000000 - 0xDFFFFFFF (bit 24 - 27) : Bypass cache
 * 0xE0000000 - 0xFFFFFFFF (bit 28 - 31) : Cached, write-through
 *
 * Note that this is both for instruction and data caches,
 * as cacheattr_set macro sets them both to the same set of
 * attributes.
 */
_memmap_cacheattr_intel_s1000 = 0x1212fff2;

PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_intel_s1000);
SECTIONS
{

#include <linker/rel-sections.ld>

  .ResetVector.text : ALIGN(4)
  {
    . = CONFIG_TEXT_SECTION_OFFSET;
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    _ResetVector_text_end = ABSOLUTE(.);
  } >vector_reset_text :vector_reset_text_phdr
  .ResetVector.literal : ALIGN(4)
  {
    _ResetVector_literal_start = ABSOLUTE(.);
    *(.ResetVector.literal)
    _ResetVector_literal_end = ABSOLUTE(.);
  } >vector_reset_lit :vector_reset_lit_phdr
 .MemoryExceptionVector.literal : ALIGN(4)
  {
    _MemoryExceptionVector_literal_start = ABSOLUTE(.);
    KEEP (*(.MemoryExceptionVector.literal))
    _MemoryExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_memory_lit :vector_memory_lit_phdr
  .MemoryExceptionVector.text : ALIGN(4)
  {
    _MemoryExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.MemoryExceptionVector.text))
    _MemoryExceptionVector_text_end = ABSOLUTE(.);
  } >vector_memory_text :vector_memory_text_phdr
   .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
  } >vector_base_text :vector_base_text_phdr
  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
  } >vector_int2_lit :vector_int2_lit_phdr
  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
  } >vector_int2_text :vector_int2_text_phdr
  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
  } >vector_int3_lit :vector_int3_lit_phdr
  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
  } >vector_int3_text :vector_int3_text_phdr
  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
  } >vector_int4_lit :vector_int4_lit_phdr
  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
  } >vector_int4_text :vector_int4_text_phdr
  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
  } >vector_int5_lit :vector_int5_lit_phdr
  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
  } >vector_int5_text :vector_int5_text_phdr
  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_int6_lit :vector_int6_lit_phdr
  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
  } >vector_int6_text :vector_int6_text_phdr
  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_int7_lit :vector_int7_lit_phdr
  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
  } >vector_int7_text :vector_int7_text_phdr
  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_kernel_lit :vector_kernel_lit_phdr
  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
  } >vector_kernel_text :vector_kernel_text_phdr
  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_user_lit :vector_user_lit_phdr
  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
  } >vector_user_text :vector_user_text_phdr
  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
  } >vector_double_lit :vector_double_lit_phdr
  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
  } >vector_double_text :vector_double_text_phdr

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    *(.iram0.text)
    KEEP(*(.init))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >ram :ram_phdr
#include <linker/common-rom.ld>

  .noinit :  ALIGN(4)
  {
    *(.noinit)
    *(.noinit.*)
  } >ram :ram_phdr
  .rodata : ALIGN(4)
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
  } >ram :ram_phdr
  .data : ALIGN(4)
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
  } >ram :ram_phdr
  .lit4 : ALIGN(4)
  {
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
  } >ram :ram_phdr
#include <linker/common-ram.ld>

  .bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
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
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
  } >ram :ram_phdr

  /* stack */
  _end = ALIGN(8);
  PROVIDE(end = ALIGN(8));
  __stack = DT_L2_SRAM_BASE + DT_L2_SRAM_SIZE;
  /* dma buffers */
  .lpbuf (NOLOAD): ALIGN(4)
  {
    _dma_buf_start = ABSOLUTE(.);
    *(.dma_buffers)
    _dma_buf_end = ABSOLUTE(.);
  } >LPRAM_REGION
  .comment  0 :  { *(.comment) }
  .debug 0 : { *(.debug) }
  .line 0 : { *(.line) }
  .debug_srcinfo 0 : { *(.debug_srcinfo) }
  .debug_sfnames 0 : { *(.debug_sfnames) }
  .debug_aranges 0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  .debug_info 0 : { *(.debug_info) }
  .debug_abbrev 0 : { *(.debug_abbrev) }
  .debug_line 0 : { *(.debug_line) }
  .debug_frame 0 : { *(.debug_frame) }
  .debug_str 0 : { *(.debug_str) }
  .debug_loc 0 : { *(.debug_loc) }
  .debug_macinfo 0 : { *(.debug_macinfo) }
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames 0 : { *(.debug_varnames) }
  .debug_ranges  0 :  { *(.debug_ranges) }
  .xtensa.info  0 :  { *(.xtensa.info) }
  .xt.insn 0 :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 :
  {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit 0 :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .xt.profile_range 0 :
  {
    KEEP (*(.xt.profile_range))
    KEEP (*(.gnu.linkonce.profile_range.*))
  }
  .xt.profile_ranges 0 :
  {
    KEEP (*(.xt.profile_ranges))
    KEEP (*(.gnu.linkonce.xt.profile_ranges.*))
  }
  .xt.profile_files 0 :
  {
    KEEP (*(.xt.profile_files))
    KEEP (*(.gnu.linkonce.xt.profile_files.*))
  }
#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif
}
