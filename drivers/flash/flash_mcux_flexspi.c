/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_imx_flexspi

#include <drivers/flash.h>
#include <logging/log.h>
#include <sys/util.h>
#include "flash_mcux_flexspi.h"

LOG_MODULE_REGISTER(flash_flexspi, CONFIG_FLASH_LOG_LEVEL);

struct flash_flexspi_config {
	FLEXSPI_Type *base;
	uint8_t *ahb_base;
	bool ahb_bufferable;
	bool ahb_cacheable;
	bool ahb_prefetch;
	bool ahb_read_addr_opt;
	bool combination_mode;
	flexspi_read_sample_clock_t rx_sample_clock;
};

struct flash_flexspi_data {
	size_t size[kFLEXSPI_PortCount];
};

int flash_flexspi_update_lut(const struct device *dev, uint32_t index,
		const uint32_t *cmd, uint32_t count)
{
	const struct flash_flexspi_config *config = dev->config;

	FLEXSPI_UpdateLUT(config->base, index, cmd, count);

	return 0;
}

int flash_flexspi_set_flash_config(const struct device *dev,
		const flexspi_device_config_t *device_config,
		flexspi_port_t port)
{
	const struct flash_flexspi_config *config = dev->config;
	struct flash_flexspi_data *data = dev->data;

	if (port >= kFLEXSPI_PortCount) {
		LOG_ERR("Invalid port number");
		return -EINVAL;
	}

	data->size[port] = device_config->flashSize * KB(1);

	FLEXSPI_SetFlashConfig(config->base,
			       (flexspi_device_config_t *) device_config,
			       port);

	return 0;
}

int flash_flexspi_reset(const struct device *dev)
{
	const struct flash_flexspi_config *config = dev->config;

	FLEXSPI_SoftwareReset(config->base);

	return 0;
}

int flash_flexspi_transfer(const struct device *dev,
		flexspi_transfer_t *transfer)
{
	const struct flash_flexspi_config *config = dev->config;
	status_t status = FLEXSPI_TransferBlocking(config->base, transfer);

	if (status != kStatus_Success) {
		LOG_ERR("Transfer error: %d", status);
		return -EIO;
	}

	return 0;
}

void *flash_flexspi_get_ahb_address(const struct device *dev,
		flexspi_port_t port, off_t offset)
{
	const struct flash_flexspi_config *config = dev->config;
	struct flash_flexspi_data *data = dev->data;
	int i;

	if (port >= kFLEXSPI_PortCount) {
		LOG_ERR("Invalid port number");
		return NULL;
	}

	for (i = 0; i < port; i++) {
		offset += data->size[port];
	}

	return config->ahb_base + offset;
}

static int flash_flexspi_init(const struct device *dev)
{
	const struct flash_flexspi_config *config = dev->config;
	flexspi_config_t flexspi_config;

	FLEXSPI_GetDefaultConfig(&flexspi_config);

	flexspi_config.ahbConfig.enableAHBBufferable = config->ahb_bufferable;
	flexspi_config.ahbConfig.enableAHBCachable = config->ahb_cacheable;
	flexspi_config.ahbConfig.enableAHBPrefetch = config->ahb_prefetch;
	flexspi_config.ahbConfig.enableReadAddressOpt = config->ahb_read_addr_opt;
	flexspi_config.enableCombination = config->combination_mode;
	flexspi_config.rxSampleClock = config->rx_sample_clock;

	FLEXSPI_Init(config->base, &flexspi_config);

	return 0;
}

#define FLASH_FLEXSPI(n)						\
	static const struct flash_flexspi_config			\
		flash_flexspi_config_##n = {				\
		.base = (FLEXSPI_Type *) DT_INST_REG_ADDR(n),		\
		.ahb_base = (uint8_t *) DT_INST_REG_ADDR_BY_IDX(n, 1),	\
		.ahb_bufferable = DT_INST_PROP(n, ahb_bufferable),	\
		.ahb_cacheable = DT_INST_PROP(n, ahb_cacheable),	\
		.ahb_prefetch = DT_INST_PROP(n, ahb_prefetch),		\
		.ahb_read_addr_opt = DT_INST_PROP(n, ahb_read_addr_opt),\
		.combination_mode = DT_INST_PROP(n, combination_mode),	\
		.rx_sample_clock = DT_INST_PROP(n, rx_clock_source),	\
	};								\
									\
	static struct flash_flexspi_data flash_flexspi_data_##n;	\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      flash_flexspi_init,			\
			      device_pm_control_nop,			\
			      &flash_flexspi_data_##n,			\
			      &flash_flexspi_config_##n,		\
			      POST_KERNEL,				\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(FLASH_FLEXSPI)
