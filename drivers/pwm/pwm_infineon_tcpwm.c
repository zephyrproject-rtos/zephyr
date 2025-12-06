/*
 * Copyright (c) 2025 Infineon Technologies AG,
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
#include <zephyr/drivers/timer/ifx_tcpwm.h>
#include <zephyr/dt-bindings/pwm/pwm_ifx_tcpwm.h>

#include <cy_tcpwm_pwm.h>
#include <cy_gpio.h>
#include <cy_sysclk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_ifx_tcpwm, CONFIG_PWM_LOG_LEVEL);

struct ifx_tcpwm_pwm_config {
	TCPWM_GRP_CNT_Type *reg_base;
	const struct pinctrl_dev_config *pcfg;
	bool resolution_32_bits;
	cy_en_divider_types_t divider_type;
	uint32_t divider_sel;
	uint32_t divider_val;
	uint32_t tcpwm_index;
};

static int ifx_tcpwm_pwm_init(const struct device *dev)
{
	const struct ifx_tcpwm_pwm_config *config = dev->config;
	cy_en_tcpwm_status_t status;
	int ret;
	uint32_t clk_connection;

	const cy_stc_tcpwm_pwm_config_t pwm_config = {
		.pwmMode = CY_TCPWM_PWM_MODE_PWM,
		.clockPrescaler = CY_TCPWM_PWM_PRESCALER_DIVBY_1,
		.pwmAlignment = CY_TCPWM_PWM_LEFT_ALIGN,
		.runMode = CY_TCPWM_PWM_CONTINUOUS,
		.countInputMode = CY_TCPWM_INPUT_LEVEL,
		.countInput = CY_TCPWM_INPUT_1,
		.enableCompareSwap = true,
		.enablePeriodSwap = true,
	};

	/* Configure PWM clock */
	Cy_SysClk_PeriphDisableDivider(config->divider_type, config->divider_sel);
	Cy_SysClk_PeriphSetDivider(config->divider_type, config->divider_sel, config->divider_val);
	Cy_SysClk_PeriphEnableDivider(config->divider_type, config->divider_sel);

	/* Calculate clock connection based on TCPWM index */
	if (config->resolution_32_bits) {
		clk_connection = PCLK_TCPWM0_CLOCK_COUNTER_EN0 + config->tcpwm_index;
	} else {
		clk_connection = PCLK_TCPWM0_CLOCK_COUNTER_EN256 + config->tcpwm_index;
	}

	Cy_SysClk_PeriphAssignDivider(clk_connection, config->divider_type, config->divider_sel);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
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

	const struct ifx_tcpwm_pwm_config *config = dev->config;

	*cycles = Cy_SysClk_PeriphGetFrequency(config->divider_type, config->divider_sel);

	return 0;
}

static DEVICE_API(pwm, ifx_tcpwm_pwm_api) = {
	.set_cycles = ifx_tcpwm_pwm_set_cycles,
	.get_cycles_per_sec = ifx_tcpwm_pwm_get_cycles_per_sec,
};

#define INFINEON_TCPWM_PWM_INIT(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct ifx_tcpwm_pwm_config pwm_tcpwm_config_##n = {                          \
		.reg_base = (TCPWM_GRP_CNT_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),                  \
		.tcpwm_index = (DT_REG_ADDR(DT_INST_PARENT(n)) -                                   \
				DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))) /                       \
			       DT_REG_SIZE(DT_INST_PARENT(n)),                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.resolution_32_bits =                                                              \
			(DT_PROP(DT_INST_PARENT(n), resolution) == 32) ? true : false,             \
		.divider_type = DT_PROP(DT_INST_PARENT(n), divider_type),                          \
		.divider_sel = DT_PROP(DT_INST_PARENT(n), divider_sel),                            \
		.divider_val = DT_PROP(DT_INST_PARENT(n), divider_val),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_tcpwm_pwm_init, NULL, NULL, &pwm_tcpwm_config_##n,            \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &ifx_tcpwm_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_TCPWM_PWM_INIT)
