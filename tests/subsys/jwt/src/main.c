/*
 * RFC 7519 Json Web Tokens
 *
 * Copyright (C) 2018, Linaro, Ltd
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr/ztest.h>
#include <zephyr/data/json.h>
#include <zephyr/data/jwt.h>

#include <stdbool.h>

#define DEFAULT_SUB          "iot-work-199419"
#define DEFAULT_EXP          1530312026
#define DEFAULT_IAT          1530308426
#define DEFAULT_CUSTOM_CLAIM "custom_value"
#define DEFAULT_CLAIMS                                                                             \
	{                                                                                          \
		.sub = DEFAULT_SUB, .exp = DEFAULT_EXP, .iat = DEFAULT_IAT, .custom_claims_obj = { \
			.custom_claim = DEFAULT_CUSTOM_CLAIM                                       \
		}                                                                                  \
	}

extern unsigned char jwt_test_rsa_private_der[];
extern unsigned int jwt_test_rsa_private_der_len;
extern unsigned char jwt_test_ecdsa_private_der[];
extern unsigned int jwt_test_ecdsa_private_der_len;

struct custom_claims {
	const char *custom_claim;
};

struct jwt_claims {
	const char *sub;
	const int64_t exp;
	const int64_t iat;
	struct custom_claims custom_claims_obj;
};

struct json_obj_descr custom_claims_desc[] = {
	JSON_OBJ_DESCR_PRIM(struct custom_claims, custom_claim, JSON_TOK_STRING)};
struct json_obj_descr jwt_claims_desc[] = {
	JSON_OBJ_DESCR_PRIM(struct jwt_claims, sub, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct jwt_claims, exp, JSON_TOK_INT64),
	JSON_OBJ_DESCR_PRIM(struct jwt_claims, iat, JSON_TOK_INT64),
	JSON_OBJ_DESCR_OBJECT(struct jwt_claims, custom_claims_obj, custom_claims_desc),
};

struct jwt_header {
	const char *alg;
	const char *typ;
};

static struct json_obj_descr jwt_header_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct jwt_header, alg, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct jwt_header, typ, JSON_TOK_STRING)};

psa_status_t key_import_helper(const uint8_t *key_der, const size_t key_der_len,
			       const psa_algorithm_t alg, const psa_key_type_t key_type,
			       psa_key_id_t *key_out_id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, key_type);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	return psa_import_key(&attr, key_der, key_der_len, key_out_id);
}

static psa_key_id_t import_rsa_key(void)
{
	psa_key_id_t key_id;
	const psa_status_t res = key_import_helper(
		jwt_test_rsa_private_der, jwt_test_rsa_private_der_len,
		PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256), PSA_KEY_TYPE_RSA_KEY_PAIR, &key_id);

	zassert_equal(res, PSA_SUCCESS, "Key import");
	return key_id;
}

static psa_key_id_t import_ecdsa_key(void)
{
	psa_key_id_t key_id;
	const psa_status_t res =
		key_import_helper(jwt_test_ecdsa_private_der, jwt_test_ecdsa_private_der_len,
				  PSA_ALG_ECDSA(PSA_ALG_SHA_256),
				  PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1), &key_id);

	zassert_equal(res, PSA_SUCCESS, "Key import");
	return key_id;
}

/*
 * Map the JWS "alg" name of RFC 7518 back to a PSA algorithm, so that the
 * token is verified with what its own header claims and not with what the
 * test asked for.  Only the names the tests can produce are listed.
 */
static psa_algorithm_t alg_from_name(const char *alg_name)
{
	if (strcmp(alg_name, "RS256") == 0) {
		return PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256);
	}
	if (strcmp(alg_name, "ES256") == 0) {
		return PSA_ALG_ECDSA(PSA_ALG_SHA_256);
	}

	return PSA_ALG_NONE;
}

/* Courtesy of the Google AI beast */
int jwt_base64_decode(const uint8_t *in, const size_t in_len, uint8_t *out, const size_t out_max,
		      size_t *out_len)
{
	if (!in || !out || !out_len) {
		return -EINVAL;
	}

	const char *b64url = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	unsigned int buf = 0;
	int bits = 0;
	size_t decoded_bytes = 0;

	*out_len = 0; /* Reset out parameter early */

	for (size_t i = 0; i < in_len; i++) {
		/* Early exit if we hit an unexpected null terminator before in_len */
		if (in[i] == '\0') {
			break;
		}

		/* Standard JWTs shouldn't have padding, but skip it if present */
		if (in[i] == '=') {
			continue;
		}

		char *p = strchr(b64url, in[i]);

		if (!p) {
			return -EBADMSG; /* Corrupted data / invalid Base64URL character */
		}

		buf = (buf << 6) | (p - b64url);
		bits += 6;

		if (bits >= 8) {
			bits -= 8;

			if (decoded_bytes >= out_max) {
				return -ENOBUFS; /* No buffer space available */
			}

			out[decoded_bytes++] = (buf >> bits) & 0xFF;
		}
	}

	*out_len = decoded_bytes;
	return 0;
}

