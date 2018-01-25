/*
 * Copyright Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * A platform and target independent link script to produce UHI
 * compliant binaries with varying levels of system initialization
 * support.
 */

 #define _LINKER
 #define _ASMLANGUAGE

 #include <autoconf.h>
 #include <linker/sections.h>
 #include <generated_dts_board.h>

 #include <linker/linker-defs.h>
 #include <linker/linker-tool.h>

 #ifdef CONFIG_XIP
 #define ROMABLE_REGION              ROM
 #else
 #define ROMABLE_REGION              RAM
 #endif
 #define RAMABLE_REGION              RAM

 MEMORY
 {
     ROM (rx)  : ORIGIN = CONFIG_FLASH_BASE_ADDRESS, LENGTH = \
         CONFIG_FLASH_SIZE * 1024
     RAM (rwx) : ORIGIN = CONFIG_SRAM_BASE_ADDRESS, LENGTH = \
        CONFIG_SRAM_SIZE * 1024
     /* Used by and documented in include/linker/intlist.ld */
     IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
 }

OUTPUT_FORMAT("elf32-tradlittlemips", "elf32-tradbigmips", \
    "elf32-tradlittlemips")
GROUP(-lc -luhi -lgcc)
SEARCH_DIR(.)
__DYNAMIC  =  0;
/* Force the exception handler to be registered */
EXTERN(__register_excpt_handler)
/* Force the exception handler to be included in the link */
EXTERN(__exception_entry)
/*
 * Require verbose exceptions. This can be changed to pull in
 * __exception_handle_quiet to reduce code size but be less
 * informative
 */
EXTERN(__exception_handle_verbose)
/* Force the interrupt handlers to be included in the link */
EXTERN(__isr_vec)
/* Require the UHI getargs support */
EXTERN(__getargs)

/*
 * Set the location of the top of the stack.  A value of 0 means
 * that it will be automatically placed at the highest address
 * available as described by the __memory_* setttings
 */
PROVIDE (__stack = 0);

/* Size of the memory returned by _get_ram_range */
PROVIDE (__memory_size = CONFIG_SRAM_SIZE * 1024);

/* Base of the memory returned by _get_ram_range */
PROVIDE (__memory_base = CONFIG_SRAM_BASE_ADDRESS);

/* Stride length for tlb software invalidate for tlbinvf
 * (mipsXXr3+). Some MIPS implementations may layout the sets/ways
 * differently in the index register. Either sets LSB or ways LSB.
 *
 * By setting this to 1 we presume that sets come first. The default boot
 * code will decrement this value from the Number of TLB entries.
 */
PROVIDE (__tlb_stride_length = 1);

/* By default, XPA is not used even if available. To enable XPA,
 * __enable_xpa should be 1.
 */
PROVIDE (__enable_xpa = 0);

/*
 * 0 = Do not use exception handler present in boot for UHI
 * 1 = Use exception handler present in boot for UHI if BEV is 0 at
 *     startup
 * 2 = Always use exception handler present in boot for UHI
 */
PROVIDE (__use_excpt_boot = 1);
/*
 * Include the code to be able to return to boot context.  This is
 * necessary if __use_excpt_boot != 0.
 */
EXTERN (__register_excpt_boot);

ASSERT (DEFINED(__register_excpt_boot) || __use_excpt_boot == 0,
	"Registration for boot context is required for UHI chaining")

/* Control if subnormal floating-point values are flushed to zero in
   hardware.  This applies to both FPU and MSA operations.  */
PROVIDE (__flush_to_zero = 1);

/* Set up the public symbols depending on whether the user has chosen
   quiet or verbose exception handling above */
EXTERN (__exception_handle);
PROVIDE(__exception_handle = (DEFINED(__exception_handle_quiet)
				      ? __exception_handle_quiet
				      : __exception_handle_verbose));
PROVIDE(_mips_handle_exception = __exception_handle);

/*
 * Initialize some symbols to be zero so we can reference them in the
 * crt0 without core dumping. These functions are all optional, but
 * we do this so we can have our crt0 always use them if they exist.
 * This is so BSPs work better when using the crt0 installed with gcc.
 * We have to initialize them twice, so we multiple object file
 * formats, as some prepend an underscore.
 */
PROVIDE (hardware_exit_hook = 0);
PROVIDE (hardware_hazard_hook = 0);
PROVIDE (hardware_init_hook = 0);
PROVIDE (software_init_hook = 0);

/* The default base address for application code is 0x80200000 which
   leaves 2M of space at the start of KSEG0 for a bootloader. */
PROVIDE (__app_start = CONFIG_SRAM_BASE_ADDRESS);
/* Set default vector spacing to 32 bytes. */
PROVIDE (__isr_vec_space = 32);
/* Leave space for 9 vector entries by default. 8 entry points and one
   fallback handler. */
PROVIDE (__isr_vec_count = 9);
/*
 * The start of flash must be set if including boot code.  By default
 * the use of boot code will mean that application code is copied
 * from flash to RAM at runtime before being executed.
 */
PROVIDE (__flash_start = DEFINED(__reset_vector) ? CONFIG_FLASH_BASE_ADDRESS : \
    __app_start);

