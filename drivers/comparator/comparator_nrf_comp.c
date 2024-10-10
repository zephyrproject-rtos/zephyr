/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx_comp.h>

#include <zephyr/drivers/comparator/nrf_comp.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT nordic_nrf_comp

#define SHIM_NRF_COMP_DT_INST_REFSEL(inst) \
	_CONCAT(COMP_NRF_COMP_REFSEL_, DT_INST_STRING_TOKEN(inst, refsel))

#define SHIM_NRF_COMP_DT_INST_REFSEL_IS_AREF(inst) \
	DT_INST_ENUM_HAS_VALUE(inst, refsel, AREF)

#define SHIM_NRF_COMP_DT_INST_EXTREFSEL(inst) \
	_CONCAT(COMP_NRF_COMP_EXTREFSEL_, DT_INST_STRING_TOKEN(inst, extrefsel))

#define SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_SE(inst) \
	DT_INST_ENUM_HAS_VALUE(inst, main_mode, SE)

#define SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_DIFF(inst) \
	DT_INST_ENUM_HAS_VALUE(inst, main_mode, DIFF)

#define SHIM_NRF_COMP_DT_INST_TH_DOWN(inst) \
	DT_INST_PROP(inst, th_down)

#define SHIM_NRF_COMP_DT_INST_TH_UP(inst) \
	DT_INST_PROP(inst, th_up)

#define SHIM_NRF_COMP_DT_INST_SP_MODE(inst) \
	_CONCAT(COMP_NRF_COMP_SP_MODE_, DT_INST_STRING_TOKEN(inst, sp_mode))

#define SHIM_NRF_COMP_DT_INST_ENABLE_HYST(inst) \
	DT_INST_PROP(inst, enable_hyst)

#define SHIM_NRF_COMP_DT_INST_ISOURCE(inst) \
	_CONCAT(COMP_NRF_COMP_ISOURCE_, DT_INST_STRING_TOKEN(inst, isource))

#define SHIM_NRF_COMP_DT_INST_PSEL(inst) \
	_CONCAT(COMP_NRF_COMP_PSEL_, DT_INST_STRING_TOKEN(inst, psel))

#if defined(COMP_HYST_HYST_Hyst40mV)
#define NRF_COMP_HYST_ENABLED NRF_COMP_HYST_40MV
#elif defined(COMP_HYST_HYST_Hyst50mV)
#define NRF_COMP_HYST_ENABLED NRF_COMP_HYST_50MV
#endif

#define NRF_COMP_HYST_DISABLED NRF_COMP_HYST_NO_HYST

#if defined(NRF_COMP_HYST_ENABLED)
#define NRF_COMP_HAS_HYST 1
#else
#define NRF_COMP_HAS_HYST 0
#endif

struct shim_nrf_comp_data {
	uint32_t event_mask;
	bool started;
	atomic_t triggered;
	comparator_callback_t callback;
	void *user_data;
};

#if (NRF_COMP_HAS_AIN_AS_PIN)
static const uint32_t shim_nrf_comp_ain_map[] = {
#if defined(CONFIG_SOC_NRF54H20) || defined(CONFIG_SOC_NRF9280)
	NRF_PIN_PORT_TO_PIN_NUMBER(0U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(1U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(2U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(3U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
#elif defined(CONFIG_SOC_NRF54L15)
	NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(11U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(12U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(13U, 1),
	NRF_PIN_PORT_TO_PIN_NUMBER(14U, 1),
#endif
};
#endif

#if SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_SE(0)
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_TH_DOWN(0) < 64);
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_TH_UP(0) < 64);
#endif

#if NRF_COMP_HAS_AIN_AS_PIN
BUILD_ASSERT((COMP_NRF_COMP_PSEL_AIN0 == 0));
BUILD_ASSERT((COMP_NRF_COMP_PSEL_AIN7 == 7));
BUILD_ASSERT((COMP_NRF_COMP_EXTREFSEL_AIN0 == 0));
BUILD_ASSERT((COMP_NRF_COMP_EXTREFSEL_AIN7 == 7));
#else
#ifndef COMP_PSEL_PSEL_AnalogInput4
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_PSEL(0) != COMP_NRF_COMP_PSEL_AIN4);
#endif

#ifndef COMP_PSEL_PSEL_AnalogInput5
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_PSEL(0) != COMP_NRF_COMP_PSEL_AIN5);
#endif

#ifndef COMP_PSEL_PSEL_AnalogInput6
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_PSEL(0) != COMP_NRF_COMP_PSEL_AIN6);
#endif

#ifndef COMP_PSEL_PSEL_AnalogInput7
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_PSEL(0) != COMP_NRF_COMP_PSEL_AIN7);
#endif
#endif

