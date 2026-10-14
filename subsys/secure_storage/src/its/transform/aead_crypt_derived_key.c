/* Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/transform/aead.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>

LOG_MODULE_DECLARE(secure_storage, CONFIG_SECURE_STORAGE_LOG_LEVEL);

/* One key for every entry rather than one per entry: deriving on each call
 * would cost a PSA key derivation per access, and the entries are already
 * told apart by their nonce and by the UID in the additional data.
 */
static psa_key_id_t s_key_id = PSA_KEY_ID_NULL;
static psa_algorithm_t s_alg = PSA_ALG_NONE;

#define KEY_DERIVATION_LABEL "secure_storage_its"

typedef psa_status_t aead_crypt_fn_t(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				     const uint8_t *nonce, size_t nonce_length,
				     const uint8_t *add_data, size_t add_data_len,
				     const uint8_t *input, size_t input_len, uint8_t *output,
				     size_t output_size, size_t *output_len);

static void log_failed_derivation(const char *step, psa_status_t status)
{
	LOG_ERR("Key derivation %s failed (%d).", step, status);
}

static int derive_key(void)
{
	psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_derivation_step_t label_step;
	psa_key_id_t derivation_key_id;
	psa_algorithm_t kdf_alg;
	psa_key_type_t key_type;
	psa_status_t status;

	secure_storage_its_transform_aead_get_derivation_key(&derivation_key_id, &kdf_alg);

	status = psa_key_derivation_setup(&operation, kdf_alg);
	if (status != PSA_SUCCESS) {
		log_failed_derivation("setup", status);
		goto exit;
	}

	status = psa_key_derivation_input_key(&operation, PSA_KEY_DERIVATION_INPUT_SECRET,
					      derivation_key_id);
	if (status != PSA_SUCCESS) {
		log_failed_derivation("input key", status);
		goto exit;
	}

	/* HKDF takes the label as its info string, the SP 800-108 KDFs as a label. */
	label_step = PSA_ALG_IS_HKDF(kdf_alg) ? PSA_KEY_DERIVATION_INPUT_INFO
					      : PSA_KEY_DERIVATION_INPUT_LABEL;
	status = psa_key_derivation_input_bytes(&operation, label_step, KEY_DERIVATION_LABEL,
						sizeof(KEY_DERIVATION_LABEL) - 1);
	if (status != PSA_SUCCESS) {
		log_failed_derivation("input label", status);
		goto exit;
	}

	secure_storage_its_transform_aead_get_scheme(&key_type, &s_alg);

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_type(&attributes, key_type);
	psa_set_key_algorithm(&attributes, s_alg);
	psa_set_key_bits(&attributes,
			 PSA_BYTES_TO_BITS(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE));

	status = psa_key_derivation_output_key(&attributes, &operation, &s_key_id);
	if (status != PSA_SUCCESS) {
		log_failed_derivation("output key", status);
	}

exit:
	psa_key_derivation_abort(&operation);
	return status;
}
SYS_INIT(derive_key, APPLICATION, CONFIG_SECURE_STORAGE_INIT_PRIORITY);

psa_status_t secure_storage_its_transform_aead_crypt(
		psa_key_usage_t operation, secure_storage_its_uid_t uid,
		const uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE],
		size_t add_data_len, const uint8_t *add_data, size_t input_len,
		const uint8_t *input, size_t output_size, uint8_t *output, size_t *output_len)
{
	aead_crypt_fn_t *aead_crypt =
		(operation == PSA_KEY_USAGE_ENCRYPT) ? psa_aead_encrypt : psa_aead_decrypt;

	if (s_key_id == PSA_KEY_ID_NULL && derive_key() != PSA_SUCCESS) {
		return PSA_ERROR_BAD_STATE;
	}

	return aead_crypt(s_key_id, s_alg, nonce, CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE,
			  add_data, add_data_len, input, input_len, output, output_size, output_len);
}
