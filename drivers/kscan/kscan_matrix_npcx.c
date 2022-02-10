/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_kscan_matrix

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/kscan_matrix.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include "soc_miwu.h"

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_KSCAN_LOG_LEVEL
LOG_MODULE_REGISTER(kscan_npcx);

/* Driver config */
struct kscan_npcx_config {
	/* keyboard scan controller base address */
	struct kbs_reg *base;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Keyboard scan input (KSI) wake-up irq */
	int irq;
	/* Size of keyboard inputs-wui mapping array */
	int wui_size;
	/* Mapping table between keyboard inputs and wui */
	struct npcx_wui wui_maps[];
};

struct kscan_npcx_data {
	kscan_isr_callback_t isr_callback;
};

#define NPCX_KB_ROW_MASK (BIT(CONFIG_KSCAN_MATRIX_MAX_ROWS) - 1)

static struct miwu_dev_callback ksi_callback[CONFIG_KSCAN_MATRIX_MAX_ROWS];

/* Keyboard Scan local functions */
static void kscan_npcx_ksi_isr(const struct device *dev, struct npcx_wui *wui)
{
	ARG_UNUSED(wui);
	struct kscan_npcx_data *const data = dev->data;

	if (data->isr_callback != NULL) {
		data->isr_callback(dev);
	}
}

static void kscan_npcx_init_ksi_wui_callback(const struct device *dev,
					     struct miwu_dev_callback *callback,
					     const struct npcx_wui *wui,
					     miwu_dev_callback_handler_t handler)
{
	/* KSI signal which has no wake-up input source */
	if (wui->table == NPCX_MIWU_TABLE_NONE) {
		return;
	}

	/* Install callback function */
	npcx_miwu_init_dev_callback(callback, wui, handler, dev);
	npcx_miwu_manage_dev_callback(callback, 1);

	/* Configure MIWU setting and enable its interrupt */
	npcx_miwu_interrupt_configure(wui, NPCX_MIWU_MODE_EDGE, NPCX_MIWU_TRIG_BOTH);
	npcx_miwu_irq_enable(wui);
}

static int kscan_matrix_npcx_configure(const struct device *dev, kscan_isr_callback_t isr_callback)
{
	struct kscan_npcx_data *const data = dev->data;

	/* Configure callback function between kscan task and driver */
	data->isr_callback = isr_callback;

	return 0;
}

static int kscan_matrix_npcx_drive_column(const struct device *dev, int col)
{
	const struct kscan_npcx_config *config = dev->config;
	struct kbs_reg *const inst = config->base;
	/*
	 * Nuvoton 'Keyboard Scan' module supports 18 x 8 matrix
	 * It also support automatic scan functionality.
	 */
	uint32_t mask;

	if (col >= CONFIG_KSCAN_MATRIX_MAX_COLUMNS) {
		return -EINVAL;
	}

	/* Drive all lines to high. ie. Key detection is disabled. */
	if (col == KEYBOARD_COLUMN_DRIVE_NONE) {
		mask = ~0;
	}
	/* Drive all lines to low for detection any key press */
	else if (col == KEYBOARD_COLUMN_DRIVE_ALL) {
		mask = ~(BIT(CONFIG_KSCAN_MATRIX_MAX_COLUMNS) - 1);
	}
	/* Drive one line to low for determining which key's state changed. */
	else {
		mask = ~BIT(col);
	}

	LOG_DBG("%x", mask);

	/* Set KBSOUT */
	inst->KBSOUT0 = (mask & 0xFFFF);
	inst->KBSOUT1 = ((mask >> 16) & 0x03);

	return 0;
}

static int kscan_matrix_npcx_read_row(const struct device *dev, int *row)
{
	const struct kscan_npcx_config *config = dev->config;
	struct kbs_reg *const inst = config->base;
	int val;

	val = inst->KBSIN;

	/* 1 means key pressed, otherwise means key released. */
	*row = (~val & NPCX_KB_ROW_MASK);

	return 0;
}

static int kscan_matrix_npcx_resume_detection(const struct device *dev, bool resume)
{
	const struct kscan_npcx_config *const config = dev->config;

	if (resume) {
		irq_enable(config->irq);
	} else {
		irq_disable(config->irq);
	}

	return 0;
}

static int kscan_npcx_init(const struct device *dev)
{
	const struct device *clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	const struct kscan_npcx_config *const config = dev->config;
	struct kbs_reg *const inst = config->base;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on KBSCAN controller device clock */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t *)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on KBSCAN clock fail %d", ret);
	}

	/* Pull-up KBSIN0-7 internally */
	inst->KBSINPU = 0xFF;

	/*
	 * Keyboard Scan Control Register
	 *
	 * [6:7] - KBHDRV KBSOUTn signals output buffers are open-drain.
	 * [3] - KBSINC   Auto-increment of Buffer Data register is disabled
	 * [2] - KBSIEN   Interrupt of Auto-Scan is disabled
	 * [1] - KBSMODE  Key detection mechanism is implemented by firmware
	 * [0] - START    Write 0 to this field is not affected
	 */
	inst->KBSCTL = 0x00;

	/*
	 * Select quasi-bidirectional buffers for KSO pins. It reduces the
	 * low-to-high transition time. This feature only supports in npcx7.
	 */
	if (IS_ENABLED(CONFIG_KSCAN_NPCX_KSO_HIGH_DRIVE)) {
		SET_FIELD(inst->KBSCTL, NPCX_KBSCTL_KBHDRV_FIELD, 0x01);
	}

	/* Drive all column lines to low for detection any key press */
	kscan_matrix_npcx_drive_column(dev, KEYBOARD_COLUMN_DRIVE_NONE);

	/* Configure wake-up input and callback for keyboard input signal */
	for (int i = 0; i < ARRAY_SIZE(ksi_callback); i++)
		kscan_npcx_init_ksi_wui_callback(dev, &ksi_callback[i], &config->wui_maps[i],
						 kscan_npcx_ksi_isr);

	/* Configure pin-mux for kscan device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("kscan pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

static const struct kscan_matrix_driver_api kscan_matrix_npcx_driver_api = {
	.matrix_config = kscan_matrix_npcx_configure,
	.matrix_drive_column = kscan_matrix_npcx_drive_column,
	.matrix_read_row = kscan_matrix_npcx_read_row,
	.matrix_resume_detection = kscan_matrix_npcx_resume_detection,
};

/* Keyboard scan initialization macro functions */
#define NPCX_KSCAN_INIT(inst)                                                                      \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct kscan_npcx_config kscan_cfg_##inst = {                                 \
		.base = (struct kbs_reg *)DT_INST_REG_ADDR(inst),                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(inst),                                             \
		.irq = DT_INST_IRQN(inst),                                                         \
		.wui_size = NPCX_DT_WUI_ITEMS_LEN(inst),                                           \
		.wui_maps = NPCX_DT_WUI_ITEMS_LIST(inst),                                          \
	};                                                                                         \
	static struct kscan_npcx_data kscan_data_##inst;                                           \
	DEVICE_DT_INST_DEFINE(inst, kscan_npcx_init, NULL, &kscan_data_##inst, &kscan_cfg_##inst,  \
			      POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,                             \
			      &kscan_matrix_npcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPCX_KSCAN_INIT)
