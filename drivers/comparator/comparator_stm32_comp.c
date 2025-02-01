/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>

#include <stm32_ll_comp.h>
#include <stm32_ll_system.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stm32_comp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_comp

#define LL_COMP_INPUT_PLUS_IN0		LL_COMP_INPUT_PLUS_IO1
#define LL_COMP_INPUT_PLUS_IN1		LL_COMP_INPUT_PLUS_IO2

#define LL_COMP_INPUT_MINUS_IN0		LL_COMP_INPUT_MINUS_IO1
#define LL_COMP_INPUT_MINUS_IN1		LL_COMP_INPUT_MINUS_IO2

#define STM32_COMP_DT_INST_P_IN(inst)	\
	CONCAT(LL_COMP_INPUT_PLUS_, DT_INST_STRING_TOKEN(inst, positive_input))

#define STM32_COMP_DT_INST_N_IN(inst)	\
	CONCAT(LL_COMP_INPUT_MINUS_, DT_INST_STRING_TOKEN(inst, negative_input))

#define STM32_COMP_DT_INST_HYST_MODE(inst)	\
	CONCAT(LL_COMP_HYSTERESIS_, DT_INST_STRING_TOKEN(inst, hysteresis))

#define STM32_COMP_DT_INST_INV_OUT(inst)	\
	CONCAT(LL_COMP_OUTPUTPOL_, DT_INST_STRING_TOKEN(inst, invert_output))

#define STM32_COMP_DT_INST_BLANK_SEL(inst)	\
	CONCAT(LL_COMP_BLANKINGSRC_, DT_INST_STRING_TOKEN(inst, st_blank_sel))

#define STM32_COMP_DT_INST_LOCK(inst)		DT_INST_PROP(inst, st_lock_enable)

#define STM32_COMP_DT_MILLER_EFFECT_HOLD_ENABLE(inst)	\
	DT_INST_PROP(inst, st_miller_effect_hold_enable)

#define STM32_COMP_DT_EXTI_LINE_NUMBER(inst)	DT_INST_PROP(inst, st_exti_line)

#define STM32_COMP_DT_POWER_MODE(inst)	\
	CONCAT(LL_COMP_POWERMODE_, DT_INST_STRING_TOKEN(inst, st_power_mode))

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_comp)
#define STM32_COMP_DT_INST_COMP_CONFIG_INIT(inst)		\
	{							\
		.PowerMode = STM32_COMP_DT_POWER_MODE(inst),	\
		.InputPlus = STM32_COMP_DT_INST_P_IN(inst),	\
		.InputMinus = STM32_COMP_DT_INST_N_IN(inst),	\
		.InputHysteresis = STM32_COMP_DT_INST_HYST_MODE(inst), \
		.OutputPolarity = STM32_COMP_DT_INST_INV_OUT(inst), \
		.OutputBlankingSource = STM32_COMP_DT_INST_BLANK_SEL(inst), \
	}
#else /* st_stm32h7_comp */
#define STM32_COMP_DT_INST_COMP_CONFIG_INIT(inst)		\
	{							\
		.InputPlus = STM32_COMP_DT_INST_P_IN(inst),	\
		.InputMinus = STM32_COMP_DT_INST_N_IN(inst),	\
		.InputHysteresis = STM32_COMP_DT_INST_HYST_MODE(inst), \
		.OutputPolarity = STM32_COMP_DT_INST_INV_OUT(inst), \
		.OutputBlankingSource = STM32_COMP_DT_INST_BLANK_SEL(inst), \
	}
#endif /* st_stm32h7_comp */

struct stm32_comp_config {
	COMP_TypeDef *comp;
	const struct stm32_pclken *pclken;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_init)(void);
	const uint32_t irq_nr;
	const LL_COMP_InitTypeDef comp_config;
	const uint32_t exti_line_number;
	const bool lock_enable;
	const bool miller_effect_hold_enable;
};

struct stm32_comp_data {
	comparator_callback_t callback;
	void *user_data;
};

__maybe_unused static bool stm32_comp_is_resumed(void)
{
#if CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(DEVICE_DT_INST_GET(0), &state);
	return state == PM_DEVICE_STATE_ACTIVE;
#else
	return true;
#endif /* CONFIG_PM_DEVICE */
}

static int stm32_comp_get_output(const struct device *dev)
{
	const struct stm32_comp_config *cfg = dev->config;

	return LL_COMP_ReadOutputLevel(cfg->comp);
}

static int stm32_comp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct stm32_comp_config *cfg = dev->config;
	struct stm32_comp_data *data = dev->data;
	stm32_exti_trigger_type exti_trigger = 0U;
	int ret = 0;

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		exti_trigger = STM32_EXTI_TRIG_NONE;
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		exti_trigger = STM32_EXTI_TRIG_RISING;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		exti_trigger = STM32_EXTI_TRIG_FALLING;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		exti_trigger = STM32_EXTI_TRIG_BOTH;
		break;
	}

	irq_disable(cfg->irq_nr);
	LL_COMP_Disable(cfg->comp);

	ret = stm32_exti_enable(cfg->exti_line_number, exti_trigger,
				STM32_EXTI_MODE_IT);
	if (ret != 0) {
		LOG_DBG("%s: EXTI init failed (%d)", dev->name, ret);
		return ret;
	}

	if (stm32_comp_is_resumed()) {
		LL_COMP_Enable(cfg->comp);
	}

	if (data->callback != NULL) {
		irq_enable(cfg->irq_nr);
	}

	return ret;
}

