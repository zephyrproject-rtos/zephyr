/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <soc.h>

#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(memc_max32_hpb, CONFIG_MEMC_LOG_LEVEL);

#include <hpb.h>
#include <emcc.h>

#define DT_DRV_COMPAT adi_max32_hpb

struct memc_max32_hpb_config {
	const struct device *clock;
	const struct pinctrl_dev_config *pcfg;
};

struct memc_max32_hpb_mem_config {
	uint8_t reg;

	mxc_hpb_mem_config_t config;
};

/* clang-format off */

#define MEM_CONFIG(n)                                                                              \
	{                                                                                          \
		.reg = DT_REG_ADDR(n),                                                             \
		.config = {.device_type = DT_PROP(n, device_type),                                 \
			   .base_addr = DT_PROP(n, base_address),                                  \
			   .latency_cycle = DT_PROP_OR(n, latency_cycles, 1),                      \
			   .write_cs_high = DT_PROP_OR(n, write_cs_high, 0),                       \
			   .read_cs_high = DT_PROP_OR(n, read_cs_high, 0),                         \
			   .write_cs_hold = DT_PROP_OR(n, write_cs_hold, 0),                       \
			   .read_cs_hold = DT_PROP_OR(n, read_cs_hold, 0),                         \
			   .write_cs_setup = DT_PROP_OR(n, write_cs_setup, 0),                     \
			   .read_cs_setup = DT_PROP_OR(n, read_cs_setup, 0),                       \
			   .fixed_latency = DT_PROP_OR(n, fixed_read_latency, 0),                  \
			   COND_CODE_1(DT_NODE_HAS_PROP(n, config_regs),                           \
				(.cfg_reg_val = config_regs_##n,                                   \
				 .cfg_reg_val_len = ARRAY_SIZE(config_regs_##n)), ()) },           \
	}

/* clang-format on */

#define CR_ENTRY(idx, n)                                                                           \
	{                                                                                          \
		.addr = DT_PROP_BY_IDX(n, config_regs, idx),                                       \
		.val = DT_PROP_BY_IDX(n, config_reg_vals, idx),                                    \
	}

#define MEM_CR_ENTRIES(n)                                                                          \
	COND_CODE_1(DT_NODE_HAS_PROP(n, config_regs), (                                            \
		BUILD_ASSERT(DT_PROP_LEN(n, config_regs) == DT_PROP_LEN(n, config_reg_vals),       \
		   "The config-regs and config-reg-vals properties of adi,max32-hpb memory device" \
		   " child nodes must be the same length");                                        \
		static const mxc_hpb_cfg_reg_val_t config_regs_##n[] =                             \
		    { LISTIFY(DT_PROP_LEN(n, config_regs), CR_ENTRY, (,), n) };                    \
	), ())

/** memory device configuration(s). */
DT_INST_FOREACH_CHILD(0, MEM_CR_ENTRIES)

/* clang-format off */

static const struct memc_max32_hpb_mem_config mem_configs[] = {
	DT_INST_FOREACH_CHILD_SEP(0, MEM_CONFIG, (,))};

#define CLOCK_CFG(node_id, prop, idx)                                                              \
	{.bus = DT_CLOCKS_CELL_BY_IDX(node_id, idx, offset),                                       \
	 .bit = DT_CLOCKS_CELL_BY_IDX(node_id, idx, bit)}

static const struct max32_perclk perclks[] = {
	DT_INST_FOREACH_PROP_ELEM_SEP(0, clocks, CLOCK_CFG, (,))};

/* clang-format on */
static int memc_max32_hpb_init(const struct device *dev)
{
	const struct memc_max32_hpb_config *config = dev->config;

	int r;
	const mxc_hpb_mem_config_t *mem0 = NULL, *mem1 = NULL;

	if (!device_is_ready(config->clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(perclks); i++) {
		r = clock_control_on(config->clock, (clock_control_subsys_t)&perclks[i]);
		if (r < 0) {
			LOG_ERR("Could not initialize HPB clock (%d)", r);
			return r;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(mem_configs); i++) {
		if (mem_configs[i].reg == 0) {
			mem0 = &mem_configs[i].config;
		} else if (mem_configs[i].reg == 1) {
			mem1 = &mem_configs[i].config;
		}
	}

	/* configure pinmux */
	r = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (r < 0) {
		LOG_ERR("HPB pinctrl setup failed (%d)", r);
		return r;
	}

	r = MXC_HPB_Init(mem0, mem1);
	if (r < 0) {
		LOG_ERR("HPB init failed (%d)", r);
		return r;
	}

	COND_CODE_1(DT_INST_PROP(0, enable_emcc), (MXC_EMCC_Enable()), (MXC_EMCC_Disable()));

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

static const struct memc_max32_hpb_config config = {
	.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, memc_max32_hpb_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_MEMC_INIT_PRIORITY, NULL);
