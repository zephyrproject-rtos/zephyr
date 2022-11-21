/*
 * Copyright (C) 2022 Intel Corporation
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

#define DT_DRV_COMPAT snps_designware_sdmmc

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>

#include "mmc_dw_ll.h"

#define EMMC_DESC_SIZE		(1<<20)

#define DEV_CFG(_dev)		((const struct sdhc_dw_config *)(_dev)->config)
#define DEV_DATA(_dev)		((struct sdhc_dw_data *const)(_dev)->data)
#define RESP_MAX			4
#define DELAY_IN_MS			1000

LOG_MODULE_REGISTER(sdhc_snps_dw, CONFIG_SDHC_LOG_LEVEL);

static const struct mmc_ops *dw_mmc_ops;

struct sdhc_dw_config {
	DEVICE_MMIO_ROM;
	uint32_t clk_rate;
	const struct device *dw_clk_dev;
	clock_control_subsys_t clkid;
	const struct reset_dt_spec reset_sdmmc;
	const struct reset_dt_spec reset_sdmmcocp;
};

struct sdhc_dw_data {
	DEVICE_MMIO_RAM;
	struct dw_mmc_params params;
	struct mmc_device_info info;
	struct sdhc_io host_io;
};

static int sdhc_dw_request(const struct device *dev,
	struct sdhc_command *cmd,
	struct sdhc_data *data)
{
	int ret = 0;

	if (dev == NULL) {
		LOG_ERR("Wrong parameter\n");
		return -EINVAL;
	}
	struct mmc_cmd dw_mmc_cmd;

	if (cmd->opcode == SDIO_SEND_OP_COND) {
		return 1;
	}
	/* Initialization of command structure */
	dw_mmc_cmd.cmd_idx =  cmd->opcode;
	dw_mmc_cmd.cmd_arg = cmd->arg;
	dw_mmc_cmd.resp_type = cmd->response_type;

	switch (cmd->opcode) {
	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		ret = dw_mmc_ops->prepare(data->block_addr, (uintptr_t)data->data,
			data->blocks * data->block_size);
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	case SD_READ_MULTIPLE_BLOCK:
	case SD_READ_SINGLE_BLOCK:
		ret = dw_mmc_ops->prepare(data->block_addr, (uintptr_t)data->data,
			data->blocks * data->block_size);
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	case SD_APP_SEND_OP_COND:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R3;
		break;
	case SD_SEND_IF_COND:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	case SD_SEND_CSD:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R2;
		break;
	case SD_ALL_SEND_CID:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R2;
		break;
	case SD_SEND_RELATIVE_ADDR:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	case SD_SEND_STATUS:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	case SD_SELECT_CARD:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	case SD_APP_SEND_SCR:
		ret = dw_mmc_ops->prepare(data->block_addr, (uintptr_t)data->data,
			data->blocks * data->block_size);
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	case SD_APP_CMD:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	case SD_GO_IDLE_STATE:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_NONE;
		break;
	case SD_APP_SET_BUS_WIDTH:
		dw_mmc_cmd.resp_type = MMC_RESPONSE_R1;
		break;
	default:
		break;
	}

	ret = dw_mmc_ops->send_cmd(&dw_mmc_cmd);

	if (ret == 0) {
		if (cmd->opcode == SD_READ_SINGLE_BLOCK || cmd->opcode ==
				SD_APP_SEND_SCR || cmd->opcode == SD_READ_MULTIPLE_BLOCK) {
			dw_mmc_ops->read(data->block_addr, (uintptr_t)data->data,
				data->block_size);
		}
		if (cmd->opcode == SD_WRITE_SINGLE_BLOCK ||
				cmd->opcode == SD_WRITE_MULTIPLE_BLOCK) {
			dw_mmc_ops->write(data->block_addr, (uintptr_t)data->data,
				data->block_size);
		}
	}
	for (int index = 0; index < RESP_MAX; index++) {
		cmd->response[index] = dw_mmc_cmd.resp_data[index];
	}
	return ret;
}

static int sdhc_dw_get_card_present(const struct device *dev)
{
	int ret;

	ret = dw_mmc_ops->card_present();
	return ret;
}

static int sdhc_dw_card_busy(const struct device *dev)
{
	int ret;

	ret = dw_mmc_ops->busy();
	return ret;
}

static int sdhc_dw_get_host_props(const struct device *dev,
	struct sdhc_host_props *props)
{
	if (dev == NULL) {
		LOG_ERR("Wrong parameter\n");
		return -EINVAL;
	}
	memset(props, 0, sizeof(struct sdhc_host_props));
	props->f_min = SDMMC_CLOCK_400KHZ;
	props->f_max = SD_CLOCK_25MHZ;
	props->power_delay = DELAY_IN_MS;
	props->host_caps.vol_330_support = true;
	props->is_spi = false;
	return 0;
}

