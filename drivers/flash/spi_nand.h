/*
 * Copyright (c) 2022-2025 Macronix International Co., Ltd.
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unless otherwise stated, all defines have been confirmed to be common between Macronix and Micron
 * SPI NAND devices (MX35LF1G and MT29F4G).
 */

#ifndef __SPI_NAND_H__
#define __SPI_NAND_H__

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#define SPI_NAND_MAX_ID_LEN 8

/** SPI NAND commands */
enum spi_nand_cmd {
	/** Clear WEL bit in the status register */
	SPI_NAND_CMD_WRITE_DISABLE = 0x04,
	/** Set WEL bit in the status register */
	SPI_NAND_CMD_WRITE_ENABLE = 0x06,
	/** Read data from main storage to NAND cache */
	SPI_NAND_CMD_PAGE_READ = 0x13,
	/** Read data from NAND cache */
	SPI_NAND_CMD_READ_CACHE = 0x03,
	/** Write memory contents to NAND cache */
	SPI_NAND_CMD_PROGRAM_LOAD = 0x02,
	/** Copy data from NAND cache to main storage */
	SPI_NAND_CMD_PROGRAM_EXECUTE = 0x10,
	/** Erase a single block in main storage */
	SPI_NAND_CMD_BLOCK_ERASE = 0xD8,
	/** Get device configuration */
	SPI_NAND_CMD_GET_FEATURE = 0x0F,
	/** Set device configuration */
	SPI_NAND_CMD_SET_FEATURE = 0x1F,
	/** Read 2 byte device identifier */
	SPI_NAND_CMD_READ_ID = 0x9F,
	/** Reset memory device into known state */
	SPI_NAND_CMD_RESET = 0xFF,
};

/** Get/Set feature address */
enum spi_nand_feature {
	/** Block protection configuration */
	SPI_NAND_FEATURE_ADDR_BLOCK_PROT = 0xA0,
	/** General device configuration */
	SPI_NAND_FEATURE_ADDR_CONFIG = 0xB0,
	/** Device status flags */
	SPI_NAND_FEATURE_ADDR_STATUS = 0xC0,
};

enum spi_nand_feature_block_prot {
	/**  Bit definitions differ between chips, but a value of 0 unlocks all blocks */
	SPI_NAND_FEATURE_BLOCK_PROT_DISABLE_ALL = 0x00,
};

/* Micron:
 * BIT6 BIT5 BIT4  State
 *    0    0    0  No errors
 *    0    0    1  Errors detected and corrected
 *    0    1    0  Errors detected but not corrected
 *    0    1    1  Errors detected and corrected. Refresh recommended
 *    1    0    1  Errors detected and corrected. Refresh necessary
 * Macronix:
 * BIT5 BIT4  State
 *    0    0  No errors
 *    0    1  Errors detected and corrected
 *    1    0  Errors detected but not corrected
 *    1    1  Errors detected and corrected. Refresh recommended
 */
enum spi_nand_feature_status {
	/** Operation in progress */
	SPI_NAND_FEATURE_STATUS_OIP = BIT(0),
	/** Write enable latch */
	SPI_NAND_FEATURE_STATUS_WEL = BIT(1),
	/** Block erase operation failed */
	SPI_NAND_FEATURE_STATUS_ERASE_FAIL = BIT(2),
	/** Page program operation failed */
	SPI_NAND_FEATURE_STATUS_PROGRAM_FAIL = BIT(3),
	/** No ECC errors */
	SPI_NAND_FEATURE_ECC_NO_ERRORS = 0x00,
	/** ECC errors detected and corrected */
	SPI_NAND_FEATURE_ECC_ERROR_CORRECTED = BIT(4),
	/** ECC errors detected and NOT corrected */
	SPI_NAND_FEATURE_ECC_ERROR_NOT_CORRECTED = BIT(5),
	/** ECC errors detected, corrected and data should be refreshed */
	SPI_NAND_FEATURE_ECC_ERROR_CORRECTED_REFRESH = BIT(4) | BIT(5),
	/** Mask for the common ECC status bits */
	SPI_NAND_FEATURE_ECC_MASK = BIT(4) | BIT(5),
	/** Read page to cache operation is executing */
	SPI_NAND_FEATURE_STATUS_CACHE_BUSY = BIT(7),
};

