/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MARLIN3_HAL_SFC_HAL_H
#define __MARLIN3_HAL_SFC_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"
#include "hal_sfc_phy.h"

#define CMD_READ_ID     0x9f
#define CMD_RSTEN       0x66
#define CMD_RST         0x99
#define CMD_PE_SUSPEND          0x75
#define CMD_PE_RESUME           0x7A
#define CMD_ENTER_QPI           0x38
#define CMD_EXIT_QPI            0xFF
#define CMD_WRITE_DISABLE       0x04
#define CMD_WRITE_ENABLE        0x06
#define CMD_NORMAL_READ     0x03
#define CMD_FAST_READ           0x0B
#define CMD_READ_1_1_2          0x3B
#define CMD_READ_1_1_4          0x6B
#define CMD_2IO_READ            0xBB
#define CMD_4IO_READ            0xEB

#define CMD_WRITE_STATUS        0x01
#define CMD_READ_STATUS1        0x05
#define CMD_READ_STATUS2        0x35

#define CMD_PAGE_PROGRAM        0x02

#define CMD_SECTOR_ERASE        0x20
#define CMD_CHIP_ERASE          0xC7
#define CMD_SETBURST            0xC0

#define STATUS_WIP              (1 << 0)
#define STATUS_WEL              (1 << 1)

#define CREATE_CMD_(cmd_desc, _cmd, _byte_len, _cmd_mode, _bit_mode) \
	    create_cmd(&(cmd_desc), _cmd, _byte_len, _cmd_mode, _bit_mode, SEND_MODE_0)
#define CREATE_CMD_REV(cmd_desc, _cmd, _byte_len, _cmd_mode, _bit_mode) \
	    create_cmd(&(cmd_desc),  _cmd, _byte_len, _cmd_mode, _bit_mode, SEND_MODE_1)

#define SPIFLASH_SIZE_2MB		0x00200000
#define SPIFLASH_SIZE_4MB		0x00400000
#define SPIFLASH_SIZE_8MB		0x00800000
#define SPIFLASH_SIZE_16MB		0x01000000
#define SPIFLASH_SIZE_32MB		0x02000000

#define SPIFLASH_SIZE_16Mbit		SPIFLASH_SIZE_2MB
#define SPIFLASH_SIZE_32Mbit		SPIFLASH_SIZE_4MB
#define SPIFLASH_SIZE_64Mbit		SPIFLASH_SIZE_8MB
#define SPIFLASH_SIZE_128Mbit	SPIFLASH_SIZE_16MB
#define SPIFLASH_SIZE_256Mbit	SPIFLASH_SIZE_32MB

#define SPIFLASH_VOLTAGE_3V		1
#define SPIFLASH_VOLTAGE_1V8	0

#define SPI_MODE					0x0
#define QPI_MODE				0x1

#define ADDR_24BIT				0x0
#define ADDR_32BIT				0x1

#define DUMMY_0CLOCKS			0
#define DUMMY_2CLOCKS			1
#define DUMMY_4CLOCKS			2
#define DUMMY_6CLOCKS			3
#define DUMMY_8CLOCKS			3
#define DUMMY_10CLOCKS			3

#define READ_FREQ_104M			1
#define READ_FREQ_133M			2

#define CONFIG_SYS_HZ			1000
#define SPI_FLASH_PROG_TIMEOUT				(20 * CONFIG_SYS_HZ)
#define SPI_FLASH_PAGE_ERASE_TIMEOUT		(5 * CONFIG_SYS_HZ)
#define SPI_FLASH_SECTOR_ERASE_TIMEOUT	(10 * CONFIG_SYS_HZ)
#define SPI_FLASH_CHIP_ERASE_TIMEOUT		(125 * CONFIG_SYS_HZ)
#define SPI_FLASH_MAX_TIMEOUT				(200 * CONFIG_SYS_HZ)

#define SPI_FLASH_WEL_TIMEOUT				(2 * CONFIG_SYS_HZ)
#define SPI_FLASH_WIP_TIMEOUT				(2 * CONFIG_SYS_HZ)
#define SPI_FLASH_SFC_INT_TIMEOUT			(2 * CONFIG_SYS_HZ)

#define TIMEOUT_WRSR						(100)
#define TIMEOUT_WRVCR						(100)
#define TIMEOUT_WEVECR						(100)

#define CMD_READ_ARRAY_SLOW				0x03
#define CMD_READ_ARRAY_FAST				0x0B
#define CMD_READ_ARRAY_LEGACY				0xE8

