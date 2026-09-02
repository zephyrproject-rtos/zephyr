/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP AHB SRAM Controller (SRAMC) memory-controller driver.
 *
 * This is an init-only "interface" driver: at boot it reads the async-SRAM
 * bus timing from devicetree, fills an MCUXpresso SDK sramc_config_t, and
 * calls the SDK HAL SRAMC_Init() so the external asynchronous parallel SRAM
 * becomes accessible as a normal memory-mapped (AXI) window.
 *
 * The controller programs SRAMCR0/SRAMCR1 which physically live in the
 * BLK_CTRL_WAKEUPMIX block (see i.MX RT1180 RM, chapter "AHB SRAM
 * Controller (SRAMC)"). No runtime data-transfer API is implemented; the
 * external SRAM is described by a child mmio-sram node and accessed directly.
 */

#define DT_DRV_COMPAT nxp_sramc

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_nxp_sramc, CONFIG_MEMC_LOG_LEVEL);

#include "fsl_sramc.h"

struct memc_nxp_sramc_config {
	BLK_CTRL_WAKEUPMIX_Type *base;
	const struct pinctrl_dev_config *pcfg;
	/* SRAMCR0 timing fields */
	uint8_t turnaround_time;
	uint8_t address_hold_time;
	uint8_t address_setup_time;
	uint8_t ce_hold_time;
	uint8_t ce_setup_time;
	uint8_t adv_polarity;   /* sramc_adv_polarity_t */
	uint8_t address_mode;   /* sramc_address_mode_t  */
	uint8_t port_size;      /* sramc_port_size_t     */
	bool bus_timeout_enable;
	uint8_t bus_timeout_counter;
	/* SRAMCR1 timing fields */
	uint8_t prescaler;      /* sramc_prescaler_t */
	uint8_t re_high_time;
	uint8_t re_low_time;
	uint8_t we_high_time;
	uint8_t we_low_time;
};

static int memc_nxp_sramc_init(const struct device *dev)
{
	const struct memc_nxp_sramc_config *cfg = dev->config;
	sramc_config_t sramc_cfg;
	int ret;

	/* Route the SRAMC external bus signals (data/addr/CE/OE/WE/ADV/CS). */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("SRAMC pinctrl apply failed (%d)", ret);
		return ret;
	}

	/* Start from SDK defaults, then override with devicetree timing. */
	SRAMC_GetDefaultConfig(&sramc_cfg);

	sramc_cfg.turnaroundTime      = cfg->turnaround_time;
	sramc_cfg.addressHoldTime     = cfg->address_hold_time;
	sramc_cfg.addressSetupTime    = cfg->address_setup_time;
	sramc_cfg.ceHoldTime          = cfg->ce_hold_time;
	sramc_cfg.ceSetupTime         = cfg->ce_setup_time;
	sramc_cfg.advPolarity         = (sramc_adv_polarity_t)cfg->adv_polarity;
	sramc_cfg.addressMode         = (sramc_address_mode_t)cfg->address_mode;
	sramc_cfg.portSize            = (sramc_port_size_t)cfg->port_size;
	sramc_cfg.busTimeoutEnable    = cfg->bus_timeout_enable;
	sramc_cfg.busTimeoutCounter   = cfg->bus_timeout_counter;
	sramc_cfg.prescaler           = (sramc_prescaler_t)cfg->prescaler;
	sramc_cfg.readEnableHighTime  = cfg->re_high_time;
	sramc_cfg.readEnableLowTime   = cfg->re_low_time;
	sramc_cfg.writeEnableHighTime = cfg->we_high_time;
	sramc_cfg.writeEnableLowTime  = cfg->we_low_time;

	SRAMC_Init(cfg->base, &sramc_cfg);

	LOG_DBG("SRAMC initialized (base %p, port %s)", (void *)cfg->base,
		cfg->port_size == kSRAMC_PortSize16Bit ? "16-bit" : "8-bit");

	return 0;
}

#define MEMC_NXP_SRAMC_DEFINE(inst)							\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static const struct memc_nxp_sramc_config memc_nxp_sramc_cfg_##inst = {		\
		.base = (BLK_CTRL_WAKEUPMIX_Type *)DT_INST_REG_ADDR(inst),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.turnaround_time    = DT_INST_PROP(inst, turnaround_time),		\
		.address_hold_time  = DT_INST_PROP(inst, address_hold_time),		\
		.address_setup_time = DT_INST_PROP(inst, address_setup_time),		\
		.ce_hold_time       = DT_INST_PROP(inst, ce_hold_time),			\
		.ce_setup_time      = DT_INST_PROP(inst, ce_setup_time),		\
		.adv_polarity       = DT_INST_ENUM_IDX(inst, adv_polarity),		\
		.address_mode       = DT_INST_ENUM_IDX(inst, address_mode),		\
		.port_size          = DT_INST_ENUM_IDX(inst, port_size),		\
		.bus_timeout_enable = DT_INST_PROP(inst, bus_timeout_enable),		\
		.bus_timeout_counter = DT_INST_PROP(inst, bus_timeout_counter),		\
		.prescaler          = DT_INST_ENUM_IDX(inst, prescaler),		\
		.re_high_time       = DT_INST_PROP(inst, read_enable_high_time),	\
		.re_low_time        = DT_INST_PROP(inst, read_enable_low_time),		\
		.we_high_time       = DT_INST_PROP(inst, write_enable_high_time),	\
		.we_low_time        = DT_INST_PROP(inst, write_enable_low_time),	\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, memc_nxp_sramc_init, NULL, NULL,			\
			      &memc_nxp_sramc_cfg_##inst, POST_KERNEL,			\
			      CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_NXP_SRAMC_DEFINE)
