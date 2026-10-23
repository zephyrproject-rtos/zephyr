/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/tls_credentials_keygen.h>
#include <zephyr/psa/key_ids.h>

#include <psa/crypto.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_csr.h>

#include "common.h"

static const sec_tag_t test_tags[] = {TEST_SEC_TAG, TEST_SEC_TAG_2, TEST_SEC_TAG_LARGE};

void reset_state(void)
{
	zassert_equal(psa_crypto_init(), PSA_SUCCESS);

	for (size_t i = 0; i < ARRAY_SIZE(test_tags); i++) {
		(void)tls_credential_delete(test_tags[i], TLS_CREDENTIAL_PRIVATE_KEY_PSA);
		(void)tls_credential_delete(test_tags[i], TLS_CREDENTIAL_PRIVATE_KEY);
	}

	for (size_t i = 0; i < KEY_ID_COUNT; i++) {
		(void)psa_destroy_key(KEY_ID_MIN + i);
	}

	(void)psa_destroy_key(FOREIGN_KEY_ID);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_state();
}

static void foreign_key_create(psa_key_id_t id, size_t bits)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t out;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, bits);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_ANY_HASH));
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attr, id);
	zassert_equal(psa_generate_key(&attr, &out), PSA_SUCCESS, "PSA key generation failed");
}

static void key_register(sec_tag_t tag, psa_key_id_t id)
{
	zassert_ok(tls_credential_add(tag, TLS_CREDENTIAL_PRIVATE_KEY_PSA, &id, sizeof(id)));
}

static bool key_exists(psa_key_id_t id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	bool exists = psa_get_key_attributes(id, &attr) == PSA_SUCCESS;

	psa_reset_key_attributes(&attr);

	return exists;
}

psa_key_id_t key_id_get(sec_tag_t tag)
{
	psa_key_id_t id = PSA_KEY_ID_NULL;

	zassert_ok(tls_credential_keygen_key_id_get(tag, &id));

	return id;
}

/* Verify the signature of the CSR with the exported public key. */
static void csr_signature_check(sec_tag_t tag, const mbedtls_x509_csr *csr)
{
	uint8_t pub[TLS_CREDENTIAL_KEYGEN_PUBKEY_MAX_SIZE];
	uint8_t hash[PSA_HASH_MAX_SIZE];
	psa_algorithm_t alg;
	mbedtls_pk_context pk;
	size_t hash_len;
	size_t len;

	alg = csr->MBEDTLS_PRIVATE(sig_md) == MBEDTLS_MD_SHA384 ? PSA_ALG_SHA_384 : PSA_ALG_SHA_256;
	zassert_equal(psa_hash_compute(alg, csr->cri.p, csr->cri.len, hash, sizeof(hash),
				       &hash_len), PSA_SUCCESS);

	zassert_ok(tls_credential_keygen_pubkey_get(tag, pub, sizeof(pub), &len));
	mbedtls_pk_init(&pk);
	zassert_equal(mbedtls_pk_parse_public_key(&pk, pub, len), 0, "public key does not parse");
	zassert_equal(mbedtls_pk_verify(&pk, csr->MBEDTLS_PRIVATE(sig_md), hash, hash_len,
					csr->MBEDTLS_PRIVATE(sig).p, csr->MBEDTLS_PRIVATE(sig).len),
		      0, "CSR signature does not verify with the exported public key");
	mbedtls_pk_free(&pk);
}

static void csr_check(sec_tag_t tag, mbedtls_md_type_t md)
{
	uint8_t der[512];
	size_t len = 0;
	mbedtls_x509_csr csr;
	char subj[64];

	zassert_ok(tls_credential_keygen_csr(tag, "CN=test,O=Nordic", der, sizeof(der), &len));
	zassert_true(len > 0, "CSR length must be positive");
	zassert_equal(der[0], 0x30, "DER CSR must begin with a SEQUENCE tag");

	/* Parsing checks the structure, the subject and the signature made through PSA. */
	mbedtls_x509_csr_init(&csr);
	zassert_equal(mbedtls_x509_csr_parse_der(&csr, der, len), 0, "CSR does not parse");
	zassert_true(mbedtls_x509_dn_gets(subj, sizeof(subj), &csr.subject) > 0);
	zassert_not_null(strstr(subj, "CN=test"), "unexpected CSR subject: %s", subj);
	zassert_not_null(strstr(subj, "O=Nordic"), "unexpected CSR subject: %s", subj);
	zassert_equal(csr.MBEDTLS_PRIVATE(sig_md), md, "unexpected CSR signature hash");
	csr_signature_check(tag, &csr);
	mbedtls_x509_csr_free(&csr);
}

