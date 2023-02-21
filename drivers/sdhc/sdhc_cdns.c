/*
 * Copyright (C) 2023 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdns_sdhc

#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>

#include "sdhc_cdns_ll.h"

#define EMMC_DESC_SIZE	(1<<20)

#define DEV_CFG(_dev)	((const struct sdhc_cdns_config *)(_dev)->config)
#define DEV_DATA(_dev)	((struct sdhc_cdns_data *const)(_dev)->data)

LOG_MODULE_REGISTER(sdhc_cdns, CONFIG_SDHC_LOG_LEVEL);

static const struct sdmmc_ops *cdns_sdmmc_ops;

struct sdhc_cdns_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	DEVICE_MMIO_NAMED_ROM(sd_pinmux);
	DEVICE_MMIO_NAMED_ROM(combo_phy);
	uint32_t clk_rate;
	const struct device *cdns_clk_dev;
	clock_control_subsys_t clkid;
	const struct reset_dt_spec reset_sdmmc;
	const struct reset_dt_spec reset_sdmmcocp;
	const struct reset_dt_spec reset_softphy;
};

struct sdhc_cdns_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	DEVICE_MMIO_NAMED_RAM(sd_pinmux);
	DEVICE_MMIO_NAMED_RAM(combo_phy);
	struct cdns_sdmmc_params params;
	struct sdmmc_device_info info;
	struct sdhc_io host_io;
};

static int sdhc_cdns_request(const struct device *dev,
	struct sdhc_command *cmd,
	struct sdhc_data *data)
{
	int ret = 0;
	struct sdmmc_cmd cdns_sdmmc_cmd;

	if (dev == NULL) {
		return -EINVAL;
	}

	if (cmd->opcode == SDIO_SEND_OP_COND) {
		return 1;
	}

	/* Initialization of command structure */
	cdns_sdmmc_cmd.cmd_idx =  cmd->opcode;
	cdns_sdmmc_cmd.cmd_arg = cmd->arg;
	cdns_sdmmc_cmd.resp_type = (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK);

	/* Sending command as per the data or non data */
	if (data) {
		ret = cdns_sdmmc_ops->prepare(data->block_addr, (uintptr_t)data->data,
				data);
		cdns_sdmmc_cmd.resp_type = MMC_RESPONSE_R1;
	} else {
		switch (cmd->opcode) {
		case SD_GO_IDLE_STATE:
			cdns_sdmmc_cmd.resp_type = MMC_RESPONSE_NONE;
			break;

		case SD_APP_SEND_OP_COND:
			cdns_sdmmc_cmd.resp_type = MMC_RESPONSE_R3;
			break;

		case SD_SEND_IF_COND:
			cdns_sdmmc_cmd.resp_type = MMC_RESPONSE_R7;
			break;

		case SD_SEND_CSD:

		case SD_ALL_SEND_CID:
			cdns_sdmmc_cmd.resp_type = MMC_RESPONSE_R2;
			break;

		case SD_SEND_RELATIVE_ADDR:
			cdns_sdmmc_cmd.resp_type = MMC_RESPONSE_R6;
			break;

		case SD_SEND_STATUS:

		case SD_SELECT_CARD:

		case SD_APP_CMD:

		case SD_APP_SET_BUS_WIDTH:
			cdns_sdmmc_cmd.resp_type = MMC_RESPONSE_R1;
			break;

		default:
			break;
		}
	}

	ret = cdns_sdmmc_ops->send_cmd(&cdns_sdmmc_cmd, data);

	if (ret == 0) {
		if (cmd->opcode == SD_READ_SINGLE_BLOCK || cmd->opcode == SD_APP_SEND_SCR ||
			cmd->opcode == SD_READ_MULTIPLE_BLOCK) {
			cdns_sdmmc_ops->cache_invd(data->block_addr, (uintptr_t)data->data,
				data->block_size);
		}
	}
	for (int i = 0; i < 4; i++) {
		cmd->response[i] = cdns_sdmmc_cmd.resp_data[i];
	}
	return ret;
}

static int sdhc_cdns_get_card_present(const struct device *dev)
{
	int ret;

	ret = cdns_sdmmc_ops->card_present();
	return ret;
}

static int sdhc_cdns_card_busy(const struct device *dev)
{
	int ret;

	ret = cdns_sdmmc_ops->busy();
	return ret;
}

static int sdhc_cdns_get_host_props(const struct device *dev,
	struct sdhc_host_props *props)
{
	if (dev == NULL) {
		return -EINVAL;
	}
	memset(props, 0, sizeof(struct sdhc_host_props));
	props->f_min = SDMMC_CLOCK_400KHZ;
	props->f_max = SD_CLOCK_25MHZ;
	props->power_delay = 1000;
	props->host_caps.vol_330_support = true;
	props->is_spi = false;
	return 0;
}

static int sdhc_cdns_reset(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	return cdns_sdmmc_ops->reset();
}

