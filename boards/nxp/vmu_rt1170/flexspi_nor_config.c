/*
 * Copyright (c) 2019, MADMACHINE LIMITED
 * Copyright 2024 NXP
 *
 * refer to hal_nxp board file
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <flexspi_nor_config.h>

/*!
 * @brief ROM API init
 *
 * Get the bootloader api entry address.
 */
void ROM_API_Init(void);

/*!
 * @brief Initialize Serial NOR devices via FLEXSPI
 *
 * This function checks and initializes the FLEXSPI module for the other FLEXSPI APIs.
 *
 * @param instance storage the instance of FLEXSPI.
 * @param config A pointer to the storage for the driver runtime state.
 *
 * @retval kStatus_Success Api was executed successfully.
 * @retval kStatus_InvalidArgument A invalid argument is provided.
 * @retval kStatus_ROM_FLEXSPI_InvalidSequence A invalid Sequence is provided.
 * @retval kStatus_ROM_FLEXSPI_SequenceExecutionTimeout Sequence Execution timeout.
 * @retval kStatus_ROM_FLEXSPI_DeviceTimeout the device timeout
 */
status_t ROM_FLEXSPI_NorFlash_Init(uint32_t instance, struct flexspi_nor_config_t *config);

#ifdef CONFIG_NXP_IMXRT_BOOT_HEADER
__attribute__((section(".boot_hdr.conf")))

/* Config used for booting */

const struct flexspi_nor_config_t qspi_flash_config = {
	.mem_config = {
		.tag = FLEXSPI_CFG_BLK_TAG,
		.version = FLEXSPI_CFG_BLK_VERSION,
		.read_sample_clk_src =
			flexspi_read_sample_clk_loopback_internally,
		.cs_hold_time = 1u,
		.cs_setup_time = 1u,
		.sflash_pad_type = serial_flash_1_pad,
		.serial_clk_freq = flexspi_serial_clk_80mhz,
		.sflash_a1_size = 64u * 1024u * 1024u,
		.lookup_table = {
			FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
					0x03, RADDR_SDR,
					FLEXSPI_1PAD, 0x18),
			FLEXSPI_LUT_SEQ(READ_SDR, FLEXSPI_1PAD,
					0x04, STOP,
					FLEXSPI_1PAD, 0),
		},
	},
	.page_size = 256u,
	.sector_size = 4u * 1024u,
	.block_size = 64u * 1024u,
	.is_uniform_block_size = false,
};
#endif /* CONFIG_NXP_IMXRT_BOOT_HEADER */

/* Config used for code execution */
const struct flexspi_nor_config_t g_flash_fast_config = {
	.mem_config = {
		.tag                 = FLEXSPI_CFG_BLK_TAG,
		.version             = FLEXSPI_CFG_BLK_VERSION,
		.read_sample_clk_src    = flexspi_read_sample_clk_external_input_from_dqs_pad,
		.cs_hold_time          = 1,
		.cs_setup_time         = 1,
		.device_mode_cfg_enable = 1,
		.device_mode_type      = device_config_cmd_type_spi2xpi,
		.wait_time_cfg_commands = 1,
		.device_mode_seq = {
			.seq_num   = 1,
			.seq_id    = 6, /* See Lookup table for more details */
			.reserved = 0,
		},
		.device_mode_arg = 2, /* Enable OPI DDR mode */
		.controller_misc_option =
		(1u << flexspi_misc_offset_safe_config_freq_enable)
			| (1u << flexspi_misc_offset_ddr_mode_enable),
		.device_type    = flexspi_device_type_serial_nor,
		.sflash_pad_type = serial_flash_8_pads,
		.serial_clk_freq = flexspi_serial_clk_200mhz,
		.sflash_a1_size  = 64ul * 1024u * 1024u,
		.busy_offset      = 0u,
		.busy_bit_polarity = 0u,
		.lookup_table = {
			/* Read */
			[0 + 0] = FLEXSPI_LUT_SEQ(CMD_DDR, FLEXSPI_8PAD,
				0xEE, CMD_DDR, FLEXSPI_8PAD, 0x11),
			[0 + 1] = FLEXSPI_LUT_SEQ(RADDR_DDR, FLEXSPI_8PAD,
				0x20, DUMMY_DDR, FLEXSPI_8PAD, 0x28),
			[0 + 2] = FLEXSPI_LUT_SEQ(READ_DDR, FLEXSPI_8PAD,
				0x04, STOP, FLEXSPI_1PAD, 0x00),

			/* Write enable SPI */
			[4 * 3 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x06, STOP, FLEXSPI_1PAD, 0x00),

			/*Write Configuration Register 2 =01, Enable OPI DDR mode*/
			[4 * 6 + 0] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x72, CMD_SDR, FLEXSPI_1PAD, 0x00),
			[4 * 6 + 1] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x00, CMD_SDR, FLEXSPI_1PAD, 0x00),
			[4 * 6 + 2] = FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD,
				0x00, WRITE_SDR, FLEXSPI_1PAD, 0x01),

		},
	},
	.page_size           = 256u,
	.sector_size         = 4u * 1024u,
	.block_size          = 64u * 1024u,
	.is_uniform_block_size = false,
	.ipcmd_serial_clk_freq = 1,
	.serial_nor_type = 2,
	.reserve2[0] = 0x7008200,
};


__ramfunc int imxrt_reclock_initialize(void)
{
	const uint32_t instance =  1;

	volatile struct flexspi_nor_config_t bootConfig;

	memcpy((struct flexspi_nor_config_t *)&bootConfig, &g_flash_fast_config,
	       sizeof(struct flexspi_nor_config_t));
	bootConfig.mem_config.tag = FLEXSPI_CFG_BLK_TAG;

	ROM_API_Init();

	ROM_FLEXSPI_NorFlash_Init(instance, (struct flexspi_nor_config_t *)&bootConfig);

	return 0;
}

SYS_INIT(imxrt_reclock_initialize,  PRE_KERNEL_1, 0);
