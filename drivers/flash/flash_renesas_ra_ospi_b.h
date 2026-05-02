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
#include "spi_nor_s28hx512t.h"
#include "spi_nor.h"

/* Device node */
#define RA_OSPI_B_NOR_NODE DT_INST(0, renesas_ra_ospi_b_nor)

/* Flash erase value */
#define ERASE_VALUE (0xff)

/* Page size */
#define PAGE_SIZE_BYTE 64

/* Flash device sector size */
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

/* Configure flash device */
#define DATA_CFR2V_REGISTER          (0x83)
#define DATA_CFR3V_REGISTER          (0x40)
#define DATA_SET_SPI_CFR5V_REGISTER  (0x40)
#define DATA_SET_OSPI_CFR5V_REGISTER (0x43)

/* Flash device address space mapping */
#define APP_ADDRESS(sector_no)                                                                     \
	((uint8_t *)(BSP_FEATURE_OSPI_B_DEVICE_1_START_ADDRESS +                                   \
		     ((sector_no) * SPI_NOR_SECTOR_SIZE)))

/* Erase command */
static spi_flash_erase_command_t erase_command_list[] = {
	{.command = SPI_NOR_CMD_SE_4B, .size = SPI_NOR_SECTOR_SIZE},
	{.command = S28HX512T_SPI_NOR_CMD_SE_256KB, .size = SECTOR_SIZE_256K},
	{.command = S28HX512T_SPI_NOR_CMD_ERCHP, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE}};

static spi_flash_erase_command_t high_speed_erase_command_list[] = {
	{.command = S28HX512T_SPI_NOR_OCMD_SE_4KB, .size = SPI_NOR_SECTOR_SIZE},
	{.command = S28HX512T_SPI_NOR_OCMD_SE_256KB, .size = SECTOR_SIZE_256K},
	{.command = S28HX512T_SPI_NOR_OCMD_ERCHP, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE}};

/* Erase command length */
#define ERASE_COMMAND_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

