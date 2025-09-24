/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "evkbimxrt1050_flexspi_nor_config.h"

#if defined(XIP_BOOT_HEADER_ENABLE) && (XIP_BOOT_HEADER_ENABLE == 1)
__attribute__((section(".boot_hdr.conf"), used))

const flexspi_nor_config_t qspi_flash_config = {
	.mem_config = {
		.tag = FLEXSPI_CFG_BLK_TAG,
		.version = FLEXSPI_CFG_BLK_VERSION,
		.read_sample_clk_src
			= flexspi_read_sample_clk_loopback_from_dqs_pad,
		.cs_hold_time = 3u,
		.cs_setup_time = 3u,
		.sflash_pad_type = serial_flash_4_pads,
		.serial_clk_freq = flexspi_serial_clk_100mhz,
		.sflash_a1_size = 8u * 1024u * 1024u,
		.lookup_table = {
			/* Read LUTs */
			FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0xEB, RADDR_SDR, FLEXSPI_4PAD, 0x18),
			FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD,
				0x06, READ_SDR, FLEXSPI_4PAD, 0x04),
		},
	},
	.page_size = 256u,
	.sector_size = 4u * 1024u,
	.block_size = 64u * 1024u,
	.is_uniform_block_size = false,
};
#endif /* XIP_BOOT_HEADER_ENABLE */
