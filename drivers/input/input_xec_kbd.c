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
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(input_xec_kbd, CONFIG_INPUT_LOG_LEVEL);

struct xec_kbd_config {
	struct input_kbd_matrix_common_config common;

	mm_reg_t base;
	const struct pinctrl_dev_config *pcfg;
	uint8_t enc_pcr;
	uint8_t girq;
	uint8_t girq_pos;
	bool wakeup_source;
};

struct xec_kbd_data {
	struct input_kbd_matrix_common_data common;
	bool pm_lock_taken;
};

static void xec_kbd_drive_column(const struct device *dev, int data)
{
	struct xec_kbd_config const *cfg = dev->config;
	mm_reg_t base = cfg->base;

	if (data == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		/* KSO output controlled by the KSO_SELECT field */
		sys_write32(MCHP_KSCAN_KSO_ALL, base + XEC_KBD_KSO_SEL_OFS);
	} else if (data == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		/* Keyboard scan disabled. All KSO output buffers disabled */
		sys_write32(MCHP_KSCAN_KSO_EN, base + XEC_KBD_KSO_SEL_OFS);
	} else {
		/* Assume, ALL was previously set */
		sys_write32(data, base + XEC_KBD_KSO_SEL_OFS);
	}
}

static kbd_row_t xec_kbd_read_row(const struct device *dev)
{
	struct xec_kbd_config const *cfg = dev->config;
	mm_reg_t base = cfg->base;

	/* In this implementation a 1 means key pressed */
	return ~(sys_read32(base + XEC_KBD_KSI_IN_OFS) & 0xff);
}

static void xec_kbd_isr(const struct device *dev)
{
	struct xec_kbd_config const *cfg = dev->config;

	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
	irq_disable(DT_INST_IRQN(0));

	input_kbd_matrix_poll_start(dev);
}

static void xec_kbd_set_detect_mode(const struct device *dev, bool enabled)
{
	struct xec_kbd_config const *cfg = dev->config;
	struct xec_kbd_data *data = dev->data;
	mm_reg_t base = cfg->base;

	if (enabled) {
		if (data->pm_lock_taken) {
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE,
						 PM_ALL_SUBSTATES);
		}

		sys_write32(MCHP_KSCAN_KSO_SEL_REG_MASK, base + XEC_KBD_KSI_STS_OFS);

		soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
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
	mm_reg_t base = cfg->base;
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

		sys_clear_bits(base + XEC_KBD_KSO_SEL_OFS, BIT(MCHP_KSCAN_KSO_EN_POS));
		/* Clear status register */
		sys_write32(MCHP_KSCAN_KSO_SEL_REG_MASK, base + XEC_KBD_KSI_STS_OFS);
		sys_write32(MCHP_KSCAN_KSI_IEN_REG_MASK, base + XEC_KBD_KSI_IEN_OFS);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		sys_set_bits(base + XEC_KBD_KSO_SEL_OFS, BIT(MCHP_KSCAN_KSO_EN_POS));
		sys_write32(~MCHP_KSCAN_KSI_IEN_REG_MASK, base + XEC_KBD_KSI_IEN_OFS);
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
	mm_reg_t base = cfg->base;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC KSCAN pinctrl init failed (%d)", ret);
		return ret;
	}

	soc_xec_pcr_sleep_en_clear(cfg->enc_pcr);

	/* Enable predrive */
	sys_set_bits(base + XEC_KBD_KSO_SEL_OFS, BIT(MCHP_KSCAN_KSO_EN_POS));
	sys_write32(MCHP_KSCAN_EXT_CTRL_PREDRV_EN, base + XEC_KBD_EXT_CTRL_OFS);
	sys_clear_bits(base + XEC_KBD_KSO_SEL_OFS, BIT(MCHP_KSCAN_KSO_EN_POS));
	sys_write32(MCHP_KSCAN_KSI_IEN_REG_MASK, base + XEC_KBD_KSI_IEN_OFS);

	/* Interrupts are enabled in the thread function */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    xec_kbd_isr, DEVICE_DT_INST_GET(0), 0);

	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
	soc_ecia_girq_ctrl(cfg->girq, cfg->girq_pos, 1u);

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

#define DEV_CFG_GIRQ(inst)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, 0))
#define DEV_CFG_GIRQ_POS(inst) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, 0))

/* To enable wakeup, set the "wakeup-source" on the keyboard scanning device
 * node.
 */
static struct xec_kbd_config xec_kbd_cfg_0 = {
	.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(0, &xec_kbd_api),
	.base = DT_INST_REG_ADDR(0),
	.girq = DEV_CFG_GIRQ(0),
	.girq_pos = DEV_CFG_GIRQ_POS(0),
	.enc_pcr = DT_INST_PROP(0, pcrs),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.wakeup_source = DT_INST_PROP(0, wakeup_source)};

static struct xec_kbd_data kbd_data_0;

DEVICE_DT_INST_DEFINE(0, xec_kbd_init,
		      PM_DEVICE_DT_INST_GET(0), &kbd_data_0, &xec_kbd_cfg_0,
		      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one microchip,xec-kbd compatible node can be supported");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, row_size), 1, 8), "invalid row-size");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, col_size), 1, 18), "invalid col-size");