#define RD_CMD_1BIT					(0x00 << 0)
#define RD_CMD_2BIT					(0x01 << 0)
#define RD_CMD_4BIT					(0x02 << 0)
#define RD_CMD_MSK					(0x03 << 0)

#define RD_ADDR_1BIT				(0x00 << 2)
#define RD_ADDR_2BIT				(0x01 << 2)
#define RD_ADDR_4BIT				(0x02 << 2)
#define RD_ADDR_MSK					(0x03 << 2)

#define RD_DUMY_1BIT				(0x00 << 4)
#define RD_DUMY_2BIT				(0x01 << 4)
#define RD_DUMY_4BIT				(0x02 << 4)
#define RD_DUMY_MSK					(0x03 << 4)

#define RD_DATA_1BIT				(0x00 << 6)
#define RD_DATA_2BIT				(0x01 << 6)
#define RD_DATA_4BIT				(0x02 << 6)
#define RD_DATA_MSK					(0x03 << 6)

#define WR_CMD_1BIT					(0x00 << 8)
#define WR_CMD_2BIT					(0x01 << 8)
#define WR_CMD_4BIT					(0x02 << 8)
#define WR_CMD_MSK					(0x03 << 8)

#define WR_ADDR_1BIT				(0x00 << 10)
#define WR_ADDR_2BIT				(0x01 << 10)
#define WR_ADDR_4BIT				(0x02 << 10)
#define WR_ADDR_MSK				(0x03 << 10)

#define WR_DATA_1BIT				(0x00 << 14)
#define WR_DATA_2BIT				(0x01 << 14)
#define WR_DATA_4BIT				(0x02 << 14)
#define WR_DATA_MSK				(0x03 << 14)

