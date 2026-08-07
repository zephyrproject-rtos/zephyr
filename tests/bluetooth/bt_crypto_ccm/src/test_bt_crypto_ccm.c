/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/crypto.h>

#include "common/bt_str.h"

#include "test_vectors.h"

LOG_MODULE_REGISTER(test_bt_crypto_ccm, LOG_LEVEL_INF);

ZTEST_SUITE(bt_crypto_ccm, NULL, NULL, NULL, NULL, NULL);

ZTEST(bt_crypto_ccm, test_result_rfc_test_vectors)
{
	for (int i = 0; i < NUMBER_OF_TEST; i++) {
		LOG_DBG("=============== Packet Vector #%d ==================", i + 1);
		int err;
		struct test_data *p = input_packets[i];

		p->expected_output_len = p->input_len + p->mic_len;

		const uint8_t *aad = p->input;

		uint8_t *encrypted_data =
			(uint8_t *)malloc(p->expected_output_len * sizeof(uint8_t));

		memcpy(encrypted_data, p->input, p->input_len);

		err = bt_ccm_encrypt(p->key, p->nonce, &p->input[p->aad_len],
				     p->input_len - p->aad_len, aad, p->aad_len,
				     &encrypted_data[p->aad_len], p->mic_len);
		zassert_true(err == 0, "CCM Encrypt failed for packet vector %d with error %d",
			     i + 1, err);

		LOG_DBG("encrypted data %s (len: %d)",
			bt_hex(encrypted_data, p->expected_output_len), p->expected_output_len);
		LOG_DBG("expected data  %s (len: %d)",
			bt_hex(p->expected_output, p->expected_output_len), p->expected_output_len);

		zassert_mem_equal(encrypted_data, p->expected_output, p->expected_output_len,
				  "Encrypted data are not correct for packet vector %d", i + 1);

		uint8_t *decrypted_data = (uint8_t *)malloc(p->input_len * sizeof(uint8_t));

		memcpy(decrypted_data, encrypted_data, p->aad_len);

		err = bt_ccm_decrypt(p->key, p->nonce, &encrypted_data[p->aad_len],
				     p->input_len - p->aad_len, aad, p->aad_len,
				     &decrypted_data[p->aad_len], p->mic_len);
		zassert_true(err == 0, "CCM Decrypt failed for packet vector %d with error %d",
			     i + 1, err);

		LOG_DBG("decrypted data %s (len: %d)", bt_hex(decrypted_data, p->input_len),
			p->input_len);
		LOG_DBG("expected data %s (len: %d)", bt_hex(p->input, p->input_len), p->input_len);

		zassert_mem_equal(decrypted_data, p->input, p->input_len,
				  "Decrypted data are not correct for packet vector %d", i + 1);

		free(encrypted_data);
		free(decrypted_data);
	}
}
