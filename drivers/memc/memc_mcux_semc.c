/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcux_semc

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/memc/memc_mcux_semc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(memc_mcux_semc, CONFIG_MEMC_LOG_LEVEL);

#define MEMC_MCUX_SEMC_RESET_TIMEOUT_MS 10U

struct memc_mcux_semc_config {
	SEMC_Type *base;
	const struct pinctrl_dev_config *pincfg;
	int16_t cmd_timeout_cycles;
	int16_t bus_timeout_cycles;
	bool dqs_pad_loopback;
};

struct memc_mcux_semc_data {
	struct k_mutex lock;
	semc_config_t cfg;
};

static int memc_mcux_semc_status_to_errno(status_t status)
{
	switch (status) {
	case kStatus_Success:
		return 0;
	case kStatus_SEMC_InvalidDeviceType:
	case kStatus_SEMC_InvalidMemorySize:
	case kStatus_SEMC_InvalidIpcmdDataSize:
	case kStatus_SEMC_InvalidAddressPortWidth:
	case kStatus_SEMC_InvalidDataPortWidth:
	case kStatus_SEMC_InvalidSwPinmuxSelection:
	case kStatus_SEMC_InvalidBurstLength:
	case kStatus_SEMC_InvalidColumnAddressBitWidth:
	case kStatus_SEMC_InvalidBaseAddress:
	case kStatus_SEMC_InvalidTimerSetting:
		return -EINVAL;
	case kStatus_SEMC_IpCommandExecutionError:
	case kStatus_SEMC_AxiCommandExecutionError:
		return -EIO;
	default:
		return -EIO;
	}
}

static int memc_mcux_semc_check_dev(const struct device *dev)
{
	if ((dev == NULL) || !device_is_ready(dev)) {
		return -ENODEV;
	}

	return 0;
}

int memc_mcux_semc_get_default_config(semc_config_t *config)
{
	if (config == NULL) {
		return -EINVAL;
	}

	SEMC_GetDefaultConfig(config);
	return 0;
}

int memc_mcux_semc_reconfigure(const struct device *dev, const semc_config_t *config)
{
	int ret = memc_mcux_semc_check_dev(dev);
	struct memc_mcux_semc_data *data;
	const struct memc_mcux_semc_config *dev_cfg;

	if (ret != 0) {
		return ret;
	}

	if (config == NULL) {
		return -EINVAL;
	}

	data = dev->data;
	dev_cfg = dev->config;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->cfg = *config;
	SEMC_Init(dev_cfg->base, &data->cfg);
	k_mutex_unlock(&data->lock);

	return 0;
}

