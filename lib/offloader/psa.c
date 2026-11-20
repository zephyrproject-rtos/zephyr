/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <psa/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/offloader/engine.h>

OFFLOADER_ENGINE_DEFINE(psa_offload_engine, CONFIG_OFFLOADER_PSA_WORKERS,
			CONFIG_OFFLOADER_PSA_STACK_SIZE);

struct crypto_init_args {
	psa_status_t ret;
};

static void crypto_init(void *args)
{
	struct crypto_init_args *a = args;

	a->ret = psa_crypto_init();
}

psa_status_t offloader_psa_crypto_init(void)
{
	struct crypto_init_args a;

	offloader_dispatch(&psa_offload_engine, crypto_init, &a);

	return a.ret;
}

struct import_key_args {
	const psa_key_attributes_t *attributes;
	const uint8_t *data;
	size_t data_length;
	psa_key_id_t *key;
	psa_status_t ret;
};

static void import_key(void *args)
{
	struct import_key_args *a = args;

	a->ret = psa_import_key(a->attributes, a->data, a->data_length, a->key);
}

psa_status_t offloader_psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				   size_t data_length, psa_key_id_t *key)
{
	struct import_key_args a = {
		.attributes = attributes,
		.data = data,
		.data_length = data_length,
		.key = key,
	};

	offloader_dispatch(&psa_offload_engine, import_key, &a);

	return a.ret;
}

struct destroy_key_args {
	psa_key_id_t key;
	psa_status_t ret;
};

static void destroy_key(void *args)
{
	struct destroy_key_args *a = args;

	a->ret = psa_destroy_key(a->key);
}

psa_status_t offloader_psa_destroy_key(psa_key_id_t key)
{
	struct destroy_key_args a = {
		.key = key,
	};

	offloader_dispatch(&psa_offload_engine, destroy_key, &a);

	return a.ret;
}

struct generate_random_args {
	uint8_t *output;
	size_t output_size;
	psa_status_t ret;
};

static void generate_random(void *args)
{
	struct generate_random_args *a = args;

	a->ret = psa_generate_random(a->output, a->output_size);
}

psa_status_t offloader_psa_generate_random(uint8_t *output, size_t output_size)
{
	struct generate_random_args a = {
		.output = output,
		.output_size = output_size,
	};

	offloader_dispatch(&psa_offload_engine, generate_random, &a);

	return a.ret;
}

struct hash_compute_args {
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	uint8_t *hash;
	size_t hash_size;
	size_t *hash_length;
	psa_status_t ret;
};

static void hash_compute(void *args)
{
	struct hash_compute_args *a = args;

	a->ret = psa_hash_compute(a->alg, a->input, a->input_length, a->hash, a->hash_size,
					 a->hash_length);
}

psa_status_t offloader_psa_hash_compute(psa_algorithm_t alg, const uint8_t *input,
				     size_t input_length, uint8_t *hash, size_t hash_size,
				     size_t *hash_length)
{
	struct hash_compute_args a = {
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.hash = hash,
		.hash_size = hash_size,
		.hash_length = hash_length,
	};

	offloader_dispatch(&psa_offload_engine, hash_compute, &a);

	return a.ret;
}

struct sign_hash_args {
	psa_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *hash;
	size_t hash_length;
	uint8_t *signature;
	size_t signature_size;
	size_t *signature_length;
	psa_status_t ret;
};

static void sign_hash(void *args)
{
	struct sign_hash_args *a = args;

	a->ret = psa_sign_hash(a->key, a->alg, a->hash, a->hash_length, a->signature,
				      a->signature_size, a->signature_length);
}

psa_status_t offloader_psa_sign_hash(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
				  size_t hash_length, uint8_t *signature, size_t signature_size,
				  size_t *signature_length)
{
	struct sign_hash_args a = {
		.key = key,
		.alg = alg,
		.hash = hash,
		.hash_length = hash_length,
		.signature = signature,
		.signature_size = signature_size,
		.signature_length = signature_length,
	};

	offloader_dispatch(&psa_offload_engine, sign_hash, &a);

	return a.ret;
}

struct verify_hash_args {
	psa_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *hash;
	size_t hash_length;
	const uint8_t *signature;
	size_t signature_length;
	psa_status_t ret;
};

