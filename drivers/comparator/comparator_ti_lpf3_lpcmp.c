/*
 * Copyright 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/pinctrl.h>

#include <driverlib/evtsvt.h>
#include <driverlib/lpcmp.h>

#include <inc/hw_memmap.h>

LOG_MODULE_REGISTER(ti_lpf3_lpcmp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT ti_lpf3_lpcmp

#define TI_LPF3_LPCMP_STARTUP_TIME_US  (100000U)
#define TI_LPF3_LPCMP_SETTLING_TIME_US (110U)

/* Array to map the devicetree enum values for comparator positive input to hal_ti macros. */

static const uint32_t ti_lpf3_lpcmp_positive_input_map[] = {
	LPCMP_POS_INPUT_A1,
	LPCMP_POS_INPUT_A2,
	LPCMP_POS_INPUT_A3,
	LPCMP_POS_INPUT_VDDS,
};

/* Array to map the devicetree enum values for comparator negative input to hal_ti macros. */

static const uint32_t ti_lpf3_lpcmp_negative_input_map[] = {
	LPCMP_NEG_INPUT_A2,
	LPCMP_NEG_INPUT_A3,
	LPCMP_NEG_INPUT_VDDS,
	LPCMP_NEG_INPUT_VDDD,
};

/* Array to map the devicetree enum values for volatage division ratio to hal_ti macros. */

static const uint32_t ti_lpf3_lpcmp_voltage_divider_map[] = {
	LPCMP_DIVISION_FACTOR_1_1, LPCMP_DIVISION_FACTOR_3_4, LPCMP_DIVISION_FACTOR_1_2,
	LPCMP_DIVISION_FACTOR_1_3, LPCMP_DIVISION_FACTOR_1_4};

/* Array to map the devicetree enum values for voltage division path to hal_ti macros. */

static const uint32_t ti_lpf3_lpcmp_voltage_divider_input_select[] = {
	LPCMP_DIVISION_PATH_N,
	LPCMP_DIVISION_PATH_P,
};

struct ti_lpf3_lpcmp_config {
	uint32_t base;
	uint8_t positive_mux_input;
	uint8_t negative_mux_input;
	bool enable_output_pin;
	uint8_t input_division_factor;
	uint8_t division_input;
	uint32_t irq_line;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	const struct device *gpio_dev;
};

struct ti_lpf3_lpcmp_data {
	uint32_t trigger_config;
	comparator_callback_t callback;
	void *user_data;
	bool trigger_enabled;
};

static void ti_lpf3_lpcmp_isr(const struct device *dev)
{
	struct ti_lpf3_lpcmp_data *data = dev->data;

	LPCMPClearEvent();

	if (data->callback) {
		data->callback(dev, data->user_data);
	}
}

static int ti_lpf3_lpcmp_get_output(const struct device *dev)
{
	int output;

	output = (int)LPCMPIsOutputHigh();

	return output;
}

static int ti_lpf3_lpcmp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	struct ti_lpf3_lpcmp_data *data = dev->data;

	unsigned int key;

	key = irq_lock();

	data->trigger_enabled = (trigger != COMPARATOR_TRIGGER_NONE);
	data->trigger_config = 0;

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		data->trigger_config = LPCMP_POLARITY_RISE;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		data->trigger_config = LPCMP_POLARITY_FALL;
		break;
	default:
		LOG_ERR("Invalid comparator trigger type.");
		irq_unlock(key);
		return -ENOTSUP;
	}

	LPCMPDisableEvent();
	LPCMPDisable();

	if (trigger == COMPARATOR_TRIGGER_NONE) {
		EVTSVTConfigureEvent(EVTSVT_SUB_CPUIRQ1, 0);
		LPCMPClearEvent();
	} else {
		LPCMPSetPolarity(data->trigger_config);
		LPCMPEnableWakeup();
		EVTSVTConfigureEvent(EVTSVT_SUB_CPUIRQ1, EVTSVT_PUB_AON_LPMCMP_IRQ);
	}

	LPCMPEnable();
	k_busy_wait(TI_LPF3_LPCMP_SETTLING_TIME_US);
	LPCMPClearEvent();
	k_busy_wait(TI_LPF3_LPCMP_SETTLING_TIME_US);

	if (trigger != COMPARATOR_TRIGGER_NONE && data->callback) {
		LPCMPEnableEvent();
	}

	irq_unlock(key);

	return 0;
}

