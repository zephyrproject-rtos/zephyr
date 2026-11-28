/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_RTS5817_QSPI_H_
#define ZEPHYR_DRIVERS_FLASH_RTS5817_QSPI_H_

#include <stdint.h>
#include "spi_nor.h"

#define RTS5817_QSPI_XFER_CMD_ONLY           0x00
#define RTS5817_QSPI_XFER_CMD_ADDR           0x01
#define RTS5817_QSPI_XFER_CMD_DATA_OUT       0x02
#define RTS5817_QSPI_XFER_CMD_DATA_IN        0x03
#define RTS5817_QSPI_XFER_CMD_ADDR_DATA_OUT  0x04
#define RTS5817_QSPI_XFER_CMD_ADDR_DATA_IN   0x05
#define RTS5817_QSPI_XFER_POLL_STATUS        0x06
#define RTS5817_QSPI_XFER_FAST_READ          0x07
#define RTS5817_QSPI_XFER_FAST_READ_DUAL_OUT 0x08
#define RTS5817_QSPI_XFER_FAST_READ_DUAL_IO  0x09
#define RTS5817_QSPI_XFER_FAST_READ_QUAD_OUT 0x0A
#define RTS5817_QSPI_XFER_FAST_READ_QUAD_IO  0x0B

/* Opcodes not defined in spi_nor.h */
#define RTS5817_FLASH_CMD_READ_UID         0x4B /* Read unique ID */
#define RTS5817_FLASH_CMD_VOLATILE_SR_WREN 0x50 /* Write enable for volatile status register */

enum rts5817_flash_id {
	RTS5817_FLASH_ID_XT25F16F = 0x0B4015,
	RTS5817_FLASH_ID_FM25W16 = 0xA12815,
	RTS5817_FLASH_ID_GD25Q16 = 0xC84015,
	RTS5817_FLASH_ID_P25Q16SH = 0x856015,
	RTS5817_FLASH_ID_W25Q16JVSIQ = 0xEF4015,
	RTS5817_FLASH_ID_ZG25VQ16D = 0x5E4015,
	RTS5817_FLASH_ID_UNKNOWN = 0x000000, /* flash not supported */
	RTS5817_FLASH_ID_NOFLASH = 0xFFFFFF,
};

/*
 * RTS5817_STATUS_REG_DUAL_COMBINED_WRITE: 2 status register bytes, read in 2 steps, write in 1 step
 * RTS5817_STATUS_REG_DUAL_SPLIT_WRITE: 2 status register bytes, read in 2 steps, write in 2 steps
 */
enum rts5817_status_reg_access {
	RTS5817_STATUS_REG_DUAL_COMBINED_WRITE = 1,
	RTS5817_STATUS_REG_DUAL_SPLIT_WRITE = 2,
};

struct rts5817_spi_flash_params {
	/* 3-byte JEDEC ID returned by the read ID command (0x9F). */
	enum rts5817_flash_id jedec_id;
	enum rts5817_status_reg_access status_reg_access;
	/* Controller encoding of the read dummy cycles: the number of cycles minus one. */
	uint8_t dummy_cycles_reg;
	/* Quad enable bit index: 0 to 7 in status register 1, 8 to 15 in status register 2. */
	uint8_t quad_enable_bit;
	/* Write enable command for a volatile status register write; non-volatile writes use
	 * SPI_NOR_CMD_WREN instead.
	 */
	uint8_t volatile_status_reg_wren_cmd;
	/* The flash returns a unique ID for the READ_UID command (0x4B). */
	bool supports_uid;
	/* Flash size in bytes. */
	uint32_t flash_size;
};

#endif /* ZEPHYR_DRIVERS_FLASH_RTS5817_QSPI_H_ */
