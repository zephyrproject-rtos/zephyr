/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

KEEP(*(.cy_ramfunc))
KEEP(*(.cy_socmem_data))
KEEP(*(.cy_itcm))
*cy_syslib_ext.S.obj(.text*)