static int stm32_comp_trigger_is_pending(const struct device *dev)
{
	const struct stm32_comp_config *cfg = dev->config;

	if (stm32_exti_is_pending(cfg->exti_line_number)) {
		stm32_exti_clear_pending(cfg->exti_line_number);
		return 1;
	}
	return 0;
}

static int stm32_comp_set_trigger_callback(const struct device *dev, comparator_callback_t callback,
					   void *user_data)
{
	const struct stm32_comp_config *cfg = dev->config;
	struct stm32_comp_data *data = dev->data;

	irq_disable(cfg->irq_nr);

	data->callback = callback;
	data->user_data = user_data;

	irq_enable(cfg->irq_nr);

	if (data->callback != NULL && stm32_comp_trigger_is_pending(dev)) {
		callback(dev, user_data);
	}

	return 0;
}

static DEVICE_API(comparator, stm32_comp_comp_api) = {
	.get_output = stm32_comp_get_output,
	.set_trigger = stm32_comp_set_trigger,
	.set_trigger_callback = stm32_comp_set_trigger_callback,
	.trigger_is_pending = stm32_comp_trigger_is_pending,
};

static int stm32_comp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct stm32_comp_config *cfg = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		if (cfg->lock_enable) {
			LL_COMP_Lock(cfg->comp);
		}
		LL_COMP_Enable(cfg->comp);
	}

#if CONFIG_PM_DEVICE
	if (action == PM_DEVICE_ACTION_SUSPEND) {
		LL_COMP_Disable(cfg->comp);
	}
#endif

	return 0;
}

static void stm32_comp_irq_handler(const struct device *dev)
{
	const struct stm32_comp_config *cfg = dev->config;
	struct stm32_comp_data *data = dev->data;

	if (stm32_exti_is_pending(cfg->exti_line_number)) {
		stm32_exti_clear_pending(cfg->exti_line_number);
	}

	if (data->callback == NULL) {
		return;
	}

	data->callback(dev, data->user_data);
}

static int stm32_comp_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_comp_config *cfg = dev->config;
	int ret = 0;

	ret = device_is_ready(clk);
	if (!ret) {
		LOG_ERR("%s clock control device not ready (%d)", dev->name, ret);
		return ret;
	}

	/* Enable COMP bus clock */
	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret != 0) {
		LOG_ERR("%s clock op failed (%d)", dev->name, ret);
		return ret;
	}

	/* Enable COMP clock source */
	clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL);
	if (ret != 0) {
		LOG_ERR("%s clock configure failed (%d)", dev->name, ret);
		return ret;
	}

	/* Configure COMP inputs as specified in Device Tree, if any */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		/*
		 * If the COMP is used only with internal channels, then no pinctrl is
		 * provided in Device Tree, and pinctrl_apply_state returns -ENOENT,
		 * but this should not be treated as an error.
		 */
		LOG_ERR("%s pinctrl setup failed (%d)", dev->name, ret);
		return ret;
	}

	ret = LL_COMP_Init(cfg->comp, (LL_COMP_InitTypeDef *)&cfg->comp_config);
	if (ret != 0) {
		LOG_ERR("COMP instance is locked (%d)", ret);
		return ret;
	}

	if (cfg->miller_effect_hold_enable) {
#if defined(CONFIG_COMPARATOR_STM32_COMP_MILLER_EFFECT_HANDLING)
		SET_BIT(cfg->comp->CSR, BIT(1U));
#endif /* CONFIG_COMPARATOR_STM32_COMP_MILLER_EFFECT_HANDLING */
	}

	cfg->irq_init();

	return pm_device_driver_init(dev, stm32_comp_pm_callback);
}

#define STM32_COMP_IRQ_HANDLER_SYM(inst)	_CONCAT(stm32_comp_irq_init, inst)

#define STM32_COMP_IRQ_HANDLER_DEFINE(inst)				\
	static void STM32_COMP_IRQ_HANDLER_SYM(inst)(void)		\
	{							\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), \
			stm32_comp_irq_handler, DEVICE_DT_INST_GET(inst), 0); \
		irq_enable(DT_INST_IRQN(inst));			\
	}

#define STM32_COMP_DEVICE(inst)						\
	static const struct stm32_pclken comp_clk[] = STM32_DT_INST_CLOCKS(inst); \
	PINCTRL_DT_INST_DEFINE(inst);				\
	static struct stm32_comp_data _CONCAT(data, inst);	\
	STM32_COMP_IRQ_HANDLER_DEFINE(inst)			\
	static const struct stm32_comp_config _CONCAT(config, inst) = { \
		.comp = (COMP_TypeDef *)DT_INST_REG_ADDR(inst),	\
		.pclken = comp_clk,				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	\
		.irq_init = STM32_COMP_IRQ_HANDLER_SYM(inst),	\
		.irq_nr = DT_INST_IRQN(inst),			\
		.comp_config = STM32_COMP_DT_INST_COMP_CONFIG_INIT(inst), \
		.exti_line_number = STM32_COMP_DT_EXTI_LINE_NUMBER(inst), \
		.lock_enable = STM32_COMP_DT_INST_LOCK(inst),	\
		.miller_effect_hold_enable = STM32_COMP_DT_MILLER_EFFECT_HOLD_ENABLE(inst) \
	};							\
	PM_DEVICE_DT_INST_DEFINE(inst, stm32_comp_pm_callback); \
	DEVICE_DT_INST_DEFINE(inst, stm32_comp_init, PM_DEVICE_DT_INST_GET(inst), \
				  &_CONCAT(data, inst), &_CONCAT(config, inst), POST_KERNEL, \
				  CONFIG_COMPARATOR_INIT_PRIORITY, &stm32_comp_comp_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_COMP_DEVICE)
