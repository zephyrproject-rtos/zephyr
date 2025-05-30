/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_IRONSIDE_DVFS_H_
#define ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_IRONSIDE_DVFS_H_

#include <stdint.h>
#include <stddef.h>
#include <errno.h>

enum ironside_dvfs_oppoint {
	IRONSIDE_DVFS_OPP_HIGH = 0,
	IRONSIDE_DVFS_OPP_MEDLOW = 1,
	IRONSIDE_DVFS_OPP_LOW = 2,
	IRONSIDE_DVFS_OPP_COUNT
};

/**
 * @name IRONside DVFS service error codes.
 * @{
 */

/** The requested DVFS oppoint is not allowed. */
#define IRONSIDE_DVFS_ERROR_WRONG_OPPOINT (EINVAL)
/** Waiting for mutex lock timed out, or hardware is busy. */
#define IRONSIDE_DVFS_ERROR_BUSY (EBUSY)
/** There is configuration error in the DVFS service. */
#define IRONSIDE_DVFS_ERROR_OPPOINT_DATA (ECANCELED)

/**
 * @}
 */

/* IRONside call identifiers with implicit versions.
 *
 * With the initial "version 0", the service ABI is allowed to break until the
 * first production release of IRONside SE.
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

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_NRF_IRONSIDE_IRONSIDE_DVFS_H_ */
