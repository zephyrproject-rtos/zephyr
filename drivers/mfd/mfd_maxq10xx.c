/*
 * Copyright (c) 2025 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_maxq10xx

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/mfd/mfd_maxq10xx.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_maxq10xx, CONFIG_MFD_LOG_LEVEL);

struct mfd_maxq10xx_config {
	struct spi_dt_spec spi;
};

struct mfd_maxq10xx_data {
	struct k_sem sem_lock;
};

struct k_sem *mfd_maxq10xx_get_lock(const struct device *dev)
{
	struct mfd_maxq10xx_data *data = dev->data;

	return &data->sem_lock;
}

static int mfd_maxq10xx_init(const struct device *dev)
{
	const struct mfd_maxq10xx_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}

	return 0;
}

BUILD_ASSERT(CONFIG_SPI_INIT_PRIORITY < CONFIG_MFD_MAXQ10XX_INIT_PRIORITY,
	     "SPI driver must be initialized before maxq10xx mfd driver");

#define DEFINE_MAXQ10XX_MFD(_num)                                                                  \
	static const struct mfd_maxq10xx_config mfd_maxq10xx_config##_num = {                      \
		.spi = SPI_DT_SPEC_INST_GET(_num, SPI_WORD_SET(8), 0),                             \
	};                                                                                         \
	static struct mfd_maxq10xx_data mfd_maxq10xx_data##_num = {                                \
		.sem_lock = Z_SEM_INITIALIZER(mfd_maxq10xx_data##_num.sem_lock, 1, 1),             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(_num, mfd_maxq10xx_init, NULL, &mfd_maxq10xx_data##_num,             \
			      &mfd_maxq10xx_config##_num, POST_KERNEL,                             \
			      CONFIG_MFD_MAXQ10XX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_MAXQ10XX_MFD);
