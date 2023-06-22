/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/aarch32/cortex_a_r/scripts/linker.ld>

SECTIONS
{
#ifdef CONFIG_OPENAMP_RSC_TABLE
    SECTION_PROLOGUE(.resource_table,,)
    {
        KEEP(*(.resource_table*))
    } GROUP_LINK_IN(ROMABLE_REGION)
#endif /* CONFIG_OPENAMP_RSC_TABLE */
}
