/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Place text sections tagged with Silicon Labs code classification annotations into flash.
 */

SECTION_PROLOGUE(.cc_text,,)
{
    _cc_text_start = .;
    *(SORT_BY_ALIGNMENT(text_*[0-9]))
    _cc_text_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)
_cc_text_size = _cc_text_end - _cc_text_start;
