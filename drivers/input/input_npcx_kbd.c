/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_kbd

#include "soc_miwu.h"

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <soc.h>
LOG_MODULE_REGISTER(input_npcx_kbd, CONFIG_INPUT_LOG_LEVEL);

#define ROW_SIZE DT_INST_PROP(0, row_size)

/* Driver config */
struct npcx_kbd_config {
	struct input_kbd_matrix_common_config common;
	/* Keyboard scan controller base address */
	struct kbs_reg *base;
	/* Clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* Pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Keyboard scan input (KSI) wake-up irq */
	int irq;
	/* Size of keyboard inputs-wui mapping array */
	int wui_size;
	/* Mapping table between keyboard inputs and wui */
	struct npcx_wui wui_maps[];
};

struct npcx_kbd_data {
	struct input_kbd_matrix_common_data common;
	struct miwu_callback ksi_callback[ROW_SIZE];
};

INPUT_KBD_STRUCT_CHECK(struct npcx_kbd_config, struct npcx_kbd_data);

/* Keyboard scan local functions */
static void npcx_kbd_ksi_isr(const struct device *dev, struct npcx_wui *wui)
{
	ARG_UNUSED(wui);

	input_kbd_matrix_poll_start(dev);
}

static void npcx_kbd_set_detect_mode(const struct device *dev, bool enabled)
{
	const struct npcx_kbd_config *const config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;

	if (enabled) {
		for (int i = 0; i < common->row_size; i++) {
			npcx_miwu_irq_get_and_clear_pending(&config->wui_maps[i]);
		}

		irq_enable(config->irq);
	} else {
		irq_disable(config->irq);
	}
}

static void npcx_kbd_drive_column(const struct device *dev, int col)
{
	const struct npcx_kbd_config *config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	struct kbs_reg *const inst = config->base;
	uint32_t mask;

	if (col >= common->col_size) {
		LOG_ERR("invalid column: %d", col);
		return;
	}

	if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		/* Drive all lines to high: key detection is disabled */
		mask = ~0;
	} else if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		/* Drive all lines to low for detection any key press */
		mask = ~BIT_MASK(common->col_size);
	} else {
		/*
		 * Drive one line to low for determining which key's state
		 * changed.
		 */
		mask = ~BIT(col);
	}

	LOG_DBG("Drive col mask: %x", mask);

	inst->KBSOUT0 = (mask & 0xFFFF);
	inst->KBSOUT1 = ((mask >> 16) & 0x03);
}

static kbd_row_t npcx_kbd_read_row(const struct device *dev)
{
	const struct npcx_kbd_config *config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	struct kbs_reg *const inst = config->base;
	kbd_row_t val;

	val = inst->KBSIN;

	/* 1 means key pressed, otherwise means key released. */
	val = ~val & BIT_MASK(common->row_size);

	return val;
}

static void npcx_kbd_init_ksi_wui_callback(const struct device *dev,
					   struct miwu_callback *callback,
					   const struct npcx_wui *wui,
					   miwu_dev_callback_handler_t handler)
{
	/* KSI signal which has no wake-up input source */
	if (wui->table == NPCX_MIWU_TABLE_NONE) {
		return;
	}

	/* Install callback function */
	npcx_miwu_init_dev_callback(callback, wui, handler, dev);
	npcx_miwu_manage_callback(callback, 1);

	/* Configure MIWU setting and enable its interrupt */
	npcx_miwu_interrupt_configure(wui, NPCX_MIWU_MODE_EDGE, NPCX_MIWU_TRIG_LOW);
	npcx_miwu_irq_enable(wui);
}

static int npcx_kbd_init(const struct device *dev)
{
	const struct device *clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	const struct npcx_kbd_config *const config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	struct npcx_kbd_data *const data = dev->data;
	struct kbs_reg *const inst = config->base;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on KBSCAN controller device clock */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on KBSCAN clock fail %d", ret);
		return -EIO;
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
	if (IS_ENABLED(CONFIG_INPUT_NPCX_KBD_KSO_HIGH_DRIVE)) {
		SET_FIELD(inst->KBSCTL, NPCX_KBSCTL_KBHDRV_FIELD, 0x01);
	}

	/* Drive all column lines to low for detection any key press */
	npcx_kbd_drive_column(dev, INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE);

	if (common->row_size != ROW_SIZE) {
		LOG_ERR("Unexpected ROW_SIZE: %d != %d", common->row_size, ROW_SIZE);
		return -EINVAL;
	}

	/* Configure wake-up input and callback for keyboard input signal */
	for (int i = 0; i < common->row_size; i++) {
		npcx_kbd_init_ksi_wui_callback(
				dev, &data->ksi_callback[i], &config->wui_maps[i],
				npcx_kbd_ksi_isr);
	}

	/* Configure pin-mux for keyboard scan device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("keyboard scan pinctrl setup failed (%d)", ret);
		return ret;
	}

	return input_kbd_matrix_common_init(dev);
}

PINCTRL_DT_INST_DEFINE(0);

INPUT_KBD_MATRIX_DT_INST_DEFINE(0);

static const struct input_kbd_matrix_api npcx_kbd_api = {
	.drive_column = npcx_kbd_drive_column,
	.read_row = npcx_kbd_read_row,
	.set_detect_mode = npcx_kbd_set_detect_mode,
};

static const struct npcx_kbd_config npcx_kbd_cfg_0 = {
	.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(0, &npcx_kbd_api),
	.base = (struct kbs_reg *)DT_INST_REG_ADDR(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
	.irq = DT_INST_IRQN(0),
	.wui_size = NPCX_DT_WUI_ITEMS_LEN(0),
	.wui_maps = NPCX_DT_WUI_ITEMS_LIST(0),
};

static struct npcx_kbd_data npcx_kbd_data_0;

DEVICE_DT_INST_DEFINE(0, npcx_kbd_init, NULL,
		      &npcx_kbd_data_0, &npcx_kbd_cfg_0,
		      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one nuvoton,npcx-kbd compatible node can be supported");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, row_size), 1, 8), "invalid row-size");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, col_size), 1, 18), "invalid col-size");
