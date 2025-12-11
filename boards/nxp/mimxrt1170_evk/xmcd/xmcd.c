/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xmcd.h"

#if defined(XIP_BOOT_HEADER_ENABLE) && (XIP_BOOT_HEADER_ENABLE == 1)

#if defined(XIP_BOOT_HEADER_XMCD_ENABLE) && (XIP_BOOT_HEADER_XMCD_ENABLE == 1)
__attribute__((section(".boot_hdr.xmcd_data"), used))

const uint32_t xmcd_data[] = {
	/* Tag = 0xC, Version = 0, Memory Interface: SEMC,
	 * Instance: 0 - ignored,
	 * Configuration block type: 0 - Ignored(Handled inside
	 * the SDRAM configuration structure)
	 * Configuration block size: 13 (4-byte header + 9-byte
	 * option block)
	 */
	0xC010000D,
	/* Magic_number = 0xA1, Version = 1,
	 * Config_option: Simplified, SDRAM clock: 198MHz
	 */
	0xC60001A1,
	/* SDRAM CS0 size: 64MBytes */
	0x00010000,
	/* Port_size: 32-bit */
	0x02};

#endif /* XIP_BOOT_HEADER_XMCD_ENABLE */
#endif /* XIP_BOOT_HEADER_ENABLE */
