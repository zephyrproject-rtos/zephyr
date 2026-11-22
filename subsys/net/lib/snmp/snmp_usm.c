/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 *
 * SNMPv3 User-based Security Model (RFC 3414)
 * HMAC-SHA1/MD5 authentication + AES-128-CFB privacy via PSA Crypto (mbedtls v4.x).
 */

#include <zephyr/net/snmp/snmp_usm.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include <zephyr/net/snmp/snmp_core.h>
#include "snmp_internal.h"
#include <string.h>
#include <errno.h>
#include <zephyr/random/random.h>
#if defined(CONFIG_HWINFO)
#include <zephyr/drivers/hwinfo.h>
#endif

#if defined(CONFIG_SNMP_VERSION_3)
#include <psa/crypto.h>
#endif

#if (defined(CONFIG_SNMP) || defined(CONFIG_SNMP_AGENT)) && defined(CONFIG_SNMP_VERSION_3)

#if defined(__ZEPHYR__)
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(snmp, CONFIG_SNMP_LOG_LEVEL);
#else
#define LOG_WRN(...)
#define LOG_ERR(...)
#endif

#ifndef CONFIG_SNMP_V3_AUTH_PROTOCOL
#define CONFIG_SNMP_V3_AUTH_PROTOCOL SNMP_USM_AUTH_SHA1
#endif
#ifndef CONFIG_SNMP_V3_PRIV_PROTOCOL
#define CONFIG_SNMP_V3_PRIV_PROTOCOL SNMP_USM_PRIV_AES
#endif

#if defined(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

#define HMAC_SHA1_LEN         12
#define HMAC_MD5_LEN          12
#define HMAC_SHA256_LEN       24
#define AES_IV_LEN            16
#define SNMP_ENGINE_BOOTS_MAX 2147483647U

static inline uint8_t usm_auth_mac_len(uint8_t auth_protocol)
{
	switch (auth_protocol) {
	case SNMP_USM_AUTH_SHA256:
		return HMAC_SHA256_LEN;
	case SNMP_USM_AUTH_SHA1:
		return HMAC_SHA1_LEN;
	case SNMP_USM_AUTH_MD5:
		return HMAC_MD5_LEN;
	default:
		return 0;
	}
}

static inline psa_algorithm_t usm_auth_alg(uint8_t auth_protocol)
{
	switch (auth_protocol) {
	case SNMP_USM_AUTH_SHA256:
		return PSA_ALG_HMAC(PSA_ALG_SHA_256);
	case SNMP_USM_AUTH_SHA1:
		return PSA_ALG_HMAC(PSA_ALG_SHA_1);
	case SNMP_USM_AUTH_MD5:
	default:
		return PSA_ALG_HMAC(PSA_ALG_MD5);
	}
}

#if defined(CONFIG_SETTINGS)
/* Buffer for the composed "<prefix>/boots" key. */
#define SNMP_USM_SETTINGS_KEY_MAX 64

/*
 * Compose this engine's engineBoots key from its settings prefix.
 * @return key length on success, -EINVAL without a prefix, -ENAMETOOLONG if it does not
 *         fit. No global settings handler is involved (contract 3.7).
 */
static int usm_boots_key(const struct snmp_agent *inst, char *out, size_t out_sz)
{
	int len;

	if (!inst->settings_prefix) {
		return -EINVAL;
	}

	len = snprintk(out, out_sz, "%s/boots", inst->settings_prefix);
	if (len < 0 || (size_t)len >= out_sz) {
		return -ENAMETOOLONG;
	}
	return len;
}
#endif /* CONFIG_SETTINGS */

/* ---- HMAC computation ---- */

static int hmac_compute(psa_algorithm_t alg, const uint8_t *key, size_t key_len, const uint8_t *in,
			size_t in_len, uint8_t *mac, size_t mac_sz, size_t *mac_len)
{
	psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_SIGN_MESSAGE);

	psa_set_key_algorithm(&attrs, alg);
	psa_set_key_type(&attrs, PSA_KEY_TYPE_HMAC);

	psa_key_id_t key_id;
	psa_status_t status = psa_import_key(&attrs, key, key_len, &key_id);
	if (status != PSA_SUCCESS) {

		return -EIO;
	}

	psa_mac_operation_t op = psa_mac_operation_init();

	status = psa_mac_sign_setup(&op, key_id, alg);
	if (status != PSA_SUCCESS) {
		psa_mac_abort(&op);
		psa_destroy_key(key_id);
		return -EIO;
	}

	status = psa_mac_update(&op, in, in_len);
	if (status != PSA_SUCCESS) {
		psa_mac_abort(&op);
		psa_destroy_key(key_id);
		return -EIO;
	}

	status = psa_mac_sign_finish(&op, mac, mac_sz, mac_len);
	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

/* ---- Password-to-Key (RFC 3414 A.2.1) ---- */

static int usm_password_to_key(psa_algorithm_t hash_alg, const char *password, size_t pw_len,
			       uint8_t *ku, size_t ku_sz, size_t *ku_len)
{
	if (!password || !pw_len || !ku || !ku_len) {
		return -EINVAL;
	}

	psa_hash_operation_t op = psa_hash_operation_init();
	psa_status_t status = psa_hash_setup(&op, hash_alg);
	if (status != PSA_SUCCESS) {

		return -EIO;
	}

	uint8_t buf[64];
	size_t password_index = 0;
	size_t count = 0;

	/* RFC 3414 A.2.1: Process 1,048,576 octets of password cycling in 64-byte chunks */
	while (count < 1048576) {
		for (size_t i = 0; i < 64; i++) {
			buf[i] = (uint8_t)password[password_index++ % pw_len];
		}
		status = psa_hash_update(&op, buf, 64);
		if (status != PSA_SUCCESS) {
			psa_hash_abort(&op);
			return -EIO;
		}
		count += 64;
	}

	status = psa_hash_finish(&op, ku, ku_sz, ku_len);
	if (status != PSA_SUCCESS) {
		psa_hash_abort(&op);
		return -EIO;
	}

	return 0;
}

/* ---- Key localization (RFC 3414 A.2.2 & RFC 3826 §2.4) ---- */

/*
 * The localization hash is always the user's *authentication* protocol, even for the
 * privacy key (RFC 3414 A.2.2 and RFC 3826 §2.4 derive Ku from the auth hash). Passing
 * the privacy protocol here was a bug: SNMP_USM_PRIV_AES == SNMP_USM_AUTH_SHA1 == 1 and
 * SNMP_USM_PRIV_DES == SNMP_USM_AUTH_MD5 == 2, so an AES user looked correct by accident
 * and every non-SHA-1 auth protocol produced a key the peer could not reproduce.
 */
