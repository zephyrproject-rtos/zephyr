
/*
 * SPDX-License-Identifier: Apache-2.0
 * Extracted from:
 * ext/hal/cypress/.../devices/psoc6/linker/gcc/cy8c6xx6_cm0plus.ld
 */

/* Cortex-M0+ application flash image area */
. = ALIGN(4);
__cy_m0p_code_start = . ;
KEEP(*(.cy_m0p_image))
__cy_m0p_code_end = . ;
