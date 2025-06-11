/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_kbd

#include <errno.h>
#include <soc.h>
#include <soc_dt.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it51xxx.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-it51xxx-wuc.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_ite_it51xxx_kbd);

#define KEYBOARD_KSI_PIN_COUNT IT8XXX2_DT_INST_WUCCTRL_LEN(0)

/* 0x04: Keyboard Scan In Data */
#define REG_KBS_KSI 0x04
/* 0x80: Keyboard Scan Out Data (3 bytes value and 4 bytes aligned) */
#define REG_KBS_KSO 0x80

struct it51xxx_kbd_wuc_map_cfg {
	/* WUC control device structure */
	const struct device *wucs;
	/* WUC pin mask */
	uint8_t mask;
};

struct it51xxx_kbd_config {
	struct input_kbd_matrix_common_config common;
	/* Keyboard scan controller base address */
	uintptr_t base;
	/* Keyboard scan input (KSI) wake-up irq */
	int irq;
	/* KSI[7:0] wake-up input source configuration list */
	const struct it51xxx_kbd_wuc_map_cfg *wuc_map_list;
	/* keyboard scan alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	/* KSO16 GPIO cells */
	struct gpio_dt_spec kso16_gpios;
	/* KSO17 GPIO cells */
	struct gpio_dt_spec kso17_gpios;
	/* Mask of signals to ignore */
	uint32_t kso_ignore_mask;
};

struct it51xxx_kbd_data {
	struct input_kbd_matrix_common_data common;
	/* KSI[7:0] wake-up interrupt status mask */
	uint8_t ksi_pin_mask;
};

INPUT_KBD_STRUCT_CHECK(struct it51xxx_kbd_config, struct it51xxx_kbd_data);

static void it51xxx_kbd_drive_column(const struct device *dev, int col)
{
	const struct it51xxx_kbd_config *const config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	const uintptr_t base = config->base;
	const uint32_t kso_mask = BIT_MASK(common->col_size) & ~config->kso_ignore_mask;
	uint32_t kso_val, reg_val;
	unsigned int key;

	if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		/* Tri-state all outputs */
		kso_val = kso_mask;
	} else if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		/* Assert all outputs */
		kso_val = 0;
	} else {
		/* Assert a single output */
		kso_val = kso_mask ^ BIT(col);
	}

	/*
	 * Set keyboard scan output data, disable global interrupts for critical section.
	 * The KBS_KSO registers contains both keyboard and GPIO output settings.
	 * Not all bits are for the keyboard will be driven, so a critical section
	 * is needed to avoid race conditions.
	 */
	key = irq_lock();
	reg_val = sys_read32(base + REG_KBS_KSO) & ~kso_mask;
	sys_write32((reg_val | (kso_val & kso_mask)), base + REG_KBS_KSO);
	irq_unlock(key);
}

static kbd_row_t it51xxx_kbd_read_row(const struct device *dev)
{
	const struct it51xxx_kbd_config *const config = dev->config;
	const uintptr_t base = config->base;
	uint8_t reg_val;

	/* Bits are active-low, so toggle it (return 1 means key pressed) */
	reg_val = sys_read8(base + REG_KBS_KSI);

	return reg_val ^ 0xff;
}

static void it51xxx_kbd_clear_status(const struct device *dev)
{
	const struct it51xxx_kbd_config *const config = dev->config;
	struct it51xxx_kbd_data *data = dev->data;

	/*
	 * W/C wakeup interrupt status of KSI[7:0] pins
	 *
	 * NOTE: We want to clear the status as soon as possible,
	 *       so clear KSI[7:0] pins at a time.
	 */
	it51xxx_wuc_clear_status(config->wuc_map_list[0].wucs, data->ksi_pin_mask);

	/* W/C interrupt status of KSI[7:0] pins */
	ite_intc_isr_clear(config->irq);
}

static void it51xxx_kbd_isr(const struct device *dev)
{
	it51xxx_kbd_clear_status(dev);

	input_kbd_matrix_poll_start(dev);
}

static void it51xxx_kbd_set_detect_mode(const struct device *dev, bool enable)
{
	const struct it51xxx_kbd_config *const config = dev->config;

	if (enable) {
		it51xxx_kbd_clear_status(dev);

		irq_enable(config->irq);
	} else {
		irq_disable(config->irq);
	}
}

