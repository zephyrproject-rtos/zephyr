/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_ccm_vector_1(void);
extern void test_ccm_vector_2(void);
extern void test_ccm_vector_3(void);
extern void test_ccm_vector_4(void);
extern void test_ccm_vector_5(void);
extern void test_ccm_vector_6(void);
extern void test_ccm_vector_7(void);
extern void test_ccm_vector_8(void);
extern void test_ctr_sp_800_38a_encrypt_decrypt(void);
extern void test_cbc_sp_800_38a_encrypt_decrypt(void);
extern void test_cmac_mode(void);
extern void test_ctr_prng_vector(void);
extern void test_ctr_prng_reseed(void);
extern void test_ctr_prng_uninstantiate(void);
extern void test_ctr_prng_robustness(void);
extern void test_ecc_dh(void);
extern void test_ecc_dsa(void);
extern void test_hmac_1(void);
extern void test_hmac_2(void);
extern void test_hmac_3(void);
extern void test_hmac_4(void);
extern void test_hmac_5(void);
extern void test_hmac_6(void);
extern void test_hmac_7(void);
extern void test_sha256_1(void);
extern void test_sha256_2(void);
extern void test_sha256_3(void);
extern void test_sha256_4(void);
extern void test_sha256_5(void);
extern void test_sha256_6(void);
extern void test_sha256_7(void);
extern void test_sha256_8(void);
extern void test_sha256_9(void);
extern void test_sha256_10(void);
extern void test_sha256_11(void);
extern void test_sha256_12(void);
extern void test_sha256_13_and_14(void);
extern void test_aes_key_chain(void);
extern void test_aes_vectors(void);
extern void test_aes_fixed_key_variable_text(void);
extern void test_aes_variable_key_fixed_text(void);


/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_tinycrypt,
			ztest_unit_test(test_aes_key_chain),
			ztest_unit_test(test_aes_vectors),
			ztest_unit_test(test_aes_fixed_key_variable_text),
			ztest_unit_test(test_aes_variable_key_fixed_text),
			ztest_unit_test(test_sha256_1),
			ztest_unit_test(test_sha256_2),
			ztest_unit_test(test_sha256_3),
			ztest_unit_test(test_sha256_4),
			ztest_unit_test(test_sha256_5),
			ztest_unit_test(test_sha256_6),
			ztest_unit_test(test_sha256_7),
			ztest_unit_test(test_sha256_8),
			ztest_unit_test(test_sha256_9),
			ztest_unit_test(test_sha256_10),
			ztest_unit_test(test_sha256_11),
			ztest_unit_test(test_sha256_12),
			ztest_unit_test(test_sha256_13_and_14),
			ztest_unit_test(test_hmac_1),
			ztest_unit_test(test_hmac_2),
			ztest_unit_test(test_hmac_3),
			ztest_unit_test(test_hmac_4),
			ztest_unit_test(test_hmac_5),
			ztest_unit_test(test_hmac_6),
			ztest_unit_test(test_hmac_7),
			ztest_unit_test(test_ccm_vector_1),
			ztest_unit_test(test_ccm_vector_2),
			ztest_unit_test(test_ccm_vector_3),
			ztest_unit_test(test_ccm_vector_4),
			ztest_unit_test(test_ccm_vector_5),
			ztest_unit_test(test_ccm_vector_6),
			ztest_unit_test(test_ccm_vector_7),
			ztest_unit_test(test_ccm_vector_8),
			ztest_unit_test(test_ecc_dh),
			ztest_unit_test(test_ecc_dsa),
			ztest_unit_test(test_cmac_mode),
			ztest_unit_test(test_ctr_prng_vector),
			ztest_unit_test(test_ctr_prng_reseed),
			ztest_unit_test(test_ctr_prng_uninstantiate),
			ztest_unit_test(test_ctr_prng_robustness),
			ztest_unit_test(test_ctr_sp_800_38a_encrypt_decrypt),
			ztest_unit_test(test_cbc_sp_800_38a_encrypt_decrypt)
				);
	ztest_run_test_suite(test_tinycrypt);
}
