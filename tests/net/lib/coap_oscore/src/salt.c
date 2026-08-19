/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/coap.h>

#include "coap_oscore_internal.h"

#define COAP_BUF_SIZE 128

static const uint8_t test_master_secret[] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
};
static const uint8_t test_client_id[] = {0x01};
static const uint8_t test_server_id[] = {0x02};

ZTEST(coap_oscore_salt, test_oscore_master_salt_zero_encrypt_decrypt)
{
	uint8_t plain[COAP_BUF_SIZE];
	uint8_t protected[COAP_BUF_SIZE];
	uint8_t decrypted[COAP_BUF_SIZE];
	uint8_t token[] = {0xaa, 0xbb};
	const uint8_t payload[] = "salt-ok";
	struct coap_packet req;
	struct coap_packet outer;
	struct coap_packet dec;
	struct coap_oscore_context *ctx_client;
	struct coap_oscore_context *ctx_server;
	struct coap_oscore_context *ctx_verified = NULL;
	struct coap_oscore_init_params params_client = {
		.master_secret = test_master_secret,
		.master_secret_len = sizeof(test_master_secret),
		.sender_id = test_client_id,
		.sender_id_len = sizeof(test_client_id),
		.recipient_id = test_server_id,
		.recipient_id_len = sizeof(test_server_id),
		.master_salt = NULL,
		.master_salt_len = 0,
		.aead_alg = COAP_OSCORE_AEAD_AES_CCM_16_64_128,
		.hkdf = COAP_OSCORE_HKDF_SHA_256,
		.fresh_master_secret_salt = true,
	};
	struct coap_oscore_init_params params_server = {
		.master_secret = test_master_secret,
		.master_secret_len = sizeof(test_master_secret),
		.sender_id = test_server_id,
		.sender_id_len = sizeof(test_server_id),
		.recipient_id = test_client_id,
		.recipient_id_len = sizeof(test_client_id),
		.master_salt = NULL,
		.master_salt_len = 0,
		.aead_alg = COAP_OSCORE_AEAD_AES_CCM_16_64_128,
		.hkdf = COAP_OSCORE_HKDF_SHA_256,
		.fresh_master_secret_salt = true,
	};
	uint32_t protected_len = sizeof(protected);
	uint32_t decrypted_len = sizeof(decrypted);
	const uint8_t *dec_payload;
	uint16_t dec_payload_len;
	uint8_t error_code = 0;
	int ret;
	bool needs_echo_challenge = false;

	ret = coap_oscore_context_add(&params_client, &ctx_client);
	zassert_ok(ret, "Client context add failed (%d)", ret);

	ret = coap_oscore_context_add(&params_server, &ctx_server);
	zassert_ok(ret, "Server context add failed (%d)", ret);

	ret = coap_packet_init(&req, plain, sizeof(plain), COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token, COAP_METHOD_POST, 0x1234);
	zassert_ok(ret, "CoAP request init failed (%d)", ret);

	ret = coap_packet_append_option(&req, COAP_OPTION_URI_PATH, (const uint8_t *)"salt", 4);
	zassert_ok(ret, "URI path append failed (%d)", ret);

	ret = coap_packet_append_payload_marker(&req);
	zassert_ok(ret, "Payload marker append failed (%d)", ret);

	ret = coap_packet_append_payload(&req, payload, sizeof(payload) - 1);
	zassert_ok(ret, "Payload append failed (%d)", ret);

	ret = coap_oscore_protect(req.data, req.offset, protected, &protected_len, ctx_client);
	zassert_ok(ret, "coap_oscore_protect failed (%d)", ret);

	ret = coap_packet_parse(&outer, protected, protected_len, NULL, 0);
	zassert_ok(ret, "Outer packet parse failed (%d)", ret);
	zassert_true(coap_oscore_msg_has_oscore(&outer), "Expected OSCORE option");

	ret = coap_oscore_verify(&outer, protected, protected_len, decrypted, &decrypted_len,
				 &ctx_verified, &error_code, &needs_echo_challenge);
	zassert_ok(ret, "coap_oscore_verify failed (%d, code=%u)", ret, error_code);
	zassert_true(ctx_verified == ctx_server, "Verify chose unexpected context");
	zassert_false(needs_echo_challenge, "Did not expect Echo challenge");

	ret = coap_oscore_context_dec_refcount(ctx_verified);
	zassert_ok(ret, "Context dec refcount failed (%d)", ret);

	ret = coap_packet_parse(&dec, decrypted, decrypted_len, NULL, 0);
	zassert_ok(ret, "Decrypted packet parse failed (%d)", ret);

	dec_payload = coap_packet_get_payload(&dec, &dec_payload_len);
	zassert_not_null(dec_payload, "Missing decrypted payload");
	zassert_equal(dec_payload_len, sizeof(payload) - 1, "Unexpected decrypted payload length");
	zassert_mem_equal(dec_payload, payload, dec_payload_len, "Decrypted payload mismatch");

	ret = coap_oscore_context_remove(ctx_server);
	zassert_ok(ret, "Server context remove failed (%d)", ret);

	ret = coap_oscore_context_remove(ctx_client);
	zassert_ok(ret, "Client context remove failed (%d)", ret);
}

ZTEST_SUITE(coap_oscore_salt, NULL, NULL, NULL, NULL, NULL);
