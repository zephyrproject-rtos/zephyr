/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.itcm_text,,)
{
    . = ALIGN(4);
    _itcm_text_start = .;
    KEEP(*(.itcm_text))
    _itcm_text_end = .;
} GROUP_LINK_IN(ITCM)
