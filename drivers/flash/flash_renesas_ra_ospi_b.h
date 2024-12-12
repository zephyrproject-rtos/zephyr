/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_RENESAS_RA_OSPI_B_H_
#define ZEPHYR_DRIVERS_FLASH_RENESAS_RA_OSPI_B_H_

#include <zephyr/drivers/flash.h>
#include <zephyr/dt-bindings/flash_controller/xspi.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <r_spi_flash_api.h>
#include <r_ospi_b.h>
#include "spi_nor.h"

/* Device node */
#define RA_OSPI_B_NOR_NODE DT_INST(0, renesas_ra_ospi_b_nor)

/* Flash size, Write size */
#define FLASH_SIZE   DT_PROP(RA_OSPI_B_NOR_NODE, size)
#define WRITE_BLK_SZ DT_PROP(RA_OSPI_B_NOR_NODE, write_block_size)

/* Protocol Mode */
#define PROTOCOL_MODE DT_PROP(RA_OSPI_B_NOR_NODE, protocol_mode)

/* Data Rate */
#define DATA_RATE DT_PROP(RA_OSPI_B_NOR_NODE, data_rate)

/* Max frequency */
#define MAX_FREQUENCY DT_PROP(RA_OSPI_B_NOR_NODE, ospi_max_frequency)

/* Flash erase value */
#define ERASE_VALUE (0xff)

/* Page size */
#define PAGE_SIZE_BYTE 64

/* Flash device sector size */
#define SECTOR_SIZE_4K   (0x1000)
#define SECTOR_SIZE_128K (0x20000)
#define SECTOR_SIZE_256K (0x40000)

/* Flash device timing */
#define TIME_ERASE_256K (16000)
#define TIME_ERASE_4K   (1000U)
#define TIME_WRITE      (1000U)

/* Bit status */
#define WRITE_STATUS_BIT (0)
#define WRITE_ENABLE_BIT (1)

/* Calibration sector */
#define SECTOR_THREE (2U)

/* Flash opcodes */
#define SPI_NOR_CMD_WR_REG2  (0x71) /* Write config register 2 */
#define SPI_NOR_CMD_RSFDPID  (0x5A) /* Read SFDP ID */
#define SPI_NOR_CMD_RREG     (0x65) /* Read Any Register */
#define SPI_NOR_CMD_SE_4KB   (0x21) /* Sector Erase 4Kb address */
#define SPI_NOR_CMD_SE_256KB (0xDC) /* Sector Erase 128Kb and 256Kb address */
#define SPI_NOR_CMD_DE       (0x60) /* Chip Erase */

/* Flash octal opcodes */
#define SPI_NOR_OCMD_WEN      (0x0606) /* Octal Write enable */
#define SPI_NOR_OCMD_RSR      (0x0505) /* Octal Read status register */
#define SPI_NOR_OCMD_WR_REG2  (0x7171) /* Octal Write config register 2 */
#define SPI_NOR_OCMD_RDID     (0x9F9F) /* Octal Read JEDEC ID */
#define SPI_NOR_OCMD_RSFDPID  (0x5A5A) /* Octal Read SFDP ID */
#define SPI_NOR_OCMD_RREG     (0x6565) /* Octal Read Any Register */
#define SPI_NOR_OCMD_PP_4B    (0x1212) /* Octal Page Program 4 Byte Address */
#define SPI_NOR_OCMD_READ     (0xEEEE) /* Octal Read data */
#define SPI_NOR_OCMD_RST_EN   (0x6666) /* Octal Reset Enable */
#define SPI_NOR_OCMD_RST_MEM  (0x9999) /* Reset Memory */
#define SPI_NOR_OCMD_SE_4KB   (0x2121) /* Octal Sector Erase 4Kb address */
#define SPI_NOR_OCMD_SE_256KB (0xDCDC) /* Octal Sector Erase 128Kb and 256Kb address */
#define SPI_NOR_OCMD_DE       (0x6060) /* Octal Chip Erase */

