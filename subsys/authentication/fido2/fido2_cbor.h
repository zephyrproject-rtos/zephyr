/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FIDO2_CBOR_H_
#define FIDO2_CBOR_H_

#include <zephyr/authentication/fido2/fido2_types.h>
#include <zephyr/authentication/fido2/fido2_attestation.h>

/** Maximum number of algorithms in pubKeyCredParams */
#define FIDO2_MAX_ALGORITHMS 8

/** Maximum PIN auth param size across all supported protocols */
#define FIDO2_CBOR_PIN_AUTH_MAX_SIZE 32

/** Maximum encrypted PIN size */
#define FIDO2_CBOR_PIN_ENC_MAX_SIZE 80

/** Encrypted PIN hash size */
#define FIDO2_CBOR_PIN_HASH_ENC_SIZE 32

/** Number of attestation statement format identifiers */
#define FIDO2_CBOR_MAX_ATTESTATION_FORMATS 4

/** Maximum attestation statement format identifier length */
#define FIDO2_CBOR_ATTESTATION_FMT_MAX_LEN 32

/** CTAP2 canonical CBOR-encoded COSE ES256 public key size */
#define FIDO2_COSE_ES256_KEY_SIZE 77

/** ES256 is the only supported algorithm for now */
#define FIDO2_COSE_KEY_MAX_SIZE FIDO2_COSE_ES256_KEY_SIZE

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
 * Decoded authenticatorGetAssertion request parameters.
 */
struct fido2_get_assertion_params {
	/** 0x01: rpId */
	char rp_id[FIDO2_RP_ID_MAX_LEN];
	/** 0x02: clientDataHash (32 bytes) */
	uint8_t client_data_hash[FIDO2_SHA256_SIZE];
	/** 0x03: allowList credential IDs */
	uint8_t allow_ids[CONFIG_FIDO2_MAX_CREDENTIALS][FIDO2_CREDENTIAL_ID_MAX_SIZE];
	/** allowList credential ID lengths */
	size_t allow_id_lens[CONFIG_FIDO2_MAX_CREDENTIALS];
	/** Number of allowList entries */
	uint8_t num_allow;
	/** 0x05: options.up (defaults to true when absent) */
	bool up;
	/** Whether options.up was present */
	bool has_up_option;
	/** 0x05: options.uv (user verification requested) */
	bool uv;
	/** Whether options.uv was present */
	bool has_uv_option;
	/** 0x06: pinUvAuthParam */
	uint8_t pin_uv_auth_param[FIDO2_CBOR_PIN_AUTH_MAX_SIZE];
	/** Length of pinUvAuthParam (0 = probe, 16 = P1, 32 = P2) */
	size_t pin_uv_auth_param_len;
	/** Whether pinUvAuthParam key was present */
	bool has_pin_uv_auth_param;
	/** 0x07: pinUvAuthProtocol version */
	uint8_t pin_uv_auth_protocol;
	/** Whether pinUvAuthProtocol key was present */
	bool has_pin_uv_auth_protocol;
	/** 0x08: enterpriseAttestation */
	uint8_t enterprise_attestation;
	/** Whether enterpriseAttestation was present */
	bool has_enterprise_attestation;
	/** 0x09: attestationFormatsPreference */
	char attestation_formats[FIDO2_CBOR_MAX_ATTESTATION_FORMATS]
				[FIDO2_CBOR_ATTESTATION_FMT_MAX_LEN + 1];
	/** Number of attestation format preferences */
	uint8_t num_attestation_formats;
};

/**
 * Decoded authenticatorClientPin request parameters.
 */
struct fido2_client_pin_params {
	/** 0x01: pinUvAuthProtocol */
	uint8_t pin_uv_auth_protocol;
	/** Whether pinUvAuthProtocol was present */
	bool has_pin_uv_auth_protocol;
	/** 0x02: subCommand */
	uint8_t sub_command;
	/** 0x03: keyAgreement */
	uint8_t key_agreement[FIDO2_P256_UNCOMPRESSED_KEY_SIZE];
	/** Length of keyAgreement */
	size_t key_agreement_len;
	/** Whether keyAgreement was present */
	bool has_key_agreement;
	/** 0x04: pinUvAuthParam */
	uint8_t pin_uv_auth_param[FIDO2_CBOR_PIN_AUTH_MAX_SIZE];
	/** Length of pinUvAuthParam */
	size_t pin_uv_auth_param_len;
	/** Whether pinUvAuthParam was present */
	bool has_pin_uv_auth_param;
	/** 0x05: newPinEnc */
	uint8_t new_pin_enc[FIDO2_CBOR_PIN_ENC_MAX_SIZE];
	/** newPinEnc len */
	size_t new_pin_enc_len;
	/** Whether newPinEnc was present */
	bool has_new_pin_enc;
	/** 0x06: pinHashEnc */
	uint8_t pin_hash_enc[FIDO2_CBOR_PIN_HASH_ENC_SIZE];
	/** pinHashEnc len */
	size_t pin_hash_enc_len;
	/** Whether pinHashEnc was present */
	bool has_pin_hash_enc;
	/** 0x09: permissions */
	uint32_t permissions;
	/** Whether permissions was present */
	bool has_permissions;
	/** 0x0A: rpId */
	char rp_id[FIDO2_RP_ID_MAX_LEN];
	/** Whether rpId was present */
	bool has_rp_id;
};

