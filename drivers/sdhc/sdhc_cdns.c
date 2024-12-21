/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdns_sdhc

#include <zephyr/drivers/sdhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>

#include "sdhc_cdns_ll.h"

#define SDHC_CDNS_DESC_SIZE	(1<<20)
#define COMBOPHY_ADDR_MASK	0x0000FFFFU

#define DEV_CFG(_dev)	((const struct sdhc_cdns_config *)(_dev)->config)
#define DEV_DATA(_dev)	((struct sdhc_cdns_data *const)(_dev)->data)

LOG_MODULE_REGISTER(sdhc_cdns, CONFIG_SDHC_LOG_LEVEL);

/* SDMMC operations FPs are the element of structure*/
static const struct sdhc_cdns_ops *cdns_sdmmc_ops;

struct sdhc_cdns_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	DEVICE_MMIO_NAMED_ROM(combo_phy);
	/* Clock rate for host */
	uint32_t clk_rate;
	/* power delay prop for host */
	uint32_t power_delay_ms;
	/* run time device structure */
	const struct device *cdns_clk_dev;
	/* type to identify a clock controller sub-system */
	clock_control_subsys_t clkid;
	/* Reset controller device configuration. */
	const struct reset_dt_spec reset_sdmmc;
	const struct reset_dt_spec reset_sdmmcocp;
	const struct reset_dt_spec reset_softphy;
};

struct sdhc_cdns_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	DEVICE_MMIO_NAMED_RAM(combo_phy);
	/* Host controller parameters */
	struct sdhc_cdns_params params;
	/* sdmmc device informartaion for host */
	struct sdmmc_device_info info;
	/* Input/Output configuration */
	struct sdhc_io host_io;
};

static int sdhc_cdns_request(const struct device *dev,
	struct sdhc_command *cmd,
	struct sdhc_data *data)
{
	int ret = 0;
	struct sdmmc_cmd cdns_sdmmc_cmd;

	if (cmd == NULL) {
		LOG_ERR("Wrong CMD parameter");
		return -EINVAL;
	}

	/* Initialization of command structure */
	cdns_sdmmc_cmd.cmd_idx =  cmd->opcode;
	cdns_sdmmc_cmd.cmd_arg = cmd->arg;
	cdns_sdmmc_cmd.resp_type = (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK);

	/* Sending command as per the data or non data */
	if (data) {
		ret = cdns_sdmmc_ops->prepare(data->block_addr, (uintptr_t)data->data,
				data);
		if (ret != 0) {
			LOG_ERR("DMA Prepare failed");
			return -EINVAL;
		}
	}

	ret = cdns_sdmmc_ops->send_cmd(&cdns_sdmmc_cmd, data);

	if (ret == 0) {
		if (cmd->opcode == SD_READ_SINGLE_BLOCK || cmd->opcode == SD_APP_SEND_SCR ||
			cmd->opcode == SD_READ_MULTIPLE_BLOCK) {

			if (data == NULL) {
				LOG_ERR("Invalid data parameter");
				return -ENODATA;
			}
			ret = cdns_sdmmc_ops->cache_invd(data->block_addr, (uintptr_t)data->data,
				data->block_size);
			if (ret != 0) {
				return ret;
			}
		}
	}
	/* copying all responses as per response type */
	for (int i = 0; i < 4; i++) {
		cmd->response[i] = cdns_sdmmc_cmd.resp_data[i];
	}
	return ret;
}

static int sdhc_cdns_get_card_present(const struct device *dev)
{
	return cdns_sdmmc_ops->card_present();
}

static int sdhc_cdns_card_busy(const struct device *dev)
{
	return cdns_sdmmc_ops->busy();
}

static int sdhc_cdns_get_host_props(const struct device *dev,
	struct sdhc_host_props *props)
{
	const struct sdhc_cdns_config *sdhc_config = DEV_CFG(dev);

	memset(props, 0, sizeof(struct sdhc_host_props));
	props->f_min = SDMMC_CLOCK_400KHZ;
	/*
	 * default max speed is 25MHZ, as per SCR register
	 * it will switch accordingly
	 */
	props->f_max = SD_CLOCK_25MHZ;
	props->power_delay = sdhc_config->power_delay_ms;
	props->host_caps.vol_330_support = true;
	props->is_spi = false;
	return 0;
}

static int sdhc_cdns_reset(const struct device *dev)
{
	return cdns_sdmmc_ops->reset();
}

