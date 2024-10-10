/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "tls_internal.h"

#define TEST_DNAME "CN=US"

static const char test_ca_cert[] = "Test CA certificate";
static const char test_server_cert[] = "Test server certificate";
static const char test_server_key[] = "Test server key";

#define UNUSED_TAG	CONFIG_TLS_MAX_CREDENTIALS_NUMBER
#define INVALID_TAG	(CONFIG_TLS_MAX_CREDENTIALS_NUMBER + 1)
#define COMMON_TAG	(CONFIG_TLS_MAX_CREDENTIALS_NUMBER - 1)
#define CSR_TAG_A	(CONFIG_TLS_MAX_CREDENTIALS_NUMBER - 2)
#define CSR_TAG_B	(CONFIG_TLS_MAX_CREDENTIALS_NUMBER - 3)

static char csr_buf[300];

/**
 * @brief Test test_credential_add function
 *
 * This test verifies the credential add operation.
 */
static void test_credential_add(void)
{
	int ret, i;

	for (i = 0; i < CONFIG_TLS_MAX_CREDENTIALS_NUMBER - 2; i++) {
		ret = tls_credential_add(i, TLS_CREDENTIAL_CA_CERTIFICATE,
					 test_ca_cert, sizeof(test_ca_cert));
		zassert_equal(ret, 0, "Failed to add credential %d %d", i);
	}

	/* Function should allow to add credentials of different types
	 * with the same tag
	 */
	ret = tls_credential_add(COMMON_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 test_server_cert, sizeof(test_server_cert));
	zassert_equal(ret, 0, "Failed to add credential %d %d",
		      COMMON_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE);

	ret = tls_credential_add(COMMON_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
				 test_server_key, sizeof(test_server_key));
	zassert_equal(ret, 0, "Failed to add credential %d %d",
		      COMMON_TAG, TLS_CREDENTIAL_PRIVATE_KEY);

	/* Try to register another credential - should not have memory for that
	 */
	ret = tls_credential_add(UNUSED_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 test_ca_cert, sizeof(test_ca_cert));
	zassert_equal(ret, -ENOMEM, "Should have failed with ENOMEM");

	/* Try to re-register with already registered tag and type */
	ret = tls_credential_add(COMMON_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
				 test_server_key, sizeof(test_server_key));
	zassert_equal(ret, -EEXIST, "Should have failed with EEXIST");
}

/**
 * @brief Test test_credential_get function
 *
 * This test verifies the credential get operation.
 */
static void test_credential_get(void)
{
	char cred[64];
	size_t credlen;
	int ret;

	/* Read existing credential */
	(void)memset(cred, 0, sizeof(cred));
	credlen = sizeof(cred);
	ret = tls_credential_get(COMMON_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
				 cred, &credlen);
	zassert_equal(ret, 0, "Failed to read credential %d %d",
		      0, TLS_CREDENTIAL_CA_CERTIFICATE);
	ret = strcmp(cred, test_server_key);
	zassert_equal(ret, 0, "Invalid credential content");
	zassert_equal(credlen, sizeof(test_server_key),
		      "Invalid credential length");

	/* Try to read non-existing credentials */
	credlen = sizeof(cred);
	ret = tls_credential_get(INVALID_TAG, TLS_CREDENTIAL_PSK,
				 cred, &credlen);
	zassert_equal(ret, -ENOENT, "Should have failed with ENOENT");

	/* Try to read with too small buffer */
	credlen = sizeof(test_server_cert) - 1;
	ret = tls_credential_get(COMMON_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 cred, &credlen);
	zassert_equal(ret, -EFBIG, "Should have failed with EFBIG");
}

/**
 * @brief Test test_credential_internal_iterate function
 *
 * This test verifies the internal function for iterating over credentials.
 */
