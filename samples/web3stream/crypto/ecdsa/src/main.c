/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "psa/crypto.h"
#include <string.h>

int main(void)
{
	psa_status_t status = 0;
	enum {
        key_bits = 256,
    };
    static uint8_t exported[PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(key_bits)];
	static uint8_t exported_pairkey[PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(key_bits)];
	unsigned char message[] = "iotex_ecdsa_test_only";
	unsigned char sign[128] = {0};
    size_t  sign_length   = 0;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	size_t exported_length = 0;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		printk("Failed to initialize PSA Crypto %x\n", status);
		return (-1);
	}
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_K1));
	psa_set_key_bits(&attributes, key_bits);
	status = psa_generate_key(&attributes, &key_id);
	if (status != PSA_SUCCESS) {
		printk("Failed to generate key %d\n", status);
		return (-2);		
	}
	printk("Success to generate a key pair: key id : %d\n", (int)key_id);
	status = psa_export_key(key_id, exported_pairkey, sizeof(exported_pairkey), &exported_length);
	if (status != PSA_SUCCESS) {
		printk("Failed to export pair key %d\n", (int)status);
		return (-6);
	}
	printk("Exported a pair key len %d\n", (int)exported_length);
	for (int i = 0; i < exported_length; i++)
	{
		printk("%02x ", exported_pairkey[i]);
	}
	printk("\n");
	status = psa_export_public_key(key_id, exported, sizeof(exported), &exported_length);
	if (status != PSA_SUCCESS) {
		printk("Failed to export public key %d\n", status);
		return (-5);
	}
	printk("Exported a public key len %d\n", (int)exported_length);
	for (int i = 0; i < exported_length; i++)
	{
		printk("%02x ", exported[i]);
	}
	printk("\n");

	status = psa_sign_message( key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), message, strlen((const char *)message), 
								(unsigned char *)sign, sizeof(sign), &sign_length);
	if (status != PSA_SUCCESS)
	{
		printk("Failed to sign message %x\n", status);
		return (-3);
	}
	status = psa_verify_message( key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), message, strlen((const char *)message), 
								(unsigned char *)sign, sign_length);
	if (status != PSA_SUCCESS)
	{
		printk("Failed to verify message %x\n", status);
		return (-4);
	}
	printk("Success to verify message\n");

	while(1);
	return  0;
}
