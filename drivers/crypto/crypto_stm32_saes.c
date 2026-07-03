/*
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
#include <zephyr/drivers/entropy/stm32_entropy.h>

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_stm32_saes);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_saes)

/* The STM32 Secure AES co-processor requires the STM32 RNG peripheral to be up,
 * running and available as pre-requisite for the SAES operations.
 */
#if !DT_NODE_EXISTS(DT_NODELABEL(rng)) || !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rng))
#error "SAES requires RNG peripheral, but no enabled RNG device found in the device tree."
#endif /* Pre-requisite */

#if !defined(CONFIG_ENTROPY_STM32_RNG_CLOCKON_API)
#error "SAES requires RNG clock-on support in the STM32 entropy driver, "\
	"but CONFIG_ENTROPY_STM32_RNG_CLOCKON_API is not enabled"
#endif /* Pre-requisite */

#define DT_DRV_COMPAT st_stm32_saes

#define STM32_CRYPTO_TYPEDEF AES_TypeDef

#define CRYP_SUPPORT (CAP_RAW_KEY | CAP_OPAQUE_KEY_HNDL | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | \
		      CAP_NO_IV_PREFIX)

#define CRYPTO_STM32_AES_MODES (CRYPTO_STM32_CAPS_AES_ECB | CRYPTO_STM32_CAPS_AES_CBC)

#define CRYPTO_SAES_MAX_SESSION CONFIG_CRYPTO_STM32_MAX_SESSION

struct crypto_stm32_session crypto_stm32_saes_sessions[CRYPTO_SAES_MAX_SESSION];

static int crypto_stm32_saes_get_unused_session_index(const struct device *dev)
{
	int i;

	struct crypto_stm32_data *data = CRYPTO_STM32_DATA(dev);

	k_sem_take(&data->session_sem, K_FOREVER);

	for (i = 0; i < CRYPTO_SAES_MAX_SESSION; i++) {
		if (!crypto_stm32_saes_sessions[i].in_use) {
			crypto_stm32_saes_sessions[i].in_use = true;
			k_sem_give(&data->session_sem);
			return i;
		}
	}

	k_sem_give(&data->session_sem);

	return -1;
}

static int crypto_stm32_saes_session_setup(const struct device *dev, struct cipher_ctx *ctx,
					   enum cipher_algo algo, enum cipher_mode mode,
					   enum cipher_op op_type)
{
	int ctx_idx, ret;
	struct crypto_stm32_session *session;

	ret = entropy_stm32_clockon_request();
	if (ret != 0) {
		LOG_ERR("Failed to enable RNG device");
		return ret;
	}

	ctx_idx = crypto_stm32_saes_get_unused_session_index(dev);
	if (ctx_idx < 0) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}
	session = &crypto_stm32_saes_sessions[ctx_idx];

	ret = crypto_stm32_session_setup(dev, ctx, algo, mode, op_type, session);

	/* Here, the session may be linked (so marked as in_use) or not to the
	 * cipher context depending on the status of crypto_stm32_session_setup.
	 */
	if (ret != 0) {
		/* Release the session, since not in use but in error */
		LOG_DBG("Session setup failed, release session");
		session->in_use = false;
	}

	return ret;
}

static int crypto_stm32_saes_session_free(const struct device *dev, struct cipher_ctx *ctx)
{
	int i, ret;
	uint32_t in_use_count = 0;

	/* Count the number of sessions currently in use, including the one hold
	 * by the cipher context.
	 */
	for (i = 0; i < CRYPTO_SAES_MAX_SESSION; i++) {
		if (crypto_stm32_saes_sessions[i].in_use) {
			in_use_count++;
		}
	}

	ret = crypto_stm32_session_free(dev, ctx, in_use_count);

	entropy_stm32_clockon_release();

	return ret;
}

static int crypto_stm32_saes_query_caps(const struct device *dev)
{
	return CRYP_SUPPORT;
}

static int crypto_stm32_saes_init(const struct device *dev)
{
	/* Ensure RNG is ready before SAES initialization,
	 * since SAES requires RNG as pre-requisite.
	 */
	int ret;

	LOG_DBG("SAES initialization started");

	ret = entropy_stm32_clockon_request();
	if (ret != 0) {
		LOG_ERR("Failed to enable RNG device");
		return ret;
	}

	ret = crypto_stm32_init(dev, CRYPTO_STM32_AES_MODES);

	entropy_stm32_clockon_release();

	LOG_DBG("SAES initialization completed");
	return ret;
}

static DEVICE_API(crypto, crypto_saes_enc_funcs) = {
	.cipher_begin_session = crypto_stm32_saes_session_setup,
	.cipher_free_session = crypto_stm32_saes_session_free,
	.cipher_async_callback_set = NULL,
	.query_hw_caps = crypto_stm32_saes_query_caps,
};

static struct crypto_stm32_data crypto_stm32_saes_dev_data = {
	.hcryp = {
		.Instance = (STM32_CRYPTO_TYPEDEF *)DT_INST_REG_ADDR(0),
	}
};

static const struct crypto_stm32_config crypto_stm32_saes_dev_config = {
	.reset = RESET_DT_SPEC_INST_GET(0),
	.pclken = STM32_DT_INST_CLOCK_INFO(0),
};

DEVICE_DT_INST_DEFINE(0, crypto_stm32_saes_init, NULL,
		      &crypto_stm32_saes_dev_data,
		      &crypto_stm32_saes_dev_config, POST_KERNEL,
		      CONFIG_CRYPTO_INIT_PRIORITY, (void *)&crypto_saes_enc_funcs);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_saes) */
