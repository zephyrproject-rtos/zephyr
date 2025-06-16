/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_NCT_FLASH_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_NCT_FLASH_API_EX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>

enum flash_nct_ex_ops {
	/*
	 * NCT specific transceive execution.
	 *
	 * Execute a SPI transaction through flash controller. Users can
	 * perform a customized SPI transaction to gread or write the device's
	 * configuration such as status registers of nor flash, power on/off,
	 * and so on.
	 */
	FLASH_NCT_EX_OP_EXEC_TRANSCEIVE = FLASH_EX_OP_VENDOR_BASE,
	/*
	 * NCT Configure specific operation for Quad-SPI nor flash.
	 *
	 * It configures specific operation for Quad-SPI nor flash such as lock
	 * or unlock UMA mode, set write protection pin of internal flash, and
	 * so on.
	 */
	FLASH_NCT_EX_OP_SET_QSPI_OPER,
	/*
	 * NCT Get specific operation for Quad-SPI nor flash.
	 *
	 * It returns current specific operation for Quad-SPI nor flash.
	 */
	FLASH_NCT_EX_OP_GET_QSPI_OPER,
};

/* Structures used by FLASH_NCT_EX_OP_EXEC_TRANSCEIVE */
struct nct_ex_ops_transceive_in {
	uint8_t opcode;
	uint8_t *tx_buf;
	size_t  tx_count;
	uint32_t addr;
	size_t  addr_count;
	size_t rx_count;
};

struct nct_ex_ops_transceive_out {
	uint8_t *rx_buf;
};

/* Structures used by FLASH_NCT_EX_OP_SET_QSPI_OPER */
struct nct_ex_ops_qspi_oper_in {
	bool enable;
	uint32_t mask;
};

/* Structures used by FLASH_NCT_EX_OP_GET_QSPI_OPER */
struct nct_ex_ops_qspi_oper_out {
	uint32_t oper;
};

/* Specific NCT QSPI devices control bits */
#define NCT_EX_OP_LOCK_TRANSCEIVE		BIT(0) /* Lock/Unlock transceive */
#define NCT_EX_OP_INT_FLASH_WP 		BIT(1) /* Issue write protection of internal flash */
#define NCT_EX_OP_EXT_FLASH_WP 		BIT(2) /* Issue write protection of external flash */
#define NCT_EX_OP_EXT_FLASH_SPIP_WP	BIT(3) /* Issue write protection of external flash */

#ifdef __cplusplus
}
#endif

#endif /* __ZEPHYR_INCLUDE_DRIVERS_NCT_FLASH_API_EX_H__ */
