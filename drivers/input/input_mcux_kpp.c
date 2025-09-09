/*
 * Copyright 2025, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <fsl_kpp.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/imx_ccm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(kpp, CONFIG_INPUT_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_mcux_kpp

#define INPUT_KPP_COLUMNNUM_MAX  KPP_KEYPAD_COLUMNNUM_MAX
#define INPUT_KPP_ROWNUM_MAX     KPP_KEYPAD_ROWNUM_MAX
#define INPUT_KPP_ROWNUM_MAX     KPP_KEYPAD_ROWNUM_MAX

struct kpp_config {
	KPP_Type *base;
	const struct device *ccm_dev;
	clock_control_subsys_t clk_sub_sys;
	const struct pinctrl_dev_config *pcfg;
};

struct kpp_data {
	uint32_t clock_rate;
	struct k_work_delayable work;
	uint8_t read_keys_old[KPP_KEYPAD_COLUMNNUM_MAX];
	uint8_t read_keys_new[KPP_KEYPAD_COLUMNNUM_MAX];
	uint8_t key_pressed_number;
	const struct device *dev;
};

static void get_source_clk_rate(const struct device *dev, uint32_t *clk_rate)
{
	const struct kpp_config *dev_cfg = dev->config;
	const struct device *ccm_dev = dev_cfg->ccm_dev;
	clock_control_subsys_t clk_sub_sys = dev_cfg->clk_sub_sys;

	if (!device_is_ready(ccm_dev)) {
		LOG_ERR("CCM driver is not installed");
		*clk_rate = 0;
		return;
	}

	clock_control_get_rate(ccm_dev, clk_sub_sys, clk_rate);
}

static void kpp_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct kpp_data *drv_data = CONTAINER_OF(dwork, struct kpp_data, work);
	const struct device *dev = drv_data->dev;
	const struct kpp_config *config = dev->config;

	uint8_t read_keys_new[KPP_KEYPAD_COLUMNNUM_MAX];

	/* Read the key press data */
	KPP_keyPressScanning(config->base, read_keys_new, drv_data->clock_rate);

	/* Analyze the keypad data */
	for (int col = 0; col < INPUT_KPP_COLUMNNUM_MAX; col++) {
		if (drv_data->read_keys_old[col] == read_keys_new[col]) {
			continue;
		}
		for (int row = 0; row < INPUT_KPP_ROWNUM_MAX; row++) {
			if (((drv_data->read_keys_old[col] ^ read_keys_new[col])
				& BIT(row)) == 0) {
				continue;
			}
			if ((read_keys_new[col] & BIT(row)) != 0) {
				/* Key press event */
				KPP_SetSynchronizeChain(config->base,
					kKPP_ClearKeyDepressSyncChain);
				input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
				input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
				input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
				drv_data->key_pressed_number++;
			} else {
				/* Key release event */
				KPP_SetSynchronizeChain(config->base,
					kKPP_SetKeyReleasesSyncChain);
				input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
				input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
				input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
				drv_data->key_pressed_number--;
			}
			drv_data->read_keys_old[col] = read_keys_new[col];
		}
	}

	if (drv_data->key_pressed_number == 0U) {
		KPP_ClearStatusFlag(config->base, kKPP_keyDepressInterrupt |
			kKPP_keyReleaseInterrupt);
		KPP_EnableInterrupts(config->base, kKPP_keyDepressInterrupt);
	} else {
		k_work_schedule(&drv_data->work, K_MSEC(CONFIG_INPUT_KPP_PERIOD_MS));
	}
}

static void kpp_isr(const struct device *dev)
{
	const struct kpp_config *config = dev->config;
	struct kpp_data *drv_data = dev->data;

	uint16_t status = KPP_GetStatusFlag(config->base);

	if ((status & kKPP_keyDepressInterrupt) == 0) {
		LOG_ERR("No key press or release detected");
		return;
	}

	drv_data->key_pressed_number = 0;
	/* Disable interrupts. */
	KPP_DisableInterrupts(config->base, kKPP_keyDepressInterrupt |
		kKPP_keyReleaseInterrupt);
	/* Clear status. */
	KPP_ClearStatusFlag(config->base, kKPP_keyDepressInterrupt |
		kKPP_keyReleaseInterrupt);
	/* Key depress report */
	k_work_schedule(&drv_data->work, K_MSEC(0));
}

static int input_kpp_init(const struct device *dev)
{
	const struct kpp_config *config = dev->config;
	struct kpp_data *drv_data = dev->data;
	kpp_config_t kppConfig;

	if (!device_is_ready(config->ccm_dev)) {
		LOG_ERR("CCM driver is not installed");
		return -ENODEV;
	}

	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		LOG_ERR("Failed to configure pin");
		return ret;
	}

	kppConfig.activeRow = 0xFF;
	kppConfig.activeColumn = 0xFF;
	kppConfig.interrupt = kKPP_keyDepressInterrupt;

	KPP_Init(config->base, &kppConfig);

	get_source_clk_rate(dev, &drv_data->clock_rate);

	drv_data->dev = dev;

	/* Read the key press data */
	KPP_keyPressScanning(config->base, drv_data->read_keys_old, drv_data->clock_rate);

	k_work_init_delayable(&drv_data->work, kpp_work_handler);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		kpp_isr, DEVICE_DT_INST_GET(0), 0);
	return 0;
}

#define INPUT_KPP_INIT(n)                                                               \
	static struct kpp_data kpp_data_##n;                                            \
                                                                                        \
	PINCTRL_DT_INST_DEFINE(n);	                                                \
                                                                                        \
	static const struct kpp_config kpp_config_##n = {                               \
		.base = (KPP_Type *)DT_INST_REG_ADDR(n),                                \
		.clk_sub_sys =                                                          \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, 0, name),	\
		.ccm_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                              \
	};                                                                              \
                                                                                        \
	DEVICE_DT_INST_DEFINE(n,                                                        \
			      input_kpp_init,                                           \
			      NULL,                                                     \
			      &kpp_data_##n,                                            \
			      &kpp_config_##n,                                          \
			      POST_KERNEL,                                              \
			      CONFIG_INPUT_INIT_PRIORITY,                               \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_KPP_INIT)
