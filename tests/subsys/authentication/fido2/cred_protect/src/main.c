/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * credProtect enforcement. The test plays the platform: it talks CTAP2
 * to the authenticator through a loopback transport, sets a PIN and
 * uses pinUvAuthToken (PIN protocol one) for user verification.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/authentication/fido2/fido2.h>
#include <zephyr/authentication/fido2/fido2_transport.h>
#include <zephyr/authentication/fido2/fido2_types.h>

#include <psa/crypto.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>

#define CTAP_MAKE_CREDENTIAL    0x01U
#define CTAP_GET_ASSERTION      0x02U
#define CTAP_CLIENT_PIN         0x06U
#define CTAP_GET_NEXT_ASSERTION 0x08U

#define CP_GET_KEY_AGREEMENT 0x02U
#define CP_SET_PIN           0x03U
#define CP_GET_PIN_TOKEN     0x05U

#define PIN_PROTOCOL_ONE 1U
#define PIN_AUTH_LEN     16U
#define TOKEN_LEN        32U
#define AES_BLOCK        16U
#define PADDED_PIN_LEN   64U

/* Offset of the credential ID length in attested credential data */
#define AUTH_DATA_CRED_ID_LEN_OFF (FIDO2_AUTH_DATA_HEADER_SIZE + FIDO2_AAGUID_SIZE)
#define AUTH_DATA_FLAGS_OFF       FIDO2_SHA256_SIZE
#define AUTH_DATA_FLAG_UV_BIT     BIT(2)

#define CBOR_BUF_SIZE (CONFIG_FIDO2_CBOR_MAX_SIZE + 1)

static const char test_pin[] = "1234";

struct cred_ref {
	uint8_t id[FIDO2_CREDENTIAL_ID_MAX_SIZE];
	size_t len;
};

struct assertion {
	struct cred_ref cred;
	uint8_t flags;
	uint32_t num_credentials;
};

/* Loopback transport */

static fido2_transport_recv_cb_t recv_cb;
static uint8_t resp_frame[CBOR_BUF_SIZE];
static size_t resp_frame_len;
K_SEM_DEFINE(resp_sem, 0, 1);

static int loop_init(fido2_transport_recv_cb_t cb, fido2_transport_cancel_cb_t cancel_cb)
{
	ARG_UNUSED(cancel_cb);
	recv_cb = cb;

	return 0;
}

static int loop_send(uint32_t cid, const uint8_t *data, size_t len)
{
	ARG_UNUSED(cid);

	if (len > sizeof(resp_frame)) {
		return -EMSGSIZE;
	}

	memcpy(resp_frame, data, len);
	resp_frame_len = len;
	k_sem_give(&resp_sem);

	return 0;
}

static void loop_notify(uint32_t cid, enum fido2_wire_status status)
{
	ARG_UNUSED(cid);
	ARG_UNUSED(status);
}

static void loop_shutdown(void)
{
}

static const struct fido2_transport_api loop_api = {
	.init = loop_init,
	.send = loop_send,
	.notify = loop_notify,
	.shutdown = loop_shutdown,
};

FIDO2_TRANSPORT_DEFINE(loop_transport, "LOOP", &loop_api);

/* Returns the CTAP status byte; the response CBOR is left in resp_frame + 1. */
static uint8_t ctap(uint8_t cmd, const uint8_t *cbor, size_t cbor_len)
{
	static uint8_t req[CBOR_BUF_SIZE];

	zassert_true(cbor_len < sizeof(req));
	req[0] = cmd;
	if (cbor_len > 0) {
		memcpy(req + 1, cbor, cbor_len);
	}

	resp_frame_len = 0;
	zassert_equal(recv_cb(&loop_transport, 1, req, cbor_len + 1), 0);
	zassert_equal(k_sem_take(&resp_sem, K_SECONDS(10)), 0, "no CTAP response");
	zassert_true(resp_frame_len >= 1);

	return resp_frame[0];
}

/* Platform-side crypto */

static void sha256(const uint8_t *data, size_t len, uint8_t out[FIDO2_SHA256_SIZE])
{
	size_t olen;

	zassert_equal(psa_hash_compute(PSA_ALG_SHA_256, data, len, out, FIDO2_SHA256_SIZE, &olen),
		      PSA_SUCCESS);
}

