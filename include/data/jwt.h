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
 * @brief JSON Web Token (JWT)
 * @defgroup jwt JSON Web Token (JWT)
 * @ingroup structured_data
 * @{
 */

/**
 * @brief JWT data tracking.
 *
 * JSON Web Tokens contain several sections, each encoded in base-64.
 * This structure tracks the token as it is being built, including
 * limits on the amount of available space.  It should be initialized
 * with jwt_init().
 */
struct jwt_builder {
	/** The base of the buffer we are writing to. */
	char *base;

	/** The place in this buffer where we are currently writing.
	 */
	char *buf;

	/** The length remaining to write. */
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
 * The buffer size should at least be as long as JWT_BUILDER_MAX_SIZE
 * returns.
 *
 * @param builder The builder to initialize.
 * @param buffer The buffer to write the token to.
 * @param buffer_size The size of this buffer.  The token will be NULL
 * terminated, which needs to be allowed for in this size.
 *
 * @retval 0 Success
 * @retval -ENOSPC Buffer is insufficient to initialize
 */
int jwt_init_builder(struct jwt_builder *builder,
		     char *buffer,
		     size_t buffer_size);

/**
 * @brief add JWT primary payload.
 */
int jwt_add_payload(struct jwt_builder *builder,
		    s32_t exp,
		    s32_t iat,
		    const char *aud);

/**
 * @brief Sign the JWT token.
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
