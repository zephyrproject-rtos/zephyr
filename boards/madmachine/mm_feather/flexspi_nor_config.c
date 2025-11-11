/*
 * Copyright (c) 2019, MADMACHINE LIMITED
 *
 * refer to hal_nxp board file
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <flexspi_nor_config.h>

#ifdef CONFIG_NXP_IMXRT_BOOT_HEADER
__attribute__((section(".boot_hdr.conf")))

const struct flexspi_nor_config_t qspi_flash_config = {
	.mem_config = {
		.tag = FLEXSPI_CFG_BLK_TAG,
		.version = FLEXSPI_CFG_BLK_VERSION,
		.read_sample_clk_src =
			FLEXSPI_READ_SAMPLE_CLK_LOOPBACK_FROM_DQS_PAD,
		.cs_hold_time = 3u,
		.cs_setup_time = 3u,
		.sflash_pad_type = SERIAL_FLASH_4_PADS,
		.serial_clk_freq = FLEXSPI_SERIAL_CLK_100MHZ,
		.sflash_a1_size = 8u * 1024u * 1024u,
		.lookup_table = {
			FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
					0xEB, RADDR_SDR,
					FLEXSPI_4PAD, 0x18),
			FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD,
					0x06, READ_SDR,
					FLEXSPI_4PAD, 0x04),
		},
	},
	.page_size = 256u,
	.sector_size = 4u * 1024u,
	.block_size = 256u * 1024u,
	.is_uniform_block_size = false,
};
#endif /* CONFIG_NXP_IMXRT_BOOT_HEADER */
