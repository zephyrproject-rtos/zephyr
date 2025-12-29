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
LOG_MODULE_REGISTER(pwm_ifx_tcpwm, CONFIG_PWM_LOG_LEVEL);

struct ifx_tcpwm_pwm_config {
	TCPWM_Type *reg_base;
	const struct pinctrl_dev_config *pcfg;
	bool resolution_32_bits;
	uint32_t tcpwm_index;
	uint32_t index;
	uint32_t clk_dst;
};

struct ifx_tcpwm_pwm_data {
	struct ifx_cat1_clock clock;
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
#if !defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
		.line_out_sel = CY_TCPWM_OUTPUT_PWM_SIGNAL,
		.linecompl_out_sel = CY_TCPWM_OUTPUT_INVERTED_PWM_SIGNAL
#endif
	};

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
	status = Cy_TCPWM_PWM_Init(config->reg_base, config->tcpwm_index, &pwm_config);
	if (status != CY_TCPWM_SUCCESS) {
		LOG_ERR("PWM init failed for counter %u: 0x%08x", config->tcpwm_index, status);
		return -ENOTSUP;
	}

	return 0;
}

/* Common PWM status checking */
static inline bool is_ifx_tcpwm_pwm_not_running(const struct ifx_tcpwm_pwm_config *config)
{
	uint32_t pwm_status = Cy_TCPWM_PWM_GetStatus(config->reg_base, config->tcpwm_index);

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	return (pwm_status & CY_TCPWM_PWM_STATUS_COUNTER_RUNNING) == 0;
#else
	return (pwm_status & TCPWM_GRP_CNT_V2_STATUS_RUNNING_Msk) == 0;
#endif
}

/* Set period value */
static inline void ifx_tcpwm_pwm_set_period(const struct ifx_tcpwm_pwm_config *config,
					    uint32_t period_cycles)
{
	Cy_TCPWM_PWM_SetPeriod1(config->reg_base, config->tcpwm_index,
				period_cycles > 0 ? period_cycles - 1 : 0);
}

/* Set compare value */
static inline void ifx_tcpwm_pwm_set_compare(const struct ifx_tcpwm_pwm_config *config,
					     uint32_t compare_value)
{
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	Cy_TCPWM_PWM_SetCompare1(config->reg_base, config->tcpwm_index, compare_value);
#else
	Cy_TCPWM_PWM_SetCompare0BufVal(config->reg_base, config->tcpwm_index, compare_value);
#endif
}

/* Trigger capture/swap */
static inline void ifx_tcpwm_trigger_swap(const struct ifx_tcpwm_pwm_config *config)
{
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	Cy_TCPWM_TriggerCaptureOrSwap(config->reg_base, BIT(config->tcpwm_index));
#else
	Cy_TCPWM_TriggerCaptureOrSwap_Single(config->reg_base, config->tcpwm_index);
#endif
}

/* Trigger start */
static inline void ifx_tcpwm_trigger_start(const struct ifx_tcpwm_pwm_config *config)
{
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	Cy_TCPWM_TriggerStart(config->reg_base, BIT(config->tcpwm_index));
#else
	Cy_TCPWM_TriggerStart_Single(config->reg_base, config->tcpwm_index);
#endif
}

/* Set polarity */
static inline void ifx_tcpwm_pwm_set_polarity(const struct ifx_tcpwm_pwm_config *config,
					      pwm_flags_t flags)
{
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED) {
		TCPWM_CNT_CTRL(config->reg_base, config->tcpwm_index) |=
			TCPWM_CNT_CTRL_QUADRATURE_MODE_Msk;
	} else {
		TCPWM_CNT_CTRL(config->reg_base, config->tcpwm_index) &=
			~TCPWM_CNT_CTRL_QUADRATURE_MODE_Msk;
	}
#else
/* Macro to get pointer to counter cat1 struct from base address and counter number */
#define IFX_CAT1_TCPWM_GRP_CNT_PTR(base, cntNum)                                                   \
	(((TCPWM_Type *)(base))->GRP + TCPWM_GRP_CNT_GET_GRP(cntNum))->CNT + ((cntNum) % 256U);

	uint32_t ctrl_temp;
	TCPWM_GRP_CNT_Type *cnt_ptr =
		IFX_CAT1_TCPWM_GRP_CNT_PTR(config->reg_base, config->tcpwm_index);

	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED) {
		cnt_ptr->CTRL |= TCPWM_GRP_CNT_V2_CTRL_QUAD_ENCODING_MODE_Msk;
	} else {
		cnt_ptr->CTRL &= ~TCPWM_GRP_CNT_V2_CTRL_QUAD_ENCODING_MODE_Msk;
	}

	ctrl_temp = cnt_ptr->CTRL & ~TCPWM_GRP_CNT_V2_CTRL_PWM_DISABLE_MODE_Msk;

	cnt_ptr->CTRL = ctrl_temp |
			_VAL2FLD(TCPWM_GRP_CNT_V2_CTRL_PWM_DISABLE_MODE,
				 (flags & PWM_IFX_TCPWM_OUTPUT_MASK) >> PWM_IFX_TCPWM_OUTPUT_POS);