static int usm_key_localize(uint8_t *out_key, uint16_t *out_len, const char *password,
			    uint8_t auth_protocol, const uint8_t *engine_id, uint16_t engine_id_len,
			    bool is_priv)
{
	psa_algorithm_t hash_alg;

	if (!out_key || !out_len || !password || !engine_id) {
		return -EINVAL;
	}

	switch (auth_protocol) {
	case SNMP_USM_AUTH_SHA256:
		hash_alg = PSA_ALG_SHA_256;
		break;
	case SNMP_USM_AUTH_SHA1:
		hash_alg = PSA_ALG_SHA_1;
		break;
	case SNMP_USM_AUTH_MD5:
		hash_alg = PSA_ALG_MD5;
		break;
	default:
		/* No auth protocol means no key to localize at all. */
		return -EINVAL;
	}

	uint8_t ku[64];
	size_t ku_len = 0;
	int ret =
		usm_password_to_key(hash_alg, password, strlen(password), ku, sizeof(ku), &ku_len);
	if (ret < 0) {
		return ret;
	}

	/* RFC 3414 A.2.2 / RFC 7860: Buffer = Ku (hash_size) || engine_id (engine_id_len) || Ku
	 * (hash_size) */
	size_t total_len = ku_len + engine_id_len + ku_len;
	uint8_t buffer[128];
	if (total_len > sizeof(buffer)) {

		return -ENOBUFS;
	}

	memcpy(buffer, ku, ku_len);
	memcpy(buffer + ku_len, engine_id, engine_id_len);
	memcpy(buffer + ku_len + engine_id_len, ku, ku_len);

	uint8_t kul[64];
	size_t kul_len = 0;
	psa_status_t status =
		psa_hash_compute(hash_alg, buffer, total_len, kul, sizeof(kul), &kul_len);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	if (is_priv) {
		/* RFC 3826 §2.4: AES-128 priv key uses first 16 octets (128 bits) */
		if (kul_len < 16) {
			return -EIO;
		}
		memcpy(out_key, kul, 16);
		*out_len = 16;
	} else {
		/* RFC 3414 §6.2.4 / §7.2.4 / RFC 7860: localized auth key (SHA1=20, MD5=16,
		 * SHA256=32) */
		if (kul_len > 32) {
			return -ENOBUFS;
		}
		memcpy(out_key, kul, kul_len);
		*out_len = (uint16_t)kul_len;
	}

	return 0;
}

/* ---- AES-128-CFB Encrypt ---- */

static int aes128_cfb_encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *plaintext,
			      size_t pt_len, uint8_t *ciphertext, size_t ct_sz, size_t *ct_len)
{
#if !defined(CONFIG_SNMP_USM_PRIV_AES)
	(void)key;
	(void)iv;
	(void)plaintext;
	(void)pt_len;
	(void)ciphertext;
	(void)ct_sz;
	(void)ct_len;
	return -ENOTSUP;
#else
	psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_ENCRYPT);

	psa_set_key_algorithm(&attrs, PSA_ALG_CFB);
	psa_set_key_type(&attrs, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attrs, 128);

	psa_key_id_t key_id;
	psa_status_t status = psa_import_key(&attrs, key, 16, &key_id);
	if (status != PSA_SUCCESS) {

		return -EIO;
	}

	psa_cipher_operation_t op = PSA_CIPHER_OPERATION_INIT;
	status = psa_cipher_encrypt_setup(&op, key_id, PSA_ALG_CFB);

	if (status != PSA_SUCCESS) {
		psa_cipher_abort(&op);
		psa_destroy_key(key_id);
		return -EIO;
	}

	status = psa_cipher_set_iv(&op, iv, AES_IV_LEN);
	if (status != PSA_SUCCESS) {
		psa_cipher_abort(&op);
		psa_destroy_key(key_id);
		return -EIO;
	}

	size_t out_len = 0;
	status = psa_cipher_update(&op, plaintext, pt_len, ciphertext, ct_sz, &out_len);

	if (status != PSA_SUCCESS) {
		psa_cipher_abort(&op);
		psa_destroy_key(key_id);
		return -EIO;
	}

	size_t final_len = 0;
	status = psa_cipher_finish(&op, ciphertext + out_len, ct_sz - out_len, &final_len);

	psa_cipher_abort(&op);
	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	*ct_len = out_len + final_len;
	return 0;
#endif
}

/* ---- AES-128-CFB Decrypt ---- */

static int aes128_cfb_decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *ciphertext,
			      size_t ct_len, uint8_t *plaintext, size_t pt_sz, size_t *pt_len)
{
#if !defined(CONFIG_SNMP_USM_PRIV_AES)
	(void)key;
	(void)iv;
	(void)ciphertext;
	(void)ct_len;
	(void)plaintext;
	(void)pt_sz;
	(void)pt_len;
	return -ENOTSUP;
#else
	psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_DECRYPT);

	psa_set_key_algorithm(&attrs, PSA_ALG_CFB);
	psa_set_key_type(&attrs, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attrs, 128);

	psa_key_id_t key_id;
	psa_status_t status = psa_import_key(&attrs, key, 16, &key_id);
	if (status != PSA_SUCCESS) {

		return -EIO;
	}

	psa_cipher_operation_t op = PSA_CIPHER_OPERATION_INIT;
	status = psa_cipher_decrypt_setup(&op, key_id, PSA_ALG_CFB);

	if (status != PSA_SUCCESS) {
		psa_cipher_abort(&op);
		psa_destroy_key(key_id);
		return -EIO;
	}

	status = psa_cipher_set_iv(&op, iv, AES_IV_LEN);
	if (status != PSA_SUCCESS) {
		psa_cipher_abort(&op);
		psa_destroy_key(key_id);
		return -EIO;
	}

	size_t out_len = 0;
	status = psa_cipher_update(&op, ciphertext, ct_len, plaintext, pt_sz, &out_len);

	if (status != PSA_SUCCESS) {
		psa_cipher_abort(&op);
		psa_destroy_key(key_id);
		return -EIO;
	}

	size_t final_len = 0;
	status = psa_cipher_finish(&op, plaintext + out_len, pt_sz - out_len, &final_len);

	psa_cipher_abort(&op);
	psa_destroy_key(key_id);

	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	*pt_len = out_len + final_len;
	return 0;
#endif
}

/* ---- Find encrypted scopedPDU OCTET STRING in raw buffer ---- */

static int find_encrypted_scopedpdu(const uint8_t *buf, size_t buf_len, const uint8_t **ct_data,
				    size_t *ct_len)
{
	size_t off = 0;
	uint8_t tag;
	size_t len;
	int ret;

	ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {
		return -EBADMSG;
	}

	ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_INTEGER) {
		return -EBADMSG;
	}
	off += len;

	ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {
		return -EBADMSG;
	}
	off += len;

	ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_OCTET_STRING) {
		return -EBADMSG;
	}
	off += len;

	ret = snmp_ber_decode_header(buf, buf_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_OCTET_STRING) {
		return -EBADMSG;
	}

	*ct_data = &buf[off];
	*ct_len = len;
	return 0;
}

/* ---- Parse decrypted scopedPDU plaintext into msg->scoped ---- */

static int parse_scopedpdu_plaintext(const uint8_t *data, size_t data_len,
				     struct snmp_v3_scoped_pdu *scoped)
{
	size_t off = 0;
	uint8_t tag;
	size_t len;
	int ret;
	uint16_t tmp_len;

