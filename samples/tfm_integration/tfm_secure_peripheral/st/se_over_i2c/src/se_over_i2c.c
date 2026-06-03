/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tfm_ns_interface.h>

#include "se_over_i2c.h"

#include <psa/client.h>
#include <psa_manifest/sid.h>

psa_status_t se_i2c_probe(struct i2c_probe_result *out)
{
	psa_status_t status;
	psa_handle_t handle;
	psa_outvec out_vec[1] = {{out, sizeof(*out)}};

	handle = psa_connect(TFM_SE_I2C_PROBE_SID, TFM_SE_I2C_PROBE_VERSION);
	if (!PSA_HANDLE_IS_VALID(handle)) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	status = psa_call(handle, PSA_IPC_CALL, NULL, 0, out_vec, 1);

	psa_close(handle);

	return status;
}

psa_status_t se_i2c_echo(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len)
{
	psa_handle_t handle;
	psa_status_t status;

	psa_invec in_vec[] = {{tx_data, tx_len}};
	psa_outvec out_vec[] = {{rx_data, rx_len}};

	handle = psa_connect(TFM_SE_I2C_ECHO_SID, 1);
	if (!PSA_HANDLE_IS_VALID(handle)) {
		return PSA_ERROR_COMMUNICATION_FAILURE;
	}

	status = psa_call(handle, PSA_IPC_CALL, in_vec, 1, out_vec, 1);

	psa_close(handle);

	return status;
}
