/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_NPCX_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_NPCX_FLASH_API_EX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>

enum flash_npcx_ex_ops {
	/*
	 * NPCX User Mode Access (UMA) mode execution.
	 *
	 * Execute a SPI transaction via User Mode Access (UMA) mode. Users can
	 * perform a customized SPI transaction to gread or write the device's
	 * configuration such as status registers of nor flash, power on/off,
	 * and so on.
	 */
	FLASH_NPCX_EX_OP_EXEC_UMA = FLASH_EX_OP_VENDOR_BASE,
	/*
	 * NPCX Configure specific operation for Quad-SPI nor flash.
	 *
	 * It configures specific operation for Quad-SPI nor flash such as lock
	 * or unlock UMA mode, set write protection pin of internal flash, and
	 * so on.
	 */
	FLASH_NPCX_EX_OP_SET_QSPI_OPER,
	/*
	 * NPCX Get specific operation for Quad-SPI nor flash.
	 *
	 * It returns current specific operation for Quad-SPI nor flash.
	 */
	FLASH_NPCX_EX_OP_GET_QSPI_OPER,
};

/* Structures used by FLASH_NPCX_EX_OP_EXEC_UMA */
struct npcx_ex_ops_uma_in {
	uint8_t opcode;
	uint8_t *tx_buf;
	size_t  tx_count;
	uint32_t addr;
	size_t  addr_count;
	size_t rx_count;
};

struct npcx_ex_ops_uma_out {
	uint8_t *rx_buf;
};

/* Structures used by FLASH_NPCX_EX_OP_SET_QSPI_OPER */
struct npcx_ex_ops_qspi_oper_in {
	bool enable;
	uint32_t mask;
};

/* Structures used by FLASH_NPCX_EX_OP_GET_QSPI_OPER */
struct npcx_ex_ops_qspi_oper_out {
	uint32_t oper;
};

/* Specific NPCX QSPI devices control bits */
#define NPCX_EX_OP_LOCK_UMA	BIT(0) /* Lock/Unlock UMA mode */
#define NPCX_EX_OP_INT_FLASH_WP BIT(1) /* Issue write protection of internal flash */

#ifdef __cplusplus
}
#endif

#endif /* __ZEPHYR_INCLUDE_DRIVERS_NPCX_FLASH_API_EX_H__ */
