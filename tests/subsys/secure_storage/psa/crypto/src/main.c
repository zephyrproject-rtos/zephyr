/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/psa/key_ids.h>
#include <zephyr/sys/util.h>
#include <psa/crypto.h>
#include <psa/internal_trusted_storage.h>
#include <psa/protected_storage.h>

ZTEST_SUITE(secure_storage_psa_crypto, NULL, NULL, NULL, NULL, NULL);

#define ID       ZEPHYR_PSA_APPLICATION_KEY_ID_RANGE_BEGIN
#define KEY_TYPE PSA_KEY_TYPE_AES
#define ALG      PSA_ALG_CBC_NO_PADDING
#define KEY_BITS 256

static void fill_key_attributes(psa_key_attributes_t *key_attributes)
{
	*key_attributes = psa_key_attributes_init();
	psa_set_key_lifetime(key_attributes, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_usage_flags(key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_id(key_attributes, ID);
	psa_set_key_type(key_attributes, KEY_TYPE);
	psa_set_key_algorithm(key_attributes, ALG);
	psa_set_key_bits(key_attributes, KEY_BITS);
}

static void fill_data(uint8_t *data, size_t size)
{
	zassert_equal(psa_generate_random(data, size), PSA_SUCCESS);
}

ZTEST(secure_storage_psa_crypto, test_its_caller_isolation)
{
	psa_status_t ret;
	psa_key_attributes_t key_attributes;
	psa_key_attributes_t retrieved_key_attributes;
	psa_key_id_t key_id;
	uint8_t data[32];
	size_t data_length;
	uint8_t its_data[sizeof(data)];
	uint8_t ps_data[sizeof(data)];

	fill_data(its_data, sizeof(its_data));
	fill_data(ps_data, sizeof(ps_data));
	zassert_true(memcmp(its_data, ps_data, sizeof(data)));
	ret = psa_its_set(ID, sizeof(its_data), its_data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_set(ID, sizeof(ps_data), ps_data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	fill_key_attributes(&key_attributes);
	ret = psa_generate_key(&key_attributes, &key_id);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(key_id, ID);
	ret = psa_purge_key(ID);
	zassert_equal(ret, PSA_SUCCESS);

	ret = psa_its_get(ID, 0, sizeof(data), data, &data_length);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(data_length, sizeof(data));
	zassert_mem_equal(data, its_data, sizeof(data));
	ret = psa_its_remove(ID);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_its_remove(ID);
	zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);

	ret = psa_ps_get(ID, 0, sizeof(data), data, &data_length);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(data_length, sizeof(data));
	zassert_mem_equal(data, ps_data, sizeof(data));
	ret = psa_ps_remove(ID);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_ps_remove(ID);
	zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);

	ret = psa_get_key_attributes(ID, &retrieved_key_attributes);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_mem_equal(&retrieved_key_attributes, &key_attributes, sizeof(key_attributes));
	ret = psa_destroy_key(ID);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_get_key_attributes(ID, &retrieved_key_attributes);
	zassert_equal(ret, PSA_ERROR_INVALID_HANDLE);
}

ZTEST(secure_storage_psa_crypto, test_persistent_key_usage)
{
	psa_status_t ret;
	psa_key_attributes_t key_attributes;
	psa_key_id_t key_id;
	uint8_t key_material[KEY_BITS / BITS_PER_BYTE];

	fill_key_attributes(&key_attributes);
	fill_data(key_material, sizeof(key_material));
	ret = psa_import_key(&key_attributes, key_material, sizeof(key_material), &key_id);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(key_id, ID);
	ret = psa_purge_key(ID);
	zassert_equal(ret, PSA_SUCCESS);

	static uint8_t plaintext[1024];
	static uint8_t ciphertext[PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(KEY_TYPE, ALG, sizeof(plaintext))];
	static uint8_t decrypted_text[sizeof(plaintext)];
	size_t output_length;

	fill_data(plaintext, sizeof(plaintext));
	ret = psa_cipher_encrypt(ID, ALG, plaintext, sizeof(plaintext),
				 ciphertext, sizeof(ciphertext), &output_length);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(output_length, sizeof(ciphertext));
	ret = psa_purge_key(ID);
	zassert_equal(ret, PSA_SUCCESS);

	ret = psa_cipher_decrypt(ID, ALG, ciphertext, output_length,
				 decrypted_text, sizeof(decrypted_text), &output_length);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(output_length, sizeof(plaintext));
	zassert_mem_equal(plaintext, decrypted_text, sizeof(plaintext));
	ret = psa_purge_key(ID);
	zassert_equal(ret, PSA_SUCCESS);

	ret = psa_destroy_key(ID);
	zassert_equal(ret, PSA_SUCCESS);
	ret = psa_destroy_key(ID);
	zassert_equal(ret, PSA_ERROR_INVALID_HANDLE);
}
