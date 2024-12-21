/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* For RW612, fw_cpu1 is used for wifi firmware.
 * Place the wifi and ble firmwares in rodata section. If both binaries
 * are placed next to each other, the firmware loader will detect the
 * second binary as part of the first one, leading to initialization
 * issue, so add a padding of one word */
#if defined(CONFIG_NXP_MONOLITHIC_WIFI)
. = ALIGN(4);
KEEP(*(.fw_cpu1))
. += 4;
#endif

#if defined(CONFIG_NXP_MONOLITHIC_NBU)
. = ALIGN(4);
KEEP(*(.fw_cpu2))
#endif