/* Command length */
#define COMMAND_LENGTH_SPI  (1U)
#define COMMAND_LENGTH_OSPI (2U)

/* Transfer address length */
#define ADDRESS_DUMMY        (0U)
#define ADDRESS_LENGTH_ZERO  (0U)
#define ADDRESS_LENGTH_THREE (3U)
#define ADDRESS_LENGTH_FOUR  (4U)

/* Transfer data length */
#define DATA_DUMMY        (0U)
#define DATA_LENGTH_ZERO  (0U)
#define DATA_LENGTH_ONE   (1U)
#define DATA_LENGTH_TWO   (2U)
#define DATA_LENGTH_FOUR  (4U)
#define DATA_LENGTH_EIGHT (8U)

/* Transfer dummy cycles */
#define DUMMY_CYCLE_WRITE_SPI          (0U)
#define DUMMY_CYCLE_WRITE_OSPI         (0U)
#define DUMMY_CYCLE_READ_STATUS_SPI    (0U)
#define DUMMY_CYCLE_READ_STATUS_OSPI   (4U)
#define DUMMY_CYCLE_READ_REGISTER_SPI  (1U)
#define DUMMY_CYCLE_READ_REGISTER_OSPI (4U)
#define DUMMY_CYCLE_READ_MEMORY_SPI    (3U)
#define DUMMY_CYCLE_READ_SFDP_SPI      (8U)
#define DUMMY_CYCLE_READ_SFDP_OSPI     (8U)
#define DUMMY_CYCLE_READ_MEMORY_OSPI   (10U)

/* Flash device register address */
#define ADDRESS_CFR1V_REGISTER (0x00800002)
#define ADDRESS_CFR2V_REGISTER (0x00800003)
#define ADDRESS_CFR3V_REGISTER (0x00800004)
#define ADDRESS_CFR4V_REGISTER (0x00800005)
#define ADDRESS_CFR5V_REGISTER (0x00800006)

/* Configure flash device */
#define DATA_CFR2V_REGISTER          (0x83)
#define DATA_CFR3V_REGISTER          (0x40)
#define DATA_SET_SPI_CFR5V_REGISTER  (0x40)
#define DATA_SET_OSPI_CFR5V_REGISTER (0x43)

/* Flash device address space mapping */
#define APP_ADDRESS(sector_no)                                                                     \
	((uint8_t *)(BSP_FEATURE_OSPI_B_DEVICE_1_START_ADDRESS + ((sector_no) * SECTOR_SIZE_4K)))

/* Flash device status bit */
#define WEN_BIT_MASK  (0x00000002)
#define BUSY_BIT_MASK (0x00000001)

/* Erase command */
static const spi_flash_erase_command_t erase_command_list[] = {
	{.command = SPI_NOR_CMD_SE_4KB, .size = SECTOR_SIZE_4K},
	{.command = SPI_NOR_CMD_SE_256KB, .size = SECTOR_SIZE_256K},
	{.command = SPI_NOR_CMD_DE, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE}};

static const spi_flash_erase_command_t high_speed_erase_command_list[] = {
	{.command = SPI_NOR_OCMD_SE_4KB, .size = SECTOR_SIZE_4K},
	{.command = SPI_NOR_OCMD_SE_256KB, .size = SECTOR_SIZE_256K},
	{.command = SPI_NOR_OCMD_DE, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE}};

/* Erase command length */
#define ERASE_COMMAND_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Reset value */
#define RESET_VALUE (0x00)

