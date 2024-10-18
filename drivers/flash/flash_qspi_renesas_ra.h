/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_QSPI_RENESAS_RA_H_
#define ZEPHYR_DRIVERS_FLASH_QSPI_RENESAS_RA_H_

#include <zephyr/drivers/flash.h>
#include <api/r_spi_flash_api.h>
#include <zephyr/drivers/flash/ra_flash_api_extensions.h>

#define QSPI_CMD_RESET_EN       (0x66) /* Reset Enable */
#define QSPI_CMD_RESET_MEM      (0x99) /* Reset Memory */
#define Ex_SPI_CMD_READ_ID      (0x9F) /* Extended Spi Read JEDEC ID */
#define QSPI_CMD_READ_ID        (0xAF) /* Qpi Read JEDEC ID */
#define QSPI_CMD_READ_SFDP      (0x5A) /* Read SFDP */
#define QSPI_CMD_WRITE_ENABLE   (0x06) /* Write Enable command */
#define QSPI_CMD_READ_STATUS    (0x05)
#define QSPI_CMD_EXIT_QPI_MODE  (0xF5) /* QPI mode exit command */
#define QSPI_CMD_ENTER_QPI_MODE (0x35) /* QPI mode entry command */
#define QSPI_CMD_PAGE_PROGRAM   (0x02) /* Page Program command */
#define QSPI_CMD_XIP_ENTER      (0x20) /* XIP Enter command */
#define QSPI_CMD_XIP_EXIT       (0xFF) /* XIP Exit command */
#define WRITE_STATUS_BIT        0
#if defined(CONFIG_SOC_SERIES_RA6E2)
#define STATUS_REG_PAYLOAD {0x01, 0x00}
#define SET_SREG_VALUE     (0x00)
#else
#define STATUS_REG_PAYLOAD {0x01, 0x40, 0x00}
#define SET_SREG_VALUE     (0x40)
#endif
/* one byte data transfer */
#define ONE_BYTE             (1)
#define THREE_BYTE           (3)
#define FOUR_BYTE            (4)
#define RESET_VALUE          (0x00)
/* default memory value */
#define QSPI_DEFAULT_MEM_VAL (0xFF)
#define QSPI0_NODE           DT_INST_PARENT(0)
#define RA_QSPI_NOR_NODE     DT_INST(0, renesas_ra_qspi_nor)
#define QSPI_WRITE_BLK_SZ    DT_PROP(RA_QSPI_NOR_NODE, write_block_size)
#define QSPI_ERASE_BLK_SZ    DT_PROP(RA_QSPI_NOR_NODE, erase_block_size)
/* QSPI flash page Size */
#define PAGE_WRITE_SIZE      (256U)
#define PAGE_SIZE_BYTE       (256U)
/* sector size of QSPI flash device */
#define BLOCK_SIZE_4K        (4096U)
#define BLOCK_SIZE_32K       (32678U)
#define BLOCK_SIZE_64K       (65536U)
/* QSPI flash address through page*/
#define QSPI_FLASH_ADDRESS(page_no)                                                                \
	((uint8_t *)(QSPI_DEVICE_START_ADDRESS + ((page_no) * PAGE_WRITE_SIZE)))

/* Flash Size*/
#define QSPI_NOR_FLASH_SIZE DT_PROP(RA_QSPI_NOR_NODE, size)

struct qspi_flash_ra_data {
	struct st_qspi_instance_ctrl qspi_ctrl;
	struct st_spi_flash_cfg qspi_cfg;
	struct k_sem sem;
};
struct ra_qspi_nor_flash_config {
	const struct pinctrl_dev_config *pcfg;
};

static const spi_flash_erase_command_t g_qspi_erase_command_list[4] = {
	{.command = 0x20, .size = 4096},
	{.command = 0x52, .size = 32768},
	{.command = 0xD8, .size = 65536},
	{.command = 0xC7, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE},
};

#define ERASE_COMMAND_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif /* ZEPHYR_DRIVERS_FLASH_QSPI_RENESAS_RA_H_ */
