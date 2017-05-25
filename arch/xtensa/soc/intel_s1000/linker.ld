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

#define RAMABLE_REGION sram0_seg :sram0_phdr
#define ROMABLE_REGION srom1_seg :srom1_phdr

MEMORY
{
  sram0_seg :                         	org = 0x80000000, len = 0x3EFE0000
  srom0_seg :                         	org = 0xBEFE0000, len = 0x300
  srom1_seg :                         	org = 0xBEFE0300, len = 0x100
  srom2_seg :                         	org = 0xBEFE0400, len = 0x180
  srom4_seg :                         	org = 0xBEFE0800, len = 0x178
  srom5_seg :                         	org = 0xBEFE0978, len = 0x8
  srom6_seg :                         	org = 0xBEFE0980, len = 0x38
  srom7_seg :                         	org = 0xBEFE09B8, len = 0x8
  srom8_seg :                         	org = 0xBEFE09C0, len = 0x38
  srom9_seg :                         	org = 0xBEFE09F8, len = 0x8
  srom10_seg :                        	org = 0xBEFE0A00, len = 0x38
  srom11_seg :                        	org = 0xBEFE0A38, len = 0x8
  srom12_seg :                        	org = 0xBEFE0A40, len = 0x38
  srom13_seg :                        	org = 0xBEFE0A78, len = 0x8
  srom14_seg :                        	org = 0xBEFE0A80, len = 0x38
  srom15_seg :                        	org = 0xBEFE0AB8, len = 0x8
  srom16_seg :                        	org = 0xBEFE0AC0, len = 0x38
  srom17_seg :                        	org = 0xBEFE0AF8, len = 0x8
  srom18_seg :                        	org = 0xBEFE0B00, len = 0x38
  srom19_seg :                        	org = 0xBEFE0B38, len = 0x8
  srom20_seg :                        	org = 0xBEFE0B40, len = 0x38
  srom21_seg :                        	org = 0xBEFE0B78, len = 0x48
  srom22_seg :                        	org = 0xBEFE0BC0, len = 0x40
  srom23_seg :                        	org = 0xBEFE0C00, len = 0x1D400
#ifdef CONFIG_GEN_ISR_TABLES
  IDT_LIST :                            org = 0xBEFFE000, len = 0x2000
#endif
}