void verify_jwt(const psa_key_id_t signature_key_id, const char *expected_alg_name,
		const char *null_terminated_jwt)
{
	/* Extract the three pieces of the JWT (header, payload, signature) */
	uint8_t header_buf_b64[512] = {0};
	uint8_t payload_buf_b64[512] = {0};
	uint8_t signature_buf_b64[512] = {0};

	int res = sscanf(null_terminated_jwt, "%[^.].%[^.].%s", header_buf_b64, payload_buf_b64,
			 signature_buf_b64);
	zassert_equal(res, 3, "Extracting JWT components");

	size_t header_buf_decoded_len = 0;
	uint8_t header_buf[512] = {0};
	size_t payload_buf_decoded_len = 0;
	uint8_t payload_buf[512] = {0};
	size_t signature_buf_decoded_len = 0;
	uint8_t signature_buf[512] = {0};

	/* Decode the header */
	res = jwt_base64_decode(header_buf_b64, strlen((const char *)header_buf_b64), header_buf,
				sizeof(header_buf), &header_buf_decoded_len);
	zassert_equal(res, 0, "Decoding header");

	/* Decode the payload */
	res = jwt_base64_decode(payload_buf_b64, strlen((const char *)payload_buf_b64), payload_buf,
				sizeof(payload_buf), &payload_buf_decoded_len);
	zassert_equal(res, 0, "Decoding payload");

	/* Decode the signature */
	res = jwt_base64_decode(signature_buf_b64, strlen((const char *)signature_buf_b64),
				signature_buf, sizeof(signature_buf), &signature_buf_decoded_len);
	zassert_equal(res, 0, "Decoding signature");

	/* Parse and verify the header */
	struct jwt_header header = {};

	int64_t parse_res = json_obj_parse((char *)header_buf, header_buf_decoded_len,
					   jwt_header_descr, ARRAY_SIZE(jwt_header_descr), &header);
	zassert_true(parse_res > 0, "JSON object parsing");
	zassert_str_equal(header.alg, expected_alg_name);
	zassert_str_equal(header.typ, "JWT");

	/*
	 * Verify the signature over the base64 encoded header and payload with
	 * the delimiter, using the algorithm the header announces.
	 */
	const psa_algorithm_t alg = alg_from_name(header.alg);

	zassert_not_equal(alg, PSA_ALG_NONE, "Unknown header algorithm");

	const size_t message_input_length =
		strlen((const char *)header_buf_b64) + 1 + strlen((const char *)payload_buf_b64);
	res = psa_verify_message(signature_key_id, alg, (const uint8_t *)null_terminated_jwt,
				 message_input_length, signature_buf, signature_buf_decoded_len);
	zassert_equal(res, 0, "Verifying payload");

	/* Parse and verify the payload (claims). */
	struct jwt_claims payload = {};

	parse_res = json_obj_parse((char *)payload_buf, payload_buf_decoded_len, jwt_claims_desc,
				   ARRAY_SIZE(jwt_claims_desc), &payload);
	zassert_true(parse_res > 0, "JSON object parsing");
	zassert_str_equal(payload.sub, DEFAULT_SUB);
	zassert_equal(payload.exp, DEFAULT_EXP);
	zassert_equal(payload.iat, DEFAULT_IAT);
	zassert_str_equal(payload.custom_claims_obj.custom_claim, DEFAULT_CUSTOM_CLAIM);
}

void create_and_validate_jwt(const psa_key_id_t signature_key_id, const psa_algorithm_t alg,
			     const char *expected_alg_name)
{
	char buf[1024] = {0};
	struct jwt_builder build;

	int res = jwt_init_builder(&build, buf, sizeof(buf), signature_key_id, alg);

	zassert_equal(res, 0, "Setting up jwt");

	const struct jwt_claims claims = DEFAULT_CLAIMS;

	res = jwt_add_payload(&build, &claims, jwt_claims_desc, ARRAY_SIZE(jwt_claims_desc));
	zassert_equal(res, 0, "Adding payload");

	res = jwt_sign(&build);
	zassert_equal(res, 0, "Signing payload");
	zassert_equal(build.overflowed, false, "Not overflow");

	verify_jwt(signature_key_id, expected_alg_name, buf);
}

