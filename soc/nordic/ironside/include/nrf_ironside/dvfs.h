/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_DVFS_H_
#define ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_DVFS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>

enum ironside_dvfs_oppoint {
	IRONSIDE_DVFS_OPP_HIGH = 0,
	IRONSIDE_DVFS_OPP_MEDLOW = 1,
	IRONSIDE_DVFS_OPP_LOW = 2
};

/**
 * @brief Number of DVFS oppoints supported by IronSide.
 *
 * This is the number of different DVFS oppoints that can be set on IronSide.
 * The oppoints are defined in the `ironside_dvfs_oppoint` enum.
 */
#define IRONSIDE_DVFS_OPPOINT_COUNT (3)

/**
 * @name IronSide DVFS service error codes.
 * @{
 */

/** The requested DVFS oppoint is not allowed. */
#define IRONSIDE_DVFS_ERROR_WRONG_OPPOINT    (1)
/** Waiting for mutex lock timed out, or hardware is busy. */
#define IRONSIDE_DVFS_ERROR_BUSY             (2)
/** There is configuration error in the DVFS service. */
#define IRONSIDE_DVFS_ERROR_OPPOINT_DATA     (3)
/** The caller does not have permission to change the DVFS oppoint. */
#define IRONSIDE_DVFS_ERROR_PERMISSION       (4)
/** The requested DVFS oppoint is already set, no change needed. */
#define IRONSIDE_DVFS_ERROR_NO_CHANGE_NEEDED (5)
/** The operation timed out, possibly due to a hardware issue. */
#define IRONSIDE_DVFS_ERROR_TIMEOUT          (6)
/** The DVFS oppoint change operation is not allowed in the ISR context. */
#define IRONSIDE_DVFS_ERROR_ISR_NOT_ALLOWED  (7)

/**
 * @}
 */

/* IronSide call identifiers with implicit versions.
 *
 * With the initial "version 0", the service ABI is allowed to break until the
 * first production release of IronSide SE.
 */
#define IRONSIDE_CALL_ID_DVFS_SERVICE_V0 3

/* Index of the DVFS oppoint within the service buffer. */
#define IRONSIDE_DVFS_SERVICE_OPPOINT_IDX (0)
/* Index of the return code within the service buffer. */
#define IRONSIDE_DVFS_SERVICE_RETCODE_IDX (0)

/**
 * @brief Change the current DVFS oppoint.
 *
 * This function will request a change of the current DVFS oppoint to the
 * specified value. It will block until the change is applied.
 *
 * @param dvfs_oppoint The new DVFS oppoint to set.
 * @return int 0 on success, negative error code on failure.
 */
int ironside_dvfs_change_oppoint(enum ironside_dvfs_oppoint dvfs_oppoint);

/**
 * @brief Check if the given oppoint is valid.
 *
 * @param dvfs_oppoint The oppoint to check.
 * @return true if the oppoint is valid, false otherwise.
 */
static inline bool ironside_dvfs_is_oppoint_valid(enum ironside_dvfs_oppoint dvfs_oppoint)
{
	if (dvfs_oppoint != IRONSIDE_DVFS_OPP_HIGH && dvfs_oppoint != IRONSIDE_DVFS_OPP_MEDLOW &&
	    dvfs_oppoint != IRONSIDE_DVFS_OPP_LOW) {
		return false;
	}

	return true;
}

#endif /* ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_DVFS_H_ */
