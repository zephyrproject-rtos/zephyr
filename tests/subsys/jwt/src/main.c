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

psa_status_t key_import_helper(const psa_key_id_t key_id, const uint8_t *key_der,
			       const size_t key_der_len, const psa_algorithm_t alg,
			       const psa_key_type_t key_type, psa_key_id_t *key_out_id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_id(&attr, key_id);
	psa_set_key_type(&attr, key_type);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	return psa_import_key(&attr, key_der, key_der_len, key_out_id);
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

void verify_jwt(const psa_key_id_t signature_key_id, const char *alg_str, const psa_algorithm_t alg,
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

	/* Verify the signature over the base64 encoded header and payload with the delimiter */
	const size_t message_input_length =
		strlen((const char *)header_buf_b64) + 1 + strlen((const char *)payload_buf_b64);
	res = psa_verify_message(signature_key_id, alg, (const uint8_t *)null_terminated_jwt,
				 message_input_length, signature_buf, signature_buf_decoded_len);
	zassert_equal(res, 0, "Verifying payload");

	/* Parse and verify the header */
	struct jwt_header header = {};

	int64_t parse_res = json_obj_parse((char *)header_buf, header_buf_decoded_len,
					   jwt_header_descr, ARRAY_SIZE(jwt_header_descr), &header);
	zassert_true(parse_res > 0, "JSON object parsing");
	zassert_str_equal(header.alg, alg_str);
	zassert_str_equal(header.typ, "JWT");

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

void create_and_validate_jwt(const psa_key_id_t signature_key_id, const char *alg_str,
			     const psa_algorithm_t alg)
{
	char buf[1024] = {0};
	struct jwt_builder build;

	int res = jwt_init_builder(&build, buf, sizeof(buf), alg_str, "JWT");
	zassert_equal(res, 0, "Setting up jwt");

	const struct jwt_claims claims = DEFAULT_CLAIMS;

	res = jwt_add_payload(&build, &claims, jwt_claims_desc, ARRAY_SIZE(jwt_claims_desc));
	zassert_equal(res, 0, "Adding payload");

	res = jwt_sign(&build, signature_key_id, alg);
	zassert_equal(res, 0, "Signing payload");
	zassert_equal(build.overflowed, false, "Not overflow");

	verify_jwt(signature_key_id, alg_str, alg, buf);
}

ZTEST(jwt_tests, test_jwt_rsa)
{
	psa_key_id_t key_id;
	const psa_algorithm_t alg = PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256);
	const psa_status_t res =
		key_import_helper(1, jwt_test_rsa_private_der, jwt_test_rsa_private_der_len, alg,
				  PSA_KEY_TYPE_RSA_KEY_PAIR, &key_id);
	zassert_equal(res, 0, "Key import");

	create_and_validate_jwt(key_id, "RS256", alg);
}

ZTEST(jwt_tests, test_ecdsa_rsa)
{
	psa_key_id_t key_id;
	const psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
	const psa_status_t res =
		key_import_helper(1, jwt_test_ecdsa_private_der, jwt_test_ecdsa_private_der_len,
				  alg, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1), &key_id);
	zassert_equal(res, 0, "Key import");

	create_and_validate_jwt(key_id, "ES512", alg);
}

ZTEST_SUITE(jwt_tests, NULL, NULL, NULL, NULL, NULL);
