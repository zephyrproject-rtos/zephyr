/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "flash_config.h"

/*
 * FRDM-IMXRT700 uses a Winbond W25Q25PW quad SPI NOR flash on XSPI0 (the
 * MIMXRT700-EVK uses an octal part instead). The configuration below mirrors
 * the MCUXpresso SDK frdmimxrt700 flash_config so the boot configuration block
 * stays consistent between the SDK and Zephyr:
 *   W25Q25PW @ QSPI / SDR / 166 MHz / Read = 0xEC (1-4-4, 4-byte address)
 */
#if defined(CONFIG_NXP_IMXRT_BOOT_HEADER) && (CONFIG_NXP_IMXRT_BOOT_HEADER == 1)
__attribute__((section(".flash_conf"), used))

const fc_static_platform_config_t flash_config = {
	.xspi_fcb_block = {
		.mem_config = {
			.tag = FC_XSPI_CFG_BLK_TAG,
			.version = FC_XSPI_CFG_BLK_VERSION,
			.read_sample_clk_src =
				FC_XSPI_READ_SAMPLE_CLK_LOOPBACK_FROM_DQS_PAD,
			.cs_hold_time = 3u,
			.cs_setup_time = 3u,
			.controller_misc_option =
				(1u << FC_XSPI_MISC_OFFSET_SAFE_CONFIG_FREQ_ENABLE),
			.device_type = 1u,
			.sflash_pad_type = 4u,
			.serial_clk_freq = FC_XSPI_SERIAL_CLK_166MHZ,
			.sflash_a1_size = 32ul * 1024u * 1024u,
			.lut_custom_seq_enable = 0u,
			.lookup_table = {
				/* Read (1-4-4, 4-byte address), 0xEC */
				[0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD,
					0xEC, FC_CMD_RADDR_SDR, FC_XSPI_4PAD, 0x20),
				[1] = FC_XSPI_LUT_SEQ(FC_CMD_MODE_SDR, FC_XSPI_4PAD,
					0x00, FC_CMD_DUMMY_SDR, FC_XSPI_4PAD, 0x04),
				[2] = FC_XSPI_LUT_SEQ(FC_CMD_READ_SDR, FC_XSPI_4PAD,
					0x04, FC_CMD_STOP, FC_XSPI_1PAD, 0x0),

				/* Read status SPI */
				[5 * 1 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD,
					0x05, FC_CMD_READ_SDR, FC_XSPI_1PAD, 0x04),

				/* Write enable */
				[5 * 3 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_1PAD,
					0x06, FC_CMD_STOP, FC_XSPI_1PAD, 0x04),

				/* Erase sector */
				[5 * 5 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_4PAD,
					0x21, FC_CMD_RADDR_SDR, FC_XSPI_4PAD, 0x20),

				/* Page program */
				[5 * 9 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_4PAD,
					0x12, FC_CMD_RADDR_SDR, FC_XSPI_4PAD, 0x20),
				[5 * 9 + 1] = FC_XSPI_LUT_SEQ(FC_CMD_WRITE_SDR,
					FC_XSPI_4PAD, 0x4, FC_CMD_STOP, FC_XSPI_4PAD, 0x0),

				/* Erase chip */
				[5 * 13 + 0] = FC_XSPI_LUT_SEQ(FC_CMD_SDR, FC_XSPI_4PAD,
					0x60, FC_CMD_STOP, FC_XSPI_4PAD, 0x0),
			},
		},
		.page_size = 256u,
		.sector_size = 4u * 1024u,
		.ipcmd_serial_clk_freq = 1u,
		.block_size = 64u * 1024u,
		.flash_state_ctx = 0x06004100u,
	},
#ifdef BOOT_ENABLE_XSPI1_PSRAM
	.psram_config_block = {
		.xmcd_header = 0xC0010008,
		.xmcd_opt0 = 0xC0000700,
	},
#endif
};

#endif /* defined(CONFIG_NXP_IMXRT_BOOT_HEADER) && (CONFIG_NXP_IMXRT_BOOT_HEADER == 1) */
