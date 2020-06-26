/*
 * Linker Script for Baytrail.
 *
 * This script is run through the GNU C preprocessor to align the memory
 * offsets with headers.
 *
 * Use spaces for formatting as cpp ignore tab sizes.
 */

#include <config.h>
#include <sof/lib/memory.h>
#include <xtensa/config/core-isa.h>

OUTPUT_ARCH(xtensa)

MEMORY
{
  vector_reset_text :
        org = XCHAL_RESET_VECTOR_PADDR,
        len = SOF_MEM_RESET_TEXT_SIZE
  vector_reset_lit :
        org = XCHAL_RESET_VECTOR_PADDR + SOF_MEM_RESET_TEXT_SIZE,
        len = SOF_MEM_RESET_LIT_SIZE
  vector_base_text :
        org = SOF_MEM_VECBASE_TEXT_BASE,
        len = SOF_MEM_VECBASE_LIT_SIZE
  vector_int2_lit :
        org = XCHAL_INTLEVEL2_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_int2_text :
        org = XCHAL_INTLEVEL2_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  vector_int3_lit :
        org = XCHAL_INTLEVEL3_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_int3_text :
        org = XCHAL_INTLEVEL3_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  vector_int4_lit :
        org = XCHAL_INTLEVEL4_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_int4_text :
        org = XCHAL_INTLEVEL4_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  vector_int5_lit :
        org = XCHAL_INTLEVEL5_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_int5_text :
        org = XCHAL_INTLEVEL5_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  vector_int6_lit :
        org = XCHAL_INTLEVEL6_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_int6_text :
        org = XCHAL_INTLEVEL6_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  vector_int7_lit :
        org = XCHAL_INTLEVEL7_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_int7_text :
        org = XCHAL_INTLEVEL7_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  vector_kernel_lit :
        org = XCHAL_KERNEL_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_kernel_text :
        org = XCHAL_KERNEL_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  vector_user_lit :
        org = XCHAL_USER_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_user_text :
        org = XCHAL_USER_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  vector_double_lit :
        org = XCHAL_DOUBLEEXC_VECTOR_PADDR - SOF_MEM_VECT_LIT_SIZE,
        len = SOF_MEM_VECT_LIT_SIZE
  vector_double_text :
        org = XCHAL_DOUBLEEXC_VECTOR_PADDR,
        len = SOF_MEM_VECT_TEXT_SIZE
  sof_text_start :
        org = XCHAL_NMI_VECTOR_VADDR + SOF_MEM_VECT_SIZE,
        len = (IRAM_BASE + IRAM_SIZE) - (XCHAL_NMI_VECTOR_VADDR + SOF_MEM_VECT_SIZE)
  sof_data_ro :
        org = DRAM0_BASE,
        len = SOF_MEM_RO_SIZE
  sof_data :
        org = DRAM0_BASE + SOF_MEM_RO_SIZE,
        len = HEAP_SYSTEM_BASE - (DRAM0_BASE + SOF_MEM_RO_SIZE)
  system_heap :
        org = HEAP_SYSTEM_BASE,
        len = HEAP_SYSTEM_SIZE
  system_runtime_heap :
        org = HEAP_SYS_RUNTIME_BASE,
        len = HEAP_SYS_RUNTIME_SIZE
  runtime_heap :
        org = HEAP_RUNTIME_BASE,
        len = HEAP_RUNTIME_SIZE
  buffer_heap :
        org = HEAP_BUFFER_BASE,
        len = HEAP_BUFFER_SIZE
  sof_stack :
        org = SOF_STACK_END,
        len = SOF_STACK_BASE - SOF_STACK_END
  mailbox :
        org = MAILBOX_BASE,
        len = MAILBOX_SIZE
  virtual_thread :
        org = SOF_VIRTUAL_THREAD_BASE,
        len = SOF_VIRTUAL_THREAD_SIZE
  static_log_entries_seg (!ari) :
        org = LOG_ENTRY_ELF_BASE,
        len = LOG_ENTRY_ELF_SIZE
}