static void hmac_left16(const uint8_t key[32], const uint8_t *msg, size_t len,
			uint8_t out[PIN_AUTH_LEN])
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t mac[FIDO2_SHA256_SIZE];
	psa_key_id_t id;
	size_t mac_len;

	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	zassert_equal(psa_import_key(&attr, key, 32, &id), PSA_SUCCESS);
	zassert_equal(psa_mac_compute(id, PSA_ALG_HMAC(PSA_ALG_SHA_256), msg, len, mac, sizeof(mac),
				      &mac_len),
		      PSA_SUCCESS);
	psa_destroy_key(id);
	memcpy(out, mac, PIN_AUTH_LEN);
}

static void aes_cbc(bool encrypt, const uint8_t key[32], const uint8_t *in, size_t len,
		    uint8_t *out)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_cipher_operation_t op = PSA_CIPHER_OPERATION_INIT;
	static const uint8_t iv[AES_BLOCK];
	psa_key_id_t id;
	size_t olen = 0;
	size_t flen = 0;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, encrypt ? PSA_KEY_USAGE_ENCRYPT : PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_CBC_NO_PADDING);
	zassert_equal(psa_import_key(&attr, key, 32, &id), PSA_SUCCESS);

	if (encrypt) {
		zassert_equal(psa_cipher_encrypt_setup(&op, id, PSA_ALG_CBC_NO_PADDING),
			      PSA_SUCCESS);
	} else {
		zassert_equal(psa_cipher_decrypt_setup(&op, id, PSA_ALG_CBC_NO_PADDING),
			      PSA_SUCCESS);
	}
	zassert_equal(psa_cipher_set_iv(&op, iv, sizeof(iv)), PSA_SUCCESS);
	zassert_equal(psa_cipher_update(&op, in, len, out, len, &olen), PSA_SUCCESS);
	zassert_equal(psa_cipher_finish(&op, out + olen, len - olen, &flen), PSA_SUCCESS);
	zassert_equal(olen + flen, len);
	psa_destroy_key(id);
}

static bool cose_key_put(zcbor_state_t *zs, const uint8_t pub[FIDO2_P256_UNCOMPRESSED_KEY_SIZE])
{
	return zcbor_map_start_encode(zs, 5) && zcbor_int32_put(zs, 1) && zcbor_int32_put(zs, 2) &&
	       zcbor_int32_put(zs, 3) && zcbor_int32_put(zs, -25) && zcbor_int32_put(zs, -1) &&
	       zcbor_int32_put(zs, 1) && zcbor_int32_put(zs, -2) &&
	       zcbor_bstr_encode_ptr(zs, (const char *)pub + 1, 32) && zcbor_int32_put(zs, -3) &&
	       zcbor_bstr_encode_ptr(zs, (const char *)pub + 33, 32) && zcbor_map_end_encode(zs, 5);
}

/*
 * ClientPIN key agreement: returns the platform public key and the shared
 * secret SHA-256(ECDH(platform, authenticator).x).
 */
static void key_agreement(uint8_t platform_pub[FIDO2_P256_UNCOMPRESSED_KEY_SIZE],
			  uint8_t shared[FIDO2_SHA256_SIZE])
{
	uint8_t cbor[32];
	uint8_t auth_pub[FIDO2_P256_UNCOMPRESSED_KEY_SIZE];
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t z[32];
	size_t len;
	psa_key_id_t id;
	uint32_t key;

	ZCBOR_STATE_E(es, 2, cbor, sizeof(cbor), 1);

	zassert_true(zcbor_map_start_encode(es, 2) && zcbor_uint32_put(es, 1) &&
		     zcbor_uint32_put(es, PIN_PROTOCOL_ONE) && zcbor_uint32_put(es, 2) &&
		     zcbor_uint32_put(es, CP_GET_KEY_AGREEMENT) && zcbor_map_end_encode(es, 2));
	zassert_equal(ctap(CTAP_CLIENT_PIN, cbor, es->payload - cbor), FIDO2_OK);

	ZCBOR_STATE_D(ds, 3, resp_frame + 1, resp_frame_len - 1, 1, 0);

	auth_pub[0] = FIDO2_EC_POINT_UNCOMPRESSED;
	zassert_true(zcbor_map_start_decode(ds) && zcbor_uint32_decode(ds, &key) && key == 1U &&
		     zcbor_map_start_decode(ds));
	while (!zcbor_array_at_end(ds)) {
		struct zcbor_string s;
		int32_t k;

		zassert_true(zcbor_int32_decode(ds, &k));
		if (k == -2 || k == -3) {
			zassert_true(zcbor_bstr_decode(ds, &s) && s.len == 32U);
			memcpy(auth_pub + (k == -2 ? 1 : 33), s.value, 32);
		} else {
			zassert_true(zcbor_any_skip(ds, NULL));
		}
	}
	zassert_true(zcbor_map_end_decode(ds));

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	zassert_equal(psa_generate_key(&attr, &id), PSA_SUCCESS);
	zassert_equal(
		psa_export_public_key(id, platform_pub, FIDO2_P256_UNCOMPRESSED_KEY_SIZE, &len),
		PSA_SUCCESS);
	zassert_equal(psa_raw_key_agreement(PSA_ALG_ECDH, id, auth_pub, sizeof(auth_pub), z,
					    sizeof(z), &len),
		      PSA_SUCCESS);
	psa_destroy_key(id);
	sha256(z, len, shared);
}

