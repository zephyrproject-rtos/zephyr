/*
 * Copyright (c) 2025 Bayrem Gharsellaoui
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_HASH_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_HASH_PRIV_H_

/* Max digest length: SHA256 = 32 bytes */
#define STM32_HASH_MAX_DIGEST_SIZE (32)

#if defined(CONFIG_SOC_SERIES_STM32H7RSX)

#define hash_config_t			HASH_ConfigTypeDef
#define HASH_DATATYPE_8B		HASH_BYTE_SWAP
#define STM32_HASH_SHA224_START		HAL_HASH_Start
#define STM32_HASH_SHA256_START		HAL_HASH_Start

#else /* CONFIG_SOC_SERIES_STM32H7RSX */

#define hash_config_t			HASH_InitTypeDef
#define STM32_HASH_SHA224_START		HAL_HASHEx_SHA224_Start
#define STM32_HASH_SHA256_START		HAL_HASHEx_SHA256_Start

#endif /* CONFIG_SOC_SERIES_STM32H7RSX */

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

static inline HAL_StatusTypeDef hal_func_hash_SHA224_start(HASH_HandleTypeDef *hhash,
					void *p_in_buffer,
					uint32_t in_size_byte,
					void *p_out_buffer)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	hhash->Init.Algorithm = HASH_ALGOSELECTION_SHA224;
	if (HAL_HASH_SetConfig(hhash, &hhash->Init) != HAL_OK) {
		return HAL_ERROR;
	}
#endif /* CONFIG_SOC_SERIES_STM32H7RSX */
	return STM32_HASH_SHA224_START(hhash, p_in_buffer, in_size_byte, p_out_buffer,
			HAL_MAX_DELAY);
}

static inline HAL_StatusTypeDef hal_func_hash_SHA256_start(HASH_HandleTypeDef *hhash,
					void *p_in_buffer,
					uint32_t in_size_byte,
					void *p_out_buffer)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	hhash->Init.Algorithm = HASH_ALGOSELECTION_SHA256;

	if (HAL_HASH_SetConfig(hhash, &hhash->Init) != HAL_OK) {
		return HAL_ERROR;
	}
#endif /* CONFIG_SOC_SERIES_STM32H7RSX */
	return STM32_HASH_SHA256_START(hhash, p_in_buffer, in_size_byte, p_out_buffer,
			HAL_MAX_DELAY);
}

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_HASH_PRIV_H_ */