PHDRS
{
  vector_reset_text_phdr PT_LOAD;
  vector_reset_lit_phdr PT_LOAD;
  vector_base_text_phdr PT_LOAD;
  vector_base_lit_phdr PT_LOAD;
  vector_int2_text_phdr PT_LOAD;
  vector_int2_lit_phdr PT_LOAD;
  vector_int3_text_phdr PT_LOAD;
  vector_int3_lit_phdr PT_LOAD;
  vector_int4_text_phdr PT_LOAD;
  vector_int4_lit_phdr PT_LOAD;
  vector_int5_text_phdr PT_LOAD;
  vector_int5_lit_phdr PT_LOAD;
  vector_int6_text_phdr PT_LOAD;
  vector_int6_lit_phdr PT_LOAD;
  vector_int7_text_phdr PT_LOAD;
  vector_int7_lit_phdr PT_LOAD;
  vector_kernel_text_phdr PT_LOAD;
  vector_kernel_lit_phdr PT_LOAD;
  vector_user_text_phdr PT_LOAD;
  vector_user_lit_phdr PT_LOAD;
  vector_double_text_phdr PT_LOAD;
  vector_double_lit_phdr PT_LOAD;
  sof_text_start_phdr PT_LOAD;
  sof_data_ro_phdr PT_LOAD;
  sof_data_phdr PT_LOAD;
  sof_data_bss_phdr PT_LOAD;
  system_heap_phdr PT_LOAD;
  system_runtime_heap_phdr PT_LOAD;
  runtime_heap_phdr PT_LOAD;
  buffer_heap_phdr PT_LOAD;
  mailbox_phdr PT_LOAD;
  virtual_thread_phdr PT_LOAD;
  sof_stack_phdr PT_LOAD;
  static_log_entries_phdr PT_NOTE;
}

/*  Default entry point:  */
ENTRY(_ResetVector)
_rom_store_table = 0;

/* ABI0 does not use Window base */
PROVIDE(_memmap_vecbase_reset = SOF_MEM_VECBASE_TEXT_BASE);

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
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wbna_trapnull);

SECTIONS
{
  .ResetVector.text : ALIGN(4)
  {
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
    KEEP(*(.init))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >sof_text_start :sof_text_start_phdr

  .reset.rodata : ALIGN(4)
  {
    _reset_rodata_start = ABSOLUTE(.);
    *(.reset.rodata)
    _reset_rodata_end = ABSOLUTE(.);
  } >sof_data_ro :sof_data_ro_phdr

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
  } >sof_data :sof_data_phdr

  .module_init : ALIGN(4)
  {
    _module_init_start = ABSOLUTE(.);
    *(*.module_init)
    _module_init_end = ABSOLUTE(.);
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
  } >sof_data :sof_data_bss_phdr

  /* stack */
  _end = SOF_STACK_END;
  PROVIDE(end = SOF_STACK_END);
  _stack_sentry = SOF_STACK_END;
  __stack = SOF_STACK_BASE;

  _virtual_thread_ptr = _virtual_thread_start;

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

  .system_heap (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (32);
    _system_heap_start = ABSOLUTE(.);
    . = . + HEAP_SYSTEM_SIZE;
    _system_heap_end = ABSOLUTE(.);
  } >system_heap :system_heap_phdr

  .system_runtime_heap (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (32);
    _system_runtime_heap_start = ABSOLUTE(.);
    . = . + HEAP_SYS_RUNTIME_SIZE;
    _system_runtime_heap_end = ABSOLUTE(.);
  } >system_runtime_heap :system_runtime_heap_phdr

  .runtime_heap (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (32);
    _runtime_heap_start = ABSOLUTE(.);
    . = . + HEAP_RUNTIME_SIZE;
    _runtime_heap_end = ABSOLUTE(.);
  } >runtime_heap :runtime_heap_phdr

  .buffer_heap (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (32);
    _buffer_heap_start = ABSOLUTE(.);
    . = . + HEAP_BUFFER_SIZE;
    _buffer_heap_end = ABSOLUTE(.);
  } >buffer_heap :buffer_heap_phdr

  .mailbox (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (32);
    _mailbox_start = ABSOLUTE(.);
    . = . + MAILBOX_SIZE;
    _mailbox_end = ABSOLUTE(.);
  } >mailbox :mailbox_phdr

   .virtual_thread (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (32);
    _virtual_thread_start = ABSOLUTE(.);
    . = . + SOF_VIRTUAL_THREAD_SIZE;
    _virtual_thread_end = ABSOLUTE(.);
  } >virtual_thread :virtual_thread_phdr

  .sof_stack (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (4096);
    _sof_stack_start = ABSOLUTE(.);
    . = . + SOF_STACK_TOTAL_SIZE;
    _sof_stack_end = ABSOLUTE(.);
  } >sof_stack :sof_stack_phdr

  .static_log_entries (COPY) : ALIGN(1024)
  {
    *(*.static_log*)
  } > static_log_entries_seg :static_log_entries_phdr

  .fw_ready : ALIGN(4)
  {
    KEEP (*(.fw_ready))
    KEEP (*(.fw_ready_metadata))
  } >sof_data :sof_data_phdr
}