static int it51xxx_kbd_init(const struct device *dev)
{
	const struct it51xxx_kbd_config *const config = dev->config;
	const struct input_kbd_matrix_common_config *common = &config->common;
	struct it51xxx_kbd_data *data = dev->data;
	const uintptr_t base = config->base;
	const uint32_t kso_mask = BIT_MASK(common->col_size) & ~config->kso_ignore_mask;
	int status;
	uint32_t reg_val;

	/* Disable wakeup and interrupt of KSI pins before configuring */
	it51xxx_kbd_set_detect_mode(dev, false);

	if (common->col_size > 16) {
		/*
		 * For KSO[16] and KSO[17]:
		 * Set pull up and open-drain by their GPIO ports corresponding
		 * GPCRx and GPOTx registers.
		 */
		gpio_pin_configure_dt(&config->kso16_gpios, (GPIO_OPEN_DRAIN | GPIO_PULL_UP));
		gpio_pin_configure_dt(&config->kso17_gpios, (GPIO_OPEN_DRAIN | GPIO_PULL_UP));
	}
	/* Enable keyboard scan alternate function */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to enable keyboard scan alternate function");
		return status;
	}

	/* KSO[col_size:0] pins output low */
	reg_val = sys_read32(base + REG_KBS_KSO) & ~kso_mask;
	sys_write32(reg_val, base + REG_KBS_KSO);

	for (int i = 0; i < KEYBOARD_KSI_PIN_COUNT; i++) {
		/* Select wakeup interrupt falling-edge triggered of KSI[7:0] pins */
		it51xxx_wuc_set_polarity(config->wuc_map_list[i].wucs, config->wuc_map_list[i].mask,
					 WUC_TYPE_EDGE_FALLING);
		/* W/C wakeup interrupt status of KSI[7:0] pins */
		it51xxx_wuc_clear_status(config->wuc_map_list[i].wucs,
					 config->wuc_map_list[i].mask);
		/* Enable wakeup interrupt of KSI[7:0] pins */
		it51xxx_wuc_enable(config->wuc_map_list[i].wucs, config->wuc_map_list[i].mask);

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

	irq_connect_dynamic(DT_INST_IRQN(0), 0, (void (*)(const void *))it51xxx_kbd_isr,
			    (const void *)dev, 0);

	return input_kbd_matrix_common_init(dev);
}

static const struct it51xxx_kbd_wuc_map_cfg it51xxx_kbd_wuc[IT8XXX2_DT_INST_WUCCTRL_LEN(0)] =
	IT8XXX2_DT_WUC_ITEMS_LIST(0);

PINCTRL_DT_INST_DEFINE(0);

INPUT_KBD_MATRIX_DT_INST_DEFINE(0);

static const struct input_kbd_matrix_api it51xxx_kbd_api = {
	.drive_column = it51xxx_kbd_drive_column,
	.read_row = it51xxx_kbd_read_row,
	.set_detect_mode = it51xxx_kbd_set_detect_mode,
};

static const struct it51xxx_kbd_config it51xxx_kbd_cfg_0 = {
	.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(0, &it51xxx_kbd_api),
	.base = DT_INST_REG_ADDR(0),
	.irq = DT_INST_IRQN(0),
	.wuc_map_list = it51xxx_kbd_wuc,
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.kso16_gpios = GPIO_DT_SPEC_INST_GET(0, kso16_gpios),
	.kso17_gpios = GPIO_DT_SPEC_INST_GET(0, kso17_gpios),
	.kso_ignore_mask = DT_INST_PROP(0, kso_ignore_mask),
};

static struct it51xxx_kbd_data it51xxx_kbd_data_0;

PM_DEVICE_DT_INST_DEFINE(0, input_kbd_matrix_pm_action);

DEVICE_DT_INST_DEFINE(0, &it51xxx_kbd_init, PM_DEVICE_DT_INST_GET(0), &it51xxx_kbd_data_0,
		      &it51xxx_kbd_cfg_0, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE_SYSTEM_MANAGED) || IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME),
	     "CONFIG_PM_DEVICE_RUNTIME must be enabled when using CONFIG_PM_DEVICE_SYSTEM_MANAGED");

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one ite,it51xxx-kbd compatible node can be supported");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, row_size), 1, 8), "invalid row-size");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, col_size), 1, 18), "invalid col-size");
