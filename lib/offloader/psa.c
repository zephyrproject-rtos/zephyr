/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <psa/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/offloader/engine.h>
#include <zephyr/offloader/shim/psa.h>

/* I'm not sure more threads will acctually help unless the PSA backend is only in software? */
#define NUMBER_OF_OFFLOADER_PSA_THREADS 1
OFFLOADER_ENGINE_DEFINE(psa_offload_engine, NUMBER_OF_OFFLOADER_PSA_THREADS,
			CONFIG_OFFLOADER_PSA_STACK_SIZE, CONFIG_OFFLOADER_PSA_PRIO);

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

	a->ret = psa_aead_encrypt(a->key, a->alg, a->nonce, a->nonce_length, a->additional_data,
				  a->additional_data_length, a->plaintext, a->plaintext_length,
				  a->ciphertext, a->ciphertext_size, a->ciphertext_length);
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

	a->ret = psa_aead_decrypt(a->key, a->alg, a->nonce, a->nonce_length, a->additional_data,
				  a->additional_data_length, a->ciphertext, a->ciphertext_length,
				  a->plaintext, a->plaintext_size, a->plaintext_length);
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

	a->ret = psa_mac_compute(a->key, a->alg, a->input, a->input_length, a->mac, a->mac_size,
				 a->mac_length);
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
					  const uint8_t *input, size_t input_length,
					  uint8_t *output, size_t output_size,
					  size_t *output_length)
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
					  const uint8_t *input, size_t input_length,
					  uint8_t *output, size_t output_size,
					  size_t *output_length)
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