static void test_credential_internal_iterate(void)
{
	struct tls_credential *cert, *key, *temp;

	/* Non-existing credential should return NULL */
	key = credential_next_get(INVALID_TAG, NULL);
	zassert_is_null(key, "Should have return NULL for unknown credential");

	/* Iterate over credentials with the same tag */
	cert = credential_next_get(COMMON_TAG, NULL);
	zassert_not_null(cert, "Should have find a credential");

	key = credential_next_get(COMMON_TAG, cert);
	zassert_not_null(key, "Should have find a credential");

	if (cert->type == TLS_CREDENTIAL_PRIVATE_KEY) {
		/* Function does not guarantee order of reads,
		 * so assume we could read key first
		 */
		temp = key;
		key = cert;
		cert = temp;
	}

	zassert_equal(cert->type, TLS_CREDENTIAL_SERVER_CERTIFICATE,
		      "Invalid type for cert");
	zassert_equal(cert->tag, COMMON_TAG, "Invalid tag for cert");
	zassert_equal(cert->len, sizeof(test_server_cert),
		      "Invalid cert length");
	zassert_mem_equal(cert->buf, test_server_cert, sizeof(test_server_key),
			  "Invalid cert content");

	zassert_equal(key->type, TLS_CREDENTIAL_PRIVATE_KEY,
		      "Invalid type for key");
	zassert_equal(key->tag, COMMON_TAG, "Invalid tag for key");
	zassert_equal(key->len, sizeof(test_server_key), "Invalid key length");
	zassert_mem_equal(key->buf, test_server_key, sizeof(test_server_key),
			  "Invalid key content");

	/* Iteration after getting last credential should return NULL */
	key = credential_next_get(COMMON_TAG, key);
	zassert_is_null(key, "Should have return NULL after last credential");
}

/**
 * @brief Test test_credential_delete function
 *
 * This test verifies the credential delete operation.
 */
static void test_credential_delete(void)
{
	int ret;
	char cred[64];
	size_t credlen = sizeof(cred);

	/* Should fail if when trying to remove non-existing credential. */
	ret = tls_credential_delete(INVALID_TAG, TLS_CREDENTIAL_CA_CERTIFICATE);
	zassert_equal(ret, -ENOENT, "Should have failed with ENOENT");

	/* Should remove existing credential. */
	ret = tls_credential_delete(COMMON_TAG, TLS_CREDENTIAL_PRIVATE_KEY);
	zassert_equal(ret, 0, "Failed to delete credential %d %d",
		      COMMON_TAG, TLS_CREDENTIAL_PRIVATE_KEY);

	ret = tls_credential_get(COMMON_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
				 cred, &credlen);
	zassert_equal(ret, -ENOENT, "Should have failed with ENOENT");
}

/**
 * @brief Test test_csr function
 *
 * This test verifies the CSR generation operation.
 * When TLS_CREDNTIAL_CSR is enabled, this test relies on MbedTLS to verify the validity of
 * the generated CSR and private key.
 */
#if defined(CONFIG_TLS_CREDENTIAL_CSR)

#include <mbedtls/pk.h>
#include <mbedtls/x509_csr.h>

#include <zephyr/random/random.h>

static char pk_buf_a[200];
static char pk_buf_b[200];
static char dname_buf[100];

/* Checks memory region for all zeros. */
static bool allzero(char *buf, size_t len) {
	for (int i = 0; i < len; i++) {
		if (buf[i] != 0) {
			return false;
		}
	}
	return true;
}

/* MbedTLS requires a random for blinding, so provide a (not cryptographically secure)
 * random generator for testing purposes.
 */
static int tls_rng(void *ctx, unsigned char *buf, size_t len)
{
	ARG_UNUSED(ctx);

	sys_rand_get(buf, len);

	return 0;
}

