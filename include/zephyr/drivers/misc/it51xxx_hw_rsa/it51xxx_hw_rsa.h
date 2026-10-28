/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the it51xxx hw rsa driver
 * @ingroup it51xxx_hw_rsa_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_IT51XXX_HW_RSA_IT51XXX_HW_RSA_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_IT51XXX_HW_RSA_IT51XXX_HW_RSA_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for IT51XXX hardware RSA controller.
 * @defgroup it51xxx_hw_rsa_interface IT51XXX HW RSA
 * @ingroup misc_interfaces
 * @{
 */

/**
 * @brief Message buffers for RSA encryption and decryption.
 *
 * For encryption, @ref plain provides the input and @ref cipher receives
 * the output. For decryption, @ref cipher provides the input and
 * @ref plain receives the output.
 */
struct it51xxx_rsa_messages {
	/** Plaintext buffer. */
	uint8_t *plain;
	/** Size of the plaintext buffer, in bytes. */
	size_t plain_size;
	/** Ciphertext buffer. */
	uint8_t *cipher;
	/** Size of the ciphertext buffer, in bytes. */
	size_t cipher_size;
};

/**
 * @brief RSA key buffers and sizes.
 */
struct it51xxx_rsa_keys {
	/** RSA private key. */
	const uint8_t *private_key;
	/** Size of the private key, in bytes. */
	size_t private_key_sz;
	/** RSA public key. */
	const uint8_t *public_key;
	/** Size of the public key, in bytes. */
	size_t public_key_sz;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * IT51XXX hw rsa driver API definition
 *
 * (Internal use only.)
 */
__subsystem struct it51xxx_hw_rsa_driver_api {
	int (*configure)(const struct device *dev, const struct it51xxx_rsa_keys keys);
	int (*encrypt)(const struct device *dev, struct it51xxx_rsa_messages const *msgs);
	int (*decrypt)(const struct device *dev, const bool exponent_3,
		       struct it51xxx_rsa_messages const *msgs);
};
/**
 * @endcond
 */

/**
 * @brief Configure the RSA public and private keys.
 *
 * @param dev Pointer to the hardware RSA device.
 * @param keys RSA public and private keys.
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid key configuration.
 * @retval -ENOSYS Not implemented.
 */
static inline int it51xxx_hw_rsa_configure(const struct device *dev,
					   const struct it51xxx_rsa_keys keys)
{
	const struct it51xxx_hw_rsa_driver_api *api =
		(const struct it51xxx_hw_rsa_driver_api *)dev->api;

	if (api->configure == NULL) {
		return -ENOSYS;
	}

	return api->configure(dev, keys);
}

/**
 * @brief Encrypt the message.
 *
 * @param dev Pointer to the hardware RSA device.
 * @param msgs Message descriptor containing the input plaintext buffer,
 *             output ciphertext buffer, and their sizes.
 *
 * @retval 0 Encryption completed successfully.
 * @retval -EINVAL Invalid encryption parameters.
 * @retval -ENOSYS Not implemented.
 */
static inline int it51xxx_hw_rsa_encrypt(const struct device *dev,
					 struct it51xxx_rsa_messages const *msgs)
{
	const struct it51xxx_hw_rsa_driver_api *api =
		(const struct it51xxx_hw_rsa_driver_api *)dev->api;

	if (api->encrypt == NULL) {
		return -ENOSYS;
	}

	return api->encrypt(dev, msgs);
}

/**
 * @brief Decrypt the message.
 *
 * @param dev Pointer to the hardware RSA device.
 * @param exponent_3 Select RSA exponent 3 if true, or 65537 if false.
 * @param msgs Message descriptor containing the input ciphertext buffer,
 *             output plaintext buffer, and their sizes.
 *
 * @retval 0 Decryption completed successfully.
 * @retval -EINVAL Invalid decryption parameters.
 * @retval -ENOSYS Not implemented.
 */
static inline int it51xxx_hw_rsa_decrypt(const struct device *dev, const bool exponent_3,
					 struct it51xxx_rsa_messages const *msgs)
{
	const struct it51xxx_hw_rsa_driver_api *api =
		(const struct it51xxx_hw_rsa_driver_api *)dev->api;

	if (api->decrypt == NULL) {
		return -ENOSYS;
	}

	return api->decrypt(dev, exponent_3, msgs);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_IT51XXX_HW_RSA_IT51XXX_HW_RSA_H_ */
