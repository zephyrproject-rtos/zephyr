/*
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_imx_flexspi

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>

#include "memc_mcux_flexspi.h"


/*
 * NOTE: If CONFIG_FLASH_MCUX_FLEXSPI_XIP is selected, Any external functions
 * called while interacting with the flexspi MUST be relocated to SRAM or ITCM
 * at runtime, so that the chip does not access the flexspi to read program
 * instructions while it is being written to
 */
#if defined(CONFIG_FLASH_MCUX_FLEXSPI_XIP) && (CONFIG_MEMC_LOG_LEVEL > 0)
#warning "Enabling memc driver logging and XIP mode simultaneously can cause \
	read-while-write hazards. This configuration is not recommended."
#endif

LOG_MODULE_REGISTER(memc_flexspi, CONFIG_MEMC_LOG_LEVEL);

struct memc_flexspi_buf_cfg {
	uint16_t prefetch;
	uint16_t priority;
	uint16_t master_id;
	uint16_t buf_size;
} __packed;

/* flexspi device data should be stored in RAM to avoid read-while-write hazards */
struct memc_flexspi_data {
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
	const struct pinctrl_dev_config *pincfg;
	size_t size[kFLEXSPI_PortCount];
	struct memc_flexspi_buf_cfg *buf_cfg;
	uint8_t buf_cfg_cnt;
};

void memc_flexspi_wait_bus_idle(const struct device *dev)
{
	struct memc_flexspi_data *data = dev->data;

	while (false == FLEXSPI_GetBusIdleStatus(data->base)) {
	}
}

bool memc_flexspi_is_running_xip(const struct device *dev)
{
	struct memc_flexspi_data *data = dev->data;

	return data->xip;
}

int memc_flexspi_update_lut(const struct device *dev, uint32_t index,
		const uint32_t *cmd, uint32_t count)
{
	struct memc_flexspi_data *data = dev->data;

	FLEXSPI_UpdateLUT(data->base, index, cmd, count);

	return 0;
}

int memc_flexspi_set_device_config(const struct device *dev,
		const flexspi_device_config_t *device_config,
		flexspi_port_t port)
{
	struct memc_flexspi_data *data = dev->data;

	if (port >= kFLEXSPI_PortCount) {
		LOG_ERR("Invalid port number");
		return -EINVAL;
	}

	data->size[port] = device_config->flashSize * KB(1);

	FLEXSPI_SetFlashConfig(data->base,
			       (flexspi_device_config_t *) device_config,
			       port);

	return 0;
}

int memc_flexspi_reset(const struct device *dev)
{
	struct memc_flexspi_data *data = dev->data;

	FLEXSPI_SoftwareReset(data->base);

	return 0;
}

int memc_flexspi_transfer(const struct device *dev,
		flexspi_transfer_t *transfer)
{
	struct memc_flexspi_data *data = dev->data;
	status_t status = FLEXSPI_TransferBlocking(data->base, transfer);

	if (status != kStatus_Success) {
		LOG_ERR("Transfer error: %d", status);
		return -EIO;
	}

	return 0;
}

void *memc_flexspi_get_ahb_address(const struct device *dev,
		flexspi_port_t port, off_t offset)
{
	struct memc_flexspi_data *data = dev->data;
	int i;

	if (port >= kFLEXSPI_PortCount) {
		LOG_ERR("Invalid port number: %u", port);
		return NULL;
	}

	for (i = 0; i < port; i++) {
		offset += data->size[port];
	}

	return data->ahb_base + offset;
}

