/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_COUNTER_H_
#define ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_COUNTER_H_

#include <stdint.h>
#include <zephyr/toolchain.h>
#include <nrf_ironside/call.h>

/**
 * @name Counter service error codes.
 * @{
 */

/** Counter value is lower than current value (monotonic violation). */
#define IRONSIDE_COUNTER_ERROR_TOO_LOW         (1)
/** Invalid counter ID. */
#define IRONSIDE_COUNTER_ERROR_INVALID_ID      (2)
/** Counter is locked and cannot be modified. */
#define IRONSIDE_COUNTER_ERROR_LOCKED          (3)
/** Invalid parameter. */
#define IRONSIDE_COUNTER_ERROR_INVALID_PARAM   (4)
/** Storage operation failed. */
#define IRONSIDE_COUNTER_ERROR_STORAGE_FAILURE (5)

/**
 * @}
 */

/** Maximum value for a counter */
#define IRONSIDE_COUNTER_MAX_VALUE UINT32_MAX

/** Number of counters */
#define IRONSIDE_COUNTER_NUM 4

/**
 * @brief Counter identifiers.
 */
enum ironside_counter {
	IRONSIDE_COUNTER_0 = 0,
	IRONSIDE_COUNTER_1,
	IRONSIDE_COUNTER_2,
	IRONSIDE_COUNTER_3
};

/* IronSide call identifiers with implicit versions. */
#define IRONSIDE_CALL_ID_COUNTER_SET_V1  6
#define IRONSIDE_CALL_ID_COUNTER_GET_V1  7
#define IRONSIDE_CALL_ID_COUNTER_LOCK_V1 8

enum {
	IRONSIDE_COUNTER_SET_SERVICE_COUNTER_ID_IDX,
	IRONSIDE_COUNTER_SET_SERVICE_VALUE_IDX,
	/* The last enum value is reserved for the number of arguments */
	IRONSIDE_COUNTER_SET_NUM_ARGS
};

enum {
	IRONSIDE_COUNTER_GET_SERVICE_COUNTER_ID_IDX,
	/* The last enum value is reserved for the number of arguments */
	IRONSIDE_COUNTER_GET_NUM_ARGS
};

enum {
	IRONSIDE_COUNTER_LOCK_SERVICE_COUNTER_ID_IDX,
	/* The last enum value is reserved for the number of arguments */
	IRONSIDE_COUNTER_LOCK_NUM_ARGS
};

/* Index of the return code within the service buffer. */
#define IRONSIDE_COUNTER_SET_SERVICE_RETCODE_IDX  (0)
#define IRONSIDE_COUNTER_GET_SERVICE_RETCODE_IDX  (0)
#define IRONSIDE_COUNTER_LOCK_SERVICE_RETCODE_IDX (0)

/* Index of the value within the GET response buffer. */
#define IRONSIDE_COUNTER_GET_SERVICE_VALUE_IDX (1)

BUILD_ASSERT(IRONSIDE_COUNTER_SET_NUM_ARGS <= NRF_IRONSIDE_CALL_NUM_ARGS);
BUILD_ASSERT(IRONSIDE_COUNTER_GET_NUM_ARGS <= NRF_IRONSIDE_CALL_NUM_ARGS);
BUILD_ASSERT(IRONSIDE_COUNTER_LOCK_NUM_ARGS <= NRF_IRONSIDE_CALL_NUM_ARGS);

/**
 * @brief Set a counter value.
 *
 * This sets the specified counter to the given value. The counter is monotonic,
 * so the new value must be greater than or equal to the current value.
 * If the counter is locked, the operation will fail.
 *
 * @note Counters are automatically initialized to 0 during the first boot in LCS ROT.
 *       The monotonic constraint applies to all subsequent writes.
 *
 * @param counter_id Counter identifier.
 * @param value New counter value.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_COUNTER_ERROR_INVALID_ID if counter_id is invalid.
 * @retval -IRONSIDE_COUNTER_ERROR_TOO_LOW if value is lower than current value.
 * @retval -IRONSIDE_COUNTER_ERROR_LOCKED if counter is locked.
 * @retval -IRONSIDE_COUNTER_ERROR_STORAGE_FAILURE if storage operation failed.
 * @retval Positive error status if reported by IronSide call (see error codes in @ref call.h).
 */
int ironside_counter_set(enum ironside_counter counter_id, uint32_t value);

/**
 * @brief Get a counter value.
 *
 * This retrieves the current value of the specified counter.
 *
 * @note Counters are automatically initialized to 0 during the first boot in LCS ROT,
 *       so this function will always succeed for valid counter IDs.
 *
 * @param counter_id Counter identifier.
 * @param value Pointer to store the counter value.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_COUNTER_ERROR_INVALID_ID if counter_id is invalid.
 * @retval -IRONSIDE_COUNTER_ERROR_INVALID_PARAM if value is NULL.
 * @retval -IRONSIDE_COUNTER_ERROR_STORAGE_FAILURE if storage operation failed.
 * @retval Positive error status if reported by IronSide call (see error codes in @ref call.h).
 */
int ironside_counter_get(enum ironside_counter counter_id, uint32_t *value);

/**
 * @brief Lock a counter for the current boot.
 *
 * This locks the specified counter, preventing any further modifications until the next reboot.
 * The lock state is not persistent and will be cleared on reboot.
 *
 * @note The intended use case is for a bootloader to lock a counter before transferring control
 *       to the next boot stage, preventing that image from modifying the counter value.
 *
 * @param counter_id Counter identifier.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_COUNTER_ERROR_INVALID_ID if counter_id is invalid.
 * @retval Positive error status if reported by IronSide call (see error codes in @ref call.h).
 */
int ironside_counter_lock(enum ironside_counter counter_id);

#endif /* ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_COUNTER_H_ */
