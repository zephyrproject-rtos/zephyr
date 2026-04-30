/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_RTS5817_QSPI_H_
#define ZEPHYR_DRIVERS_FLASH_RTS5817_QSPI_H_

#define SPI_C_MODE0                   0x00
#define SPI_CA_MODE0                  0x01
#define SPI_CDO_MODE0                 0x02
#define SPI_CDI_MODE0                 0x03
#define SPI_CADO_MODE0                0x04
#define SPI_CADI_MODE0                0x05
#define SPI_POLLING_MODE0             0x06
#define SPI_FAST_READ_MODE            0x07
#define SPI_FAST_READ_DUAL_OUT_MODE   0x08
#define SPI_FAST_READ_DUAL_INOUT_MODE 0x09
#define SPI_FAST_READ_QUAD_OUT_MODE   0x0A
#define SPI_FAST_READ_QUAD_INOUT_MODE 0x0B

/* common op code */
#define SF_WREN                 0x06
#define SF_WRDI                 0x04
#define SF_RDSR                 0x05
#define SF_RDSR_H               0x35
#define SF_WRSR                 0x01
#define SF_WRSR_H               0x31
#define SF_READ                 0x03
#define SF_FAST_READ            0x0B
#define SF_FAST_READ_DUAL_OUT   0x3B
#define SF_FAST_READ_DUAL_INOUT 0xBB
#define SF_FAST_READ_QUAD_OUT   0x6B
#define SF_FAST_READ_QUAD_INOUT 0xEB
#define SF_PAGE_PROG            0x02
#define SF_CHIP_ERASE           0xC7
#define SF_SECT_ERASE           0x20
#define SF_BLOCK_ERASE          0xD8
#define SF_READID               0x9F
#define SF_READ_UID             0x4B
#define SF_READ_UID_MXIC        0x5A
#define SF_PWR_DOWN             0xB9
#define SF_RELS_PDWN            0xAB
#define SF_VSRWE                0x50

/* store support flash nums& flash table */
#define SUPPORT_FLASH_NUM 15

typedef enum {
	GD25Q80E = 0xC84014,
	XT25F08B = 0x0B4014,
	XT25F16F = 0x0B4015,
	MX25V8035 = 0xC22314,
	MX25R8035 = 0xC22814,
	FM25W04 = 0xA12813,
	FM25W16 = 0xA12815,
	GD25Q16 = 0xC84015,
	GD25Q32 = 0xC84016,
	P25Q16SH = 0x856015,
	P25Q32SH = 0x856016,
	W25Q32JVSIQ = 0xEF4016,
	W25Q16JVSIQ = 0xEF4015,
	ZG25VQ32D = 0x5E4016,
	ZG25VQ16D = 0x5E4015,
	UNKNOWN = 0x000000, /* flash do not support */
	NOFLASH = 0xFFFFFF,
} device_id_t;

/*
 * SREG1_R1_W1 status register 1 byte ,read status step 1, write status step 1
 * SREG2_R2_W1 status register 2 byte ,read status step 2, write status step 1
 * SREG2_R2_W2 status register 2 byte ,read status step 2, write status step 2
 */
#define SREG1_R1_W1 1
#define SREG2_R2_W1 2
#define SREG2_R2_W2 3

typedef struct {
	device_id_t device_id;
	uint8_t str_rw_type;     /* status register  operation */
	uint8_t dummy_cycle;     /* default read dummy cycle */
	uint8_t location_qe;     /* Quad enable bit location */
	uint8_t sr_wren_command; /* write enable command for write sr, if flash support 0x50 (write
				  * enable for volatile status register) command, select SF_EWSR,
				  * else select SF_WREN
				  */
	uint32_t flash_size;     /* flash size (bytes) */
} spi_flash_params;

static const spi_flash_params spi_flash_params_table[SUPPORT_FLASH_NUM + 1] = {
	{GD25Q80E, SREG2_R2_W1, 0x07, 9, SF_VSRWE, 0x100000},
	{XT25F08B, SREG2_R2_W1, 0x07, 9, SF_VSRWE, 0x100000},
	{XT25F16F, SREG2_R2_W1, 0x07, 9, SF_VSRWE, 0x200000},
	{MX25V8035, SREG1_R1_W1, 0x07, 6, SF_WREN, 0x100000},
	{MX25R8035, SREG1_R1_W1, 0x07, 6, SF_WREN, 0x100000},
	{FM25W04, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x80000},
	{FM25W16, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x200000},
	{GD25Q16, SREG2_R2_W1, 0x07, 9, SF_VSRWE, 0x200000},
	{GD25Q32, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x400000},
	{P25Q16SH, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x200000},
	{P25Q32SH, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x400000},
	{W25Q32JVSIQ, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x200000},
	{W25Q16JVSIQ, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x400000},
	{ZG25VQ32D, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x200000},
	{ZG25VQ16D, SREG2_R2_W2, 0x07, 9, SF_VSRWE, 0x400000},
	{UNKNOWN, SREG1_R1_W1, 0x07, 0, SF_WREN, 0x1000000},
};

#endif /* ZEPHYR_DRIVERS_FLASH_RTS5817_QSPI_H_ */
