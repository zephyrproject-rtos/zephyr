/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/base64.h>

#include <psa/crypto.h>
#include <mbedtls/x509_csr.h>

#include "common.h"

/* Host tools capture the command output, so the markers below are a contract. */

static const struct shell *dummy;

static void *shell_setup(void)
{
	dummy = shell_backend_dummy_get_ptr();
	WAIT_FOR(shell_ready(dummy), 20000, k_msleep(1));
	zassert_true(shell_ready(dummy), "timed out waiting for dummy shell backend");
	return NULL;
}

static void shell_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_state();
	shell_backend_dummy_clear_output(dummy);
}

static const char *run(const char *cmd, int *ret)
{
	size_t size;
	int r;

	shell_backend_dummy_clear_output(dummy);
	r = shell_execute_cmd(dummy, cmd);
	if (ret != NULL) {
		*ret = r;
	}
	return shell_backend_dummy_get_output(dummy, &size);
}

/* Copy the token that follows @p prefix in @p out. Returns its length, or 0 if not found. */
static size_t token_after(const char *out, const char *prefix, char *tok, size_t tok_size)
{
	const char *p = strstr(out, prefix);
	size_t i = 0;

	if (p == NULL) {
		return 0;
	}
	p += strlen(prefix);

	while (p[i] != '\0' && p[i] != '\r' && p[i] != '\n' && p[i] != ' ' && i < tok_size - 1) {
		tok[i] = p[i];
		i++;
	}
	tok[i] = '\0';

	return i;
}

ZTEST_SUITE(tls_credentials_keygen_shell, NULL, shell_setup, shell_before, NULL, NULL);

ZTEST(tls_credentials_keygen_shell, test_keygen)
{
	psa_key_id_t id;
	int ret;
	const char *out = run("cred keygen 42", &ret);

	zassert_ok(ret, "keygen command returned %d", ret);
	zassert_not_null(strstr(out, "Generated device key in sec tag 42"),
			 "missing success line, got: %s", out);
	zassert_ok(tls_credential_keygen_key_id_get(TEST_SEC_TAG, &id),
		   "key not created by shell command");
}

ZTEST(tls_credentials_keygen_shell, test_keygen_type)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	int ret;

	(void)run("cred keygen 42 p384", &ret);
	if (!IS_ENABLED(CONFIG_TLS_CREDENTIALS_KEYGEN_SECP384R1)) {
		zassert_not_equal(ret, 0, "disabled key type must be rejected");
		return;
	}

	zassert_ok(ret, "keygen command returned %d", ret);
	zassert_equal(psa_get_key_attributes(key_id_get(TEST_SEC_TAG), &attr), PSA_SUCCESS);
	zassert_equal(psa_get_key_bits(&attr), 384);
	psa_reset_key_attributes(&attr);

	(void)run("cred keygen 43 p521", &ret);
	zassert_not_equal(ret, 0, "unknown key type must be rejected");
}

ZTEST(tls_credentials_keygen_shell, test_keygen_already_exists)
{
	int ret;
	const char *out;

	(void)run("cred keygen 42", NULL);
	out = run("cred keygen 42", &ret);

	zassert_not_equal(ret, 0, "second keygen should fail");
	zassert_not_null(strstr(out, "already exists"), "missing warning, got: %s", out);
	zassert_not_null(strstr(out, "keydel"), "should point user to 'keydel', got: %s", out);
}

ZTEST(tls_credentials_keygen_shell, test_csr)
{
	int ret;
	const char *out;
	char tok[1024];
	uint8_t der[512];
	size_t tok_len;
	size_t der_len = 0;
	mbedtls_x509_csr csr;

	(void)run("cred keygen 42", NULL);
	out = run("cred csr 42 CN=test,O=Nordic", &ret);

	zassert_ok(ret, "csr command returned %d", ret);
	zassert_not_null(strstr(out, "CSR generation complete"),
			 "missing completion marker, got: %s", out);

	/* A truncated line would decode to a DER that does not parse. */
	tok_len = token_after(out, "CSR: ", tok, sizeof(tok));
	zassert_true(tok_len > 0, "no CSR token in output: %s", out);
	zassert_ok(base64_decode(der, sizeof(der), &der_len, tok, tok_len),
		   "CSR Base64 does not decode");

	mbedtls_x509_csr_init(&csr);
	zassert_equal(mbedtls_x509_csr_parse_der(&csr, der, der_len), 0,
		      "decoded CSR does not parse");
	mbedtls_x509_csr_free(&csr);
}

