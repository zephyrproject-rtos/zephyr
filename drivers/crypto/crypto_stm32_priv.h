/*
 * Copyright (c) 2020 Markus Fuchs <markus.fuchs@de.sauter-bc.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_PRIV_H_

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
#define crypt_config_t CRYP_InitTypeDef
#else
#define crypt_config_t CRYP_ConfigTypeDef
#endif

/* Maximum supported key length is 256 bits */
#define CRYPTO_STM32_AES_MAX_KEY_LEN (256 / 8)

/* STM32 crypto engines' capabilities. */
#define CRYPTO_STM32_CAPS_AES_ECB BIT(CRYPTO_CIPHER_MODE_ECB /* 1 */)
#define CRYPTO_STM32_CAPS_AES_CBC BIT(CRYPTO_CIPHER_MODE_CBC /* 2 */)
#define CRYPTO_STM32_CAPS_AES_CTR BIT(CRYPTO_CIPHER_MODE_CTR /* 3 */)
#define CRYPTO_STM32_CAPS_AES_CCM BIT(CRYPTO_CIPHER_MODE_CCM /* 4 */)
#define CRYPTO_STM32_CAPS_AES_GCM BIT(CRYPTO_CIPHER_MODE_GCM /* 5 */)

#define CRYPTO_STM32_CAPS_AES_MODES_MSK                                                            \
	(CRYPTO_STM32_CAPS_AES_ECB | CRYPTO_STM32_CAPS_AES_CBC | CRYPTO_STM32_CAPS_AES_CTR |       \
	 CRYPTO_STM32_CAPS_AES_CCM | CRYPTO_STM32_CAPS_AES_GCM)

struct crypto_stm32_config {
	const struct reset_dt_spec reset;
	struct stm32_pclken pclken;
};

struct crypto_stm32_data {
	CRYP_HandleTypeDef hcryp;
	struct k_sem device_sem;
	struct k_sem session_sem;
	uint32_t caps;
};

struct crypto_stm32_session {
	crypt_config_t config;
	uint32_t key[CRYPTO_STM32_AES_MAX_KEY_LEN / sizeof(uint32_t)];
	bool in_use;
};

#define CRYPTO_STM32_CFG(dev) \
	((const struct crypto_stm32_config *const)(dev)->config)

#define CRYPTO_STM32_DATA(dev) \
	((struct crypto_stm32_data *const)(dev)->data)

#define CRYPTO_STM32_SESSN(ctx) \
	((struct crypto_stm32_session *const)(ctx)->drv_sessn_state)

/* STM32 crypto driver internal functions shared between different engines (AES, SAES)
 * These APIs are not meant to be exposed outside of the STM32 crypto engines drivers.
 */

int crypto_stm32_session_setup(const struct device *dev, struct cipher_ctx *ctx,
			       enum cipher_algo algo, enum cipher_mode mode, enum cipher_op op_type,
			       struct crypto_stm32_session *session);

int crypto_stm32_session_free(const struct device *dev, struct cipher_ctx *ctx,
			      uint32_t nb_sessions_in_use);

int crypto_stm32_init(const struct device *dev, uint32_t crypto_stm32_caps);

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_PRIV_H_ */