#ifndef COMP_PSEL_PSEL_VddDiv2
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_PSEL(0) != COMP_NRF_COMP_PSEL_VDD_DIV2);
#endif

#ifndef COMP_PSEL_PSEL_VddhDiv5
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_PSEL(0) != COMP_NRF_COMP_PSEL_VDDH_DIV5);
#endif

#ifndef COMP_MODE_SP_Normal
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_SP_MODE(0) != COMP_NRF_COMP_SP_MODE_NORMAL);
#endif

#if NRF_COMP_HAS_ISOURCE
#ifndef COMP_ISOURCE_ISOURCE_Ien2uA5
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_ISOURCE(0) != COMP_NRF_COMP_ISOURCE_2UA5);
#endif

#ifndef COMP_ISOURCE_ISOURCE_Ien5uA
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_ISOURCE(0) != COMP_NRF_COMP_ISOURCE_5UA);
#endif

#ifndef COMP_ISOURCE_ISOURCE_Ien10uA
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_ISOURCE(0) != COMP_NRF_COMP_ISOURCE_10UA);
#endif
#endif

#if SHIM_NRF_COMP_DT_INST_REFSEL_IS_AREF(0)
#ifndef COMP_EXTREFSEL_EXTREFSEL_AnalogReference4
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_EXTREFSEL(0) != COMP_NRF_COMP_EXTREFSEL_AIN4);
#endif

#ifndef COMP_EXTREFSEL_EXTREFSEL_AnalogReference5
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_EXTREFSEL(0) != COMP_NRF_COMP_EXTREFSEL_AIN5);
#endif

#ifndef COMP_EXTREFSEL_EXTREFSEL_AnalogReference6
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_EXTREFSEL(0) != COMP_NRF_COMP_EXTREFSEL_AIN6);
#endif

#ifndef COMP_EXTREFSEL_EXTREFSEL_AnalogReference7
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_EXTREFSEL(0) != COMP_NRF_COMP_EXTREFSEL_AIN7);
#endif
#endif

#if SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_SE(0)
#ifndef COMP_REFSEL_REFSEL_Int1V8
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_REFSEL(0) != COMP_NRF_COMP_REFSEL_INT_1V8);
#endif

#ifndef COMP_REFSEL_REFSEL_Int2V4
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_REFSEL(0) != COMP_NRF_COMP_REFSEL_INT_2V4);
#endif

#ifndef COMP_REFSEL_REFSEL_AVDDAO1V8
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_REFSEL(0) != COMP_NRF_COMP_REFSEL_AVDDAO1V8);
#endif

#ifndef COMP_REFSEL_REFSEL_VDD
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_REFSEL(0) != COMP_NRF_COMP_REFSEL_VDD);
#endif
#endif

#if SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_DIFF(0)
#if SHIM_NRF_COMP_DT_INST_ENABLE_HYST(0)
BUILD_ASSERT(NRF_COMP_HAS_HYST);
#endif
#endif

#if SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_SE(0)
static const struct comp_nrf_comp_se_config shim_nrf_comp_config0 = {
	.psel = SHIM_NRF_COMP_DT_INST_PSEL(0),
	.sp_mode = SHIM_NRF_COMP_DT_INST_SP_MODE(0),
	.isource = SHIM_NRF_COMP_DT_INST_ISOURCE(0),
#if SHIM_NRF_COMP_DT_INST_REFSEL_IS_AREF(0)
	.extrefsel = SHIM_NRF_COMP_DT_INST_EXTREFSEL(0),
#endif
	.refsel = SHIM_NRF_COMP_DT_INST_REFSEL(0),
	.th_down = SHIM_NRF_COMP_DT_INST_TH_DOWN(0),
	.th_up = SHIM_NRF_COMP_DT_INST_TH_UP(0),
};
#else
static const struct comp_nrf_comp_diff_config shim_nrf_comp_config0 = {
	.psel = SHIM_NRF_COMP_DT_INST_PSEL(0),
	.sp_mode = SHIM_NRF_COMP_DT_INST_SP_MODE(0),
	.isource = SHIM_NRF_COMP_DT_INST_ISOURCE(0),
	.extrefsel = SHIM_NRF_COMP_DT_INST_EXTREFSEL(0),
	.enable_hyst = SHIM_NRF_COMP_DT_INST_ENABLE_HYST(0),
};
#endif