static void set_pin(void)
{
	uint8_t platform_pub[FIDO2_P256_UNCOMPRESSED_KEY_SIZE];
	uint8_t shared[FIDO2_SHA256_SIZE];
	uint8_t padded[PADDED_PIN_LEN] = {0};
	uint8_t new_pin_enc[PADDED_PIN_LEN];
	uint8_t auth[PIN_AUTH_LEN];
	uint8_t cbor[256];

	ZCBOR_STATE_E(es, 3, cbor, sizeof(cbor), 1);

	key_agreement(platform_pub, shared);
	memcpy(padded, test_pin, strlen(test_pin));
	aes_cbc(true, shared, padded, sizeof(padded), new_pin_enc);
	hmac_left16(shared, new_pin_enc, sizeof(new_pin_enc), auth);

	zassert_true(zcbor_map_start_encode(es, 5) && zcbor_uint32_put(es, 1) &&
		     zcbor_uint32_put(es, PIN_PROTOCOL_ONE) && zcbor_uint32_put(es, 2) &&
		     zcbor_uint32_put(es, CP_SET_PIN) && zcbor_uint32_put(es, 3) &&
		     cose_key_put(es, platform_pub) && zcbor_uint32_put(es, 4) &&
		     zcbor_bstr_encode_ptr(es, (const char *)auth, sizeof(auth)) &&
		     zcbor_uint32_put(es, 5) &&
		     zcbor_bstr_encode_ptr(es, (const char *)new_pin_enc, sizeof(new_pin_enc)) &&
		     zcbor_map_end_encode(es, 5));
	zassert_equal(ctap(CTAP_CLIENT_PIN, cbor, es->payload - cbor), FIDO2_OK);
}

static void get_pin_token(uint8_t token[TOKEN_LEN])
{
	uint8_t platform_pub[FIDO2_P256_UNCOMPRESSED_KEY_SIZE];
	uint8_t shared[FIDO2_SHA256_SIZE];
	uint8_t pin_hash[FIDO2_SHA256_SIZE];
	uint8_t pin_hash_enc[AES_BLOCK];
	uint8_t cbor[256];
	struct zcbor_string token_enc = {0};
	uint32_t key;

	ZCBOR_STATE_E(es, 3, cbor, sizeof(cbor), 1);

	key_agreement(platform_pub, shared);
	sha256((const uint8_t *)test_pin, strlen(test_pin), pin_hash);
	aes_cbc(true, shared, pin_hash, sizeof(pin_hash_enc), pin_hash_enc);

	zassert_true(zcbor_map_start_encode(es, 4) && zcbor_uint32_put(es, 1) &&
		     zcbor_uint32_put(es, PIN_PROTOCOL_ONE) && zcbor_uint32_put(es, 2) &&
		     zcbor_uint32_put(es, CP_GET_PIN_TOKEN) && zcbor_uint32_put(es, 3) &&
		     cose_key_put(es, platform_pub) && zcbor_uint32_put(es, 6) &&
		     zcbor_bstr_encode_ptr(es, (const char *)pin_hash_enc, sizeof(pin_hash_enc)) &&
		     zcbor_map_end_encode(es, 4));
	zassert_equal(ctap(CTAP_CLIENT_PIN, cbor, es->payload - cbor), FIDO2_OK);

	ZCBOR_STATE_D(ds, 2, resp_frame + 1, resp_frame_len - 1, 1, 0);

	zassert_true(zcbor_map_start_decode(ds));
	while (!zcbor_array_at_end(ds)) {
		zassert_true(zcbor_uint32_decode(ds, &key));
		if (key == 2U) {
			zassert_true(zcbor_bstr_decode(ds, &token_enc));
		} else {
			zassert_true(zcbor_any_skip(ds, NULL));
		}
	}
	zassert_true(zcbor_map_end_decode(ds));
	zassert_equal(token_enc.len, TOKEN_LEN);

	aes_cbc(false, shared, token_enc.value, TOKEN_LEN, token);
}

