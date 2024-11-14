/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define SRAM_START DT_REG_ADDR(DT_NODELABEL(sram0))
#define SRAM_SIZE  DT_REG_SIZE(DT_NODELABEL(sram0))
#define DRAM_START DT_REG_ADDR(DT_NODELABEL(dram0))
#define DRAM_SIZE  DT_REG_SIZE(DT_NODELABEL(dram0))

MEMORY {
  sram (rwx) : ORIGIN = SRAM_START, LENGTH = SRAM_SIZE
  dram (rwx) : ORIGIN = DRAM_START, LENGTH = DRAM_SIZE

  IDT_LIST (rwx) : ORIGIN = 0xfff00000, LENGTH = 0x00100000 /* see below */
}

/* Included files want this API defined */
#define RAMABLE_REGION dram
#define ROMABLE_REGION dram

ENTRY(mtk_adsp_boot_entry)

SECTIONS {

#include <xtensa_vectors.ld>
  > sram

  .iram : {
    *(.iram0.*)
    *(.literal.iram .iram.*)
  } > sram

  _mtk_adsp_sram_end = .;

  .text : {
    __text_region_start = .;
    KEEP(*(.literal.init .init))
    *(.literal.init.* .init.*)
    *(.literal .text .literal.* .text.*)
    *(.gnu.linkonce.literal.* .gnu.linkone.t.*)
    *(.fini.literal .fini .fini.literal.* .fini.*)
    __text_region_end = .;
  } > dram

  .rodata : {
    __rodata_region_start = .;
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)

#include <snippets-rodata.ld>

    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    *(.gnu.linkonce.h.*)
  } > dram

#include <zephyr/linker/common-rom.ld>

  __rodata_region_end = .;

    . = ALIGN(4096); /* Switching MPU permissions, align by 4k */

  _image_ram_start = .;

  .data : {
    __data_start = .;
    *(.data .data.*)
    _trace_ctx_start = ABSOLUTE(.);
    *(.trace_ctx)
    _trace_ctx_end = ABSOLUTE(.);
    __data_end = .;
  } > dram

#include <zephyr/linker/common-ram.ld>

  .bss (NOLOAD) :
  {
    _bss_start = .;
    *(.bss .bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    _bss_end = .;
  } > dram

#include <zephyr/linker/common-noinit.ld>
#include <snippets-sections.ld>

  . = ALIGN(4096);
  _end = .;
  _mtk_adsp_dram_end = .;

  /* Non-runtime-loaded sections below */

#include <zephyr/linker/debug-sections.ld>

   /DISCARD/ : { *(.note.GNU-stack) }

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
  .xt.profile_range 0 : {
    KEEP (*(.xt.profile_range))
    KEEP (*(.gnu.linkonce.profile_range.*))
  }
  .xt.profile_ranges 0 : {
    KEEP (*(.xt.profile_ranges))
    KEEP (*(.gnu.linkonce.xt.profile_ranges.*))
  }
  .xt.profile_files 0 : {
    KEEP (*(.xt.profile_files))
    KEEP (*(.gnu.linkonce.xt.profile_files.*))
  }

/* Boilerplate.  The Xtensa arch uses CONFIG_GEN_ISR_TABLES to
 * generate the handler table, but doesn't actually use the (x86
 * specific) stuff that wants to go to a IDT_LIST memory area.  We
 * just dump it in an unreadable area at the top of memory.
 */
#include <zephyr/linker/intlist.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif
} /* SECTIONS */