SECTIONS
{
  /* Start of bootrom */
  .bootrom CONFIG_FLASH_BASE_ADDRESS : /* Runs uncached (from 0x9fc00000) until
    I$ is initialized. */
  AT (__flash_start)
  {
    *(.reset)		/* Reset entry point. */
    *(.boot)		/* Boot code. */
    . = ALIGN(8);
  } > ROMABLE_REGION = 0

  PROVIDE (__flash_app_start = SIZEOF(.bootrom) + __flash_start);

  /* Start of the application */
  .exception_vector ALIGN(__app_start, 0x1000) :
  AT (__flash_app_start)
  {
    __excpt_ebase = ABSOLUTE(.);
    __base = .;
    KEEP(* (.text.__exception_entry))
    . = __base + 0x200;
    KEEP(* (SORT(.text.__isr_vec*)))
    /* Leave space for all the vector entries */
    . = __base + 0x200 + (__isr_vec_space * __isr_vec_count);
    ASSERT(__isr_vec_space == (DEFINED(__isr_vec_sw0)
			       ? __isr_vec_sw1 - __isr_vec_sw0
			       : __isr_vec_space),
	   "Actual ISR vector spacing does not match __isr_vec_space");
    ASSERT(__base + 0x200 == (DEFINED(__isr_vec_sw0)
			      ? __isr_vec_sw0 & 0xfffffffe : __base + 0x200),
	   "__isr_vec_sw0 is not placed at EBASE + 0x200");
    . = ALIGN(8);
  } > ROMABLE_REGION = 0

  .text : {
#ifdef CONFIG_GEN_SW_ISR_TABLE
    KEEP(*(SW_ISR_TABLE))
#endif
    _image_text_start = .;
    _ftext = . ;
    PROVIDE (eprol  =  .);
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
    *(.mips16.fn.*)
    *(.mips16.call.*)
  } > ROMABLE_REGION
  .init : {
    KEEP (*(.init))
  } > ROMABLE_REGION
  .fini : {
    KEEP (*(.fini))
  } > ROMABLE_REGION
  .rel.sdata : {
    PROVIDE (__runtime_reloc_start = .);
    *(.rel.sdata)
    PROVIDE (__runtime_reloc_stop = .);
  } > RAMABLE_REGION
  PROVIDE (etext  =  .);
  _etext  =  .;

  .eh_frame_hdr : { *(.eh_frame_hdr) } > RAMABLE_REGION
  .eh_frame : { KEEP (*(.eh_frame)) } > RAMABLE_REGION
  .gcc_except_table : { *(.gcc_except_table) } > RAMABLE_REGION
  .jcr : { KEEP (*(.jcr)) } > RAMABLE_REGION
  .ctors    :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */

    KEEP (*crtbegin.o(.ctors))

    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */

    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } > RAMABLE_REGION

  .dtors    :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } > RAMABLE_REGION

  _image_text_end = .;

#include <linker/common-rom.ld>

  . = .;
  .MIPS.abiflags : {
    __MIPS_abiflags_start = .;
    *(.MIPS.abiflags)
    __MIPS_abiflags_end = .;
  } > RAMABLE_REGION
  .rodata : {
    *(.rdata)
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
  } > RAMABLE_REGION
   . = ALIGN(16);
   _fdata = .;
  .data : {
    _image_ram_start = .;
    __data_ram_start = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
  } > RAMABLE_REGION
  . = ALIGN(8);
  _gp = . + 0x8000;
  __global = _gp;
  .lit8 : {
    *(.lit8)
  } > RAMABLE_REGION
  .lit4 : {
    *(.lit4)
  } > RAMABLE_REGION
  .sdata : {
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
  } > RAMABLE_REGION

#include <linker/common-ram.ld>
  __data_ram_end = .;

  . = ALIGN(4);
  PROVIDE (edata  =  .);
  _edata  =  .;
  _fbss = .;
  .sbss : {
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
  } > RAMABLE_REGION
  .bss : {
    _bss_start = . ;
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
  } > RAMABLE_REGION

  . = ALIGN(4);
  PROVIDE (end = .);
  _end = .;
  _image_ram_end = .;

  /* Now place the data that is only needed within start.S and can be
     overwritten by the heap.  */
  .startdata : {
    *(.startdata)
  } > RAMABLE_REGION

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif

  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to
     the beginning of the section so we begin them at 0.  */

  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }

  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }

  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }

  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  .debug_ranges   0 : { *(.debug_ranges) }

  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }

  /* Special sections generated by gcc */
  /* Newer GNU linkers strip by default */
  .mdebug.abi32            0 : { KEEP(*(.mdebug.abi32)) }
  .mdebug.abiN32           0 : { KEEP(*(.mdebug.abiN32)) }
  .mdebug.abi64            0 : { KEEP(*(.mdebug.abi64)) }
  .mdebug.abiO64           0 : { KEEP(*(.mdebug.abiO64)) }
  .mdebug.eabi32           0 : { KEEP(*(.mdebug.eabi32)) }
  .mdebug.eabi64           0 : { KEEP(*(.mdebug.eabi64)) }
  .gcc_compiled_long32     0 : { KEEP(*(.gcc_compiled_long32)) }
  .gcc_compiled_long64     0 : { KEEP(*(.gcc_compiled_long64)) }
}