static int sdhc_cdns_init(const struct device *dev)
{
	struct sdhc_cdns_data *const data = DEV_DATA(dev);
	const struct sdhc_cdns_config *sdhc_config = DEV_CFG(dev);
	int ret;

	/* SDHC reg base */
	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);
	/* ComboPhy reg base */
	DEVICE_MMIO_NAMED_MAP(dev, combo_phy, K_MEM_CACHE_NONE);

	/* clock setting */
	if (sdhc_config->clk_rate == 0U) {
		if (!device_is_ready(sdhc_config->cdns_clk_dev)) {
			LOG_ERR("Clock controller device is not ready");
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
	data->params.reg_phy = DEVICE_MMIO_NAMED_GET(dev, combo_phy);
	data->params.combophy = (DEVICE_MMIO_NAMED_ROM_PTR((dev),
		combo_phy)->phys_addr);
	data->params.combophy = (data->params.combophy & COMBOPHY_ADDR_MASK);

	/* resetting the lines */
	if (sdhc_config->reset_sdmmc.dev != NULL) {
		if (!device_is_ready(sdhc_config->reset_sdmmc.dev) ||
			!device_is_ready(sdhc_config->reset_sdmmcocp.dev) ||
			!device_is_ready(sdhc_config->reset_softphy.dev)) {
			LOG_ERR("Reset device not found");
			return -ENODEV;
		}

		ret = reset_line_toggle(sdhc_config->reset_softphy.dev,
			sdhc_config->reset_softphy.id);
		if (ret != 0) {
			LOG_ERR("Softphy Reset failed");
			return ret;
		}

		ret = reset_line_toggle(sdhc_config->reset_sdmmc.dev,
			sdhc_config->reset_sdmmc.id);
		if (ret != 0) {
			LOG_ERR("sdmmc Reset failed");
			return ret;
		}

		ret = reset_line_toggle(sdhc_config->reset_sdmmcocp.dev,
			sdhc_config->reset_sdmmcocp.id);
		if (ret != 0) {
			LOG_ERR("sdmmcocp Reset failed");
			return ret;
		}
	}

	/* Init function to call lower layer file */
	sdhc_cdns_sdmmc_init(&data->params, &data->info, &cdns_sdmmc_ops);

	ret = sdhc_cdns_reset(dev);
	if (ret != 0U) {
		LOG_ERR("Card reset failed");
		return ret;
	}

	/* Init operation called for register initialisation */
	ret = cdns_sdmmc_ops->init();
	if (ret != 0U) {
		LOG_ERR("Card initialization failed");
		return ret;
	}

	return 0;
}

static int sdhc_cdns_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_cdns_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;

	if (host_io->bus_width != ios->bus_width || host_io->clock !=
		ios->clock) {
		host_io->bus_width = ios->bus_width;
		host_io->clock = ios->clock;
		return cdns_sdmmc_ops->set_ios(ios->clock, ios->bus_width);
	}
	return 0;
}

static DEVICE_API(sdhc, sdhc_cdns_api) = {
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

#define SDHC_CDNS_RESET_SPEC_INIT(inst) \
	.reset_sdmmc = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0),	\
	.reset_sdmmcocp = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 1),\
	.reset_softphy = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 2),

#define SDHC_CDNS_INIT(inst)						\
	static struct sdhc_cdns_desc cdns_desc				\
			[CONFIG_CDNS_DESC_COUNT];			\
									\
	static const struct sdhc_cdns_config sdhc_cdns_config_##inst = {\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(			\
				reg_base, DT_DRV_INST(inst)),		\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(			\
				combo_phy, DT_DRV_INST(inst)),		\
		SDHC_CDNS_CLOCK_RATE_INIT(inst)				\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, resets),		\
			(SDHC_CDNS_RESET_SPEC_INIT(inst)))	\
		.power_delay_ms = DT_INST_PROP(inst, power_delay_ms),	\
	};								\
	static struct sdhc_cdns_data sdhc_cdns_data_##inst = {		\
		.params = {						\
			.bus_width = SDHC_BUS_WIDTH1BIT,		\
			.desc_base = (uintptr_t) &cdns_desc,		\
			.desc_size = SDHC_CDNS_DESC_SIZE,		\
			.flags = 0,					\
		},							\
		.info = {						\
			.cdn_sdmmc_dev_type = SD_DS,			\
			.ocr_voltage = OCR_3_3_3_4 | OCR_3_2_3_3,	\
		},							\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			&sdhc_cdns_init,				\
			NULL,						\
			&sdhc_cdns_data_##inst,				\
			&sdhc_cdns_config_##inst,			\
			POST_KERNEL,					\
			CONFIG_SDHC_INIT_PRIORITY,			\
			&sdhc_cdns_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_CDNS_INIT)
