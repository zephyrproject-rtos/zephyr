/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_HAL_NORDIC_IRONSIDE_SE_INCLUDE_IRONSIDE_ZEPHYR_SE_DVFS_H_
#define ZEPHYR_MODULES_HAL_NORDIC_IRONSIDE_SE_INCLUDE_IRONSIDE_ZEPHYR_SE_DVFS_H_

#include <ironside/se/api.h>

/** The DVFS oppoint change operation is not allowed in the ISR context. */
#define IRONSIDE_SE_DVFS_ERROR_ISR_NOT_ALLOWED (7)

/**
 * @brief Change the current DVFS oppoint.
 *
 * This function will request a change of the current DVFS oppoint to the
 * specified value. It will block until the change is applied.
 *
 * @param dvfs_oppoint The new DVFS oppoint to set.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_DVFS_ERROR_WRONG_OPPOINT if the requested DVFS oppoint is not allowed.
 * @retval -IRONSIDE_SE_DVFS_ERROR_BUSY if waiting for mutex lock timed out, or hardware is busy.
 * @retval -IRONSIDE_SE_DVFS_ERROR_OPPOINT_DATA if there is configuration error in the DVFS service.
 * @retval -IRONSIDE_SE_DVFS_ERROR_PERMISSION if the caller does not have permission to change the
 * DVFS oppoint.
 * @retval -IRONSIDE_SE_DVFS_ERROR_NO_CHANGE_NEEDED if the requested DVFS oppoint is already set.
 * @retval -IRONSIDE_SE_DVFS_ERROR_TIMEOUT if the operation timed out, possibly due to a hardware
 * issue.
 * @retval -IRONSIDE_SE_DVFS_ERROR_ISR_NOT_ALLOWED if the DVFS oppoint change operation is not
 *  allowed in the ISR context.
 * @retval Positive error status if reported by IronSide call (see error codes in @ref call.h).
 */
int ironside_se_dvfs_change_oppoint(enum ironside_se_dvfs_oppoint dvfs_oppoint);

#endif /* ZEPHYR_MODULES_HAL_NORDIC_IRONSIDE_SE_INCLUDE_IRONSIDE_ZEPHYR_SE_DVFS_H_ */
