/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <aurix_endinit.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_infineon_aurix, CONFIG_CAN_LOG_LEVEL);

#define MCMCAN_CLC 0x00

#if CONFIG_SOC_SERIES_TC3X
#define MCMCAN_MCR     0x30
#define MCMCAN_N_INTR0 0x14
#define MCMCAN_N_INTR1 0x18
#define MCMCAN_N_NPCR  0x40
#elif CONFIG_SOC_SERIES_TC4X
#define MCMCAN_MCR     0x70
#define MCMCAN_N_INTR0 0x2C
#define MCMCAN_N_INTR1 0x30
#define MCMCAN_N_NPCR  0x4C
#endif

#define MCMCAN_NODE(n)   (0x100 + (n) * 0x400)
#define MCMCAN_NODE_MCAN 0x100
#define MCMCAN_NODES     4

#define CLC_DISR BIT(0)
#define CLC_DISS BIT(1)

#define MCR_CLKSEL(n)   (GENMASK(1, 0) << ((n) * 2))
#define MCR_RBUSY       BIT(28)
#define MCR_RINIT       BIT(29)
#define MCR_CI          BIT(30)
#define MCR_CCCE        BIT(31)
#define MCR_CLKSEL_BOTH 3

#define NPCR_RXSEL GENMASK(2, 0)

#define INTR1_RX_MASK GENMASK(19, 4)

#define MCMCAN_TIMEOUT_US 1000

struct can_aurix_module_config {
	mem_addr_t base;
	const struct device *clock_dev;
	clock_control_subsys_t mcan_clk;
	clock_control_subsys_t mcanh_clk;
};

struct can_aurix_config {
	mm_reg_t base;
	mem_addr_t mrba;
	mem_addr_t mram;
	const struct device *module;
	uint8_t node;
	uint8_t line0;
	uint8_t line1;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

static int can_aurix_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_aurix_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_read_reg(cfg->base, reg, val);
}

static int can_aurix_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_aurix_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_write_reg(cfg->base, reg, val);
}

static int can_aurix_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_aurix_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_read_mram(cfg->mram, offset, dst, len);
}

static int can_aurix_write_mram(const struct device *dev, uint16_t offset, const void *src,
				size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_aurix_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_write_mram(cfg->mram, offset, src, len);
}

static int can_aurix_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_aurix_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_clear_mram(cfg->mram, offset, len);
}

static int can_aurix_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_aurix_config *cfg = mcan_cfg->custom;
	const struct can_aurix_module_config *mod = cfg->module->config;

	return clock_control_get_rate(mod->clock_dev, mod->mcan_clk, rate);
}

static bool mcr_ready(mem_addr_t mcr)
{
	return WAIT_FOR((sys_read32(mcr) & MCR_RBUSY) == 0U, MCMCAN_TIMEOUT_US, k_busy_wait(1));
}

/* CLKSELn and RINIT take writes only while CCCE and CI are already set */
static void mcr_write(mem_addr_t mcr, uint32_t val)
{
	val &= ~(MCR_CCCE | MCR_CI);
	sys_write32(sys_read32(mcr) | MCR_CCCE | MCR_CI, mcr);
	sys_write32(val | MCR_CCCE | MCR_CI, mcr);
	sys_write32(val, mcr);
}

static int can_aurix_node_setup(const struct can_aurix_config *cfg)
{
	const struct can_aurix_module_config *mod = cfg->module->config;
	mem_addr_t node = mod->base + MCMCAN_NODE(cfg->node);
	mem_addr_t mcr = mod->base + MCMCAN_MCR;
	const struct pinctrl_state *state;
	uint32_t intr1 = 0;
	int rxsel = -1;
	int ret;

	ret = pinctrl_lookup_state(cfg->pcfg, PINCTRL_STATE_DEFAULT, &state);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t i = 0; i < state->pin_cnt; i++) {
		if (!state->pins[i].output) {
			rxsel = AURIX_PINMUX_FUNC(state->pins[i].pinmux);
		}
	}

	if (rxsel < 0) {
		LOG_ERR("no receive pin in pinctrl-0");
		return -EINVAL;
	}

	for (uint32_t shift = 0; shift < 32; shift += 4) {
		intr1 |= (uint32_t)(((BIT(shift) & INTR1_RX_MASK) != 0U) ? cfg->line1 : cfg->line0)
			 << shift;
	}

	sys_write32(0x11111111U * cfg->line0, node + MCMCAN_N_INTR0);
	sys_write32(intr1, node + MCMCAN_N_INTR1);
	sys_write32(FIELD_PREP(NPCR_RXSEL, rxsel), node + MCMCAN_N_NPCR);

	if (!mcr_ready(mcr)) {
		return -ETIMEDOUT;
	}

	mcr_write(mcr, (sys_read32(mcr) & ~MCR_CLKSEL(cfg->node)) |
			       (MCR_CLKSEL_BOTH << (cfg->node * 2)));

	return 0;
}