ZTEST_SUITE(tls_credentials_keygen_api, NULL, NULL, before, NULL, NULL);

ZTEST(tls_credentials_keygen_api, test_generate)
{
	psa_key_id_t id = PSA_KEY_ID_NULL;

	zassert_equal(tls_credential_keygen_key_id_get(TEST_SEC_TAG, &id), -ENOENT);
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	zassert_ok(tls_credential_keygen_key_id_get(TEST_SEC_TAG, &id));
	zassert_equal(tls_credential_keygen_generate(TEST_SEC_TAG, P256), -EEXIST,
		      "second generate must report -EEXIST");
}

ZTEST(tls_credentials_keygen_api, test_key_attributes)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t id;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	id = key_id_get(TEST_SEC_TAG);

	zassert_true(id >= KEY_ID_MIN && id < KEY_ID_MIN + KEY_ID_COUNT,
		     "key ID 0x%08x not in the reserved range", id);
	zassert_equal(psa_get_key_attributes(id, &attr), PSA_SUCCESS);
	zassert_equal(psa_get_key_type(&attr), PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	zassert_equal(psa_get_key_bits(&attr), 256);
	zassert_equal(psa_get_key_algorithm(&attr), PSA_ALG_ECDSA(PSA_ALG_ANY_HASH),
		      "TLS must be able to choose the hash");
	zassert_false(PSA_KEY_LIFETIME_IS_VOLATILE(psa_get_key_lifetime(&attr)),
		      "key must be persistent");
	zassert_false(psa_get_key_usage_flags(&attr) & PSA_KEY_USAGE_EXPORT,
		      "key must not be exportable");
	zassert_true(psa_get_key_usage_flags(&attr) & PSA_KEY_USAGE_SIGN_HASH);
	zassert_true(psa_get_key_usage_flags(&attr) & PSA_KEY_USAGE_VERIFY_MESSAGE,
		      "a JWT signer verifies its signature");
	zassert_false(psa_get_key_usage_flags(&attr) & PSA_KEY_USAGE_VERIFY_HASH,
		      "key must not have usages that it does not need");
	psa_reset_key_attributes(&attr);
}

ZTEST(tls_credentials_keygen_api, test_invalid_type)
{
	zassert_equal(tls_credential_keygen_generate(TEST_SEC_TAG,
						     (enum tls_credential_keygen_type)99),
		      -EINVAL);
}

ZTEST(tls_credentials_keygen_api, test_p384)
{
	uint8_t pub[TLS_CREDENTIAL_KEYGEN_PUBKEY_MAX_SIZE];
	size_t len = 0;

	if (!IS_ENABLED(CONFIG_TLS_CREDENTIALS_KEYGEN_SECP384R1)) {
		zassert_equal(tls_credential_keygen_generate(TEST_SEC_TAG, P384), -ENOTSUP);
		return;
	}

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P384));
	zassert_ok(tls_credential_keygen_pubkey_get(TEST_SEC_TAG, pub, sizeof(pub), &len));
	zassert_equal(len, TLS_CREDENTIAL_KEYGEN_PUBKEY_MAX_SIZE,
		      "P-384 SubjectPublicKeyInfo must be %d bytes",
		      TLS_CREDENTIAL_KEYGEN_PUBKEY_MAX_SIZE);
	csr_check(TEST_SEC_TAG, MBEDTLS_MD_SHA384);
}

ZTEST(tls_credentials_keygen_api, test_large_sec_tag)
{
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG_LARGE, P256));
	csr_check(TEST_SEC_TAG_LARGE, MBEDTLS_MD_SHA256);
}