	ret = snmp_ber_decode_header(data, data_len, &off, &tag, &len);
	if (ret < 0 || tag != ASN1_TAG_SEQUENCE) {
		return -EBADMSG;
	}

	ret = snmp_ber_decode_octet_string(data, data_len, &off, scoped->context_engine_id,
					   SNMP_V3_ENGINE_ID_MAX_LEN, &tmp_len);
	if (ret < 0) {
		return ret;
	}
	scoped->context_engine_id_len = tmp_len;

	ret = snmp_ber_decode_octet_string(data, data_len, &off, scoped->context_name,
					   sizeof(scoped->context_name) - 1, &tmp_len);
	if (ret < 0) {
		return ret;
	}
	scoped->context_name_len = tmp_len;
	scoped->context_name[tmp_len] = '\0';

	/* No version field exists in msgData; keep v2 semantics for the MIB engine. */
	scoped->pdu.version = 3;

	return snmp_ber_decode_pdu_body(data, data_len, &off, &scoped->pdu);
}

/* ---- Build plaintext scopedPDU bytes for encryption ---- */

static int build_scopedpdu_plaintext(const struct snmp_v3_msg *msg, uint8_t *out, size_t out_sz,
				     size_t *out_len)
{
	int ret;
	uint8_t body[CONFIG_SNMP_MAX_PDU_SIZE];
	size_t body_off = 0;

	ret = snmp_ber_encode_octet_string(body, sizeof(body), &body_off, ASN1_TAG_OCTET_STRING,
					   msg->scoped.context_engine_id,
					   msg->scoped.context_engine_id_len);
	if (ret < 0) {
		return ret;
	}

	ret = snmp_ber_encode_octet_string(body, sizeof(body), &body_off, ASN1_TAG_OCTET_STRING,
					   (const uint8_t *)msg->scoped.context_name,
					   msg->scoped.context_name_len);
	if (ret < 0) {
		return ret;
	}

	size_t pdu_len = 0;
	ret = snmp_ber_encode_pdu_body(&body[body_off], sizeof(body) - body_off, &pdu_len,

				       &msg->scoped.pdu);
	if (ret < 0) {
		return ret;
	}
	body_off += pdu_len;

	size_t hdr_off = 0;
	ret = snmp_ber_encode_header(out, out_sz, &hdr_off, ASN1_TAG_SEQUENCE, body_off);

	if (ret < 0) {
		return ret;
	}

	if (hdr_off + body_off > out_sz) {
		return -ENOBUFS;
	}
	memcpy(&out[hdr_off], body, body_off);
	*out_len = hdr_off + body_off;
	return 0;
}

/* ---- User lookup ---- */

static struct snmp_usm_user *usm_find_user(struct snmp_agent *inst, const char *name)
{
	if (!inst || !name) {
		return NULL;
	}
	for (size_t i = 0; i < CONFIG_SNMP_V3_MAX_USERS; i++) {
		if (inst->users[i].in_use && strcmp(inst->users[i].user_name, name) == 0) {
			return &inst->users[i];
		}
	}
	return NULL;
}

/* ---- HMAC computation over the datagram as received (RFC 3414 §6.3.1) ---- */

/*
 * Compute HMAC over (prefix || zeros || suffix) without materialising a copy of the
 * message. The authentication parameter is zeroed in the *received* octets, exactly as
 * the sender did, so a peer that chose a different valid BER length encoding still
 * verifies. The previous implementation re-encoded the message, which changed those
 * lengths and made authentication depend on both sides encoding identically.
 */
static int hmac_compute_raw(psa_algorithm_t alg, const uint8_t *key, size_t key_len,
			    const uint8_t *raw, size_t raw_len, size_t zero_off, size_t zero_len,
			    uint8_t *mac, size_t mac_sz, size_t *mac_len)
{
	static const uint8_t zeros[SNMP_V3_AUTH_PARAMS_LEN];
	psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	psa_mac_operation_t op = psa_mac_operation_init();
	psa_status_t status;

	if (zero_len > sizeof(zeros) || zero_off > raw_len || zero_len > raw_len - zero_off) {
		return -EINVAL;
	}

	psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attrs, alg);
	psa_set_key_type(&attrs, PSA_KEY_TYPE_HMAC);

	status = psa_import_key(&attrs, key, key_len, &key_id);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	status = psa_mac_sign_setup(&op, key_id, alg);
	if (status != PSA_SUCCESS) {
		goto fail;
	}

	status = psa_mac_update(&op, raw, zero_off);
	if (status != PSA_SUCCESS) {
		goto fail;
	}

	status = psa_mac_update(&op, zeros, zero_len);
	if (status != PSA_SUCCESS) {
		goto fail;
	}

	status = psa_mac_update(&op, &raw[zero_off + zero_len], raw_len - zero_off - zero_len);
	if (status != PSA_SUCCESS) {
		goto fail;
	}

	status = psa_mac_sign_finish(&op, mac, mac_sz, mac_len);
	if (status == PSA_SUCCESS) {
		psa_destroy_key(key_id);
		return 0;
	}

fail:
	psa_mac_abort(&op);
	psa_destroy_key(key_id);
	return -EIO;
}

/* ---- HMAC verification ---- */

static int usm_verify_auth(const struct snmp_v3_msg *msg, const struct snmp_usm_user *user,
			   const uint8_t *raw_buf, size_t raw_len)
{
	uint8_t trunc;

	if (!msg || !user || !raw_buf) {
		return -EINVAL;
	}

	trunc = usm_auth_mac_len(user->auth_protocol);
	if (trunc == 0) {
		return -EINVAL;
	}

	/*
	 * RFC 3414 §6.3.2: a message whose authentication parameter is not the length
	 * implied by the negotiated protocol is not authentic.
	 */
	if (msg->usm.auth_params_len != trunc) {
		return -EACCES;
	}

	if (msg->usm.auth_params_raw_off == 0 || msg->usm.auth_params_raw_off > raw_len ||
	    trunc > raw_len - msg->usm.auth_params_raw_off) {
		return -EBADMSG;
	}

	uint8_t computed[64];
	size_t computed_len = 0;
	int ret =
		hmac_compute_raw(usm_auth_alg(user->auth_protocol), user->auth_key,
				 user->auth_key_len, raw_buf, raw_len, msg->usm.auth_params_raw_off,
				 trunc, computed, sizeof(computed), &computed_len);
	if (ret < 0) {
		return ret;
	}
	if (computed_len < trunc) {
		return -EACCES;
	}

	/* Constant-time compare: the digest is attacker-supplied and recomputed locally. */
	uint8_t diff = 0;

	for (size_t i = 0; i < trunc; i++) {
		diff |= computed[i] ^ msg->usm.auth_params[i];
	}
	if (diff != 0) {
		return -EACCES;
	}

	return 0;
}

/* ---- Instance-based APIs ---- */

