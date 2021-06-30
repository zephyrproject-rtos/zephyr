
#include <psa/crypto.h>
#include <stdbool.h>
#include <stdint.h>
#include "tfm_secure_api.h"
#include "tfm_api.h"

static psa_status_t tfm_dp_init(void)
{
	return TFM_SUCCESS;
}

#define NUM_SECRETS 5

struct dp_secret {
	uint8_t secret[16];
};

struct dp_secret secrets[NUM_SECRETS] = {
	{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
	{{1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
	{{2,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
	{{3,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
	{{4,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
};

typedef void (*psa_write_callback_t)(void *handle, uint8_t *digest,
				uint32_t digest_size);

static psa_status_t tfm_dp_secret_digest(uint32_t secret_index,
			size_t digest_size, size_t *p_digest_size,
			psa_write_callback_t callback, void *handle)
{
	uint8_t digest[32];
	psa_status_t status;

	/* Check that secret_index is valid. */
	if (secret_index >= NUM_SECRETS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Check that digest_size is valid. */
	if (digest_size != sizeof(digest)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = psa_hash_compute(PSA_ALG_SHA_256, secrets[secret_index].secret,
				sizeof(secrets[secret_index].secret), digest,
				digest_size, p_digest_size);

	if (status != PSA_SUCCESS) {
		return status;
	}
	if (*p_digest_size != digest_size){
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	callback(handle, digest, digest_size);
}

#ifndef TFM_PSA_API

#include "tfm_memory_utils.h"

/*
 * \brief Indicates whether DP has been initialised.
 */
static bool dp_is_init = false;

/*
 * \brief Initialises DP, if not already initialised.
 *
 * \note In library mode, initialisation is delayed until the first secure
 *	   function call, as calls to the Crypto service are required for
 *	   initialisation.
 *
 * \return PSA_SUCCESS if DP is initialised, PSA_ERROR_GENERIC_ERROR
 *		 otherwise.
 */
static psa_status_t dp_check_init(void)
{
	if (!dp_is_init) {
		if (tfm_dp_init() != PSA_SUCCESS) {
			return PSA_ERROR_GENERIC_ERROR;
		}
		dp_is_init = true;
	}

	return PSA_SUCCESS;
}


void psa_write_digest(void *handle, uint8_t *digest, uint32_t digest_size)
{
	tfm_memcpy(handle, digest, digest_size);
}

psa_status_t tfm_dp_secret_digest_req(psa_invec *in_vec, size_t in_len,
				psa_outvec *out_vec, size_t out_len)
{
	uint32_t secret_index;

	if (dp_check_init() != PSA_SUCCESS) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	if ((in_len != 1) || (out_len != 1)) {
		/* The number of arguments are incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (in_vec[0].len != sizeof(secret_index)) {
		/* The input argument size is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	secret_index = *((uint32_t *)in_vec[0].base);

	return tfm_dp_secret_digest(
				secret_index, out_vec[0].len, &out_vec[0].len,
				psa_write_digest, (void *)out_vec[0].base);
}

#else /* !defined(TFM_PSA_API) */
#include "psa/service.h"
#include "psa_manifest/tfm_dummy_partition.h"

typedef psa_status_t (*dp_func_t)(psa_msg_t *);

static void psa_write_digest_0(void *handle, uint8_t *digest,
				uint32_t digest_size)
{
	psa_write((psa_handle_t)handle, 0, digest, digest_size);
}

static psa_status_t tfm_dp_secret_digest_ipc(psa_msg_t *msg)
{
	size_t num = 0;
	uint32_t secret_index;

	if (msg->in_size[0] != sizeof(secret_index)) {
		/* The size of the argument is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	num = psa_read(msg->handle, 0, &secret_index, msg->in_size[0]);
	if (num != msg->in_size[0]) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	return tfm_dp_secret_digest(
				secret_index, msg->out_size[0], &msg->out_size[0],
				psa_write_digest_0, (void *)msg->handle);
}


static void dp_signal_handle(psa_signal_t signal, dp_func_t pfn)
{
	psa_status_t status;
	psa_msg_t msg;

	status = psa_get(signal, &msg);
	switch (msg.type) {
	case PSA_IPC_CONNECT:
		psa_reply(msg.handle, PSA_SUCCESS);
		break;
	case PSA_IPC_CALL:
		status = pfn(&msg);
		psa_reply(msg.handle, status);
		break;
	case PSA_IPC_DISCONNECT:
		psa_reply(msg.handle, PSA_SUCCESS);
		break;
	default:
		psa_panic();
	}
}
#endif /* !defined(TFM_PSA_API) */

psa_status_t tfm_dp_req_mngr_init(void)
{
#ifdef TFM_PSA_API
	psa_signal_t signals = 0;

	if (tfm_dp_init() != PSA_SUCCESS) {
		psa_panic();
	}

	while (1) {
		signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
		if (signals & TFM_DP_SECRET_DIGEST_SIGNAL) {
			dp_signal_handle(TFM_DP_SECRET_DIGEST_SIGNAL,
					tfm_dp_secret_digest_ipc);
		} else {
			psa_panic();
		}
	}
#else
	/* In library mode, initialisation is delayed until the first secure
	 * function call, as calls to the Crypto service are required for
	 * initialisation.
	 */
	return PSA_SUCCESS;
#endif
}