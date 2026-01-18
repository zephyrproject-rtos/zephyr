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

#include <zephyr/logging/log.h>
#include <cy_device_headers.h>
#include <infineon_kconfig.h>
#include <cy_tcpwm_pwm.h>
#include <cy_tcpwm.h>
#include <cy_gpio.h>
#include <cy_sysclk.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>

LOG_MODULE_REGISTER(pwm_ifx_tcpwm, CONFIG_PWM_LOG_LEVEL);

struct ifx_tcpwm_pwm_config {
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	TCPWM_Type * reg_base;
#else
	TCPWM_GRP_CNT_Type *reg_base;
#endif
	const struct pinctrl_dev_config *pcfg;
	bool resolution_32_bits;
	uint32_t tcpwm_index;
	uint32_t index;
};

struct ifx_tcpwm_pwm_data {
	struct ifx_cat1_clock clock;
};

static int ifx_tcpwm_pwm_init(const struct device *dev)
{
	const struct ifx_tcpwm_pwm_config *config = dev->config;
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
	};

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	status = Cy_TCPWM_PWM_Init(config->reg_base, config->tcpwm_index, &pwm_config);
#else
	status = IFX_TCPWM_PWM_Init(config->reg_base, &pwm_config);
#endif

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (status != CY_TCPWM_SUCCESS) {
		LOG_ERR("PWM init failed for counter %u: 0x%08x", config->tcpwm_index, status);
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
#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1)
	uint32_t ctrl_temp;

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
#else
	/* PSOC4 config */
	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED) {
		config->reg_base->CTRL |= TCPWM_CNT_CTRL_QUADRATURE_MODE_Msk;
	} else {
		config->reg_base->CTRL &= ~TCPWM_CNT_CTRL_QUADRATURE_MODE_Msk;
	}

	pwm_status = Cy_TCPWM_PWM_GetStatus(config->reg_base, config->tcpwm_index);
	if ((pwm_status & TCPWM_CNT_STATUS_RUNNING_Msk) == 0) {
		if ((period_cycles != 0) && (pulse_cycles != 0)) {
			Cy_TCPWM_PWM_SetPeriod0(config->reg_base, config->tcpwm_index,
						period_cycles - 1);
			Cy_TCPWM_PWM_SetCompare0(config->reg_base, config->tcpwm_index,
						 pulse_cycles);
		}
	}

	if (period_cycles == 0) {
		Cy_TCPWM_PWM_SetPeriod1(config->reg_base, config->tcpwm_index, 0);
		Cy_TCPWM_PWM_SetCompare0(config->reg_base, config->tcpwm_index, 0);
		Cy_TCPWM_TriggerCaptureOrSwap(config->reg_base, config->tcpwm_index);
	} else {
		Cy_TCPWM_PWM_SetPeriod1(config->reg_base, config->tcpwm_index, period_cycles - 1);
		Cy_TCPWM_PWM_SetCompare0(config->reg_base, config->tcpwm_index, pulse_cycles);
		Cy_TCPWM_TriggerCaptureOrSwap(config->reg_base, config->tcpwm_index);
	}

	Cy_TCPWM_PWM_Enable(config->reg_base, config->tcpwm_index);
	Cy_TCPWM_TriggerStart(config->reg_base, BIT(config->tcpwm_index));
#endif
	return 0;
}

static int ifx_tcpwm_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					    uint64_t *cycles)
{
	ARG_UNUSED(channel);

	struct ifx_tcpwm_pwm_data *const data = dev->data;
	const struct ifx_tcpwm_pwm_config *config = dev->config;
	en_clk_dst_t clk_connection;

	/* Determine tcpwm block number based on its resolution */
	uint32_t tcpwm_block = config->resolution_32_bits ? 0 : 1;

	/* Calculate clock connection based on TCPWM index */
	clk_connection = ifx_cat1_tcpwm_get_clock_index(tcpwm_block, config->index);

	*cycles = ifx_cat1_utils_peri_pclk_get_frequency(clk_connection, &data->clock);

	return 0;
}

static DEVICE_API(pwm, ifx_tcpwm_pwm_api) = {
	.set_cycles = ifx_tcpwm_pwm_set_cycles,
	.get_cycles_per_sec = ifx_tcpwm_pwm_get_cycles_per_sec,
};

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define COUNTER_PERI_CLOCK_INSTANCE(n) DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),
#else
#define COUNTER_PERI_CLOCK_INSTANCE(n)
#endif

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define PWM_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),                 \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	}
#elif defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
#define PWM_PERI_CLOCK_INIT(n)                                                                     \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	}
#endif

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
#define IFX_TCPWM_BASE_INIT(n) .reg_base = (TCPWM_Type *)(DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n))))
#else
#define IFX_TCPWM_BASE_INIT(n) .reg_base = (TCPWM_GRP_CNT_Type *)(DT_REG_ADDR(DT_INST_PARENT(n)))
#endif

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
#define TCPWM_PWM_IDX(n) .tcpwm_index = DT_NODE_CHILD_IDX(DT_INST_PARENT(n))
#else
#define TCPWM_PWM_IDX(n)                                                                           \
	.tcpwm_index =                                                                             \
		(DT_REG_ADDR(DT_INST_PARENT(n)) - DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))) /     \
		DT_REG_SIZE(DT_INST_PARENT(n))
#endif

#define INFINEON_TCPWM_PWM_INIT(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct ifx_tcpwm_pwm_data ifx_tcpwm_pwm##n##_data = {PWM_PERI_CLOCK_INIT(n)};       \
                                                                                                   \
	static const struct ifx_tcpwm_pwm_config pwm_tcpwm_config_##n = {                          \
		.tcpwm_index = (DT_REG_ADDR(DT_INST_PARENT(n)) -                                   \
				DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))) /                       \
			       DT_REG_SIZE(DT_INST_PARENT(n)),                                     \
		IFX_TCPWM_BASE_INIT(n),                                                            \
		TCPWM_PWM_IDX(n),                                                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.resolution_32_bits =                                                              \
			(DT_PROP(DT_INST_PARENT(n), resolution) == 32) ? true : false,             \
		.index = (DT_REG_ADDR(DT_INST_PARENT(n)) -                                         \
			  DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))) /                             \
			 DT_REG_SIZE(DT_INST_PARENT(n)),                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_tcpwm_pwm_init, NULL, &ifx_tcpwm_pwm##n##_data,               \
			      &pwm_tcpwm_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,        \
			      &ifx_tcpwm_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_TCPWM_PWM_INIT)
