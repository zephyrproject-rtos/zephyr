/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-tool.h>


GROUP_START(PSRAM)
  SECTION_PROLOGUE(.vg_fb_bss,(NOLOAD),SUBALIGN(4))
  {
    __vg_psram_start = .;
    *(.lvgl_buf*);
    __vg_psram_end = .;
  } GROUP_LINK_IN(PSRAM)

