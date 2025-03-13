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

/* 0x00: Keyboard Scan Out */
#define REG_KBS_KSOL          (0x00)
/* 0x01: Keyboard Scan Out */
#define REG_KBS_KSOH1         (0x01)
/* 0x02: Keyboard Scan Out Control */
#define REG_KBS_KSOCTRL       (0x02)
#define KBS_KSOPU             BIT(2)
#define KBS_KSOOD             BIT(0)
/* 0x03: Keyboard Scan Out */
#define REG_KBS_KSOH2         (0x03)
/* 0x04: Keyboard Scan In */
#define REG_KBS_KSI           (0x04)
/* 0x05: Keyboard Scan In Control */
#define REG_KBS_KSICTRL       (0x05)
#define KBS_KSIPU             BIT(2)
/* 0x06: Keyboard Scan In [7:0] GPIO Control */
#define REG_KBS_KSIGCTRL      (0x00)
/* 0x07: Keyboard Scan In [7:0] GPIO Output Enable */
#define REG_KBS_KSIGOEN       (0x00)
/* 0x08: Keyboard Scan In [7:0] GPIO Data */
#define REG_KBS_KSIGDAT       (0x00)
/* 0x09: Keyboard Scan In [7:0] GPIO Data Mirror */
#define REG_KBS_KSIGDMRR      (0x00)
/* 0x0A: Keyboard Scan Out [15:8] GPIO Control */
#define REG_KBS_KSOHGCTRL     (0x00)
/* 0x0B: Keyboard Scan Out [15:8] GPIO Output Enable */
#define REG_KBS_KSOHGOEN      (0x00)
/* 0x0C: Keyboard Scan Out [15:8] GPIO Data Mirror */
#define REG_KBS_KSOHGDMRR     (0x00)
/* 0x0D: Keyboard Scan Out [7:0] GPIO Control */
#define REG_KBS_KSOLGCTRL     (0x00)
#define KBS_KSO2GCTRL         BIT(2)
/* 0x0E: Keyboard Scan Out [7:0] GPIO Output Enable */
#define REG_KBS_KSOLGOEN      (0x00)
#define KBS_KSO2GOEN          BIT(2)

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
	/* KSI[7:0]/KSO[17:0] keyboard scan alternate configuration */
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
	const uint8_t ksol_mask = kso_mask & 0xff;
	const uint8_t ksoh1_mask = (kso_mask >> 8) & 0xff;
	uint32_t kso_val;
	unsigned int key;
	uint8_t reg_val;

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

	/* Set KSO[17:0] output data, disable global interrupts for critical section.
	 * The KBS_KSO* registers contains both keyboard and GPIO output settings.
	 * Not all bits are for the keyboard will be driven, so a critical section
	 * is needed to avoid race conditions.
	 */
	key = irq_lock();
	reg_val = sys_read8(base + REG_KBS_KSOL);
	sys_write8((reg_val & ~ksol_mask) | (kso_val & ksol_mask), base + REG_KBS_KSOL);

	reg_val = sys_read8(base + REG_KBS_KSOH1);
	sys_write8((reg_val & ~ksoh1_mask) | ((kso_val >> 8) & ksoh1_mask), base + REG_KBS_KSOH1);
	irq_unlock(key);

	if (common->col_size > 16) {
		sys_write8((kso_val >> 16) & 0xff, base + REG_KBS_KSOH2);
	}
}

static kbd_row_t it51xxx_kbd_read_row(const struct device *dev)
{
	const struct it51xxx_kbd_config *const config = dev->config;
	const uintptr_t base = config->base;
	uint8_t reg_val;

	/* Bits are active-low, so toggle it (return 1 means key pressed) */
	reg_val = sys_read8(base + REG_KBS_KSI);

	return (reg_val ^ 0xff);
}

static void it51xxx_kbd_isr(const struct device *dev)
{
	const struct it51xxx_kbd_config *const config = dev->config;
	struct it51xxx_kbd_data *data = dev->data;

	/*
	 * W/C wakeup interrupt status of KSI[7:0] pins
	 *
	 * NOTE: We want to clear the status as soon as possible,
	 *       so clear KSI[7:0] pins at a time.
	 */
	it51xxx_wuc_clear_status(config->wuc_map_list[0].wucs,
				 data->ksi_pin_mask);

	/* W/C interrupt status of KSI[7:0] pins */
	ite_intc_isr_clear(config->irq);

	input_kbd_matrix_poll_start(dev);
}

static void it51xxx_kbd_set_detect_mode(const struct device *dev, bool enable)
{
	const struct it51xxx_kbd_config *const config = dev->config;
	struct it51xxx_kbd_data *data = dev->data;

	if (enable) {
		/*
		 * W/C wakeup interrupt status of KSI[7:0] pins
		 *
		 * NOTE: We want to clear the status as soon as possible,
		 *       so clear KSI[7:0] pins at a time.
		 */
		it51xxx_wuc_clear_status(config->wuc_map_list[0].wucs,
					 data->ksi_pin_mask);

		/* W/C interrupt status of KSI[7:0] pins */
		ite_intc_isr_clear(config->irq);

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
	const uint8_t ksol_mask = kso_mask & 0xff;
	const uint8_t ksoh1_mask = (kso_mask >> 8) & 0xff;
	int status;
	uint8_t reg_val;

	/* Disable wakeup and interrupt of KSI pins before configuring */
	it51xxx_kbd_set_detect_mode(dev, false);

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
	reg_val = sys_read8(base + REG_KBS_KSOL);
	sys_write8((reg_val & ~ksol_mask), base + REG_KBS_KSOL);

	reg_val = sys_read8(base + REG_KBS_KSOH1);
	sys_write8((reg_val & ~ksoh1_mask), base + REG_KBS_KSOH1);
	if (common->col_size > 16) {
		sys_write8(0x00, base + REG_KBS_KSOH2);
	}

	for (int i = 0; i < KEYBOARD_KSI_PIN_COUNT; i++) {
		/* Select wakeup interrupt falling-edge triggered of KSI[7:0] pins */
		it51xxx_wuc_set_polarity(config->wuc_map_list[i].wucs,
					 config->wuc_map_list[i].mask,
					 WUC_TYPE_EDGE_FALLING);
		/* W/C wakeup interrupt status of KSI[7:0] pins */
		it51xxx_wuc_clear_status(config->wuc_map_list[i].wucs,
					 config->wuc_map_list[i].mask);
		/* Enable wakeup interrupt of KSI[7:0] pins */
		it51xxx_wuc_enable(config->wuc_map_list[i].wucs,
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
			    (void (*)(const void *))it51xxx_kbd_isr,
			    (const void *)dev, 0);

	return input_kbd_matrix_common_init(dev);
}

static const struct it51xxx_kbd_wuc_map_cfg
	it51xxx_kbd_wuc[IT8XXX2_DT_INST_WUCCTRL_LEN(0)] = IT8XXX2_DT_WUC_ITEMS_LIST(0);

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

DEVICE_DT_INST_DEFINE(0, &it51xxx_kbd_init, PM_DEVICE_DT_INST_GET(0),
		      &it51xxx_kbd_data_0, &it51xxx_kbd_cfg_0,
		      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE_SYSTEM_MANAGED) ||
	     IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME),
	     "CONFIG_PM_DEVICE_RUNTIME must be enabled when using CONFIG_PM_DEVICE_SYSTEM_MANAGED");

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one ite,it51xxx-kbd compatible node can be supported");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, row_size), 1, 8), "invalid row-size");
BUILD_ASSERT(IN_RANGE(DT_INST_PROP(0, col_size), 1, 18), "invalid col-size");

