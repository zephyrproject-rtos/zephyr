/*
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

/* Reserved 4 bytes to save vector table base address. The RAM load region
 * stops ld from inheriting the ROM one of a preceding section (.sram_vt).
 */
  SECTION_PROLOGUE(.vt_pointer,(NOLOAD),)
  {
          *(.vt_pointer_section)
          *(".vt_pointer_section.*")
  } GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
