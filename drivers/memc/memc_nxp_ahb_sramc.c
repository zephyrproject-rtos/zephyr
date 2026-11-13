/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NXP AHB SRAM Controller (AHB_SRAMC) memory-controller driver.
 *
 * This driver owns the controller: its clock, its pin control and the MCR
 * settings shared by every chip select. External devices are separate devices
 * and program their own chip select through
 * memc_nxp_ahb_sramc_configure_target().
 */

#define DT_DRV_COMPAT nxp_ahb_sramc

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/memc/memc_nxp_ahb_sramc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_sramc.h>

LOG_MODULE_REGISTER(memc_nxp_ahb_sramc, CONFIG_MEMC_LOG_LEVEL);

struct sramc_timing_field {
	bool add_one; /* Whether the field counts as (N + 1) granularity periods */
	uint8_t width; /* Field width in bits */
};

static const struct sramc_timing_field
	sramc_timing_fields[MEMC_NXP_AHB_SRAMC_TIMING_COUNT] = {
		[MEMC_NXP_AHB_SRAMC_TIMING_CE_SETUP] = {.add_one = false, .width = 4U},
		[MEMC_NXP_AHB_SRAMC_TIMING_CE_HOLD] = {.add_one = false, .width = 4U},
		[MEMC_NXP_AHB_SRAMC_TIMING_ADDRESS_SETUP] = {.add_one = false, .width = 4U},
		[MEMC_NXP_AHB_SRAMC_TIMING_ADDRESS_HOLD] = {.add_one = true, .width = 4U},
		[MEMC_NXP_AHB_SRAMC_TIMING_TURNAROUND] = {.add_one = true, .width = 4U},
		[MEMC_NXP_AHB_SRAMC_TIMING_WE_LOW] = {.add_one = true, .width = 8U},
		[MEMC_NXP_AHB_SRAMC_TIMING_WE_HIGH] = {.add_one = true, .width = 8U},
		[MEMC_NXP_AHB_SRAMC_TIMING_RE_LOW] = {.add_one = true, .width = 8U},
		[MEMC_NXP_AHB_SRAMC_TIMING_RE_HIGH] = {.add_one = true, .width = 8U},
	};

struct memc_nxp_ahb_sramc_data {
	/* Functional clock rate, which every timing field is counted in. */
	uint32_t hclk_hz;
	/* Chip selects that already carry a configured device. */
	uint8_t configured_cs;
};

struct memc_nxp_ahb_sramc_config {
	AHB_SRAMC_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	/* Shared by every chip select: the hardware keeps these in MCR. */
	uint8_t port_size;
	uint8_t address_mode;
	uint8_t adv_polarity;
	uint8_t bus_timeout;
	bool func_clock_always_on;
};

/*
 * Program MCR from the controller-wide devicetree properties.
 */
static void sramc_program_controller_config(AHB_SRAMC_Type *base,
					    const struct memc_nxp_ahb_sramc_config *cfg)
{
	SRAMC_LockRegisters(base, false);

	base->MCR = AHB_SRAMC_MCR_CG_DIS(cfg->func_clock_always_on ? 1U : 0U) |
		    AHB_SRAMC_MCR_BTO(cfg->bus_timeout) |
		    AHB_SRAMC_MCR_BTOEN(cfg->bus_timeout != 0U ? 1U : 0U) |
		    AHB_SRAMC_MCR_ADVP(cfg->adv_polarity) |
		    AHB_SRAMC_MCR_AM(cfg->address_mode) | AHB_SRAMC_MCR_PS(cfg->port_size);

	SRAMC_LockRegisters(base, true);
}

/*
 * Convert one timing to its register field value at the given prescaler.
 */
static int sramc_timing_field_value(uint32_t hclk_hz, uint32_t time_ns, uint32_t prescaler,
				    const struct sramc_timing_field *field, uint32_t *value)
{
	/* A requested time of 0 ns always maps to a register value of 0. */
	if (time_ns == 0U) {
		*value = 0U;
		return 0;
	}

	if (SRAMC_CalculateTimingValue(hclk_hz, time_ns, prescaler, field->add_one, field->width,
				       value) != kStatus_Success) {
		return -ERANGE;
	}

	return 0;
}

/*
 * Choose a prescaler for one chip select and convert all nine of its timings
 * into the register fields they land in.
 *
 * The prescaler sets the granularity every field of that chip select is
 * counted in, and is itself per chip select. The finest prescaler the timings
 * fit in gives the least rounding.
 */
static int sramc_resolve_timings(uint32_t hclk_hz, const struct memc_nxp_ahb_sramc_target *target,
				 uint32_t *prescaler, uint32_t *values)
{
	/*
	 * Walk from the finest prescaler upwards and stop at the first one every
	 * field fits in.
	 */

	for (uint32_t pre = (uint32_t)kSRAMC_Prescaler_1; pre <= (uint32_t)kSRAMC_Prescaler_4;
	     pre++) {
		bool fits = true;

		for (size_t i = 0U; i < MEMC_NXP_AHB_SRAMC_TIMING_COUNT; i++) {
			if (sramc_timing_field_value(hclk_hz, target->timing_ns[i], pre,
						     &sramc_timing_fields[i], &values[i]) != 0) {
				fits = false;
				break;
			}
		}

		if (fits) {
			*prescaler = pre;
			return 0;
		}
	}

	return -ERANGE;
}

/*
 * Program the CFG0_CSn/CFG1_CSn pair of one chip select. The other chip selects
 * are left untouched.
 */
