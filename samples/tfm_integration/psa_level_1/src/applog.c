/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <logging/log.h>

#include "psa/crypto.h"
#include "psa/protected_storage.h"
#include "applog.h"

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

psa_status_t al_psa_status(psa_status_t status, const char *func_name)
{
	switch (status) {
	case PSA_SUCCESS:
		break;

	/* Generic PSA errors (psa/error.h). */
	case PSA_ERROR_PROGRAMMER_ERROR:
		LOG_ERR("Programmer error\n");
		break;
	case PSA_ERROR_CONNECTION_REFUSED:
		LOG_ERR("Connection refused\n");
		break;
	case PSA_ERROR_CONNECTION_BUSY:
		LOG_ERR("Connection busy\n");
		break;
	case PSA_ERROR_GENERIC_ERROR:
		LOG_ERR("Generic error\n");
		break;
	case PSA_ERROR_NOT_PERMITTED:
		LOG_ERR("Not permitted\n");
		break;
	case PSA_ERROR_NOT_SUPPORTED:
		LOG_ERR("Unsupported operation\n");
		break;
	case PSA_ERROR_INVALID_ARGUMENT:
		LOG_ERR("Invalid argument\n");
		break;
	case PSA_ERROR_INVALID_HANDLE:
		LOG_ERR("Invalid handle\n");
		break;
	case PSA_ERROR_BAD_STATE:
		LOG_ERR("Bad state\n");
		break;
	case PSA_ERROR_BUFFER_TOO_SMALL:
		LOG_ERR("Buffer too small\n");
		break;
	case PSA_ERROR_ALREADY_EXISTS:
		LOG_ERR("Already exists\n");
		break;
	case PSA_ERROR_DOES_NOT_EXIST:
		LOG_ERR("Does not exist\n");
		break;
	case PSA_ERROR_INSUFFICIENT_MEMORY:
		LOG_ERR("Insufficient memory\n");
		break;
	case PSA_ERROR_INSUFFICIENT_STORAGE:
		LOG_ERR("Insufficient storage\n");
		break;
	case PSA_ERROR_INSUFFICIENT_DATA:
		LOG_ERR("Insufficient memory data\n");
		break;
	case PSA_ERROR_SERVICE_FAILURE:
		LOG_ERR("Service failure\n");
		break;
	case PSA_ERROR_COMMUNICATION_FAILURE:
		LOG_ERR("Communication failure\n");
		break;
	case PSA_ERROR_STORAGE_FAILURE:
		LOG_ERR("Storage failure\n");
		break;
	case PSA_ERROR_HARDWARE_FAILURE:
		LOG_ERR("Hardware failure\n");
		break;
	case PSA_ERROR_INVALID_SIGNATURE:
		LOG_ERR("Invalid signature\n");
		break;

	/* PSA crypto errors (psa/crypto_values.h). */
	case PSA_ERROR_INSUFFICIENT_ENTROPY:
		LOG_ERR("CRYPTO: Insufficient entropy\n");
		break;
	case PSA_ERROR_TAMPERING_DETECTED:
		LOG_ERR("CRYPTO: Tampering detected\n");
		break;

	/* PSA protected storage errors (psa/protected_storage.h). */
	case PSA_PS_ERROR_INVALID_ARGUMENT:
		LOG_ERR("PS: Invalid argument\n");
		break;
	case PSA_PS_ERROR_UID_NOT_FOUND:
		LOG_ERR("PS: UID/resource not found\n");
		break;
	case PSA_PS_ERROR_STORAGE_FAILURE:
		LOG_ERR("PS: Physical storage failure\n");
		break;
	case PSA_PS_ERROR_OPERATION_FAILED:
		LOG_ERR("PS: Unspecified internal error\r");
		break;
	case PSA_PS_ERROR_DATA_CORRUPT:
		LOG_ERR("PS: Authentication failure reading key\n");
		break;
	case PSA_PS_ERROR_AUTH_FAILED:
		LOG_ERR("PS: Data authentication failure.\n");
		break;

	/* Catch-all error handler. */
	default:
		LOG_ERR("Unhandled status response: %d\n", status);
		break;
	}

	/* Display the calling function name for debug purposes. */
	if (status != PSA_SUCCESS) {
		LOG_ERR("Function: '%s'\n", func_name);
	}

	return status;
}
