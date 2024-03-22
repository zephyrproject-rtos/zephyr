/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Cortex-M0+ application flash image area */

. = ALIGN(4);
__cy_m0p_code_start = . ;
KEEP(*(.cy_m0p_image))
. = CONFIG_SOC_PSOC6_CM0P_IMAGE_ROM_SIZE;
__cy_m0p_code_end = . ;
