/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_NPCM_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_NPCM_FLASH_API_EX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>

enum flash_npcm_ex_ops {
	/*
	 * NPCM specific transceive execution.
	 *
	 * Execute a SPI transaction through flash controller. Users can
	 * perform a customized SPI transaction to gread or write the device's
	 * configuration such as status registers of nor flash, power on/off,
	 * and so on.
	 */
	FLASH_NPCM_EX_OP_EXEC_TRANSCEIVE = FLASH_EX_OP_VENDOR_BASE,
	/*
	 * NPCM Configure specific operation for Quad-SPI nor flash.
	 *
	 * It configures specific operation for Quad-SPI nor flash such as lock
	 * or unlock UMA mode, set write protection pin of internal flash, and
	 * so on.
	 */
	FLASH_NPCM_EX_OP_SET_QSPI_OPER,
	/*
	 * NPCM Get specific operation for Quad-SPI nor flash.
	 *
	 * It returns current specific operation for Quad-SPI nor flash.
	 */
	FLASH_NPCM_EX_OP_GET_QSPI_OPER,
	/*
	 * NPCM Set 4-byte addressing mode for flash devices.
	 *
	 * Configure flash controller to enable or disable 4-byte addressing
	 * mode. This operation synchronizes both the flash chip addressing mode
	 * and the hardware controller registers to ensure DRA (Direct Read Access)
	 * operations work correctly in the selected addressing mode.
	 */
	FLASH_NPCM_EX_OP_SET_4B_MODE,
	/*
	 * NPCM Initialize CRC/Checksum calculation.
	 *
	 * Configure the FIU CRC engine with mode (CRC32/Checksum), data source
	 * (firmware/flash), and initial value.
	 */
	FLASH_NPCM_EX_OP_CRC_INIT,
	/*
	 * NPCM Add data to CRC/Checksum calculation.
	 *
	 * Feed data into the CRC engine for calculation. Can be called multiple
	 * times to process data in chunks.
	 */
	FLASH_NPCM_EX_OP_CRC_ADD_DATA,
	/*
	 * NPCM Get CRC/Checksum result.
	 *
	 * Read the final CRC32 or Checksum result from the FIU hardware.
	 */
	FLASH_NPCM_EX_OP_CRC_GET_RESULT,
};

/* CRC/Checksum modes */
enum npcm_crc_mode {
	NPCM_CRC_MODE_CRC32 = 0,      /* CRC32 calculation */
	NPCM_CRC_MODE_CHECKSUM = 1,   /* Checksum calculation */
};

/* CRC/Checksum data sources */
enum npcm_crc_source {
	NPCM_CRC_SRC_FIRMWARE = 0,    /* Data from firmware */
	NPCM_CRC_SRC_FLASH = 1,       /* Data from flash via UMA */
};

/* Structures used by FLASH_NPCM_EX_OP_EXEC_TRANSCEIVE */
struct npcm_ex_ops_transceive_in {
	uint8_t opcode;
	uint8_t *tx_buf;
	size_t  tx_count;
	uint32_t addr;
	size_t  addr_count;
	size_t rx_count;
};

struct npcm_ex_ops_transceive_out {
	uint8_t *rx_buf;
};

/* Structures used by FLASH_NPCM_EX_OP_SET_QSPI_OPER */
struct npcm_ex_ops_qspi_oper_in {
	bool enable;
	uint32_t mask;
};

/* Structures used by FLASH_NPCM_EX_OP_GET_QSPI_OPER */
struct npcm_ex_ops_qspi_oper_out {
	uint32_t oper;
};

/* Structures used by FLASH_NPCM_EX_OP_SET_4B_MODE */
struct npcm_ex_ops_set_4b_mode_in {
	bool enable; /* true = enable 4-byte mode, false = disable (3-byte mode) */
};

/* Structures used by FLASH_NPCM_EX_OP_CRC_INIT */
struct npcm_ex_ops_crc_init_in {
	enum npcm_crc_mode mode;       /* CRC32 or Checksum */
	enum npcm_crc_source source;   /* Firmware or Flash */
	uint32_t initial_value;        /* Initial CRC value: 0xFFFFFFFF for CRC32, 0 for Checksum */
};

/* Structures used by FLASH_NPCM_EX_OP_CRC_ADD_DATA */
struct npcm_ex_ops_crc_add_data_in {
	const uint8_t *data;           /* Pointer to data buffer */
	size_t length;                  /* Number of bytes to process */
};

/* Structures used by FLASH_NPCM_EX_OP_CRC_GET_RESULT */
struct npcm_ex_ops_crc_result_out {
	uint32_t result;               /* CRC32 or Checksum result */
};

/* Specific NPCM QSPI devices control bits */
#define NPCM_EX_OP_LOCK_TRANSCEIVE	BIT(0) /* Lock/Unlock transceive */
#define NPCM_EX_OP_INT_FLASH_WP	BIT(1) /* Issue write protection of internal flash */
#define NPCM_EX_OP_EXT_FLASH_WP	BIT(2) /* Issue write protection of external flash */

#ifdef __cplusplus
}
#endif

#endif /* __ZEPHYR_INCLUDE_DRIVERS_NPCM_FLASH_API_EX_H__ */
