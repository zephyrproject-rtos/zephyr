/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Cortex-M0+ application ram image area */

SECTION_PROLOGUE(.ram_m0p_image,(NOLOAD),)
{
. = CONFIG_SOC_PSOC6_CM0P_IMAGE_RAM_SIZE;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
