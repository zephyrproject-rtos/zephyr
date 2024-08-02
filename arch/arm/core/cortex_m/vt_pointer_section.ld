/*
 * SPDX-License-Identifier: Apache-2.0
 */

/* Copied from linker.ld */

/* Reserved 4 bytes to save vector table base address */
  SECTION_PROLOGUE(.vt_pointer,(NOLOAD),)
  {
          *(.vt_pointer_section)
          *(".vt_pointer_section.*")
  } GROUP_LINK_IN(RAMABLE_REGION)