static void verify_hash(void *args)
{
	struct verify_hash_args *a = args;

	a->ret = psa_verify_hash(a->key, a->alg, a->hash, a->hash_length, a->signature,
					a->signature_length);
}

psa_status_t offloader_psa_verify_hash(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
				    size_t hash_length, const uint8_t *signature,
				    size_t signature_length)
{
	struct verify_hash_args a = {
		.key = key,
		.alg = alg,
		.hash = hash,
		.hash_length = hash_length,
		.signature = signature,
		.signature_length = signature_length,
	};

	offloader_dispatch(&psa_offload_engine, verify_hash, &a);

	return a.ret;
}

struct aead_encrypt_args {
	psa_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *nonce;
	size_t nonce_length;
	const uint8_t *additional_data;
	size_t additional_data_length;
	const uint8_t *plaintext;
	size_t plaintext_length;
	uint8_t *ciphertext;
	size_t ciphertext_size;
	size_t *ciphertext_length;
	psa_status_t ret;
};

static void aead_encrypt(void *args)
{
	struct aead_encrypt_args *a = args;

	a->ret = psa_aead_encrypt(a->key, a->alg, a->nonce, a->nonce_length,
					 a->additional_data, a->additional_data_length,
					 a->plaintext, a->plaintext_length, a->ciphertext,
					 a->ciphertext_size, a->ciphertext_length);
}

psa_status_t offloader_psa_aead_encrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
				     size_t nonce_length, const uint8_t *additional_data,
				     size_t additional_data_length, const uint8_t *plaintext,
				     size_t plaintext_length, uint8_t *ciphertext,
				     size_t ciphertext_size, size_t *ciphertext_length)
{
	struct aead_encrypt_args a = {
		.key = key,
		.alg = alg,
		.nonce = nonce,
		.nonce_length = nonce_length,
		.additional_data = additional_data,
		.additional_data_length = additional_data_length,
		.plaintext = plaintext,
		.plaintext_length = plaintext_length,
		.ciphertext = ciphertext,
		.ciphertext_size = ciphertext_size,
		.ciphertext_length = ciphertext_length,
	};

	offloader_dispatch(&psa_offload_engine, aead_encrypt, &a);

	return a.ret;
}

struct aead_decrypt_args {
	psa_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *nonce;
	size_t nonce_length;
	const uint8_t *additional_data;
	size_t additional_data_length;
	const uint8_t *ciphertext;
	size_t ciphertext_length;
	uint8_t *plaintext;
	size_t plaintext_size;
	size_t *plaintext_length;
	psa_status_t ret;
};

static void aead_decrypt(void *args)
{
	struct aead_decrypt_args *a = args;

	a->ret = psa_aead_decrypt(a->key, a->alg, a->nonce, a->nonce_length,
					 a->additional_data, a->additional_data_length,
					 a->ciphertext, a->ciphertext_length, a->plaintext,
					 a->plaintext_size, a->plaintext_length);
}

psa_status_t offloader_psa_aead_decrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
				     size_t nonce_length, const uint8_t *additional_data,
				     size_t additional_data_length, const uint8_t *ciphertext,
				     size_t ciphertext_length, uint8_t *plaintext,
				     size_t plaintext_size, size_t *plaintext_length)
{
	struct aead_decrypt_args a = {
		.key = key,
		.alg = alg,
		.nonce = nonce,
		.nonce_length = nonce_length,
		.additional_data = additional_data,
		.additional_data_length = additional_data_length,
		.ciphertext = ciphertext,
		.ciphertext_length = ciphertext_length,
		.plaintext = plaintext,
		.plaintext_size = plaintext_size,
		.plaintext_length = plaintext_length,
	};

	offloader_dispatch(&psa_offload_engine, aead_decrypt, &a);

	return a.ret;
}

struct mac_compute_args {
	psa_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	uint8_t *mac;
	size_t mac_size;
	size_t *mac_length;
	psa_status_t ret;
};

static void mac_compute(void *args)
{
	struct mac_compute_args *a = args;

	a->ret = psa_mac_compute(a->key, a->alg, a->input, a->input_length, a->mac,
					a->mac_size, a->mac_length);
}

psa_status_t offloader_psa_mac_compute(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				    size_t input_length, uint8_t *mac, size_t mac_size,
				    size_t *mac_length)
{
	struct mac_compute_args a = {
		.key = key,
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.mac = mac,
		.mac_size = mac_size,
		.mac_length = mac_length,
	};

	offloader_dispatch(&psa_offload_engine, mac_compute, &a);

	return a.ret;
}

