/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_H_
#define ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_H_

/**
 * @file it51xxx_crypto.h
 *
 * Public API for IT51XXX Crypto driver
 */

#include "psa/crypto.h"

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Size of the shared hardware crypto DLM region in bytes. */
#define IT51XXX_HWCRYPTO_DLM_SIZE KB(4)

/**
 * @brief Shared DLM storage for it51xxx hardware crypto algorithms.
 *
 * SHA, RSA, and other crypto algorithms share the same 4 KiB DLM
 * memory region. Union members use the same underlying storage,
 * so access must be serialized.
 */
union hwcrypto_dlm_block {
	/** Raw access to the entire shared DLM region. */
	uint8_t raw_data[IT51XXX_HWCRYPTO_DLM_SIZE];
};

/**
 * @brief Shared DLM instance used by it51xxx hardware crypto algorithms.
 */
extern union hwcrypto_dlm_block hwcrypto_dlm;

/**
 *@brief Get the hardware crypto register base address.
 *
 *@return Register base address obtained from devicetree.
 */
mm_reg_t it51xxx_crypto_get_base(void);

/**
 *@brief Lock the shared hardware crypto resource.
 *
 *@param[in] timeout Maximum time to wait for the mutex.
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
int it51xxx_crypto_lock(k_timeout_t timeout);

/**
 *@brief Unlock the shared hardware crypto resource.
 *
 * Must be called by the thread that holds the mutex.
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
void it51xxx_crypto_unlock(void);

/**
 *@brief Initialize shared hardware crypto resources.
 *
 * Call before starting any hardware crypto operation.
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t it51xxx_psa_crypto_init(void);

/**
 *@brief Release shared hardware crypto resources.
 *
 * Call only after all hardware crypto operations have completed.
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t it51xxx_psa_crypto_free(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_H_ */
