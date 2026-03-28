/*
 * Copyright 2025 The ChromiumOS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Andes QSPI-NOR XIP flash extended operations.
 * @ingroup andes_flash_xip_ex_op
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_ANDES_FLASH_XIP_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_ANDES_FLASH_XIP_API_EX_H__

/**
 * @brief Extended operations for Andes QSPI-NOR XIP flash.
 * @defgroup andes_flash_xip_ex_op Andes QSPI-NOR XIP
 * @ingroup flash_ex_op
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>

/**
 * @brief Enumeration for Andes flash extended operations.
 */
enum flash_andes_xip_ex_ops {
	/**
	 * @brief Get the three status registers (SR1, SR2, SR3).
	 *
	 * This operation reads the contents of the three status registers from the flash device.
	 *
	 * @param out Pointer to a @ref andes_xip_ex_ops_get_out structure to store the register
	 *            values.
	 */
	FLASH_ANDES_XIP_EX_OP_GET_STATUS_REGS = FLASH_EX_OP_VENDOR_BASE,
	/**
	 * @brief Set the three status registers (SR1, SR2, SR3).
	 *
	 * This operation writes new values to the status registers, applying a mask to modify only
	 * specific bits.
	 * This operation is protected by a software lock that can be controlled with
	 * @ref FLASH_ANDES_XIP_EX_OP_LOCK.
	 *
	 * @param in Pointer to a @ref andes_xip_ex_ops_set_in structure containing the values and
	 *           masks to write.
	 */
	FLASH_ANDES_XIP_EX_OP_SET_STATUS_REGS,
	/**
	 * @brief Set a software lock to prevent status register modification.
	 *
	 * This operation enables or disables a software lock that prevents the
	 * @ref FLASH_ANDES_XIP_EX_OP_SET_STATUS_REGS operation from executing.
	 *
	 * @param in Pointer to a @ref andes_xip_ex_ops_lock_in structure specifying whether to
	 *           enable or disable the lock.
	 */
	FLASH_ANDES_XIP_EX_OP_LOCK,
	/**
	 * @brief Get the current state of the software status register lock.
	 *
	 * @param out Pointer to a @ref andes_xip_ex_ops_lock_state_out structure to store the
	 *            current lock state.
	 */
	FLASH_ANDES_XIP_EX_OP_LOCK_STATE,
	/**
	 * @brief Set the SPI command for memory-mapped read mode.
	 *
	 * This operation configures the command used by the hardware for Execute-In-Place (XIP) or
	 * memory-mapped reads.
	 *
	 * @param in Pointer to a @ref andes_xip_ex_ops_mem_read_cmd_in structure specifying the
	 *           read command to use.
	 */
	FLASH_ANDES_XIP_EX_OP_MEM_READ_CMD,
};

/**
 * @brief SPI commands for memory-mapped read mode.
 *
 * These values represent the different SPI read commands that can be configured for memory-mapped
 * access using the @ref FLASH_ANDES_XIP_EX_OP_MEM_READ_CMD operation.
 * The command codes correspond to standard SPI flash read command opcodes.
 */
enum flash_andes_xip_mem_rd_cmd {
	FLASH_ANDES_XIP_MEM_RD_CMD_03 = 0,  /**< Normal Read (0x03) */
	FLASH_ANDES_XIP_MEM_RD_CMD_0B = 1,  /**< Fast Read (0x0B) */
	FLASH_ANDES_XIP_MEM_RD_CMD_3B = 2,  /**< Dual I/O Fast Read (0x3B) */
	FLASH_ANDES_XIP_MEM_RD_CMD_6B = 3,  /**< Quad I/O Fast Read (0x6B) */
	FLASH_ANDES_XIP_MEM_RD_CMD_BB = 4,  /**< Dual Output Fast Read (0xBB) */
	FLASH_ANDES_XIP_MEM_RD_CMD_EB = 5,  /**< Quad Output Fast Read (0xEB) */
	FLASH_ANDES_XIP_MEM_RD_CMD_13 = 8,  /**< Normal Read with 4-byte address (0x13) */
	FLASH_ANDES_XIP_MEM_RD_CMD_0C = 9,  /**< Fast Read with 4-byte address (0x0C) */
	FLASH_ANDES_XIP_MEM_RD_CMD_3C = 10, /**< Dual I/O Fast Read with 4-byte address (0x3C) */
	FLASH_ANDES_XIP_MEM_RD_CMD_6C = 11, /**< Quad I/O Fast Read with 4-byte address (0x6C) */
	FLASH_ANDES_XIP_MEM_RD_CMD_BC = 12, /**< Dual Output Fast Read with 4-byte address (0xBC) */
	FLASH_ANDES_XIP_MEM_RD_CMD_EC = 13, /**< Quad Output Fast Read with 4-byte address (0xEC) */
};

/**
 * @brief Output parameters for @ref FLASH_ANDES_XIP_EX_OP_GET_STATUS_REGS operation.
 */
struct andes_xip_ex_ops_get_out {
	/** Buffer for read status registers. */
	uint8_t regs[3];
};

/**
 * @brief Input parameters for @ref FLASH_ANDES_XIP_EX_OP_SET_STATUS_REGS operation.
 */
struct andes_xip_ex_ops_set_in {
	/** Status registers to write. */
	uint8_t regs[3];
	/** Mask of status registers to change. */
	uint8_t masks[3];
};

/**
 * @brief Input parameters for @ref FLASH_ANDES_XIP_EX_OP_LOCK operation.
 */
struct andes_xip_ex_ops_lock_in {
	/** Set to true to enable the lock, false to disable. */
	bool enable;
};

/**
 * @brief Output parameters for @ref FLASH_ANDES_XIP_EX_OP_LOCK_STATE operation.
 */
struct andes_xip_ex_ops_lock_state_out {
	/** Current lock state. */
	bool state;
};

/**
 * @brief Input parameters for @ref FLASH_ANDES_XIP_EX_OP_MEM_READ_CMD operation.
 */
struct andes_xip_ex_ops_mem_read_cmd_in {
	/** SPI command used for memory-mapped mode. */
	enum flash_andes_xip_mem_rd_cmd cmd;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __ZEPHYR_INCLUDE_DRIVERS_ANDES_FLASH_XIP_API_EX_H__ */
