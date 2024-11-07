/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BOARD_VERSION_H_
#define _BOARD_VERSION_H_

#include "board.h"

/**@brief Get the board/HW version
 *
 * @note  This function will init the ADC, perform a reading, and
 *	  return the HW version.
 *
 * @param board_rev	Pointer to container for board version
 *
 * @return 0 on success.
 * Error code on fault or -ESPIPE if no valid version found
 */
int board_version_get(struct board_version *board_rev);

/**@brief Check that the FW is compatible with the HW version
 *
 * @note  This function will init the ADC, perform a reading, and
 * check for valid version match.
 *
 * @note The board file must define a BOARD_VERSION_ARR array of
 * possible valid ADC register values (voltages) for the divider.
 * A BOARD_VERSION_VALID_MSK with valid version bits must also be defined.
 *
 * @return 0 on success. Error code on fault or -EPERM if incompatible board version.
 */
int board_version_valid_check(void);

#endif /* _BOARD_VERSION_H_ */
