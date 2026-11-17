/*
 * Copyright (C) 2023-2026 HPMicro
 * Copyright (C) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/toolchain.h>

#if defined(CONFIG_XIP)
/*
 * BootROM reads this NOR configuration block from flash offset 0x400.
 * The values match the HPM5300EVK QSPI NOR configuration.
 */
__section(".nor_cfg_option") static const uint32_t hpm_nor_cfg_option[4] __used = {
	0xfcf90002U,
	0x00000005U,
	0x00001000U,
	0x00000000U,
};
#endif
