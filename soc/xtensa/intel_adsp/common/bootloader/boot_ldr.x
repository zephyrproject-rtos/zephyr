OUTPUT_ARCH(xtensa)

#include <soc/memory.h>

PROVIDE(__memctl_default = 0x00000000);
PROVIDE(_MemErrorHandler = 0x00000000);
MEMORY
{
  boot_entry_text :
        org = IMR_BOOT_LDR_TEXT_ENTRY_BASE,
        len = IMR_BOOT_LDR_TEXT_ENTRY_SIZE
  boot_entry_lit :
        org = IMR_BOOT_LDR_LIT_BASE,
        len = IMR_BOOT_LDR_LIT_SIZE
  sof_text :
        org = IMR_BOOT_LDR_TEXT_BASE,
        len = IMR_BOOT_LDR_TEXT_SIZE,
  sof_data :
        org = IMR_BOOT_LDR_DATA_BASE,
        len = IMR_BOOT_LDR_DATA_SIZE
  sof_bss_data :
        org = IMR_BOOT_LDR_BSS_BASE,
        len = IMR_BOOT_LDR_BSS_SIZE
  sof_stack :
        org = BOOT_LDR_STACK_BASE,
        len = BOOT_LDR_STACK_SIZE
  wnd0 :
        org = HP_SRAM_WIN0_BASE,
        len = HP_SRAM_WIN0_SIZE
}
PHDRS
{
  boot_entry_text_phdr PT_LOAD;
  boot_entry_lit_phdr PT_LOAD;
  sof_text_phdr PT_LOAD;
  sof_data_phdr PT_LOAD;
  sof_bss_data_phdr PT_LOAD;
  sof_stack_phdr PT_LOAD;
  wnd0_phdr PT_LOAD;
}
ENTRY(boot_entry)
EXTERN(reset_vector)
SECTIONS
{
  .boot_entry.text : ALIGN(4)
  {
    _boot_entry_text_start = ABSOLUTE(.);
    KEEP (*(.boot_entry.text))
    _boot_entry_text_end = ABSOLUTE(.);
  } >boot_entry_text :boot_entry_text_phdr
  .boot_entry.literal : ALIGN(4)
  {
    _boot_entry_literal_start = ABSOLUTE(.);
    *(.boot_entry.literal)
    *(.literal .literal.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    _boot_entry_literal_end = ABSOLUTE(.);
  } >boot_entry_lit :boot_entry_lit_phdr
  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *( .text .text.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    KEEP (*(.ResetVector.text))
    KEEP (*(.ResetHandler.text))
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >sof_text :sof_text_phdr
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
  } >sof_data :sof_data_phdr
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
    _data_end = ABSOLUTE(.);
  } >sof_data :sof_data_phdr
  .lit4 : ALIGN(4)
  {
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
  } >sof_data :sof_data_phdr
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
  } >sof_bss_data :sof_bss_data_phdr
 _man = 0x1234567;
  PROVIDE(_memmap_vecbase_reset = (((((((0xBE000000 + 0x8000) + 0x2000) + 0x800) + 0x800) + 0x1000) + 0x2000) + (0x1000 + 0x1000)));
  _memmap_cacheattr_wbna_trapnull = 0xFF42FFF2;
  PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wbna_trapnull);
  __stack = 0xBE000000 + (1 * 0x1000);
  __wnd0 = ((((((0xBE000000 + 0x8000) + 0x2000) + 0x800) + 0x800) + 0x1000) + 0x2000);
  __wnd0_size = (0x1000 + 0x1000);
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
}
