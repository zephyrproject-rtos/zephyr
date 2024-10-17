/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright (c) 2022 NXP
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_acmp.h>

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/comparator/mcux_acmp.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_kinetis_acmp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_kinetis_acmp

/*
 * DAC is a register defined in the MCUX HAL. We don't need it here and it conflicts
 * with the COMP_MCUX_ACMP_PORT_INPUT_DAC definition so undef it here.
 */
#ifdef DAC
#undef DAC
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT) && (FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT == 1U)
#define COMP_MCUX_ACMP_HAS_OFFSET 1
#else
#define COMP_MCUX_ACMP_HAS_OFFSET 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C0_HYSTCTR_BIT) && (FSL_FEATURE_ACMP_HAS_C0_HYSTCTR_BIT == 1U)
#define COMP_MCUX_ACMP_HAS_HYSTERESIS 1
#else
#define COMP_MCUX_ACMP_HAS_HYSTERESIS 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT == 1U)
#define COMP_MCUX_ACMP_HAS_INPSEL 1
#else
#define COMP_MCUX_ACMP_HAS_INPSEL 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT == 1U)
#define COMP_MCUX_ACMP_HAS_INNSEL 1
#else
#define COMP_MCUX_ACMP_HAS_INNSEL 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C1_DACOE_BIT) && (FSL_FEATURE_ACMP_HAS_C1_DACOE_BIT == 1U)
#define COMP_MCUX_ACMP_HAS_DAC_OUT_ENABLE 1
#else
#define COMP_MCUX_ACMP_HAS_DAC_OUT_ENABLE 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT) && (FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT == 1U)
#define COMP_MCUX_ACMP_HAS_DAC_WORK_MODE 1
#else
#define COMP_MCUX_ACMP_HAS_DAC_WORK_MODE 0
#endif

#if defined(FSL_FEATURE_ACMP_HAS_C3_REG) && (FSL_FEATURE_ACMP_HAS_C3_REG != 0U)
#define COMP_MCUX_ACMP_HAS_DISCRETE_MODE 1
#else
#define COMP_MCUX_ACMP_HAS_DISCRETE_MODE 0
#endif

#if !(defined(FSL_FEATURE_ACMP_HAS_NO_WINDOW_MODE) && (FSL_FEATURE_ACMP_HAS_NO_WINDOW_MODE == 1U))
#define COMP_MCUX_ACMP_HAS_WINDOW_MODE 1
#else
#define COMP_MCUX_ACMP_HAS_WINDOW_MODE 0
#endif

#define MCUX_ACMP_ENUM(name, value) \
	_CONCAT_4(COMP_MCUX_ACMP_, name, _, value)

#define MCUX_ACMP_DT_INST_ENUM(inst, name, prop) \
	MCUX_ACMP_ENUM(name, DT_INST_STRING_TOKEN(inst, prop))

#define MCUX_ACMP_DT_INST_ENUM_OR(inst, name, prop, or)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, prop),						\
		    (MCUX_ACMP_DT_INST_ENUM(inst, name, prop)),					\
		    (MCUX_ACMP_ENUM(name, or)))

#define MCUX_ACMP_DT_INST_OFFSET_MODE(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, OFFSET_MODE, offset_mode, LEVEL0)

#define MCUX_ACMP_DT_INST_HYST_MODE(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, HYSTERESIS_MODE, hysteresis_mode, LEVEL0)

#define MCUX_ACMP_DT_INST_EN_HS_MODE(inst) \
	DT_INST_PROP(inst, enable_high_speed_mode)

#define MCUX_ACMP_DT_INST_INV_OUT(inst) \
	DT_INST_PROP(inst, invert_output)

#define MCUX_ACMP_DT_INST_USE_UNFILTERED_OUT(inst) \
	DT_INST_PROP(inst, use_unfiltered_output)

#define MCUX_ACMP_DT_INST_EN_PIN_OUT(inst) \
	DT_INST_PROP(inst, enable_pin_out)