static ospi_b_table_t const high_speed_erase_commands = {
	.p_table = &high_speed_erase_command_list,
	.length = ERASE_COMMAND_LENGTH(high_speed_erase_command_list),
};

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
				       .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR},
	[TRANSFER_WRITE_CFR2V_SPI] = {.command = S28HX512T_SPI_NOR_CMD_WR_WRARG,
				      .address = S28HX512T_SPI_NOR_CFR2V_ADDR,
				      .data = DATA_CFR2V_REGISTER,
				      .command_length = COMMAND_LENGTH_SPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_ONE,
				      .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR},
	[TRANSFER_WRITE_CFR3V_SPI] = {.command = S28HX512T_SPI_NOR_CMD_WR_WRARG,
				      .address = S28HX512T_SPI_NOR_CFR3V_ADDR,
				      .data = DATA_CFR3V_REGISTER,
				      .command_length = COMMAND_LENGTH_SPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_ONE,
				      .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR},
	[TRANSFER_WRITE_CFR5V_SPI] = {.command = S28HX512T_SPI_NOR_CMD_WR_WRARG,
				      .address = S28HX512T_SPI_NOR_CFR5V_ADDR,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_SPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_ONE,
				      .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR},
	[TRANSFER_READ_STATUS_SPI] = {.command = SPI_NOR_CMD_RDSR,
				      .address = ADDRESS_DUMMY,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_SPI,
				      .address_length = ADDRESS_LENGTH_ZERO,
				      .data_length = DATA_LENGTH_ONE,
				      .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_STATUS},
	[TRANSFER_READ_CFR2V_SPI] = {.command = S28HX512T_SPI_NOR_CMD_RREG,
				     .address = S28HX512T_SPI_NOR_CFR2V_ADDR,
				     .data = DATA_DUMMY,
				     .command_length = COMMAND_LENGTH_SPI,
				     .address_length = ADDRESS_LENGTH_FOUR,
				     .data_length = DATA_LENGTH_ONE,
				     .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_REG},
	[TRANSFER_READ_CFR3V_SPI] = {.command = S28HX512T_SPI_NOR_CMD_RREG,
				     .address = S28HX512T_SPI_NOR_CFR3V_ADDR,
				     .data = DATA_DUMMY,
				     .command_length = COMMAND_LENGTH_SPI,
				     .address_length = ADDRESS_LENGTH_FOUR,
				     .data_length = DATA_LENGTH_ONE,
				     .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_REG},
	[TRANSFER_READ_CFR5V_SPI] = {.command = S28HX512T_SPI_NOR_CMD_RREG,
				     .address = S28HX512T_SPI_NOR_CFR5V_ADDR,
				     .data = DATA_DUMMY,
				     .command_length = COMMAND_LENGTH_SPI,
				     .address_length = ADDRESS_LENGTH_FOUR,
				     .data_length = DATA_LENGTH_ONE,
				     .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_REG},
	[TRANSFER_READ_DEVICE_ID_SPI] = {.command = SPI_NOR_CMD_RDID,
					 .address = ADDRESS_DUMMY,
					 .data = DATA_DUMMY,
					 .command_length = COMMAND_LENGTH_SPI,
					 .address_length = ADDRESS_LENGTH_ZERO,
					 .data_length = DATA_LENGTH_FOUR,
					 .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_STATUS},
	[TRANSFER_READ_SFDP_ID_SPI] = {.command = S28HX512T_SPI_NOR_CMD_RSFDPID,
				       .address = ADDRESS_DUMMY,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_SPI,
				       .address_length = ADDRESS_LENGTH_THREE,
				       .data_length = DATA_LENGTH_EIGHT,
				       .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_SFDP},
	[TRANSFER_RESET_ENABLE_SPI] = {.command = SPI_NOR_CMD_RESET_EN,
				       .address = ADDRESS_DUMMY,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_SPI,
				       .address_length = ADDRESS_LENGTH_ZERO,
				       .data_length = DATA_LENGTH_ZERO,
				       .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR},
	[TRANSFER_RESET_MEM_SPI] = {.command = SPI_NOR_CMD_RESET_MEM,
				    .address = ADDRESS_DUMMY,
				    .data = DATA_DUMMY,
				    .command_length = COMMAND_LENGTH_SPI,
				    .address_length = ADDRESS_LENGTH_ZERO,
				    .data_length = DATA_LENGTH_ZERO,
				    .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR},
	/* Transfer structure for OPI mode */
	[TRANSFER_WRITE_ENABLE_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_WEN,
					.address = ADDRESS_DUMMY,
					.data = DATA_DUMMY,
					.command_length = COMMAND_LENGTH_OSPI,
					.address_length = ADDRESS_LENGTH_ZERO,
					.data_length = DATA_LENGTH_ZERO,
					.dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR_OCTAL},
	[TRANSFER_WRITE_CFR2V_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_WR_REG2,
				       .address = S28HX512T_SPI_NOR_CFR2V_ADDR,
				       .data = DATA_CFR2V_REGISTER,
				       .command_length = COMMAND_LENGTH_OSPI,
				       .address_length = ADDRESS_LENGTH_FOUR,
				       .data_length = DATA_LENGTH_TWO,
				       .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR_OCTAL},
	[TRANSFER_WRITE_CFR3V_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_WR_REG2,
				       .address = S28HX512T_SPI_NOR_CFR3V_ADDR,
				       .data = DATA_CFR3V_REGISTER,
				       .command_length = COMMAND_LENGTH_OSPI,
				       .address_length = ADDRESS_LENGTH_FOUR,
				       .data_length = DATA_LENGTH_TWO,
				       .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR_OCTAL},
	[TRANSFER_WRITE_CFR5V_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_WR_REG2,
				       .address = S28HX512T_SPI_NOR_CFR5V_ADDR,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_OSPI,
				       .address_length = ADDRESS_LENGTH_FOUR,
				       .data_length = DATA_LENGTH_TWO,
				       .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR_OCTAL},
	[TRANSFER_READ_STATUS_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_RSR,
				       .address = ADDRESS_DUMMY,
				       .data = DATA_DUMMY,
				       .command_length = COMMAND_LENGTH_OSPI,
				       .address_length = ADDRESS_LENGTH_FOUR,
				       .data_length = DATA_LENGTH_TWO,
				       .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_STATUS_OCTAL},
	[TRANSFER_READ_CFR2V_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_RSR,
				      .address = S28HX512T_SPI_NOR_CFR2V_ADDR,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_OSPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_TWO,
				      .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_REG_OCTAL},
	[TRANSFER_READ_CFR3V_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_RSR,
				      .address = S28HX512T_SPI_NOR_CFR3V_ADDR,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_OSPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_TWO,
				      .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_REG_OCTAL},
	[TRANSFER_READ_CFR5V_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_RREG,
				      .address = S28HX512T_SPI_NOR_CFR5V_ADDR,
				      .data = DATA_DUMMY,
				      .command_length = COMMAND_LENGTH_OSPI,
				      .address_length = ADDRESS_LENGTH_FOUR,
				      .data_length = DATA_LENGTH_TWO,
				      .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_REG_OCTAL},
	[TRANSFER_READ_DEVICE_ID_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_RDID,
					  .address = ADDRESS_DUMMY,
					  .data = DATA_DUMMY,
					  .command_length = COMMAND_LENGTH_OSPI,
					  .address_length = ADDRESS_LENGTH_FOUR,
					  .data_length = DATA_LENGTH_FOUR,
					  .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_STATUS_OCTAL},
	[TRANSFER_READ_SFDP_ID_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_RSFDPID,
					.address = ADDRESS_DUMMY,
					.data = DATA_DUMMY,
					.command_length = COMMAND_LENGTH_OSPI,
					.address_length = ADDRESS_LENGTH_FOUR,
					.data_length = DATA_LENGTH_EIGHT,
					.dummy_cycles = S28HX512T_SPI_NOR_DUMMY_RD_SFDP_OCTAL},
	[TRANSFER_RESET_ENABLE_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_RST_EN,
					.address = ADDRESS_DUMMY,
					.data = DATA_DUMMY,
					.command_length = COMMAND_LENGTH_OSPI,
					.address_length = ADDRESS_LENGTH_ZERO,
					.data_length = DATA_LENGTH_ZERO,
					.dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR_OCTAL},
	[TRANSFER_RESET_MEM_OSPI] = {.command = S28HX512T_SPI_NOR_OCMD_RST_MEM,
				     .address = ADDRESS_DUMMY,
				     .data = DATA_DUMMY,
				     .command_length = COMMAND_LENGTH_OSPI,
				     .address_length = ADDRESS_LENGTH_ZERO,
				     .data_length = DATA_LENGTH_ZERO,
				     .dummy_cycles = S28HX512T_SPI_NOR_DUMMY_WR_OCTAL},
};

#endif /* ZEPHYR_DRIVERS_FLASH_RENESAS_RA_OSPI_B_H_ */
