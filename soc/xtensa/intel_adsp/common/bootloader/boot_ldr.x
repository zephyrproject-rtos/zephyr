OUTPUT_ARCH(xtensa)
ENTRY(boot_entry)

#include <autoconf.h> /* Not a "zephyr" file, need this explicitly */
#include <cavs-mem.h>

/* Offset of the entry point from the manifest start in IMR.  Magic
 * number must be synchronized with the module and rimage configuration!
 */
#define ENTRY_POINT_OFF 0x6000

/* These are legacy; needed by xtensa arch bootstrap code, but not by the
 * bootloader per se which isn't responsible for handling exception
 * setup or region protection option configuration.
 */
PROVIDE(_memmap_vecbase_reset = 0xbe010000);
PROVIDE(_memmap_cacheattr_reset = 0xff42fff2);

MEMORY {
  imr :
        org = CONFIG_IMR_MANIFEST_ADDR + ENTRY_POINT_OFF,
        len = 0x100000
}

PHDRS {
  imr_phdr PT_LOAD;
}

SECTIONS {
  .text : {
    /* Entry point MUST be here per external configuration */
    KEEP (*(.boot_entry.text))

    *(.boot_entry.literal)
    *(.literal .literal.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *( .text .text.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    KEEP (*(.ResetVector.text))
    KEEP (*(.ResetHandler.text))
  } >imr :imr_phdr

  .rodata : {
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
  } >imr :imr_phdr

  /* Note that bootloader ".bss" goes into the ELF program header as
   * real data, that way we can be sure the ROM loader has cleared the
   * memory.
   */
  .bss : ALIGN(8) {
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
  } >imr :imr_phdr

  /* The .data section comes last.  This is because rimage seems to
   * want this page-aligned or it will throw an error, not sure why
   * since all the ROM cares about is a contiguous region.
   */
  .data : ALIGN(4096) {
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
  } >imr :imr_phdr

  .comment 0 : { *(.comment) }
  .debug 0 : { *(.debug) }
  .debug_ranges 0 : { *(.debug_ranges) }
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
  .debug.xt.callgraph 0 : {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
}