#endif
}

/* Set initial period/compare values when PWM is not running */
static inline void ifx_tcpwm_pwm_set_initial_values(const struct ifx_tcpwm_pwm_config *config,
						    uint32_t period_cycles, uint32_t pulse_cycles)
{
	if ((period_cycles != 0) && (pulse_cycles != 0)) {
		Cy_TCPWM_PWM_SetPeriod0(config->reg_base, config->tcpwm_index, period_cycles - 1);
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
		Cy_TCPWM_PWM_SetCompare0(config->reg_base, config->tcpwm_index, pulse_cycles);
#else
		Cy_TCPWM_PWM_SetCompare0Val(config->reg_base, config->tcpwm_index, pulse_cycles);
#endif
	}
}

/* Validate cycles based on resolution */
static inline int ifx_tcpwm_pwm_validate_cycles(const struct ifx_tcpwm_pwm_config *config,
						uint32_t period_cycles, uint32_t pulse_cycles)
{
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
	return 0;
}

static int ifx_tcpwm_pwm_set_cycles(const struct device *dev, uint32_t channel,
				    uint32_t period_cycles, uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	ARG_UNUSED(channel);

	const struct ifx_tcpwm_pwm_config *config = dev->config;
	int ret;

	/* Validate cycles based on resolution */
	ret = ifx_tcpwm_pwm_validate_cycles(config, period_cycles, pulse_cycles);
	if (ret != 0) {
		LOG_ERR("PWM cycles validation failed");
		return -EINVAL;
	}

	/* Set polarity based on flags */
	ifx_tcpwm_pwm_set_polarity(config, flags);

	/* If the PWM is not yet running, write the period and compare directly */
	if (is_ifx_tcpwm_pwm_not_running(config)) {
		ifx_tcpwm_pwm_set_initial_values(config, period_cycles, pulse_cycles);
	}

	/* Special case, if period_cycles is 0, set the period and compare to zero.  If we were to
	 * disable the PWM, the output would be set to High-Z, wheras this will set the output to
	 * the zero duty cycle state instead.
	 */
	if (period_cycles == 0) {
		ifx_tcpwm_pwm_set_period(config, 0);
		ifx_tcpwm_pwm_set_compare(config, 0);
		ifx_tcpwm_trigger_swap(config);
	} else {
		/* Update period and compare values using buffer registers so the new values take
		 * effect on the next TC event.  This prevents glitches in PWM output depending on
		 * where in the PWM cycle the update occurs.
		 */
		ifx_tcpwm_pwm_set_period(config, period_cycles);
		ifx_tcpwm_pwm_set_compare(config, pulse_cycles);
		/* Trigger the swap by writing to the SW trigger command register.
		 */
		ifx_tcpwm_trigger_swap(config);
	}

	/* Enable the TCPWM for PWM mode of operation */
	Cy_TCPWM_PWM_Enable(config->reg_base, config->tcpwm_index);

	/* Start the TCPWM block */
	ifx_tcpwm_trigger_start(config);

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

static DEVICE_API(pwm, ifx_tcpwm_pwm_api) = {
	.set_cycles = ifx_tcpwm_pwm_set_cycles,
	.get_cycles_per_sec = ifx_tcpwm_pwm_get_cycles_per_sec,
};

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define PWM_PERI_CLOCK_INIT(n)                                                                     \
	.clock =                                                                                   \
		{                                                                                  \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),         \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
		}
#else
#define PWM_PERI_CLOCK_INIT(n)                                                                     \
	.clock =                                                                                   \
		{                                                                                  \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
		}
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
		.reg_base = (TCPWM_Type *)(DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))),             \
		TCPWM_PWM_IDX(n),                                                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.resolution_32_bits =                                                              \
			(DT_PROP(DT_INST_PARENT(n), resolution) == 32) ? true : false,             \
		.index = (DT_REG_ADDR(DT_INST_PARENT(n)) -                                         \
			  DT_REG_ADDR(DT_PARENT(DT_INST_PARENT(n)))) /                             \
			 DT_REG_SIZE(DT_INST_PARENT(n)),                                           \
		.clk_dst = DT_PROP(DT_INST_PARENT(n), clk_dst),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_tcpwm_pwm_init, NULL, &ifx_tcpwm_pwm##n##_data,               \
			      &pwm_tcpwm_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,        \
			      &ifx_tcpwm_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_TCPWM_PWM_INIT)
