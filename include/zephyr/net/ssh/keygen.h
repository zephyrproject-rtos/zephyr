/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SSH_KEYGEN_H_
#define ZEPHYR_INCLUDE_NET_SSH_KEYGEN_H_

/**
 * @file keygen.h
 *
 * @brief SSH keygen API
 *
 * @defgroup ssh_keygen SSH keygen API
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Types of host keys that can be generated */
enum ssh_host_key_type {
	/** RSA host key type */
	SSH_HOST_KEY_TYPE_RSA,
};

/** @brief Formats that SSH host keys can be exported/imported in */
enum ssh_host_key_format {
	/** DER format */
	SSH_HOST_KEY_FORMAT_DER,
	/** PEM format */
	SSH_HOST_KEY_FORMAT_PEM,
};

/**
 * @brief Generate a new SSH host key
 *
 * @param key_index Index of the key to generate. Must be between 0 and
 *        CONFIG_SSH_MAX_HOST_KEYS - 1
 * @param key_type Type of key to generate
 * @param key_size_bits Size of the key to generate, in bits. For RSA keys,
 *        this must be at least 2048
 *
 * @return 0 on success, negative error code on failure
 */
int ssh_keygen(int key_index,
	       enum ssh_host_key_type key_type,
	       size_t key_size_bits);

/**
 * @brief Export a generated SSH host key
 *
 * @param key_index Index of the key to export. Must be between 0 and
 *        CONFIG_SSH_MAX_HOST_KEYS - 1
 * @param private_key Whether to export the private key (true) or public
 *        key (false)
 * @param fmt Format to export the key in
 * @param buf Buffer to write the exported key to
 * @param buf_len Length of the buffer, in bytes
 *
 * @return On success, the number of bytes written to the buffer. On failure,
 *         a negative error code
 */
int ssh_keygen_export(int key_index,
		      bool private_key,
		      enum ssh_host_key_format fmt,
		      void *buf,
		      size_t buf_len);

/**
 * @brief Import an SSH host key
 *
 * @param key_index Index of the key to import. Must be between 0 and
 *        CONFIG_SSH_MAX_HOST_KEYS - 1
 * @param private_key Whether the key being imported is a private key (true)
 *        or a public key (false)
 * @param fmt Format the key is in
 * @param buf Buffer containing the key to import
 * @param buf_len Length of the buffer, in bytes
 *
 * @return 0 on success, negative error code on failure
 */
int ssh_keygen_import(int key_index,
		      bool private_key,
		      enum ssh_host_key_format fmt,
		      const void *buf,
		      size_t buf_len);

/**
 * @brief Free a generated SSH host key
 *
 * @param key_index Index of the key to free. Must be between 0 and
 *        CONFIG_SSH_MAX_HOST_KEYS - 1
 *
 * @return 0 on success, negative error code on failure
 */
int ssh_keygen_free(int key_index);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SSH_KEYGEN_H_ */
