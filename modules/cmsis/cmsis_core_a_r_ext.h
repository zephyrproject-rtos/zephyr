/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CMSIS extension
 *
 * This header provides CMSIS-style register access functions and macros that
 * are not currently available in the CMSIS.
 *
 * NOTE: cmsis.h includes this file; do not manually include this file.
 */

#ifndef ZEPHYR_MODULES_CMSIS_CMSIS_A_R_EXT_H_
#define ZEPHYR_MODULES_CMSIS_CMSIS_A_R_EXT_H_

/* FSR Register Definitions */
#if defined(CONFIG_AARCH32_ARMV8_R)
#define FSR_FS_TRANSLATION_FAULT			(4)
#define FSR_FS_PERMISSION_FAULT				(12)
#define FSR_FS_SYNC_EXTERNAL_ABORT			(16)
#define FSR_FS_ASYNC_EXTERNAL_ABORT			(17)
#define FSR_FS_SYNC_PARITY_ERROR			(24)
#define FSR_FS_ASYNC_PARITY_ERROR			(25)
#define FSR_FS_ALIGNMENT_FAULT				(33)
#define FSR_FS_DEBUG_EVENT				(34)
#define FSR_FS_UNSUPPORTED_EXCLUSIVE_ACCESS_FAULT	(53)
#elif defined(CONFIG_ARMV7_A)

/**
 * N.B.: these FSR encodings are only valid when the
 * Short-descriptor translation table format is used
 */

#define FSR_FS_ALIGNMENT_FAULT					(1)
#define FSR_FS_DEBUG_EVENT					(2)
#define FSR_FS_ACCESS_FLAG_FAULT_1ST_LEVEL			(3)
#define FSR_FS_CACHE_MAINTENANCE_INSTRUCTION_FAULT		(4)
#define FSR_FS_TRANSLATION_FAULT				(5)
#define FSR_FS_ACCESS_FLAG_FAULT_2ND_LEVEL			(6)
#define FSR_FS_TRANSLATION_FAULT_2ND_LEVEL			(7)
#define FSR_FS_SYNC_EXTERNAL_ABORT				(8)
#define FSR_FS_DOMAIN_FAULT_1ST_LEVEL				(9)
#define FSR_FS_DOMAIN_FAULT_2ND_LEVEL				(11)
#define FSR_FS_SYNC_EXTERNAL_ABORT_TRANSLATION_TABLE_1ST_LEVEL	(12)
#define FSR_FS_PERMISSION_FAULT					(13)
#define FSR_FS_SYNC_EXTERNAL_ABORT_TRANSLATION_TABLE_2ND_LEVEL	(14)
#define FSR_FS_PERMISSION_FAULT_2ND_LEVEL			(15)
#define FSR_FS_TLB_CONFLICT_FAULT				(16)
#define FSR_FS_ASYNC_EXTERNAL_ABORT				(22)
#define FSR_FS_ASYNC_PARITY_ERROR				(24)
#define FSR_FS_SYNC_PARITY_ERROR				(25)
#define FSR_FS_SYNC_PARITY_ERROR_TRANSLATION_TABLE_1ST_LEVEL	(28)
#define FSR_FS_SYNC_PARITY_ERROR_TRANSLATION_TABLE_2ND_LEVEL	(30)
#else
#define FSR_FS_BACKGROUND_FAULT		(0)
#define FSR_FS_ALIGNMENT_FAULT		(1)
#define FSR_FS_DEBUG_EVENT		(2)
#define FSR_FS_SYNC_EXTERNAL_ABORT	(8)
#define FSR_FS_PERMISSION_FAULT		(13)
#define FSR_FS_ASYNC_EXTERNAL_ABORT	(22)
#define FSR_FS_ASYNC_PARITY_ERROR	(24)
#define FSR_FS_SYNC_PARITY_ERROR	(25)
#endif

/* DBGDSCR Register Definitions */
#define DBGDSCR_MOE_Pos			(2U)
#define DBGDSCR_MOE_Msk			(0xFUL << DBGDSCR_MOE_Pos)

#define DBGDSCR_MOE_HALT_REQUEST	(0)
#define DBGDSCR_MOE_BREAKPOINT		(1)
#define DBGDSCR_MOE_ASYNC_WATCHPOINT	(2)
#define DBGDSCR_MOE_BKPT_INSTRUCTION	(3)
#define DBGDSCR_MOE_EXT_DEBUG_REQUEST	(4)
#define DBGDSCR_MOE_VECTOR_CATCH	(5)
#define DBGDSCR_MOE_OS_UNLOCK_CATCH	(8)
#define DBGDSCR_MOE_SYNC_WATCHPOINT	(10)

__STATIC_FORCEINLINE uint32_t __get_DFAR(void)
{
	uint32_t result;
	__get_CP(15, 0, result, 6, 0, 0);
	return result;
}

__STATIC_FORCEINLINE uint32_t __get_IFAR(void)
{
	uint32_t result;
	__get_CP(15, 0, result, 6, 0, 2);
	return result;
}

__STATIC_FORCEINLINE uint32_t __get_DBGDSCR(void)
{
	uint32_t result;
	__get_CP(14, 0, result, 0, 1, 0);
	return result;
}

#endif /* ZEPHYR_MODULES_CMSIS_CMSIS_A_R_EXT_H_ */