ZTEST(tls_credentials_keygen_api, test_raw_private_key_exists)
{
	static const uint8_t dummy[] = {0xAA};

	zassert_ok(tls_credential_add(TEST_SEC_TAG, TLS_CREDENTIAL_PRIVATE_KEY, dummy,
				      sizeof(dummy)));
	zassert_equal(tls_credential_keygen_generate(TEST_SEC_TAG, P256), -EEXIST,
		      "a tag with a private key must be rejected");
	zassert_equal(tls_credential_keygen_delete(TEST_SEC_TAG), -ENOENT,
		      "a raw private key is not a generated key");
}

ZTEST(tls_credentials_keygen_api, test_delete)
{
	psa_key_id_t id;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	id = key_id_get(TEST_SEC_TAG);
	zassert_ok(tls_credential_keygen_delete(TEST_SEC_TAG));
	zassert_false(key_exists(id), "key must be destroyed");
	zassert_equal(tls_credential_keygen_key_id_get(TEST_SEC_TAG, &id), -ENOENT);

	zassert_equal(tls_credential_keygen_delete(TEST_SEC_TAG), -ENOENT,
		      "deleting a key that does not exist must be reported");
}

ZTEST(tls_credentials_keygen_api, test_pubkey_get)
{
	uint8_t pub[TLS_CREDENTIAL_KEYGEN_PUBKEY_MAX_SIZE];
	size_t len = 0;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));

	zassert_equal(tls_credential_keygen_pubkey_get(TEST_SEC_TAG, NULL, sizeof(pub), &len),
		      -EINVAL, "NULL output must be rejected");
	zassert_equal(tls_credential_keygen_pubkey_get(TEST_SEC_TAG, pub, 64, &len), -EFBIG,
		      "a small buffer must be reported");
	zassert_ok(tls_credential_keygen_pubkey_get(TEST_SEC_TAG, pub, sizeof(pub), &len));
	zassert_equal(len, 91, "P-256 SubjectPublicKeyInfo must be 91 bytes");
	zassert_equal(pub[0], 0x30, "SubjectPublicKeyInfo must begin with a SEQUENCE tag");
}

ZTEST(tls_credentials_keygen_api, test_csr_generate)
{
	uint8_t der[512];
	size_t len = 0;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));

	zassert_equal(tls_credential_keygen_csr(TEST_SEC_TAG, NULL, der, sizeof(der), &len),
		      -EINVAL, "NULL subject must be rejected");
	zassert_equal(tls_credential_keygen_csr(TEST_SEC_TAG, "not a subject", der, sizeof(der),
						&len),
		      -EINVAL, "invalid subject must be rejected");
	zassert_equal(tls_credential_keygen_csr(TEST_SEC_TAG_2, "CN=test", der, sizeof(der),
						&len),
		      -ENOENT, "missing key must be reported");

	csr_check(TEST_SEC_TAG, MBEDTLS_MD_SHA256);
}

ZTEST_SUITE(tls_credentials_keygen_range, NULL, NULL, before, NULL, NULL);

ZTEST(tls_credentials_keygen_range, test_foreign_key_rejected)
{
	uint8_t buf[512];
	size_t len;
	psa_key_id_t id;

	/* A registration that points to a key of another PSA user. */
	foreign_key_create(FOREIGN_KEY_ID, 256);
	key_register(TEST_SEC_TAG, FOREIGN_KEY_ID);

	zassert_equal(tls_credential_keygen_key_id_get(TEST_SEC_TAG, &id), -EPERM);
	zassert_equal(tls_credential_keygen_csr(TEST_SEC_TAG, "CN=test", buf, sizeof(buf), &len),
		      -EPERM, "CSR must not be signed with a foreign key");
	zassert_equal(tls_credential_keygen_pubkey_get(TEST_SEC_TAG, buf, sizeof(buf), &len),
		      -EPERM);
	zassert_equal(tls_credential_keygen_delete(TEST_SEC_TAG), -EPERM);
	zassert_true(key_exists(FOREIGN_KEY_ID), "foreign key must not be destroyed");
}

ZTEST(tls_credentials_keygen_range, test_two_sec_tags)
{
	psa_key_id_t id;
	psa_key_id_t id_2;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG_2, P256));

	id = key_id_get(TEST_SEC_TAG);
	id_2 = key_id_get(TEST_SEC_TAG_2);
	zassert_not_equal(id, id_2, "each tag must have its own key");

	/* Deleting one key keeps the other. */
	zassert_ok(tls_credential_keygen_delete(TEST_SEC_TAG));
	zassert_false(key_exists(id));
	zassert_equal(key_id_get(TEST_SEC_TAG_2), id_2);
}

