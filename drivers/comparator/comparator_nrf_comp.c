/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx_comp.h>
#include <hal/nrf_gpio.h>

#include <zephyr/drivers/comparator/nrf_comp.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT nordic_nrf_comp

#define SHIM_NRF_COMP_DT_INST_REFSEL(inst) \
	_CONCAT(COMP_NRF_COMP_REFSEL_, DT_INST_STRING_TOKEN(inst, refsel))

#define SHIM_NRF_COMP_DT_INST_REFSEL_IS_AREF(inst) \
	DT_INST_ENUM_HAS_VALUE(inst, refsel, aref)

#define SHIM_NRF_COMP_DT_INST_EXTREFSEL(inst) DT_INST_PROP(inst, extrefsel)

#define SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_SE(inst) \
	DT_INST_ENUM_HAS_VALUE(inst, main_mode, se)

#define SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_DIFF(inst) \
	DT_INST_ENUM_HAS_VALUE(inst, main_mode, diff)

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

#define SHIM_NRF_COMP_DT_INST_PSEL(inst) DT_INST_PROP(inst, psel)

struct shim_nrf_comp_data {
	uint32_t event_mask;
	bool started;
	atomic_t triggered;
	comparator_callback_t callback;
	void *user_data;
};

#if SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_SE(0)
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_TH_DOWN(0) < 64);
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_TH_UP(0) < 64);
#endif

BUILD_ASSERT((NRF_COMP_AIN0 == NRFX_ANALOG_EXTERNAL_AIN0) &&
	     (NRF_COMP_AIN1 == NRFX_ANALOG_EXTERNAL_AIN1) &&
	     (NRF_COMP_AIN2 == NRFX_ANALOG_EXTERNAL_AIN2) &&
	     (NRF_COMP_AIN3 == NRFX_ANALOG_EXTERNAL_AIN3) &&
	     (NRF_COMP_AIN4 == NRFX_ANALOG_EXTERNAL_AIN4) &&
	     (NRF_COMP_AIN5 == NRFX_ANALOG_EXTERNAL_AIN5) &&
	     (NRF_COMP_AIN6 == NRFX_ANALOG_EXTERNAL_AIN6) &&
	     (NRF_COMP_AIN7 == NRFX_ANALOG_EXTERNAL_AIN7) &&
#if NRF_COMP_HAS_VDDH_DIV5
	     (NRF_COMP_AIN_VDDH_DIV5 == NRFX_ANALOG_INTERNAL_VDDHDIV5) &&
#endif
#if NRF_COMP_HAS_VDD_DIV2
	     (NRF_COMP_AIN_VDD_DIV2 == NRFX_ANALOG_INTERNAL_VDDDIV2) &&
#endif
	     1,
	     "Definitions from nrf-comp.h do not match those from nrfx_analog_common.h");

#if !NRF_COMP_HAS_SP_MODE_NORMAL
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_SP_MODE(0) != COMP_NRF_COMP_SP_MODE_NORMAL);
#endif

#if SHIM_NRF_COMP_DT_INST_MAIN_MODE_IS_SE(0)
#if !NRF_COMP_HAS_REF_INT_1V8
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_REFSEL(0) != COMP_NRF_COMP_REFSEL_INT_1V8);
#endif

#if !NRF_COMP_HAS_REF_INT_2V4
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_REFSEL(0) != COMP_NRF_COMP_REFSEL_INT_2V4);
#endif

#if !NRF_COMP_HAS_REF_AVDDAO1V8
BUILD_ASSERT(SHIM_NRF_COMP_DT_INST_REFSEL(0) != COMP_NRF_COMP_REFSEL_AVDDAO1V8);
#endif

#if !NRF_COMP_HAS_REF_VDD
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

static int shim_nrf_comp_sp_mode_to_nrf(enum comp_nrf_comp_sp_mode shim,
					nrf_comp_sp_mode_t *nrf)
{
	switch (shim) {
	case COMP_NRF_COMP_SP_MODE_LOW:
		*nrf = NRF_COMP_SP_MODE_LOW;
		break;

#if NRF_COMP_HAS_SP_MODE_NORMAL
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
					nrf_comp_isource_t *nrf)
{
	switch (shim) {
	case COMP_NRF_COMP_ISOURCE_DISABLED:
		*nrf = NRF_COMP_ISOURCE_OFF;
		break;
	case COMP_NRF_COMP_ISOURCE_2UA5:
		*nrf = NRF_COMP_ISOURCE_IEN_2UA5;
		break;
	case COMP_NRF_COMP_ISOURCE_5UA:
		*nrf = NRF_COMP_ISOURCE_IEN_5UA;
		break;
	case COMP_NRF_COMP_ISOURCE_10UA:
		*nrf = NRF_COMP_ISOURCE_IEN_10UA;
		break;
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

#if NRF_COMP_HAS_REF_INT_1V8
	case COMP_NRF_COMP_REFSEL_INT_1V8:
		*nrf = NRF_COMP_REF_INT_1V8;
		break;
#endif

#if NRF_COMP_HAS_REF_INT_2V4
	case COMP_NRF_COMP_REFSEL_INT_2V4:
		*nrf = NRF_COMP_REF_INT_2V4;
		break;
#endif

#if NRF_COMP_HAS_REF_AVDDAO1V8
	case COMP_NRF_COMP_REFSEL_AVDDAO1V8:
		*nrf = NRF_COMP_REF_AVDDAO1V8;
		break;
#endif

#if NRF_COMP_HAS_REF_VDD
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

	nrf->ext_ref = (nrfx_analog_input_t)shim->extrefsel;
	nrf->input = (nrfx_analog_input_t)shim->psel;

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

	nrf->interrupt_priority = 0;
	return 0;
}

static int shim_nrf_comp_diff_config_to_nrf(const struct comp_nrf_comp_diff_config *shim,
					    nrfx_comp_config_t *nrf)
{
	nrf->reference = NRF_COMP_REF_AREF;

	nrf->ext_ref = (nrfx_analog_input_t)shim->extrefsel;
	nrf->input = (nrfx_analog_input_t)shim->psel;

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

static DEVICE_API(comparator, shim_nrf_comp_api) = {
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

	if (nrfx_comp_init(&nrf, shim_nrf_comp_event_handler) != 0) {
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
