/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xmcd.h"

#if defined(CONFIG_NXP_IMXRT_BOOT_HEADER) && defined(CONFIG_EXTERNAL_MEM_CONFIG_DATA)

#if defined(XIP_BOOT_HEADER_XMCD_ENABLE) && (XIP_BOOT_HEADER_XMCD_ENABLE == 1)
__attribute__((section(".boot_hdr.xmcd_data"), used))

const uint32_t xmcd_data[] = {
	/* XMCD header: Tag = 0xC, Version = 0,
	 * Memory interface: 0 - FlexSPI, Instance: 2,
	 * Configuration block type: 0 - Simplified,
	 * Configuration block size: 12 (4-byte header + 8-byte option block)
	 */
	0xC002000CU,
	/* Simplified FlexSPI RAM configuration option 0:
	 * Tag = 0xC, Option size = 1 (two option words),
	 * Device type: 0 - HyperRAM, Misc: 0 - 1.8V,
	 * Maximum frequency: 8 - 166MHz, Size: 0 - auto detection
	 */
	0xC1000800U,
	/* Simplified FlexSPI RAM configuration option 1:
	 * RAM connection: 0 - PORTA, primary pinmux/DQS groups,
	 * write dummy cycles: 7, read dummy cycles: 7
	 */
	0x00000077U};

#endif /* XIP_BOOT_HEADER_XMCD_ENABLE */
#endif /* defined(CONFIG_NXP_IMXRT_BOOT_HEADER) && defined(CONFIG_EXTERNAL_MEM_CONFIG_DATA) */
