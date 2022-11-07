/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 802.15.4 6LoWPAN authentication and encryption
 *
 * This is not to be included by the application.
 */

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY

#include <zephyr/net/ieee802154.h>

int ieee802154_security_setup_session(struct ieee802154_security_ctx *sec_ctx, uint8_t level,
				      uint8_t key_mode, uint8_t *key, uint8_t key_len);

/**
 * @brief Decrypt an authenticated payload.
 *
 * @param sec_ctx Pointer to an IEEE 802.15.4 security context.
 * @param frame Pointer to the frame data in original (little endian) byte order.
 * @param hdr_len Length of the MHR.
 * @param payload_len Length of the MAC payload.
 * @param tag_size Length of the authentication tag.
 * @param src_ext_addr Pointer to the extended source address of the frame (in little endian byte
 *                     order).
 * @param frame_counter Frame counter in CPU byte order.
 */
bool ieee802154_decrypt_auth(struct ieee802154_security_ctx *sec_ctx, uint8_t *frame,
			     uint8_t hdr_len, uint8_t payload_len, uint8_t tag_size,
			     uint8_t *src_ext_addr, uint32_t frame_counter);

/**
 * @brief Encrypt an authenticated payload.
 *
 * @param sec_ctx Pointer to an IEEE 802.15.4 security context.
 * @param frame Pointer to the frame data in original (little endian) byte order.
 * @param hdr_len Length of the MHR.
 * @param payload_len Length of the MAC payload.
 * @param tag_size Length of the authentication tag.
 * @param src_ext_addr Pointer to the extended source address of the frame (in little endian byte
 *                     order).
 */
bool ieee802154_encrypt_auth(struct ieee802154_security_ctx *sec_ctx, uint8_t *frame,
			     uint8_t hdr_len, uint8_t payload_len,
			     uint8_t tag_size, uint8_t *src_ext_addr);

int ieee802154_security_init(struct ieee802154_security_ctx *sec_ctx);

#else

#define ieee802154_decrypt_auth(...)  true
#define ieee802154_encrypt_auth(...)  true
#define ieee802154_security_init(...) 0

#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */
