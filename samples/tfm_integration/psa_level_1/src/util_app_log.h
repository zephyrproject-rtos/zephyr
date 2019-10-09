/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include "psa/error.h"
#include "psa/initial_attestation.h"
#include "psa/protected_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logs PSA response messages other than PSA_SUCCESS for debugging
 *        purposes.
 *
 * @param status        The psa_status_t value to log.
 * @param func_name     The name of the function that made this function call.
 *
 * @return Returns the psa_status_t value passed into the function.
 */
psa_status_t al_psa_status(psa_status_t status, const char *func_name);

/**
 * @brief Logs PSA protected storage service error responses.
 *
 * @param error         The psa_ps_status_t value to log.
 * @param func_name     The name of the function that made this function call.
 *
 * @return Returns the psa_attest_err_t value passed into the function.
 */
psa_ps_status_t al_psa_ps_err(psa_ps_status_t err,
			      const char *func_name);

/**
 * @brief Logs PSA initial attestation service error responses.
 *
 * @param error         The psa_attest_err_t value to log.
 * @param func_name     The name of the function that made this function call.
 *
 * @return Returns the psa_attest_err_t value passed into the function.
 */
enum psa_attest_err_t al_psa_attest_err(enum psa_attest_err_t err,
					const char *func_name);

/**
 * @brief Calls 'log_process' in Zephyr to dump any queued log messages.
 */
void al_dump_log(void);

#ifdef __cplusplus
}
#endif
