/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NXP AHB SRAM Controller (AHB_SRAMC) memory-controller driver.
 * Supports asynchronous SRAM connected via the AHB SRAM Controller.
 */

#define DT_DRV_COMPAT nxp_ahb_sramc

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/memc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_sramc.h>

LOG_MODULE_REGISTER(memc_nxp_ahb_sramc, CONFIG_MEMC_LOG_LEVEL);

/* The timings of one chip select, in the order they are converted below. */
enum sramc_timing {
	SRAMC_TIMING_CE_SETUP,
	SRAMC_TIMING_CE_HOLD,
	SRAMC_TIMING_ADDRESS_SETUP,
	SRAMC_TIMING_ADDRESS_HOLD,
	SRAMC_TIMING_TURNAROUND,
	SRAMC_TIMING_WE_LOW,
	SRAMC_TIMING_WE_HIGH,
	SRAMC_TIMING_RE_LOW,
	SRAMC_TIMING_RE_HIGH,
	SRAMC_TIMING_COUNT,
};

struct sramc_timing_field {
	bool add_one; /* Whether the field counts as (N + 1) granularity periods */
	uint8_t width; /* Field width in bits */
};

static const struct sramc_timing_field sramc_timing_fields[SRAMC_TIMING_COUNT] = {
	[SRAMC_TIMING_CE_SETUP] = {.add_one = false, .width = 4U},
	[SRAMC_TIMING_CE_HOLD] = {.add_one = false, .width = 4U},
	[SRAMC_TIMING_ADDRESS_SETUP] = {.add_one = false, .width = 4U},
	[SRAMC_TIMING_ADDRESS_HOLD] = {.add_one = true, .width = 4U},
	[SRAMC_TIMING_TURNAROUND] = {.add_one = true, .width = 4U},
	[SRAMC_TIMING_WE_LOW] = {.add_one = true, .width = 8U},
	[SRAMC_TIMING_WE_HIGH] = {.add_one = true, .width = 8U},
	[SRAMC_TIMING_RE_LOW] = {.add_one = true, .width = 8U},
	[SRAMC_TIMING_RE_HIGH] = {.add_one = true, .width = 8U},
};

struct memc_nxp_ahb_sramc_cs {
	uint32_t timing_ns[SRAMC_TIMING_COUNT];
	/* Memory-mapped range of this chip select, from its memory-region. */
	void *mem_base;
	uint64_t mem_size;
	uint8_t chip_select;
	uint8_t wait_mode;
	uint8_t wait_pin;
	bool wait_enable;
	bool wait_active_high;
};

struct memc_nxp_ahb_sramc_config {
	AHB_SRAMC_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	const struct memc_nxp_ahb_sramc_cs *cs;
	uint8_t cs_count;
	/* Shared by every chip select: the hardware keeps these in MCR. */
	uint8_t port_size;
	uint8_t address_mode;
	uint8_t adv_polarity;
	uint8_t bus_timeout;
	bool func_clock_always_on;
};

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
 * Resolve one chip select's timings: choose the prescaler and convert all nine
 * nanosecond values into the register fields they land in.
 */
