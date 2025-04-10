/*
 * Copyright (c) 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_UUID_H_
#define ZEPHYR_INCLUDE_SYS_UUID_H_

/**
 * @file
 *
 * @brief Utility functions for the generation and parsing of Universal Unique Identifier.
 * @details This driver is compliant with RFC9562: https://datatracker.ietf.org/doc/rfc9562/
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup uuid UUID
 * @since 4.0
 * @version 0.1.0
 * @ingroup utilities
 * @{
 */

/** @brief Number of bytes in the binary representation of a UUID. */
#define UUID_SIZE 16U

/** @brief Length of the UUID canonical string representation, including the NULL terminator. */
#define UUID_STR_LEN 37U

/** @brief Length of the UUID base64 string representation, including the NULL terminator. */
#define UUID_BASE64_LEN 25U

/**
 * @brief Length of the UUID base64 URL and filename safe string representation, including the
 * NULL terminator.
 */
#define UUID_BASE64URL_LEN 23U

/** @brief Binary representation of a UUID. */
struct uuid {
	/** @cond INTERNAL_HIDDEN */
	uint8_t val[UUID_SIZE];
	/** @endcond */
};

/**
 * @brief Generate a UUIDv4.
 *
 * @param out The UUID where the result will be written.
 *
 * @retval 0 The UUID has been correctly generated and stored in @p out
 * @retval -EINVAL @p out is not acceptable
 */
int uuid_generate_v4(struct uuid *out);

/**
 * @brief Generate a UUIDv5.
 *
 * @details This function computes a deterministic UUID starting from a namespace UUID and binary
 * data.
 *
 * @param namespace A pointer to an UUID to be used as namespace.
 * @param data A pointer to the data that will be hashed to produce the UUID.
 * @param data_size The size of the data buffer.
 * @param out The UUID where the result will be written.
 *
 * @retval 0 The UUID has been correctly generated and stored in @p out
 * @retval -EINVAL @p out is not acceptable
 * @retval -ENOMEM Memory allocation failed
 * @retval -ENOTSUP mbedTLS returned an unrecognized error
 */
int uuid_generate_v5(const struct uuid *namespace, const void *data, size_t data_size,
		     struct uuid *out);

/**
 * @brief Copy an UUID into another UUID.
 *
 * @param data Input data to copy.
 * @param out Destination for the copy.
 *
 * @retval 0 The UUID has been correctly copied in @p dst
 * @retval -EINVAL @p dst is not acceptable
 */
int uuid_copy(const struct uuid *data, struct uuid *out);

/**
 * @brief Create a uuid_t from a binary (big-endian) formatted UUID.
 *
 * @param data The buffer where the binary UUID is stored in a big-endian order.
 * @param out The UUID where the result will be written.
 *
 * @retval 0 The UUID has been correctly parsed and stored in @p out
 * @retval -EINVAL @p data or @p out are not acceptable
 */
int uuid_from_buffer(const uint8_t data[UUID_SIZE], struct uuid *out);

/**
 * @brief Parse a UUID from its canonical (RFC9562) string representation.
 *
 * @param data A pointer to the string to be parsed.
 * @param out The UUID where the result will be written.
 *
 * @retval 0 The UUID has been correctly parsed and stored in @p out
 * @retval -EINVAL @p input or @p out are not acceptable
 */
int uuid_from_string(const char data[UUID_STR_LEN], struct uuid *out);

/**
 * @brief Create a uuid_t from a binary (big-endian) formatted UUID.
 *
 * @param data The input UUID to store in the buffer.
 * @param out The buffer where the binary UUID is stored in a big-endian order.
 *
 * @retval 0 The UUID has been correctly parsed and stored in @p buff
 * @retval -EINVAL @p buff is not acceptable
 */
int uuid_to_buffer(const struct uuid *data, uint8_t out[UUID_SIZE]);

/**
 * @brief Convert a UUID to its canonical (RFC9562) string representation.
 *
 * @param data The UUID to convert to string.
 * @param out A pointer to a previously allocated buffer where the result will be written.
 *
 * @retval 0 The UUID has been converted and written in @p out
 * @retval -EINVAL @p out is not acceptable
 */
int uuid_to_string(const struct uuid *data, char out[UUID_STR_LEN]);

/**
 * @brief Convert a UUID to its base 64 (RFC 3548, RFC 4648) string representation.
 *
 * @param data The UUID to convert to string.
 * @param out A pointer to a previously allocated buffer where the result will be written.
 *
 * @retval 0 The UUID has been converted and written in @p out
 * @retval -EINVAL @p out is not acceptable
 */
int uuid_to_base64(const struct uuid *data, char out[UUID_BASE64_LEN]);

/**
 * @brief Convert a UUID to its base 64 (RFC 4648 sec. 5) URL and filename safe string
 * representation.
 *
 * @param data The UUID to convert to string.
 * @param out A pointer to a previously allocated buffer where the result will be written.
 *
 * @retval 0 The UUID has been converted and written in @p out
 * @retval -EINVAL @p out is not acceptable
 */
int uuid_to_base64url(const struct uuid *data, char out[UUID_BASE64URL_LEN]);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_UUID_H_ */
