/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MFD driver for the Infineon MXCRYPTOLITE block.
 *
 * The MXCRYPTOLITE block is a single hardware crypto engine that provides both
 * a True Random Number Generator (TRNG) and AES / SHA-256 acceleration.  Both
 * functions share one register base address.  This MFD parent owns that base
 * address and a mutex that serialises access between the two child function
 * drivers (entropy TRNG and crypto AES/SHA).
 */

#define DT_DRV_COMPAT infineon_mxcryptolite

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mfd/infineon_mxcryptolite.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_infineon_mxcryptolite, CONFIG_MFD_LOG_LEVEL);

struct mfd_infineon_mxcryptolite_config {
	CRYPTOLITE_Type *base;
};

struct mfd_infineon_mxcryptolite_data {
	struct k_mutex lock;
};

CRYPTOLITE_Type *mfd_infineon_mxcryptolite_get_base(const struct device *dev)
{
	const struct mfd_infineon_mxcryptolite_config *cfg = dev->config;

	return cfg->base;
}

int mfd_infineon_mxcryptolite_lock(const struct device *dev, k_timeout_t timeout)
{
	struct mfd_infineon_mxcryptolite_data *data = dev->data;

	return k_mutex_lock(&data->lock, timeout);
}

void mfd_infineon_mxcryptolite_unlock(const struct device *dev)
{
	struct mfd_infineon_mxcryptolite_data *data = dev->data;

	k_mutex_unlock(&data->lock);
}

static int mfd_infineon_mxcryptolite_init(const struct device *dev)
{
	struct mfd_infineon_mxcryptolite_data *data = dev->data;

	k_mutex_init(&data->lock);

	return 0;
}

#define MFD_INFINEON_MXCRYPTOLITE_DEFINE(n)                                                        \
	static struct mfd_infineon_mxcryptolite_data mfd_infineon_mxcryptolite_data_##n;           \
	static const struct mfd_infineon_mxcryptolite_config                                       \
		mfd_infineon_mxcryptolite_config_##n = {                                           \
			.base = (CRYPTOLITE_Type *)DT_INST_REG_ADDR(n),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mfd_infineon_mxcryptolite_init, NULL,                             \
			      &mfd_infineon_mxcryptolite_data_##n,                                 \
			      &mfd_infineon_mxcryptolite_config_##n, PRE_KERNEL_1,                 \
			      CONFIG_MFD_INFINEON_MXCRYPTOLITE_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_INFINEON_MXCRYPTOLITE_DEFINE)