/* Transfer table */
typedef enum e_transfer {
	TRANSFER_WRITE_ENABLE_SPI = 0,
	TRANSFER_WRITE_CFR2V_SPI,
	TRANSFER_WRITE_CFR3V_SPI,
	TRANSFER_WRITE_CFR5V_SPI,
	TRANSFER_READ_STATUS_SPI,
	TRANSFER_READ_CFR2V_SPI,
	TRANSFER_READ_CFR3V_SPI,
	TRANSFER_READ_CFR5V_SPI,
	TRANSFER_READ_DEVICE_ID_SPI,
	TRANSFER_READ_SFDP_ID_SPI,
	TRANSFER_RESET_ENABLE_SPI,
	TRANSFER_RESET_MEM_SPI,

	TRANSFER_WRITE_ENABLE_OSPI,
	TRANSFER_WRITE_CFR2V_OSPI,
	TRANSFER_WRITE_CFR3V_OSPI,
	TRANSFER_WRITE_CFR5V_OSPI,
	TRANSFER_READ_STATUS_OSPI,
	TRANSFER_READ_CFR2V_OSPI,
	TRANSFER_READ_CFR3V_OSPI,
	TRANSFER_READ_CFR5V_OSPI,
	TRANSFER_READ_DEVICE_ID_OSPI,
	TRANSFER_READ_SFDP_ID_OSPI,
	TRANSFER_RESET_ENABLE_OSPI,
	TRANSFER_RESET_MEM_OSPI,
	TRANSFER_MAX
} transfer_t;