/* pinUvAuthParam over clientDataHash, from a fresh pinUvAuthToken */
static void pin_uv_auth_param(const uint8_t cdh[FIDO2_SHA256_SIZE], uint8_t param[PIN_AUTH_LEN])
{
	uint8_t token[TOKEN_LEN];

	get_pin_token(token);
	hmac_left16(token, cdh, FIDO2_SHA256_SIZE, param);
}

static bool cred_list_put(zcbor_state_t *zs, const struct cred_ref *cred)
{
	return zcbor_list_start_encode(zs, 1) && zcbor_map_start_encode(zs, 2) &&
	       zcbor_tstr_put_lit(zs, "id") &&
	       zcbor_bstr_encode_ptr(zs, (const char *)cred->id, cred->len) &&
	       zcbor_tstr_put_lit(zs, "type") && zcbor_tstr_put_lit(zs, "public-key") &&
	       zcbor_map_end_encode(zs, 2) && zcbor_list_end_encode(zs, 1);
}

/* CTAP commands */

/*
 * MakeCredential for (rp_id, user). cred_protect 0 omits the extension.
 * On success the new credential ID is returned in out, if not NULL.
 */
static uint8_t make_credential(const char *rp_id, const char *user, uint8_t cred_protect, bool rk,
			       bool uv, const struct cred_ref *exclude, struct cred_ref *out)
{
	static uint8_t cbor[CONFIG_FIDO2_CBOR_MAX_SIZE];
	uint8_t cdh[FIDO2_SHA256_SIZE];
	uint8_t param[PIN_AUTH_LEN];
	struct zcbor_string auth_data = {0};
	uint32_t key;
	uint8_t status;
	bool ok;

	ZCBOR_STATE_E(es, 4, cbor, sizeof(cbor), 1);

	memset(cdh, 0x11, sizeof(cdh));
	if (uv) {
		pin_uv_auth_param(cdh, param);
	}

	ok = zcbor_map_start_encode(es, 9) && zcbor_uint32_put(es, 1) &&
	     zcbor_bstr_encode_ptr(es, (const char *)cdh, sizeof(cdh)) && zcbor_uint32_put(es, 2) &&
	     zcbor_map_start_encode(es, 1) && zcbor_tstr_put_lit(es, "id") &&
	     zcbor_tstr_encode_ptr(es, rp_id, strlen(rp_id)) && zcbor_map_end_encode(es, 1) &&
	     zcbor_uint32_put(es, 3) && zcbor_map_start_encode(es, 2) &&
	     zcbor_tstr_put_lit(es, "id") && zcbor_bstr_encode_ptr(es, user, strlen(user)) &&
	     zcbor_tstr_put_lit(es, "name") && zcbor_tstr_encode_ptr(es, user, strlen(user)) &&
	     zcbor_map_end_encode(es, 2) && zcbor_uint32_put(es, 4) &&
	     zcbor_list_start_encode(es, 1) && zcbor_map_start_encode(es, 2) &&
	     zcbor_tstr_put_lit(es, "alg") && zcbor_int32_put(es, FIDO2_COSE_ES256) &&
	     zcbor_tstr_put_lit(es, "type") && zcbor_tstr_put_lit(es, "public-key") &&
	     zcbor_map_end_encode(es, 2) && zcbor_list_end_encode(es, 1);
	if (ok && exclude != NULL) {
		ok = zcbor_uint32_put(es, 5) && cred_list_put(es, exclude);
	}
	if (ok && cred_protect != 0U) {
		ok = zcbor_uint32_put(es, 6) && zcbor_map_start_encode(es, 1) &&
		     zcbor_tstr_put_lit(es, "credProtect") && zcbor_uint32_put(es, cred_protect) &&
		     zcbor_map_end_encode(es, 1);
	}
	if (ok) {
		ok = zcbor_uint32_put(es, 7) && zcbor_map_start_encode(es, 1) &&
		     zcbor_tstr_put_lit(es, "rk") && zcbor_bool_put(es, rk) &&
		     zcbor_map_end_encode(es, 1);
	}
	if (ok && uv) {
		ok = zcbor_uint32_put(es, 8) &&
		     zcbor_bstr_encode_ptr(es, (const char *)param, sizeof(param)) &&
		     zcbor_uint32_put(es, 9) && zcbor_uint32_put(es, PIN_PROTOCOL_ONE);
	}
	zassert_true(ok && zcbor_map_end_encode(es, 9), "MakeCredential encode failed");

	status = ctap(CTAP_MAKE_CREDENTIAL, cbor, es->payload - cbor);
	if (status != FIDO2_OK || out == NULL) {
		return status;
	}

	ZCBOR_STATE_D(ds, 3, resp_frame + 1, resp_frame_len - 1, 1, 0);

	zassert_true(zcbor_map_start_decode(ds));
	while (!zcbor_array_at_end(ds)) {
		zassert_true(zcbor_uint32_decode(ds, &key));
		if (key == 2U) {
			zassert_true(zcbor_bstr_decode(ds, &auth_data));
		} else {
			zassert_true(zcbor_any_skip(ds, NULL));
		}
	}
	zassert_true(zcbor_map_end_decode(ds));
	zassert_true(auth_data.len > AUTH_DATA_CRED_ID_LEN_OFF + 2U);

	out->len = sys_get_be16(auth_data.value + AUTH_DATA_CRED_ID_LEN_OFF);
	zassert_true(out->len <= sizeof(out->id));
	memcpy(out->id, auth_data.value + AUTH_DATA_CRED_ID_LEN_OFF + 2U, out->len);

	return status;
}

