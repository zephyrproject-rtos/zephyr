/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

#include "psa/crypto.h"
#include "psa_crypto.h"
#include "util_app_log.h"
#include "util_sformat.h"

/** Declare a reference to the application logging interface. */
LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

/**
 * @brief Generates random values using the TF-M crypto service.
 */
void crp_test_rng(void)
{
	psa_status_t status;
	uint8_t outbuf[256] = { 0 };
	struct sf_hex_tbl_fmt fmt = {
		.ascii = true,
		.addr_label = true,
		.addr = 0
	};

	status = al_psa_status(psa_generate_random(outbuf, 256), __func__);
	LOG_INF("Generating 256 bytes of random data.");
	al_dump_log();
	sf_hex_tabulate_16(&fmt, outbuf, 256);
}

/**
 * @brief Demonstrates how to calcute the SHA-256 hash of a value in chunks.
 */
void crp_test_sha256(void)
{
	const psa_algorithm_t alg = PSA_ALG_SHA_256;
	static const char *const msg[] = { "This is my test message, ",
					   "please generate a hash for this." };
	const size_t msg_size[] = { 25, 32 };
	uint8_t hash_val[PSA_HASH_SIZE(PSA_ALG_SHA_256)] = { 0 };
	size_t hash_len;
	const uint32_t msg_num = ARRAY_SIZE(msg) / sizeof(msg[0]);
	uint32_t idx;
	psa_status_t status;
	psa_hash_operation_t handle = psa_hash_operation_init();

	LOG_INF("Calculating SHA-256 hash of value.");
	al_dump_log();

	/* Setup the hash object. */
	status = al_psa_status(psa_hash_setup(&handle, alg), __func__);
	if (status != PSA_SUCCESS) {
		goto err;
	}

	/* Update object with all the message chunks. */
	for (idx = 0; idx < msg_num; idx++) {
		status = al_psa_status(
			psa_hash_update(
				&handle,
				(const uint8_t *)msg[idx], msg_size[idx]),
			__func__);
		if (status != PSA_SUCCESS) {
			goto err;
		}
	}

	/* Finalize the hash calculation. */
	status = al_psa_status(
		psa_hash_finish(&handle,
				hash_val,
				sizeof(hash_val),
				&hash_len),
		__func__);
	if (status != PSA_SUCCESS) {
		goto err;
	}

	/* Display the SHA-256 hash for debug purposes */
	struct sf_hex_tbl_fmt fmt = {
		.ascii = false,
		.addr_label = true,
		.addr = 0
	};
	sf_hex_tabulate_16(&fmt, hash_val, (size_t)(PSA_HASH_SIZE(alg)));

	return;
err:
	psa_hash_abort(&handle);
	al_dump_log();
}
