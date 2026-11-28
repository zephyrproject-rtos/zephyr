/*
 * Copyright (c) 2020 Markus Fuchs <markus.fuchs@de.sauter-bc.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_PRIV_H_

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_cryp)
#define DT_DRV_COMPAT st_stm32_cryp
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_aes)
#define DT_DRV_COMPAT st_stm32_aes
#else
#error No STM32 HW Crypto Accelerator in device tree
#endif

/*
 * HAL2 has its own IT API (HAL_AES_{En,De}crypt_IT() on hal_aes_handle_t, with
 * HAL_AES_OutCpltCallback()/HAL_AES_ErrorCallback()), incompatible with the
 * HAL_CRYP_{En,De}crypt_IT()/HAL_CRYP_*Callback() on CRYP_HandleTypeDef used below.
 * Not implemented here yet, so keep HAL2 on the polling do_aes() path.
 */
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes) && !defined(CONFIG_STM32_HAL2) &&                   \
	DT_INST_IRQ_HAS_IDX(0, 0)
#define STM32_CRYP_USE_ISR
#endif

#ifdef CONFIG_STM32_HAL2
typedef struct hal2_aes_config_data crypt_config_t;
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32l4_aes)
#define crypt_config_t CRYP_InitTypeDef
#else
#define crypt_config_t CRYP_ConfigTypeDef
#endif /* CONFIG_STM32_HAL2 */

#ifdef CONFIG_STM32_HAL2
typedef hal_aes_handle_t	hal_crypt_handle_t;
#else
typedef CRYP_HandleTypeDef	hal_crypt_handle_t;
#endif /* CONFIG_STM32_HAL2 */

/* Maximum supported key length is 256 bits */
#define CRYPTO_STM32_AES_MAX_KEY_LEN (256 / 8)

struct crypto_stm32_config {
	const struct reset_dt_spec reset;
	struct stm32_pclken pclken;
};

struct crypto_stm32_data {
	hal_crypt_handle_t hcryp;
	struct k_sem device_sem;
	struct k_sem session_sem;
#if defined(STM32_CRYP_USE_ISR)
	struct {
		struct k_sem complete_sem;
		int error;
	} isr_session;
#endif
};

#ifdef CONFIG_STM32_HAL2
/* HAL2 does not define a structure for the AES operatiopn configure, hence this local one */
struct hal2_aes_config_data {
	enum cipher_mode mode;
	uint32_t *iv;
	size_t iv_length;
};
#endif /* CONFIG_STM32_HAL2 */

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

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_PRIV_H_ */
