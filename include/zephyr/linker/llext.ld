/* Specify the memory areas */
MEMORY
{
  all                   (RWX) : ORIGIN = 0x00000000, LENGTH = 128k
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into internal flash */
  .text :
  {
    . = ALIGN(4);
    *(.literal .literal.*)
    *(.text)                 /* .text sections (code) */
    *(.text*)                /* .text* sections (code) */
    . = ALIGN(4);
    KEEP(*(SORT_BY_NAME(._log_*.static.*)))
    . = ALIGN(4);
  } > all

  .rodata :
  {
    . = ALIGN(4);
    __init_APPLICATION_start = .; KEEP(*(SORT(.z_init_APPLICATION?_*))); KEEP(*(SORT(.z_init_APPLICATION??_*)));
    __init_end = .;
    . = ALIGN(4); __static_thread_data_list_start = .; KEEP(*(SORT_BY_NAME(.__static_thread_data.static.*))); __static_thread_data_list_end = .;
    *(.rodata)               /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)              /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } > all

  .data :
  {
    . = ALIGN(4);
    *(.data)                 /* .data sections */
    *(.data*)                /* .data* sections */
    . = ALIGN(4);
    *(SORT_BY_NAME(._k_*.static.*))
    . = ALIGN(4);
  } > all

  .got :
  {
      . = ALIGN(4);
      app_sgot = .;
      *(.got)
      *(.got.plt)
      app_egot = .;
      . = ALIGN(4);
  } > all

  /* .got.loc section (xtensa only) */
  .got.loc :
  {
      . = ALIGN(4);
      *(.got.loc)
      . = ALIGN(4);
  } >all


  /* PLT section contains code for accessing the dynamically linked functions
  * ie funtions from shared libraries in a postion independent manner */
  .plt :
  {
      . = ALIGN(4);
      *(.plt)
      . = ALIGN(4);
  } >all

    /* List of exported symbols in extension */
  .exported_sym :
  {
    . = ALIGN(4);
    KEEP(*(.exported_sym))
    . = ALIGN(4);
  } > all

  .bss :
  {
    . = ALIGN(4);
    *(.bss)
    *(.bss*)
    *(.noinit)
    *(.noinit*)
    . = ALIGN(4);
  } > all

  /** This section will be used by the debugger and disassembler to get more information
    * about raw data present in the code.
    * Indeed, it may be required to add some padding at some points in the code
    * in order to align a branch/jump destination on a particular bound.
    * Padding these instructions will generate null bytes that shall be
    * interpreted as data, and not code by the debugger or disassembler.
    * This section will only be present in the ELF file, not in the final binary
    * For more details, check GCC-212
    */
  .xt.prop 0 :
  {
    KEEP (*(.xt.prop))
  }

  .xt.lit 0 :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
  }



  .dynsym : { *(.dynsym) }
  .dynstr : { *(.dynstr) }
  /* useless for llext but usefull for inspecting elf with standard tools (nm, readelf, objdump). */
  .hash : { *(.hash) }
  .dynamic : { *(.dynamic) }

  /* Remove information from the standard libraries */
  /*/DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }*/

  /* https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst#build-attributes */
  /DISCARD/ : { *(.ARM.attributes) }

  /*
   .ARM.extab and .ARM.exidx are related to unwinding.
   You can find more information here http://infocenter.arm.com/help/topic/com.arm.doc.ihi0044e/index.html.
   You don't need them if you don't care about unwinding (unwinding is useful for C++ exception and for debugging)
  */
  /DISCARD/ :  {  *(.ARM.extab* .gnu.linkonce.armextab.*)  }
  /DISCARD/ :  {  *(.ARM.exidx*)  }

  /* discard .comment section */
  /DISCARD/ :  {  *(.comment)  }

}