struct cipher_encrypt_args {
	psa_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	uint8_t *output;
	size_t output_size;
	size_t *output_length;
	psa_status_t ret;
};

static void cipher_encrypt(void *args)
{
	struct cipher_encrypt_args *a = args;

	a->ret = psa_cipher_encrypt(a->key, a->alg, a->input, a->input_length, a->output,
					   a->output_size, a->output_length);
}

psa_status_t offloader_psa_cipher_encrypt(psa_key_id_t key, psa_algorithm_t alg,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length)
{
	struct cipher_encrypt_args a = {
		.key = key,
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.output = output,
		.output_size = output_size,
		.output_length = output_length,
	};

	offloader_dispatch(&psa_offload_engine, cipher_encrypt, &a);

	return a.ret;
}

struct cipher_decrypt_args {
	psa_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	uint8_t *output;
	size_t output_size;
	size_t *output_length;
	psa_status_t ret;
};

static void cipher_decrypt(void *args)
{
	struct cipher_decrypt_args *a = args;

	a->ret = psa_cipher_decrypt(a->key, a->alg, a->input, a->input_length, a->output,
					   a->output_size, a->output_length);
}

psa_status_t offloader_psa_cipher_decrypt(psa_key_id_t key, psa_algorithm_t alg,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length)
{
	struct cipher_decrypt_args a = {
		.key = key,
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.output = output,
		.output_size = output_size,
		.output_length = output_length,
	};

	offloader_dispatch(&psa_offload_engine, cipher_decrypt, &a);

	return a.ret;
}

struct generate_key_args {
	const psa_key_attributes_t *attributes;
	mbedtls_svc_key_id_t *key;
	psa_status_t ret;
};

static void generate_key(void *args)
{
	struct generate_key_args *a = args;

	a->ret = psa_generate_key(a->attributes, a->key);
}

psa_status_t offloader_psa_generate_key(const psa_key_attributes_t *attributes,
				     mbedtls_svc_key_id_t *key)
{
	struct generate_key_args a = {
		.attributes = attributes,
		.key = key,
	};

	offloader_dispatch(&psa_offload_engine, generate_key, &a);

	return a.ret;
}

struct generate_key_custom_args {
	const psa_key_attributes_t *attributes;
	const psa_custom_key_parameters_t *custom;
	const uint8_t *custom_data;
	size_t custom_data_length;
	mbedtls_svc_key_id_t *key;
	psa_status_t ret;
};

static void generate_key_custom(void *args)
{
	struct generate_key_custom_args *a = args;

	a->ret = psa_generate_key_custom(a->attributes, a->custom, a->custom_data,
						a->custom_data_length, a->key);
}

psa_status_t offloader_psa_generate_key_custom(const psa_key_attributes_t *attributes,
					    const psa_custom_key_parameters_t *custom,
					    const uint8_t *custom_data, size_t custom_data_length,
					    mbedtls_svc_key_id_t *key)
{
	struct generate_key_custom_args a = {
		.attributes = attributes,
		.custom = custom,
		.custom_data = custom_data,
		.custom_data_length = custom_data_length,
		.key = key,
	};

	offloader_dispatch(&psa_offload_engine, generate_key_custom, &a);

	return a.ret;
}

struct sign_message_args {
	mbedtls_svc_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	uint8_t *signature;
	size_t signature_size;
	size_t *signature_length;
	psa_status_t ret;
};

static void sign_message(void *args)
{
	struct sign_message_args *a = args;

	a->ret = psa_sign_message(a->key, a->alg, a->input, a->input_length, a->signature,
					 a->signature_size, a->signature_length);
}

psa_status_t offloader_psa_sign_message(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				     const uint8_t *input, size_t input_length, uint8_t *signature,
				     size_t signature_size, size_t *signature_length)
{
	struct sign_message_args a = {
		.key = key,
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.signature = signature,
		.signature_size = signature_size,
		.signature_length = signature_length,
	};

	offloader_dispatch(&psa_offload_engine, sign_message, &a);

	return a.ret;
}

struct verify_message_args {
	mbedtls_svc_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	const uint8_t *signature;
	size_t signature_length;
	psa_status_t ret;
};

