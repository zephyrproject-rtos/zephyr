/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_PUFS_H_
#define ZEPHYR_DRIVERS_CRYPTO_PUFS_H_

#include "pufcc.h"
#include <zephyr/crypto/crypto.h>

#define BUFFER_SIZE 512

extern uint8_t __pufcc_descriptors[BUFFER_SIZE];

enum pufs_session_type {
  PUFS_SESSION_SIGN_VERIFICATION = 0,
  PUFS_SESSION_HASH_CALCULATION,
  PUFS_SESSION_DECRYPTION,
  PUFS_SESSION_UNDEFINED
};

/* Generic Callback Signature for different Sessions */
struct crypto_callbacks {
  	cipher_completion_cb cipher_cb;
    hash_completion_cb hash_cb;
    sign_completion_cb sign_cb;
};

/* Cipher, Hash and Sign session contexts */
struct pufs_crypto_ctx {
  struct hash_ctx *hash_ctx;
  struct cipher_ctx *cipher_ctx;
  struct sign_ctx *sign_ctx;
};

/* Cipher, Hash and Sign pkts */
struct pufs_crypto_pkt {
  struct hash_pkt *hash_pkt;
  struct cipher_pkt *cipher_pkt;
  struct sign_pkt *sign_pkt;
};

/* Device constant configuration parameters */
struct pufs_data {
	enum pufs_session_type pufs_session_type;
  struct crypto_callbacks pufs_session_callback;
  struct pufs_crypto_ctx pufs_ctx;
  struct pufs_crypto_pkt pufs_pkt;
};

/* Device constant configuration parameters */
struct pufs_config {
	void (*irq_init)(void);
	uint32_t base;
  uint32_t irq_num;
};

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_PRIV_H_ */
