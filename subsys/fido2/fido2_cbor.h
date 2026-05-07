/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FIDO2_CBOR_H_
#define FIDO2_CBOR_H_

#include <zephyr/fido2/fido2_types.h>

/** Maximum number of algorithms in pubKeyCredParams */
#define FIDO2_MAX_ALGORITHMS 8

/** PIN Protocol 1 auth param size (HMAC-SHA-256 truncated to 16 bytes) */
#define FIDO2_CBOR_PIN_AUTH_SIZE_P1 16

/** PIN Protocol 2 auth param size (full HMAC-SHA-256, 32 bytes) */
#define FIDO2_CBOR_PIN_AUTH_SIZE_P2 32

/** Maximum PIN auth param size across all supported protocols */
#define FIDO2_CBOR_PIN_AUTH_MAX_SIZE 32

/** Maximum encrypted PIN size */
#define FIDO2_CBOR_PIN_ENC_MAX_SIZE 64

/** Encrypted PIN hash size */
#define FIDO2_CBOR_PIN_HASH_ENC_SIZE 16

/** Number of attestation statement format identifiers */
#define FIDO2_CBOR_MAX_ATTESTATION_FORMATS 4

/** Maximum attestation statement format identifier length */
#define FIDO2_CBOR_ATTESTATION_FMT_MAX_LEN 32

/**
 * Decoded authenticatorMakeCredential request parameters.
 */
struct fido2_make_credential_params {
	/** 0x01: clientDataHash */
	uint8_t client_data_hash[FIDO2_SHA256_SIZE];
	/** 0x02: rp.id */
	char rp_id[FIDO2_RP_ID_MAX_LEN];
	/** 0x02: rp.name */
	char rp_name[FIDO2_RP_NAME_MAX_LEN];
	/** 0x03: user.id */
	uint8_t user_id[FIDO2_USER_ID_MAX_SIZE];
	/** user.id length */
	uint16_t user_id_len;
	/** 0x03: user.name */
	char user_name[FIDO2_USER_NAME_MAX_LEN];
	/** 0x03: user.displayName */
	char user_display_name[FIDO2_USER_DISPLAY_NAME_MAX_LEN];
	/** 0x04: pubKeyCredParams algorithms */
	int32_t algorithms[FIDO2_MAX_ALGORITHMS];
	/** Number of algorithms parsed */
	uint8_t num_algorithms;
	/** 0x05: excludeList credential IDs */
	uint8_t exclude_ids[CONFIG_FIDO2_MAX_CREDENTIALS][FIDO2_CREDENTIAL_ID_MAX_SIZE];
	/** excludeList credential ID lengths */
	size_t exclude_id_lens[CONFIG_FIDO2_MAX_CREDENTIALS];
	/** Number of exclude list entries */
	uint8_t num_exclude;
	/** 0x06: extensions.hmac-secret */
	bool ext_hmac_secret;
	/** 0x06: extensions.credProtect (0 = absent, 1/2/3 = credProtect level) */
	uint8_t ext_cred_protect_level;
	/** 0x07: options.rk (resident key / discoverable) */
	bool resident_key;
	/** 0x07: options.up (user presence requested) */
	bool up;
	/** Whether options.up was present */
	bool has_up_option;
	/** 0x07: options.uv (user verification requested) */
	bool uv;
	/** Whether options.uv was present */
	bool has_uv_option;
	/** 0x08: pinUvAuthParam */
	uint8_t pin_uv_auth_param[FIDO2_CBOR_PIN_AUTH_MAX_SIZE];
	/** Length of pinUvAuthParam (0 = probe, 16 = P1, 32 = P2) */
	size_t pin_uv_auth_param_len;
	/** Whether pinUvAuthParam key was present */
	bool has_pin_uv_auth_param;
	/** 0x09: pinUvAuthProtocol version */
	uint8_t pin_uv_auth_protocol;
	/** Whether pinUvAuthProtocol key was present */
	bool has_pin_uv_auth_protocol;
	/** 0x0A: enterpriseAttestation */
	uint8_t enterprise_attestation;
	/** Whether enterpriseAttestation was present */
	bool has_enterprise_attestation;
	/** 0x0B: attestationFormatsPreference */
	char attestation_formats[FIDO2_CBOR_MAX_ATTESTATION_FORMATS]
				[FIDO2_CBOR_ATTESTATION_FMT_MAX_LEN + 1];
	/** Number of attestation format preferences */
	uint8_t num_attestation_formats;
};

/**
 * Encode authenticatorGetInfo response.
 *
 * @param info         Device info to encode
 * @param cbor_out     Output buffer
 * @param cbor_out_cap Capacity of @p cbor_out in bytes
 * @param cbor_out_len Number of bytes written to @p cbor_out
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_get_info(const struct fido2_device_info *info, uint8_t *cbor_out,
			       size_t cbor_out_cap, size_t *cbor_out_len);

/**
 * Decode authenticatorMakeCredential request.
 *
 * @param cbor_in CBOR-encoded request
 * @param cbor_in_len Length of request data
 * @param params Output: decoded parameters
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_decode_make_credential(const uint8_t *cbor_in, size_t cbor_in_len,
				      struct fido2_make_credential_params *params);

#endif /* FIDO2_CBOR_H_ */
