/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_imx_flexspi

#include <logging/log.h>
#include <sys/util.h>

#include "memc_mcux_flexspi.h"

LOG_MODULE_REGISTER(memc_flexspi, CONFIG_MEMC_LOG_LEVEL);

struct memc_flexspi_config {
	FLEXSPI_Type *base;
	uint8_t *ahb_base;
	bool xip;
	bool ahb_bufferable;
	bool ahb_cacheable;
	bool ahb_prefetch;
	bool ahb_read_addr_opt;
	bool combination_mode;
	bool sck_differential_clock;
	flexspi_read_sample_clock_t rx_sample_clock;
};

struct memc_flexspi_data {
	size_t size[kFLEXSPI_PortCount];
};

bool memc_flexspi_is_running_xip(const struct device *dev)
{
	const struct memc_flexspi_config *config = dev->config;

	return config->xip;
}

int memc_flexspi_update_lut(const struct device *dev, uint32_t index,
		const uint32_t *cmd, uint32_t count)
{
	const struct memc_flexspi_config *config = dev->config;

	FLEXSPI_UpdateLUT(config->base, index, cmd, count);

	return 0;
}

int memc_flexspi_set_device_config(const struct device *dev,
		const flexspi_device_config_t *device_config,
		flexspi_port_t port)
{
	const struct memc_flexspi_config *config = dev->config;
	struct memc_flexspi_data *data = dev->data;

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

int memc_flexspi_reset(const struct device *dev)
{
	const struct memc_flexspi_config *config = dev->config;

	FLEXSPI_SoftwareReset(config->base);

	return 0;
}

int memc_flexspi_transfer(const struct device *dev,
		flexspi_transfer_t *transfer)
{
	const struct memc_flexspi_config *config = dev->config;
	status_t status = FLEXSPI_TransferBlocking(config->base, transfer);

	if (status != kStatus_Success) {
		LOG_ERR("Transfer error: %d", status);
		return -EIO;
	}

	return 0;
}

void *memc_flexspi_get_ahb_address(const struct device *dev,
		flexspi_port_t port, off_t offset)
{
	const struct memc_flexspi_config *config = dev->config;
	struct memc_flexspi_data *data = dev->data;
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

static int memc_flexspi_init(const struct device *dev)
{
	const struct memc_flexspi_config *config = dev->config;
	flexspi_config_t flexspi_config;

	/* we should not configure the device we are running on */
	if (memc_flexspi_is_running_xip(dev)) {
		LOG_DBG("XIP active on %s, skipping init", dev->name);
		return 0;
	}

	FLEXSPI_GetDefaultConfig(&flexspi_config);

	flexspi_config.ahbConfig.enableAHBBufferable = config->ahb_bufferable;
	flexspi_config.ahbConfig.enableAHBCachable = config->ahb_cacheable;
	flexspi_config.ahbConfig.enableAHBPrefetch = config->ahb_prefetch;
	flexspi_config.ahbConfig.enableReadAddressOpt = config->ahb_read_addr_opt;
	flexspi_config.enableCombination = config->combination_mode;
	flexspi_config.enableSckBDiffOpt = config->sck_differential_clock;
	flexspi_config.rxSampleClock = config->rx_sample_clock;

	FLEXSPI_Init(config->base, &flexspi_config);

	return 0;
}

#if defined(CONFIG_XIP) && defined(CONFIG_CODE_FLEXSPI)
#define MEMC_FLEXSPI_CFG_XIP(node_id) DT_SAME_NODE(node_id, DT_NODELABEL(flexspi))
#elif defined(CONFIG_XIP) && defined(CONFIG_CODE_FLEXSPI2)
#define MEMC_FLEXSPI_CFG_XIP(node_id) DT_SAME_NODE(node_id, DT_NODELABEL(flexspi2))
#elif defined(CONFIG_SOC_SERIES_IMX_RT6XX)
#define MEMC_FLEXSPI_CFG_XIP(node_id) IS_ENABLED(CONFIG_XIP)
#else
#define MEMC_FLEXSPI_CFG_XIP(node_id) false
#endif

#define MEMC_FLEXSPI(n)							\
	static const struct memc_flexspi_config				\
		memc_flexspi_config_##n = {				\
		.base = (FLEXSPI_Type *) DT_INST_REG_ADDR(n),		\
		.xip = MEMC_FLEXSPI_CFG_XIP(DT_DRV_INST(n)),		\
		.ahb_base = (uint8_t *) DT_INST_REG_ADDR_BY_IDX(n, 1),	\
		.ahb_bufferable = DT_INST_PROP(n, ahb_bufferable),	\
		.ahb_cacheable = DT_INST_PROP(n, ahb_cacheable),	\
		.ahb_prefetch = DT_INST_PROP(n, ahb_prefetch),		\
		.ahb_read_addr_opt = DT_INST_PROP(n, ahb_read_addr_opt),\
		.combination_mode = DT_INST_PROP(n, combination_mode),	\
		.sck_differential_clock = DT_INST_PROP(n, sck_differential_clock),	\
		.rx_sample_clock = DT_INST_PROP(n, rx_clock_source),	\
	};								\
									\
	static struct memc_flexspi_data memc_flexspi_data_##n;		\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      memc_flexspi_init,			\
			      NULL,					\
			      &memc_flexspi_data_##n,			\
			      &memc_flexspi_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_FLEXSPI)