static void decode_assertion(struct assertion *out)
{
	struct zcbor_string auth_data = {0};
	struct zcbor_string str;
	uint32_t key;

	ZCBOR_STATE_D(ds, 3, resp_frame + 1, resp_frame_len - 1, 1, 0);

	memset(out, 0, sizeof(*out));
	zassert_true(zcbor_map_start_decode(ds));
	while (!zcbor_array_at_end(ds)) {
		zassert_true(zcbor_uint32_decode(ds, &key));
		if (key == 1U) {
			zassert_true(zcbor_map_start_decode(ds));
			while (!zcbor_array_at_end(ds)) {
				zassert_true(zcbor_tstr_decode(ds, &str));
				if (str.len == 2U && memcmp(str.value, "id", 2) == 0) {
					zassert_true(zcbor_bstr_decode(ds, &str));
					zassert_true(str.len <= sizeof(out->cred.id));
					memcpy(out->cred.id, str.value, str.len);
					out->cred.len = str.len;
				} else {
					zassert_true(zcbor_any_skip(ds, NULL));
				}
			}
			zassert_true(zcbor_map_end_decode(ds));
		} else if (key == 2U) {
			zassert_true(zcbor_bstr_decode(ds, &auth_data));
		} else if (key == 5U) {
			zassert_true(zcbor_uint32_decode(ds, &out->num_credentials));
		} else {
			zassert_true(zcbor_any_skip(ds, NULL));
		}
	}
	zassert_true(zcbor_map_end_decode(ds));
	zassert_true(auth_data.len > AUTH_DATA_FLAGS_OFF);
	out->flags = auth_data.value[AUTH_DATA_FLAGS_OFF];
}

/* GetAssertion with an optional single-entry allowList, UP always requested. */
static uint8_t get_assertion(const char *rp_id, const struct cred_ref *allow, bool uv,
			     struct assertion *out)
{
	static uint8_t cbor[CONFIG_FIDO2_CBOR_MAX_SIZE];
	uint8_t cdh[FIDO2_SHA256_SIZE];
	uint8_t param[PIN_AUTH_LEN];
	uint8_t status;
	bool ok;

