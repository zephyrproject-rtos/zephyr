/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_ebu

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

#include <soc.h>
#include <xmc_ebu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_xmc4xxx_ebu, CONFIG_MEMC_LOG_LEVEL);

struct memc_xmc4xxx_region_config {
	uint8_t region_index;
	uint32_t addrsel;
	uint32_t busrcon;
	uint32_t busrap;
	uint32_t buswcon;
	uint32_t buswap;
};

struct memc_xmc4xxx_config {
	XMC_EBU_t *ebu;
	uint8_t region_len;
	const struct memc_xmc4xxx_region_config *region_config;
	uint32_t clc;
	uint32_t modcon;
	uint32_t usercon;
	const struct pinctrl_dev_config *pcfg;
	uint32_t sdram_control;
	uint32_t sdram_operation_mode;
	uint32_t sdram_refresh_control;
};

#define CLC_ACK_Msk (EBU_CLC_SYNCACK_Msk | EBU_CLC_DIV2ACK_Msk | EBU_CLC_EBUDIVACK_Msk)
#define NUM_EBU_REGIONS 4

static int xmc4xxx_ebu_init(const struct device *dev)
{
	int ret;
	const struct memc_xmc4xxx_config *cfg = dev->config;
	bool sdram_enabled = false;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	XMC_EBU_Enable(cfg->ebu);
	cfg->ebu->CLC = cfg->clc;
	while ((cfg->ebu->CLC & CLC_ACK_Msk) >> 4 != cfg->clc)
		;
	cfg->ebu->MODCON = cfg->modcon;
	cfg->ebu->USERCON = cfg->usercon;

	for (int i = 0; i < cfg->region_len; i++) {
		const struct memc_xmc4xxx_region_config *region_config;
		int region_index;

		region_config = &cfg->region_config[i];
		region_index = region_config->region_index;

		if (region_index >= NUM_EBU_REGIONS) {
			LOG_ERR("Invalid region index");
			return -EINVAL;
		}

		if (((region_config->busrcon & EBU_BUSRCON0_AGEN_Msk) >> EBU_BUSRCON0_AGEN_Pos) ==
			XMC_EBU_DEVICE_TYPE_SDRAM) {
			sdram_enabled = true;
		}

		cfg->ebu->BUS[region_index].RDCON = region_config->busrcon;
		cfg->ebu->BUS[region_index].RDAPR = region_config->busrap;
		cfg->ebu->BUS[region_index].WRCON = region_config->buswcon;
		cfg->ebu->BUS[region_index].WRAPR = region_config->buswap;

		LOG_DBG("Region %d BUSRCON 0x%x", region_index, region_config->busrcon);
		LOG_DBG("Region %d BUSRAP 0x%x", region_index, region_config->busrap);
		LOG_DBG("Region %d BUSWCON 0x%x", region_index, region_config->buswcon);
		LOG_DBG("Region %d BUSWAP 0x%x", region_index, region_config->buswap);

		cfg->ebu->ADDRSEL[region_index] |= region_config->addrsel;
		while (!XMC_EBU_IsBusAribitrationSelected(cfg->ebu))
			;
	}

	if (sdram_enabled) {
		cfg->ebu->SDRMREF = cfg->sdram_refresh_control;
		cfg->ebu->SDRMCON = cfg->sdram_control;
		cfg->ebu->SDRMOD = cfg->sdram_operation_mode;
	}

	return 0;
}

#define REGION_CONFIG(node_id)                                                                     \
{                                                                                                  \
	.region_index = DT_REG_ADDR(node_id),                                                      \
	.addrsel = DT_PROP(node_id, infineon_region_enable) << EBU_ADDRSEL0_REGENAB_Pos |          \
		   DT_PROP(node_id, infineon_alternate_region_enable) << EBU_ADDRSEL0_ALTENAB_Pos |\
		   DT_PROP(node_id, infineon_write_protect_enable) << EBU_ADDRSEL0_WPROT_Pos,      \
	.busrcon = DT_PROP(node_id, infineon_bus_read_config) |                                    \
		   DT_ENUM_IDX(node_id, infineon_device_type) << EBU_BUSRCON0_AGEN_Pos |           \
		  (DT_ENUM_IDX(node_id, infineon_address_bus_width) + 1) << EBU_BUSRCON0_PORTW_Pos,\
	.buswcon = DT_PROP(node_id, infineon_bus_write_config) |                                   \
		   DT_ENUM_IDX(node_id, infineon_device_type) << EBU_BUSWCON0_AGEN_Pos,            \
	.busrap = DT_PROP(node_id, infineon_bus_read_timing),                                      \
	.buswap = DT_PROP(node_id, infineon_bus_write_timing),                                     \
},

static const struct memc_xmc4xxx_region_config xmc4xxx_ebu_region_config[] = {
	DT_INST_FOREACH_CHILD(0, REGION_CONFIG)};
PINCTRL_DT_INST_DEFINE(0);
static const struct memc_xmc4xxx_config xmc4xxx_ebu_config_0 = {
	.ebu = (XMC_EBU_t *)DT_INST_REG_ADDR(0),
	.region_len = ARRAY_SIZE(xmc4xxx_ebu_region_config),
	.region_config = xmc4xxx_ebu_region_config,
	.clc = DT_INST_PROP(0, clk_config),
	.modcon = DT_INST_PROP(0, modes_config),
	.usercon = DT_INST_PROP(0, gpio_control_config),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.sdram_control = DT_INST_PROP_OR(0, sdram_control, 0),
	.sdram_operation_mode = DT_INST_PROP_OR(0, sdram_operation_mode, 0),
	.sdram_refresh_control = DT_INST_PROP_OR(0, sdram_refresh_control, 0),
};

DEVICE_DT_INST_DEFINE(0, xmc4xxx_ebu_init, NULL, NULL, &xmc4xxx_ebu_config_0, POST_KERNEL,
		      CONFIG_MEMC_INIT_PRIORITY, NULL);
