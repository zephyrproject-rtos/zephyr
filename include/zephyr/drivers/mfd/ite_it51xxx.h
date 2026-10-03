/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for IT51XXX MFD driver
 * @ingroup mfd_interface_ite_it51xxx
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_ITE_IT51XXX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_ITE_IT51XXX_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

/**
 * @defgroup mfd_interface_ite_it51xxx MFD ITE IT51XXX interface
 * @ingroup mfd_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Size of the shared hardware crypto DLM region in bytes. */
#define IT51XXX_HWCRYPTO_DLM_SIZE KB(4)

/** SHA256 hash length in bytes. */
#define SHA_SHA256_HASH_LEN        32
/** SHA256 block length in bytes. */
#define SHA_SHA256_BLOCK_LEN       64
/** SHA256 hash length in 32-bit words. */
#define SHA_SHA256_HASH_LEN_WORDS  (SHA_SHA256_HASH_LEN / sizeof(uint32_t))
/** SHA256 block length in 32-bit words. */
#define SHA_SHA256_BLOCK_LEN_WORDS (SHA_SHA256_BLOCK_LEN / sizeof(uint32_t))

/**
 * @brief Maximum input length in bytes per SHA hardware operation.
 *
 * Larger messages are processed in chunks of up to 1024 bytes.
 * For example, a 10 KiB message requires 10 hardware operations.
 */
#define SHA_HW_MAX_INPUT_LEN       1024
/** Maximum input length in 32-bit words per SHA hardware operation. */
#define SHA_HW_MAX_INPUT_LEN_WORDS (SHA_HW_MAX_INPUT_LEN / sizeof(uint32_t))

/** Minimum RSA operand length in bytes. */
#define IT51XXX_HWCRYPTO_RSA_MIN_BYTE_LEN 64
/** Maximum RSA operand length in bytes. */
#define IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN 512

/**
 * @brief SHA DLM data.
 *
 * Must reside within the first 4k-byte of RAM and be aligned to a
 * 256-byte boundary.
 */
struct it51xxx_sha_dlm {
	/** SHA input buffer accessible as 32-bit words or bytes. */
	union {
		/** SHA input buffer accessed as 32-bit words. */
		uint32_t w_sha[SHA_HW_MAX_INPUT_LEN_WORDS];
		/** SHA input buffer accessed as bytes. */
		uint8_t w_input[SHA_HW_MAX_INPUT_LEN];
	};
	/** SHA hash state words H[0] through H[7]. */
	uint32_t h[SHA_SHA256_HASH_LEN_WORDS];
	/** SHA initialization state. */
	uint32_t sha_init;
	/** Current index into the input buffer. */
	uint32_t w_input_index;
	/** Total input message length in bytes. */
	uint32_t total_len;
} __aligned(256);

/**
 * @brief RSA DLM layout.
 *
 * Field offsets correspond to the hardware-defined layout within
 * the 4 KiB DLM region.
 */
struct it51xxx_rsa_dlm {
	/** Public key data at offsets 0x000–0x1FF. */
	uint8_t key_public[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	/** Reserved region at offsets 0x200–0x3FF. */
	uint8_t reserved_1[512];
	/** Message data at offsets 0x400–0x5FF. */
	uint8_t messages[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	/** Reserved region at offsets 0x600–0x9FF. */
	uint8_t reserved_2[1024];
	/** Private key data at offsets 0xA00–0xBFF. */
	uint8_t key_private[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	/** Reserved region at offsets 0xC00–0xFFF. */
	uint8_t reserved_3[1024];
};

/**
 * @brief Shared DLM storage for it51xxx hardware crypto algorithms.
 *
 * SHA, RSA, and other crypto algorithms share the same 4 KiB DLM
 * memory region. Union members use the same underlying storage,
 * so access must be serialized.
 */
union hwcrypto_dlm_block {
	/** SHA DLM data. */
	struct it51xxx_sha_dlm sha;
	/** RSA DLM data. */
	struct it51xxx_rsa_dlm rsa;
	/** Raw access to the entire shared DLM region. */
	uint8_t raw_data[IT51XXX_HWCRYPTO_DLM_SIZE];
};

/**
 * @brief Shared DLM instance used by it51xxx hardware crypto algorithms.
 */
extern union hwcrypto_dlm_block hwcrypto_dlm;

/**
 * @brief Get the IT51XXX MFD register base address.
 *
 * The base address is shared by the hardware crypto (AES/SHA/RSA..etc) child
 * functions.
 *
 * @param[in] dev Pointer to the parent MFD device.
 *
 * @return Base address of the MFD register block.
 */
mm_reg_t mfd_ite_it51xxx_get_base(const struct device *dev);

/**
 * @brief Lock access to the shared IT51XXX MFD hardware.
 *
 * RSA, SHA, and other child drivers must acquire this lock before accessing
 * shared MFD resources.
 *
 * @param[in] dev     Pointer to the parent MFD device.
 * @param[in] timeout Lock acquisition timeout.
 *
 * @retval 0 Lock acquired successfully.
 * @retval -EAGAIN The timeout expired before the lock was acquired.
 */
int mfd_ite_it51xxx_lock(const struct device *dev, k_timeout_t timeout);

/**
 * @brief Unlock access to the shared IT51XXX MFD hardware.
 *
 * @param[in] dev Pointer to the parent MFD device.
 */
void mfd_ite_it51xxx_unlock(const struct device *dev);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_ITE_IT51XXX_H_ */
