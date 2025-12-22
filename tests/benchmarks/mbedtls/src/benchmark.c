/*
 * Copyright (C) 2025, BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <psa/crypto.h>

#define BUF_SIZE	1024
#define LABEL_FORMAT   "%-24s:  "

#define TIMER_DURATION		K_MSEC(1000)
#define TIMER_PERIOD		TIMER_DURATION
volatile int timer_expired;
void timer_expired_callback(struct k_timer *timer)
{
	timer_expired = 1;
}

static struct k_timer timer;

uint8_t in_buf[BUF_SIZE];
uint8_t out_buf[BUF_SIZE];

#define COMPUTE_THROUGHPUT(LABEL, CODE)					\
do {									\
	unsigned long i;						\
									\
	printk(LABEL_FORMAT, LABEL);					\
									\
	status = PSA_SUCCESS;						\
	timer_expired = 0;						\
	k_timer_start(&timer, TIMER_DURATION, TIMER_PERIOD);		\
	for (i = 1; status == PSA_SUCCESS && !timer_expired; i++) {	\
		status = CODE;						\
	}								\
									\
	if (status != PSA_SUCCESS) {					\
		k_timer_stop(&timer);					\
		printk("Fail (%d)\n", status);				\
	} else {							\
		printk("%lu Ki/s\n", (i * BUF_SIZE) / 1024);		\
	}								\
} while (0)

static psa_status_t make_cipher_key(psa_key_type_t key_type,
				    psa_algorithm_t alg,
				    mbedtls_svc_key_id_t *key_id)
{
	uint8_t tmp_key[32] = { 0x5 };
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&key_attr, key_type);
	psa_set_key_algorithm(&key_attr, alg);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_ENCRYPT);
	if (psa_import_key(&key_attr, tmp_key, sizeof(tmp_key), key_id) != PSA_SUCCESS) {
		printk("Key import failed\n");
		return PSA_ERROR_GENERIC_ERROR;
	}

	return PSA_SUCCESS;
}

int main(void)
{
	mbedtls_svc_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;
	psa_status_t status = PSA_SUCCESS;
	size_t out_len;

	k_timer_init(&timer, timer_expired_callback, NULL);

	memset(in_buf, 0xaa, sizeof(in_buf));

	/* HASH */

	COMPUTE_THROUGHPUT("SHA-1",
		psa_hash_compute(PSA_ALG_SHA_1, in_buf, sizeof(in_buf),
				 out_buf, sizeof(out_buf), &out_len)
	);

	COMPUTE_THROUGHPUT("SHA-224",
		psa_hash_compute(PSA_ALG_SHA_224, in_buf, sizeof(in_buf),
				 out_buf, sizeof(out_buf), &out_len)
	);

	COMPUTE_THROUGHPUT("SHA-256",
		psa_hash_compute(PSA_ALG_SHA_256, in_buf, sizeof(in_buf),
				 out_buf, sizeof(out_buf), &out_len)
	);

	COMPUTE_THROUGHPUT("SHA-384",
		psa_hash_compute(PSA_ALG_SHA_384, in_buf, sizeof(in_buf),
				 out_buf, sizeof(out_buf), &out_len)
	);

	COMPUTE_THROUGHPUT("SHA-512",
		psa_hash_compute(PSA_ALG_SHA_512, in_buf, sizeof(in_buf),
				 out_buf, sizeof(out_buf), &out_len)
	);

	/* Ciphers */

	status = make_cipher_key(PSA_KEY_TYPE_AES, PSA_ALG_ECB_NO_PADDING, &key_id);
	if (status == PSA_SUCCESS) {
		COMPUTE_THROUGHPUT("AES-256-ECB",
			psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
					   in_buf, sizeof(in_buf),
					   out_buf, sizeof(out_buf), &out_len)
		);
		psa_destroy_key(key_id);
	} else {
		printk("Failed to import AES key (%d)", status);
	}

	status = make_cipher_key(PSA_KEY_TYPE_ARIA, PSA_ALG_ECB_NO_PADDING, &key_id);
	if (status == PSA_SUCCESS) {
		COMPUTE_THROUGHPUT("ARIA-256-ECB",
			psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
					   in_buf, sizeof(in_buf),
					   out_buf, sizeof(out_buf), &out_len)
		);
		psa_destroy_key(key_id);
	} else {
		printk("Failed to import ARIA key (%d)", status);
	}

	status = make_cipher_key(PSA_KEY_TYPE_CAMELLIA, PSA_ALG_ECB_NO_PADDING, &key_id);
	if (status == PSA_SUCCESS) {
		COMPUTE_THROUGHPUT("CAMELLIA-256-ECB",
			psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
					   in_buf, sizeof(in_buf),
					   out_buf, sizeof(out_buf), &out_len)
		);
		psa_destroy_key(key_id);
	} else {
		printk("Failed to import Camellia key (%d)", status);
	}

	printk("Benchmark completed\n");
	return 0;
}
