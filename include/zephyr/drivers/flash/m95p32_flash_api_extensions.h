/*
 * SPDX-FileCopyrightText: Copyright 2026 EXALT Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ST M95P32 page EEPROM flash API extensions.
 * @ingroup m95p32_flash_ex_op
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FLASH_M95P32_FLASH_API_EXTENSIONS_H_
#define ZEPHYR_INCLUDE_DRIVERS_FLASH_M95P32_FLASH_API_EXTENSIONS_H_

#include <stddef.h>
#include <sys/types.h>

#include <zephyr/drivers/flash.h>

/**
 * @brief ST M95P32 page EEPROM extended operations.
 * @defgroup m95p32_flash_ex_op ST M95P32
 * @ingroup flash_ex_op
 * @{
 */

/** Size of one M95P32 identification page. */
#define M95P32_ID_PAGE_SIZE 512U

/** Total size of the two M95P32 identification pages. */
#define M95P32_ID_AREA_SIZE (2U * M95P32_ID_PAGE_SIZE)

/** Start of the customer identification page. */
#define M95P32_CUSTOMER_ID_PAGE_OFFSET M95P32_ID_PAGE_SIZE

/** M95P32-specific flash extended operations. */
enum flash_m95p32_ex_ops {
	/** Read the factory and customer identification pages. */
	FLASH_M95P32_EX_OP_READ_ID_PAGE = FLASH_EX_OP_VENDOR_BASE,
	/** Write data to the customer identification page. */
	FLASH_M95P32_EX_OP_WRITE_ID_PAGE,
	/** Program main-memory pages with M95P32 buffer-load mode enabled. */
	FLASH_M95P32_EX_OP_PAGE_PROGRAM_WITH_BUFFER_LOAD,
};

/** Input parameters for @ref FLASH_M95P32_EX_OP_READ_ID_PAGE. */
struct flash_m95p32_ex_op_read_id_page_in {
	/** Offset in the M95P32 identification area. */
	off_t offset;
	/** Number of bytes to read. */
	size_t length;
};

/** Input parameters for @ref FLASH_M95P32_EX_OP_WRITE_ID_PAGE. */
struct flash_m95p32_ex_op_write_id_page_in {
	/** Offset in the customer identification page. */
	off_t offset;
	/** Data to write. */
	const void *data;
	/** Number of bytes to write. */
	size_t length;
};

/**
 * @brief Input parameters for @ref FLASH_M95P32_EX_OP_PAGE_PROGRAM_WITH_BUFFER_LOAD.
 *
 * The target array must be erased before programming. The driver splits the
 * data at the M95P32 512-byte page boundaries and uses buffer-load mode to
 * overlap programming one page with loading the next page.
 */
struct flash_m95p32_ex_op_page_program_with_buffer_load_in {
	/** Offset in the M95P32 main memory array. */
	off_t offset;
	/** Data to program. */
	const void *data;
	/** Number of bytes to program. */
	size_t length;
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FLASH_M95P32_FLASH_API_EXTENSIONS_H_ */
