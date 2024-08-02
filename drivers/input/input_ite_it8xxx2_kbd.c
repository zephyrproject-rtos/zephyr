/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_kbd

#include <errno.h>
#include <soc.h>
#include <soc_dt.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it8xxx2.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/interrupt-controller/it8xxx2-wuc.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_ite_it8xxx2_kbd);

#define KEYBOARD_KSI_PIN_COUNT IT8XXX2_DT_INST_WUCCTRL_LEN(0)

struct it8xxx2_kbd_wuc_map_cfg {
	/* WUC control device structure */
	const struct device *wucs;
	/* WUC pin mask */
	uint8_t mask;
};

struct it8xxx2_kbd_config {
	struct input_kbd_matrix_common_config common;
	/* Keyboard scan controller base address */
	struct kscan_it8xxx2_regs *base;
	/* Keyboard scan input (KSI) wake-up irq */
	int irq;
	/* KSI[7:0] wake-up input source configuration list */
	const struct it8xxx2_kbd_wuc_map_cfg *wuc_map_list;
	/* KSI[7:0]/KSO[17:0] keyboard scan alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	/* KSO16 GPIO cells */
	struct gpio_dt_spec kso16_gpios;
	/* KSO17 GPIO cells */
	struct gpio_dt_spec kso17_gpios;
};

struct it8xxx2_kbd_data {
	struct input_kbd_matrix_common_data common;
	/* KSI[7:0] wake-up interrupt status mask */
	uint8_t ksi_pin_mask;
};

INPUT_KBD_STRUCT_CHECK(struct it8xxx2_kbd_config, struct it8xxx2_kbd_data);

static void it8xxx2_kbd_drive_column(const struct device *dev, int col)
{
	const struct it8xxx2_kbd_config *const config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	struct kscan_it8xxx2_regs *const inst = config->base;
	const uint32_t kso_mask = BIT_MASK(common->col_size);
	const uint8_t ksol_mask = kso_mask & 0xff;
	const uint8_t ksoh1_mask = (kso_mask >> 8) & 0xff;
	uint32_t kso_val;
	unsigned int key;

	/* Tri-state all outputs */
	if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		kso_val = kso_mask;
	/* Assert all outputs */
	} else if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		kso_val = 0;
	/* Assert a single output */
	} else {
		kso_val = kso_mask ^ BIT(col);
	}

	/* Set KSO[17:0] output data */
	inst->KBS_KSOL = (inst->KBS_KSOL & ~ksol_mask) | (kso_val & ksol_mask);
	/*
	 * Disable global interrupts for critical section
	 * The KBS_KSOH1 register contains both keyboard and GPIO output settings.
	 * Not all bits are for the keyboard will be driven, so a critical section
	 * is needed to avoid race conditions.
	 */
	key = irq_lock();
	inst->KBS_KSOH1 = (inst->KBS_KSOH1 & ~ksoh1_mask) | ((kso_val >> 8) & ksoh1_mask);
	/* Restore interrupts */
	irq_unlock(key);

	if (common->col_size > 16) {
		inst->KBS_KSOH2 = (kso_val >> 16) & 0xff;
	}
}

static kbd_row_t it8xxx2_kbd_read_row(const struct device *dev)
{
	const struct it8xxx2_kbd_config *const config = dev->config;
	struct kscan_it8xxx2_regs *const inst = config->base;

	/* Bits are active-low, so toggle it (return 1 means key pressed) */
	return (inst->KBS_KSI ^ 0xff);
}

static void it8xxx2_kbd_isr(const struct device *dev)
{
	const struct it8xxx2_kbd_config *const config = dev->config;
	struct it8xxx2_kbd_data *data = dev->data;

	/*
	 * W/C wakeup interrupt status of KSI[7:0] pins
	 *
	 * NOTE: We want to clear the status as soon as possible,
	 *       so clear KSI[7:0] pins at a time.
	 */
	it8xxx2_wuc_clear_status(config->wuc_map_list[0].wucs,
				 data->ksi_pin_mask);

	/* W/C interrupt status of KSI[7:0] pins */
	ite_intc_isr_clear(config->irq);

	input_kbd_matrix_poll_start(dev);
}

static void it8xxx2_kbd_set_detect_mode(const struct device *dev, bool enable)
{
	const struct it8xxx2_kbd_config *const config = dev->config;
	struct it8xxx2_kbd_data *data = dev->data;

	if (enable) {
		/*
		 * W/C wakeup interrupt status of KSI[7:0] pins
		 *
		 * NOTE: We want to clear the status as soon as possible,
		 *       so clear KSI[7:0] pins at a time.
		 */
		it8xxx2_wuc_clear_status(config->wuc_map_list[0].wucs,
					 data->ksi_pin_mask);

		/* W/C interrupt status of KSI[7:0] pins */
		ite_intc_isr_clear(config->irq);

		irq_enable(config->irq);
	} else {
		irq_disable(config->irq);
	}
}

