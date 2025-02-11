/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.st_stm32f0x_vt,(NOLOAD),)
{
_ram_vector_start = .;
. += _vector_end - _vector_start;
_ram_vector_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