#define MCUX_ACMP_DT_INST_MODE_CONFIG_INIT(inst)						\
	{											\
		.offset_mode = MCUX_ACMP_DT_INST_OFFSET_MODE(inst),				\
		.hysteresis_mode = MCUX_ACMP_DT_INST_HYST_MODE(inst),				\
		.enable_high_speed_mode = MCUX_ACMP_DT_INST_EN_HS_MODE(inst),			\
		.invert_output = MCUX_ACMP_DT_INST_INV_OUT(inst),				\
		.use_unfiltered_output = MCUX_ACMP_DT_INST_USE_UNFILTERED_OUT(inst),		\
		.enable_pin_output = MCUX_ACMP_DT_INST_EN_PIN_OUT(inst),			\
	}

#define MCUX_ACMP_DT_INST_P_MUX_IN(inst) \
	MCUX_ACMP_DT_INST_ENUM(inst, MUX_INPUT, positive_mux_input)

#define MCUX_ACMP_DT_INST_N_MUX_IN(inst) \
	MCUX_ACMP_DT_INST_ENUM(inst, MUX_INPUT, negative_mux_input)

#define MCUX_ACMP_DT_INST_P_PORT_IN(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, PORT_INPUT, positive_port_input, MUX)

#define MCUX_ACMP_DT_INST_N_PORT_IN(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, PORT_INPUT, negative_port_input, MUX)

#define MCUX_ACMP_DT_INST_INPUT_CONFIG_INIT(inst)						\
	{											\
		.positive_mux_input = MCUX_ACMP_DT_INST_P_MUX_IN(inst),				\
		.negative_mux_input = MCUX_ACMP_DT_INST_N_MUX_IN(inst),				\
		.positive_port_input = MCUX_ACMP_DT_INST_P_PORT_IN(inst),			\
		.negative_port_input = MCUX_ACMP_DT_INST_N_PORT_IN(inst),			\
	}

#define MCUX_ACMP_DT_INST_FILTER_EN_SAMPLE(inst) \
	DT_INST_PROP(inst, filter_enable_sample)

#define MCUX_ACMP_DT_INST_FILTER_COUNT(inst) \
	DT_INST_PROP_OR(inst, filter_count, 0)

#define MCUX_ACMP_DT_INST_FILTER_PERIOD(inst) \
	DT_INST_PROP_OR(inst, filter_period, 0)

#define MCUX_ACMP_DT_INST_FILTER_CONFIG_INIT(inst)						\
	{											\
		.enable_sample = MCUX_ACMP_DT_INST_FILTER_EN_SAMPLE(inst),			\
		.filter_count = MCUX_ACMP_DT_INST_FILTER_COUNT(inst),				\
		.filter_period = MCUX_ACMP_DT_INST_FILTER_PERIOD(inst),				\
	}

#define MCUX_ACMP_DT_INST_DAC_VREF_SOURCE(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, DAC_VREF_SOURCE, dac_vref_source, VIN1)

#define MCUX_ACMP_DT_INST_DAC_VALUE(inst) \
	DT_INST_PROP_OR(inst, dac_value, 0)

#define MCUX_ACMP_DT_INST_DAC_EN(inst) \
	DT_INST_PROP(inst, dac_enable)

#define MCUX_ACMP_DT_INST_DAC_EN_HS(inst) \
	DT_INST_PROP(inst, dac_enable_high_speed)

#define MCUX_ACMP_DT_INST_DAC_CONFIG_INIT(inst)							\
	{											\
		.vref_source = MCUX_ACMP_DT_INST_DAC_VREF_SOURCE(inst),				\
		.value = MCUX_ACMP_DT_INST_DAC_VALUE(inst),					\
		.enable_output = MCUX_ACMP_DT_INST_DAC_EN(inst),				\
		.enable_high_speed_mode = MCUX_ACMP_DT_INST_DAC_EN_HS(inst),			\
	}

#define MCUX_ACMP_DT_INST_DM_EN_P_CH(inst) \
	DT_INST_PROP(inst, discrete_mode_enable_positive_channel)

#define MCUX_ACMP_DT_INST_DM_EN_N_CH(inst) \
	DT_INST_PROP(inst, discrete_mode_enable_negative_channel)

#define MCUX_ACMP_DT_INST_DM_EN_RES_DIV(inst) \
	DT_INST_PROP(inst, discrete_mode_enable_resistor_divider)

