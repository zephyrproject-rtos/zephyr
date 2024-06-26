/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if defined(CONFIG_NXP_MONOLITHIC_WIFI)
. = ALIGN(4);
KEEP(*(.fw_cpu1))
. += 4;
#endif

#if defined(CONFIG_NXP_MONOLITHIC_BT)
. = ALIGN(4);
KEEP(*(.fw_cpu2_ble))
#endif
