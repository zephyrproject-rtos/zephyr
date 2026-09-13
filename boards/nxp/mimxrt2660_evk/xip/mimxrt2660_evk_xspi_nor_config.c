/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Boot data the i.MX RT266x boot ROM reads from the start of the XSPI0 NOR
 * flash, before any of this image runs.
 *
 * Derived from the MCUXpresso SDK's
 * examples/_boards/mimxrt2660evk/xip/mimxrt2660evk_xspi_nor_config.c.
 *
 * Offsets are supplied by soc/nxp/imxrt/boot_header.ld from the series Kconfig:
 * the flash configuration block at 0x400, the external memory configuration
 * data at 0xA00, and the container region at 0x1000. This file fills in the
 * first two; the container comes from soc.c.
 */

#include <zephyr/kernel.h>
#include "mimxrt2660_evk_xspi_nor_config.h"

#if defined(CONFIG_NXP_IMXRT_BOOT_HEADER) && defined(CONFIG_BOOT_XSPI_NOR)

#if defined(CONFIG_EXTERNAL_MEM_CONFIG_DATA)
/*
 * External memory configuration data: what makes the on-board PSRAM usable
 * without any Zephyr driver. The ROM parses this and initializes the XSPI1
 * Xccela PSRAM before handing control to the image, which is why the PSRAM
 * memory region can simply be declared in devicetree.
 *
 * Word 0 is the configuration block header (the low byte 0x08 encodes the
 * 8-byte XMCD length), word 1 the device option word for the on-board
 * Winbond W958D6NMYA Xccela PSRAM; taken verbatim from the SDK rather than
 * reconstructed, because these encode the ROM's own interface.
 */
__attribute__((section(".boot_hdr.xmcd_data"), used))

const uint32_t xmcd_data[] = {
	0xC0010008U, 0xC0001A00U, /* W958D6NMYA */
};
#endif /* CONFIG_EXTERNAL_MEM_CONFIG_DATA */

/*
 * Flash configuration block for the on-board XSPI0 NOR (U24, Winbond
 * W25H512NW, 512 Mbit / 64 MiB), wired Quad (DATA0..3, SCLK0, SS0_N).
 *
 * The RT266x boot ROM reads this block to fetch the boot image over XSPI, so
 * for a flash-XIP image the read sequence, clock and geometry must be pinned
 * here rather than left to ROM defaults. The values match the configuration
 * that the SDK ships for this board.
 */
__attribute__((section(".boot_hdr.conf"), used))

/* clang-format off */
const struct xspi_nor_config xspi_nor_config_block = {
	.mem_config = {
		.tag = XSPI_CFG_BLK_TAG,
		.version = XSPI_CFG_BLK_VERSION,
		.read_sample_clk_src = kxSpiReadSampleClk_LoopbackFromDqsPad,
		.cs_hold_time = 3U,
		.cs_setup_time = 3U,
		.controller_misc_option = (1U << kxSpiMiscOffset_SafeConfigFreqEnable),
		.device_type = kxSpiDeviceType_SerialNOR,
		.sflash_pad_type = kSerialFlash_4Pads,
		.serial_clk_freq = kxSpiSerialClk_120MHz,
		.sflash_a1_size = 64U * 1024U * 1024U,
		.lookup_table = {
			/* Read (Quad I/O 0xEB, 24-bit addr, 7 dummy cycles) */
			[5 * CMD_LUT_SEQ_IDX_READ] =
				XSPI_LUT_SEQ(CMD, XSPI_1PAD, 0xEB, ADDR, XSPI_4PAD, 0x18),
			[5 * CMD_LUT_SEQ_IDX_READ + 1] =
				XSPI_LUT_SEQ(DUMMY, XSPI_4PAD, 0x07, READ, XSPI_4PAD, 0x08),

			/* Read Status */
			[5 * CMD_LUT_SEQ_IDX_READSTATUS] =
				XSPI_LUT_SEQ(CMD, XSPI_1PAD, 0x05, READ, XSPI_1PAD, 0x08),

			/* Write Enable */
			[5 * CMD_LUT_SEQ_IDX_WRITEENABLE] =
				XSPI_LUT_SEQ(CMD, XSPI_1PAD, 0x06, STOP, XSPI_1PAD, 0x0),

			/* Erase Sector */
			[5 * CMD_LUT_SEQ_IDX_ERASESECTOR] =
				XSPI_LUT_SEQ(CMD, XSPI_1PAD, 0x20, ADDR, XSPI_1PAD, 0x18),

			/* Erase Block */
			[5 * CMD_LUT_SEQ_IDX_ERASEBLOCK] =
				XSPI_LUT_SEQ(CMD, XSPI_1PAD, 0xD8, ADDR, XSPI_1PAD, 0x18),

			/* Page Program */
			[5 * CMD_LUT_SEQ_IDX_WRITE] =
				XSPI_LUT_SEQ(CMD, XSPI_1PAD, 0x02, ADDR, XSPI_1PAD, 0x18),
			[5 * CMD_LUT_SEQ_IDX_WRITE + 1] =
				XSPI_LUT_SEQ(WRITE, XSPI_1PAD, 0x08, STOP, XSPI_1PAD, 0x0),

			/* Chip Erase */
			[5 * CMD_LUT_SEQ_IDX_CHIPERASE] =
				XSPI_LUT_SEQ(CMD, XSPI_1PAD, 0x60, STOP, XSPI_1PAD, 0x0),
		},
	},
	.page_size = 256U,
	.sector_size = 4U * 1024U,
	.ipcmd_serial_clk_freq = 1U,
	.block_size = 64U * 1024U,
	.is_uniform_block_size = false,
};
/* clang-format on */

/* The block must fit the region reserved by the device linker script. */
BUILD_ASSERT(sizeof(struct xspi_nor_config) == 0x268U);

#endif /* CONFIG_NXP_IMXRT_BOOT_HEADER && CONFIG_BOOT_XSPI_NOR */
