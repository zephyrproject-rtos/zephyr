
/*
 * SPDX-License-Identifier: Apache-2.0
 * Extracted from:
 * ext/hal/cypress/.../devices/psoc6/linker/gcc/cy8c6xx6_cm0plus.ld
 */

/* Cortex-M0+ application ram image area */
/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.ram_m0p_image,(NOLOAD),)
{
. = CONFIG_SRAM_OFFSET;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
