/*
 * Copyright (c) 2025 Realtek Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_kbd

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/irq.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/sys/util_macro.h>
#include "reg/reg_kbm.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_realtek_rts5912_kbd, CONFIG_INPUT_LOG_LEVEL);

struct rts5912_kbd_config {
	struct input_kbd_matrix_common_config common;
	volatile struct kbm_regs *base;
	uint32_t irq;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct rts5912_sccon_subsys sccon_cfg;
	uint32_t kso_ignore_mask;
};

struct rts5912_kbd_data {
	struct input_kbd_matrix_common_data common;
};

INPUT_KBD_STRUCT_CHECK(struct rts5912_kbd_config, struct rts5912_kbd_data);

static void rts5912_kbd_drive_column(const struct device *dev, int col)
{
	const struct rts5912_kbd_config *config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	volatile struct kbm_regs *inst = config->base;
	const uint32_t kso_mask = BIT_MASK(common->col_size) & ~config->kso_ignore_mask;
	uint32_t kso_val;
	uint32_t key;

	if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		kso_val = kso_mask;
	} else if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		kso_val = 0;
	} else {
		kso_val = kso_mask ^ BIT(col);
	}

	key = irq_lock();
	inst->scan_out = kso_val;
	irq_unlock(key);
}

static kbd_row_t rts5912_kbd_read_row(const struct device *dev)
{
	const struct rts5912_kbd_config *config = dev->config;
	volatile struct kbm_regs *inst = config->base;
	const struct input_kbd_matrix_common_config *common = &config->common;
	const uint32_t ksi_mask = BIT_MASK(common->row_size);

	return (inst->scan_in ^ ksi_mask);
}

static void rts5912_intc_isr_clear(const struct device *dev)
{
	const struct rts5912_kbd_config *config = dev->config;
	volatile struct kbm_regs *inst = config->base;

	inst->ctrl |= KBM_CTRL_KSIINTSTS_Msk;
}

static void rts5912_kbd_isr(const struct device *dev)
{
	rts5912_intc_isr_clear(dev);
	input_kbd_matrix_poll_start(dev);
}

static void rts5912_kbd_set_detect_mode(const struct device *dev, bool enable)
{
	const struct rts5912_kbd_config *config = dev->config;

	if (enable) {
		rts5912_intc_isr_clear(dev);

		irq_enable(config->irq);
	} else {
		irq_disable(config->irq);
	}
}

static int rts5912_kbd_init(const struct device *dev)
{
	const struct rts5912_kbd_config *config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;

	volatile struct kbm_regs *inst = config->base;

	const uint32_t kso_mask = BIT_MASK(common->col_size) & ~config->kso_ignore_mask;
	const uint32_t ksi_mask = BIT_MASK(common->row_size);

	int ret;

	rts5912_kbd_set_detect_mode(dev, false);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure KSI and KSO pins: %d", ret);
		return ret;
	}

	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("clock kbd device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t)&config->sccon_cfg);
	if (ret != 0) {
		LOG_ERR("kbd clock power on fail: %d", ret);
		return ret;
	}

	inst->scan_out = 0x00;

	if (ksi_mask & BIT(8)) {
		inst->ctrl |= KBM_CTRL_KSI8EN_Msk;
	}

	if (ksi_mask & BIT(9)) {
		inst->ctrl |= KBM_CTRL_KSI9EN_Msk;
	}

	if (kso_mask & BIT(18)) {
		inst->ctrl |= KBM_CTRL_KSO18EN_Msk;
	}

	if (kso_mask & BIT(19)) {
		inst->ctrl |= KBM_CTRL_KSO19EN_Msk;
	}

	inst->ctrl |= KBM_CTRL_KSOTYPE_Msk;

	inst->int_en |= ksi_mask;

	rts5912_intc_isr_clear(dev);

	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), rts5912_kbd_isr,
		    DEVICE_DT_INST_GET(0), 0);

	return input_kbd_matrix_common_init(dev);
}

#if defined(CONFIG_PM_DEVICE)
static int input_kbd_matrix_pm_action_suspend(const struct device *dev)
{
	const struct rts5912_kbd_config *config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	volatile struct kbm_regs *inst = config->base;
	const uint32_t kso_mask = BIT_MASK(common->col_size) & ~config->kso_ignore_mask;
	int ret;

	ret = clock_control_off(config->clk_dev, (clock_control_subsys_t)&config->sccon_cfg);
	if (ret != 0) {
		LOG_ERR("clock_control_off failed: %d", ret);
		return ret;
	}
	inst->int_en = 0;

	rts5912_intc_isr_clear(dev);

	if (kso_mask & BIT(18)) {
		inst->ctrl &= ~KBM_CTRL_KSO18EN_Msk;
	}

	if (kso_mask & BIT(19)) {
		inst->ctrl &= ~KBM_CTRL_KSO19EN_Msk;
	}

	inst->scan_out = 0x00;
	inst->ctrl &= ~KBM_CTRL_KSOTYPE_Msk;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret < 0) {
		LOG_ERR("pinctrl_apply_state failed: %d", ret);
		return ret;
	}

	return 0;
}

static int input_kbd_matrix_pm_action_resume(const struct device *dev)
{
	const struct rts5912_kbd_config *config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	volatile struct kbm_regs *inst = config->base;
	const uint32_t kso_mask = BIT_MASK(common->col_size) & ~config->kso_ignore_mask;
	const uint32_t ksi_mask = BIT_MASK(common->row_size);
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("pinctrl_apply_state failed: %d", ret);
		return ret;
	}
	inst->ctrl |= KBM_CTRL_KSOTYPE_Msk;

	inst->scan_out = 0x00;

	if (kso_mask & BIT(18)) {
		inst->ctrl |= KBM_CTRL_KSO18EN_Msk;
	}

	if (kso_mask & BIT(19)) {
		inst->ctrl |= KBM_CTRL_KSO19EN_Msk;
	}
	inst->int_en |= ksi_mask;
	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t)&config->sccon_cfg);
	if (ret != 0) {
		LOG_ERR("clock_control_on failed: %d", ret);
		return ret;
	}

	return 0;
}

static int input_kbd_matrix_pm_action_rts5912(const struct device *dev,
					      enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = input_kbd_matrix_pm_action_resume(dev);
		if (ret != 0) {
			LOG_ERR("kbd rts5912 resume fail: %d", ret);
			return ret;
		}
		ret = input_kbd_matrix_pm_action(dev, action);
		if (ret != 0) {
			LOG_ERR("kbd pm resume fail: %d", ret);
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = input_kbd_matrix_pm_action_suspend(dev);
		if (ret != 0) {
			LOG_ERR("kbd rts5912 suspend fail: %d", ret);
			return ret;
		}
		ret = input_kbd_matrix_pm_action(dev, action);
		if (ret != 0) {
			LOG_ERR("kbd pm suspend fail: %d", ret);
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif
PINCTRL_DT_INST_DEFINE(0);

INPUT_KBD_MATRIX_DT_INST_DEFINE(0);

static const struct input_kbd_matrix_api rts5912_kbd_api = {
	.drive_column = rts5912_kbd_drive_column,
	.read_row = rts5912_kbd_read_row,
	.set_detect_mode = rts5912_kbd_set_detect_mode,
};

static const struct rts5912_kbd_config rts5912_kbd_cfg_0 = {
	.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(0, &rts5912_kbd_api),
	.base = (volatile struct kbm_regs *)DT_INST_REG_ADDR_BY_IDX(0, 0),
	.irq = DT_INST_IRQN(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.sccon_cfg = {
		.clk_grp = DT_CLOCKS_CELL(DT_NODELABEL(kbd), clk_grp),
		.clk_idx = DT_CLOCKS_CELL(DT_NODELABEL(kbd), clk_idx),
	},
	.kso_ignore_mask = DT_INST_PROP_OR(0, kso_ignore_mask, 0x00),
};

static struct rts5912_kbd_data rts5912_kbd_data_0;

PM_DEVICE_DT_INST_DEFINE(0, input_kbd_matrix_pm_action_rts5912);

DEVICE_DT_INST_DEFINE(0, &rts5912_kbd_init, PM_DEVICE_DT_INST_GET(0), &rts5912_kbd_data_0,
		      &rts5912_kbd_cfg_0, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE_SYSTEM_MANAGED) || IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME),
	     "CONFIG_PM_DEVICE_RUNTIME must be enabled when using CONFIG_PM_DEVICE_SYSTEM_MANAGED");

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one realtek,rts5912-kbd compatible node can be supported");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, row_size), 1, 9), "invalid row-size");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, col_size), 1, 19), "invalid col-size");
