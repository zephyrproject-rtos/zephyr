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

#define hash_config_t	HASH_ConfigTypeDef
#define HASH_DATATYPE_8B	HASH_BYTE_SWAP

#else /* CONFIG_SOC_SERIES_STM32H7RSX */

#define hash_config_t	HASH_InitTypeDef

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
	return HAL_HASH_Start(hhash,
			(const uint8_t *const)p_in_buffer,
#else
	return HAL_HASHEx_SHA224_Start(hhash,
			p_in_buffer,
#endif /* CONFIG_SOC_SERIES_STM32H7RSX */
			in_size_byte,
			p_out_buffer,
			HAL_MAX_DELAY);
}

static inline HAL_StatusTypeDef hal_func_hash_SHA256_start(HASH_HandleTypeDef *hhash,
					void *p_in_buffer,
					uint32_t in_size_byte,
					void *p_out_buffer)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	return HAL_HASH_Start(hhash,
			(const uint8_t *const)p_in_buffer,
#else
	return HAL_HASHEx_SHA256_Start(hhash,
			p_in_buffer,
#endif /* CONFIG_SOC_SERIES_STM32H7RSX */
			in_size_byte,
			p_out_buffer,
			HAL_MAX_DELAY);
}


#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_HASH_PRIV_H_ */
