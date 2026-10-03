/*
 * Copyright (c) 2026 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_mfd

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/ite_it51xxx.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_ite_it51xxx, CONFIG_MFD_LOG_LEVEL);

Z_GENERIC_SECTION(.__hwcrypto_dlm_block) union hwcrypto_dlm_block hwcrypto_dlm;

struct mfd_ite_it51xxx_config {
	mm_reg_t base;
};

struct mfd_ite_it51xxx_data {
	struct k_mutex lock;
};

mm_reg_t mfd_ite_it51xxx_get_base(const struct device *dev)
{
	const struct mfd_ite_it51xxx_config *config = dev->config;

	return config->base;
}

int mfd_ite_it51xxx_lock(const struct device *dev, k_timeout_t timeout)
{
	struct mfd_ite_it51xxx_data *data = dev->data;

	return k_mutex_lock(&data->lock, timeout);
}

void mfd_ite_it51xxx_unlock(const struct device *dev)
{
	struct mfd_ite_it51xxx_data *data = dev->data;

	k_mutex_unlock(&data->lock);
}

static int mfd_ite_it51xxx_init(const struct device *dev)
{
	struct mfd_ite_it51xxx_data *data = dev->data;

	k_mutex_init(&data->lock);

	return 0;
}

#define MFD_ITE_IT51XXX_DEFINE(n)                                                                  \
	static struct mfd_ite_it51xxx_data mfd_ite_it51xxx_data_##n;                               \
	static const struct mfd_ite_it51xxx_config mfd_ite_it51xxx_config_##n = {                  \
		.base = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mfd_ite_it51xxx_init, NULL, &mfd_ite_it51xxx_data_##n,            \
			      &mfd_ite_it51xxx_config_##n, PRE_KERNEL_1,                           \
			      CONFIG_MFD_ITE_IT51XXX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_ITE_IT51XXX_DEFINE)