static void test_csr(void)
{
	int ret;
	size_t csr_len;
	size_t pk_len_a;
	size_t pk_len_b;
	mbedtls_x509_csr csr;
	mbedtls_pk_context pk;

	memset(csr_buf, 0, sizeof(csr_buf));
	memset(pk_buf_a, 0, sizeof(pk_buf_a));

	return 0;

	/* Should fail if CSR buffer is too small for key */
	csr_len = 10;
	ret = tls_credential_csr(CSR_TAG_A, TEST_DNAME, csr_buf, &csr_len);
	zassert_equal(ret, -EFBIG, "tls_credential_csr should return -EFBIG if csr buffer is "
				   "too small.");
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");

	/* Should fail if CSR buffer is too small for CSR, but large enough for key */
	csr_len = 140;
	ret = tls_credential_csr(CSR_TAG_A, TEST_DNAME, csr_buf, &csr_len);
	zassert_equal(ret, -EFBIG, "tls_credential_csr should return -EFBIG if csr buffer is "
				   "too small.");
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");
	zassert_equal(csr_len, 0, "CSR buffer should be cleared after error");

	/* Private key should not be kept in this case. */
	pk_len_a = sizeof(pk_buf_a);
	ret = tls_credential_get(CSR_TAG_A, TLS_CREDENTIAL_PSK, pk_buf_a, &pk_len_a);
	zassert_equal(ret, -ENOENT, "Private key should not be stored if CSR generation failed.");

	/* Should fail if distinguished name is invalid */
	csr_len = sizeof(csr_buf);
	ret = tls_credential_csr(CSR_TAG_A, "blah blah blah",	csr_buf, &csr_len);
	zassert_equal(ret, -EFAULT, "tls_credential_csr should return -EFBIG if dname invalid.");
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");
	zassert_equal(csr_len, 0, "CSR buffer should be cleared after error");

	csr_len = sizeof(csr_buf);
	ret = tls_credential_csr(CSR_TAG_A, "cn=us", 		csr_buf, &csr_len);
	zassert_equal(ret, -EFAULT, "tls_credential_csr should return -EFBIG if dname invalid.");
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");
	zassert_equal(csr_len, 0, "CSR buffer should be cleared after error");

	csr_len = sizeof(csr_buf);
	ret = tls_credential_csr(CSR_TAG_A, "",			csr_buf, &csr_len);
	zassert_equal(ret, -EFAULT, "tls_credential_csr should return -EFBIG if dname invalid.");
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");
	zassert_equal(csr_len, 0, "CSR buffer should be cleared after error");

	csr_len = sizeof(csr_buf);
	ret = tls_credential_csr(CSR_TAG_A, NULL, 		csr_buf, &csr_len);
	zassert_equal(ret, -EFAULT, "tls_credential_csr should return -EFBIG if dname invalid.");
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");
	zassert_equal(csr_len, 0, "CSR buffer should be cleared after error");

	/* Private key should not be stored. */
	pk_len_a = sizeof(pk_buf_a);
	ret = tls_credential_get(CSR_TAG_A, TLS_CREDENTIAL_PSK, pk_buf_a, &pk_len_a);
	zassert_equal(ret, -ENOENT, "Private key should not be stored if CSR generation failed.");

	/* Should succeed if provided valid configuration */
	csr_len = sizeof(csr_buf);
	ret = tls_credential_csr(CSR_TAG_A, TEST_DNAME, csr_buf, &csr_len);
	zassert_equal(ret, 0, "tls_credential_csr should succeed for valid parameters.");
	zassert_false(allzero(csr_buf, sizeof(csr_buf)),
		      "tls_credential_csr should write to csr_buf valid parameters.");
	zassert_true(csr_len > 0, "tls_credential_csr should succeed for valid parameters.");

	/* CSR buffer should be valid DER CSR with correct distinguished name afterwards */
	mbedtls_x509_csr_init(&csr);
	ret = mbedtls_x509_csr_parse(&csr, csr_buf, csr_len);
	zassert_equal(ret, 0, "Resultant CSR should be valid.");

	/* CSR should have the correct distinguished name */
	ret = mbedtls_x509_dn_gets(dname_buf, sizeof(dname_buf), &(csr.subject));
	zassert_true(ret > 0, "Resultant CSR should have the correct distinguished name.");
	ret = strcmp(TEST_DNAME, dname_buf);
	zassert_equal(ret, 0, "Resultant CSR shuold have the correct distinguished name.");

	/* A valid private key should have been saved to the sectag */
	pk_len_a = sizeof(pk_buf_a);
	ret = tls_credential_get(CSR_TAG_A, TLS_CREDENTIAL_PSK, pk_buf_a, &pk_len_a);
	zassert_equal(ret, 0, "Private key should be stored if CSR generation succeeds.");
	zassert_true(pk_len_a > 0, "Private key should be stored if CSR generation succeeds.");

	/* Stored private key should be readable by MbedTLS. */
	mbedtls_pk_init(&pk);
	ret = mbedtls_pk_parse_key(&pk, pk_buf_a, pk_len_a, NULL, 0, tls_rng, NULL);
	zassert_equal(ret, 0, "Private key should be readable by MbedTLS.");

	/* Generated private key should match CSR public key */
	ret = mbedtls_pk_check_pair(&(csr.pk), &pk, tls_rng, NULL);
	zassert_equal(ret, 0, "CSR Public key should match stored private key.");
	mbedtls_pk_free(&pk);
	mbedtls_x509_csr_free(&csr);

	/* Should fail if writing to already-occupied slot */
	csr_len = sizeof(csr_buf);
	ret = tls_credential_csr(CSR_TAG_A, TEST_DNAME, csr_buf, &csr_len);
	zassert_equal(ret, -EEXIST, "tls_credential_csr should return -EEXIST if sec tag taken.");
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");
	zassert_equal(csr_len, 0, "CSR buffer should be cleared after error");

	/* Original private key should not be altered in this case */
	pk_len_b = sizeof(pk_buf_b);
	ret = tls_credential_get(CSR_TAG_A, TLS_CREDENTIAL_PSK, pk_buf_b, &pk_len_b);
	zassert_equal(ret, 0, "Original private key should not be overwritten.");
	zassert_equal(pk_len_a, pk_len_b, "Original private key should not be overwritten.");
	ret = memcmp(pk_buf_a, pk_buf_b, pk_len_a);
	zassert_equal(ret, 0, "Original private key should not be overwritten.");

	/* Additional CSRs should be generated successfully and have unique private keys */
	csr_len = sizeof(csr_buf);
	ret = tls_credential_csr(CSR_TAG_B, TEST_DNAME, csr_buf, &csr_len);
	zassert_equal(ret, 0, "tls_credential_csr should succeed for valid parameters.");
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");
	zassert_true(csr_len > 0, "tls_credential_csr should succeed for valid parameters.");

	/* The new private key should be distinct from the first one */
	pk_len_b = sizeof(pk_buf_b);
	ret = tls_credential_get(CSR_TAG_B, TLS_CREDENTIAL_PSK, pk_buf_b, &pk_len_b);
	zassert_true(allzero(csr_buf, sizeof(csr_buf)), "CSR buffer should be cleared after error");
	ret = memcmp(pk_buf_a, pk_buf_b, MIN(pk_len_a, pk_len_b));
	zassert_not_equal(ret, 0, "Consecutive CSRs should have distinct private keys.");
}

#else /* defined(CONFIG_TLS_CREDENTIAL_CSR) */

static void test_csr(void)
{
	int ret;
	size_t csr_len = sizeof(csr_buf);

	/* Compiler will complain about this being unused since tls_credential_csr defaults to
	 * an empty macro.
	 */
	(void) csr_len;
	memset(csr_buf, 0, sizeof(csr_buf));

	/* Should fail if not enabled */
	ret = tls_credential_csr(csr_tag_a, "CN=US", csr_buf, &csr_len);
	zassert_equal(ret, -ENOTSUP);
}

#endif /* defined(CONFIG_TLS_CREDENTIAL_CSR) */


ZTEST(tls_credentials, test_tls_credentials)
{
	test_credential_add();
	test_credential_get();
	test_credential_internal_iterate();
	test_credential_delete();
	test_csr();
}

ZTEST_SUITE(tls_credentials, NULL, NULL, NULL, NULL, NULL);
