/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * PWM driver for Infineon MCUs using the TCPWM block.
 */

#define DT_DRV_COMPAT infineon_tcpwm_pwm

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>

#include <infineon_kconfig.h>
#include <zephyr/drivers/timer/ifx_tcpwm.h>
#include <zephyr/dt-bindings/pwm/pwm_ifx_tcpwm.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>

#include <cy_tcpwm_pwm.h>
#include <cy_gpio.h>
#include <cy_sysclk.h>

#include <zephyr/logging/log.h>

#ifdef CONFIG_PWM_EVENT
#include <zephyr/drivers/pwm/pwm_utils.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

/* Infineon TCPWM PWM channels are single-channel */
#define IFX_TCPWM_PWM_CH 0

#endif /* CONFIG_PWM_EVENT */

LOG_MODULE_REGISTER(pwm_ifx_tcpwm, CONFIG_PWM_LOG_LEVEL);

struct ifx_tcpwm_pwm_config {
	TCPWM_GRP_CNT_Type *reg_base;
	const struct pinctrl_dev_config *pcfg;
	bool resolution_32_bits;
	uint32_t tcpwm_index;
	uint32_t index;
	uint32_t clk_dst;
};

struct ifx_tcpwm_pwm_data {
	struct ifx_cat1_clock clock;
#ifdef CONFIG_PWM_EVENT
	sys_slist_t event_callbacks;
	struct k_spinlock lock;
#endif /* CONFIG_PWM_EVENT */
};

static int ifx_tcpwm_pwm_init(const struct device *dev)
{
	const struct ifx_tcpwm_pwm_config *config = dev->config;
	struct ifx_tcpwm_pwm_data *const data = dev->data;

	cy_en_tcpwm_status_t status;
	int ret;

	const cy_stc_tcpwm_pwm_config_t pwm_config = {
		.pwmMode = CY_TCPWM_PWM_MODE_PWM,
		.clockPrescaler = CY_TCPWM_PWM_PRESCALER_DIVBY_1,
		.pwmAlignment = CY_TCPWM_PWM_LEFT_ALIGN,
		.runMode = CY_TCPWM_PWM_CONTINUOUS,
		.countInputMode = CY_TCPWM_INPUT_LEVEL,
		.countInput = CY_TCPWM_INPUT_1,
		.enableCompareSwap = true,
		.enablePeriodSwap = true,
		.line_out_sel = CY_TCPWM_OUTPUT_PWM_SIGNAL,
		.linecompl_out_sel = CY_TCPWM_OUTPUT_INVERTED_PWM_SIGNAL};

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Connect this TCPWM to the peripheral clock */
	status = ifx_cat1_utils_peri_pclk_assign_divider(config->clk_dst, &data->clock);
	if (status != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	/* Configure the TCPWM to be a PWM */
	status = IFX_TCPWM_PWM_Init(config->reg_base, &pwm_config);
	if (status != CY_TCPWM_SUCCESS) {
		return -ENOTSUP;
	}

	return 0;
}

static int ifx_tcpwm_pwm_set_cycles(const struct device *dev, uint32_t channel,
				    uint32_t period_cycles, uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	ARG_UNUSED(channel);

	const struct ifx_tcpwm_pwm_config *config = dev->config;
	uint32_t pwm_status;
	uint32_t ctrl_temp;

	if (!config->resolution_32_bits &&
	    ((period_cycles > UINT16_MAX) || (pulse_cycles > UINT16_MAX))) {
		/* 16-bit resolution */
		if (period_cycles > UINT16_MAX) {
			LOG_ERR("Period cycles more than 16-bits (%u)", period_cycles);
		}
		if (pulse_cycles > UINT16_MAX) {
			LOG_ERR("Pulse cycles more than 16-bits (%u)", pulse_cycles);
		}
		return -EINVAL;
	}
	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED) {
		config->reg_base->CTRL |= TCPWM_GRP_CNT_V2_CTRL_QUAD_ENCODING_MODE_Msk;
	} else {
		config->reg_base->CTRL &= ~TCPWM_GRP_CNT_V2_CTRL_QUAD_ENCODING_MODE_Msk;
	}

	ctrl_temp = config->reg_base->CTRL & ~TCPWM_GRP_CNT_V2_CTRL_PWM_DISABLE_MODE_Msk;

	config->reg_base->CTRL = ctrl_temp | _VAL2FLD(TCPWM_GRP_CNT_V2_CTRL_PWM_DISABLE_MODE,
						      (flags & PWM_IFX_TCPWM_OUTPUT_MASK) >>
							      PWM_IFX_TCPWM_OUTPUT_POS);

	/* If the PWM is not yet running, write the period and compare directly pwm won't start
	 * correctly.
	 */
	pwm_status = IFX_TCPWM_PWM_GetStatus(config->reg_base);
	if ((pwm_status & TCPWM_GRP_CNT_V2_STATUS_RUNNING_Msk) == 0) {
		if ((period_cycles != 0) && (pulse_cycles != 0)) {
			IFX_TCPWM_PWM_SetPeriod0(config->reg_base, period_cycles - 1);
			IFX_TCPWM_PWM_SetCompare0Val(config->reg_base, pulse_cycles);
		}
	}

	/* Special case, if period_cycles is 0, set the period and compare to zero.  If we were to
	 * disable the PWM, the output would be set to High-Z, wheras this will set the output to
	 * the zero duty cycle state instead.
	 */
	if (period_cycles == 0) {
		IFX_TCPWM_PWM_SetPeriod1(config->reg_base, 0);
		IFX_TCPWM_PWM_SetCompare0BufVal(config->reg_base, 0);
		IFX_TCPWM_TriggerCaptureOrSwap_Single(config->reg_base);
	} else {
		/* Update period and compare values using buffer registers so the new values take
		 * effect on the next TC event.  This prevents glitches in PWM output depending on
		 * where in the PWM cycle the update occurs.
		 */
		IFX_TCPWM_PWM_SetPeriod1(config->reg_base, period_cycles - 1);
		IFX_TCPWM_PWM_SetCompare0BufVal(config->reg_base, pulse_cycles);

		/* Trigger the swap by writing to the SW trigger command register.
		 */
		IFX_TCPWM_TriggerCaptureOrSwap_Single(config->reg_base);
	}
	/* Enable the TCPWM for PWM mode of operation */
	IFX_TCPWM_PWM_Enable(config->reg_base);

	/* Start the TCPWM block */
	IFX_TCPWM_TriggerStart_Single(config->reg_base);

	return 0;
}