	ZCBOR_STATE_E(es, 4, cbor, sizeof(cbor), 1);

	memset(cdh, 0x22, sizeof(cdh));
	if (uv) {
		pin_uv_auth_param(cdh, param);
	}

	ok = zcbor_map_start_encode(es, 6) && zcbor_uint32_put(es, 1) &&
	     zcbor_tstr_encode_ptr(es, rp_id, strlen(rp_id)) && zcbor_uint32_put(es, 2) &&
	     zcbor_bstr_encode_ptr(es, (const char *)cdh, sizeof(cdh));
	if (ok && allow != NULL) {
		ok = zcbor_uint32_put(es, 3) && cred_list_put(es, allow);
	}
	if (ok && uv) {
		ok = zcbor_uint32_put(es, 6) &&
		     zcbor_bstr_encode_ptr(es, (const char *)param, sizeof(param)) &&
		     zcbor_uint32_put(es, 7) && zcbor_uint32_put(es, PIN_PROTOCOL_ONE);
	}
	zassert_true(ok && zcbor_map_end_encode(es, 6), "GetAssertion encode failed");

	status = ctap(CTAP_GET_ASSERTION, cbor, es->payload - cbor);
	if (status == FIDO2_OK && out != NULL) {
		decode_assertion(out);
	}

	return status;
}

static void assert_same_cred(const struct cred_ref *a, const struct cred_ref *b)
{
	zassert_equal(a->len, b->len);
	zassert_mem_equal(a->id, b->id, a->len);
}

/* Tests */