/* Micron:
 * BIT7 BIT6 BIT1  State
 *    0    0    0  Normal Operation
 *    0    0    1  Access to permanent block protect status read mode
 *    0    1    0  Access OTP area/Parameter/Unique ID
 *    1    1    0  Access to OTP data protection bit to lock OTP area
 *    1    0    1  Access to SPI NOR read protocol enable mod
 *    1    1    1  Access to permanent block lock protection disable mode
 * Macronix:
 * BIT7 BIT6  State
 *    0    0  Normal Operation
 *    0    1  Access OTP area/Parameter/Unique ID
 *    1    0  Invalid
 *    1    1  Secure OTP Protection by using the Program Execution
 */
enum spi_nand_feature_config {
	/** Common behaviour when set standalone */
	SPI_NAND_FEATURE_CONFIG_OTP_EN = BIT(6),
	/** On-die ECC is enabled (Not documented by Macronix, but present in their driver) */
	SPI_NAND_FEATURE_CONFIG_ECC_EN = BIT(4),
};

/* ONFI 5.2, Revision 1.0, Section 6.7.1 */
struct spi_nand_onfi_parameter_page {
	/*
	 * Revision information and features block
	 */

	/** Must be bytes {'O', 'N', 'F', 'I'} */
	uint8_t signature[4];
	/** ONFI version support */
	uint16_t revision_number;
	/** Features supported */
	uint16_t feature_support;
	/** Optional commands supported */
	uint16_t optional_commands;
	/** ONFI-JEDEC JTG primary advanced command support */
	uint8_t advanced_command_support;
	/** Training commands supported (Field 0) */
	uint8_t training_command_support0;
	/** Extended parameter page length */
	uint16_t extended_parameter_page_length;
	/** Number of parameter pages */
	uint8_t num_parameter_pages;
	/** Training commands supported (Field 1) */
	uint8_t training_command_support1;
	/* Reserved for future use (Block 0) */
	uint8_t reserved0[16];

	/*
	 * Manufacturer information block
	 */

	/** Device manufacturer (12 ASCII characters) */
	char device_manufacturer[12];
	/** Device model (20 ASCII characters) */
	char device_model[20];
	/** JEDEC manufacturer ID */
	uint8_t jedec_manufacturer_id;
	uint16_t date_code;
	/* Reserved for future use (Block 1) */
	uint8_t reserved1[13];

	/*
	 * Memory organization block
	 */

	/** Number of data bytes per page */
	uint32_t data_bytes_per_page;
	/** Number of spare bytes per page */
	uint16_t spare_bytes_per_page;
	/* Reserved for future use (Block 2) */
	uint8_t reserved2[6];
	/** Number of pages per block */
	uint32_t pages_per_block;
	/** Number of blocks per logical unit (LUN) */
	uint32_t blocks_per_lun;
	/** Number of logical units (LUNs) */
	uint8_t num_lun;
	/** Number of address cycles (4-7: Column cycles, 0-3 Row cycles) */
	uint8_t address_cycles;
	/** Number of bits per cell */
	uint8_t bits_per_cell;
	/** Bad blocks maximum per LUN */
	uint16_t bad_blocks_per_lun;
	/** Block endurance */
	uint16_t block_endurance;
	/** Guaranteed valid blocks at beginning of target */
	uint8_t beginning_blocks_valid;
	/** Block endurance for guaranteed valid blocks */
	uint16_t beginning_blocks_endurance;
	/** Number of programs per page */
	uint8_t programs_per_page;
	/* Reserved for future use (Block 3) */
	uint8_t reserved3[1];
	/** Number of bits ECC correctability */
	uint8_t ecc_correctability;
	/** Number of plane address bits */
	uint8_t plane_address_bits;
	/** Multi-plane operation attributes */
	uint8_t multi_plane_attributes;
	/* Reserved for future use (Block 4) */
	uint8_t reserved4[1];
	/** NV-DDR3 timing mode support */
	uint16_t ddr3_timing_support;
	/** NV-LPDDR4 timing mode support */
	uint32_t lpddr4_timing_support;
	/* Reserved for future use (Block 5) */
	uint8_t reserved5[6];

	/*
	 * Electrical parameters block
	 */

	/** Blob of electrical parameters */
	uint8_t electrical_parameters[36];

	/*
	 * Vendor block
	 */

	/** Vendor specific Revision number */
	uint16_t vendor_revision;
	/** Vendor specific information  */
	uint8_t vendor_info[88];

	/*
	 * Validity block
	 */

	uint16_t integrity_crc;
} __packed;

BUILD_ASSERT(sizeof(struct spi_nand_onfi_parameter_page) == 256);

#endif /*__SPI_NAND_H__*/
