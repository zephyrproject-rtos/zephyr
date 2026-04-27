/*
 * Copyright (c) 2020 Markus Fuchs <markus.fuchs@de.sauter-bc.com>
 * Copyright (c) 2026 Vossloh AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>

#include "crypto_stm32_priv.h"

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_stm32_aes);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_cryp)
#define DT_DRV_COMPAT st_stm32_cryp
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_aes)
#define DT_DRV_COMPAT st_stm32_aes
#endif

#if defined(DT_DRV_COMPAT) /* Build only if the compatible is present in the device tree. */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_cryp)
#define STM32_CRYPTO_TYPEDEF            CRYP_TypeDef
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_aes)
#define STM32_CRYPTO_TYPEDEF            AES_TypeDef
#endif

#define CRYP_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | \
		      CAP_NO_IV_PREFIX)

#define CRYPTO_STM32_AES_MODES                                                                     \
	(CRYPTO_STM32_CAPS_AES_ECB | CRYPTO_STM32_CAPS_AES_CBC | CRYPTO_STM32_CAPS_AES_CTR)

#define CRYPTO_AES_MAX_SESSION CONFIG_CRYPTO_STM32_MAX_SESSION

struct crypto_stm32_session crypto_stm32_aes_sessions[CRYPTO_AES_MAX_SESSION];

static int crypto_stm32_aes_get_unused_session_index(const struct device *dev)
{
	int i;

	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);

	k_sem_take(&data->session_sem, K_FOREVER);

	for (i = 0; i < CRYPTO_AES_MAX_SESSION; i++) {
		if (!crypto_stm32_aes_sessions[i].in_use) {
			crypto_stm32_aes_sessions[i].in_use = true;
			k_sem_give(&data->session_sem);
			return i;
		}
	}

	k_sem_give(&data->session_sem);

	return -1;
}

static int crypto_stm32_aes_session_setup(const struct device *dev, struct cipher_ctx *ctx,
					  enum cipher_algo algo, enum cipher_mode mode,
					  enum cipher_op op_type)
{
	int ctx_idx, ret;
	struct crypto_stm32_session *session;

	ctx_idx = crypto_stm32_aes_get_unused_session_index(dev);
	if (ctx_idx < 0) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}
	session = &crypto_stm32_aes_sessions[ctx_idx];

	ret = crypto_stm32_session_setup(dev, ctx, algo, mode, op_type, session);

	/* Here, the session may be linked (so marked as in_use) or not to the
	 * cipher context depending on the status of crypto_stm32_session_setup.
	 */

	return ret;
}

static int crypto_stm32_aes_session_free(const struct device *dev, struct cipher_ctx *ctx)
{
	int i;
	uint32_t in_use_count = 0;

	/* Count the number of sessions currently in use, including the one hold
	 * by the cipher context.
	 */
	for (i = 0; i < CRYPTO_AES_MAX_SESSION; i++) {
		if (crypto_stm32_aes_sessions[i].in_use) {
			in_use_count++;
		}
	}

	return crypto_stm32_session_free(dev, ctx, in_use_count);
}

static int crypto_stm32_aes_query_caps(const struct device *dev)
{
	return CRYP_SUPPORT;
}

static int crypto_stm32_aes_init(const struct device *dev)
{
	return crypto_stm32_init(dev, CRYPTO_STM32_AES_MODES);
}

static DEVICE_API(crypto, crypto_aes_enc_funcs) = {
	.cipher_begin_session = crypto_stm32_aes_session_setup,
	.cipher_free_session = crypto_stm32_aes_session_free,
	.cipher_async_callback_set = NULL,
	.query_hw_caps = crypto_stm32_aes_query_caps,
};

static struct crypto_stm32_data crypto_stm32_aes_dev_data = {
	.hcryp = {
		.Instance = (STM32_CRYPTO_TYPEDEF *)DT_INST_REG_ADDR(0),
	}
};

static const struct crypto_stm32_config crypto_stm32_aes_dev_config = {
	.reset = RESET_DT_SPEC_INST_GET(0),
	.pclken = STM32_DT_INST_CLOCK_INFO(0),
};

DEVICE_DT_INST_DEFINE(0, crypto_stm32_aes_init, NULL,
		      &crypto_stm32_aes_dev_data,
		      &crypto_stm32_aes_dev_config, POST_KERNEL,
		      CONFIG_CRYPTO_INIT_PRIORITY, (void *)&crypto_aes_enc_funcs);

#endif /* DT_DRV_COMPAT */