static int sdhc_dw_reset(const struct device *dev)
{
	if (dev == NULL) {
		LOG_ERR("Wrong parameter\n");
		return -EINVAL;
	}
	/* Reset host I/O */
	return 0;
}

static int sdhc_dw_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	struct sdhc_dw_data *const data = DEV_DATA(dev);
	const struct sdhc_dw_config *sdhc_config = DEV_CFG(dev);
	uint32_t ret;

	/* clock setting */
	if (sdhc_config->clk_rate == 0U) {
		if (!device_is_ready(sdhc_config->dw_clk_dev)) {
			LOG_ERR("Device is not ready\n");
			return -EINVAL;
		}

		ret = clock_control_get_rate(sdhc_config->dw_clk_dev,
			sdhc_config->clkid, &data->params.clk_rate);

		if (ret != 0) {
			return ret;
		}
	} else {
		data->params.clk_rate = sdhc_config->clk_rate;
	}

	/* Setting regbase */
	data->params.reg_base = DEVICE_MMIO_GET(dev);

	dw_mmc_init(&data->params, &data->info, &dw_mmc_ops);

	/*resetting the lines*/
	if (!device_is_ready(sdhc_config->reset_sdmmc.dev) ||
		!device_is_ready(sdhc_config->reset_sdmmcocp.dev)) {
		LOG_ERR("Reset device node not found");
		return -ENODEV;
	}

	reset_line_assert(sdhc_config->reset_sdmmc.dev,
		sdhc_config->reset_sdmmc.id);
	reset_line_deassert(sdhc_config->reset_sdmmc.dev,
		sdhc_config->reset_sdmmc.id);

	reset_line_assert(sdhc_config->reset_sdmmcocp.dev,
		sdhc_config->reset_sdmmcocp.id);
	reset_line_deassert(sdhc_config->reset_sdmmcocp.dev,
		sdhc_config->reset_sdmmcocp.id);

	dw_mmc_ops->init();
	return 0;
}

static int sdhc_dw_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_dw_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;

	if (host_io->bus_width != ios->bus_width || host_io->clock != ios->clock) {
		host_io->bus_width = ios->bus_width;
		host_io->clock = ios->clock;
		return dw_mmc_ops->set_ios(ios->clock, ios->bus_width);
	}
	return 0;
}

static struct sdhc_driver_api sdhc_dw_api = {
	.request = sdhc_dw_request,
	.set_io = sdhc_dw_set_io,
	.get_host_props = sdhc_dw_get_host_props,
	.get_card_present = sdhc_dw_get_card_present,
	.reset = sdhc_dw_reset,
	.card_busy = sdhc_dw_card_busy,
};

#define SDMMC_SNPS_DW_CLOCK_RATE_INIT(inst) \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clock_frequency), \
			( \
				.clk_rate = DT_INST_PROP(inst, clock_frequency), \
				.dw_clk_dev = NULL, \
				.clkid = (clock_control_subsys_t)0, \
			), \
			( \
				.clk_rate = 0, \
				.dw_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)), \
				.clkid = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clkid), \
			) \
		)

#define SDHC_DW_INIT(inst)									\
	static struct dw_idmac_desc dw_desc __aligned(512);		\
										\
	const struct sdhc_dw_config sdhc_dw_config_##inst = {	\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),			\
		SDMMC_SNPS_DW_CLOCK_RATE_INIT(inst)			\
		.reset_sdmmc = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0),	\
		.reset_sdmmcocp = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 1),	\
	};											\
	struct sdhc_dw_data sdhc_dw_data_##inst = {				\
		.params = {								\
			.bus_width = SDHC_BUS_WIDTH1BIT,					\
			.desc_base = (uintptr_t) &dw_desc,				\
			.desc_size = EMMC_DESC_SIZE,					\
			.flags = 0,							\
		},										\
		.info = {								\
			.mmc_dev_type = MMC_IS_SD,						\
			.ocr_voltage = OCR_3_3_3_4 | OCR_3_2_3_3,		\
		},										\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			&sdhc_dw_init,						\
			NULL,								\
			&sdhc_dw_data_##inst,							\
			&sdhc_dw_config_##inst,							\
			POST_KERNEL,						\
			CONFIG_SDHC_INIT_PRIORITY,						\
			&sdhc_dw_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_DW_INIT)
