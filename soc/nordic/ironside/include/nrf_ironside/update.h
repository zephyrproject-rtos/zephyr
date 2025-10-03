/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_UPDATE_H_
#define ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_UPDATE_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @name Update service error codes.
 * @{
 */

/** Caller does not have access to the provided update candidate buffer. */
#define IRONSIDE_UPDATE_ERROR_NOT_PERMITTED     (1)
/** Failed to write the update metadata to SICR. */
#define IRONSIDE_UPDATE_ERROR_SICR_WRITE_FAILED (2)
/** Update candidate is placed outside of valid range */
#define IRONSIDE_UPDATE_ERROR_INVALID_ADDRESS (3)

/**
 * @}
 */

/** Size of the update blob */
#ifdef CONFIG_SOC_SERIES_NRF54HX
#define IRONSIDE_UPDATE_BLOB_SIZE (160 * 1024)
#elif CONFIG_SOC_SERIES_NRF92X
#define IRONSIDE_UPDATE_BLOB_SIZE (160 * 1024)
#else
#error "Missing update blob size"
#endif

/** Min address used for storing the update candidate */
#define IRONSIDE_UPDATE_MIN_ADDRESS (0x0e100000)
/** Max address used for storing the update candidate */
#define IRONSIDE_UPDATE_MAX_ADDRESS (0x0e200000 - IRONSIDE_UPDATE_BLOB_SIZE)

/** Length of the update manifest in bytes */
#define IRONSIDE_UPDATE_MANIFEST_LENGTH  (256)
/** Length of the update public key in bytes. */
#define IRONSIDE_UPDATE_PUBKEY_LENGTH    (32)
/** Length of the update signature in bytes. */
#define IRONSIDE_UPDATE_SIGNATURE_LENGTH (64)

/* IronSide call identifiers with implicit versions.
 *
 * With the initial "version 0", the service ABI is allowed to break until the
 * first production release of IronSide SE.
 */
#define IRONSIDE_CALL_ID_UPDATE_SERVICE_V0 1

/* Index of the update blob pointer within the service buffer. */
#define IRONSIDE_UPDATE_SERVICE_UPDATE_PTR_IDX (0)
/* Index of the return code within the service buffer. */
#define IRONSIDE_UPDATE_SERVICE_RETCODE_IDX    (0)

/**
 * @brief IronSide update blob.
 */
struct ironside_update_blob {
	uint8_t manifest[IRONSIDE_UPDATE_MANIFEST_LENGTH];
	uint8_t pubkey[IRONSIDE_UPDATE_PUBKEY_LENGTH];
	uint8_t signature[IRONSIDE_UPDATE_SIGNATURE_LENGTH];
	uint32_t firmware[];
};

/**
 * @brief Request a firmware upgrade of the IronSide SE.
 *
 * This invokes the IronSide SE update service. The device must be restarted for the update
 * to be installed. Check the update status in the application boot report to see if the update
 * was successfully installed.
 *
 * @param update Pointer to update blob
 *
 * @retval 0 on a successful request (although the update itself may still fail).
 * @retval -IRONSIDE_UPDATE_ERROR_INVALID_ADDRESS if the address of the update is outside of the
 *          accepted range.
 * @retval -IRONSIDE_UPDATE_ERROR_NOT_PERMITTED if missing access to the update candidate.
 * @retval -IRONSIDE_UPDATE_ERROR_SICR_WRITE_FAILED if writing update parameters to SICR failed.
 * @retval Positive error status if reported by IronSide call (see error codes in @ref call.h).
 *
 */
int ironside_update(const struct ironside_update_blob *update);

#endif /* ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_UPDATE_H_ */