static int ti_lpf3_lpcmp_trigger_is_pending(const struct device *dev)
{
	const struct ti_lpf3_lpcmp_config *config = dev->config;
	struct ti_lpf3_lpcmp_data *data = dev->data;

	if (LPCMPEventFlagIsSet()) {
		LPCMPClearEvent();
		return data->trigger_enabled ? 1 : 0;
	}

	return 0;
}

static int ti_lpf3_lpcmp_set_trigger_callback(const struct device *dev,
					      comparator_callback_t callback, void *user_data)
{
	struct ti_lpf3_lpcmp_data *data = dev->data;

	unsigned int lock;

	lock = irq_lock();

	data->callback = callback;
	data->user_data = user_data;

	irq_unlock(lock);

	return 0;
}

static int ti_lpf3_lpcmp_init(const struct device *dev)
{
	const struct ti_lpf3_lpcmp_config *config = dev->config;
	int ret;

	LPCMPDisableEvent();

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	if (ret) {
		LOG_ERR("Error in applying pin states for comparator !");
		return ret;
	}

	LPCMPSelectPositiveInput(ti_lpf3_lpcmp_positive_input_map[config->positive_mux_input]);
	LPCMPSelectNegativeInput(ti_lpf3_lpcmp_negative_input_map[config->negative_mux_input]);

	if (config->enable_output_pin) {
		LPCMPEnableOutputAtPin();
	} else {
		LPCMPDisableOutputAtPin();
	}

	LPCMPSetDividerPath(ti_lpf3_lpcmp_voltage_divider_input_select[config->division_input]);
	LPCMPSetDividerRatio(ti_lpf3_lpcmp_voltage_divider_map[config->input_division_factor]);

	config->irq_config_func(dev);

	LPCMPEnable();
	k_busy_wait(TI_LPF3_LPCMP_STARTUP_TIME_US);
	LPCMPClearEvent();
	k_busy_wait(TI_LPF3_LPCMP_SETTLING_TIME_US);

	return 0;
}

static DEVICE_API(comparator, ti_lpf3_lpcmp_api) = {
	.get_output = ti_lpf3_lpcmp_get_output,
	.set_trigger = ti_lpf3_lpcmp_set_trigger,
	.set_trigger_callback = ti_lpf3_lpcmp_set_trigger_callback,
	.trigger_is_pending = ti_lpf3_lpcmp_trigger_is_pending,
};

#define TI_LPF3_LPCMP_DEVICE_INIT(inst)                                                            \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct ti_lpf3_lpcmp_data _CONCAT(data, inst) = {                                   \
		.trigger_config = 0,                                                               \
	};                                                                                         \
                                                                                                   \
	static void _CONCAT(ti_lpf3_lpcmp_irq_config, inst)(const struct device *dev)              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), ti_lpf3_lpcmp_isr,    \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static const struct ti_lpf3_lpcmp_config _CONCAT(config, inst) = {                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.positive_mux_input = DT_ENUM_IDX_OR(DT_DRV_INST(inst), positive_mux_input, 0),    \
		.negative_mux_input = DT_ENUM_IDX_OR(DT_DRV_INST(inst), negative_mux_input, 0),    \
		.enable_output_pin = DT_INST_PROP_OR(inst, enable_output_pin, 0),                  \
		.input_division_factor =                                                           \
			DT_ENUM_IDX_OR(DT_DRV_INST(inst), input_division_factor, 0),               \
		.division_input = DT_ENUM_IDX_OR(DT_DRV_INST(inst), division_input, 0),            \
		.gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)),                                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.irq_config_func = _CONCAT(ti_lpf3_lpcmp_irq_config, inst),                        \
		.irq_line = DT_INST_IRQN(inst),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ti_lpf3_lpcmp_init, NULL, &_CONCAT(data, inst),                \
			      &_CONCAT(config, inst), POST_KERNEL,                                 \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &ti_lpf3_lpcmp_api);

DT_INST_FOREACH_STATUS_OKAY(TI_LPF3_LPCMP_DEVICE_INIT)