static struct shim_nrf_comp_data shim_nrf_comp_data0;

#if CONFIG_PM_DEVICE
static bool shim_nrf_comp_is_resumed(void)
{
	enum pm_device_state state;

	(void)pm_device_state_get(DEVICE_DT_INST_GET(0), &state);
	return state == PM_DEVICE_STATE_ACTIVE;
}
#else
static bool shim_nrf_comp_is_resumed(void)
{
	return true;
}
#endif

static void shim_nrf_comp_start(void)
{
	if (shim_nrf_comp_data0.started) {
		return;
	}

	nrfx_comp_start(shim_nrf_comp_data0.event_mask, 0);
	shim_nrf_comp_data0.started = true;
}

static void shim_nrf_comp_stop(void)
{
	if (!shim_nrf_comp_data0.started) {
		return;
	}

	nrfx_comp_stop();
	shim_nrf_comp_data0.started = false;
}

static int shim_nrf_comp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		shim_nrf_comp_start();
		break;

#if CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		shim_nrf_comp_stop();
		break;
#endif

	default:
		return -ENOTSUP;
	}

	return 0;
}

#if (NRF_COMP_HAS_AIN_AS_PIN)
static int shim_nrf_comp_psel_to_nrf(enum comp_nrf_comp_psel shim,
				     nrf_comp_input_t *nrf)
{
	if (shim >= ARRAY_SIZE(shim_nrf_comp_ain_map)) {
		return -EINVAL;
	}

	*nrf = shim_nrf_comp_ain_map[(uint32_t)shim];
	return 0;
}
#else
static int shim_nrf_comp_psel_to_nrf(enum comp_nrf_comp_psel shim,
				     nrf_comp_input_t *nrf)
{
	switch (shim) {
	case COMP_NRF_COMP_PSEL_AIN0:
		*nrf = NRF_COMP_INPUT_0;
		break;

	case COMP_NRF_COMP_PSEL_AIN1:
		*nrf = NRF_COMP_INPUT_1;
		break;

	case COMP_NRF_COMP_PSEL_AIN2:
		*nrf = NRF_COMP_INPUT_2;
		break;

	case COMP_NRF_COMP_PSEL_AIN3:
		*nrf = NRF_COMP_INPUT_3;
		break;

#if defined(COMP_PSEL_PSEL_AnalogInput4)
	case COMP_NRF_COMP_PSEL_AIN4:
		*nrf = NRF_COMP_INPUT_4;
		break;
#endif

#if defined(COMP_PSEL_PSEL_AnalogInput5)
	case COMP_NRF_COMP_PSEL_AIN5:
		*nrf = NRF_COMP_INPUT_5;
		break;
#endif

#if defined(COMP_PSEL_PSEL_AnalogInput6)
	case COMP_NRF_COMP_PSEL_AIN6:
		*nrf = NRF_COMP_INPUT_6;
		break;
#endif

#if defined(COMP_PSEL_PSEL_AnalogInput7)
	case COMP_NRF_COMP_PSEL_AIN7:
		*nrf = NRF_COMP_INPUT_7;
		break;
#endif

#if defined(COMP_PSEL_PSEL_VddDiv2)
	case COMP_NRF_COMP_PSEL_VDD_DIV2:
		*nrf = NRF_COMP_VDD_DIV2;
		break;
#endif

#if defined(COMP_PSEL_PSEL_VddhDiv5)
	case COMP_NRF_COMP_PSEL_VDDH_DIV5:
		*nrf = NRF_COMP_VDDH_DIV5;
		break;
#endif

	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static int shim_nrf_comp_sp_mode_to_nrf(enum comp_nrf_comp_sp_mode shim,
					nrf_comp_sp_mode_t *nrf)
{
	switch (shim) {
	case COMP_NRF_COMP_SP_MODE_LOW:
		*nrf = NRF_COMP_SP_MODE_LOW;
		break;

#if defined(COMP_MODE_SP_Normal)
	case COMP_NRF_COMP_SP_MODE_NORMAL:
		*nrf = NRF_COMP_SP_MODE_NORMAL;
		break;
#endif

	case COMP_NRF_COMP_SP_MODE_HIGH:
		*nrf = NRF_COMP_SP_MODE_HIGH;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

#if NRF_COMP_HAS_ISOURCE
static int shim_nrf_comp_isource_to_nrf(enum comp_nrf_comp_isource shim,
					nrf_isource_t *nrf)
{
	switch (shim) {
	case COMP_NRF_COMP_ISOURCE_DISABLED:
		*nrf = NRF_COMP_ISOURCE_OFF;
		break;

#if defined(COMP_ISOURCE_ISOURCE_Ien2uA5)
	case COMP_NRF_COMP_ISOURCE_2UA5:
		*nrf = NRF_COMP_ISOURCE_IEN_2UA5;
		break;
#endif

#if defined(COMP_ISOURCE_ISOURCE_Ien5uA)
	case COMP_NRF_COMP_ISOURCE_5UA:
		*nrf = NRF_COMP_ISOURCE_IEN_5UA;
		break;
#endif

#if defined(COMP_ISOURCE_ISOURCE_Ien10uA)
	case COMP_NRF_COMP_ISOURCE_10UA:
		*nrf = NRF_COMP_ISOURCE_IEN_10UA;
		break;
#endif

	default:
		return -EINVAL;
	}

	return 0;
}
#endif

#if (NRF_COMP_HAS_AIN_AS_PIN)
static int shim_nrf_comp_extrefsel_to_nrf(enum comp_nrf_comp_extrefsel shim,
					  nrf_comp_ext_ref_t *nrf)
{
	if (shim >= ARRAY_SIZE(shim_nrf_comp_ain_map)) {
		return -EINVAL;
	}

	*nrf = shim_nrf_comp_ain_map[(uint32_t)shim];
	return 0;
}
#else
static int shim_nrf_comp_extrefsel_to_nrf(enum comp_nrf_comp_extrefsel shim,
					  nrf_comp_ext_ref_t *nrf)
{
	switch (shim) {
	case COMP_NRF_COMP_EXTREFSEL_AIN0:
		*nrf = NRF_COMP_EXT_REF_0;
		break;

	case COMP_NRF_COMP_EXTREFSEL_AIN1:
		*nrf = NRF_COMP_EXT_REF_1;
		break;

	case COMP_NRF_COMP_EXTREFSEL_AIN2:
		*nrf = NRF_COMP_EXT_REF_2;
		break;

	case COMP_NRF_COMP_EXTREFSEL_AIN3:
		*nrf = NRF_COMP_EXT_REF_3;
		break;

#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference4)
	case COMP_NRF_COMP_EXTREFSEL_AIN4:
		*nrf = NRF_COMP_EXT_REF_4;
		break;
#endif

#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference5)
	case COMP_NRF_COMP_EXTREFSEL_AIN5:
		*nrf = NRF_COMP_EXT_REF_5;
		break;
#endif

#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference6)
	case COMP_NRF_COMP_EXTREFSEL_AIN6:
		*nrf = NRF_COMP_EXT_REF_6;
		break;
#endif

#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference7)
	case COMP_NRF_COMP_EXTREFSEL_AIN7:
		*nrf = NRF_COMP_EXT_REF_7;
		break;
#endif

	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static int shim_nrf_comp_refsel_to_nrf(enum comp_nrf_comp_refsel shim,
				       nrf_comp_ref_t *nrf)
{
	switch (shim) {
	case COMP_NRF_COMP_REFSEL_INT_1V2:
		*nrf = NRF_COMP_REF_INT_1V2;
		break;

#if defined(COMP_REFSEL_REFSEL_Int1V8)
	case COMP_NRF_COMP_REFSEL_INT_1V8:
		*nrf = NRF_COMP_REF_INT_1V8;
		break;
#endif

#if defined(COMP_REFSEL_REFSEL_Int2V4)
	case COMP_NRF_COMP_REFSEL_INT_2V4:
		*nrf = NRF_COMP_REF_INT_2V4;
		break;
#endif

#if defined(COMP_REFSEL_REFSEL_AVDDAO1V8)
	case COMP_NRF_COMP_REFSEL_AVDDAO1V8:
		*nrf = NRF_COMP_REF_AVDDAO1V8;
		break;
#endif

#if defined(COMP_REFSEL_REFSEL_VDD)
	case COMP_NRF_COMP_REFSEL_VDD:
		*nrf = NRF_COMP_REF_VDD;
		break;
#endif

	case COMP_NRF_COMP_REFSEL_AREF:
		*nrf = NRF_COMP_REF_AREF;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int shim_nrf_comp_se_config_to_nrf(const struct comp_nrf_comp_se_config *shim,
					  nrfx_comp_config_t *nrf)
{
	if (shim_nrf_comp_refsel_to_nrf(shim->refsel, &nrf->reference)) {
		return -EINVAL;
	}

	if (shim_nrf_comp_extrefsel_to_nrf(shim->extrefsel, &nrf->ext_ref)) {
		return -EINVAL;
	}

	nrf->main_mode = NRF_COMP_MAIN_MODE_SE;

	if (shim->th_down > 63 || shim->th_up > 63) {
		return -EINVAL;
	}

	nrf->threshold.th_down = shim->th_down;
	nrf->threshold.th_up = shim->th_up;

	if (shim_nrf_comp_sp_mode_to_nrf(shim->sp_mode, &nrf->speed_mode)) {
		return -EINVAL;
	}

	nrf->hyst = NRF_COMP_HYST_NO_HYST;

#if NRF_COMP_HAS_ISOURCE
	if (shim_nrf_comp_isource_to_nrf(shim->isource, &nrf->isource)) {
		return -EINVAL;
	}
#else
	if (shim->isource != COMP_NRF_COMP_ISOURCE_DISABLED) {
		return -EINVAL;
	}
#endif

	if (shim_nrf_comp_psel_to_nrf(shim->psel, &nrf->input)) {
		return -EINVAL;
	}

	nrf->interrupt_priority = 0;
	return 0;
}

static int shim_nrf_comp_diff_config_to_nrf(const struct comp_nrf_comp_diff_config *shim,
					    nrfx_comp_config_t *nrf)
{
	nrf->reference = NRF_COMP_REF_AREF;

	if (shim_nrf_comp_extrefsel_to_nrf(shim->extrefsel, &nrf->ext_ref)) {
		return -EINVAL;
	}

	nrf->main_mode = NRF_COMP_MAIN_MODE_DIFF;
	nrf->threshold.th_down = 0;
	nrf->threshold.th_up = 0;

	if (shim_nrf_comp_sp_mode_to_nrf(shim->sp_mode, &nrf->speed_mode)) {
		return -EINVAL;
	}

#if NRF_COMP_HAS_HYST
	if (shim->enable_hyst) {
		nrf->hyst = NRF_COMP_HYST_ENABLED;
	} else {
		nrf->hyst = NRF_COMP_HYST_DISABLED;
	}
#else
	if (shim->enable_hyst) {
		return -EINVAL;
	}
#endif

#if NRF_COMP_HAS_ISOURCE
	if (shim_nrf_comp_isource_to_nrf(shim->isource, &nrf->isource)) {
		return -EINVAL;
	}
#else
	if (shim->isource != COMP_NRF_COMP_ISOURCE_DISABLED) {
		return -EINVAL;
	}
#endif

	if (shim_nrf_comp_psel_to_nrf(shim->psel, &nrf->input)) {
		return -EINVAL;
	}

	nrf->interrupt_priority = 0;
	return 0;
}

static int shim_nrf_comp_get_output(const struct device *dev)
{
	ARG_UNUSED(dev);

	return nrfx_comp_sample();
}

static int shim_nrf_comp_set_trigger(const struct device *dev,
				     enum comparator_trigger trigger)
{
	shim_nrf_comp_stop();

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		shim_nrf_comp_data0.event_mask = 0;
		break;

	case COMPARATOR_TRIGGER_RISING_EDGE:
		shim_nrf_comp_data0.event_mask = NRF_COMP_INT_UP_MASK;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		shim_nrf_comp_data0.event_mask = NRF_COMP_INT_DOWN_MASK;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		shim_nrf_comp_data0.event_mask = NRF_COMP_INT_CROSS_MASK;
		break;
	}

	if (shim_nrf_comp_is_resumed()) {
		shim_nrf_comp_start();
	}

	return 0;
}

static int shim_nrf_comp_set_trigger_callback(const struct device *dev,
					      comparator_callback_t callback,
					      void *user_data)
{
	shim_nrf_comp_stop();

	shim_nrf_comp_data0.callback = callback;
	shim_nrf_comp_data0.user_data = user_data;

	if (callback != NULL && atomic_test_and_clear_bit(&shim_nrf_comp_data0.triggered, 0)) {
		callback(dev, user_data);
	}

	if (shim_nrf_comp_is_resumed()) {
		shim_nrf_comp_start();
	}

	return 0;
}

static int shim_nrf_comp_trigger_is_pending(const struct device *dev)
{
	ARG_UNUSED(dev);

	return atomic_test_and_clear_bit(&shim_nrf_comp_data0.triggered, 0);
}

static const struct comparator_driver_api shim_nrf_comp_api = {
	.get_output = shim_nrf_comp_get_output,
	.set_trigger = shim_nrf_comp_set_trigger,
	.set_trigger_callback = shim_nrf_comp_set_trigger_callback,
	.trigger_is_pending = shim_nrf_comp_trigger_is_pending,
};

static int shim_nrf_comp_reconfigure(const nrfx_comp_config_t *nrf)
{
	shim_nrf_comp_stop();

	(void)nrfx_comp_reconfigure(nrf);

	if (shim_nrf_comp_is_resumed()) {
		shim_nrf_comp_start();
	}

	return 0;
}

int comp_nrf_comp_configure_se(const struct device *dev,
			       const struct comp_nrf_comp_se_config *config)
{
	nrfx_comp_config_t nrf = {};

	ARG_UNUSED(dev);

	if (shim_nrf_comp_se_config_to_nrf(config, &nrf)) {
		return -EINVAL;
	}

	return shim_nrf_comp_reconfigure(&nrf);
}

int comp_nrf_comp_configure_diff(const struct device *dev,
				 const struct comp_nrf_comp_diff_config *config)
{
	nrfx_comp_config_t nrf = {};

	ARG_UNUSED(dev);

	if (shim_nrf_comp_diff_config_to_nrf(config, &nrf)) {
		return -EINVAL;
	}

	return shim_nrf_comp_reconfigure(&nrf);
}

static void shim_nrf_comp_event_handler(nrf_comp_event_t event)
{
	ARG_UNUSED(event);

	if (shim_nrf_comp_data0.callback == NULL) {
		atomic_set_bit(&shim_nrf_comp_data0.triggered, 0);
		return;
	}

	shim_nrf_comp_data0.callback(DEVICE_DT_INST_GET(0), shim_nrf_comp_data0.user_data);
	atomic_clear_bit(&shim_nrf_comp_data0.triggered, 0);
}

static int shim_nrf_comp_init(const struct device *dev)
{
	nrfx_comp_config_t nrf = {};

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    nrfx_isr,
		    nrfx_comp_irq_handler,
		    0);

	irq_enable(DT_INST_IRQN(0));

#if SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_SE(0)
	(void)shim_nrf_comp_se_config_to_nrf(&shim_nrf_comp_config0, &nrf);
#else
	(void)shim_nrf_comp_diff_config_to_nrf(&shim_nrf_comp_config0, &nrf);
#endif

	if (nrfx_comp_init(&nrf, shim_nrf_comp_event_handler) != NRFX_SUCCESS) {
		return -ENODEV;
	}

	return pm_device_driver_init(dev, shim_nrf_comp_pm_callback);
}

PM_DEVICE_DT_INST_DEFINE(0, shim_nrf_comp_pm_callback);

DEVICE_DT_INST_DEFINE(0,
		      shim_nrf_comp_init,
		      PM_DEVICE_DT_INST_GET(0),
		      NULL,
		      NULL,
		      POST_KERNEL,
		      CONFIG_COMPARATOR_INIT_PRIORITY,
		      &shim_nrf_comp_api);
