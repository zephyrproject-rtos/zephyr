/*
 * Copyright (c) 2025 Bayrem Gharsellaoui
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_HASH_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_HASH_PRIV_H_

#define hash_config_t HASH_InitTypeDef

/* Max digest length: SHA256 = 32 bytes */
#define STM32_HASH_MAX_DIGEST_SIZE (32)

struct crypto_stm32_hash_config {
	const struct reset_dt_spec reset;
	struct stm32_pclken pclken;
};

struct crypto_stm32_hash_data {
	HASH_HandleTypeDef hhash;
	struct k_sem device_sem;
	struct k_sem session_sem;
};

struct crypto_stm32_hash_session {
	hash_config_t config;
	uint8_t digest[STM32_HASH_MAX_DIGEST_SIZE];
	bool in_use;
	enum hash_algo algo;
};

#define CRYPTO_STM32_HASH_CFG(dev) ((const struct crypto_stm32_hash_config *const)(dev)->config)

#define CRYPTO_STM32_HASH_DATA(dev) ((struct crypto_stm32_hash_data *const)(dev)->data)

#define CRYPTO_STM32_HASH_SESSN(ctx)                                                               \
	((struct crypto_stm32_hash_session *const)(ctx)->drv_sessn_state)

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_HASH_PRIV_H_ */
