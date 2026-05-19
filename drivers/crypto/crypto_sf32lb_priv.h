/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_SIFLI_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_SIFLI_PRIV_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/sf32lb.h>

#include <register.h>
#include <aes_acc.h>

/* Register offsets using AES_ACC_TypeDef from register.h */
#define AES_COMMAND_OFFSET      offsetof(AES_ACC_TypeDef, COMMAND)
#define AES_STATUS_OFFSET       offsetof(AES_ACC_TypeDef, STATUS)
#define AES_IRQ_OFFSET          offsetof(AES_ACC_TypeDef, IRQ)
#define AES_SETTING_OFFSET      offsetof(AES_ACC_TypeDef, SETTING)
#define AES_AES_SETTING_OFFSET  offsetof(AES_ACC_TypeDef, AES_SETTING)
#define AES_DMA_IN_OFFSET       offsetof(AES_ACC_TypeDef, DMA_IN)
#define AES_DMA_OUT_OFFSET      offsetof(AES_ACC_TypeDef, DMA_OUT)
#define AES_DMA_DATA_OFFSET     offsetof(AES_ACC_TypeDef, DMA_DATA)
#define AES_IV_W0_OFFSET        offsetof(AES_ACC_TypeDef, IV_W0)
#define AES_IV_W1_OFFSET        offsetof(AES_ACC_TypeDef, IV_W1)
#define AES_IV_W2_OFFSET        offsetof(AES_ACC_TypeDef, IV_W2)
#define AES_IV_W3_OFFSET        offsetof(AES_ACC_TypeDef, IV_W3)
#define AES_EXT_KEY_W0_OFFSET   offsetof(AES_ACC_TypeDef, EXT_KEY_W0)
#define AES_EXT_KEY_W1_OFFSET   offsetof(AES_ACC_TypeDef, EXT_KEY_W1)
#define AES_EXT_KEY_W2_OFFSET   offsetof(AES_ACC_TypeDef, EXT_KEY_W2)
#define AES_EXT_KEY_W3_OFFSET   offsetof(AES_ACC_TypeDef, EXT_KEY_W3)
#define AES_EXT_KEY_W4_OFFSET   offsetof(AES_ACC_TypeDef, EXT_KEY_W4)
#define AES_EXT_KEY_W5_OFFSET   offsetof(AES_ACC_TypeDef, EXT_KEY_W5)
#define AES_EXT_KEY_W6_OFFSET   offsetof(AES_ACC_TypeDef, EXT_KEY_W6)
#define AES_EXT_KEY_W7_OFFSET   offsetof(AES_ACC_TypeDef, EXT_KEY_W7)
#define AES_HASH_SETTING_OFFSET offsetof(AES_ACC_TypeDef, HASH_SETTING)
#define AES_HASH_DMA_IN_OFFSET  offsetof(AES_ACC_TypeDef, HASH_DMA_IN)
#define AES_HASH_DMA_DATA_OFFSET offsetof(AES_ACC_TypeDef, HASH_DMA_DATA)
#define AES_HASH_IV_H0_OFFSET   offsetof(AES_ACC_TypeDef, HASH_IV_H0)
#define AES_HASH_IV_H1_OFFSET   offsetof(AES_ACC_TypeDef, HASH_IV_H1)
#define AES_HASH_IV_H2_OFFSET   offsetof(AES_ACC_TypeDef, HASH_IV_H2)
#define AES_HASH_IV_H3_OFFSET   offsetof(AES_ACC_TypeDef, HASH_IV_H3)
#define AES_HASH_IV_H4_OFFSET   offsetof(AES_ACC_TypeDef, HASH_IV_H4)
#define AES_HASH_IV_H5_OFFSET   offsetof(AES_ACC_TypeDef, HASH_IV_H5)
#define AES_HASH_IV_H6_OFFSET   offsetof(AES_ACC_TypeDef, HASH_IV_H6)
#define AES_HASH_IV_H7_OFFSET   offsetof(AES_ACC_TypeDef, HASH_IV_H7)
#define AES_HASH_RESULT_H0_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_H0)
#define AES_HASH_RESULT_H1_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_H1)
#define AES_HASH_RESULT_H2_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_H2)
#define AES_HASH_RESULT_H3_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_H3)
#define AES_HASH_RESULT_H4_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_H4)
#define AES_HASH_RESULT_H5_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_H5)
#define AES_HASH_RESULT_H6_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_H6)
#define AES_HASH_RESULT_H7_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_H7)
#define AES_HASH_LEN_L_OFFSET   offsetof(AES_ACC_TypeDef, HASH_LEN_L)
#define AES_HASH_LEN_H_OFFSET   offsetof(AES_ACC_TypeDef, HASH_LEN_H)
#define AES_HASH_RESULT_LEN_L_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_LEN_L)
#define AES_HASH_RESULT_LEN_H_OFFSET offsetof(AES_ACC_TypeDef, HASH_RESULT_LEN_H)

