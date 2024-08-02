/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "tls_internal.h"

static const char test_ca_cert[] = "Test CA certificate";
static const char test_server_cert[] = "Test server certificate";
static const char test_server_key[] = "Test server key";

static const int invalid_tag = CONFIG_TLS_MAX_CREDENTIALS_NUMBER + 1;
static const int unused_tag = CONFIG_TLS_MAX_CREDENTIALS_NUMBER;
static const int common_tag = CONFIG_TLS_MAX_CREDENTIALS_NUMBER - 1;

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
	ret = tls_credential_add(common_tag, TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 test_server_cert, sizeof(test_server_cert));
	zassert_equal(ret, 0, "Failed to add credential %d %d",
		      common_tag, TLS_CREDENTIAL_SERVER_CERTIFICATE);

	ret = tls_credential_add(common_tag, TLS_CREDENTIAL_PRIVATE_KEY,
				 test_server_key, sizeof(test_server_key));
	zassert_equal(ret, 0, "Failed to add credential %d %d",
		      common_tag, TLS_CREDENTIAL_PRIVATE_KEY);

	/* Try to register another credential - should not have memory for that
	 */
	ret = tls_credential_add(unused_tag, TLS_CREDENTIAL_CA_CERTIFICATE,
				 test_ca_cert, sizeof(test_ca_cert));
	zassert_equal(ret, -ENOMEM, "Should have failed with ENOMEM");

	/* Try to re-register with already registered tag and type */
	ret = tls_credential_add(common_tag, TLS_CREDENTIAL_PRIVATE_KEY,
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
	ret = tls_credential_get(common_tag, TLS_CREDENTIAL_PRIVATE_KEY,
				 cred, &credlen);
	zassert_equal(ret, 0, "Failed to read credential %d %d",
		      0, TLS_CREDENTIAL_CA_CERTIFICATE);
	ret = strcmp(cred, test_server_key);
	zassert_equal(ret, 0, "Invalid credential content");
	zassert_equal(credlen, sizeof(test_server_key),
		      "Invalid credential length");

	/* Try to read non-existing credentials */
	credlen = sizeof(cred);
	ret = tls_credential_get(invalid_tag, TLS_CREDENTIAL_PSK,
				 cred, &credlen);
	zassert_equal(ret, -ENOENT, "Should have failed with ENOENT");

	/* Try to read with too small buffer */
	credlen = sizeof(test_server_cert) - 1;
	ret = tls_credential_get(common_tag, TLS_CREDENTIAL_SERVER_CERTIFICATE,
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
	key = credential_next_get(invalid_tag, NULL);
	zassert_is_null(key, "Should have return NULL for unknown credential");

	/* Iterate over credentials with the same tag */
	cert = credential_next_get(common_tag, NULL);
	zassert_not_null(cert, "Should have find a credential");

	key = credential_next_get(common_tag, cert);
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
	zassert_equal(cert->tag, common_tag, "Invalid tag for cert");
	zassert_equal(cert->len, sizeof(test_server_cert),
		      "Invalid cert length");
	zassert_mem_equal(cert->buf, test_server_cert, sizeof(test_server_key),
			  "Invalid cert content");

	zassert_equal(key->type, TLS_CREDENTIAL_PRIVATE_KEY,
		      "Invalid type for key");
	zassert_equal(key->tag, common_tag, "Invalid tag for key");
	zassert_equal(key->len, sizeof(test_server_key), "Invalid key length");
	zassert_mem_equal(key->buf, test_server_key, sizeof(test_server_key),
			  "Invalid key content");

	/* Iteration after getting last credential should return NULL */
	key = credential_next_get(common_tag, key);
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
	ret = tls_credential_delete(invalid_tag, TLS_CREDENTIAL_CA_CERTIFICATE);
	zassert_equal(ret, -ENOENT, "Should have failed with ENOENT");

	/* Should remove existing credential. */
	ret = tls_credential_delete(common_tag, TLS_CREDENTIAL_PRIVATE_KEY);
	zassert_equal(ret, 0, "Failed to delete credential %d %d",
		      common_tag, TLS_CREDENTIAL_PRIVATE_KEY);

	ret = tls_credential_get(common_tag, TLS_CREDENTIAL_PRIVATE_KEY,
				 cred, &credlen);
	zassert_equal(ret, -ENOENT, "Should have failed with ENOENT");
}

ZTEST(tls_crecentials, test_tls_crecentials)
{
	test_credential_add();
	test_credential_get();
	test_credential_internal_iterate();
	test_credential_delete();
}

ZTEST_SUITE(tls_crecentials, NULL, NULL, NULL, NULL, NULL);
