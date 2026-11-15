/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <psa/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/call_offloader/call_offloader.h>

CALL_OFFLOADER_ENGINE_DEFINE(psa_engine, CONFIG_CALL_OFFLOADER_PSA_WORKERS,
			     CONFIG_CALL_OFFLOADER_PSA_STACK_SIZE);

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_crypto_init)
{
	return psa_crypto_init();
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_import_key,
			(const psa_key_attributes_t *) attributes, (const uint8_t *) data,
			(size_t) data_length, (psa_key_id_t *) key)
{
	return psa_import_key(attributes, data, data_length, key);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_destroy_key, (psa_key_id_t) key)
{
	return psa_destroy_key(key);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_generate_random, (uint8_t *) output,
			(size_t) output_size)
{
	return psa_generate_random(output, output_size);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_hash_compute, (psa_algorithm_t) alg,
			(const uint8_t *) input, (size_t) input_length, (uint8_t *) hash,
			(size_t) hash_size, (size_t *) hash_length)
{
	return psa_hash_compute(alg, input, input_length, hash, hash_size, hash_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_sign_hash, (psa_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) hash, (size_t) hash_length,
			(uint8_t *) signature, (size_t) signature_size, (size_t *) signature_length)
{
	return psa_sign_hash(key, alg, hash, hash_length, signature, signature_size,
			     signature_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_verify_hash, (psa_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) hash, (size_t) hash_length,
			(const uint8_t *) signature, (size_t) signature_length)
{
	return psa_verify_hash(key, alg, hash, hash_length, signature, signature_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_aead_encrypt, (psa_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) nonce, (size_t) nonce_length,
			(const uint8_t *) additional_data, (size_t) additional_data_length,
			(const uint8_t *) plaintext, (size_t) plaintext_length,
			(uint8_t *) ciphertext, (size_t) ciphertext_size,
			(size_t *) ciphertext_length)
{
	return psa_aead_encrypt(key, alg, nonce, nonce_length, additional_data,
				additional_data_length, plaintext, plaintext_length, ciphertext,
				ciphertext_size, ciphertext_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_aead_decrypt, (psa_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) nonce, (size_t) nonce_length,
			(const uint8_t *) additional_data, (size_t) additional_data_length,
			(const uint8_t *) ciphertext, (size_t) ciphertext_length,
			(uint8_t *) plaintext, (size_t) plaintext_size, (size_t *) plaintext_length)
{
	return psa_aead_decrypt(key, alg, nonce, nonce_length, additional_data,
				additional_data_length, ciphertext, ciphertext_length, plaintext,
				plaintext_size, plaintext_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_mac_compute, (psa_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) input, (size_t) input_length,
			(uint8_t *) mac, (size_t) mac_size, (size_t *) mac_length)
{
	return psa_mac_compute(key, alg, input, input_length, mac, mac_size, mac_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_cipher_encrypt, (psa_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) input, (size_t) input_length,
			(uint8_t *) output, (size_t) output_size, (size_t *) output_length)
{
	return psa_cipher_encrypt(key, alg, input, input_length, output, output_size,
				  output_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_cipher_decrypt, (psa_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) input, (size_t) input_length,
			(uint8_t *) output, (size_t) output_size, (size_t *) output_length)
{
	return psa_cipher_decrypt(key, alg, input, input_length, output, output_size,
				  output_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_generate_key,
			(const psa_key_attributes_t *) attributes, (mbedtls_svc_key_id_t *) key)
{
	return psa_generate_key(attributes, key);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_generate_key_custom,
			(const psa_key_attributes_t *) attributes,
			(const psa_custom_key_parameters_t *) custom, (const uint8_t *) custom_data,
			(size_t) custom_data_length, (mbedtls_svc_key_id_t *) key)
{
	return psa_generate_key_custom(attributes, custom, custom_data, custom_data_length, key);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_sign_message, (mbedtls_svc_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) input, (size_t) input_length,
			(uint8_t *) signature, (size_t) signature_size, (size_t *) signature_length)
{
	return psa_sign_message(key, alg, input, input_length, signature, signature_size,
				signature_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_verify_message, (mbedtls_svc_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) input, (size_t) input_length,
			(const uint8_t *) signature, (size_t) signature_length)
{
	return psa_verify_message(key, alg, input, input_length, signature, signature_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_asymmetric_encrypt,
			(mbedtls_svc_key_id_t) key, (psa_algorithm_t) alg, (const uint8_t *) input,
			(size_t) input_length, (const uint8_t *) salt, (size_t) salt_length,
			(uint8_t *) output, (size_t) output_size, (size_t *) output_length)
{
	return psa_asymmetric_encrypt(key, alg, input, input_length, salt, salt_length, output,
				      output_size, output_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_asymmetric_decrypt,
			(mbedtls_svc_key_id_t) key, (psa_algorithm_t) alg, (const uint8_t *) input,
			(size_t) input_length, (const uint8_t *) salt, (size_t) salt_length,
			(uint8_t *) output, (size_t) output_size, (size_t *) output_length)
{
	return psa_asymmetric_decrypt(key, alg, input, input_length, salt, salt_length, output,
				      output_size, output_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_raw_key_agreement, (psa_algorithm_t) alg,
			(mbedtls_svc_key_id_t) private_key, (const uint8_t *) peer_key,
			(size_t) peer_key_length, (uint8_t *) output, (size_t) output_size,
			(size_t *) output_length)
{
	return psa_raw_key_agreement(alg, private_key, peer_key, peer_key_length, output,
				     output_size, output_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_export_public_key, (mbedtls_svc_key_id_t) key,
			(uint8_t *) data, (size_t) data_size, (size_t *) data_length)
{
	return psa_export_public_key(key, data, data_size, data_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_export_key, (mbedtls_svc_key_id_t) key,
			(uint8_t *) data, (size_t) data_size, (size_t *) data_length)
{
	return psa_export_key(key, data, data_size, data_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_hash_compare, (psa_algorithm_t) alg,
			(const uint8_t *) input, (size_t) input_length, (const uint8_t *) hash,
			(size_t) hash_length)
{
	return psa_hash_compare(alg, input, input_length, hash, hash_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_mac_verify, (mbedtls_svc_key_id_t) key,
			(psa_algorithm_t) alg, (const uint8_t *) input, (size_t) input_length,
			(const uint8_t *) mac, (size_t) mac_length)
{
	return psa_mac_verify(key, alg, input, input_length, mac, mac_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_key_derivation_key_agreement,
			(psa_key_derivation_operation_t *) operation,
			(psa_key_derivation_step_t) step, (mbedtls_svc_key_id_t) private_key,
			(const uint8_t *) peer_key, (size_t) peer_key_length)
{
	return psa_key_derivation_key_agreement(operation, step, private_key, peer_key,
						peer_key_length);
}

CALL_OFFLOADER_WRAP(psa_engine, psa_status_t, psa_key_derivation_output_key,
			(const psa_key_attributes_t *) attributes,
			(psa_key_derivation_operation_t *) operation, (mbedtls_svc_key_id_t *) key)
{
	return psa_key_derivation_output_key(attributes, operation, key);
}