/*
 * Validate a user record before it can enter the local keys table. RFC 3414 §11.2
 * requires pass phrases of at least 8 octets; authPriv without an authentication
 * protocol is meaningless (privacy keys are localized through the auth hash), and DES
 * is not implemented, so accepting it would only fail later at runtime.
 */
static int usm_user_validate(const struct snmp_usm_user *user)
{
	size_t name_len;
	size_t auth_len;
	size_t priv_len;

	if (!user) {
		return -EINVAL;
	}

	name_len = snmp_strnlen(user->user_name, sizeof(user->user_name));
	if (name_len == 0 || name_len >= sizeof(user->user_name)) {
		return -EINVAL;
	}

	switch (user->auth_protocol) {
	case SNMP_USM_AUTH_NONE:
	case SNMP_USM_AUTH_SHA1:
	case SNMP_USM_AUTH_MD5:
	case SNMP_USM_AUTH_SHA256:
		break;
	default:
		return -EINVAL;
	}

	switch (user->priv_protocol) {
	case SNMP_USM_PRIV_NONE:
	case SNMP_USM_PRIV_AES:
		break;
	case SNMP_USM_PRIV_DES:
	default:
		/* DES-CBC is advertised nowhere in this implementation. */
		return -ENOTSUP;
	}

	if (user->priv_protocol != SNMP_USM_PRIV_NONE &&
	    user->auth_protocol == SNMP_USM_AUTH_NONE) {
		return -EINVAL;
	}

	auth_len = snmp_strnlen(user->auth_pass, sizeof(user->auth_pass));
	priv_len = snmp_strnlen(user->priv_pass, sizeof(user->priv_pass));

	if (auth_len >= sizeof(user->auth_pass) || priv_len >= sizeof(user->priv_pass)) {
		return -EINVAL;
	}
	if (user->auth_protocol != SNMP_USM_AUTH_NONE && auth_len < 8) {
		return -EINVAL;
	}
	if (user->priv_protocol != SNMP_USM_PRIV_NONE && priv_len < 8) {
		return -EINVAL;
	}
	if (user->priv_protocol == SNMP_USM_PRIV_AES && !IS_ENABLED(CONFIG_SNMP_USM_PRIV_AES)) {
		return -ENOTSUP;
	}

	return 0;
}

