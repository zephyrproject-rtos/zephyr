/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MFD driver for the Infineon MXCRYPTO block.
 *
 * The MXCRYPTO block is a single hardware crypto engine that provides both a
 * True Random Number Generator (TRNG) and AES / SHA acceleration.  Both
 * functions share one register base address.  This MFD parent owns that base
 * address and a mutex that serialises access between the two child function
 * drivers (entropy TRNG and crypto AES/SHA).
 */

#define DT_DRV_COMPAT infineon_mxcrypto

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mfd/infineon_mxcrypto.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_infineon_mxcrypto, CONFIG_MFD_LOG_LEVEL);

#include "cy_crypto_core_hw.h"

struct mfd_infineon_mxcrypto_config {
	CRYPTO_Type *base;
};

struct mfd_infineon_mxcrypto_data {
	struct k_mutex lock;
};

CRYPTO_Type *mfd_infineon_mxcrypto_get_base(const struct device *dev)
{
	const struct mfd_infineon_mxcrypto_config *cfg = dev->config;

	return cfg->base;
}

int mfd_infineon_mxcrypto_lock(const struct device *dev, k_timeout_t timeout)
{
	struct mfd_infineon_mxcrypto_data *data = dev->data;

	return k_mutex_lock(&data->lock, timeout);
}

void mfd_infineon_mxcrypto_unlock(const struct device *dev)
{
	struct mfd_infineon_mxcrypto_data *data = dev->data;

	k_mutex_unlock(&data->lock);
}

static int mfd_infineon_mxcrypto_init(const struct device *dev)
{
	const struct mfd_infineon_mxcrypto_config *cfg = dev->config;
	struct mfd_infineon_mxcrypto_data *data = dev->data;
	cy_en_crypto_status_t rc;

	k_mutex_init(&data->lock);

	rc = Cy_Crypto_Core_Enable(cfg->base);
	if (rc != CY_CRYPTO_SUCCESS) {
		LOG_ERR("MXCRYPTO enable failed: %d", (int)rc);
		return -EIO;
	}

	return 0;
}

#define MFD_INFINEON_MXCRYPTO_DEFINE(n)                                                            \
	static struct mfd_infineon_mxcrypto_data mfd_infineon_mxcrypto_data_##n;                   \
	static const struct mfd_infineon_mxcrypto_config mfd_infineon_mxcrypto_config_##n = {      \
		.base = (CRYPTO_Type *)DT_INST_REG_ADDR(n),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mfd_infineon_mxcrypto_init, NULL,                                 \
			      &mfd_infineon_mxcrypto_data_##n,                                     \
			      &mfd_infineon_mxcrypto_config_##n, PRE_KERNEL_1,                     \
			      CONFIG_MFD_INFINEON_MXCRYPTO_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_INFINEON_MXCRYPTO_DEFINE)
