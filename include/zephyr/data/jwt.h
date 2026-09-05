/*
 * Copyright (c) 2018 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DATA_JWT_H_
#define ZEPHYR_INCLUDE_DATA_JWT_H_

#include <zephyr/data/json.h>

#include <stdbool.h>
#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JSON Web Token (JWT) - RFC 7519
 * @defgroup jwt JSON Web Token (JWT)
 * @ingroup json
 * @{
 */

/**
 * @brief JWT data tracking.
 *
 * JSON Web Tokens contain several sections, each encoded in Base64URL.
 * This structure tracks the token as it is being built, including
 * limits on the amount of available space.  It should be initialized
 * with jwt_init_builder().
 */
struct jwt_builder {
	/** The base of the buffer we are writing to. */
	char *base;

	/** The place in this buffer where we are currently writing.
	 */
	char *buf;

	/** The remaining free space in @p buf. */
	size_t len;

	/**
	 * Flag that is set if we try to write past the end of the
	 * buffer.  If set, the token is not valid.
	 */
	bool overflowed;

	/* Pending bytes yet to be converted to base64. */
	unsigned char wip[3];

	/* Number of pending bytes. */
	int pending;

	/* The key the token is signed with. */
	psa_key_id_t key_id;

	/* The signature algorithm announced in the header. */
	psa_algorithm_t alg;

	/* The signature size of the key, in bytes. */
	size_t sig_size;
};

/**
 * @brief Initialize the JWT builder.
 *
 * Initialize the given JWT builder for the creation of a fresh token.
 * The buffer size should be long enough to store the entire token.
 *
 * The header is generated from @p key_id and @p alg, so that the "alg"
 * value it announces always describes the signature that jwt_sign()
 * produces.  The combination has to map onto one of the JWS algorithms
 * of RFC 7518 section 3.1:
 *
 * - "RS256", "RS384" and "RS512" for RSASSA-PKCS1-v1_5 keys.
 * - "PS256", "PS384" and "PS512" for RSASSA-PSS keys.
 * - "ES256", "ES384" and "ES512" for ECDSA keys on P-256, P-384 and
 *   P-521 respectively.  RFC 7518 ties each of these to a single curve
 *   and hash, so the key size and the hash have to agree.
 *
 * @param builder The builder to initialize.
 * @param buffer The buffer to write the token to.
 * @param buffer_size The size of this buffer.  The token will be NULL
 * terminated, which needs to be allowed for in this size.
 * @param key_id The key that will sign the token.
 * @param alg The PSA signature algorithm to sign the token with.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP The key and algorithm have no JWS "alg" equivalent.
 * @retval -ENOMEM Buffer is insufficient to initialize.
 * @retval -EINVAL The key is not usable.
 */
int jwt_init_builder(struct jwt_builder *builder, char *buffer, size_t buffer_size,
		     psa_key_id_t key_id, psa_algorithm_t alg);

/**
 * @brief Add JWT payload.
 *
 * Add the payload (the claims) to a previously initialized builder.  The
 * claims are described by a JSON object descriptor, so any set of claims
 * can be used.  See RFC 7519 section 4 for the registered claim names.
 *
 * @param builder A previously initialized builder.
 * @param payload Your payload struct.
 * @param payload_json The JSON object descriptor.
 * @param payload_json_len The length of the descriptor.
 *
 * @retval 0 Success.
 * @retval <0 Failure.
 */
int jwt_add_payload(struct jwt_builder *builder, const void *payload,
		    const struct json_obj_descr *payload_json, size_t payload_json_len);

/**
 * @brief Sign the JWT.
 *
 * Sign a previously initialized builder that has a payload, using the key
 * and the algorithm that were given to jwt_init_builder().
 *
 * @param builder A previously initialized builder with payload.
 *
 * @retval 0 Success.
 * @retval -ENOMEM The buffer is too small to hold the signature.
 * @retval -EINVAL The signature could not be computed.
 */
int jwt_sign(struct jwt_builder *builder);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DATA_JWT_H_ */