static int sdhc_cdns_init(const struct device *dev)
{
	struct sdhc_cdns_data *const data = DEV_DATA(dev);
	const struct sdhc_cdns_config *sdhc_config = DEV_CFG(dev);
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, sd_pinmux, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, combo_phy, K_MEM_CACHE_NONE);

	/* clock setting */
	if (sdhc_config->clk_rate == 0U) {
		if (!device_is_ready(sdhc_config->cdns_clk_dev)) {
			LOG_ERR("Device is not ready\n");
			return -EINVAL;
		}

		ret = clock_control_get_rate(sdhc_config->cdns_clk_dev,
			sdhc_config->clkid, &data->params.clk_rate);

		if (ret != 0) {
			return ret;
		}
	} else {
		data->params.clk_rate = sdhc_config->clk_rate;
	}

	/* Setting regbase */
	data->params.reg_base = DEVICE_MMIO_NAMED_GET(dev, reg_base);
	data->params.reg_pinmux = DEVICE_MMIO_NAMED_GET(dev, sd_pinmux);
	data->params.reg_phy = DEVICE_MMIO_NAMED_GET(dev, combo_phy);
	data->params.combophy = (DEVICE_MMIO_NAMED_ROM_PTR((dev), combo_phy)->phys_addr);
	data->params.combophy = ((data->params.combophy) & (0x0000FFFF));
	/*resetting the lines*/
	if (!device_is_ready(sdhc_config->reset_sdmmc.dev) ||
		!device_is_ready(sdhc_config->reset_sdmmcocp.dev) ||
		!device_is_ready(sdhc_config->reset_softphy.dev)) {
		LOG_ERR("Reset device node not found");
		return -ENODEV;
	}

	reset_line_assert(sdhc_config->reset_softphy.dev,
		sdhc_config->reset_softphy.id);
	reset_line_deassert(sdhc_config->reset_softphy.dev,
		sdhc_config->reset_softphy.id);

	reset_line_assert(sdhc_config->reset_sdmmc.dev,
		sdhc_config->reset_sdmmc.id);
	reset_line_deassert(sdhc_config->reset_sdmmc.dev,
		sdhc_config->reset_sdmmc.id);

	reset_line_assert(sdhc_config->reset_sdmmcocp.dev,
		sdhc_config->reset_sdmmcocp.id);
	reset_line_deassert(sdhc_config->reset_sdmmcocp.dev,
		sdhc_config->reset_sdmmcocp.id);

	/* Issue cdns_INIT */
	cdns_sdmmc_init(&data->params, &data->info, &cdns_sdmmc_ops);

	ret = sdhc_cdns_reset(dev);
	if (ret != 0U) {
		LOG_ERR("reset is failed ... try again");
		return ret;
	}

	ret = cdns_sdmmc_ops->init();
	if (ret != 0U) {
		LOG_ERR("Initialization is failed");
		return ret;
	}

	return 0;
}

static int sdhc_cdns_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_cdns_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;

	if (host_io->bus_width != ios->bus_width || host_io->clock != ios->clock) {
		host_io->bus_width = ios->bus_width;
		host_io->clock = ios->clock;
		return cdns_sdmmc_ops->set_ios(ios->clock, ios->bus_width);
	}
	return 0;
}

static struct sdhc_driver_api sdhc_cdns_api = {
	.request = sdhc_cdns_request,
	.set_io = sdhc_cdns_set_io,
	.get_host_props = sdhc_cdns_get_host_props,
	.get_card_present = sdhc_cdns_get_card_present,
	.reset = sdhc_cdns_reset,
	.card_busy = sdhc_cdns_card_busy,
};

#define SDHC_CDNS_CLOCK_RATE_INIT(inst) \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clock_frequency), \
			( \
				.clk_rate = DT_INST_PROP(inst, clock_frequency), \
				.cdns_clk_dev = NULL, \
				.clkid = (clock_control_subsys_t)0, \
			), \
			( \
				.clk_rate = 0, \
				.cdns_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)), \
				.clkid = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clkid), \
			) \
		)

#define SDHC_CDNS_INIT(inst)									\
	static struct cdns_idmac_desc cdns_desc	\
			[CONFIG_CDNS_DESC_COUNT] __aligned(32);	\
										\
	const struct sdhc_cdns_config sdhc_cdns_config_##inst = {	\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(			\
				reg_base, DT_DRV_INST(inst)),		\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(			\
				sd_pinmux, DT_DRV_INST(inst)),		\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(			\
				combo_phy, DT_DRV_INST(inst)),		\
		SDHC_CDNS_CLOCK_RATE_INIT(inst)			\
		.reset_sdmmc = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0),	\
		.reset_sdmmcocp = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 1),	\
		.reset_softphy = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 2),	\
	};										\
	struct sdhc_cdns_data sdhc_cdns_data_##inst = {		\
		.params = {							\
			.bus_width = SDHC_BUS_WIDTH1BIT,			\
			.desc_base = (uintptr_t) &cdns_desc,		\
			.desc_size = EMMC_DESC_SIZE,				\
			.flags = 0,							\
		},										\
		.info = {								\
			.cdn_sdmmc_dev_type = SD_DS,				\
			.ocr_voltage = OCR_3_3_3_4 | OCR_3_2_3_3,	\
		},									\
	};										\
	DEVICE_DT_INST_DEFINE(inst,						\
			&sdhc_cdns_init,						\
			NULL,									\
			&sdhc_cdns_data_##inst,					\
			&sdhc_cdns_config_##inst,				\
			POST_KERNEL,							\
			CONFIG_SDHC_INIT_PRIORITY,				\
			&sdhc_cdns_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_CDNS_INIT)