#define MCUX_ACMP_DT_INST_DM_CLOCK_SOURCE(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, DM_CLOCK, discrete_mode_clock_source, SLOW)

#define MCUX_ACMP_DT_INST_DM_SAMPLE_TIME(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, DM_SAMPLE_TIME, discrete_mode_sample_time, T1)

#define MCUX_ACMP_DT_INST_DM_PHASE1_TIME(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, DM_PHASE_TIME, discrete_mode_phase1_time, ALT0)

#define MCUX_ACMP_DT_INST_DM_PHASE2_TIME(inst) \
	MCUX_ACMP_DT_INST_ENUM_OR(inst, DM_PHASE_TIME, discrete_mode_phase2_time, ALT0)

#define MCUX_ACMP_DT_INST_DM_CONFIG_INIT(inst)							\
	{											\
		.enable_positive_channel = MCUX_ACMP_DT_INST_DM_EN_P_CH(inst),			\
		.enable_negative_channel = MCUX_ACMP_DT_INST_DM_EN_N_CH(inst),			\
		.enable_resistor_divider = MCUX_ACMP_DT_INST_DM_EN_RES_DIV(inst),		\
		.clock_source = MCUX_ACMP_DT_INST_DM_CLOCK_SOURCE(inst),			\
		.sample_time = MCUX_ACMP_DT_INST_DM_SAMPLE_TIME(inst),				\
		.phase1_time = MCUX_ACMP_DT_INST_DM_PHASE1_TIME(inst),				\
		.phase2_time = MCUX_ACMP_DT_INST_DM_PHASE2_TIME(inst),				\
	}

#define MCUX_ACMP_DT_INST_EN_WINDOW_MODE(inst) \
	DT_INST_PROP(inst, enable_window_mode)

struct mcux_acmp_config {
	CMP_Type *base;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_init)(void);
	const struct comp_mcux_acmp_mode_config mode_config;
	const struct comp_mcux_acmp_input_config input_config;
	const struct comp_mcux_acmp_filter_config filter_config;
	const struct comp_mcux_acmp_dac_config dac_config;
#if COMP_MCUX_ACMP_HAS_DISCRETE_MODE
	const struct comp_mcux_acmp_dm_config dm_config;
#endif
#if COMP_MCUX_ACMP_HAS_WINDOW_MODE
	bool enable_window_mode;
#endif
};

#if MCUX_ACMP_HAS_OFFSET
BUILD_ASSERT((int)kACMP_OffsetLevel0 == (int)COMP_MCUX_ACMP_OFFSET_MODE_LEVEL0);
BUILD_ASSERT((int)kACMP_OffsetLevel1 == (int)COMP_MCUX_ACMP_OFFSET_MODE_LEVEL1);
#endif

#if COMP_MCUX_ACMP_HAS_HYSTERESIS
BUILD_ASSERT((int)kACMP_HysteresisLevel0 == (int)COMP_MCUX_ACMP_HYSTERESIS_MODE_LEVEL0);
BUILD_ASSERT((int)kACMP_HysteresisLevel1 == (int)COMP_MCUX_ACMP_HYSTERESIS_MODE_LEVEL1);
BUILD_ASSERT((int)kACMP_HysteresisLevel2 == (int)COMP_MCUX_ACMP_HYSTERESIS_MODE_LEVEL2);
BUILD_ASSERT((int)kACMP_HysteresisLevel3 == (int)COMP_MCUX_ACMP_HYSTERESIS_MODE_LEVEL3);
#endif

BUILD_ASSERT((int)kACMP_VrefSourceVin1 == (int)COMP_MCUX_ACMP_DAC_VREF_SOURCE_VIN1);
BUILD_ASSERT((int)kACMP_VrefSourceVin2 == (int)COMP_MCUX_ACMP_DAC_VREF_SOURCE_VIN2);

#if MCUX_ACMP_HAS_INPSEL || MCUX_ACMP_HAS_INNSEL
BUILD_ASSERT((int)kACMP_PortInputFromDAC == (int)COMP_MCUX_ACMP_PORT_INPUT_DAC);
BUILD_ASSERT((int)kACMP_PortInputFromMux == (int)COMP_MCUX_ACMP_PORT_INPUT_MUX);
#endif

