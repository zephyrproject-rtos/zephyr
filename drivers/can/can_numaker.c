/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_canfd

#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <NuMicro.h>

LOG_MODULE_REGISTER(can_numaker, CONFIG_CAN_LOG_LEVEL);

/* Implementation notes
 *
 * 1. Use Bosch M_CAN driver (m_can) as backend
 * 2. For new SoC series port, add CAN in clock_control_get_rate()
 */

struct can_numaker_config {
	mm_reg_t canfd_base;
	mem_addr_t mrba;
	mem_addr_t mram;
	const struct reset_dt_spec reset;
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clk_dev;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
};

static int can_numaker_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_numaker_config *config = mcan_config->custom;
	struct numaker_scc_subsys scc_subsys = {.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC,
						.pcc.clk_modidx = config->clk_modidx};
	int rc;

	rc = clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)&scc_subsys, rate);
	if (rc < 0) {
		LOG_ERR("Failed clock_control_get_rate(): %d", rc);
		return rc;
	}

	return 0;
}

static inline int can_numaker_init_unlocked(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_numaker_config *config = mcan_config->custom;
	struct numaker_scc_subsys scc_subsys;
	int rc;

	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = config->clk_modidx;
	scc_subsys.pcc.clk_src = config->clk_src;
	scc_subsys.pcc.clk_div = config->clk_div;

	/* To enable clock */
	rc = clock_control_on(config->clk_dev, (clock_control_subsys_t)&scc_subsys);
	if (rc < 0) {
		return rc;
	}
	/* To set module clock */
	rc = clock_control_configure(config->clk_dev, (clock_control_subsys_t)&scc_subsys, NULL);
	if (rc < 0) {
		return rc;
	}

	/* Configure pinmux (NuMaker's SYS MFP) */
	rc = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}

	/* Reset CAN to default state, same as BSP's SYS_ResetModule(id_rst) */
	reset_line_toggle_dt(&config->reset);

	config->irq_config_func(dev);

	rc = can_mcan_configure_mram(dev, config->mrba, config->mram);
	if (rc != 0) {
		return rc;
	}

	rc = can_mcan_init(dev);
	if (rc < 0) {
		LOG_ERR("Failed to initialize mcan: %d", rc);
		return rc;
	}

#if CONFIG_CAN_LOG_LEVEL >= LOG_LEVEL_DBG
	uint32_t rate;

	rc = can_numaker_get_core_clock(dev, &rate);
	if (rc < 0) {
		return rc;
	}

	LOG_DBG("CAN core clock: %d", rate);
#endif

	return rc;
}

static int can_numaker_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_numaker_config *config = mcan_config->custom;
	int rc;

	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("clock controller not ready");
		return -ENODEV;
	}

	SYS_UnlockReg();
	rc = can_numaker_init_unlocked(dev);
	SYS_LockReg();

	return rc;
}

static DEVICE_API(can, can_numaker_driver_api) = {
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
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.get_core_clock = can_numaker_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static int can_numaker_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_numaker_config *numaker_cfg = mcan_cfg->custom;

	return can_mcan_sys_read_reg(numaker_cfg->canfd_base, reg, val);
}

static int can_numaker_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_numaker_config *numaker_cfg = mcan_cfg->custom;

	return can_mcan_sys_write_reg(numaker_cfg->canfd_base, reg, val);
}

static int can_numaker_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_numaker_config *numaker_cfg = mcan_cfg->custom;

	return can_mcan_sys_read_mram(numaker_cfg->mram, offset, dst, len);
}

static int can_numaker_write_mram(const struct device *dev, uint16_t offset, const void *src,
				  size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_numaker_config *numaker_cfg = mcan_cfg->custom;

	return can_mcan_sys_write_mram(numaker_cfg->mram, offset, src, len);
}

static int can_numaker_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_numaker_config *numaker_cfg = mcan_cfg->custom;

	return can_mcan_sys_clear_mram(numaker_cfg->mram, offset, len);
}

static const struct can_mcan_ops can_numaker_ops = {
	.read_reg = can_numaker_read_reg,
	.write_reg = can_numaker_write_reg,
	.read_mram = can_numaker_read_mram,
	.write_mram = can_numaker_write_mram,
	.clear_mram = can_numaker_clear_mram,
};

#define CAN_NUMAKER_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(inst, can_numaker_cbs_##inst);                           \
                                                                                                   \
	static void can_numaker_irq_config_func_##inst(const struct device *dev)                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, int0, irq),                                  \
			    DT_INST_IRQ_BY_NAME(inst, int0, priority), can_mcan_line_0_isr,        \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, int0, irq));                                  \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, int1, irq),                                  \
			    DT_INST_IRQ_BY_NAME(inst, int1, priority), can_mcan_line_1_isr,        \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, int1, irq));                                  \
	}                                                                                          \
                                                                                                   \
	static const struct can_numaker_config can_numaker_config_##inst = {                       \
		.canfd_base = CAN_MCAN_DT_INST_MCAN_ADDR(inst),                                    \
		.mrba = CAN_MCAN_DT_INST_MRBA(inst),                                               \
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(inst),                                          \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),                       \
		.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),                                \
		.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),                               \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                    \
		.irq_config_func = can_numaker_irq_config_func_##inst,                             \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
	};                                                                                         \
                                                                                                   \
	static const struct can_mcan_config can_mcan_config_##inst = CAN_MCAN_DT_CONFIG_INST_GET(  \
		inst, &can_numaker_config_##inst, &can_numaker_ops, &can_numaker_cbs_##inst);      \
                                                                                                   \
	static uint32_t can_numaker_data_##inst;                                                   \
                                                                                                   \
	static struct can_mcan_data can_mcan_data_##inst =                                         \
		CAN_MCAN_DATA_INITIALIZER(&can_numaker_data_##inst);                               \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_numaker_init, NULL, &can_mcan_data_##inst,             \
				  &can_mcan_config_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,  \
				  &can_numaker_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_NUMAKER_INIT);