static int ifx_tcpwm_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					    uint64_t *cycles)
{
	ARG_UNUSED(channel);

	struct ifx_tcpwm_pwm_data *const data = dev->data;
	const struct ifx_tcpwm_pwm_config *config = dev->config;

	*cycles = ifx_cat1_utils_peri_pclk_get_frequency(config->clk_dst, &data->clock);

	return 0;
}

#ifdef CONFIG_PWM_EVENT
static void ifx_tcpwm_pwm_isr(const struct device *dev)
{
	const struct ifx_tcpwm_pwm_config *config = dev->config;
	struct ifx_tcpwm_pwm_data *data = dev->data;
	uint32_t pending;

	pending = IFX_TCPWM_GetInterruptStatusMasked(config->reg_base);
	IFX_TCPWM_ClearInterrupt(config->reg_base, pending);

	if ((pending & CY_TCPWM_INT_ON_TC) != 0) {
		pwm_fire_event_callbacks(&data->event_callbacks, dev, IFX_TCPWM_PWM_CH,
					 PWM_EVENT_TYPE_PERIOD);
	}

	if ((pending & CY_TCPWM_INT_ON_CC0) != 0) {
		pwm_fire_event_callbacks(&data->event_callbacks, dev, IFX_TCPWM_PWM_CH,
					 PWM_EVENT_TYPE_COMPARE_CAPTURE);
	}
}

static void ifx_tcpwm_pwm_update_interrupts(const struct device *dev)
{
	const struct ifx_tcpwm_pwm_config *config = dev->config;
	struct ifx_tcpwm_pwm_data *data = dev->data;
	struct pwm_event_callback *cb;
	struct pwm_event_callback *tmp;
	uint32_t mask = CY_TCPWM_INT_NONE;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->event_callbacks, cb, tmp, node) {
		if ((cb->event_mask & PWM_EVENT_TYPE_PERIOD) != 0) {
			mask |= CY_TCPWM_INT_ON_TC;
		}
		if ((cb->event_mask & PWM_EVENT_TYPE_COMPARE_CAPTURE) != 0) {
			mask |= CY_TCPWM_INT_ON_CC0;
		}
	}

	IFX_TCPWM_ClearInterrupt(config->reg_base, mask);
	IFX_TCPWM_SetInterruptMask(config->reg_base, mask);
}