static void sramc_program_chip_select_config(AHB_SRAMC_Type *base,
					     const struct memc_nxp_ahb_sramc_target *target,
					     uint32_t prescaler, const uint32_t *values)
{
	uint8_t cs = target->chip_select;

	SRAMC_LockRegisters(base, false);

	base->CFG0_CS[cs].CFG0_CS =
		AHB_SRAMC_CFG0_CS_PRE(prescaler) |
		AHB_SRAMC_CFG0_CS_TA(values[MEMC_NXP_AHB_SRAMC_TIMING_TURNAROUND]) |
		AHB_SRAMC_CFG0_CS_AH(values[MEMC_NXP_AHB_SRAMC_TIMING_ADDRESS_HOLD]) |
		AHB_SRAMC_CFG0_CS_AS(values[MEMC_NXP_AHB_SRAMC_TIMING_ADDRESS_SETUP]) |
		AHB_SRAMC_CFG0_CS_CEH(values[MEMC_NXP_AHB_SRAMC_TIMING_CE_HOLD]) |
		AHB_SRAMC_CFG0_CS_CES(values[MEMC_NXP_AHB_SRAMC_TIMING_CE_SETUP]) |
		AHB_SRAMC_CFG0_CS_WP_POL(target->wait_active_high ? 1U : 0U) |
		AHB_SRAMC_CFG0_CS_WP_MUX(target->wait_pin) |
		AHB_SRAMC_CFG0_CS_WM_SEL(target->wait_mode) |
		AHB_SRAMC_CFG0_CS_WM_EN(target->wait_enable ? 1U : 0U);

	base->CFG0_CS[cs].CFG1_CS =
		AHB_SRAMC_CFG1_CS_REH(values[MEMC_NXP_AHB_SRAMC_TIMING_RE_HIGH]) |
		AHB_SRAMC_CFG1_CS_REL(values[MEMC_NXP_AHB_SRAMC_TIMING_RE_LOW]) |
		AHB_SRAMC_CFG1_CS_WEH(values[MEMC_NXP_AHB_SRAMC_TIMING_WE_HIGH]) |
		AHB_SRAMC_CFG1_CS_WEL(values[MEMC_NXP_AHB_SRAMC_TIMING_WE_LOW]);

	SRAMC_LockRegisters(base, true);
}

int memc_nxp_ahb_sramc_configure_target(const struct device *dev,
					const struct memc_nxp_ahb_sramc_target *target)
{
	const struct memc_nxp_ahb_sramc_config *cfg;
	struct memc_nxp_ahb_sramc_data *data;
	uint32_t values[MEMC_NXP_AHB_SRAMC_TIMING_COUNT];
	uint32_t prescaler;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (target == NULL) {
		return -EINVAL;
	}

	if (target->chip_select >= AHB_SRAMC_CFG0_CS_COUNT) {
		return -EINVAL;
	}

	cfg = dev->config;
	data = dev->data;

	if ((data->configured_cs & BIT(target->chip_select)) != 0U) {
		return -EBUSY;
	}

	ret = sramc_resolve_timings(data->hclk_hz, target, &prescaler, values);
	if (ret != 0) {
		return ret;
	}

	sramc_program_chip_select_config(cfg->base, target, prescaler, values);

	data->configured_cs |= (uint8_t)BIT(target->chip_select);

	LOG_DBG("CS%u ready, granularity %u clocks", target->chip_select, prescaler + 1U);

	return 0;
}

static int memc_nxp_ahb_sramc_init(const struct device *dev)
{
	const struct memc_nxp_ahb_sramc_config *cfg = dev->config;
	struct memc_nxp_ahb_sramc_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock controller is not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret != 0) {
		LOG_ERR("cannot ungate the controller (%d)", ret);
		return ret;
	}

	ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &data->hclk_hz);
	if (ret != 0) {
		LOG_ERR("cannot read the functional clock rate (%d)", ret);
		return ret;
	}

	if (data->hclk_hz == 0U) {
		LOG_ERR("functional clock rate is zero");
		return -EINVAL;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("cannot apply the pin configuration (%d)", ret);
		return ret;
	}

	/* Program MCR; every chip select is programmed by its own device. */
	sramc_program_controller_config(cfg->base, cfg);

	return 0;
}

#define MEMC_NXP_AHB_SRAMC_DEFINE(inst) \
	PINCTRL_DT_INST_DEFINE(inst); \
 \
	static struct memc_nxp_ahb_sramc_data memc_nxp_ahb_sramc_data_##inst; \
 \
	static const struct memc_nxp_ahb_sramc_config memc_nxp_ahb_sramc_cfg_##inst = { \
		.base = (AHB_SRAMC_Type *)DT_INST_REG_ADDR(inst), \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)), \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name), \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), \
		.port_size = DT_INST_ENUM_IDX(inst, port_size), \
		.address_mode = DT_INST_ENUM_IDX(inst, address_mode), \
		.adv_polarity = DT_INST_ENUM_IDX(inst, adv_polarity), \
		.bus_timeout = DT_INST_PROP(inst, bus_timeout), \
		.func_clock_always_on = DT_INST_PROP(inst, function_clock_always_on), \
	}; \
 \
	DEVICE_DT_INST_DEFINE(inst, memc_nxp_ahb_sramc_init, NULL, \
			      &memc_nxp_ahb_sramc_data_##inst, &memc_nxp_ahb_sramc_cfg_##inst, \
			      POST_KERNEL, CONFIG_MEMC_NXP_AHB_SRAMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_NXP_AHB_SRAMC_DEFINE)
