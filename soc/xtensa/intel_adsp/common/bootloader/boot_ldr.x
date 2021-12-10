OUTPUT_ARCH(xtensa)
ENTRY(rom_entry)

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
    *(.imr .imr.*)
  } >imr :imr_phdr

  /* The data sections come last.  This is because rimage seems to
   * want this page-aligned or it will throw an error, not sure why
   * since all the ROM cares about is a contiguous region.  And it's
   * particularly infuriating as it precludes linker .rodata next to
   * .text.
   */
  .rodata : ALIGN(4096) {
    *(.imrdata .imrdata.*)
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