spi_flash_direct_transfer_t direct_transfer[TRANSFER_MAX] = {
	/* Transfer structure for SPI mode */
	[TRANSFER_WRITE_ENABLE_SPI] = {.command = SPI_NOR_CMD_WREN,
				       .address = ADDRESS_DUMMY,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_SPI,
				       .address_length = ADDRESS_LENGTH_ZERO,
				       .data_length = DATA_LENGTH_ZERO,
				       .dummy_cycles = DUMMY_CYCLE_WRITE_SPI},
	[TRANSFER_WRITE_CFR2V_SPI] = {.command = SPI_NOR_CMD_WR_REG2,
				      .address = ADDRESS_CFR2V_REGISTER,
				      .data = DATA_CFR2V_REGISTER,
				      .command_length = COMMAND_LENGTH_SPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_ONE,
				      .dummy_cycles = DUMMY_CYCLE_WRITE_SPI},
	[TRANSFER_WRITE_CFR3V_SPI] = {.command = SPI_NOR_CMD_WR_REG2,
				      .address = ADDRESS_CFR3V_REGISTER,
				      .data = DATA_CFR3V_REGISTER,
				      .command_length = COMMAND_LENGTH_SPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_ONE,
				      .dummy_cycles = DUMMY_CYCLE_WRITE_SPI},
	[TRANSFER_WRITE_CFR5V_SPI] = {.command = SPI_NOR_CMD_WR_REG2,
				      .address = ADDRESS_CFR5V_REGISTER,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_SPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_ONE,
				      .dummy_cycles = DUMMY_CYCLE_WRITE_SPI},
	[TRANSFER_READ_STATUS_SPI] = {.command = SPI_NOR_CMD_RDSR,
				      .address = ADDRESS_DUMMY,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_SPI,
				      .address_length = ADDRESS_LENGTH_ZERO,
				      .data_length = DATA_LENGTH_ONE,
				      .dummy_cycles = DUMMY_CYCLE_READ_STATUS_SPI},
	[TRANSFER_READ_CFR2V_SPI] = {.command = SPI_NOR_CMD_RREG,
				     .address = ADDRESS_CFR2V_REGISTER,
				     .data = DATA_DUMMY,
				     .command_length = COMMAND_LENGTH_SPI,
				     .address_length = ADDRESS_LENGTH_FOUR,
				     .data_length = DATA_LENGTH_ONE,
				     .dummy_cycles = DUMMY_CYCLE_READ_REGISTER_SPI},
	[TRANSFER_READ_CFR3V_SPI] = {.command = SPI_NOR_CMD_RREG,
				     .address = ADDRESS_CFR3V_REGISTER,
				     .data = DATA_DUMMY,
				     .command_length = COMMAND_LENGTH_SPI,
				     .address_length = ADDRESS_LENGTH_FOUR,
				     .data_length = DATA_LENGTH_ONE,
				     .dummy_cycles = DUMMY_CYCLE_READ_REGISTER_SPI},
	[TRANSFER_READ_CFR5V_SPI] = {.command = SPI_NOR_CMD_RREG,
				     .address = ADDRESS_CFR5V_REGISTER,
				     .data = DATA_DUMMY,
				     .command_length = COMMAND_LENGTH_SPI,
				     .address_length = ADDRESS_LENGTH_FOUR,
				     .data_length = DATA_LENGTH_ONE,
				     .dummy_cycles = DUMMY_CYCLE_READ_REGISTER_SPI},
	[TRANSFER_READ_DEVICE_ID_SPI] = {.command = SPI_NOR_CMD_RDID,
					 .address = ADDRESS_DUMMY,
					 .data = DATA_DUMMY,
					 .command_length = COMMAND_LENGTH_SPI,
					 .address_length = ADDRESS_LENGTH_ZERO,
					 .data_length = DATA_LENGTH_FOUR,
					 .dummy_cycles = DUMMY_CYCLE_READ_STATUS_SPI},
	[TRANSFER_READ_SFDP_ID_SPI] = {.command = SPI_NOR_CMD_RSFDPID,
				       .address = ADDRESS_DUMMY,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_SPI,
				       .address_length = ADDRESS_LENGTH_THREE,
				       .data_length = DATA_LENGTH_EIGHT,
				       .dummy_cycles = DUMMY_CYCLE_READ_SFDP_SPI},
	[TRANSFER_RESET_ENABLE_SPI] = {.command = SPI_NOR_CMD_RESET_EN,
				       .address = ADDRESS_DUMMY,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_SPI,
				       .address_length = ADDRESS_LENGTH_ZERO,
				       .data_length = DATA_LENGTH_ZERO,
				       .dummy_cycles = DUMMY_CYCLE_WRITE_SPI},
	[TRANSFER_RESET_MEM_SPI] = {.command = SPI_NOR_CMD_RESET_MEM,
				    .address = ADDRESS_DUMMY,
				    .data = DATA_DUMMY,
				    .command_length = COMMAND_LENGTH_SPI,
				    .address_length = ADDRESS_LENGTH_ZERO,
				    .data_length = DATA_LENGTH_ZERO,
				    .dummy_cycles = DUMMY_CYCLE_WRITE_SPI},
	/* Transfer structure for OPI mode */
	[TRANSFER_WRITE_ENABLE_OSPI] = {.command = SPI_NOR_OCMD_WEN,
					.address = ADDRESS_DUMMY,
					.data = DATA_DUMMY,
					.command_length = COMMAND_LENGTH_OSPI,
					.address_length = ADDRESS_LENGTH_ZERO,
					.data_length = DATA_LENGTH_ZERO,
					.dummy_cycles = DUMMY_CYCLE_WRITE_OSPI},
	[TRANSFER_WRITE_CFR2V_OSPI] = {.command = SPI_NOR_OCMD_WR_REG2,
				       .address = ADDRESS_CFR2V_REGISTER,
				       .data = DATA_CFR2V_REGISTER,
				       .command_length = COMMAND_LENGTH_OSPI,
				       .address_length = ADDRESS_LENGTH_FOUR,
				       .data_length = DATA_LENGTH_TWO,
				       .dummy_cycles = DUMMY_CYCLE_WRITE_OSPI},
	[TRANSFER_WRITE_CFR3V_OSPI] = {.command = SPI_NOR_OCMD_WR_REG2,
				       .address = ADDRESS_CFR3V_REGISTER,
				       .data = DATA_CFR3V_REGISTER,
				       .command_length = COMMAND_LENGTH_OSPI,
				       .address_length = ADDRESS_LENGTH_FOUR,
				       .data_length = DATA_LENGTH_TWO,
				       .dummy_cycles = DUMMY_CYCLE_WRITE_OSPI},
	[TRANSFER_WRITE_CFR5V_OSPI] = {.command = SPI_NOR_OCMD_WR_REG2,
				       .address = ADDRESS_CFR5V_REGISTER,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_OSPI,
				       .address_length = ADDRESS_LENGTH_FOUR,
				       .data_length = DATA_LENGTH_TWO,
				       .dummy_cycles = DUMMY_CYCLE_WRITE_OSPI},
	[TRANSFER_READ_STATUS_OSPI] = {.command = SPI_NOR_OCMD_RSR,
				       .address = ADDRESS_DUMMY,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_OSPI,
				       .address_length = ADDRESS_LENGTH_FOUR,
				       .data_length = DATA_LENGTH_TWO,
				       .dummy_cycles = DUMMY_CYCLE_READ_STATUS_OSPI},
	[TRANSFER_READ_CFR2V_OSPI] = {.command = SPI_NOR_OCMD_RSR,
				      .address = ADDRESS_CFR2V_REGISTER,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_OSPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_TWO,
				      .dummy_cycles = DUMMY_CYCLE_READ_REGISTER_OSPI},
	[TRANSFER_READ_CFR3V_OSPI] = {.command = SPI_NOR_OCMD_RSR,
				      .address = ADDRESS_CFR3V_REGISTER,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_OSPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_TWO,
				      .dummy_cycles = DUMMY_CYCLE_READ_REGISTER_OSPI},
	[TRANSFER_READ_CFR5V_OSPI] = {.command = SPI_NOR_OCMD_RREG,
				      .address = ADDRESS_CFR5V_REGISTER,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_OSPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_TWO,
				      .dummy_cycles = DUMMY_CYCLE_READ_REGISTER_OSPI},
	[TRANSFER_READ_DEVICE_ID_OSPI] = {.command = SPI_NOR_OCMD_RDID,
					  .address = ADDRESS_DUMMY,
					  .data = DATA_DUMMY,
					  .command_length = COMMAND_LENGTH_OSPI,
					  .address_length = ADDRESS_LENGTH_FOUR,
					  .data_length = DATA_LENGTH_FOUR,
					  .dummy_cycles = DUMMY_CYCLE_READ_STATUS_OSPI},
	[TRANSFER_READ_SFDP_ID_OSPI] = {.command = SPI_NOR_OCMD_RSFDPID,
					.address = ADDRESS_DUMMY,
					.data = DATA_DUMMY,
					.command_length = COMMAND_LENGTH_OSPI,
					.address_length = ADDRESS_LENGTH_FOUR,
					.data_length = DATA_LENGTH_EIGHT,
					.dummy_cycles = DUMMY_CYCLE_READ_SFDP_OSPI},
	[TRANSFER_RESET_ENABLE_OSPI] = {.command = SPI_NOR_OCMD_RST_EN,
					.address = ADDRESS_DUMMY,
					.data = DATA_DUMMY,
					.command_length = COMMAND_LENGTH_OSPI,
					.address_length = ADDRESS_LENGTH_ZERO,
					.data_length = DATA_LENGTH_ZERO,
					.dummy_cycles = DUMMY_CYCLE_WRITE_OSPI},
	[TRANSFER_RESET_MEM_OSPI] = {.command = SPI_NOR_OCMD_RST_MEM,
				     .address = ADDRESS_DUMMY,
				     .data = DATA_DUMMY,
				     .command_length = COMMAND_LENGTH_OSPI,
				     .address_length = ADDRESS_LENGTH_ZERO,
				     .data_length = DATA_LENGTH_ZERO,
				     .dummy_cycles = DUMMY_CYCLE_WRITE_OSPI},
};

#define ZEPHYR_ERROR(err)                                                                          \
	{                                                                                          \
		if (err != 0) {                                                                    \
			err = -EIO;                                                                \
			break;                                                                     \
		}                                                                                  \
	}

#endif /* ZEPHYR_DRIVERS_FLASH_RENESAS_RA_OSPI_B_H_ */