PHDRS
{
  sram0_phdr PT_LOAD;
  sram0_bss_phdr PT_LOAD;
  srom0_phdr PT_LOAD;
  srom1_phdr PT_LOAD;
  srom2_phdr PT_LOAD;
  srom3_phdr PT_LOAD;
  srom4_phdr PT_LOAD;
  srom5_phdr PT_LOAD;
  srom6_phdr PT_LOAD;
  srom7_phdr PT_LOAD;
  srom8_phdr PT_LOAD;
  srom9_phdr PT_LOAD;
  srom10_phdr PT_LOAD;
  srom11_phdr PT_LOAD;
  srom12_phdr PT_LOAD;
  srom13_phdr PT_LOAD;
  srom14_phdr PT_LOAD;
  srom15_phdr PT_LOAD;
  srom16_phdr PT_LOAD;
  srom17_phdr PT_LOAD;
  srom18_phdr PT_LOAD;
  srom19_phdr PT_LOAD;
  srom20_phdr PT_LOAD;
  srom21_phdr PT_LOAD;
  srom22_phdr PT_LOAD;
  srom23_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(_ResetVector)

/*  Memory boundary addresses:  */
_memmap_mem_sram_start = 0x80000000;
_memmap_mem_sram_end   = 0xbefe0000;
_memmap_mem_srom_start = 0xbefe0000;
_memmap_mem_srom_end   = 0xbf000000;

/*  Memory segment boundary addresses:  */
_memmap_seg_sram0_start = 0x80000000;
_memmap_seg_sram0_max   = 0xbefe0000;
_memmap_seg_srom0_start = 0xbefe0000;
_memmap_seg_srom0_max   = 0xbefe0300;
_memmap_seg_srom1_start = 0xbefe0300;
_memmap_seg_srom1_max   = 0xbefe0400;
_memmap_seg_srom2_start = 0xbefe0400;
_memmap_seg_srom2_max   = 0xbefe0580;
_memmap_seg_srom4_start = 0xbefe0800;
_memmap_seg_srom4_max   = 0xbefe0978;
_memmap_seg_srom5_start = 0xbefe0978;
_memmap_seg_srom5_max   = 0xbefe0980;
_memmap_seg_srom6_start = 0xbefe0980;
_memmap_seg_srom6_max   = 0xbefe09b8;
_memmap_seg_srom7_start = 0xbefe09b8;
_memmap_seg_srom7_max   = 0xbefe09c0;
_memmap_seg_srom8_start = 0xbefe09c0;
_memmap_seg_srom8_max   = 0xbefe09f8;
_memmap_seg_srom9_start = 0xbefe09f8;
_memmap_seg_srom9_max   = 0xbefe0a00;
_memmap_seg_srom10_start = 0xbefe0a00;
_memmap_seg_srom10_max   = 0xbefe0a38;
_memmap_seg_srom11_start = 0xbefe0a38;
_memmap_seg_srom11_max   = 0xbefe0a40;
_memmap_seg_srom12_start = 0xbefe0a40;
_memmap_seg_srom12_max   = 0xbefe0a78;
_memmap_seg_srom13_start = 0xbefe0a78;
_memmap_seg_srom13_max   = 0xbefe0a80;
_memmap_seg_srom14_start = 0xbefe0a80;
_memmap_seg_srom14_max   = 0xbefe0ab8;
_memmap_seg_srom15_start = 0xbefe0ab8;
_memmap_seg_srom15_max   = 0xbefe0ac0;
_memmap_seg_srom16_start = 0xbefe0ac0;
_memmap_seg_srom16_max   = 0xbefe0af8;
_memmap_seg_srom17_start = 0xbefe0af8;
_memmap_seg_srom17_max   = 0xbefe0b00;
_memmap_seg_srom18_start = 0xbefe0b00;
_memmap_seg_srom18_max   = 0xbefe0b38;
_memmap_seg_srom19_start = 0xbefe0b38;
_memmap_seg_srom19_max   = 0xbefe0b40;
_memmap_seg_srom20_start = 0xbefe0b40;
_memmap_seg_srom20_max   = 0xbefe0b78;
_memmap_seg_srom21_start = 0xbefe0b78;
_memmap_seg_srom21_max   = 0xbefe0bc0;
_memmap_seg_srom22_start = 0xbefe0bc0;
_memmap_seg_srom22_max   = 0xbefe0c00;
_memmap_seg_srom23_start = 0xbefe0c00;
_memmap_seg_srom23_max   = 0xbf000000;

_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = 0xbefe0800);
PROVIDE(_memmap_reset_vector = 0xbefe0000);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x00440000;
_memmap_cacheattr_wt_base = 0x00110000;
_memmap_cacheattr_bp_base = 0x00220000;
_memmap_cacheattr_unused_mask = 0xFF00FFFF;
_memmap_cacheattr_wb_trapnull = 0x2244222F;
_memmap_cacheattr_wba_trapnull = 0x2244222F;
_memmap_cacheattr_wbna_trapnull = 0x2255222F;
_memmap_cacheattr_wt_trapnull = 0x2211222F;
_memmap_cacheattr_bp_trapnull = 0x2222222F;
_memmap_cacheattr_wb_strict = 0xFF44FFFF;
_memmap_cacheattr_wt_strict = 0xFF11FFFF;
_memmap_cacheattr_bp_strict = 0xFF22FFFF;
_memmap_cacheattr_wb_allvalid = 0x22442222;
_memmap_cacheattr_wt_allvalid = 0x22112222;
_memmap_cacheattr_bp_allvalid = 0x22222222;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{

  .sram.rodata : ALIGN(4)
  {
    _image_ram_start = ABSOLUTE(.);
    _sram_rodata_start = ABSOLUTE(.);
    *(.sram.rodata)
    _sram_rodata_end = ABSOLUTE(.);
  } >sram0_seg :sram0_phdr

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
  } >sram0_seg :sram0_phdr

  .sram.text : ALIGN(4)
  {
    _sram_text_start = ABSOLUTE(.);
    *(.sram.literal .sram.text)
    _sram_text_end = ABSOLUTE(.);
  } >sram0_seg :sram0_phdr

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
  } >sram0_seg :sram0_phdr
  _image_text_end = .;

  .sram.data : ALIGN(4)
  {
    _sram_data_start = ABSOLUTE(.);
    *(.sram.data)
    _sram_data_end = ABSOLUTE(.);
  } >sram0_seg :sram0_phdr

  .noinit :  ALIGN(4)
  {
    *(.noinit)
    *(.noinit.*)
  } >sram0_seg :sram0_phdr

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
  } >sram0_seg :sram0_phdr

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
    _memmap_seg_sram0_end = ALIGN(0x8);
  } >sram0_seg :sram0_bss_phdr
  __stack = 0xbefe0000;
  _heap_sentry = 0xbefe0000;

  .ResetVector.text : ALIGN(4)
  {
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    _ResetVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom0_end = ALIGN(0x8);
  } >srom0_seg :srom0_phdr

  .MemoryExceptionVector.literal : ALIGN(4)
  {
    _MemoryExceptionVector_literal_start = ABSOLUTE(.);
    *(.MemoryExceptionVector.literal)
    _MemoryExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom1_end = ALIGN(0x8);
  } >srom1_seg :srom1_phdr

  .MemoryExceptionVector.text : ALIGN(4)
  {
    _MemoryExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.MemoryExceptionVector.text))
    _MemoryExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom2_end = ALIGN(0x8);
  } >srom2_seg :srom2_phdr

  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    _WindowVectors_text_end = ABSOLUTE(.);
    _memmap_seg_srom4_end = ALIGN(0x8);
  } >srom4_seg :srom4_phdr

  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom5_end = ALIGN(0x8);
  } >srom5_seg :srom5_phdr

  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    _Level2InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom6_end = ALIGN(0x8);
  } >srom6_seg :srom6_phdr

  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom7_end = ALIGN(0x8);
  } >srom7_seg :srom7_phdr

  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    _Level3InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom8_end = ALIGN(0x8);
  } >srom8_seg :srom8_phdr

  .Level4InterruptVector.literal : ALIGN(4)
  {
    _Level4InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level4InterruptVector.literal)
    _Level4InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom9_end = ALIGN(0x8);
  } >srom9_seg :srom9_phdr

  .Level4InterruptVector.text : ALIGN(4)
  {
    _Level4InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level4InterruptVector.text))
    _Level4InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom10_end = ALIGN(0x8);
  } >srom10_seg :srom10_phdr

  .Level5InterruptVector.literal : ALIGN(4)
  {
    _Level5InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level5InterruptVector.literal)
    _Level5InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom11_end = ALIGN(0x8);
  } >srom11_seg :srom11_phdr

  .Level5InterruptVector.text : ALIGN(4)
  {
    _Level5InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level5InterruptVector.text))
    _Level5InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom12_end = ALIGN(0x8);
  } >srom12_seg :srom12_phdr

  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom13_end = ALIGN(0x8);
  } >srom13_seg :srom13_phdr

  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    _DebugExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom14_end = ALIGN(0x8);
  } >srom14_seg :srom14_phdr

  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom15_end = ALIGN(0x8);
  } >srom15_seg :srom15_phdr

  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    _NMIExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom16_end = ALIGN(0x8);
  } >srom16_seg :srom16_phdr

  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom17_end = ALIGN(0x8);
  } >srom17_seg :srom17_phdr

  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    _KernelExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom18_end = ALIGN(0x8);
  } >srom18_seg :srom18_phdr

  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    _UserExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom19_end = ALIGN(0x8);
  } >srom19_seg :srom19_phdr

  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    _UserExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom20_end = ALIGN(0x8);
  } >srom20_seg :srom20_phdr

  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_srom21_end = ALIGN(0x8);
  } >srom21_seg :srom21_phdr

  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_srom22_end = ALIGN(0x8);
  } >srom22_seg :srom22_phdr

  .srom.rodata : ALIGN(4)
  {
    _srom_rodata_start = ABSOLUTE(.);
    *(.srom.rodata)
    _srom_rodata_end = ABSOLUTE(.);
  } >srom23_seg :srom23_phdr

  .srom.text : ALIGN(4)
  {
    _srom_text_start = ABSOLUTE(.);
    *(.srom.literal .srom.text)
    _srom_text_end = ABSOLUTE(.);
    _memmap_seg_srom23_end = ALIGN(0x8);
  } >srom23_seg :srom23_phdr
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