#if COMP_MCUX_ACMP_HAS_DISCRETE_MODE
BUILD_ASSERT((int)kACMP_DiscreteClockSlow == (int)COMP_MCUX_ACMP_DM_CLOCK_SLOW);
BUILD_ASSERT((int)kACMP_DiscreteClockFast == (int)COMP_MCUX_ACMP_DM_CLOCK_FAST);

BUILD_ASSERT((int)kACMP_DiscreteSampleTimeAs1T == (int)COMP_MCUX_ACMP_DM_SAMPLE_TIME_T1);
BUILD_ASSERT((int)kACMP_DiscreteSampleTimeAs2T == (int)COMP_MCUX_ACMP_DM_SAMPLE_TIME_T2);
BUILD_ASSERT((int)kACMP_DiscreteSampleTimeAs4T == (int)COMP_MCUX_ACMP_DM_SAMPLE_TIME_T4);
BUILD_ASSERT((int)kACMP_DiscreteSampleTimeAs8T == (int)COMP_MCUX_ACMP_DM_SAMPLE_TIME_T8);
BUILD_ASSERT((int)kACMP_DiscreteSampleTimeAs16T == (int)COMP_MCUX_ACMP_DM_SAMPLE_TIME_T16);
BUILD_ASSERT((int)kACMP_DiscreteSampleTimeAs32T == (int)COMP_MCUX_ACMP_DM_SAMPLE_TIME_T32);
BUILD_ASSERT((int)kACMP_DiscreteSampleTimeAs64T == (int)COMP_MCUX_ACMP_DM_SAMPLE_TIME_T64);
BUILD_ASSERT((int)kACMP_DiscreteSampleTimeAs256T == (int)COMP_MCUX_ACMP_DM_SAMPLE_TIME_T256);

BUILD_ASSERT((int)kACMP_DiscretePhaseTimeAlt0 == (int)COMP_MCUX_ACMP_DM_PHASE_TIME_ALT0);
BUILD_ASSERT((int)kACMP_DiscretePhaseTimeAlt1 == (int)COMP_MCUX_ACMP_DM_PHASE_TIME_ALT1);
BUILD_ASSERT((int)kACMP_DiscretePhaseTimeAlt2 == (int)COMP_MCUX_ACMP_DM_PHASE_TIME_ALT2);
BUILD_ASSERT((int)kACMP_DiscretePhaseTimeAlt3 == (int)COMP_MCUX_ACMP_DM_PHASE_TIME_ALT3);
BUILD_ASSERT((int)kACMP_DiscretePhaseTimeAlt4 == (int)COMP_MCUX_ACMP_DM_PHASE_TIME_ALT4);
BUILD_ASSERT((int)kACMP_DiscretePhaseTimeAlt5 == (int)COMP_MCUX_ACMP_DM_PHASE_TIME_ALT5);
BUILD_ASSERT((int)kACMP_DiscretePhaseTimeAlt6 == (int)COMP_MCUX_ACMP_DM_PHASE_TIME_ALT6);
BUILD_ASSERT((int)kACMP_DiscretePhaseTimeAlt7 == (int)COMP_MCUX_ACMP_DM_PHASE_TIME_ALT7);
#endif

struct mcux_acmp_data {
	uint32_t interrupt_mask;
	comparator_callback_t callback;
	void *user_data;
};

#if CONFIG_PM_DEVICE
static bool mcux_acmp_is_resumed(const struct device *dev)
{
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	return state == PM_DEVICE_STATE_ACTIVE;
}
#else
static bool mcux_acmp_is_resumed(const struct device *dev)
{
	ARG_UNUSED(dev);
	return true;
}
#endif

static int mcux_acmp_get_output(const struct device *dev)
{
	const struct mcux_acmp_config *config = dev->config;
	uint32_t status;

	status = ACMP_GetStatusFlags(config->base);
	return (status & kACMP_OutputAssertEventFlag) ? 1 : 0;
}

