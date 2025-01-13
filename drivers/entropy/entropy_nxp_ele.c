/* entropy_nxp_ele.c - NXP ELE entropy driver */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ele_trng

#include <zephyr/logging/log.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include "sss_crypto.h"

struct entropy_ele_data_str {
	struct k_sem sem_lock;
};

static struct entropy_ele_data_str entropy_ele_data;

static int entropy_ele_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	sss_sscp_rng_t ctx;
	int result = -EIO;

	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(&entropy_ele_data == dev->data);

	k_sem_take(&entropy_ele_data.sem_lock, K_FOREVER);

	if (CRYPTO_InitHardware() != kStatus_Success) {
		goto exit;
	}

	if (sss_sscp_rng_context_init(&g_sssSession, &ctx, 0u) != kStatus_SSS_Success) {
		goto exit;
	}

	if (sss_sscp_rng_get_random(&ctx, buf, len) != kStatus_SSS_Success) {
		goto exit;
	}

	if (sss_sscp_rng_free(&ctx) != kStatus_SSS_Success) {
		goto exit;
	}

	result = 0;

exit:
	k_sem_give(&entropy_ele_data.sem_lock);
	return result;
}

static int entropy_ele_init(const struct device *dev)
{
	__ASSERT_NO_MSG(&entropy_ele_data == dev->data);

	k_sem_init(&entropy_ele_data.sem_lock, 1, 1);

	return 0;
}

static DEVICE_API(entropy, entropy_ele_api_funcs) = {.get_entropy = entropy_ele_get_entropy};

DEVICE_DT_INST_DEFINE(0, entropy_ele_init, NULL, &entropy_ele_data, NULL, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_ele_api_funcs);
