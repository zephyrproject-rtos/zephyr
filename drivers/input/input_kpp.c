/*
 * Copyright (c) 2025, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input_kpp.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/imx_ccm.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(kpp, CONFIG_INPUT_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_mcux_kpp

struct kpp_config {
	KPP_Type *base;
	const struct device *ccm_dev;
	clock_control_subsys_t clk_sub_sys;
};


struct kpp_data {
	struct k_mutex lock;
	kpp_callback callback;
};

static void get_source_clk_rate(const struct device *dev, uint32_t *clk_rate)
{
	const struct kpp_config *dev_cfg = dev->config;
	const struct device *ccm_dev = dev_cfg->ccm_dev;
	clock_control_subsys_t clk_sub_sys = dev_cfg->clk_sub_sys;
	uint32_t rate = 0;

	if (device_is_ready(ccm_dev)) {
		clock_control_get_rate(ccm_dev, clk_sub_sys, &rate);
	} else {
		LOG_ERR("CCM driver is not installed");
		*clk_rate = rate;
		return;
	}
	*clk_rate = rate;
}

static void kpp_isr(const struct device *dev)
{
	struct kpp_data *data = dev->data;
	const struct kpp_config *config = dev->config;

	uint16_t status = KPP_GetStatusFlag(config->base);

	if ((status & kKPP_keyDepressInterrupt) != 0U) {
		/* Disable interrupts. */
		KPP_DisableInterrupts(config->base, kKPP_keyDepressInterrupt);
		if (data->callback != NULL) {
			data->callback(dev);
		}
		KPP_SetSynchronizeChain(config->base, kKPP_ClearKeyDepressSyncChain);
		/* Enable interrupts. */
		KPP_EnableInterrupts(config->base, kKPP_keyDepressInterrupt);
	} else if ((status & kKPP_keyReleaseInterrupt) != 0U) {
		/* Disable interrupts. */
		KPP_DisableInterrupts(config->base, kKPP_keyReleaseInterrupt);
		if (data->callback != NULL) {
			data->callback(dev);
		}
		KPP_SetSynchronizeChain(config->base, kKPP_SetKeyReleasesSyncChain);
	} else {
		LOG_ERR("No key press or release detected");
	}

}

void input_kpp_set_callback(const struct device *dev, kpp_callback cb)
{
	struct kpp_data *data = dev->data;

	data->callback = cb;
}

void input_kpp_config(const struct device *dev, uint8_t active_row_mask,
				uint8_t active_column_mask, enum kpp_event interrupt_event)
{
	const struct kpp_config *config = dev->config;
	struct kpp_data *drv_data = dev->data;
	KPP_Type *base = config->base;
	uint16_t interrupt_source_mask = 0U;

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	/* Clear all. */
	base->KPSR &= (uint16_t)(~(KPP_KPSR_KRIE_MASK | KPP_KPSR_KDIE_MASK));

	/* Enable the keypad row and set the column strobe output to open drain. */
	base->KPCR = KPP_KPCR_KRE(active_row_mask);
	base->KPDR = KPP_KPDR_KCD((uint8_t) ~(active_column_mask));
	base->KPCR |= KPP_KPCR_KCO(active_column_mask);

	/* Set the input direction for row and output direction for column. */
	base->KDDR = KPP_KDDR_KCDD(active_column_mask) |
			KPP_KDDR_KRDD((uint8_t) ~(active_row_mask));

	if (interrupt_event == KPP_EVENT_KEY_DPRESS) {
		/* Enable key press interrupt. */
		interrupt_source_mask |= KPP_KPSR_KDIE_MASK;
	}

	if (interrupt_event == KPP_EVENT_KEY_RELEASE) {
		/* Enable key release interrupt. */
		interrupt_source_mask |= KPP_KPSR_KRIE_MASK;
	}

	/* Clear the status flag and enable the interrupt. */
	base->KPSR = KPP_KPSR_KPKR_MASK | KPP_KPSR_KPKD_MASK |
					KPP_KPSR_KDSC_MASK | interrupt_source_mask;

	k_mutex_unlock(&drv_data->lock);
}

void input_kpp_scan(const struct device *dev, uint8_t *data)
{
	const struct kpp_config *config = dev->config;
	struct kpp_data *drv_data = dev->data;
	uint32_t clk_rate;

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	get_source_clk_rate(dev, &clk_rate);

	KPP_keyPressScanning(config->base, data, clk_rate);

	k_mutex_unlock(&drv_data->lock);
}

static int input_kpp_init(const struct device *dev)
{
	const struct kpp_config *config = dev->config;
	kpp_config_t kppConfig;

	if (!device_is_ready(config->ccm_dev)) {
		LOG_ERR("CCM driver is not installed");
		return -ENODEV;
	}

	kppConfig.activeRow = 0U;
	kppConfig.activeColumn = 0U;
	kppConfig.interrupt = 0U;

	KPP_Init(config->base, &kppConfig);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		kpp_isr, DEVICE_DT_INST_GET(0), 0);
	return 0;
}

#define INPUT_KPP_INIT(n)						\
	static struct kpp_data kpp_data_##n;		\
												\
	static const struct kpp_config kpp_config_##n = {	\
		.base = (KPP_Type *)DT_INST_REG_ADDR(n),		\
		.clk_sub_sys =									\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, 0, name),	\
		.ccm_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n,				\
			input_kpp_init,					\
			NULL,							\
			&kpp_data_##n,					\
			&kpp_config_##n,				\
			POST_KERNEL,					\
			CONFIG_INPUT_INIT_PRIORITY,		\
			NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_KPP_INIT)