static int mcux_acmp_set_trigger(const struct device *dev,
				 enum comparator_trigger trigger)
{
	const struct mcux_acmp_config *config = dev->config;
	struct mcux_acmp_data *data = dev->data;

	ACMP_DisableInterrupts(config->base, UINT32_MAX);

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		data->interrupt_mask = 0;
		break;

	case COMPARATOR_TRIGGER_RISING_EDGE:
		data->interrupt_mask = kACMP_OutputRisingInterruptEnable;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		data->interrupt_mask = kACMP_OutputFallingInterruptEnable;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		data->interrupt_mask = kACMP_OutputFallingInterruptEnable |
				       kACMP_OutputRisingInterruptEnable;
		break;
	}

	if (data->interrupt_mask && data->callback != NULL) {
		ACMP_EnableInterrupts(config->base, data->interrupt_mask);
	}

	return 0;
}

static int mcux_acmp_set_trigger_callback(const struct device *dev,
					  comparator_callback_t callback,
					  void *user_data)
{
	const struct mcux_acmp_config *config = dev->config;
	struct mcux_acmp_data *data = dev->data;

	ACMP_DisableInterrupts(config->base, UINT32_MAX);

	data->callback = callback;
	data->user_data = user_data;

	if (data->callback == NULL) {
		return 0;
	}

	if (data->interrupt_mask) {
		ACMP_EnableInterrupts(config->base, data->interrupt_mask);
	}

	return 0;
}

static int mcux_acmp_trigger_is_pending(const struct device *dev)
{
	const struct mcux_acmp_config *config = dev->config;
	struct mcux_acmp_data *data = dev->data;
	uint32_t status_flags;

	status_flags = ACMP_GetStatusFlags(config->base);
	ACMP_ClearStatusFlags(config->base, UINT32_MAX);

	if ((data->interrupt_mask & kACMP_OutputRisingInterruptEnable) &&
	    (status_flags & kACMP_OutputRisingEventFlag)) {
		return 1;
	}

	if ((data->interrupt_mask & kACMP_OutputFallingInterruptEnable) &&
	    (status_flags & kACMP_OutputFallingEventFlag)) {
		return 1;
	}

	return 0;
}

static const struct comparator_driver_api mcux_acmp_comp_api = {
	.get_output = mcux_acmp_get_output,
	.set_trigger = mcux_acmp_set_trigger,
	.set_trigger_callback = mcux_acmp_set_trigger_callback,
	.trigger_is_pending = mcux_acmp_trigger_is_pending,
};

static void comp_mcux_acmp_init_mode_config(const struct device *dev,
					    const struct comp_mcux_acmp_mode_config *config)
{
	const struct mcux_acmp_config *dev_config = dev->config;
	acmp_config_t acmp_config;

#if COMP_MCUX_ACMP_HAS_OFFSET
	acmp_config.offsetMode = (acmp_offset_mode_t)config->offset_mode;
#endif

#if COMP_MCUX_ACMP_HAS_HYSTERESIS
	acmp_config.hysteresisMode = (acmp_hysteresis_mode_t)config->hysteresis_mode;
#endif

	acmp_config.enableHighSpeed = config->enable_high_speed_mode;
	acmp_config.enableInvertOutput = config->invert_output;
	acmp_config.useUnfilteredOutput = config->use_unfiltered_output;
	acmp_config.enablePinOut = config->enable_pin_output;

	ACMP_Init(dev_config->base, &acmp_config);
}

int comp_mcux_acmp_set_mode_config(const struct device *dev,
				   const struct comp_mcux_acmp_mode_config *config)
{
	const struct mcux_acmp_config *dev_config = dev->config;

	comp_mcux_acmp_init_mode_config(dev, config);

	if (mcux_acmp_is_resumed(dev)) {
		ACMP_Enable(dev_config->base, true);
	}

	return 0;
}