ZTEST(tls_credentials_keygen_range, test_unregistered_key_reused)
{
	psa_key_id_t id;

	/* A key in the range that no tag refers to, for example after tls_credential_delete(). */
	foreign_key_create(KEY_ID_MIN, 256);

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	id = key_id_get(TEST_SEC_TAG);
	zassert_equal(id, KEY_ID_MIN, "the unregistered key ID must be reused");
}

ZTEST(tls_credentials_keygen_range, test_registered_key_kept)
{
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	zassert_equal(key_id_get(TEST_SEC_TAG), KEY_ID_MIN);

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG_2, P256));
	zassert_equal(key_id_get(TEST_SEC_TAG_2), KEY_ID_MIN + 1);
	zassert_equal(key_id_get(TEST_SEC_TAG), KEY_ID_MIN, "the first key must be kept");
}

ZTEST(tls_credentials_keygen_range, test_rollback_on_registration_failure)
{
	static const uint8_t dummy[] = {0xAA};
	const sec_tag_t fill_tags[] = {9001, 9002, 9003, 9004};

	BUILD_ASSERT(ARRAY_SIZE(fill_tags) >= CONFIG_TLS_MAX_CREDENTIALS_NUMBER,
		     "must be able to fill the whole TLS credentials store");

	/* Fill the TLS credentials store so the registration fails after the key is generated. */
	for (int i = 0; i < CONFIG_TLS_MAX_CREDENTIALS_NUMBER; i++) {
		zassert_ok(tls_credential_add(fill_tags[i], TLS_CREDENTIAL_CA_CERTIFICATE, dummy,
					      sizeof(dummy)));
	}

	zassert_not_equal(tls_credential_keygen_generate(TEST_SEC_TAG, P256), 0,
			  "generate must fail when the registration cannot complete");

	for (size_t i = 0; i < KEY_ID_COUNT; i++) {
		zassert_false(key_exists(KEY_ID_MIN + i),
			      "failed registration must roll back the generated key");
	}

	for (int i = 0; i < CONFIG_TLS_MAX_CREDENTIALS_NUMBER; i++) {
		(void)tls_credential_delete(fill_tags[i], TLS_CREDENTIAL_CA_CERTIFICATE);
	}
}

ZTEST(tls_credentials_keygen_api, test_csr_buffer_too_small)
{
	uint8_t der[64];
	size_t len;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	zassert_equal(tls_credential_keygen_csr(TEST_SEC_TAG, "CN=test", der, sizeof(der), &len),
		      -EFBIG, "a small buffer must be reported");
}

ZTEST(tls_credentials_keygen_api, test_csr_size_macro)
{
	/* Short attribute names with short values have the largest encoding for their length.
	 * Mbed TLS keeps only the last value of an attribute, so each name is used once.
	 */
	static const char subject[] = "C=a,ST=a,L=a,O=a,OU=a,CN=a,DC=a,SN=a,GN=a";
	static uint8_t der[TLS_CREDENTIAL_KEYGEN_CSR_SIZE(sizeof(subject) - 1)];
	enum tls_credential_keygen_type type =
		IS_ENABLED(CONFIG_TLS_CREDENTIALS_KEYGEN_SECP384R1) ? P384 : P256;
	size_t len;

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, type));
	zassert_ok(tls_credential_keygen_csr(TEST_SEC_TAG, subject, der, sizeof(der), &len),
		   "TLS_CREDENTIAL_KEYGEN_CSR_SIZE() is too small");
}

ZTEST(tls_credentials_keygen_range, test_range_full)
{
	if (KEY_ID_COUNT > 2) {
		ztest_test_skip();
	}

	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG, P256));
	zassert_ok(tls_credential_keygen_generate(TEST_SEC_TAG_2, P256));
	zassert_equal(tls_credential_keygen_generate(TEST_SEC_TAG_LARGE, P256), -ENOSPC,
		      "a full key store must be reported");
}