static int can_aurix_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_aurix_config *cfg = mcan_cfg->custom;
	int ret;

	if (!device_is_ready(cfg->module)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->module);
		return -ENODEV;
	}

	ret = can_aurix_node_setup(cfg);
	if (ret != 0) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_configure_mram(dev, cfg->mrba, cfg->mram);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		LOG_ERR("failed to initialize M_CAN (%d)", ret);
		return ret;
	}

	cfg->irq_config_func();

	return 0;
}

static DEVICE_API(can, can_aurix_driver_api) = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_state = can_mcan_get_state,
	.get_core_clock = can_aurix_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops can_aurix_ops = {
	.read_reg = can_aurix_read_reg,
	.write_reg = can_aurix_write_reg,
	.read_mram = can_aurix_read_mram,
	.write_mram = can_aurix_write_mram,
	.clear_mram = can_aurix_clear_mram,
};

#define DT_DRV_COMPAT infineon_aurix_mcmcan_node

#define CAN_AURIX_NODE(n)                                                                          \
	((CAN_MCAN_DT_INST_MCAN_ADDR(n) - DT_REG_ADDR(DT_INST_PARENT(n)) - MCMCAN_NODE_MCAN -      \
	  MCMCAN_NODE(0)) /                                                                        \
	 (MCMCAN_NODE(1) - MCMCAN_NODE(0)))

#define CAN_AURIX_IRQ_CONNECT(n, name, isr)                                                        \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, name, irq), DT_INST_IRQ_BY_NAME(n, name, priority),     \
		    isr, DEVICE_DT_INST_GET(n), 0);                                                \
	irq_enable(DT_INST_IRQ_BY_NAME(n, name, irq))

#define CAN_AURIX_INIT(n)                                                                          \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(n);                                                 \
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(n) <= CAN_MCAN_DT_INST_MRAM_SIZE(n),      \
		     "Insufficient Message RAM size to hold elements");                            \
	BUILD_ASSERT(CAN_AURIX_NODE(n) < MCMCAN_NODES, "m_can register address out of range");     \
	BUILD_ASSERT(DT_INST_IRQ_BY_NAME(n, line1, irq) == DT_INST_IRQ_BY_NAME(n, line0, irq) + 1, \
		     "line1 must follow line0");                                                   \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(n, can_aurix_cbs_##n);                                   \
                                                                                                   \
	static void can_aurix_irq_config_##n(void)                                                 \
	{                                                                                          \
		CAN_AURIX_IRQ_CONNECT(n, line0, can_mcan_line_0_isr);                              \
		CAN_AURIX_IRQ_CONNECT(n, line1, can_mcan_line_1_isr);                              \
	}                                                                                          \
                                                                                                   \
	static const struct can_aurix_config can_aurix_cfg_##n = {                                 \
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(n),                                             \
		.mrba = CAN_MCAN_DT_INST_MRBA(n),                                                  \
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(n),                                             \
		.module = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
		.node = CAN_AURIX_NODE(n),                                                         \
		.line0 = 2 * CAN_AURIX_NODE(n),                                                    \
		.line1 = 2 * CAN_AURIX_NODE(n) + 1,                                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = can_aurix_irq_config_##n,                                       \
	};                                                                                         \
                                                                                                   \
	static const struct can_mcan_config can_mcan_cfg_##n = CAN_MCAN_DT_CONFIG_INST_GET(        \
		n, &can_aurix_cfg_##n, &can_aurix_ops, &can_aurix_cbs_##n);                        \
                                                                                                   \
	CAN_MCAN_DATA_DEFINE(can_mcan_data_##n, NULL);                                             \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(n, can_aurix_init, NULL, &can_mcan_data_##n, &can_mcan_cfg_##n,  \
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY, &can_aurix_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_AURIX_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT infineon_aurix_mcmcan

static int can_aurix_module_init(const struct device *dev)
{
	const struct can_aurix_module_config *cfg = dev->config;
	mem_addr_t mcr = cfg->base + MCMCAN_MCR;
	unsigned int key;
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->mcan_clk);
	if (ret == 0) {
		ret = clock_control_on(cfg->clock_dev, cfg->mcanh_clk);
	}
	if (ret != 0) {
		return ret;
	}

	key = aurix_endinit_cpu_unlock();
	sys_write32(sys_read32(cfg->base + MCMCAN_CLC) & ~CLC_DISR, cfg->base + MCMCAN_CLC);
	aurix_endinit_cpu_lock(key);

	if (!WAIT_FOR((sys_read32(cfg->base + MCMCAN_CLC) & CLC_DISS) == 0U, MCMCAN_TIMEOUT_US,
		      k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	if (!mcr_ready(mcr)) {
		return -ETIMEDOUT;
	}

	mcr_write(mcr, sys_read32(mcr) & ~MCR_RINIT);
	mcr_write(mcr, sys_read32(mcr) | MCR_RINIT);

	return mcr_ready(mcr) ? 0 : -ETIMEDOUT;
}

#define CAN_AURIX_MODULE_INIT(n)                                                                   \
	static const struct can_aurix_module_config can_aurix_module_cfg_##n = {                   \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.mcan_clk = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mcan, id),      \
		.mcanh_clk = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mcanh, id),    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, can_aurix_module_init, NULL, NULL, &can_aurix_module_cfg_##n,     \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(CAN_AURIX_MODULE_INIT)
