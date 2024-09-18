/*
 * Copyright (c) 2018 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DATA_JWT_H_
#define ZEPHYR_INCLUDE_DATA_JWT_H_

#include <zephyr/types.h>
#include <stdbool.h>

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
};

/**
 * @brief Initialize the JWT builder.
 *
 * Initialize the given JWT builder for the creation of a fresh token.
 * The buffer size should be long enough to store the entire token.
 *
 * @param builder The builder to initialize.
 * @param buffer The buffer to write the token to.
 * @param buffer_size The size of this buffer.  The token will be NULL
 * terminated, which needs to be allowed for in this size.
 *
 * @retval 0 Success.
 * @retval -ENOSPC Buffer is insufficient to initialize.
 */
int jwt_init_builder(struct jwt_builder *builder,
		     char *buffer,
		     size_t buffer_size);

/**
 * @brief Add JWT payload.
 *
 * Add JWT payload to a previously initialized builder with the following fields:
 * - Expiration Time
 * - Issued At
 * - Audience
 *
 * See RFC 7519 section 4.1 to get more information about these fields.
 *
 * @param builder A previously initialized builder.
 * @param exp Expiration Time (epoch format).
 * @param iat Issued At (epoch format).
 * @param aud Audience.
 *
 * @retval 0 Success.
 * @retval <0 Failure.
 */
int jwt_add_payload(struct jwt_builder *builder,
		    int32_t exp,
		    int32_t iat,
		    const char *aud);

/**
 * @brief Sign the JWT.
 *
 * Sign a previously initialized with payload JWT.
 *
 * @param builder A previously initialized builder with payload.
 * @param der_key Private key to use in DER format.
 * @param der_key_len Size of the private key in bytes.
 *
 * @retval 0 Success.
 * @retval <0 Failure.
 */
int jwt_sign(struct jwt_builder *builder,
	     const char *der_key,
	     size_t der_key_len);


static inline size_t jwt_payload_len(struct jwt_builder *builder)
{
	return (builder->buf - builder->base);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DATA_JWT_H_ */