/* Localize the configured pass phrases into this engine's keys. */
static int usm_localize_user(struct snmp_agent *inst, struct snmp_usm_user *user)
{
	int ret;

	user->auth_key_len = 0;
	user->priv_key_len = 0;

	if (user->auth_protocol != SNMP_USM_AUTH_NONE) {
		ret = usm_key_localize(user->auth_key, &user->auth_key_len, user->auth_pass,
				       user->auth_protocol, inst->engine_id, inst->engine_id_len,
				       false);
		if (ret < 0) {
			return ret;
		}
	}
	if (user->priv_protocol != SNMP_USM_PRIV_NONE) {
		ret = usm_key_localize(user->priv_key, &user->priv_key_len, user->priv_pass,
				       user->auth_protocol, inst->engine_id, inst->engine_id_len,
				       true);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

/* Caller holds inst->lock. */
static int usm_add_user_locked(struct snmp_agent *inst, const struct snmp_usm_user *user)
{
	int ret = usm_user_validate(user);

	if (ret < 0) {
		return ret;
	}

	/* Update existing if found */
	for (size_t i = 0; i < CONFIG_SNMP_V3_MAX_USERS; i++) {
		if (inst->users[i].in_use &&
		    strcmp(inst->users[i].user_name, user->user_name) == 0) {
			struct snmp_usm_user tmp = *user;

			tmp.in_use = true;
			ret = usm_localize_user(inst, &tmp);
			if (ret < 0) {
				return ret;
			}
			inst->users[i] = tmp;
			return 0;
		}
	}

	/* Find free slot */
	for (size_t i = 0; i < CONFIG_SNMP_V3_MAX_USERS; i++) {
		if (!inst->users[i].in_use) {
			struct snmp_usm_user tmp = *user;

			tmp.in_use = true;
			ret = usm_localize_user(inst, &tmp);
			if (ret < 0) {
				return ret;
			}
			inst->users[i] = tmp;
			inst->user_cnt++;
			return 0;
		}
	}

	return -ENOMEM;
}

int snmp_usm_add_user(struct snmp_agent *inst, const struct snmp_usm_user *user)
{
	int ret;

	if (!inst || !user) {
		return -EINVAL;
	}
	if (inst->engine_id_len == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&inst->lock, K_FOREVER);
	ret = usm_add_user_locked(inst, user);
	k_mutex_unlock(&inst->lock);
	return ret;
}

/*
 * Default snmpEngineID when the application does not supply one (RFC 3411 §5).
 *
 * Layout: enterprise number with the high bit set (0x80001F88), then the format
 * octet 0x05 ("octets") and the engine identifier itself. Using the SoC unique
 * device ID keeps two boards running the same firmware from deriving identical
 * localized keys out of the same pass phrase, which is exactly what a single
 * hard-coded constant does. Boards without a device ID fall back to the historical
 * constant; applications can always override the value through cfg.engine_id.
 */
static void usm_default_engine_id(struct snmp_agent *inst)
{
	static const uint8_t prefix[5] = {0x80, 0x00, 0x1F, 0x88, 0x05};
	uint8_t dev_id[12]; /* STM32H7 exposes a 96-bit UID; drivers clamp to the buffer */
	ssize_t id_len = -1;

	inst->engine_id_len = 0;

#if defined(CONFIG_HWINFO)
	id_len = hwinfo_get_device_id(dev_id, sizeof(dev_id));
#endif

	if (id_len > 0 && (size_t)id_len <= sizeof(dev_id)) {
		memcpy(inst->engine_id, prefix, sizeof(prefix));
		memcpy(&inst->engine_id[sizeof(prefix)], dev_id, (size_t)id_len);
		inst->engine_id_len = (uint8_t)(sizeof(prefix) + (size_t)id_len);
		return;
	}

	memcpy(inst->engine_id, prefix, sizeof(prefix));
	memcpy(&inst->engine_id[sizeof(prefix)], "ZEPHYR1", 7);
	inst->engine_id_len = (uint8_t)(sizeof(prefix) + 7);
}

int snmp_usm_init(struct snmp_agent *inst)
{
	int ret = 0;

	if (!inst) {
		return -EINVAL;
	}

	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed: %d", (int)status);
		return -EIO;
	}

	k_mutex_lock(&inst->lock, K_FOREVER);

	if (inst->engine_id_len == 0) {
		usm_default_engine_id(inst);
	}

	/*
	 * engineBoots comes from struct snmp_agent_cfg (applied by snmp_agent_init
	 * before calling us) or from Kconfig. Persistence is explicit and per instance:
	 * snmp_agent_load_engine_boots() reads this engine's own key, instead of a
	 * process-wide settings handler that could only ever feed the default agent.
	 */
	if (inst->engine_boots == 0) {
#if defined(CONFIG_SNMP_V3_ENGINE_BOOTS)
		inst->engine_boots = CONFIG_SNMP_V3_ENGINE_BOOTS;
#else
		inst->engine_boots = 1;
#endif
	}

	/*
	 * RFC 3826 §3.1.1.3: the salt space is per key, seeded once per engine. A
	 * cryptographic RNG failure must not silently degrade into a predictable salt,
	 * so the init call reports it and the caller can refuse to start.
	 */
	if (sys_csrand_get(&inst->priv_salt, sizeof(inst->priv_salt)) != 0) {
		inst->priv_salt = 0;
		ret = -EIO;
		goto unlock;
	}

	if (inst->user_cnt == 0) {
		struct snmp_usm_user default_user = {
			.auth_protocol = CONFIG_SNMP_V3_AUTH_PROTOCOL,
			.priv_protocol = CONFIG_SNMP_V3_PRIV_PROTOCOL,
		};
		size_t len;

		len = snmp_strnlen(CONFIG_SNMP_V3_USER_NAME, sizeof(default_user.user_name));
		if (len == 0 || len >= sizeof(default_user.user_name)) {
			ret = -EINVAL;
			goto unlock;
		}
		memcpy(default_user.user_name, CONFIG_SNMP_V3_USER_NAME, len);
		default_user.user_name[len] = '\0';

		len = snmp_strnlen(CONFIG_SNMP_V3_AUTH_PASS, sizeof(default_user.auth_pass));
		if (len >= sizeof(default_user.auth_pass)) {
			ret = -EINVAL;
			goto unlock;
		}
		memcpy(default_user.auth_pass, CONFIG_SNMP_V3_AUTH_PASS, len);
		default_user.auth_pass[len] = '\0';

		len = snmp_strnlen(CONFIG_SNMP_V3_PRIV_PASS, sizeof(default_user.priv_pass));
		if (len >= sizeof(default_user.priv_pass)) {
			ret = -EINVAL;
			goto unlock;
		}
		memcpy(default_user.priv_pass, CONFIG_SNMP_V3_PRIV_PASS, len);
		default_user.priv_pass[len] = '\0';

		ret = usm_add_user_locked(inst, &default_user);
	} else {
		/* Re-localize existing users with the current engine_id. */
		for (size_t i = 0; i < CONFIG_SNMP_V3_MAX_USERS; i++) {
			if (inst->users[i].in_use) {
				int lret = usm_localize_user(inst, &inst->users[i]);

				if (lret < 0 && ret == 0) {
					ret = lret;
				}
			}
		}
	}

unlock:
	k_mutex_unlock(&inst->lock);
	return ret;
}

/* ---- USM Report generation (RFC 3414 §3.2) ---- */

/* usmStats OIDs under 1.3.6.1.6.3.15.1.1 */
static const uint32_t s_oid_unsupported_sec_levels[] = {1, 3, 6, 1, 6, 3, 15, 1, 1, 1, 0};
static const uint32_t s_oid_not_in_time_windows[] = {1, 3, 6, 1, 6, 3, 15, 1, 1, 2, 0};
static const uint32_t s_oid_unknown_user_names[] = {1, 3, 6, 1, 6, 3, 15, 1, 1, 3, 0};
static const uint32_t s_oid_unknown_engine_ids[] = {1, 3, 6, 1, 6, 3, 15, 1, 1, 4, 0};

#define USM_REPORT_OID_LEN 11

/*
 * Turn the incoming message into a Report PDU carrying one usmStats counter.
 *
 * RFC 3414 §3.2 only permits a Report when the reportable flag was set; otherwise the
 * message is discarded. @p authenticated selects authNoPriv (used for reports where the
 * sender's identity and digest were verified) versus noAuthNoPriv.
 */
static int usm_build_report(struct snmp_agent *inst, struct snmp_v3_msg *msg, const uint32_t *oid,
			    uint32_t value, bool authenticated)
{
	struct snmp_pdu *pdu = &msg->scoped.pdu;

	if ((msg->global.msg_flags & 0x04) == 0) {
		return -EACCES;
	}

	memset(pdu, 0, sizeof(*pdu));
	pdu->type = SNMP_PDU_REPORT;
	pdu->version = 3;
	pdu->request_id = msg->global.msg_id;
	pdu->varbind_cnt = 1;

	pdu->varbinds[0].oid.len = USM_REPORT_OID_LEN;
	memcpy(pdu->varbinds[0].oid.ids, oid, USM_REPORT_OID_LEN * sizeof(uint32_t));
	pdu->varbinds[0].type = SNMP_TAG_COUNTER32;
	pdu->varbinds[0].val.uint_val = value;

	memcpy(msg->scoped.context_engine_id, inst->engine_id, inst->engine_id_len);
	msg->scoped.context_engine_id_len = inst->engine_id_len;
	msg->scoped.context_name_len = 0;

	if (authenticated) {
		msg->global.msg_flags = 0x01;
		msg->sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	} else {
		msg->global.msg_flags = 0x00;
		msg->usm.user_name_len = 0;
		msg->usm.auth_params_len = 0;
		msg->usm.priv_params_len = 0;
		msg->sec_level = SNMP_SEC_LEVEL_NOAUTH_NOPRIV;
	}

	return 0;
}

int snmp_usm_verify_incoming(struct snmp_agent *inst, struct snmp_v3_msg *msg,
			     const uint8_t *raw_buf, size_t raw_len)
{
	bool req_auth;
	bool req_priv;
	bool engine_known;
	struct snmp_usm_user *user;
	int ret;

	if (!inst || !msg || !raw_buf) {
		return -EINVAL;
	}

	/* RFC 3414 §2.2: if engine boots reaches 2147483647, stop processing */
	if (inst->engine_boots >= SNMP_ENGINE_BOOTS_MAX ||
	    msg->usm.engine_boots >= SNMP_ENGINE_BOOTS_MAX) {
		return -EACCES;
	}

	req_auth = (msg->global.msg_flags & 0x01) != 0;
	req_priv = (msg->global.msg_flags & 0x02) != 0;

	/* RFC 3414 §3.2: authPriv implies auth; priv without auth is not a level. */
	if (req_priv && !req_auth) {
		snmp_counter_inc(&inst->usm_stats.unsupported_sec_levels);
		return usm_build_report(inst, msg, s_oid_unsupported_sec_levels,
					inst->usm_stats.unsupported_sec_levels, false);
	}

	/*
	 * Engine discovery / unknown engine: boots and time both zero, an empty engine
	 * ID, or an ID that is not ours. RFC 3414 §3.2 step 4 requires a Report carrying
	 * usmStatsUnknownEngineIDs, at noAuthNoPriv because no key can be assumed yet.
	 */
	engine_known = msg->usm.engine_id_len == inst->engine_id_len &&
		       msg->usm.engine_id_len > 0 &&
		       memcmp(msg->usm.engine_id, inst->engine_id, inst->engine_id_len) == 0;

	if (!engine_known || (msg->usm.engine_boots == 0 && msg->usm.engine_time == 0)) {
		snmp_counter_inc(&inst->usm_stats.unknown_engine_ids);
		return usm_build_report(inst, msg, s_oid_unknown_engine_ids,
					inst->usm_stats.unknown_engine_ids, false);
	}

	user = usm_find_user(inst, (const char *)msg->usm.user_name);
	if (!user) {
		snmp_counter_inc(&inst->usm_stats.unknown_user_names);
		return usm_build_report(inst, msg, s_oid_unknown_user_names,
					inst->usm_stats.unknown_user_names, false);
	}

	/*
	 * The requested level must be one this user can actually provide. Reporting is
	 * done unauthenticated: the message has not been verified at this point.
	 */
	if ((req_auth && user->auth_protocol == SNMP_USM_AUTH_NONE) ||
	    (req_priv && user->priv_protocol == SNMP_USM_PRIV_NONE)) {
		snmp_counter_inc(&inst->usm_stats.unsupported_sec_levels);
		return usm_build_report(inst, msg, s_oid_unsupported_sec_levels,
					inst->usm_stats.unsupported_sec_levels, false);
	}

	if (req_auth) {
		ret = usm_verify_auth(msg, user, raw_buf, raw_len);
		if (ret < 0) {
			/* RFC 3414 §3.2 step 6: wrong digest is discarded, never reported. */
			snmp_counter_inc(&inst->usm_stats.wrong_digests);
			return -EACCES;
		}
	}

	if (req_priv) {
		if (msg->usm.priv_params_len != 8) {
			snmp_counter_inc(&inst->usm_stats.decryption_errors);
			return -EACCES;
		}

		const uint8_t *ct_data = NULL;
		size_t ct_len = 0;

		ret = find_encrypted_scopedpdu(raw_buf, raw_len, &ct_data, &ct_len);
		if (ret < 0) {
			snmp_counter_inc(&inst->usm_stats.decryption_errors);
			return -EACCES;
		}

		/* RFC 3826 §3.1.1.1 & §3.1.4: 16-byte IV = boots (4 BE) || time (4 BE) || salt (8)
		 */
		uint8_t iv[AES_IV_LEN];
		iv[0] = (uint8_t)(msg->usm.engine_boots >> 24);

		iv[1] = (uint8_t)(msg->usm.engine_boots >> 16);
		iv[2] = (uint8_t)(msg->usm.engine_boots >> 8);
		iv[3] = (uint8_t)(msg->usm.engine_boots);
		iv[4] = (uint8_t)(msg->usm.engine_time >> 24);
		iv[5] = (uint8_t)(msg->usm.engine_time >> 16);
		iv[6] = (uint8_t)(msg->usm.engine_time >> 8);
		iv[7] = (uint8_t)(msg->usm.engine_time);
		memcpy(&iv[8], msg->usm.priv_params, 8);

		uint8_t pt[CONFIG_SNMP_MAX_PDU_SIZE];
		size_t pt_len = 0;

		ret = aes128_cfb_decrypt(user->priv_key, iv, ct_data, ct_len, pt, sizeof(pt),
					 &pt_len);
		if (ret < 0) {
			snmp_counter_inc(&inst->usm_stats.decryption_errors);
			return -EACCES;
		}

		ret = parse_scopedpdu_plaintext(pt, pt_len, &msg->scoped);
		if (ret < 0) {
			snmp_counter_inc(&inst->usm_stats.decryption_errors);
			return -EACCES;
		}
	}

	/*
	 * RFC 3414 §3.2 step 7: the timeliness window is only meaningful for
	 * authenticated messages. An unauthenticated engineTime is attacker-controlled,
	 * so checking it would prove nothing and only block legitimate noAuthNoPriv use.
	 */
	if (req_auth) {
		int64_t diff =
			(int64_t)msg->usm.engine_time - (int64_t)snmp_usm_get_engine_time(inst);

		if (diff < -150 || diff > 150 || msg->usm.engine_boots != inst->engine_boots) {
			snmp_counter_inc(&inst->usm_stats.not_in_time_windows);
			return usm_build_report(inst, msg, s_oid_not_in_time_windows,
						inst->usm_stats.not_in_time_windows, true);
		}
	}

	/* Verified security level, not msgFlags: VACM must never trust the peer's claim. */
	if (req_priv) {
		msg->sec_level = SNMP_SEC_LEVEL_AUTH_PRIV;
	} else if (req_auth) {
		msg->sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	} else {
		msg->sec_level = SNMP_SEC_LEVEL_NOAUTH_NOPRIV;
	}

	return 0;
}

/*
 * Reconstruct msgFlags from a verified/desired security level. Responses and traps are
 * never reportable, so only the auth/priv bits are significant here.
 */
static uint8_t usm_flags_from_level(uint8_t level)
{
	switch (level) {
	case SNMP_SEC_LEVEL_AUTH_PRIV:
		return 0x03;
	case SNMP_SEC_LEVEL_AUTH_NOPRIV:
		return 0x01;
	default:
		return 0x00;
	}
}

int snmp_usm_build_outgoing(struct snmp_agent *inst, struct snmp_v3_msg *msg)
{
	struct snmp_usm_user *user = NULL;
	uint8_t pdu_type;

	if (!inst || !msg) {
		return -EINVAL;
	}

	/* RFC 3414 §2.2: if engine boots reaches 2147483647, stop processing */
	if (inst->engine_boots >= SNMP_ENGINE_BOOTS_MAX) {
		return -EACCES;
	}

	memcpy(msg->usm.engine_id, inst->engine_id, inst->engine_id_len);
	msg->usm.engine_id_len = inst->engine_id_len;
	msg->usm.engine_boots = inst->engine_boots;
	msg->usm.engine_time = snmp_usm_get_engine_time(inst);

	pdu_type = msg->scoped.pdu.type;

	if (pdu_type == SNMP_PDU_RESPONSE || pdu_type == SNMP_PDU_TRAP_V2) {
		/*
		 * A response must mirror the request's security level (RFC 3412 §7.1.6)
		 * and a trap uses the level configured for the notification user. Both
		 * are expressed by msg->sec_level; the reportable bit is never set.
		 */
		msg->global.msg_flags = usm_flags_from_level(msg->sec_level);
	} else if (pdu_type == SNMP_PDU_REPORT) {
		/*
		 * Keep the level chosen by usm_build_report(): reports for an unknown engine
		 * or user go out unauthenticated, notInTimeWindows is authenticated.
		 */
		msg->global.msg_flags &= (0x01 | 0x02);
	} else {
		/* Requests (Get/GetNext/GetBulk/Set/Inform) are reportable. An outgoing
		 * Inform also carries the notification user's security level. */
		msg->global.msg_flags = 0x04 | usm_flags_from_level(msg->sec_level);
	}

	if (msg->global.msg_flags & (0x01 | 0x02)) {
		user = usm_find_user(inst, (const char *)msg->usm.user_name);
		if (!user) {
			return -EACCES;
		}

		if ((msg->global.msg_flags & 0x01) && user->auth_protocol == SNMP_USM_AUTH_NONE) {
			return -EACCES;
		}
		if ((msg->global.msg_flags & 0x02) && user->priv_protocol == SNMP_USM_PRIV_NONE) {
			return -EACCES;
		}

		if (msg->global.msg_flags & 0x01) {
			msg->usm.auth_params_len = usm_auth_mac_len(user->auth_protocol);
		} else {
			msg->usm.auth_params_len = 0;
		}
	} else {
		msg->usm.auth_params_len = 0;
		msg->usm.priv_params_len = 0;
	}

	if (msg->global.msg_flags & 0x02) {
		/* RFC 3826 §3.1.1.3: per-engine monotonic salt. */
		uint64_t salt = ++inst->priv_salt;

		msg->usm.priv_params[0] = (uint8_t)(salt >> 56);
		msg->usm.priv_params[1] = (uint8_t)(salt >> 48);
		msg->usm.priv_params[2] = (uint8_t)(salt >> 40);
		msg->usm.priv_params[3] = (uint8_t)(salt >> 32);
		msg->usm.priv_params[4] = (uint8_t)(salt >> 24);
		msg->usm.priv_params[5] = (uint8_t)(salt >> 16);
		msg->usm.priv_params[6] = (uint8_t)(salt >> 8);
		msg->usm.priv_params[7] = (uint8_t)(salt);
		msg->usm.priv_params_len = 8;

		/* RFC 3826 §3.1.1.1: 16-byte IV = boots (4 BE) || time (4 BE) || salt (8) */
		uint8_t iv[AES_IV_LEN];
		iv[0] = (uint8_t)(inst->engine_boots >> 24);

		iv[1] = (uint8_t)(inst->engine_boots >> 16);
		iv[2] = (uint8_t)(inst->engine_boots >> 8);
		iv[3] = (uint8_t)(inst->engine_boots);
		iv[4] = (uint8_t)(msg->usm.engine_time >> 24);
		iv[5] = (uint8_t)(msg->usm.engine_time >> 16);
		iv[6] = (uint8_t)(msg->usm.engine_time >> 8);
		iv[7] = (uint8_t)(msg->usm.engine_time);
		memcpy(&iv[8], msg->usm.priv_params, 8);

		uint8_t plaintext[CONFIG_SNMP_MAX_PDU_SIZE];
		size_t pt_len = 0;
		int ret = build_scopedpdu_plaintext(msg, plaintext, sizeof(plaintext), &pt_len);

		if (ret < 0) {
			return ret;
		}

		size_t ct_len = 0;

		ret = aes128_cfb_encrypt(user->priv_key, iv, plaintext, pt_len,
					 msg->scoped.priv_ciphertext,
					 sizeof(msg->scoped.priv_ciphertext), &ct_len);
		if (ret < 0) {
			return ret;
		}

		msg->scoped.priv_ciphertext_len = ct_len;
	}

	memset(msg->usm.auth_params, 0, sizeof(msg->usm.auth_params));

	if (user != NULL && ((msg->global.msg_flags & 0x01U) != 0U)) {
		uint8_t hash_target[CONFIG_SNMP_MAX_PDU_SIZE];
		size_t hash_len = 0;
		int ret = snmp_ber_encode_v3_msg(hash_target, sizeof(hash_target), &hash_len, msg);

		if (ret < 0) {
			return ret;
		}

		uint8_t full_hash[64];
		size_t fh_len = 0;

		ret = hmac_compute(usm_auth_alg(user->auth_protocol), user->auth_key,
				   user->auth_key_len, hash_target, hash_len, full_hash,
				   sizeof(full_hash), &fh_len);
		if (ret < 0) {
			return ret;
		}

		uint8_t trunc = usm_auth_mac_len(user->auth_protocol);

		if (fh_len < trunc) {
			return -EIO;
		}
		memcpy(msg->usm.auth_params, full_hash, trunc);
	}

	return 0;
}

int snmp_usm_get_engine_id(struct snmp_agent *inst, uint8_t *engine_id, uint8_t *len)
{
	if (inst == NULL || engine_id == NULL || len == NULL) {
		return -EINVAL;
	}
	if (*len < inst->engine_id_len) {
		return -ENOBUFS;
	}
	memcpy(engine_id, inst->engine_id, inst->engine_id_len);
	*len = inst->engine_id_len;
	return 0;
}

int snmp_usm_set_engine_boots(struct snmp_agent *inst, uint32_t boots)
{
	if (inst == NULL || boots > SNMP_ENGINE_BOOTS_MAX) {
		return -EINVAL;
	}
	k_mutex_lock(&inst->lock, K_FOREVER);
	inst->engine_boots = boots;
#if defined(CONFIG_SETTINGS)
	/* Persist for whichever engine is configured to persist: the default-instance
	 * guard is gone (contract 3.7). */
	if (inst->settings_prefix != NULL) {
		char key[SNMP_USM_SETTINGS_KEY_MAX];

		if (usm_boots_key(inst, key, sizeof(key)) > 0) {
			int rc = settings_save_one(key, &inst->engine_boots,
						   sizeof(inst->engine_boots));

			if (rc < 0) {
				LOG_WRN("Failed to persist engineBoots (%d)", rc);
			}
		}
	}
#endif
	k_mutex_unlock(&inst->lock);
	return 0;
}

int snmp_agent_load_engine_boots(struct snmp_agent *agent)
{
	if (!agent) {
		return -EINVAL;
	}
#if defined(CONFIG_SETTINGS)
	if (!agent->settings_prefix) {
		return -EINVAL;
	}

	char key[SNMP_USM_SETTINGS_KEY_MAX];
	int klen = usm_boots_key(agent, key, sizeof(key));

	if (klen < 0) {
		return klen;
	}

	(void)settings_subsys_init();

	uint32_t persisted = 0;
	ssize_t rd = settings_load_one(key, &persisted, sizeof(persisted));

	if (rd != (ssize_t)sizeof(persisted)) {
		return (rd < 0) ? (int)rd : -ENOENT;
	}

	/*
	 * RFC 3414 3.1.1: a reboot advances the window. The stored value is the boot
	 * count at the end of the previous life, so take the larger of the two,
	 * increment below the reportable maximum, and write the new one back.
	 */
	if (agent->engine_boots < persisted) {
		agent->engine_boots = persisted;
	}
	if (agent->engine_boots < SNMP_ENGINE_BOOTS_MAX) {
		agent->engine_boots++;
	}
	int rc = settings_save_one(key, &agent->engine_boots, sizeof(agent->engine_boots));

	if (rc < 0) {
		LOG_WRN("Failed to persist engineBoots (%d)", rc);
		return rc;
	}
	return 0;
#else
	(void)agent;
	return -ENOTSUP;
#endif
}

uint32_t snmp_usm_get_engine_boots(struct snmp_agent *inst)
{
	return inst ? inst->engine_boots : 0;
}

uint32_t snmp_usm_get_engine_time(struct snmp_agent *inst)
{
	if (!inst) {
		return 0;
	}

	int64_t uptime_sec = k_uptime_get() / 1000;
	int64_t elapsed = uptime_sec - inst->engine_time_base;

	while (elapsed >= (int64_t)SNMP_ENGINE_BOOTS_MAX) {
		if (inst->engine_boots < SNMP_ENGINE_BOOTS_MAX) {
			inst->engine_boots++;
#if defined(CONFIG_SETTINGS)
			if (inst->settings_prefix != NULL) {
				char key[SNMP_USM_SETTINGS_KEY_MAX];

				if (usm_boots_key(inst, key, sizeof(key)) > 0) {
					int rc = settings_save_one(key, &inst->engine_boots,
								   sizeof(inst->engine_boots));

					if (rc < 0) {
						LOG_WRN("Failed to persist engineBoots (%d)", rc);
					}
				}
			}
#endif
		}
		inst->engine_time_base += (int64_t)SNMP_ENGINE_BOOTS_MAX;
		elapsed = uptime_sec - inst->engine_time_base;
	}

	return (uint32_t)elapsed;
}

int snmp_usm_set_engine_time(struct snmp_agent *inst, uint32_t time)
{
	if (!inst || time > SNMP_ENGINE_BOOTS_MAX) {
		return -EINVAL;
	}

	int64_t uptime_sec = k_uptime_get() / 1000;
	inst->engine_time_base = uptime_sec - (int64_t)time;

	return 0;
}

/* ---- RFC 3411 (Framework MIB), RFC 3412 (MPD MIB) and RFC 3414 (USM MIB) Managed Objects ---- */

static int usm_engine_id_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			     struct snmp_varbind *vb)
{
	(void)node;
	if (!agent || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_OCTET_STRING;
	vb->val.octet_str.len = agent->engine_id_len;
	memcpy(vb->val.octet_str.data, agent->engine_id, agent->engine_id_len);
	return 0;
}

static int usm_engine_boots_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				struct snmp_varbind *vb)
{
	(void)node;
	if (!agent || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = (int32_t)agent->engine_boots;
	return 0;
}

static int usm_engine_time_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			       struct snmp_varbind *vb)
{
	(void)node;
	if (!agent || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = (int32_t)snmp_usm_get_engine_time(agent);
	return 0;
}

static int usm_engine_max_size_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
				   struct snmp_varbind *vb)
{
	(void)node;
	if (!agent || !vb) {
		return -EINVAL;
	}
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = CONFIG_SNMP_MAX_PDU_SIZE;
	return 0;
}

