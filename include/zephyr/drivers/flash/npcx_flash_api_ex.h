/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for NPCX flash extended operations.
 * @ingroup npcx_flash_ex_op
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_NPCX_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_NPCX_FLASH_API_EX_H__

/**
 * @brief Extended operations for NPCX flash controllers.
 * @defgroup npcx_flash_ex_op NPCX
 * @ingroup flash_ex_op
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>

/**
 * @brief Enumeration for NPCX flash extended operations.
 */
enum flash_npcx_ex_ops {
	/**
	 * User Mode Access (UMA) mode execution.
	 *
	 * Execute a SPI transaction via User Mode Access (UMA) mode. Users can
	 * perform a customized SPI transaction to read or write the device's
	 * configuration such as status registers of nor flash, power on/off,
	 * and so on.
	 *
	 * @param in Pointer to a @ref npcx_ex_ops_uma_in structure specifying the
	 *           UMA transaction.
	 * @param out Pointer to a @ref npcx_ex_ops_uma_out structure to store the
	 *            result of the UMA transaction.
	 */
	FLASH_NPCX_EX_OP_EXEC_UMA = FLASH_EX_OP_VENDOR_BASE,
	/**
	 * Configure specific operation for Quad-SPI nor flash.
	 *
	 * It configures specific operation for Quad-SPI nor flash such as lock
	 * or unlock UMA mode, set write protection pin of internal flash, and
	 * so on.
	 *
	 * @param in Pointer to a @ref npcx_ex_ops_qspi_oper_in structure specifying the
	 *           operation to set.
	 */
	FLASH_NPCX_EX_OP_SET_QSPI_OPER,
	/**
	 * Get specific operation for Quad-SPI nor flash.
	 *
	 * It returns current specific operation for Quad-SPI nor flash.
	 *
	 * @param out Pointer to a @ref npcx_ex_ops_qspi_oper_out structure to store the
	 *            result of the operation.
	 */
	FLASH_NPCX_EX_OP_GET_QSPI_OPER,
};

/**
 * @brief Input parameters for @ref FLASH_NPCX_EX_OP_EXEC_UMA operation.
 *
 * Defines the content of a UMA transaction.
 */
struct npcx_ex_ops_uma_in {
	uint8_t opcode;    /**< SPI opcode (command byte). */
	uint8_t *tx_buf;   /**< Pointer to transmit buffer (may be NULL). */
	size_t tx_count;   /**< Number of bytes to transmit. */
	uint32_t addr;     /**< Address for address phase. */
	size_t addr_count; /**< Number of address bytes (0–4). */
	size_t rx_count;   /**< Number of bytes expected to be read. */
};

/**
 * @brief Output parameters for @ref FLASH_NPCX_EX_OP_EXEC_UMA operation.
 *
 * Defines where received data is stored.
 */
struct npcx_ex_ops_uma_out {
	uint8_t *rx_buf; /**< Pointer to receive buffer (must be large enough). */
};

/**
 * @brief Input parameters for @ref FLASH_NPCX_EX_OP_SET_QSPI_OPER operation.
 *
 * Used to enable or disable specific NPCX Quad-SPI operations.
 */
struct npcx_ex_ops_qspi_oper_in {
	bool enable;   /**< True to enable, false to disable. */
	uint32_t mask; /**< Mask of operations to configure. */
};

/**
 * @brief Output parameters for @ref FLASH_NPCX_EX_OP_GET_QSPI_OPER operation.
 *
 * Used to report the current QSPI operation state.
 */
struct npcx_ex_ops_qspi_oper_out {
	uint32_t oper; /**< Bitfield of currently active operations. */
};

/**
 * @name NPCX QSPI operation control bits
 * @{
 */
#define NPCX_EX_OP_LOCK_UMA	BIT(0) /**< Lock/Unlock UMA mode */
#define NPCX_EX_OP_INT_FLASH_WP BIT(1) /**< Issue write protection of internal flash */
/** @} */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __ZEPHYR_INCLUDE_DRIVERS_NPCX_FLASH_API_EX_H__ */
