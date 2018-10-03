/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Xtensa platform.
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define RAMABLE_REGION sram20_seg :sram20_phdr
#define ROMABLE_REGION srom1_seg :srom1_phdr

MEMORY
{
  sram1_seg :                         	org = 0x00002000, len = 0x178
  sram2_seg :                         	org = 0x00002178, len = 0x8
  sram3_seg :                         	org = 0x00002180, len = 0x38
  sram4_seg :                         	org = 0x000021B8, len = 0x8
  sram5_seg :                         	org = 0x000021C0, len = 0x38
  sram6_seg :                         	org = 0x000021F8, len = 0x8
  sram7_seg :                         	org = 0x00002200, len = 0x38
  sram8_seg :                         	org = 0x00002238, len = 0x8
  sram9_seg :                         	org = 0x00002240, len = 0x38
  sram10_seg :                        	org = 0x00002278, len = 0x8
  sram11_seg :                        	org = 0x00002280, len = 0x38
  sram12_seg :                        	org = 0x000022B8, len = 0x8
  sram13_seg :                        	org = 0x000022C0, len = 0x38
  sram14_seg :                        	org = 0x000022F8, len = 0x8
  sram15_seg :                        	org = 0x00002300, len = 0x38
  sram16_seg :                        	org = 0x00002338, len = 0x8
  sram17_seg :                        	org = 0x00002340, len = 0x38
  sram18_seg :                        	org = 0x00002378, len = 0x48
  sram19_seg :                        	org = 0x000023C0, len = 0x140
  sram20_seg :                        	org = 0x00002500, len = 0x3FFDB00
  srom0_seg :                         	org = 0xFE000000, len = 0x300
  srom1_seg :                         	org = 0xFE000300, len = 0xFFFD00
#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST : org = 0x3ffdd50, len = 0x2000
#endif
}