static int it8xxx2_kbd_init(const struct device *dev)
{
	const struct it8xxx2_kbd_config *const config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	struct it8xxx2_kbd_data *data = dev->data;
	struct kscan_it8xxx2_regs *const inst = config->base;
	const uint32_t kso_mask = BIT_MASK(common->col_size);
	const uint8_t ksol_mask = kso_mask & 0xff;
	const uint8_t ksoh1_mask = (kso_mask >> 8) & 0xff;
	int status;

	/* Disable wakeup and interrupt of KSI pins before configuring */
	it8xxx2_kbd_set_detect_mode(dev, false);

	if (common->col_size > 16) {
		/*
		 * For KSO[16] and KSO[17]:
		 * 1.GPOTRC:
		 *   Bit[x] = 1b: Enable the open-drain mode of KSO pin
		 * 2.GPCRCx:
		 *   Bit[7:6] = 00b: Select alternate KSO function
		 *   Bit[2] = 1b: Enable the internal pull-up of KSO pin
		 *
		 * NOTE: Set input temporarily for gpio_pin_configure(), after
		 * that pinctrl_apply_state() set to alternate function
		 * immediately.
		 */
		gpio_pin_configure_dt(&config->kso16_gpios, GPIO_INPUT);
		gpio_pin_configure_dt(&config->kso17_gpios, GPIO_INPUT);
	}
	/*
	 * Enable the internal pull-up and kbs mode of the KSI[7:0] pins.
	 * Enable the internal pull-up and kbs mode of the KSO[15:0] pins.
	 * Enable the open-drain mode of the KSO[17:0] pins.
	 */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure KSI[7:0] and KSO[17:0] pins");
		return status;
	}

	/* KSO[17:0] pins output low */
	inst->KBS_KSOL = inst->KBS_KSOL & ~ksol_mask;
	inst->KBS_KSOH1 = inst->KBS_KSOH1 & ~ksoh1_mask;
	if (common->col_size > 16) {
		inst->KBS_KSOH2 = 0x00;
	}

	for (int i = 0; i < KEYBOARD_KSI_PIN_COUNT; i++) {
		/* Select wakeup interrupt falling-edge triggered of KSI[7:0] pins */
		it8xxx2_wuc_set_polarity(config->wuc_map_list[i].wucs,
					 config->wuc_map_list[i].mask,
					 WUC_TYPE_EDGE_FALLING);
		/* W/C wakeup interrupt status of KSI[7:0] pins */
		it8xxx2_wuc_clear_status(config->wuc_map_list[i].wucs,
					 config->wuc_map_list[i].mask);
		/* Enable wakeup interrupt of KSI[7:0] pins */
		it8xxx2_wuc_enable(config->wuc_map_list[i].wucs,
				   config->wuc_map_list[i].mask);

		/*
		 * We want to clear KSI[7:0] pins status at a time when wakeup
		 * interrupt fire, so gather the KSI[7:0] pin mask value here.
		 */
		if (config->wuc_map_list[i].wucs != config->wuc_map_list[0].wucs) {
			LOG_ERR("KSI%d pin isn't in the same wuc node!", i);
		}
		data->ksi_pin_mask |= config->wuc_map_list[i].mask;
	}

	/* W/C interrupt status of KSI[7:0] pins */
	ite_intc_isr_clear(config->irq);

	irq_connect_dynamic(DT_INST_IRQN(0), 0,
			    (void (*)(const void *))it8xxx2_kbd_isr,
			    (const void *)dev, 0);

	return input_kbd_matrix_common_init(dev);
}

static const struct it8xxx2_kbd_wuc_map_cfg
	it8xxx2_kbd_wuc[IT8XXX2_DT_INST_WUCCTRL_LEN(0)] = IT8XXX2_DT_WUC_ITEMS_LIST(0);

PINCTRL_DT_INST_DEFINE(0);

INPUT_KBD_MATRIX_DT_INST_DEFINE(0);

static const struct input_kbd_matrix_api it8xxx2_kbd_api = {
	.drive_column = it8xxx2_kbd_drive_column,
	.read_row = it8xxx2_kbd_read_row,
	.set_detect_mode = it8xxx2_kbd_set_detect_mode,
};

static const struct it8xxx2_kbd_config it8xxx2_kbd_cfg_0 = {
	.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(0, &it8xxx2_kbd_api),
	.base = (struct kscan_it8xxx2_regs *)DT_INST_REG_ADDR_BY_IDX(0, 0),
	.irq = DT_INST_IRQN(0),
	.wuc_map_list = it8xxx2_kbd_wuc,
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.kso16_gpios = GPIO_DT_SPEC_INST_GET(0, kso16_gpios),
	.kso17_gpios = GPIO_DT_SPEC_INST_GET(0, kso17_gpios),
};

static struct it8xxx2_kbd_data it8xxx2_kbd_data_0;

DEVICE_DT_INST_DEFINE(0, &it8xxx2_kbd_init, NULL,
		      &it8xxx2_kbd_data_0, &it8xxx2_kbd_cfg_0,
		      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one ite,it8xxx2-kbd compatible node can be supported");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, row_size), 1, 8), "invalid row-size");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, col_size), 1, 18), "invalid col-size");
