/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

#include "psa/crypto.h"
#include "util_app_log.h"

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

psa_status_t al_psa_status(psa_status_t status, const char *func_name)
{
	switch (status) {
	case PSA_SUCCESS:
		break;

	/* Generic PSA errors (psa/error.h). */
	case PSA_ERROR_PROGRAMMER_ERROR:
		LOG_ERR("Programmer error");
		break;
	case PSA_ERROR_CONNECTION_REFUSED:
		LOG_ERR("Connection refused");
		break;
	case PSA_ERROR_CONNECTION_BUSY:
		LOG_ERR("Connection busy");
		break;
	case PSA_ERROR_GENERIC_ERROR:
		LOG_ERR("Generic error");
		break;
	case PSA_ERROR_NOT_PERMITTED:
		LOG_ERR("Not permitted");
		break;
	case PSA_ERROR_NOT_SUPPORTED:
		LOG_ERR("Unsupported operation");
		break;
	case PSA_ERROR_INVALID_ARGUMENT:
		LOG_ERR("Invalid argument");
		break;
	case PSA_ERROR_INVALID_HANDLE:
		LOG_ERR("Invalid handle");
		break;
	case PSA_ERROR_BAD_STATE:
		LOG_ERR("Bad state");
		break;
	case PSA_ERROR_BUFFER_TOO_SMALL:
		LOG_ERR("Buffer too small");
		break;
	case PSA_ERROR_ALREADY_EXISTS:
		LOG_ERR("Already exists");
		break;
	case PSA_ERROR_DOES_NOT_EXIST:
		LOG_ERR("Does not exist");
		break;
	case PSA_ERROR_INSUFFICIENT_MEMORY:
		LOG_ERR("Insufficient memory");
		break;
	case PSA_ERROR_INSUFFICIENT_STORAGE:
		LOG_ERR("Insufficient storage");
		break;
	case PSA_ERROR_INSUFFICIENT_DATA:
		LOG_ERR("Insufficient memory data");
		break;
	case PSA_ERROR_SERVICE_FAILURE:
		LOG_ERR("Service failure");
		break;
	case PSA_ERROR_COMMUNICATION_FAILURE:
		LOG_ERR("Communication failure");
		break;
	case PSA_ERROR_STORAGE_FAILURE:
		LOG_ERR("Storage failure");
		break;
	case PSA_ERROR_HARDWARE_FAILURE:
		LOG_ERR("Hardware failure");
		break;
	case PSA_ERROR_INVALID_SIGNATURE:
		LOG_ERR("Invalid signature");
		break;

	/* PSA crypto errors (psa/crypto_values.h). */
	case PSA_ERROR_INSUFFICIENT_ENTROPY:
		LOG_ERR("CRYPTO: Insufficient entropy");
		break;
	case PSA_ERROR_TAMPERING_DETECTED:
		LOG_ERR("CRYPTO: Tampering detected");
		break;

	/* Catch-all error handler. */
	default:
		LOG_ERR("Unhandled status response: %d", status);
		break;
	}

	/* Display the calling function name for debug purposes. */
	if (status != PSA_SUCCESS) {
		LOG_ERR("Function: '%s'", func_name);
	}

	return status;
}

psa_ps_status_t al_psa_ps_err(psa_ps_status_t err,
			      const char *func_name)
{
	switch (err) {
	case PSA_SUCCESS:
		break;

	case PSA_PS_ERROR_INVALID_ARGUMENT:
		LOG_ERR("PS: Invalid argument");
		break;
	case PSA_PS_ERROR_UID_NOT_FOUND:
		LOG_ERR("PS: UID/resource not found");
		break;
	case PSA_PS_ERROR_STORAGE_FAILURE:
		LOG_ERR("PS: Physical storage failure");
		break;
	case PSA_PS_ERROR_OPERATION_FAILED:
		LOG_ERR("PS: Unspecified internal error");
		break;
	case PSA_PS_ERROR_DATA_CORRUPT:
		LOG_ERR("PS: Authentication failure reading key");
		break;
	case PSA_PS_ERROR_AUTH_FAILED:
		LOG_ERR("PS: Data authentication failure.");
		break;

	/* Catch-all error handler. */
	default:
		LOG_ERR("PS: Unhandled err response: %d", err);
		break;
	}

	/* Display the calling function name for debug purposes. */
	if (err != PSA_PS_SUCCESS) {
		LOG_ERR("Function: '%s'", func_name);
	}

	return err;
}

enum psa_attest_err_t al_psa_attest_err(enum psa_attest_err_t err,
					const char *func_name)
{
	switch (err) {
	case PSA_ATTEST_ERR_SUCCESS:
		break;

	case PSA_ATTEST_ERR_INIT_FAILED:
		LOG_ERR("ATT: Init failed");
		break;
	case PSA_ATTEST_ERR_TOKEN_BUFFER_OVERFLOW:
		LOG_ERR("ATT: Token buffer overflow");
		break;
	case PSA_ATTEST_ERR_CLAIM_UNAVAILABLE:
		LOG_ERR("ATT: claim unavailable");
		break;
	case PSA_ATTEST_ERR_INVALID_INPUT:
		LOG_ERR("ATT: invalid input");
		break;
	case PSA_ATTEST_ERR_GENERAL:
		LOG_ERR("ATT: general error");
		break;

	/* Catch-all error handler. */
	default:
		LOG_ERR("ATT: Unhandled err response: %d", err);
		break;
	}

	/* Display the calling function name for debug purposes. */
	if (err != PSA_ATTEST_ERR_SUCCESS) {
		LOG_ERR("Function: '%s'", func_name);
	}

	return err;
}

void al_dump_log(void)
{
	while (log_process(false)) {

	}
}