static void verify_message(void *args)
{
	struct verify_message_args *a = args;

	a->ret = psa_verify_message(a->key, a->alg, a->input, a->input_length,
					   a->signature, a->signature_length);
}

psa_status_t offloader_psa_verify_message(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				       const uint8_t *input, size_t input_length,
				       const uint8_t *signature, size_t signature_length)
{
	struct verify_message_args a = {
		.key = key,
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.signature = signature,
		.signature_length = signature_length,
	};

	offloader_dispatch(&psa_offload_engine, verify_message, &a);

	return a.ret;
}

struct asymmetric_encrypt_args {
	mbedtls_svc_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	const uint8_t *salt;
	size_t salt_length;
	uint8_t *output;
	size_t output_size;
	size_t *output_length;
	psa_status_t ret;
};

static void asymmetric_encrypt(void *args)
{
	struct asymmetric_encrypt_args *a = args;

	a->ret = psa_asymmetric_encrypt(a->key, a->alg, a->input, a->input_length, a->salt,
					       a->salt_length, a->output, a->output_size,
					       a->output_length);
}

psa_status_t offloader_psa_asymmetric_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
					   const uint8_t *input, size_t input_length,
					   const uint8_t *salt, size_t salt_length,
					   uint8_t *output, size_t output_size,
					   size_t *output_length)
{
	struct asymmetric_encrypt_args a = {
		.key = key,
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.salt = salt,
		.salt_length = salt_length,
		.output = output,
		.output_size = output_size,
		.output_length = output_length,
	};

	offloader_dispatch(&psa_offload_engine, asymmetric_encrypt, &a);

	return a.ret;
}

struct asymmetric_decrypt_args {
	mbedtls_svc_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	const uint8_t *salt;
	size_t salt_length;
	uint8_t *output;
	size_t output_size;
	size_t *output_length;
	psa_status_t ret;
};

static void asymmetric_decrypt(void *args)
{
	struct asymmetric_decrypt_args *a = args;

	a->ret = psa_asymmetric_decrypt(a->key, a->alg, a->input, a->input_length, a->salt,
					       a->salt_length, a->output, a->output_size,
					       a->output_length);
}

psa_status_t offloader_psa_asymmetric_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
					   const uint8_t *input, size_t input_length,
					   const uint8_t *salt, size_t salt_length,
					   uint8_t *output, size_t output_size,
					   size_t *output_length)
{
	struct asymmetric_decrypt_args a = {
		.key = key,
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.salt = salt,
		.salt_length = salt_length,
		.output = output,
		.output_size = output_size,
		.output_length = output_length,
	};

	offloader_dispatch(&psa_offload_engine, asymmetric_decrypt, &a);

	return a.ret;
}

struct raw_key_agreement_args {
	psa_algorithm_t alg;
	mbedtls_svc_key_id_t private_key;
	const uint8_t *peer_key;
	size_t peer_key_length;
	uint8_t *output;
	size_t output_size;
	size_t *output_length;
	psa_status_t ret;
};

static void raw_key_agreement(void *args)
{
	struct raw_key_agreement_args *a = args;

	a->ret = psa_raw_key_agreement(a->alg, a->private_key, a->peer_key,
					      a->peer_key_length, a->output, a->output_size,
					      a->output_length);
}

psa_status_t offloader_psa_raw_key_agreement(psa_algorithm_t alg, mbedtls_svc_key_id_t private_key,
					  const uint8_t *peer_key, size_t peer_key_length,
					  uint8_t *output, size_t output_size,
					  size_t *output_length)
{
	struct raw_key_agreement_args a = {
		.alg = alg,
		.private_key = private_key,
		.peer_key = peer_key,
		.peer_key_length = peer_key_length,
		.output = output,
		.output_size = output_size,
		.output_length = output_length,
	};

	offloader_dispatch(&psa_offload_engine, raw_key_agreement, &a);

	return a.ret;
}

struct export_public_key_args {
	mbedtls_svc_key_id_t key;
	uint8_t *data;
	size_t data_size;
	size_t *data_length;
	psa_status_t ret;
};

static void export_public_key(void *args)
{
	struct export_public_key_args *a = args;

	a->ret = psa_export_public_key(a->key, a->data, a->data_size, a->data_length);
}