/**
 * Encode a COSE_Key for ES256 (P-256) public key.
 *
 * @param pub_key Uncompressed public key
 * @param pub_key_len Public key length
 * @param cbor_out     Output buffer
 * @param cbor_out_cap Capacity of @p cbor_out in bytes
 * @param cbor_out_len Number of bytes written to @p cbor_out
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_cose_key(const uint8_t *pub_key, size_t pub_key_len, uint8_t *cbor_out,
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

/**
 * Encode authenticatorMakeCredential response.
 *
 * @param auth_data Raw authenticatorData bytes
 * @param auth_data_len Length of authenticatorData
 * @param att Attestation result from the attestation callback
 * @param cbor_out Output buffer
 * @param cbor_out_cap Capacity of @p cbor_out in bytes
 * @param cbor_out_len Number of bytes written to @p cbor_out
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_make_credential_resp(const uint8_t *auth_data, size_t auth_data_len,
					   const struct fido2_attestation_result *att,
					   uint8_t *cbor_out, size_t cbor_out_cap,
					   size_t *cbor_out_len);

/**
 * Decode authenticatorGetAssertion request.
 *
 * @param cbor_in CBOR-encoded request
 * @param cbor_in_len Length of request data
 * @param params Output: decoded parameters
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_decode_get_assertion(const uint8_t *cbor_in, size_t cbor_in_len,
				    struct fido2_get_assertion_params *params);

/**
 * Encode authenticatorGetAssertion response.
 *
 * @param cred_id Credential identifier
 * @param cred_id_len Credential identifier length
 * @param auth_data Raw authenticatorData bytes
 * @param auth_data_len Length of authenticatorData
 * @param sig Signature bytes
 * @param sig_len Signature length
 * @param user_id User handle (may be NULL if not discoverable)
 * @param user_id_len User handle length
 * @param num_credentials Total number of matching credentials (0 to omit)
 * @param cbor_out Output buffer
 * @param cbor_out_cap Output buffer size
 * @param cbor_out_len Encoded length written
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_get_assertion_resp(const uint8_t *cred_id, size_t cred_id_len,
					 const uint8_t *auth_data, size_t auth_data_len,
					 const uint8_t *sig, size_t sig_len, const uint8_t *user_id,
					 size_t user_id_len, uint16_t num_credentials,
					 uint8_t *cbor_out, size_t cbor_out_cap,
					 size_t *cbor_out_len);

/**
 * Encode authenticatorGetInfo response.
 *
 * @param info Device info to encode
 * @param cbor_out Output buffer
 * @param cbor_out_cap Capacity of @p cbor_out in bytes
 * @param cbor_out_len Number of bytes written to @p cbor_out
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_get_info(const struct fido2_device_info *info, uint8_t *cbor_out,
			       size_t cbor_out_cap, size_t *cbor_out_len);

/**
 * Decode authenticatorClientPIN request.
 *
 * @param cbor_in CBOR-encoded request
 * @param cbor_in_len Length of request data
 * @param params Output: decoded parameters
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_decode_client_pin(const uint8_t *cbor_in, size_t cbor_in_len,
				 struct fido2_client_pin_params *params);

/**
 * Encode clientPIN getPINRetries response.
 *
 * @param retries Remaining PIN retry count
 * @param power_cycle_s Requires a power cycle
 * @param cbor_out Output buffer
 * @param cbor_out_cap Buffer size
 * @param cbor_out_len Encoded length written
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_client_pin_retries(uint8_t retries, bool power_cycle_s, uint8_t *cbor_out,
					 size_t cbor_out_cap, size_t *cbor_out_len);

/**
 * Encode clientPIN getKeyAgreement response.
 *
 * @param pub_key Uncompressed P-256 public key (65 bytes)
 * @param pub_key_len Public key length
 * @param cbor_out Output buffer
 * @param cbor_out_cap Buffer size
 * @param cbor_out_len Encoded length written
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_client_pin_key_agreement(const uint8_t *pub_key, size_t pub_key_len,
					       uint8_t *cbor_out, size_t cbor_out_cap,
					       size_t *cbor_out_len);

/**
 * Encode clientPIN getPINToken response.
 *
 * @param token_enc Encrypted pinUvAuthToken
 * @param token_enc_len Length of encrypted token
 * @param cbor_out Output buffer
 * @param cbor_out_cap Buffer size
 * @param cbor_out_len Encoded length written
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_client_pin_token(const uint8_t *token_enc, size_t token_enc_len,
				       uint8_t *cbor_out, size_t cbor_out_cap,
				       size_t *cbor_out_len);

#endif /* FIDO2_CBOR_H_ */