static int usm_stats_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			 struct snmp_varbind *vb)
{
	if (!agent || !node || !vb || node->oid.len != 11) {
		return -EINVAL;
	}
	uint32_t subid = node->oid.ids[9];
	uint32_t val = 0;

	switch (subid) {
	case 1:
		val = agent->usm_stats.unsupported_sec_levels;
		break;
	case 2:
		val = agent->usm_stats.not_in_time_windows;
		break;
	case 3:
		val = agent->usm_stats.unknown_user_names;
		break;
	case 4:
		val = agent->usm_stats.unknown_engine_ids;
		break;
	case 5:
		val = agent->usm_stats.wrong_digests;
		break;
	case 6:
		val = agent->usm_stats.decryption_errors;
		break;
	default:
		return -EINVAL;
	}

	vb->type = SNMP_TAG_COUNTER32;
	vb->val.uint_val = val;
	return 0;
}

static int mpd_stats_get(struct snmp_agent *agent, const struct snmp_mib_node *node,
			 struct snmp_varbind *vb)
{
	if (!agent || !node || !vb || node->oid.len != 11) {
		return -EINVAL;
	}
	uint32_t subid = node->oid.ids[9];
	uint32_t val = 0;

	switch (subid) {
	case 1:
		val = agent->mpd_stats.unknown_security_models;
		break;
	case 2:
		val = agent->mpd_stats.invalid_msgs;
		break;
	case 3:
		val = agent->mpd_stats.unknown_pdu_handlers;
		break;
	default:
		return -EINVAL;
	}