static int memc_flexspi_init(const struct device *dev)
{
	struct memc_flexspi_data *data = dev->data;
	flexspi_config_t flexspi_config;

	/* we should not configure the device we are running on */
	if (memc_flexspi_is_running_xip(dev)) {
		LOG_DBG("XIP active on %s, skipping init", dev->name);
		return 0;
	}

	/*
	 * SOCs such as the RT1064 and RT1024 have internal flash, and no pinmux
	 * settings, continue if no pinctrl state found.
	 */
	int ret;

	ret = pinctrl_apply_state(data->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	FLEXSPI_GetDefaultConfig(&flexspi_config);

	flexspi_config.ahbConfig.enableAHBBufferable = data->ahb_bufferable;
	flexspi_config.ahbConfig.enableAHBCachable = data->ahb_cacheable;
	flexspi_config.ahbConfig.enableAHBPrefetch = data->ahb_prefetch;
	flexspi_config.ahbConfig.enableReadAddressOpt = data->ahb_read_addr_opt;
#if !(defined(FSL_FEATURE_FLEXSPI_HAS_NO_MCR0_COMBINATIONEN) && \
	FSL_FEATURE_FLEXSPI_HAS_NO_MCR0_COMBINATIONEN)
	flexspi_config.enableCombination = data->combination_mode;
#endif
	flexspi_config.enableSckBDiffOpt = data->sck_differential_clock;
	flexspi_config.rxSampleClock = data->rx_sample_clock;

	/* Configure AHB RX buffers, if any configuration settings are present */
	__ASSERT(data->buf_cfg_cnt < FSL_FEATURE_FLEXSPI_AHB_BUFFER_COUNT,
		"Maximum RX buffer configuration count exceeded");
	for (uint8_t i = 0; i < data->buf_cfg_cnt; i++) {
		/* Should AHB prefetch up to buffer size? */
		flexspi_config.ahbConfig.buffer[i].enablePrefetch = data->buf_cfg[i].prefetch;
		/* AHB access priority (used for suspending control of AHB prefetching )*/
		flexspi_config.ahbConfig.buffer[i].priority = data->buf_cfg[i].priority;
		/* AHB master index, SOC specific */
		flexspi_config.ahbConfig.buffer[i].masterIndex = data->buf_cfg[i].master_id;
		/* RX buffer allocation (total available buffer space is instance/SOC specific) */
		flexspi_config.ahbConfig.buffer[i].bufferSize = data->buf_cfg[i].buf_size;
	}

	FLEXSPI_Init(data->base, &flexspi_config);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int memc_flexspi_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct memc_flexspi_data *data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(data->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(data->pincfg, PINCTRL_STATE_SLEEP);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(CONFIG_XIP) && defined(CONFIG_CODE_FLEXSPI)
#define MEMC_FLEXSPI_CFG_XIP(node_id) DT_SAME_NODE(node_id, DT_NODELABEL(flexspi))
#elif defined(CONFIG_XIP) && defined(CONFIG_CODE_FLEXSPI2)
#define MEMC_FLEXSPI_CFG_XIP(node_id) DT_SAME_NODE(node_id, DT_NODELABEL(flexspi2))
#elif defined(CONFIG_SOC_SERIES_IMX_RT6XX) || defined(CONFIG_SOC_SERIES_IMX_RT5XX)
#define MEMC_FLEXSPI_CFG_XIP(node_id) DT_SAME_NODE(node_id, DT_NODELABEL(flexspi))
#else
#define MEMC_FLEXSPI_CFG_XIP(node_id) false
#endif

#define MEMC_FLEXSPI(n)							\
	PINCTRL_DT_INST_DEFINE(n);					\
	static uint16_t  buf_cfg_##n[] =				\
		DT_INST_PROP_OR(n, rx_buffer_config, {0});		\
									\
	static struct memc_flexspi_data					\
		memc_flexspi_data_##n = {				\
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
		.buf_cfg = (struct memc_flexspi_buf_cfg *)buf_cfg_##n,	\
		.buf_cfg_cnt = sizeof(buf_cfg_##n) /			\
			sizeof(struct memc_flexspi_buf_cfg),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(n, memc_flexspi_pm_action);		\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      memc_flexspi_init,			\
			      PM_DEVICE_DT_INST_GET(n),			\
			      &memc_flexspi_data_##n,			\
			      NULL,					\
			      POST_KERNEL,				\
			      CONFIG_MEMC_INIT_PRIORITY,	\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_FLEXSPI)
