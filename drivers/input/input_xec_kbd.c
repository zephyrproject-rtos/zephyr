/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2022 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_kbd

#include <cmsis_core.h>
#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#ifdef CONFIG_SOC_SERIES_MEC172X
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#endif

LOG_MODULE_REGISTER(input_xec_kbd, CONFIG_INPUT_LOG_LEVEL);

struct xec_kbd_config {
	struct input_kbd_matrix_common_config common;

	struct kscan_regs *regs;
	const struct pinctrl_dev_config *pcfg;
	uint8_t girq;
	uint8_t girq_pos;
#ifdef CONFIG_SOC_SERIES_MEC172X
	uint8_t pcr_idx;
	uint8_t pcr_pos;
#endif
	bool wakeup_source;
};

struct xec_kbd_data {
	struct input_kbd_matrix_common_data common;
	bool pm_lock_taken;
};

static void xec_kbd_clear_girq_status(const struct device *dev)
{
	struct xec_kbd_config const *cfg = dev->config;

#ifdef CONFIG_SOC_SERIES_MEC172X
	mchp_xec_ecia_girq_src_clr(cfg->girq, cfg->girq_pos);
#else
	MCHP_GIRQ_SRC(cfg->girq) = BIT(cfg->girq_pos);
#endif
}

static void xec_kbd_configure_girq(const struct device *dev)
{
	struct xec_kbd_config const *cfg = dev->config;

#ifdef CONFIG_SOC_SERIES_MEC172X
	mchp_xec_ecia_enable(cfg->girq, cfg->girq_pos);
#else
	MCHP_GIRQ_ENSET(cfg->girq) = BIT(cfg->girq_pos);
#endif
}

static void xec_kbd_clr_slp_en(const struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_MEC172X
	struct xec_kbd_config const *cfg = dev->config;

	z_mchp_xec_pcr_periph_sleep(cfg->pcr_idx, cfg->pcr_pos, 0);
#else
	ARG_UNUSED(dev);
	mchp_pcr_periph_slp_ctrl(PCR_KEYSCAN, 0);
#endif
}

static void xec_kbd_drive_column(const struct device *dev, int data)
{
	struct xec_kbd_config const *cfg = dev->config;
	struct kscan_regs *regs = cfg->regs;

	if (data == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		/* KSO output controlled by the KSO_SELECT field */
		regs->KSO_SEL = MCHP_KSCAN_KSO_ALL;
	} else if (data == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		/* Keyboard scan disabled. All KSO output buffers disabled */
		regs->KSO_SEL = MCHP_KSCAN_KSO_EN;
	} else {
		/* Assume, ALL was previously set */
		regs->KSO_SEL = data;
	}
}

static kbd_row_t xec_kbd_read_row(const struct device *dev)
{
	struct xec_kbd_config const *cfg = dev->config;
	struct kscan_regs *regs = cfg->regs;

	/* In this implementation a 1 means key pressed */
	return ~(regs->KSI_IN & 0xff);
}

static void xec_kbd_isr(const struct device *dev)
{
	xec_kbd_clear_girq_status(dev);
	irq_disable(DT_INST_IRQN(0));

	input_kbd_matrix_poll_start(dev);
}

static void xec_kbd_set_detect_mode(const struct device *dev, bool enabled)
{
	struct xec_kbd_config const *cfg = dev->config;
	struct xec_kbd_data *data = dev->data;
	struct kscan_regs *regs = cfg->regs;

	if (enabled) {
		if (data->pm_lock_taken) {
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE,
						 PM_ALL_SUBSTATES);
		}

		regs->KSI_STS = MCHP_KSCAN_KSO_SEL_REG_MASK;

		xec_kbd_clear_girq_status(dev);
		NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
		irq_enable(DT_INST_IRQN(0));
	} else {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE,
					 PM_ALL_SUBSTATES);
		data->pm_lock_taken = true;
	}
}

#ifdef CONFIG_PM_DEVICE
static int xec_kbd_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct xec_kbd_config const *cfg = dev->config;
	struct kscan_regs *regs = cfg->regs;
	int ret;

	ret = input_kbd_matrix_pm_action(dev, action);
	if (ret < 0) {
		return ret;
	}

	if (cfg->wakeup_source) {
		return 0;
	}

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			LOG_ERR("XEC KSCAN pinctrl init failed (%d)", ret);
			return ret;
		}

		regs->KSO_SEL &= ~BIT(MCHP_KSCAN_KSO_EN_POS);
		/* Clear status register */
		regs->KSI_STS = MCHP_KSCAN_KSO_SEL_REG_MASK;
		regs->KSI_IEN = MCHP_KSCAN_KSI_IEN_REG_MASK;
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		regs->KSO_SEL |= BIT(MCHP_KSCAN_KSO_EN_POS);
		regs->KSI_IEN = (~MCHP_KSCAN_KSI_IEN_REG_MASK);
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
		if (ret != -ENOENT) {
			/* pinctrl-1 does not exist */
			return ret;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int xec_kbd_init(const struct device *dev)
{
	struct xec_kbd_config const *cfg = dev->config;
	struct kscan_regs *regs = cfg->regs;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC KSCAN pinctrl init failed (%d)", ret);
		return ret;
	}

	xec_kbd_clr_slp_en(dev);

	/* Enable predrive */
	regs->KSO_SEL |= BIT(MCHP_KSCAN_KSO_EN_POS);
	regs->EXT_CTRL = MCHP_KSCAN_EXT_CTRL_PREDRV_EN;
	regs->KSO_SEL &= ~BIT(MCHP_KSCAN_KSO_EN_POS);
	regs->KSI_IEN = MCHP_KSCAN_KSI_IEN_REG_MASK;

	/* Interrupts are enabled in the thread function */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    xec_kbd_isr, DEVICE_DT_INST_GET(0), 0);

	xec_kbd_clear_girq_status(dev);
	xec_kbd_configure_girq(dev);

	return input_kbd_matrix_common_init(dev);
}

PINCTRL_DT_INST_DEFINE(0);

PM_DEVICE_DT_INST_DEFINE(0, xec_kbd_pm_action);

INPUT_KBD_MATRIX_DT_INST_DEFINE(0);

static const struct input_kbd_matrix_api xec_kbd_api = {
	.drive_column = xec_kbd_drive_column,
	.read_row = xec_kbd_read_row,
	.set_detect_mode = xec_kbd_set_detect_mode,
};

/* To enable wakeup, set the "wakeup-source" on the keyboard scanning device
 * node.
 */
static struct xec_kbd_config xec_kbd_cfg_0 = {
	.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(0, &xec_kbd_api),
	.regs = (struct kscan_regs *)(DT_INST_REG_ADDR(0)),
	.girq = DT_INST_PROP_BY_IDX(0, girqs, 0),
	.girq_pos = DT_INST_PROP_BY_IDX(0, girqs, 1),
#ifdef CONFIG_SOC_SERIES_MEC172X
	.pcr_idx = DT_INST_PROP_BY_IDX(0, pcrs, 0),
	.pcr_pos = DT_INST_PROP_BY_IDX(0, pcrs, 1),
#endif
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.wakeup_source = DT_INST_PROP(0, wakeup_source)
};

static struct xec_kbd_data kbd_data_0;

DEVICE_DT_INST_DEFINE(0, xec_kbd_init,
		      PM_DEVICE_DT_INST_GET(0), &kbd_data_0, &xec_kbd_cfg_0,
		      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one microchip,xec-kbd compatible node can be supported");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, row_size), 1, 8), "invalid row-size");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, col_size), 1, 18), "invalid col-size");
