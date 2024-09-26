/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief ADC driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_pwm

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>

#include <cy_tcpwm_pwm.h>
#include <cy_gpio.h>
#include <cy_sysclk.h>
#include <cyhal_hw_resources.h>
#include <cyhal_hw_types.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_ifx_cat1, CONFIG_PWM_LOG_LEVEL);

#define PWM_REG_BASE TCPWM0

struct ifx_cat1_pwm_data {
	uint32_t pwm_num;
};

struct ifx_cat1_pwm_config {
	TCPWM_GRP_CNT_Type *reg_addr;
	const struct pinctrl_dev_config *pcfg;
	bool resolution_32_bits;
	cy_en_divider_types_t divider_type;
	uint32_t divider_sel;
	uint32_t divider_val;
};

static int ifx_cat1_pwm_init(const struct device *dev)
{
	struct ifx_cat1_pwm_data *data = dev->data;
	const struct ifx_cat1_pwm_config *config = dev->config;
	cy_en_tcpwm_status_t status;
	int ret;
	uint32_t addr_offset = (uint32_t)config->reg_addr - TCPWM0_BASE;
	uint32_t clk_connection;

	const cy_stc_tcpwm_pwm_config_t pwm_config = {
		.pwmMode = CY_TCPWM_PWM_MODE_PWM,
		.clockPrescaler = CY_TCPWM_PWM_PRESCALER_DIVBY_1,
		.pwmAlignment = CY_TCPWM_PWM_LEFT_ALIGN,
		.runMode = CY_TCPWM_PWM_CONTINUOUS,
		.countInputMode = CY_TCPWM_INPUT_LEVEL,
		.countInput = CY_TCPWM_INPUT_1,
	};

	/* Configure PWM clock */
	Cy_SysClk_PeriphDisableDivider(config->divider_type, config->divider_sel);
	Cy_SysClk_PeriphSetDivider(config->divider_type, config->divider_sel, config->divider_val);
	Cy_SysClk_PeriphEnableDivider(config->divider_type, config->divider_sel);

	/* This is very specific to the cyw920829m2evk_02 and may need to be modified
	 * for other boards.
	 */
	if (addr_offset < sizeof(TCPWM_GRP_Type)) {
		clk_connection =
			PCLK_TCPWM0_CLOCK_COUNTER_EN0 + (addr_offset / sizeof(TCPWM_GRP_CNT_Type));
	} else {
		addr_offset -= sizeof(TCPWM_GRP_Type);
		clk_connection = PCLK_TCPWM0_CLOCK_COUNTER_EN256 +
				 (addr_offset / sizeof(TCPWM_GRP_CNT_Type));
	}
	Cy_SysClk_PeriphAssignDivider(clk_connection, config->divider_type, config->divider_sel);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Configure the TCPWM to be a PWM */
	data->pwm_num += addr_offset / sizeof(TCPWM_GRP_CNT_Type);
	status = Cy_TCPWM_PWM_Init(PWM_REG_BASE, data->pwm_num, &pwm_config);
	if (status != CY_TCPWM_SUCCESS) {
		return -ENOTSUP;
	}

	return 0;
}

static int ifx_cat1_pwm_set_cycles(const struct device *dev, uint32_t channel,
				   uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	struct ifx_cat1_pwm_data *data = dev->data;
	const struct ifx_cat1_pwm_config *config = dev->config;

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

	if ((period_cycles == 0) || (pulse_cycles == 0)) {
		Cy_TCPWM_PWM_Disable(PWM_REG_BASE, data->pwm_num);
	} else {
		Cy_TCPWM_PWM_SetPeriod0(PWM_REG_BASE, data->pwm_num, period_cycles);
		Cy_TCPWM_PWM_SetCompare0Val(PWM_REG_BASE, data->pwm_num, pulse_cycles);

		if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED) {
			config->reg_addr->CTRL &= ~TCPWM_GRP_CNT_V2_CTRL_QUAD_ENCODING_MODE_Msk;
			config->reg_addr->CTRL |= _VAL2FLD(TCPWM_GRP_CNT_V2_CTRL_QUAD_ENCODING_MODE,
							   CY_TCPWM_PWM_INVERT_ENABLE);
		}

		/* TODO: Add 2-bit field to top 8 bits of pwm_flags_t to set this.
		 * #define    CY_TCPWM_PWM_OUTPUT_HIGHZ    (0U)
		 * #define    CY_TCPWM_PWM_OUTPUT_RETAIN   (1U)
		 * #define    CY_TCPWM_PWM_OUTPUT_LOW      (2U)
		 * #define    CY_TCPWM_PWM_OUTPUT_HIGH     (3U)
		 * if ((flags & __) == __) {
		 *	config->reg_addr->CTRL &= ~TCPWM_GRP_CNT_V2_CTRL_PWM_DISABLE_MODE_Msk;
		 *	config->reg_addr->CTRL |= _VAL2FLD(TCPWM_GRP_CNT_V2_CTRL_PWM_DISABLE_MODE,
		 *					   __);
		 * }
		 */

		/* Enable the TCPWM for PWM mode of operation */
		Cy_TCPWM_PWM_Enable(PWM_REG_BASE, data->pwm_num);

		/* Start the TCPWM block */
		Cy_TCPWM_TriggerStart_Single(PWM_REG_BASE, data->pwm_num);
	}

	return 0;
}

static int ifx_cat1_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					   uint64_t *cycles)
{
	const struct ifx_cat1_pwm_config *config = dev->config;

	*cycles = Cy_SysClk_PeriphGetFrequency(config->divider_type, config->divider_sel);

	return 0;
}

static const struct pwm_driver_api ifx_cat1_pwm_api = {
	.set_cycles = ifx_cat1_pwm_set_cycles,
	.get_cycles_per_sec = ifx_cat1_pwm_get_cycles_per_sec,
};

#define INFINEON_CAT1_PWM_INIT(n)                                                                  \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct ifx_cat1_pwm_data pwm_cat1_data_##n;                                         \
                                                                                                   \
	static struct ifx_cat1_pwm_config pwm_cat1_config_##n = {                                  \
		.reg_addr = (TCPWM_GRP_CNT_Type *)DT_INST_REG_ADDR(n),                             \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.resolution_32_bits = (DT_INST_PROP(n, resolution) == 32) ? true : false,          \
		.divider_type = DT_INST_PROP(n, divider_type),                                     \
		.divider_sel = DT_INST_PROP(n, divider_sel),                                       \
		.divider_val = DT_INST_PROP(n, divider_val),                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_cat1_pwm_init, NULL, &pwm_cat1_data_##n,                      \
			      &pwm_cat1_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,         \
			      &ifx_cat1_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_PWM_INIT)