	vb->type = SNMP_TAG_COUNTER32;
	vb->val.uint_val = val;
	return 0;
}

int snmp_usm_register_mibs(struct snmp_agent *inst)
{
	if (!inst) {
		return -EINVAL;
	}

	/* 1. RFC 3411 snmpEngine group: 1.3.6.1.6.3.10.2.1.<subid>.0 */
	static const uint32_t engine_base[] = {1, 3, 6, 1, 6, 3, 10, 2, 1};
	struct snmp_mib_node node;

	for (uint32_t subid = 1; subid <= 4; subid++) {
		memset(&node, 0, sizeof(node));
		node.oid.len = 11;
		memcpy(node.oid.ids, engine_base, sizeof(engine_base));
		node.oid.ids[9] = subid;
		node.oid.ids[10] = 0;
		node.set_cb = NULL;
		node.user_data = NULL;

		if (subid == 1) {
			node.type = ASN1_TAG_OCTET_STRING;
			node.get_cb = usm_engine_id_get;
		} else if (subid == 2) {
			node.type = ASN1_TAG_INTEGER;
			node.get_cb = usm_engine_boots_get;
		} else if (subid == 3) {
			node.type = ASN1_TAG_INTEGER;
			node.get_cb = usm_engine_time_get;
		} else {
			node.type = ASN1_TAG_INTEGER;
			node.get_cb = usm_engine_max_size_get;
		}

		int ret = snmp_mib_register(inst, &node);
		if (ret < 0) {

			return ret;
		}
	}

	/* 2. RFC 3414 usmStats group: 1.3.6.1.6.3.15.1.1.<subid>.0 */
	static const uint32_t usm_stats_base[] = {1, 3, 6, 1, 6, 3, 15, 1, 1};

	for (uint32_t subid = 1; subid <= 6; subid++) {
		memset(&node, 0, sizeof(node));
		node.oid.len = 11;
		memcpy(node.oid.ids, usm_stats_base, sizeof(usm_stats_base));
		node.oid.ids[9] = subid;
		node.oid.ids[10] = 0;
		node.type = SNMP_TAG_COUNTER32;
		node.get_cb = usm_stats_get;
		node.set_cb = NULL;
		node.user_data = NULL;

		int ret = snmp_mib_register(inst, &node);
		if (ret < 0) {

			return ret;
		}
	}

	/* 3. RFC 3412 snmpMPD group: 1.3.6.1.6.3.11.2.1.<subid>.0 */
	static const uint32_t mpd_stats_base[] = {1, 3, 6, 1, 6, 3, 11, 2, 1};

	for (uint32_t subid = 1; subid <= 3; subid++) {
		memset(&node, 0, sizeof(node));
		node.oid.len = 11;
		memcpy(node.oid.ids, mpd_stats_base, sizeof(mpd_stats_base));
		node.oid.ids[9] = subid;
		node.oid.ids[10] = 0;
		node.type = SNMP_TAG_COUNTER32;
		node.get_cb = mpd_stats_get;
		node.set_cb = NULL;
		node.user_data = NULL;

		int ret = snmp_mib_register(inst, &node);
		if (ret < 0) {

			return ret;
		}
	}

	return 0;
}

#endif /* CONFIG_SNMP_AGENT && CONFIG_SNMP_VERSION_3 */