PHDRS
{
  sram0_phdr PT_LOAD;
  sram1_phdr PT_LOAD;
  sram2_phdr PT_LOAD;
  sram3_phdr PT_LOAD;
  sram4_phdr PT_LOAD;
  sram5_phdr PT_LOAD;
  sram6_phdr PT_LOAD;
  sram7_phdr PT_LOAD;
  sram8_phdr PT_LOAD;
  sram9_phdr PT_LOAD;
  sram10_phdr PT_LOAD;
  sram11_phdr PT_LOAD;
  sram12_phdr PT_LOAD;
  sram13_phdr PT_LOAD;
  sram14_phdr PT_LOAD;
  sram15_phdr PT_LOAD;
  sram16_phdr PT_LOAD;
  sram17_phdr PT_LOAD;
  sram18_phdr PT_LOAD;
  sram19_phdr PT_LOAD;
  sram20_phdr PT_LOAD;
  sram20_bss_phdr PT_LOAD;
  srom0_phdr PT_LOAD;
  srom1_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(CONFIG_KERNEL_ENTRY)

/*  Memory boundary addresses:  */
_memmap_mem_sram_start = 0x4;
_memmap_mem_sram_end   = 0x4000000;
_memmap_mem_srom_start = 0xfe000000;
_memmap_mem_srom_end   = 0xff000000;

/*  Memory segment boundary addresses:  */
_memmap_seg_sram1_start = 0x2000;
_memmap_seg_sram1_max   = 0x2178;
_memmap_seg_sram2_start = 0x2178;
_memmap_seg_sram2_max   = 0x2180;
_memmap_seg_sram3_start = 0x2180;
_memmap_seg_sram3_max   = 0x21b8;
_memmap_seg_sram4_start = 0x21b8;
_memmap_seg_sram4_max   = 0x21c0;
_memmap_seg_sram5_start = 0x21c0;
_memmap_seg_sram5_max   = 0x21f8;
_memmap_seg_sram6_start = 0x21f8;
_memmap_seg_sram6_max   = 0x2200;
_memmap_seg_sram7_start = 0x2200;
_memmap_seg_sram7_max   = 0x2238;
_memmap_seg_sram8_start = 0x2238;
_memmap_seg_sram8_max   = 0x2240;
_memmap_seg_sram9_start = 0x2240;
_memmap_seg_sram9_max   = 0x2278;
_memmap_seg_sram10_start = 0x2278;
_memmap_seg_sram10_max   = 0x2280;
_memmap_seg_sram11_start = 0x2280;
_memmap_seg_sram11_max   = 0x22b8;
_memmap_seg_sram12_start = 0x22b8;
_memmap_seg_sram12_max   = 0x22c0;
_memmap_seg_sram13_start = 0x22c0;
_memmap_seg_sram13_max   = 0x22f8;
_memmap_seg_sram14_start = 0x22f8;
_memmap_seg_sram14_max   = 0x2300;
_memmap_seg_sram15_start = 0x2300;
_memmap_seg_sram15_max   = 0x2338;
_memmap_seg_sram16_start = 0x2338;
_memmap_seg_sram16_max   = 0x2340;
_memmap_seg_sram17_start = 0x2340;
_memmap_seg_sram17_max   = 0x2378;
_memmap_seg_sram18_start = 0x2378;
_memmap_seg_sram18_max   = 0x23c0;
_memmap_seg_sram19_start = 0x23c0;
_memmap_seg_sram19_max   = 0x2500;
_memmap_seg_sram20_start = 0x2500;
_memmap_seg_sram20_max   = 0x4000000;
_memmap_seg_srom0_start = 0xfe000000;
_memmap_seg_srom0_max   = 0xfe000300;
_memmap_seg_srom1_start = 0xfe000300;
_memmap_seg_srom1_max   = 0xff000000;

_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = 0x2000);
PROVIDE(_memmap_reset_vector = 0xfe000000);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x70000007;
_memmap_cacheattr_wt_base = 0xB000000B;
_memmap_cacheattr_bp_base = 0x30000003;
_memmap_cacheattr_unused_mask = 0x0FFFFFF0;
_memmap_cacheattr_wb_trapnull = 0x73333337;
_memmap_cacheattr_wba_trapnull = 0x73333337;
_memmap_cacheattr_wbna_trapnull = 0x73333337;
_memmap_cacheattr_wt_trapnull = 0xB333333B;
_memmap_cacheattr_bp_trapnull = 0x33333333;
_memmap_cacheattr_wb_strict = 0x7CCCCCC7;
_memmap_cacheattr_wt_strict = 0xBCCCCCCB;
_memmap_cacheattr_bp_strict = 0x3CCCCCC3;
_memmap_cacheattr_wb_allvalid = 0x73333337;
_memmap_cacheattr_wt_allvalid = 0xB333333B;
_memmap_cacheattr_bp_allvalid = 0x33333333;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{

  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
    _memmap_seg_sram1_end = ALIGN(0x8);
  } >sram1_seg :sram1_phdr

  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram2_end = ALIGN(0x8);
  } >sram2_seg :sram2_phdr

  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram3_end = ALIGN(0x8);
  } >sram3_seg :sram3_phdr

  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram4_end = ALIGN(0x8);
  } >sram4_seg :sram4_phdr

  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram5_end = ALIGN(0x8);
  } >sram5_seg :sram5_phdr

  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram6_end = ALIGN(0x8);
  } >sram6_seg :sram6_phdr

  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram7_end = ALIGN(0x8);
  } >sram7_seg :sram7_phdr

  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram8_end = ALIGN(0x8);
  } >sram8_seg :sram8_phdr

  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram9_end = ALIGN(0x8);
  } >sram9_seg :sram9_phdr

  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram10_end = ALIGN(0x8);
  } >sram10_seg :sram10_phdr

  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram11_end = ALIGN(0x8);
  } >sram11_seg :sram11_phdr

  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram12_end = ALIGN(0x8);
  } >sram12_seg :sram12_phdr

  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram13_end = ALIGN(0x8);
  } >sram13_seg :sram13_phdr

  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram14_end = ALIGN(0x8);
  } >sram14_seg :sram14_phdr

  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram15_end = ALIGN(0x8);
  } >sram15_seg :sram15_phdr

  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram16_end = ALIGN(0x8);
  } >sram16_seg :sram16_phdr

  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram17_end = ALIGN(0x8);
  } >sram17_seg :sram17_phdr

  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram18_end = ALIGN(0x8);
  } >sram18_seg :sram18_phdr

  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram19_end = ALIGN(0x8);
  } >sram19_seg :sram19_phdr

  .sram.rodata : ALIGN(4)
  {
    _image_ram_start = ABSOLUTE(.);
    _sram_rodata_start = ABSOLUTE(.);
    *(.sram.rodata)
    _sram_rodata_end = ABSOLUTE(.);
  } >sram20_seg :sram20_phdr