/* AES Key Length definitions */
#define SIFLI_AES_KEY_LEN_128   0
#define SIFLI_AES_KEY_LEN_192   1
#define SIFLI_AES_KEY_LEN_256   2

/* AES Mode definitions */
#define SIFLI_AES_MODE_ECB      0
#define SIFLI_AES_MODE_CTR      1
#define SIFLI_AES_MODE_CBC      2

/* AES Operation Mode */
#define SIFLI_AES_DEC           0
#define SIFLI_AES_ENC           1

/* HASH algorithm definitions */
#define SIFLI_HASH_ALGO_SHA1    0
#define SIFLI_HASH_ALGO_SHA224  1
#define SIFLI_HASH_ALGO_SHA256  2
#define SIFLI_HASH_ALGO_SM3     3

/* HASH result sizes */
#define SIFLI_HASH_SHA1_SIZE    20
#define SIFLI_HASH_SHA224_SIZE  28  /* SHA-224 digest size in bytes */
#define SIFLI_HASH_SHA256_SIZE  32
#define SIFLI_HASH_SM3_SIZE     32

/* Maximum supported key length is 256 bits */
#define SIFLI_AES_MAX_KEY_LEN   (256 / 8)

/* Block size for AES */
#define SIFLI_AES_BLOCK_SIZE    16

/* Block size for hash input (64 bytes) */
#define SIFLI_HASH_BLOCK_SIZE   64

/* Poll timeout in microseconds (10 ms) */
#define CRYPTO_SIFLI_TIMEOUT_US 10000U

/* Driver config structure */
struct crypto_sifli_config {
	uintptr_t base;
	struct sf32lb_clock_dt_spec clock;
#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	void (*irq_config_func)(void);
#endif
};

/* Driver data structure */
struct crypto_sifli_data {
	struct k_sem device_sem;
	struct k_sem session_sem;
#if defined(CONFIG_CRYPTO_SIFLI_ASYNC)
	struct k_sem sync_sem;  /* Semaphore for sync fallback */
#if defined(CONFIG_CRYPTO_SIFLI_AES)
	cipher_completion_cb cipher_cb;
	struct cipher_pkt *cipher_pkt;
	int cipher_status;
#endif
#if defined(CONFIG_CRYPTO_SIFLI_HASH)
	hash_completion_cb hash_cb;
	struct hash_pkt *hash_pkt;
	uint8_t hash_algo;  /* Store algo for result copy in ISR */
	int hash_status;
#endif
#endif /* CONFIG_CRYPTO_SIFLI_ASYNC */
};

/* Session structure for cipher operations */
struct crypto_sifli_session {
	uint32_t key[SIFLI_AES_MAX_KEY_LEN / sizeof(uint32_t)];
	uint8_t key_len;
	uint8_t mode;
	bool in_use;
};

/* Session structure for hash operations */
struct crypto_sifli_hash_session {
	uint8_t algo;
	bool in_use;
};

#define CRYPTO_SIFLI_CFG(dev) \
	((const struct crypto_sifli_config *const)(dev)->config)

#define CRYPTO_SIFLI_DATA(dev) \
	((struct crypto_sifli_data *const)(dev)->data)

#define CRYPTO_SIFLI_SESSN(ctx) \
	((struct crypto_sifli_session *const)(ctx)->drv_sessn_state)

#define CRYPTO_SIFLI_HASH_SESSN(ctx) \
	((struct crypto_sifli_hash_session *const)(ctx)->drv_sessn_state)

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_SIFLI_PRIV_H_ */
