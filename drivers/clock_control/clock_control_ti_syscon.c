/*
 * Copyright 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_AM654_COMPAT ti_am654_ehrpwm_tbclk
#define DT_AM64_COMPAT  ti_am64_epwm_tbclk
#define DT_AM62_COMPAT  ti_am62_epwm_tbclk

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ti_syscon_gate_clk, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DEV_CFG(dev) ((struct ti_syscon_gate_clk_cfg *)(dev)->config)

struct ti_syscon_gate_clk_id_data {
	uint32_t offset;
	uint32_t bit;
};

#if DT_HAS_COMPAT_STATUS_OKAY(DT_AM64_COMPAT)
static const struct ti_syscon_gate_clk_id_data am64_clk_ids[] = {
	{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8},
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(DT_AM654_COMPAT)
static const struct ti_syscon_gate_clk_id_data am654_clk_ids[] = {
	{0x0, 0}, {0x4, 0}, {0x8, 0}, {0xc, 0}, {0x10, 0}, {0x14, 0},
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(DT_AM62_COMPAT)
static const struct ti_syscon_gate_clk_id_data am62_clk_ids[] = {
	{0, 0},
	{0, 1},
	{0, 2},
};
#endif

struct ti_syscon_gate_clk_cfg {
	mm_reg_t reg;
	const struct device *syscon;
	const struct ti_syscon_gate_clk_id_data *clk_ids;
	const uint32_t num_clk_ids;
};

static int ti_syscon_gate_clk_enable(const struct device *dev, clock_control_subsys_t sub_system,
				     bool enable)
{
	const struct ti_syscon_gate_clk_cfg *cfg = DEV_CFG(dev);
	uint32_t clk_id = (sub_system ? (uint32_t)sub_system : 0);
	uint32_t reg;
	uint32_t bit;
	uint32_t val;
	uint32_t rb;
	int err;

	if (clk_id >= cfg->num_clk_ids) {
		LOG_ERR("invalid clk id");
		return -EINVAL;
	}

	reg = cfg->reg + cfg->clk_ids[clk_id].offset;
	bit = cfg->clk_ids[clk_id].bit;

	err = syscon_read_reg(cfg->syscon, reg, &val);
	if (err < 0) {
		LOG_ERR("failed to read syscon register");
		return err;
	}

	if (enable) {
		val |= BIT(bit);
	} else {
		val &= ~BIT(bit);
	}

	err = syscon_write_reg(cfg->syscon, reg, val);
	if (err < 0) {
		LOG_ERR("failed to write syscon register");
		return err;
	}

	err = syscon_read_reg(cfg->syscon, reg, &rb);
	if (err < 0) {
		LOG_ERR("failed to read syscon register");
		return err;
	}

	if (rb != val) {
		LOG_ERR("readback does not match written value");
		return -EIO;
	}

	return 0;
}

static int ti_syscon_gate_clk_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	return ti_syscon_gate_clk_enable(dev, sub_system, true);
}

static int ti_syscon_gate_clk_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	return ti_syscon_gate_clk_enable(dev, sub_system, false);
}

static DEVICE_API(clock_control, ti_syscon_gate_clk_driver_api) = {
	.on = ti_syscon_gate_clk_on,
	.off = ti_syscon_gate_clk_off,
};

#define TI_SYSCON_GATE_CLK_INIT(node, clks)                                                        \
	static const struct ti_syscon_gate_clk_cfg ti_syscon_gate_clk_config_##node = {            \
		.reg = DT_REG_ADDR(node),                                                          \
		.syscon = DEVICE_DT_GET(DT_PARENT(node)),                                          \
		.clk_ids = clks,                                                                   \
		.num_clk_ids = ARRAY_SIZE(clks),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node, NULL, NULL, NULL, &ti_syscon_gate_clk_config_##node, POST_KERNEL,   \
			 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ti_syscon_gate_clk_driver_api);

/* add more compats as required */

DT_FOREACH_STATUS_OKAY_VARGS(DT_AM654_COMPAT, TI_SYSCON_GATE_CLK_INIT, am654_clk_ids)
DT_FOREACH_STATUS_OKAY_VARGS(DT_AM64_COMPAT, TI_SYSCON_GATE_CLK_INIT, am64_clk_ids)
DT_FOREACH_STATUS_OKAY_VARGS(DT_AM62_COMPAT, TI_SYSCON_GATE_CLK_INIT, am62_clk_ids)