typedef enum lock_pattern {
	SFXXM_L0X00000000_0X001000 = 1,	/* 4K */
	SFXXM_L0X00000000_0X002000,	/* 8K */
	SFXXM_L0X00000000_0X004000,	/* 16K */
	SFXXM_L0X00000000_0X008000,	/* 32K */
	SFXXM_L0X00000000_0X010000,	/* 64K */

	SFXXM_L0X00000000_0X020000,	/* 128K */
	SFXXM_L0X00000000_0X040000,	/*256 K */
	SFXXM_L0X00000000_0X080000,	/* 512K */
	SFXXM_L0X00000000_0X100000,	/* 1MB */
	SFXXM_L0X00000000_0X200000,	/* 2MB */

	SFXXM_L0X00000000_0X300000,	/* 3MB + 1MB */
	SFXXM_L0X00000000_0X380000,	/* 3MB + 512KB */
	SFXXM_L0X00000000_0X3C0000,	/* 3MB + 512KB + 256KB */
	SFXXM_L0X00000000_0X3E0000,	/* 3MB + 512KB + 256KB +128KB */
	SFXXM_L0X00000000_0X3F0000,	/* 3MB + 512KB + 256KB +128KB + 64KB */

	SFXXM_L0X00000000_0X400000,	/* 4MB */
	SFXXM_L0X00000000_0X600000,	/* 4MB + 2MB */
	SFXXM_L0X00000000_0X700000,	/* 6MB + 1MB */
	SFXXM_L0X00000000_0X780000,	/* 7MB + 512KB */
	SFXXM_L0X00000000_0X7C0000,	/* 7MB + 512KB + 256KB */

	SFXXM_L0X00000000_0X7E0000,	/* 7MB + 512KB + 256KB + 128KB */
	SFXXM_L0X00000000_0X7F0000,	/* 7MB + 512KB + 256KB  + 128KB + 64KB */
	SFXXM_L0X00000000_0X800000,	/* 8MB */
	SFXXM_L0X00000000_0X1000000,	/* 16MB */
	SFXXM_L0X00000000_0X2000000,	/* 32MB/256Mbit */

	SFXXM_L0X00000000_0X4000000,	/* 64MB/512Mbit */
	SFXXM_L0X00000000_0X8000000,	/* 128MB/1024Mbit */

	/* Serial FLASH 32Mbit(4MB) Lock range. Start from Top */
	SF32M_L0X003FF000_0X001000,	/* 4K */
	SF32M_L0X003FE000_0X002000,	/* 8K */
	SF32M_L0X003FC000_0X004000,	/* 16K */

	SF32M_L0X003F8000_0X008000,	/* 32K */
	SF32M_L0X003F0000_0X010000,	/* 64K */
	SF32M_L0X003E0000_0X020000,	/* 128K */
	SF32M_L0X003C0000_0X040000,	/* 256K */
	SF32M_L0X00380000_0X080000,	/* 512K */

	SF32M_L0X00300000_0X100000,	/* 1MB */
	SF32M_L0X00200000_0X200000,	/* 2MB */
	SF32M_L0X00000000_0X400000,	/* 4MB, All Chip */

	/* Serial Flash 64Mbit(8MB) Lock range. Start from Top */
	SF64M_L0X007FF000_0X001000,	/* 4K */
	SF64M_L0X007FE000_0X002000,	/* 8K */

	SF64M_L0X007FC000_0X004000,	/* 16K */
	SF64M_L0X007F8000_0X008000,	/* 32K */
	SF64M_L0X007F0000_0X010000,	/* 64K */
	SF64M_L0X007E0000_0X020000,	/* 128K */
	SF64M_L0X007C0000_0X040000,	/* 256K */

	SF64M_L0X00780000_0X080000,	/* 512K */
	SF64M_L0X00700000_0X100000,	/* 1MB */
	SF64M_L0X00600000_0X200000,	/* 2MB */
	SF64M_L0X00400000_0X400000,	/* 4MB */
	SF64M_L0X00000000_0X800000,	/* 8MB, AllChip */

	/* Serial Flash 128Mbit(16MB) Lock range. Start from Top */
	SF128M_L0X00FFF000_0X001000,	/* 4K */
	SF128M_L0X00FFE000_0X002000,	/* 8K */
	SF128M_L0X00FFC000_0X004000,	/* 16K */
	SF128M_L0X00FF8000_0X008000,	/* 32K */
	SF128M_L0X00FF0000_0X010000,	/* 64K */

	SF128M_L0X00FE0000_0X020000,	/* 128k */
	SF128M_L0X00FC0000_0X040000,	/* 256k */
	SF128M_L0X00F80000_0X080000,	/* 512k */
	SF128M_L0X00F00000_0X100000,	/* 1M */
	SF128M_L0X00E00000_0X200000,	/* 2M */

	SF128M_L0X00C00000_0X400000,	/* 4M */
	SF128M_L0X00800000_0X800000,	/* 8M */
	SF128M_L0X00000000_0X1000000,	/* 16M */

	/* Serial Flash 256Mbit(32MB) Lock range. Start from Top */
	SF256M_L0X01FF0000_0X010000,	/* 64K */
	SF256M_L0X01FE0000_0X020000,	/* 128K */

	SF256M_L0X01FC0000_0X040000,	/* 256K */
	SF256M_L0X01F80000_0X080000,	/* 512K */
	SF256M_L0X01F00000_0X100000,	/* 1MB */
	SF256M_L0X01E00000_0X200000,	/* 2MB */
	SF256M_L0X01C00000_0X400000,	/* 4MB */

	SF256M_L0X01800000_0X800000,	/* 8MB */
	SF256M_L0X01000000_0X1000000,	/* 16MB */
	SF256M_L0X00000000_0X2000000,	/* 32MB */

	/* Serial Flash 256Mbit(32MB) Lock range. Start from Top */
	/* Example, winbond: WPS = 0, CMP = 1 */
	SFXXM_L0X00000000_0X02000000,	/* 32M, ALL CHIP */
	SFXXM_L0X00000000_0X01FF0000,	/* 32704KB */

	SFXXM_L0X00000000_0X01FE0000,	/* 32640KB */
	SFXXM_L0X00000000_0X01FC0000,	/* 32512KB */
	SFXXM_L0X00000000_0X01F80000,	/* 32256KB */
	SFXXM_L0X00000000_0X01F00000,	/* 31MB */
	SFXXM_L0X00000000_0X01E00000,	/* 30MB */

	SFXXM_L0X00000000_0X01C00000,	/* 28MB */
	SFXXM_L0X00000000_0X01800000,	/* 24MB */
	SFXXM_L0X00000000_0X01000000,	/* 16MB */
	SF256M_L0X00010000_0X01FF0000,	/* 32704KB */
	SF256M_L0X00020000_0X01FE0000,	/* 32640KB */

	SF256M_L0X00040000_0X01FC0000,	/* 32512KB */
	SF256M_L0X00080000_0X01F80000,	/* 32256KB */
	SF256M_L0X00100000_0X01F00000,	/* 31MB */
	SF256M_L0X00200000_0X01E00000,	/* 30MB */
	SF256M_L0X00400000_0X01C00000,	/* 28MB */

	SF256M_L0X00800000_0X01800000,	/* 24MB */
	SF256M_L0X01000000_0X01000000,	/* 16MB */

	/* Serial Flash 16Mbit(2MB) Lock range. Start from Top */
	SF16M_L0X001F0000_0X010000,	/* 64K */
	SF16M_L0X001E0000_0X020000,	/* 128K */
	SF16M_L0X001C0000_0X040000,	/* 256K */
	SF16M_L0X00180000_0X080000,	/* 512K */
	SF16M_L0X00100000_0X100000,	/* 1MB */
	SF16M_L0X001FF000_0X001000,	/* 4K */
	SF16M_L0X001FE000_0X002000,	/* 8K */
	SF16M_L0X001FC000_0X004000,	/* 16K */
	SF16M_L0X001F8000_0X008000,	/* 32K */

	LOCK_PATTERN_MAX,
} LOCK_PATTERN_E;

