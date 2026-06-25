/*
 * Copyright (c) 2026 Siratul Islam <siratul.islam@linux.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FIDO2_CLIENTPIN_H_
#define FIDO2_CLIENTPIN_H_

#include <zephyr/authentication/fido2/fido2_types.h>

/** PIN Protocol 1 auth param size */
#define FIDO2_PIN_AUTH_SIZE_P1       16
/** PIN Protocol 2 auth param size */
#define FIDO2_PIN_AUTH_SIZE_P2       32
/** Maximum PIN auth param size */
#define FIDO2_PIN_AUTH_MAX_SIZE      32
/** Padded PIN size */
#define FIDO2_PIN_PADDED_SIZE        64
/** Encrypted PIN token size */
#define FIDO2_PIN_TOKEN_ENC_MAX_SIZE 48

#define FIDO2_CLIENTPIN_GET_PIN_RETRIES           0x01 /**< getPINRetries */
#define FIDO2_CLIENTPIN_GET_KEY_AGREEMENT         0x02 /**< getKeyAgreement */
#define FIDO2_CLIENTPIN_SET_PIN                   0x03 /**< setPIN */
#define FIDO2_CLIENTPIN_CHANGE_PIN                0x04 /**< changePIN */
#define FIDO2_CLIENTPIN_GET_PIN_TOKEN             0x05 /**< getPinToken */
#define FIDO2_CLIENTPIN_GET_PIN_TOKEN_UV_W_PERMS  0x06 /**< UvWithPermissions */
#define FIDO2_CLIENTPIN_GET_UV_RETRIES            0x07 /**< getUVRetries */
#define FIDO2_CLIENTPIN_GET_PIN_TOKEN_PIN_W_PERMS 0x09 /**< PinWithPermissions */

#define FIDO2_PIN_PERM_MC   BIT(0) /**< MakeCredential */
#define FIDO2_PIN_PERM_GA   BIT(1) /**< GetAssertion */
#define FIDO2_PIN_PERM_CM   BIT(2) /**< Credential Management */
#define FIDO2_PIN_PERM_BE   BIT(3) /**< Bio Enrollment */
#define FIDO2_PIN_PERM_LBW  BIT(4) /**< Large Blob Write */
#define FIDO2_PIN_PERM_ACFG BIT(5) /**< Authenticator Configuration */

/** ClientPIN protocols */
enum fido2_clientpin_protocols {
	FIDO2_PIN_PROTOCOL_V1 = 1,
	FIDO2_PIN_PROTOCOL_V2,
};

/**
 * Generates the initial ECDH key agreement key pair.
 *
 * @return 0 on success, negative errno on failure
 */
int fido2_clientpin_init(void);

/**
 * Check if PIN is set.
 *
 * @return true if PIN is set, false if not.
 */
bool fido2_clientpin_pin_is_set(void);

/**
 * Verify a pinUvAuthParam.
 *
 * @param msg Message that was authenticated.
 * @param msg_len Length of @p msg in bytes.
 * @param auth_param pinUvAuthParam received from the platform.
 * @param auth_param_len Length of @p auth_param in bytes.
 * @param permissions Permissions that the token must have.
 * @param rp_id Relying party ID.
 * @return 0 on success, negative errno on failure.
 */
int fido2_clientpin_pin_verify_pin_uv_auth_token(const uint8_t *msg, size_t msg_len,
						 const uint8_t *auth_param, size_t auth_param_len,
						 uint32_t permissions, const char *rp_id);

/**
 * Handle getPINRetries.
 *
 * @param cbor_out Output buffer for the CBOR-encoded response.
 * @param cbor_out_cap Size of @p cbor_out in bytes.
 * @param cbor_out_len Set to the number of bytes written to @p cbor_out.
 * @return FIDO2_OK on success, FIDO2_ERR_OTHER on failure.
 */
enum fido2_status fido2_clientpin_cmd_get_retries(uint8_t *cbor_out, size_t cbor_out_cap,
						  size_t *cbor_out_len);

/**
 * Handle KeyAgreement.
 *
 * @param cbor_out Output buffer for the CBOR-encoded response.
 * @param cbor_out_cap Size of @p cbor_out in bytes.
 * @param cbor_out_len Set to the number of bytes written to @p cbor_out.
 * @return FIDO2_OK on success, FIDO2_ERR_OTHER on failure.
 */
enum fido2_status fido2_clientpin_cmd_get_key_agreement(uint8_t *cbor_out, size_t cbor_out_cap,
							size_t *cbor_out_len);

/**
 * Handle setPIN.
 *
 * @param protocol PIN/UV auth protocol version.
 * @param platform_key Platform's ECDH public key.
 * @param platform_key_len Length of @p platform_key in bytes.
 * @param new_pin_enc Encrypted new pin.
 * @param new_pin_enc_len Length of @p new_pin_enc in bytes.
 * @param pin_uv_auth_param Result of calling authenticate(shared secret, newPinEnc).
 * @return FIDO2_OK on success, FIDO2_ERR_* on failure.
 */
enum fido2_status fido2_clientpin_cmd_set_pin(uint8_t protocol, const uint8_t *platform_key,
					      size_t platform_key_len, const uint8_t *new_pin_enc,
					      size_t new_pin_enc_len,
					      const uint8_t *pin_uv_auth_param);

/**
 * Handle getPinToken.
 *
 * @param protocol PIN/UV auth protocol version.
 * @param platform_key Platform's ECDH public key.
 * @param platform_key_len Length of @p platform_key in bytes.
 * @param pin_hash_enc Encrypted PIN hash.
 * @param pin_hash_enc_len Length of @p pin_hash_enc in bytes.
 * @param cbor_out Output buffer for the CBOR-encoded response.
 * @param cbor_out_cap Size of @p cbor_out in bytes.
 * @param cbor_out_len Set to the number of bytes written to @p cbor_out.
 * @return FIDO2_OK on success, FIDO2_ERR_* on failure.
 */
enum fido2_status fido2_clientpin_cmd_get_pin_token(uint8_t protocol, const uint8_t *platform_key,
						    size_t platform_key_len,
						    const uint8_t *pin_hash_enc,
						    size_t pin_hash_enc_len, uint8_t *cbor_out,
						    size_t cbor_out_cap, size_t *cbor_out_len);

#endif /* FIDO2_CLIENTPIN_H_ */