ZTEST(fido2_cred_protect, test_uv_required_discoverable)
{
	struct cred_ref cred;
	struct assertion as;

	zassert_equal(make_credential("l3rk.example", "u1", FIDO2_CRED_PROTECT_UV_REQUIRED, true,
				      true, NULL, &cred),
		      FIDO2_OK);

	zassert_equal(get_assertion("l3rk.example", NULL, false, NULL), FIDO2_ERR_NO_CREDENTIALS,
		      "level 3 credential discovered without UV");
	zassert_equal(get_assertion("l3rk.example", &cred, false, NULL), FIDO2_ERR_NO_CREDENTIALS,
		      "level 3 credential used from allowList without UV");

	zassert_equal(get_assertion("l3rk.example", NULL, true, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &cred);
	zassert_true((as.flags & AUTH_DATA_FLAG_UV_BIT) != 0U);

	zassert_equal(get_assertion("l3rk.example", &cred, true, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &cred);
}

ZTEST(fido2_cred_protect, test_uv_required_non_discoverable)
{
	struct cred_ref cred;
	struct assertion as;

	zassert_equal(make_credential("l3nd.example", "u1", FIDO2_CRED_PROTECT_UV_REQUIRED, false,
				      true, NULL, &cred),
		      FIDO2_OK);
	zassert_equal(cred.len, FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE);

	zassert_equal(get_assertion("l3nd.example", &cred, false, NULL), FIDO2_ERR_NO_CREDENTIALS,
		      "level 3 credential used without UV");

	zassert_equal(get_assertion("l3nd.example", &cred, true, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &cred);
}

ZTEST(fido2_cred_protect, test_uv_optional_with_list)
{
	struct cred_ref rk_cred;
	struct cred_ref nd_cred;
	struct assertion as;

	zassert_equal(make_credential("l2.example", "u1", FIDO2_CRED_PROTECT_UV_OPTIONAL_WITH_LIST,
				      true, true, NULL, &rk_cred),
		      FIDO2_OK);
	zassert_equal(make_credential("l2nd.example", "u1",
				      FIDO2_CRED_PROTECT_UV_OPTIONAL_WITH_LIST, false, true, NULL,
				      &nd_cred),
		      FIDO2_OK);

	zassert_equal(get_assertion("l2.example", NULL, false, NULL), FIDO2_ERR_NO_CREDENTIALS,
		      "level 2 credential discovered without UV");

	zassert_equal(get_assertion("l2.example", &rk_cred, false, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &rk_cred);
	zassert_equal(get_assertion("l2nd.example", &nd_cred, false, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &nd_cred);

	zassert_equal(get_assertion("l2.example", NULL, true, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &rk_cred);
}

ZTEST(fido2_cred_protect, test_uv_optional)
{
	struct cred_ref l1_cred;
	struct cred_ref none_cred;
	struct assertion as;

	zassert_equal(make_credential("l1.example", "u1", FIDO2_CRED_PROTECT_UV_OPTIONAL, true,
				      true, NULL, &l1_cred),
		      FIDO2_OK);
	zassert_equal(make_credential("l0nd.example", "u1", 0, false, false, NULL, &none_cred),
		      FIDO2_OK);

	zassert_equal(get_assertion("l1.example", NULL, false, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &l1_cred);
	zassert_true((as.flags & AUTH_DATA_FLAG_UV_BIT) == 0U);

	zassert_equal(get_assertion("l0nd.example", &none_cred, false, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &none_cred);
}

ZTEST(fido2_cred_protect, test_discovery_skips_protected)
{
	struct cred_ref open_cred;
	struct cred_ref protected_cred;
	struct assertion as;

	zassert_equal(make_credential("mixed.example", "open", FIDO2_CRED_PROTECT_UV_OPTIONAL, true,
				      true, NULL, &open_cred),
		      FIDO2_OK);
	zassert_equal(make_credential("mixed.example", "protected", FIDO2_CRED_PROTECT_UV_REQUIRED,
				      true, true, NULL, &protected_cred),
		      FIDO2_OK);

	/* Only the unprotected credential is visible without UV */
	zassert_equal(get_assertion("mixed.example", NULL, false, &as), FIDO2_OK);
	assert_same_cred(&as.cred, &open_cred);
	zassert_equal(as.num_credentials, 0U);
	zassert_equal(ctap(CTAP_GET_NEXT_ASSERTION, NULL, 0), FIDO2_ERR_NOT_ALLOWED,
		      "getNextAssertion returned a filtered credential");

	/* Both are visible with UV */
	zassert_equal(get_assertion("mixed.example", NULL, true, &as), FIDO2_OK);
	zassert_equal(as.num_credentials, 2U);
	zassert_equal(ctap(CTAP_GET_NEXT_ASSERTION, NULL, 0), FIDO2_OK);
}

ZTEST(fido2_cred_protect, test_exclude_list_hides_uv_required)
{
	struct cred_ref rk_cred;
	struct cred_ref nd_cred;

	zassert_equal(make_credential("excl.example", "u1", FIDO2_CRED_PROTECT_UV_REQUIRED, true,
				      true, NULL, &rk_cred),
		      FIDO2_OK);
	zassert_equal(make_credential("excl.example", "u2", FIDO2_CRED_PROTECT_UV_REQUIRED, false,
				      true, NULL, &nd_cred),
		      FIDO2_OK);

	/* Without UV a level 3 credential must not be revealed by excludeList */
	zassert_equal(make_credential("excl.example", "u3", 0, false, false, &rk_cred, NULL),
		      FIDO2_OK);
	zassert_equal(make_credential("excl.example", "u3", 0, false, false, &nd_cred, NULL),
		      FIDO2_OK);

	/* With UV it is excluded */
	zassert_equal(make_credential("excl.example", "u3", 0, false, true, &rk_cred, NULL),
		      FIDO2_ERR_CREDENTIAL_EXCLUDED);
	zassert_equal(make_credential("excl.example", "u3", 0, false, true, &nd_cred, NULL),
		      FIDO2_ERR_CREDENTIAL_EXCLUDED);
}

ZTEST(fido2_cred_protect, test_level_bound_to_rp)
{
	struct cred_ref cred;

	zassert_equal(make_credential("bound.example", "u1", FIDO2_CRED_PROTECT_UV_REQUIRED, false,
				      true, NULL, &cred),
		      FIDO2_OK);

	/* The wrapped credential does not unwrap for another RP */
	zassert_equal(get_assertion("other.example", &cred, true, NULL), FIDO2_ERR_NO_CREDENTIALS);
}

static void *fido2_cred_protect_setup(void)
{
	zassert_equal(psa_crypto_init(), PSA_SUCCESS);
	zassert_equal(fido2_init(), 0);
	zassert_equal(fido2_start(), 0);

	set_pin();

	return NULL;
}

ZTEST_SUITE(fido2_cred_protect, NULL, fido2_cred_protect_setup, NULL, NULL, NULL);
