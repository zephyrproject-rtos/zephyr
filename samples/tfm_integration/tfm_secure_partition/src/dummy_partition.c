/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tfm_veneers.h>
#include <tfm_ns_interface.h>

#include "dummy_partition.h"

#if defined(CONFIG_TFM_IPC)
#include "psa/client.h"
#include "psa_manifest/sid.h"

psa_status_t dp_secret_digest(uint32_t secret_index,
			void *p_digest,
			size_t digest_size)
{
	psa_status_t status;
	psa_handle_t handle;

	psa_invec in_vec[] = {
		{ .base = &secret_index, .len = sizeof(secret_index) },
	};

	psa_outvec out_vec[] = {
		{ .base = p_digest, .len = digest_size }
	};

	handle = psa_connect(TFM_DP_SECRET_DIGEST_SID,
				TFM_DP_SECRET_DIGEST_VERSION);
	if (!PSA_HANDLE_IS_VALID(handle)) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	status = psa_call(handle, PSA_IPC_CALL, in_vec, IOVEC_LEN(in_vec),
			out_vec, IOVEC_LEN(out_vec));

	psa_close(handle);

	return status;
}
#else /* defined(CONFIG_TFM_IPC) */
psa_status_t dp_secret_digest(uint32_t secret_index,
			void *p_digest,
			size_t digest_size)
{
	psa_status_t status;
	psa_invec in_vec[] = {
		{ .base = &secret_index, .len = sizeof(secret_index) },
	};

	psa_outvec out_vec[] = {
		{ .base = p_digest, .len = digest_size }
	};

	status = tfm_ns_interface_dispatch(
				(veneer_fn)tfm_dp_secret_digest_req_veneer,
				(uint32_t)in_vec,  IOVEC_LEN(in_vec),
				(uint32_t)out_vec, IOVEC_LEN(out_vec));

	return status;
}
#endif /* defined(CONFIG_TFM_IPC) */