static int ifx_tcpwm_pwm_manage_event_callback(const struct device *dev,
					       struct pwm_event_callback *callback, bool set)
{
	struct ifx_tcpwm_pwm_data *data = dev->data;
	int ret;

	ret = pwm_manage_event_callback(&data->event_callbacks, callback, set);
	if (ret < 0) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		ifx_tcpwm_pwm_update_interrupts(dev);
	}

	return 0;
}
#endif /* CONFIG_PWM_EVENT */

static DEVICE_API(pwm, ifx_tcpwm_pwm_api) = {
	.set_cycles = ifx_tcpwm_pwm_set_cycles,
	.get_cycles_per_sec = ifx_tcpwm_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_EVENT
	.manage_event_callback = ifx_tcpwm_pwm_manage_event_callback,
#endif /* CONFIG_PWM_EVENT */
};

/*
 * Initialize the peripheral clock divider from devicetree.
 *
 * PSoC Edge SoCs require an extra peri_group index argument to
 * IFX_CAT1_PERIPHERAL_GROUP_ADJUST, so the two variants are selected
 * at compile time based on CONFIG_SOC_FAMILY_INFINEON_EDGE.
 */
#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define PWM_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),                 \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	}
#else
#define PWM_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	}
#endif

/*
 * Per-instance wrapper init for PWM event support.
 *
 * IRQ_CONNECT requires compile-time constant arguments derived from the
 * devicetree instance index (n).  This macro generates a wrapper that
 * calls the main init function first, configures the interrupt.  This avoids
 * duplicating the bulk ofg the init logic for every pwm instance.
 */
#define INFINEON_TCPWM_PWM_IRQ_INIT(n)                                                             \
	static int ifx_tcpwm_pwm_init_##n(const struct device *dev)                                \
	{                                                                                          \
		int ret;                                                                           \
                                                                                                   \
		ret = ifx_tcpwm_pwm_init(dev);                                                     \
		if (ret < 0) {                                                                     \
			return ret;                                                                \
		}                                                                                  \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),       \
			    ifx_tcpwm_pwm_isr, DEVICE_DT_INST_GET(n), 0);                          \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                            \
		return 0;                                                                          \
	}

/* clang-format off */
/*
 * Per-instance device instantiation macro.
 *
 * Defines pinctrl state, mutable driver data, immutable config, and
 * registers the device.  When CONFIG_PWM_EVENT is enabled, the IRQ
 * wrapper init function is also emitted via INFINEON_TCPWM_PWM_IRQ_INIT.
 *
 * In the DEVICE_DT_INST_DEFINE below, the COND_CODE_1 macro is used to
 * use the wrapper init function when CONFIG_PWM_EVENT is enabled and use
 * the main init function otherwise.
 */
#define INFINEON_TCPWM_PWM_INIT(n)                                                                 \
	IF_ENABLED(CONFIG_PWM_EVENT, (INFINEON_TCPWM_PWM_IRQ_INIT(n)))                             \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct ifx_tcpwm_pwm_data ifx_tcpwm_pwm##n##_data = {PWM_PERI_CLOCK_INIT(n)};       \
                                                                                                   \
	static const struct ifx_tcpwm_pwm_config pwm_tcpwm_config_##n = {                          \
		.reg_base = (TCPWM_GRP_CNT_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),                  \
		.tcpwm_index = (DT_REG_ADDR(DT_INST_PARENT(n)) -                                   \
				DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))) /                       \
			       DT_REG_SIZE(DT_INST_PARENT(n)),                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.resolution_32_bits =                                                              \
			(DT_PROP(DT_INST_PARENT(n), resolution) == 32) ? true : false,             \
		.index = (DT_REG_ADDR(DT_INST_PARENT(n)) -                                         \
			  DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))) /                             \
			 DT_REG_SIZE(DT_INST_PARENT(n)),                                           \
		.clk_dst = DT_PROP(DT_INST_PARENT(n), clk_dst),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(                                                                     \
		n, COND_CODE_1(CONFIG_PWM_EVENT, (ifx_tcpwm_pwm_init_##n), (ifx_tcpwm_pwm_init)),  \
		NULL, &ifx_tcpwm_pwm##n##_data, &pwm_tcpwm_config_##n, POST_KERNEL,                \
		CONFIG_PWM_INIT_PRIORITY, &ifx_tcpwm_pwm_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(INFINEON_TCPWM_PWM_INIT)