ZTEST(tls_credentials_keygen_shell, test_csr_requires_subject)
{
	int ret;

	(void)run("cred keygen 42", NULL);
	(void)run("cred csr 42", &ret);

	zassert_not_equal(ret, 0, "csr without a subject should fail");
}

ZTEST(tls_credentials_keygen_shell, test_csr_invalid_subject)
{
	int ret;
	const char *out;

	(void)run("cred keygen 42", NULL);
	out = run("cred csr 42 notasubject", &ret);

	zassert_not_equal(ret, 0, "csr with an invalid subject should fail");
	zassert_not_null(strstr(out, "CSR generation failed"), "missing error, got: %s", out);
}

ZTEST(tls_credentials_keygen_shell, test_pubkey)
{
	int ret;
	const char *out;
	char tok[160];
	uint8_t dec[100];
	size_t tok_len;
	size_t dec_len = 0;

	(void)run("cred keygen 42", NULL);
	out = run("cred pubkey 42", &ret);

	zassert_ok(ret, "pubkey command returned %d", ret);
	zassert_not_null(strstr(out, "Public key export complete"),
			 "missing completion marker, got: %s", out);

	/* A 91-byte P-256 SubjectPublicKeyInfo is exactly 124 Base64 characters. */
	tok_len = token_after(out, "PUBKEY: ", tok, sizeof(tok));
	zassert_equal(tok_len, 124, "public key Base64 length %zu != 124", tok_len);
	zassert_ok(base64_decode(dec, sizeof(dec), &dec_len, tok, tok_len),
		   "public key Base64 does not decode");
	zassert_equal(dec_len, 91, "decoded public key is %zu bytes, expected 91", dec_len);
	zassert_equal(dec[0], 0x30, "decoded public key is not a SubjectPublicKeyInfo");
}

ZTEST(tls_credentials_keygen_shell, test_keydel)
{
	psa_key_id_t id;
	int ret;
	const char *out;

	(void)run("cred keygen 42", NULL);
	out = run("cred keydel 42", &ret);

	zassert_ok(ret, "keydel command returned %d", ret);
	zassert_not_null(strstr(out, "Deleted device key in sec tag 42"),
			 "missing delete confirmation, got: %s", out);
	zassert_equal(tls_credential_keygen_key_id_get(TEST_SEC_TAG, &id), -ENOENT,
		      "key still present after shell delete");
}

ZTEST(tls_credentials_keygen_shell, test_invalid_sec_tag)
{
	int ret;
	const char *out = run("cred keygen notanumber", &ret);

	zassert_not_equal(ret, 0, "invalid sec tag should fail");
	zassert_not_null(strstr(out, "not a valid sectag"), "missing error, got: %s", out);
}

ZTEST(tls_credentials_keygen_shell, test_keydel_without_key)
{
	int ret;
	const char *out = run("cred keydel 42", &ret);

	zassert_not_equal(ret, 0, "keydel without a key should fail");
	zassert_not_null(strstr(out, "Key deletion failed"), "missing error, got: %s", out);
}

ZTEST(tls_credentials_keygen_shell, test_csr_too_large)
{
	static char cmd[180];
	int ret;
	const char *out;
	size_t len;

	(void)run("cred keygen 42", NULL);

	/* The test sets a small CSR buffer, which this common name does not fit. */
	len = snprintk(cmd, sizeof(cmd), "cred csr 42 CN=");
	memset(&cmd[len], 'a', sizeof(cmd) - len - 1);
	out = run(cmd, &ret);

	zassert_not_equal(ret, 0, "a CSR that does not fit should fail");
	zassert_not_null(strstr(out, "CONFIG_TLS_CREDENTIALS_SHELL_KEYGEN_CSR_BUF_SIZE"),
			 "missing hint, got: %s", out);
}
