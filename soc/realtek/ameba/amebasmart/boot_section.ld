/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * KM0 IPC SRAM ring base for HAL ameba_ipc_api.c; the SDK ameba_layout.ld that
 * defines it is not linked. Fixed by silicon, matches ATF KM0_IPC_RAM.
 */
__km0_ipc_memory_start__ = 0x2301FD00;

  SECTION_PROLOGUE(.psram_image2.text.data,,)
  {
      *(*.sramdram.only.data)
      /*
       * Page-align the block mmu_regions.c maps R+X, so its 4 KB L2 override
       * does not overlap the surrounding R+W mapping.
       */
      . = ALIGN(4096);
      __image2_backtrace_start__ = .;
      *(*.sramdram.only.text)
      *(.wlan.critical.text*)

      /* lib_pmc.a .boot code (cpu_suspend/cpu_resume): needs this R+X, flat,
       * DRAM-retained block so cpu_resume runs at its link address, MMU off.
       */
      KEEP(*(.boot))

      . = ALIGN (4);
      __ipc_table_start__ = .;
      KEEP(*(*.ipc.table.data*))
      __ipc_table_end__ = .;
      __image2_backtrace_end__ = .;

      /* Page-align so data/BSS starts on a fresh page. */
      . = ALIGN(4096);
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