int comp_mcux_acmp_set_input_config(const struct device *dev,
				    const struct comp_mcux_acmp_input_config *config)
{
	const struct mcux_acmp_config *dev_config = dev->config;
	acmp_channel_config_t acmp_channel_config;

#if COMP_MCUX_ACMP_HAS_INPSEL
	acmp_channel_config.positivePortInput = (acmp_port_input_t)config->positive_port_input;
#endif

	acmp_channel_config.plusMuxInput = (uint32_t)config->positive_mux_input;

#if COMP_MCUX_ACMP_HAS_INNSEL
	acmp_channel_config.negativePortInput = (acmp_port_input_t)config->negative_port_input;
#endif

	acmp_channel_config.minusMuxInput = (uint32_t)config->negative_mux_input;

	ACMP_SetChannelConfig(dev_config->base, &acmp_channel_config);
	return 0;
}

int comp_mcux_acmp_set_filter_config(const struct device *dev,
				     const struct comp_mcux_acmp_filter_config *config)
{
	const struct mcux_acmp_config *dev_config = dev->config;
	acmp_filter_config_t acmp_filter_config;

	if (config->enable_sample && config->filter_count == 0) {
		return -EINVAL;
	}

	if (config->filter_count > 7) {
		return -EINVAL;
	}

	acmp_filter_config.enableSample = config->enable_sample;
	acmp_filter_config.filterCount = config->filter_count;
	acmp_filter_config.filterPeriod = config->filter_period;

	ACMP_SetFilterConfig(dev_config->base, &acmp_filter_config);
	return 0;
}

int comp_mcux_acmp_set_dac_config(const struct device *dev,
				  const struct comp_mcux_acmp_dac_config *config)
{
	const struct mcux_acmp_config *dev_config = dev->config;
	acmp_dac_config_t acmp_dac_config;

	acmp_dac_config.referenceVoltageSource =
		(acmp_reference_voltage_source_t)config->vref_source;

	acmp_dac_config.DACValue = config->value;

#if COMP_MCUX_ACMP_HAS_DAC_OUT_ENABLE
	acmp_dac_config.enableOutput = config->enable_output;
#endif

#if COMP_MCUX_ACMP_HAS_DAC_WORK_MODE
	acmp_dac_config.workMode = config->enable_high_speed_mode
				 ? kACMP_DACWorkHighSpeedMode
				 : kACMP_DACWorkLowSpeedMode;
#endif

	ACMP_SetDACConfig(dev_config->base, &acmp_dac_config);
	return 0;
}

#if COMP_MCUX_ACMP_HAS_DISCRETE_MODE
int comp_mcux_acmp_set_dm_config(const struct device *dev,
				 const struct comp_mcux_acmp_dm_config *config)
{
	const struct mcux_acmp_config *dev_config = dev->config;
	acmp_discrete_mode_config_t acmp_dm_config;

	acmp_dm_config.enablePositiveChannelDiscreteMode = config->enable_positive_channel;
	acmp_dm_config.enableNegativeChannelDiscreteMode = config->enable_negative_channel;
	acmp_dm_config.enableResistorDivider = config->enable_resistor_divider;
	acmp_dm_config.clockSource = (acmp_discrete_clock_source_t)config->clock_source;
	acmp_dm_config.sampleTime = (acmp_discrete_sample_time_t)config->sample_time;
	acmp_dm_config.phase1Time = (acmp_discrete_phase_time_t)config->phase1_time;
	acmp_dm_config.phase2Time = (acmp_discrete_phase_time_t)config->phase2_time;

	ACMP_SetDiscreteModeConfig(dev_config->base, &acmp_dm_config);
	return 0;
}
#endif

#if COMP_MCUX_ACMP_HAS_WINDOW_MODE
int comp_mcux_acmp_set_window_mode(const struct device *dev, bool enable)
{
	const struct mcux_acmp_config *config = dev->config;

	ACMP_EnableWindowMode(config->base, enable);
	return 0;
}
#endif

static int mcux_acmp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct mcux_acmp_config *config = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		ACMP_Enable(config->base, true);
	}

#if CONFIG_PM_DEVICE
	if (action == PM_DEVICE_ACTION_SUSPEND) {
		ACMP_Enable(config->base, false);
	}
#endif

	return 0;
}

static void mcux_acmp_irq_handler(const struct device *dev)
{
	const struct mcux_acmp_config *config = dev->config;
	struct mcux_acmp_data *data = dev->data;

	ACMP_ClearStatusFlags(config->base, UINT32_MAX);

	if (data->callback == NULL) {
		return;
	}

	data->callback(dev, data->user_data);
}

