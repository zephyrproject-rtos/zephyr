/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for NPCM flash controller extended operations.
 * @ingroup npcm_flash_ex_op
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_NPCM_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_NPCM_FLASH_API_EX_H__

/**
 * @brief Extended operations for NPCM flash controllers.
 * @defgroup npcm_flash_ex_op NPCM
 * @ingroup flash_ex_op
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>

/** @brief NPCM flash extended operations. */
enum flash_npcm_ex_ops {
	/**
	 * NPCM specific transceive execution.
	 *
	 * Execute a SPI transaction through flash controller. Users can
	 * perform a customized SPI transaction to read or write the device's
	 * configuration such as status registers of nor flash, power on/off,
	 * and so on.
	 */
	FLASH_NPCM_EX_OP_EXEC_TRANSCEIVE = FLASH_EX_OP_VENDOR_BASE,
	/**
	 * NPCM Configure specific operation for Quad-SPI nor flash.
	 *
	 * It configures specific operation for Quad-SPI nor flash such as lock
	 * or unlock UMA mode, set write protection pin of internal flash, and
	 * so on.
	 */
	FLASH_NPCM_EX_OP_SET_QSPI_OPER,
	/**
	 * NPCM Get specific operation for Quad-SPI nor flash.
	 *
	 * It returns current specific operation for Quad-SPI nor flash.
	 */
	FLASH_NPCM_EX_OP_GET_QSPI_OPER,
	/**
	 * NPCM Set 4-byte addressing mode for flash devices.
	 *
	 * Configure flash controller to enable or disable 4-byte addressing
	 * mode. This operation synchronizes both the flash chip addressing mode
	 * and the hardware controller registers to ensure DRA (Direct Read
	 * Access) operations work correctly in the selected addressing mode.
	 */
	FLASH_NPCM_EX_OP_SET_4B_MODE,
	/**
	 * NPCM Initialize CRC/Checksum calculation.
	 *
	 * Configure the FIU CRC engine with mode (CRC32/Checksum), data source
	 * (firmware/flash), and initial value.
	 */
	FLASH_NPCM_EX_OP_CRC_INIT,
	/**
	 * NPCM Add data to CRC/Checksum calculation.
	 *
	 * Feed data into the CRC engine for calculation. Can be called multiple
	 * times to process data in chunks.
	 */
	FLASH_NPCM_EX_OP_CRC_ADD_DATA,
	/**
	 * NPCM Get CRC/Checksum result.
	 *
	 * Read the final CRC32 or Checksum result from the FIU hardware.
	 */
	FLASH_NPCM_EX_OP_CRC_GET_RESULT,
};

/** @brief CRC/Checksum modes. */
enum npcm_crc_mode {
	/** CRC32 calculation */
	NPCM_CRC_MODE_CRC32 = 0,
	/** Checksum calculation */
	NPCM_CRC_MODE_CHECKSUM = 1,
};

/** @brief CRC/Checksum data sources. */
enum npcm_crc_source {
	/** Data from firmware */
	NPCM_CRC_SRC_FIRMWARE = 0,
	/** Data from flash via UMA */
	NPCM_CRC_SRC_FLASH = 1,
};

/** Input structure used by #FLASH_NPCM_EX_OP_EXEC_TRANSCEIVE */
struct npcm_ex_ops_transceive_in {
	/** SPI command opcode */
	uint8_t opcode;
	/** Transmit data buffer */
	uint8_t *tx_buf;
	/** Number of bytes to transmit */
	size_t tx_count;
	/** Address field */
	uint32_t addr;
	/** Number of address bytes */
	size_t addr_count;
	/** Number of bytes to receive */
	size_t rx_count;
};

/** Output structure used by #FLASH_NPCM_EX_OP_EXEC_TRANSCEIVE */
struct npcm_ex_ops_transceive_out {
	/** Receive data buffer */
	uint8_t *rx_buf;
};

/** Input structure used by #FLASH_NPCM_EX_OP_SET_QSPI_OPER */
struct npcm_ex_ops_qspi_oper_in {
	/** true to set the operations in @a mask, false to clear them */
	bool enable;
	/** Bit mask of the operations (NPCM_EX_OP_*) to be configured */
	uint32_t mask;
};

/** Output structure used by #FLASH_NPCM_EX_OP_GET_QSPI_OPER */
struct npcm_ex_ops_qspi_oper_out {
	/** Current bit mask of the active operations (NPCM_EX_OP_*) */
	uint32_t oper;
};

/** Input structure used by #FLASH_NPCM_EX_OP_SET_4B_MODE */
struct npcm_ex_ops_set_4b_mode_in {
	/** true = enable 4-byte mode, false = disable (3-byte mode) */
	bool enable;
};

/** Input structure used by #FLASH_NPCM_EX_OP_CRC_INIT */
struct npcm_ex_ops_crc_init_in {
	/** CRC32 or Checksum */
	enum npcm_crc_mode mode;
	/** Firmware or Flash */
	enum npcm_crc_source source;
	/** Initial CRC value: 0xFFFFFFFF for CRC32, 0 for Checksum */
	uint32_t initial_value;
};

/** Input structure used by #FLASH_NPCM_EX_OP_CRC_ADD_DATA */
struct npcm_ex_ops_crc_add_data_in {
	/** Pointer to data buffer */
	const uint8_t *data;
	/** Number of bytes to process */
	size_t length;
};

/** Output structure used by #FLASH_NPCM_EX_OP_CRC_GET_RESULT */
struct npcm_ex_ops_crc_result_out {
	/** CRC32 or Checksum result */
	uint32_t result;
};

/** Lock/Unlock transceive */
#define NPCM_EX_OP_LOCK_TRANSCEIVE BIT(0)
/** Issue write protection of internal flash */
#define NPCM_EX_OP_INT_FLASH_WP    BIT(1)
/** Issue write protection of external flash */
#define NPCM_EX_OP_EXT_FLASH_WP    BIT(2)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __ZEPHYR_INCLUDE_DRIVERS_NPCM_FLASH_API_EX_H__ */