struct spi_flash_lock_desc {
	u8_t reg2_value;
	u8_t reg1_value;

	u8_t lock_pattern;
};

struct spi_flash_lock_pattern {
	u8_t lock_pattern;

	u32_t start_addr;
	u32_t size;
};

void spiflash_cmd_read(struct spi_flash *flash, const u8_t * cmd,
		       u32_t cmd_len, u32_t address, const void *data_in,
		       u32_t data_len);
void spiflash_cmd_write(struct spi_flash *flash, const u8_t * cmd,
			u32_t cmd_len, const void *data_out, u32_t data_len);
#define SR_QE		(1 << 9)
#define SR_LOCK		(0)
#define SR_UNLOCK	(0)

LOCK_PATTERN_E spiflash_get_lock_pattern(u32_t start_addr, u32_t size,
					 const struct spi_flash_lock_desc
					 *lock_table, u32_t lock_table_size);
BYTE_NUM_E spi_flash_addr(u32_t * addr, u32_t support_4addr);
__ramfunc int spiflash_cmd_poll_bit(struct spi_flash *flash,
	unsigned long timeout, u8_t cmd, u32_t poll_bit, u32_t bit_value);
int spiflash_cmd_wait_ready(struct spi_flash *flash, unsigned long timeout);
int spiflash_cmd_erase(struct spi_flash *flash, u8_t erase_cmd, u32_t offset);
int spiflash_cmd_program(struct spi_flash *flash, u32_t offset, u32_t len,
			 const void *buf, u8_t cmd);
int spiflash_cmd_spi_read(struct spi_flash *flash, u8_t cmd_read,
			  u8_t dummy_bytes);
int spiflash_cmd_qpi_read(struct spi_flash *flash, u8_t cmd_read,
			  u8_t dummy_bytes);

int spiflash_write_enable(struct spi_flash *flash);
int spiflash_write_disable(struct spi_flash *flash);
int spiflash_write(struct spi_flash *flash, u32_t offset, u32_t len,
		   const void *buf);
int spiflash_erase_chip(struct spi_flash *flash);
int spiflash_suspend(struct spi_flash *flash);
int spiflash_resume(struct spi_flash *flash);
int spiflash_reset(struct spi_flash *flash);
__ramfunc int spiflash_reset_anyway(void);

int spiflash_init(void);
struct spi_flash *flash_ctr(void);

//void spi_read(SFC_CMD_DES_T * cmd_des_ptr, u32_t cmd_len, u32_t * din);
//void spi_write(SFC_CMD_DES_T * cmd_des_ptr, u32_t cmd_len);
void spi_write_cmd(struct spi_flash *flash, const void *cmd,
		   unsigned int cmd_len);
void spi_data_read(struct spi_flash *flash, const u8_t * cmd,
		   u32_t cmd_len, u32_t address, const void *data_in,
		   u32_t data_len);
void spi_write_data(struct spi_flash *flash, const void *data_out,
		    u32_t data_len);
void spi_read_data(struct spi_flash *flash, void *data_in, u32_t data_len);
__ramfunc void create_cmd(SFC_CMD_DES_T *cmd_desc_ptr, u32_t cmd,
		u32_t byte_len, CMD_MODE_E cmd_mode,
		BIT_MODE_E bit_mode, SEND_MODE_E send_mode);
__ramfunc void spiflash_set_xip_cmd(struct spi_flash *flash,
		const u8_t *cmd_read, u8_t dummy_bytes);


#ifdef __cplusplus
}
#endif

#endif