#include <linker/common-rom.ld>

  .rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    /*  C++ constructor and destructor tables, properly ordered:  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);
    _rodata_end = ABSOLUTE(.);
  } >sram20_seg :sram20_phdr

  .sram.text : ALIGN(4)
  {
    _sram_text_start = ABSOLUTE(.);
    *(.sram.literal .sram.text)
    _sram_text_end = ABSOLUTE(.);
  } >sram20_seg :sram20_phdr

  _image_text_start =  ALIGN(4);
  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >sram20_seg :sram20_phdr
  _image_text_end = .;

  .sram.data : ALIGN(4)
  {
    _sram_data_start = ABSOLUTE(.);
    *(.sram.data)
    _sram_data_end = ABSOLUTE(.);
  } >sram20_seg :sram20_phdr

  .noinit :  ALIGN(4)
  {
    *(.noinit)
    *(.noinit.*)
  } >sram20_seg :sram20_phdr

  .data : ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
    *(.data)
    *(.data.*)
    KEEP(*(SW_ISR_TABLE))
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
    _data_end = ABSOLUTE(.);
  } >sram20_seg :sram20_phdr

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
    *(.sram.bss)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    _end = ALIGN(0x8);
    _image_ram_end = ABSOLUTE(.);
    PROVIDE(end = ALIGN(0x8));
    _stack_sentry = ALIGN(0x8);
    _memmap_seg_sram20_end = ALIGN(0x8);
  } >sram20_seg :sram20_bss_phdr
  __stack = 0x4000000;
  _heap_sentry = 0x4000000;

  .ResetVector.text : ALIGN(4)
  {
    _image_rom_start = ABSOLUTE(.);
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    _ResetVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom0_end = ALIGN(0x8);
  } >srom0_seg :srom0_phdr

  .srom.rodata : ALIGN(4)
  {
    _srom_rodata_start = ABSOLUTE(.);
    *(.srom.rodata)
    _srom_rodata_end = ABSOLUTE(.);
  } >srom1_seg :srom1_phdr

  .srom.text : ALIGN(4)
  {
    _srom_text_start = ABSOLUTE(.);
    *(.srom.literal .srom.text)
    _srom_text_end = ABSOLUTE(.);
    _memmap_seg_srom1_end = ALIGN(0x8);
    _image_rom_end = ABSOLUTE(.);
  } >srom1_seg :srom1_phdr
  .debug  0 :  { *(.debug) }
  .line  0 :  { *(.line) }
  .debug_srcinfo  0 :  { *(.debug_srcinfo) }
  .debug_sfnames  0 :  { *(.debug_sfnames) }
  .debug_aranges  0 :  { *(.debug_aranges) }
  .debug_pubnames  0 :  { *(.debug_pubnames) }
  .debug_info  0 :  { *(.debug_info) }
  .debug_abbrev  0 :  { *(.debug_abbrev) }
  .debug_line  0 :  { *(.debug_line) }
  .debug_frame  0 :  { *(.debug_frame) }
  .debug_str  0 :  { *(.debug_str) }
  .debug_loc  0 :  { *(.debug_loc) }
  .debug_macinfo  0 :  { *(.debug_macinfo) }
  .debug_weaknames  0 :  { *(.debug_weaknames) }
  .debug_funcnames  0 :  { *(.debug_funcnames) }
  .debug_typenames  0 :  { *(.debug_typenames) }
  .debug_varnames  0 :  { *(.debug_varnames) }
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
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif
}