static int mcux_acmp_init(const struct device *dev)
{
	const struct mcux_acmp_config *config = dev->config;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("failed to set %s", "pincfg");
		return ret;
	}

	comp_mcux_acmp_init_mode_config(dev, &config->mode_config);

	ret = comp_mcux_acmp_set_input_config(dev, &config->input_config);
	if (ret) {
		LOG_ERR("failed to set %s", "input config");
		return ret;
	}

	ret = comp_mcux_acmp_set_filter_config(dev, &config->filter_config);
	if (ret) {
		LOG_ERR("failed to set %s", "filter config");
		return ret;
	}

	ret = comp_mcux_acmp_set_dac_config(dev, &config->dac_config);
	if (ret) {
		LOG_ERR("failed to set %s", "dac config");
		return ret;
	}

#if COMP_MCUX_ACMP_HAS_DISCRETE_MODE
	ret = comp_mcux_acmp_set_dm_config(dev, &config->dm_config);
	if (ret) {
		LOG_ERR("failed to set %s", "discrete mode config");
		return ret;
	}
#endif

#if COMP_MCUX_ACMP_HAS_WINDOW_MODE
	ret = comp_mcux_acmp_set_window_mode(dev, config->enable_window_mode);
	if (ret) {
		LOG_ERR("failed to set %s", "window mode");
		return ret;
	}
#endif

	ACMP_DisableInterrupts(config->base, UINT32_MAX);
	config->irq_init();

	return pm_device_driver_init(dev, mcux_acmp_pm_callback);
}

#define MCUX_ACMP_IRQ_HANDLER_SYM(inst) \
	_CONCAT(mcux_acmp_irq_init, inst)

#define MCUX_ACMP_IRQ_HANDLER_DEFINE(inst)							\
	static void MCUX_ACMP_IRQ_HANDLER_SYM(inst)(void)					\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst),							\
			    DT_INST_IRQ(inst, priority),					\
			    mcux_acmp_irq_handler,						\
			    DEVICE_DT_INST_GET(inst),						\
			    0);									\
												\
		irq_enable(DT_INST_IRQN(inst));							\
	}

#define MCUX_ACMP_DEVICE(inst)									\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	static struct mcux_acmp_data _CONCAT(data, inst);					\
												\
	MCUX_ACMP_IRQ_HANDLER_DEFINE(inst)							\
												\
	static const struct mcux_acmp_config _CONCAT(config, inst) = {				\
		.base = (CMP_Type *)DT_INST_REG_ADDR(inst),					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
		.irq_init = MCUX_ACMP_IRQ_HANDLER_SYM(inst),					\
		.mode_config = MCUX_ACMP_DT_INST_MODE_CONFIG_INIT(inst),			\
		.input_config = MCUX_ACMP_DT_INST_INPUT_CONFIG_INIT(inst),			\
		.filter_config = MCUX_ACMP_DT_INST_FILTER_CONFIG_INIT(inst),			\
		.dac_config = MCUX_ACMP_DT_INST_DAC_CONFIG_INIT(inst),				\
		IF_ENABLED(COMP_MCUX_ACMP_HAS_DISCRETE_MODE,					\
			   (.dm_config = MCUX_ACMP_DT_INST_DM_CONFIG_INIT(inst),))		\
		IF_ENABLED(COMP_MCUX_ACMP_HAS_WINDOW_MODE,					\
			   (.enable_window_mode = MCUX_ACMP_DT_INST_EN_WINDOW_MODE(inst),))	\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, mcux_acmp_pm_callback);					\
												\
	DEVICE_DT_INST_DEFINE(inst,								\
			      mcux_acmp_init,							\
			      PM_DEVICE_DT_INST_GET(inst),					\
			      &_CONCAT(data, inst),						\
			      &_CONCAT(config, inst),						\
			      POST_KERNEL,							\
			      CONFIG_COMPARATOR_INIT_PRIORITY,					\
			      &mcux_acmp_comp_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_ACMP_DEVICE)
