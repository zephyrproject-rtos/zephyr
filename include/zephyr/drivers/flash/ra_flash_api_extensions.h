/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Renesas RA flash extended operations.
 * @ingroup ra_flash_ex_op
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FLASH_RA_FLASH_API_EXTENSIONS_H_
#define ZEPHYR_INCLUDE_DRIVERS_FLASH_RA_FLASH_API_EXTENSIONS_H_

/**
 * @brief Extended operations for Renesas RA flash controllers.
 * @defgroup ra_flash_ex_op Renesas RA
 * @ingroup flash_ex_op
 * @{
 */

#include <zephyr/drivers/flash.h>

/**
 * @brief Enumeration for Renesas RA flash extended operations.
 */
enum ra_ex_ops {
	/**
	 * Reset Flash device (at QPI(4-4-4) mode).
	 */
	QSPI_FLASH_EX_OP_EXIT_QPI = 1,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FLASH_RA_FLASH_API_EXTENSIONS_H_ */