int memc_mcux_semc_soft_reset(const struct device *dev)
{
	int ret = memc_mcux_semc_check_dev(dev);
	const struct memc_mcux_semc_config *dev_cfg;
	struct memc_mcux_semc_data *data;
	uint32_t t_start;

	if (ret != 0) {
		return ret;
	}

	dev_cfg = dev->config;
	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Same sequence as SEMC_Init()/SEMC_Deinit() in the MCUX SDK. */
	dev_cfg->base->MCR = SEMC_MCR_SWRST_MASK;
	t_start = k_uptime_get_32();

	while ((dev_cfg->base->MCR & SEMC_MCR_SWRST_MASK) != 0U) {
		if ((k_uptime_get_32() - t_start) > MEMC_MCUX_SEMC_RESET_TIMEOUT_MS) {
			k_mutex_unlock(&data->lock);
			return -ETIMEDOUT;
		}
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

int memc_mcux_semc_configure_sdram(const struct device *dev, semc_sdram_cs_t cs,
				   semc_sdram_config_t *config, uint32_t clk_src_hz)
{
	int ret = memc_mcux_semc_check_dev(dev);
	const struct memc_mcux_semc_config *dev_cfg;
	struct memc_mcux_semc_data *data;
	status_t status;

	if (ret != 0) {
		return ret;
	}

	if ((config == NULL) || (clk_src_hz == 0U)) {
		return -EINVAL;
	}

	dev_cfg = dev->config;
	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	status = SEMC_ConfigureSDRAM(dev_cfg->base, cs, config, clk_src_hz);
	k_mutex_unlock(&data->lock);

	return memc_mcux_semc_status_to_errno(status);
}

int memc_mcux_semc_configure_sram(const struct device *dev, semc_sram_cs_t cs,
				  semc_sram_config_t *config, uint32_t clk_src_hz)
{
	int ret = memc_mcux_semc_check_dev(dev);
	const struct memc_mcux_semc_config *dev_cfg;
	struct memc_mcux_semc_data *data;
	status_t status;

	if (ret != 0) {
		return ret;
	}

	if ((config == NULL) || (clk_src_hz == 0U)) {
		return -EINVAL;
	}

	dev_cfg = dev->config;
	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	status = SEMC_ConfigureSRAMWithChipSelection(dev_cfg->base, cs, config, clk_src_hz);
	k_mutex_unlock(&data->lock);

	return memc_mcux_semc_status_to_errno(status);
}

int memc_mcux_semc_configure_nor(const struct device *dev, semc_nor_config_t *config,
				 uint32_t clk_src_hz)
{
	int ret = memc_mcux_semc_check_dev(dev);
	const struct memc_mcux_semc_config *dev_cfg;
	struct memc_mcux_semc_data *data;
	status_t status;

	if (ret != 0) {
		return ret;
	}

	if ((config == NULL) || (clk_src_hz == 0U)) {
		return -EINVAL;
	}

	dev_cfg = dev->config;
	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	status = SEMC_ConfigureNOR(dev_cfg->base, config, clk_src_hz);
	k_mutex_unlock(&data->lock);

	return memc_mcux_semc_status_to_errno(status);
}

int memc_mcux_semc_configure_nand(const struct device *dev, semc_nand_config_t *config,
				  uint32_t clk_src_hz)
{
	int ret = memc_mcux_semc_check_dev(dev);
	const struct memc_mcux_semc_config *dev_cfg;
	struct memc_mcux_semc_data *data;
	status_t status;

	if (ret != 0) {
		return ret;
	}

	if ((config == NULL) || (clk_src_hz == 0U)) {
		return -EINVAL;
	}

	dev_cfg = dev->config;
	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	status = SEMC_ConfigureNAND(dev_cfg->base, config, clk_src_hz);
	k_mutex_unlock(&data->lock);

	return memc_mcux_semc_status_to_errno(status);
}

static int memc_mcux_semc_init(const struct device *dev)
{
	const struct memc_mcux_semc_config *dev_cfg = dev->config;
	struct memc_mcux_semc_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	if (dev_cfg->pincfg != NULL) {
		ret = pinctrl_apply_state(dev_cfg->pincfg, PINCTRL_STATE_DEFAULT);
		/* Some boards rely on ROM/bootloader pinmux and provide no pinctrl state. */
		if ((ret < 0) && (ret != -ENOENT)) {
			return ret;
		}
	}

	SEMC_GetDefaultConfig(&data->cfg);
	if (dev_cfg->cmd_timeout_cycles >= 0) {
		data->cfg.cmdTimeoutCycles = (uint8_t)dev_cfg->cmd_timeout_cycles;
	}
	if (dev_cfg->bus_timeout_cycles >= 0) {
		data->cfg.busTimeoutCycles = (uint8_t)dev_cfg->bus_timeout_cycles;
	}
	if (dev_cfg->dqs_pad_loopback) {
		data->cfg.dqsMode = kSEMC_Loopbackdqspad;
	}

	SEMC_Init(dev_cfg->base, &data->cfg);

	return 0;
}

#define MEMC_MCUX_SEMC_INIT(inst)                                                                  \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct memc_mcux_semc_data memc_mcux_semc_data_##inst;                              \
	static const struct memc_mcux_semc_config memc_mcux_semc_config_##inst = {                 \
		.base = (SEMC_Type *)DT_INST_REG_ADDR(inst),                                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.cmd_timeout_cycles = DT_INST_PROP_OR(inst, nxp_cmd_timeout_cycles, -1),           \
		.bus_timeout_cycles = DT_INST_PROP_OR(inst, nxp_bus_timeout_cycles, -1),           \
		.dqs_pad_loopback = DT_INST_PROP_OR(inst, nxp_dqs_pad_loopback, 0),                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, memc_mcux_semc_init, NULL, &memc_mcux_semc_data_##inst,        \
			      &memc_mcux_semc_config_##inst, POST_KERNEL,                          \
			      CONFIG_MEMC_MCUX_SEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MCUX_SEMC_INIT)
