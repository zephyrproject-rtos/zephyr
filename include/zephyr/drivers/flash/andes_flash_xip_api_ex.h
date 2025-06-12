/*
 * Copyright 2025 The ChromiumOS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_ANDES_FLASH_XIP_API_EX_H__
#define __ZEPHYR_INCLUDE_DRIVERS_ANDES_FLASH_XIP_API_EX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>

enum flash_andes_xip_ex_ops {
	FLASH_ANDES_XIP_EX_OP_GET_STATUS_REGS = FLASH_EX_OP_VENDOR_BASE,
	FLASH_ANDES_XIP_EX_OP_SET_STATUS_REGS,
	FLASH_ANDES_XIP_EX_OP_LOCK,
	FLASH_ANDES_XIP_EX_OP_LOCK_STATE,
	FLASH_ANDES_XIP_EX_OP_MEM_READ_CMD,
};

enum flash_andes_xip_mem_rd_cmd {
	FLASH_ANDES_XIP_MEM_RD_CMD_03 = 0,
	FLASH_ANDES_XIP_MEM_RD_CMD_0B = 1,
	FLASH_ANDES_XIP_MEM_RD_CMD_3B = 2,
	FLASH_ANDES_XIP_MEM_RD_CMD_6B = 3,
	FLASH_ANDES_XIP_MEM_RD_CMD_BB = 4,
	FLASH_ANDES_XIP_MEM_RD_CMD_EB = 5,
	FLASH_ANDES_XIP_MEM_RD_CMD_13 = 8,
	FLASH_ANDES_XIP_MEM_RD_CMD_0C = 9,
	FLASH_ANDES_XIP_MEM_RD_CMD_3C = 10,
	FLASH_ANDES_XIP_MEM_RD_CMD_6C = 11,
	FLASH_ANDES_XIP_MEM_RD_CMD_BC = 12,
	FLASH_ANDES_XIP_MEM_RD_CMD_EC = 13,
};

/* Structure used by FLASH_ANDES_XIP_EX_OP_GET_STATUS_REGS */
struct andes_xip_ex_ops_get_out {
	/* Buffer for read status registers. */
	uint8_t regs[3];
};

/* Structure used by FLASH_ANDES_XIP_EX_OP_SET_STATUS_REGS */
struct andes_xip_ex_ops_set_in {
	/* Status registers to write. */
	uint8_t regs[3];
	/* Mask of status registers to change. */
	uint8_t masks[3];
};

/* Structures used by FLASH_ANDES_XIP_EX_OP_LOCK */
struct andes_xip_ex_ops_lock_in {
	bool enable;
};

/* Structures used by FLASH_ANDES_XIP_EX_OP_LOCK_STATE */
struct andes_xip_ex_ops_lock_state_out {
	bool state;
};

/* Structures used by FLASH_ANDES_XIP_EX_OP_MEM_READ_CMD */
struct andes_xip_ex_ops_mem_read_cmd_in {
	/* SPI command used for memory-mapped mode. */
	enum flash_andes_xip_mem_rd_cmd cmd;
};

#ifdef __cplusplus
}
#endif

#endif /* __ZEPHYR_INCLUDE_DRIVERS_ANDES_FLASH_XIP_API_EX_H__ */