static int sramc_cs_resolve_timings(uint32_t hclk_hz, const struct memc_nxp_ahb_sramc_cs *cs,
				    uint32_t *prescaler, uint32_t *values)
{
	/*
	 * The prescaler sets the granularity every field of that chip select is
	 * counted in. A coarser granularity lets the fields hold longer times, but it
	 * rounds every timing up by more. Walking from the finest upwards and stopping
	 * at the first that fits therefore gives the least rounding available. The
	 * prescaler is a per-chip-select field, so each one gets this done on its own.
	 */

	for (uint32_t pre = (uint32_t)kSRAMC_Prescaler_1; pre <= (uint32_t)kSRAMC_Prescaler_4;
	     pre++) {
		bool fits = true;

		for (size_t i = 0U; i < SRAMC_TIMING_COUNT; i++) {
			if (sramc_timing_field_value(hclk_hz, cs->timing_ns[i], pre,
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

static void sramc_cs_apply(const struct memc_nxp_ahb_sramc_config *cfg,
			   const struct memc_nxp_ahb_sramc_cs *cs, uint32_t prescaler,
			   const uint32_t *values)
{
	sramc_config_t sramc_cfg;

	SRAMC_GetDefaultConfig(&sramc_cfg);


	sramc_cfg.chipSelect = (sramc_chip_select_t)BIT(cs->chip_select);

	/*
	 * Controller-wide settings, every chip shares the same configuration.
	 */
	sramc_cfg.portSize = (sramc_port_size_t)cfg->port_size;
	sramc_cfg.addressMode = (sramc_address_mode_t)cfg->address_mode;
	sramc_cfg.advPolarity = (sramc_adv_polarity_t)cfg->adv_polarity;
	sramc_cfg.busTimeoutCounter = cfg->bus_timeout;
	sramc_cfg.busTimeoutEnable = (cfg->bus_timeout != 0U);
	sramc_cfg.funcClkAlwaysOn = cfg->func_clock_always_on;

	/*
	 * Chip-specific settings, from CFGn_CSn.
	 */
	sramc_cfg.prescaler = (sramc_prescaler_t)prescaler;
	sramc_cfg.ceSetupTime = (uint8_t)values[SRAMC_TIMING_CE_SETUP];
	sramc_cfg.ceHoldTime = (uint8_t)values[SRAMC_TIMING_CE_HOLD];
	sramc_cfg.addressSetupTime = (uint8_t)values[SRAMC_TIMING_ADDRESS_SETUP];
	sramc_cfg.addressHoldTime = (uint8_t)values[SRAMC_TIMING_ADDRESS_HOLD];
	sramc_cfg.turnaroundTime = (uint8_t)values[SRAMC_TIMING_TURNAROUND];
	sramc_cfg.writeEnableLowTime = (uint8_t)values[SRAMC_TIMING_WE_LOW];
	sramc_cfg.writeEnableHighTime = (uint8_t)values[SRAMC_TIMING_WE_HIGH];
	sramc_cfg.readEnableLowTime = (uint8_t)values[SRAMC_TIMING_RE_LOW];
	sramc_cfg.readEnableHighTime = (uint8_t)values[SRAMC_TIMING_RE_HIGH];
	sramc_cfg.waitModeEnable = cs->wait_enable;
	sramc_cfg.waitPinMux = (sramc_wait_pin_mux_t)cs->wait_pin;
	sramc_cfg.waitMode = (sramc_wait_mode_t)cs->wait_mode;
	sramc_cfg.waitPolarity = cs->wait_active_high ? kSRAMC_WaitActiveHigh
						      : kSRAMC_WaitActiveLow;

	SRAMC_Init(cfg->base, &sramc_cfg);
}

static int memc_nxp_ahb_sramc_init(const struct device *dev)
{
	const struct memc_nxp_ahb_sramc_config *cfg = dev->config;
	uint32_t hclk_hz;
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

	ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &hclk_hz);
	if (ret != 0) {
		LOG_ERR("cannot read the functional clock rate (%d)", ret);
		return ret;
	}

	if (hclk_hz == 0U) {
		LOG_ERR("functional clock rate is zero");
		return -EINVAL;
	}

	for (uint8_t i = 0U; i < cfg->cs_count; i++) {
		const struct memc_nxp_ahb_sramc_cs *cs = &cfg->cs[i];
		uint32_t values[SRAMC_TIMING_COUNT];
		uint32_t prescaler;

		ret = sramc_cs_resolve_timings(hclk_hz, cs, &prescaler, values);
		if (ret != 0) {
			LOG_ERR("CS%u timings are out of range at %u Hz", cs->chip_select,
				hclk_hz);
			return ret;
		}

		sramc_cs_apply(cfg, cs, prescaler, values);

		LOG_DBG("CS%u ready: %p, %llu bytes, granularity %u clocks", cs->chip_select,
			cs->mem_base, cs->mem_size, prescaler + 1U);
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("cannot apply the pin configuration (%d)", ret);
		return ret;
	}

	return 0;
}

/*
 * The MEMC API describes one memory per device, so these report the first
 * chip select declared in devicetree, which is what memc_read() and
 * memc_write() then go through. A second populated chip select is reached
 * through the linker symbols of its own memory-region instead.
 */
static void *memc_nxp_ahb_sramc_get_mem_base(const struct device *dev)
{
	const struct memc_nxp_ahb_sramc_config *cfg = dev->config;

	return cfg->cs[0].mem_base;
}

static int memc_nxp_ahb_sramc_get_size(const struct device *dev, uint64_t *size)
{
	const struct memc_nxp_ahb_sramc_config *cfg = dev->config;

	*size = cfg->cs[0].mem_size;

	return 0;
}

static DEVICE_API(memc, memc_nxp_ahb_sramc_api) = {
	.get_mem_base = memc_nxp_ahb_sramc_get_mem_base,
	.get_size = memc_nxp_ahb_sramc_get_size,
};

/* RM 30.3.2: the controller implements at most 128 KB per chip select. */
#define SRAMC_MAX_WINDOW_SIZE (128U * 1024U)

#define SRAMC_CS_ASSERT(node)                                                                      \
	BUILD_ASSERT(DT_REG_ADDR(node) < AHB_SRAMC_CFG0_CS_COUNT,                                  \
		     "chip-select index is beyond what this SoC implements");                      \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node, wait_pin) || DT_NODE_HAS_PROP(node, wait_mode),       \
		     "wait-mode is required on a chip select that sets wait-pin");                 \
	BUILD_ASSERT(DT_REG_SIZE(DT_PHANDLE(node, memory_region)) <= SRAMC_MAX_WINDOW_SIZE,        \
		     "a chip-select range holds at most 128 KB (RM 30.3.2)");

#define SRAMC_CS_CONFIG(node)                                                                      \
	{                                                                                          \
		.timing_ns = {                                                                     \
			[SRAMC_TIMING_CE_SETUP] = DT_PROP(node, ce_setup_time_ns),                 \
			[SRAMC_TIMING_CE_HOLD] = DT_PROP(node, ce_hold_time_ns),                   \
			[SRAMC_TIMING_ADDRESS_SETUP] = DT_PROP(node, address_setup_time_ns),       \
			[SRAMC_TIMING_ADDRESS_HOLD] = DT_PROP(node, address_hold_time_ns),         \
			[SRAMC_TIMING_TURNAROUND] = DT_PROP(node, turnaround_time_ns),             \
			[SRAMC_TIMING_WE_LOW] = DT_PROP(node, write_enable_low_time_ns),           \
			[SRAMC_TIMING_WE_HIGH] = DT_PROP(node, write_enable_high_time_ns),         \
			[SRAMC_TIMING_RE_LOW] = DT_PROP(node, read_enable_low_time_ns),            \
			[SRAMC_TIMING_RE_HIGH] = DT_PROP(node, read_enable_high_time_ns),          \
		},                                                                                 \
		.mem_base = (void *)DT_REG_ADDR(DT_PHANDLE(node, memory_region)),                  \
		.mem_size = (uint64_t)DT_REG_SIZE(DT_PHANDLE(node, memory_region)),                \
		.chip_select = DT_REG_ADDR(node),                                                  \
		.wait_mode = DT_PROP_OR(node, wait_mode, 0),                                       \
		.wait_pin = DT_PROP_OR(node, wait_pin, 0),                                         \
		.wait_enable = DT_NODE_HAS_PROP(node, wait_pin),                                   \
		.wait_active_high = DT_PROP(node, wait_active_high),                               \
	},

/* Two chip selects must not name the same chip-select index. */
#define SRAMC_CS_BIT(node) | BIT(DT_REG_ADDR(node))

#define SRAMC_CS_DECLARED_COUNT(inst)                                                              \
	POPCOUNT(0 DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SRAMC_CS_BIT))

#define MEMC_NXP_AHB_SRAMC_DEFINE(inst)                                                            \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(inst) > 0,                                      \
		     "nxp,ahb-sramc needs one enabled child node per populated chip select");      \
	BUILD_ASSERT(SRAMC_CS_DECLARED_COUNT(inst) == DT_INST_CHILD_NUM_STATUS_OKAY(inst),         \
		     "two chip selects share one chip-select index");                              \
												   \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SRAMC_CS_ASSERT)                                   \
												   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
												   \
	static const struct memc_nxp_ahb_sramc_cs memc_nxp_ahb_sramc_cs_##inst[] = {               \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SRAMC_CS_CONFIG)                           \
	};                                                                                         \
												   \
	static const struct memc_nxp_ahb_sramc_config memc_nxp_ahb_sramc_cfg_##inst = {            \
		.base = (AHB_SRAMC_Type *)DT_INST_REG_ADDR(inst),                                  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.cs = memc_nxp_ahb_sramc_cs_##inst,                                                \
		.cs_count = (uint8_t)ARRAY_SIZE(memc_nxp_ahb_sramc_cs_##inst),                     \
		.port_size = DT_INST_ENUM_IDX(inst, port_size),                                    \
		.address_mode = DT_INST_ENUM_IDX(inst, address_mode),                              \
		.adv_polarity = DT_INST_ENUM_IDX(inst, adv_polarity),                              \
		.bus_timeout = DT_INST_PROP(inst, bus_timeout),                                    \
		.func_clock_always_on = DT_INST_PROP(inst, function_clock_always_on),              \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(inst, memc_nxp_ahb_sramc_init, NULL, NULL,                           \
			      &memc_nxp_ahb_sramc_cfg_##inst, POST_KERNEL,                         \
			      CONFIG_MEMC_INIT_PRIORITY, &memc_nxp_ahb_sramc_api);

DT_INST_FOREACH_STATUS_OKAY(MEMC_NXP_AHB_SRAMC_DEFINE)