psa_status_t offloader_psa_export_public_key(mbedtls_svc_key_id_t key, uint8_t *data,
					  size_t data_size, size_t *data_length)
{
	struct export_public_key_args a = {
		.key = key,
		.data = data,
		.data_size = data_size,
		.data_length = data_length,
	};

	offloader_dispatch(&psa_offload_engine, export_public_key, &a);

	return a.ret;
}

struct export_key_args {
	mbedtls_svc_key_id_t key;
	uint8_t *data;
	size_t data_size;
	size_t *data_length;
	psa_status_t ret;
};

static void export_key(void *args)
{
	struct export_key_args *a = args;

	a->ret = psa_export_key(a->key, a->data, a->data_size, a->data_length);
}

psa_status_t offloader_psa_export_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
				   size_t *data_length)
{
	struct export_key_args a = {
		.key = key,
		.data = data,
		.data_size = data_size,
		.data_length = data_length,
	};

	offloader_dispatch(&psa_offload_engine, export_key, &a);

	return a.ret;
}

struct hash_compare_args {
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	const uint8_t *hash;
	size_t hash_length;
	psa_status_t ret;
};

static void hash_compare(void *args)
{
	struct hash_compare_args *a = args;

	a->ret = psa_hash_compare(a->alg, a->input, a->input_length, a->hash,
					 a->hash_length);
}

psa_status_t offloader_psa_hash_compare(psa_algorithm_t alg, const uint8_t *input,
				     size_t input_length, const uint8_t *hash, size_t hash_length)
{
	struct hash_compare_args a = {
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.hash = hash,
		.hash_length = hash_length,
	};

	offloader_dispatch(&psa_offload_engine, hash_compare, &a);

	return a.ret;
}

struct mac_verify_args {
	mbedtls_svc_key_id_t key;
	psa_algorithm_t alg;
	const uint8_t *input;
	size_t input_length;
	const uint8_t *mac;
	size_t mac_length;
	psa_status_t ret;
};

static void mac_verify(void *args)
{
	struct mac_verify_args *a = args;

	a->ret = psa_mac_verify(a->key, a->alg, a->input, a->input_length, a->mac,
				       a->mac_length);
}

psa_status_t offloader_psa_mac_verify(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				   const uint8_t *input, size_t input_length, const uint8_t *mac,
				   size_t mac_length)
{
	struct mac_verify_args a = {
		.key = key,
		.alg = alg,
		.input = input,
		.input_length = input_length,
		.mac = mac,
		.mac_length = mac_length,
	};

	offloader_dispatch(&psa_offload_engine, mac_verify, &a);

	return a.ret;
}

struct key_derivation_key_agreement_args {
	psa_key_derivation_operation_t *operation;
	psa_key_derivation_step_t step;
	mbedtls_svc_key_id_t private_key;
	const uint8_t *peer_key;
	size_t peer_key_length;
	psa_status_t ret;
};

static void key_derivation_key_agreement(void *args)
{
	struct key_derivation_key_agreement_args *a = args;

	a->ret = psa_key_derivation_key_agreement(a->operation, a->step, a->private_key,
							 a->peer_key, a->peer_key_length);
}

psa_status_t offloader_psa_key_derivation_key_agreement(psa_key_derivation_operation_t *operation,
						     psa_key_derivation_step_t step,
						     mbedtls_svc_key_id_t private_key,
						     const uint8_t *peer_key,
						     size_t peer_key_length)
{
	struct key_derivation_key_agreement_args a = {
		.operation = operation,
		.step = step,
		.private_key = private_key,
		.peer_key = peer_key,
		.peer_key_length = peer_key_length,
	};

	offloader_dispatch(&psa_offload_engine, key_derivation_key_agreement, &a);

	return a.ret;
}

struct key_derivation_output_key_args {
	const psa_key_attributes_t *attributes;
	psa_key_derivation_operation_t *operation;
	mbedtls_svc_key_id_t *key;
	psa_status_t ret;
};

static void key_derivation_output_key(void *args)
{
	struct key_derivation_output_key_args *a = args;

	a->ret = psa_key_derivation_output_key(a->attributes, a->operation, a->key);
}

psa_status_t offloader_psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
						  psa_key_derivation_operation_t *operation,
						  mbedtls_svc_key_id_t *key)
{
	struct key_derivation_output_key_args a = {
		.attributes = attributes,
		.operation = operation,
		.key = key,
	};

	offloader_dispatch(&psa_offload_engine, key_derivation_output_key, &a);

	return a.ret;
}