ZTEST(jwt_tests, test_jwt_rsa)
{
	const psa_key_id_t key_id = import_rsa_key();

	create_and_validate_jwt(key_id, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256), "RS256");

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS, "Key destroy");
}

ZTEST(jwt_tests, test_jwt_ecdsa)
{
	const psa_key_id_t key_id = import_ecdsa_key();

	create_and_validate_jwt(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), "ES256");

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS, "Key destroy");
}

/*
 * The header has to describe the signature, so a combination that has no
 * JWS name must be rejected instead of being given an arbitrary one.
 */
ZTEST(jwt_tests, test_jwt_unsupported_algorithm)
{
	char buf[1024] = {0};
	struct jwt_builder build;
	const psa_key_id_t key_id = import_ecdsa_key();

	/* RFC 7518 only pairs SHA-384 with P-384, and this key is P-256. */
	int res =
		jwt_init_builder(&build, buf, sizeof(buf), key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_384));

	zassert_equal(res, -ENOTSUP, "Curve and hash mismatch");

	/* An RSA algorithm with an EC key has no JWS name either. */
	res = jwt_init_builder(&build, buf, sizeof(buf), key_id,
			       PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));
	zassert_equal(res, -ENOTSUP, "Key type mismatch");

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS, "Key destroy");
}

ZTEST(jwt_tests, test_jwt_unknown_key)
{
	char buf[1024] = {0};
	struct jwt_builder build;
	const int res = jwt_init_builder(&build, buf, sizeof(buf), PSA_KEY_ID_NULL,
					 PSA_ALG_ECDSA(PSA_ALG_SHA_256));

	zassert_equal(res, -EINVAL, "Unknown key");
}

/*
 * The signature is sized from the key, so a buffer that cannot hold it is
 * reported rather than silently truncated.
 */
ZTEST(jwt_tests, test_jwt_buffer_too_small)
{
	char buf[256] = {0};
	struct jwt_builder build;
	const psa_key_id_t key_id = import_rsa_key();

	int res = jwt_init_builder(&build, buf, sizeof(buf), key_id,
				   PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));

	zassert_equal(res, 0, "Setting up jwt");

	const struct jwt_claims claims = DEFAULT_CLAIMS;

	res = jwt_add_payload(&build, &claims, jwt_claims_desc, ARRAY_SIZE(jwt_claims_desc));
	zassert_equal(res, 0, "Adding payload");

	res = jwt_sign(&build);
	zassert_equal(res, -ENOMEM, "Signing into a short buffer");
	zassert_true(build.overflowed, "Overflow reported");

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS, "Key destroy");
}

/*
 * The signature is encoded in place at the end of the free space, so the
 * smallest buffer that fits the token has to work and one byte less must
 * be refused.
 */
ZTEST(jwt_tests, test_jwt_minimum_buffer)
{
	char buf[1024] = {0};
	struct jwt_builder build;
	const psa_key_id_t key_id = import_ecdsa_key();
	const psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
	const struct jwt_claims claims = DEFAULT_CLAIMS;

	int res = jwt_init_builder(&build, buf, sizeof(buf), key_id, alg);

	zassert_equal(res, 0, "Setting up jwt");
	res = jwt_add_payload(&build, &claims, jwt_claims_desc, ARRAY_SIZE(jwt_claims_desc));
	zassert_equal(res, 0, "Adding payload");
	res = jwt_sign(&build);
	zassert_equal(res, 0, "Signing payload");

	/* The token plus its terminator is all the buffer that is needed. */
	const size_t token_size = strlen(buf) + 1;

	memset(buf, 0, sizeof(buf));
	res = jwt_init_builder(&build, buf, token_size, key_id, alg);
	zassert_equal(res, 0, "Setting up jwt");
	res = jwt_add_payload(&build, &claims, jwt_claims_desc, ARRAY_SIZE(jwt_claims_desc));
	zassert_equal(res, 0, "Adding payload");
	res = jwt_sign(&build);
	zassert_equal(res, 0, "Signing payload");
	zassert_equal(strlen(buf), token_size - 1, "Token length");
	verify_jwt(key_id, "ES256", buf);

	memset(buf, 0, sizeof(buf));
	res = jwt_init_builder(&build, buf, token_size - 1, key_id, alg);
	zassert_equal(res, 0, "Setting up jwt");
	res = jwt_add_payload(&build, &claims, jwt_claims_desc, ARRAY_SIZE(jwt_claims_desc));
	zassert_equal(res, 0, "Adding payload");
	res = jwt_sign(&build);
	zassert_equal(res, -ENOMEM, "Signing into a short buffer");

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS, "Key destroy");
}

ZTEST_SUITE(jwt_tests, NULL, NULL, NULL, NULL, NULL);
