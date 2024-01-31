/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

enum board_daplink_qspi_mux_mode {
	/* eXecute-In-Place mode */
	BOARD_DAPLINK_QSPI_MUX_MODE_XIP,
	/* Normal mode */
	BOARD_DAPLINK_QSPI_MUX_MODE_NORMAL,
};

/**
 * @brief Select the mode of the DAPlink QSPI multiplexer.
 *
 * Note: The multiplexer mode must not be changed while executing code from the
 *       off-board QSPI flash in XIP mode.
 *
 * @param mode The multiplexer mode to be selected.
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int board_daplink_qspi_mux_select(enum board_daplink_qspi_mux_mode mode);

/**
 * @brief Determine if the DAPlink shield is fitted.
 *
 * Determine if the DAPlink shield is fitted based on the state of the
 * DAPLINK_fitted_n signal.
 *
 * @retval true If the DAPlink shield is fitted.
 * @retval false If the DAPlink shield is not fitted.
 */
bool board_daplink_is_fitted(void);

#endif /* __INC_BOARD_H */
