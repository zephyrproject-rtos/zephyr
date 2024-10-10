/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx_lpcomp.h>

#include <zephyr/drivers/comparator/nrf_lpcomp.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <string.h>

#define DT_DRV_COMPAT nordic_nrf_lpcomp

#define SHIM_NRF_LPCOMP_DT_INST_REFSEL(inst) \
	_CONCAT(COMP_NRF_LPCOMP_REFSEL_, DT_INST_STRING_TOKEN(inst, refsel))

#define SHIM_NRF_LPCOMP_DT_INST_REFSEL_IS_AREF(inst) \
	DT_INST_ENUM_HAS_VALUE(inst, refsel, AREF)

#define SHIM_NRF_LPCOMP_DT_INST_EXTREFSEL(inst) \
	_CONCAT(COMP_NRF_LPCOMP_EXTREFSEL_, DT_INST_STRING_TOKEN(inst, extrefsel))

#define SHIM_NRF_LPCOMP_DT_INST_ENABLE_HYST(inst) \
	DT_INST_PROP(inst, enable_hyst)

#define SHIM_NRF_LPCOMP_DT_INST_PSEL(inst) \
	_CONCAT(COMP_NRF_LPCOMP_PSEL_, DT_INST_STRING_TOKEN(inst, psel))

struct shim_nrf_lpcomp_data {
	nrfx_lpcomp_config_t config;
	uint32_t event_mask;
	bool started;
	atomic_t triggered;
	comparator_callback_t callback;
	void *user_data;
};

#if (NRF_LPCOMP_HAS_AIN_AS_PIN)
static const uint32_t shim_nrf_lpcomp_ain_map[] = {
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

#if (NRF_LPCOMP_HAS_AIN_AS_PIN)
BUILD_ASSERT(COMP_NRF_LPCOMP_PSEL_AIN0 == 0);
BUILD_ASSERT(COMP_NRF_LPCOMP_PSEL_AIN7 == 7);
BUILD_ASSERT(COMP_NRF_LPCOMP_EXTREFSEL_AIN0 == 0);
BUILD_ASSERT(COMP_NRF_LPCOMP_EXTREFSEL_AIN1 == 1);
#endif

#if (LPCOMP_REFSEL_RESOLUTION == 8)
BUILD_ASSERT((SHIM_NRF_LPCOMP_DT_INST_REFSEL(0) < COMP_NRF_LPCOMP_REFSEL_VDD_1_16) ||
	     (SHIM_NRF_LPCOMP_DT_INST_REFSEL(0) > COMP_NRF_LPCOMP_REFSEL_VDD_15_16));
#endif

#if SHIM_NRF_LPCOMP_DT_INST_ENABLE_HYST(0)
BUILD_ASSERT(NRF_LPCOMP_HAS_HYST);
#endif

static struct shim_nrf_lpcomp_data shim_nrf_lpcomp_data0;

static const struct comp_nrf_lpcomp_config shim_nrf_lpcomp_config0 = {
	.psel = SHIM_NRF_LPCOMP_DT_INST_PSEL(0),
#if SHIM_NRF_LPCOMP_DT_INST_REFSEL_IS_AREF(0)
	.extrefsel = SHIM_NRF_LPCOMP_DT_INST_EXTREFSEL(0),
#endif
	.refsel = SHIM_NRF_LPCOMP_DT_INST_REFSEL(0),
	.enable_hyst = SHIM_NRF_LPCOMP_DT_INST_ENABLE_HYST(0),
};

#if CONFIG_PM_DEVICE
static bool shim_nrf_lpcomp_is_resumed(void)
{
	enum pm_device_state state;

	(void)pm_device_state_get(DEVICE_DT_INST_GET(0), &state);
	return state == PM_DEVICE_STATE_ACTIVE;
}
#else
static bool shim_nrf_lpcomp_is_resumed(void)
{
	return true;
}
#endif

static void shim_nrf_lpcomp_start(void)
{
	if (shim_nrf_lpcomp_data0.started) {
		return;
	}

	nrfx_lpcomp_start(shim_nrf_lpcomp_data0.event_mask, 0);
	shim_nrf_lpcomp_data0.started = true;
}

static void shim_nrf_lpcomp_stop(void)
{
	if (!shim_nrf_lpcomp_data0.started) {
		return;
	}

	nrfx_lpcomp_stop();
	shim_nrf_lpcomp_data0.started = false;
}

static int shim_nrf_lpcomp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);

	ARG_UNUSED(dev);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		shim_nrf_lpcomp_start();
		break;

#if CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		shim_nrf_lpcomp_stop();
		break;
#endif

	default:
		return -ENOTSUP;
	}

	return 0;
}

#if (NRF_LPCOMP_HAS_AIN_AS_PIN)
static int shim_nrf_lpcomp_psel_to_nrf(enum comp_nrf_lpcomp_psel shim,
				       nrf_lpcomp_input_t *nrf)
{
	if (shim >= ARRAY_SIZE(shim_nrf_lpcomp_ain_map)) {
		return -EINVAL;
	}

	*nrf = shim_nrf_lpcomp_ain_map[(uint32_t)shim];
	return 0;
}
#else
static int shim_nrf_lpcomp_psel_to_nrf(enum comp_nrf_lpcomp_psel shim,
				       nrf_lpcomp_input_t *nrf)
{
	switch (shim) {
	case COMP_NRF_LPCOMP_PSEL_AIN0:
		*nrf = NRF_LPCOMP_INPUT_0;
		break;

	case COMP_NRF_LPCOMP_PSEL_AIN1:
		*nrf = NRF_LPCOMP_INPUT_1;
		break;

	case COMP_NRF_LPCOMP_PSEL_AIN2:
		*nrf = NRF_LPCOMP_INPUT_2;
		break;

	case COMP_NRF_LPCOMP_PSEL_AIN3:
		*nrf = NRF_LPCOMP_INPUT_3;
		break;

	case COMP_NRF_LPCOMP_PSEL_AIN4:
		*nrf = NRF_LPCOMP_INPUT_4;
		break;

	case COMP_NRF_LPCOMP_PSEL_AIN5:
		*nrf = NRF_LPCOMP_INPUT_5;
		break;

	case COMP_NRF_LPCOMP_PSEL_AIN6:
		*nrf = NRF_LPCOMP_INPUT_6;
		break;

	case COMP_NRF_LPCOMP_PSEL_AIN7:
		*nrf = NRF_LPCOMP_INPUT_7;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}
#endif

#if (NRF_LPCOMP_HAS_AIN_AS_PIN)
static int shim_nrf_lpcomp_extrefsel_to_nrf(enum comp_nrf_lpcomp_extrefsel shim,
					    nrf_lpcomp_ext_ref_t *nrf)
{
	if (shim >= ARRAY_SIZE(shim_nrf_lpcomp_ain_map)) {
		return -EINVAL;
	}

	*nrf = shim_nrf_lpcomp_ain_map[shim];
	return 0;
}
#else
static int shim_nrf_lpcomp_extrefsel_to_nrf(enum comp_nrf_lpcomp_extrefsel shim,
					    nrf_lpcomp_ext_ref_t *nrf)
{
	switch (shim) {
	case COMP_NRF_LPCOMP_EXTREFSEL_AIN0:
		*nrf = NRF_LPCOMP_EXT_REF_REF0;
		break;

	case COMP_NRF_LPCOMP_EXTREFSEL_AIN1:
		*nrf = NRF_LPCOMP_EXT_REF_REF1;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static int shim_nrf_lpcomp_refsel_to_nrf(enum comp_nrf_lpcomp_refsel shim,
					 nrf_lpcomp_ref_t *nrf)
{
	switch (shim) {
	case COMP_NRF_LPCOMP_REFSEL_VDD_1_8:
		*nrf = NRF_LPCOMP_REF_SUPPLY_1_8;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_2_8:
		*nrf = NRF_LPCOMP_REF_SUPPLY_2_8;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_3_8:
		*nrf = NRF_LPCOMP_REF_SUPPLY_3_8;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_4_8:
		*nrf = NRF_LPCOMP_REF_SUPPLY_4_8;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_5_8:
		*nrf = NRF_LPCOMP_REF_SUPPLY_5_8;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_6_8:
		*nrf = NRF_LPCOMP_REF_SUPPLY_6_8;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_7_8:
		*nrf = NRF_LPCOMP_REF_SUPPLY_7_8;
		break;

#if (LPCOMP_REFSEL_RESOLUTION == 16)
	case COMP_NRF_LPCOMP_REFSEL_VDD_1_16:
		*nrf = NRF_LPCOMP_REF_SUPPLY_1_16;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_3_16:
		*nrf = NRF_LPCOMP_REF_SUPPLY_3_16;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_5_16:
		*nrf = NRF_LPCOMP_REF_SUPPLY_5_16;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_7_16:
		*nrf = NRF_LPCOMP_REF_SUPPLY_7_16;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_9_16:
		*nrf = NRF_LPCOMP_REF_SUPPLY_9_16;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_11_16:
		*nrf = NRF_LPCOMP_REF_SUPPLY_11_16;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_13_16:
		*nrf = NRF_LPCOMP_REF_SUPPLY_13_16;
		break;

	case COMP_NRF_LPCOMP_REFSEL_VDD_15_16:
		*nrf = NRF_LPCOMP_REF_SUPPLY_15_16;
		break;
#endif

	case COMP_NRF_LPCOMP_REFSEL_AREF:
		*nrf = NRF_LPCOMP_REF_EXT_REF;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int shim_nrf_lpcomp_config_to_nrf(const struct comp_nrf_lpcomp_config *shim,
					 nrfx_lpcomp_config_t *nrf)
{
	if (shim_nrf_lpcomp_refsel_to_nrf(shim->refsel, &nrf->reference)) {
		return -EINVAL;
	}

	if (shim_nrf_lpcomp_extrefsel_to_nrf(shim->extrefsel, &nrf->ext_ref)) {
		return -EINVAL;
	}

#if NRF_LPCOMP_HAS_HYST
	if (shim->enable_hyst) {
		nrf->hyst = NRF_LPCOMP_HYST_ENABLED;
	} else {
		nrf->hyst = NRF_LPCOMP_HYST_NOHYST;
	}
#else
	if (shim->enable_hyst) {
		return -EINVAL;
	}
#endif

	if (shim_nrf_lpcomp_psel_to_nrf(shim->psel, &nrf->input)) {
		return -EINVAL;
	}

	return 0;
}

static void shim_nrf_lpcomp_reconfigure(void)
{
	(void)nrfx_lpcomp_reconfigure(&shim_nrf_lpcomp_data0.config);
}

static int shim_nrf_lpcomp_get_output(const struct device *dev)
{
	ARG_UNUSED(dev);

	return nrfx_lpcomp_sample();
}

static int shim_nrf_lpcomp_set_trigger(const struct device *dev,
				       enum comparator_trigger trigger)
{
	shim_nrf_lpcomp_stop();

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		shim_nrf_lpcomp_data0.event_mask = 0;
		shim_nrf_lpcomp_data0.config.detection = NRF_LPCOMP_DETECT_CROSS;
		break;

	case COMPARATOR_TRIGGER_RISING_EDGE:
		shim_nrf_lpcomp_data0.event_mask = NRF_LPCOMP_INT_UP_MASK;
		shim_nrf_lpcomp_data0.config.detection = NRF_LPCOMP_DETECT_UP;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		shim_nrf_lpcomp_data0.event_mask = NRF_LPCOMP_INT_DOWN_MASK;
		shim_nrf_lpcomp_data0.config.detection = NRF_LPCOMP_DETECT_DOWN;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		shim_nrf_lpcomp_data0.event_mask = NRF_LPCOMP_INT_CROSS_MASK;
		shim_nrf_lpcomp_data0.config.detection = NRF_LPCOMP_DETECT_CROSS;
		break;
	}

	shim_nrf_lpcomp_reconfigure();

	if (shim_nrf_lpcomp_is_resumed()) {
		shim_nrf_lpcomp_start();
	}

	return 0;
}

static int shim_nrf_lpcomp_set_trigger_callback(const struct device *dev,
						comparator_callback_t callback,
						void *user_data)
{
	shim_nrf_lpcomp_stop();

	shim_nrf_lpcomp_data0.callback = callback;
	shim_nrf_lpcomp_data0.user_data = user_data;

	if (callback != NULL && atomic_test_and_clear_bit(&shim_nrf_lpcomp_data0.triggered, 0)) {
		callback(dev, user_data);
	}

	if (shim_nrf_lpcomp_is_resumed()) {
		shim_nrf_lpcomp_start();
	}

	return 0;
}

static int shim_nrf_lpcomp_trigger_is_pending(const struct device *dev)
{
	ARG_UNUSED(dev);

	return atomic_test_and_clear_bit(&shim_nrf_lpcomp_data0.triggered, 0);
}

static const struct comparator_driver_api shim_nrf_lpcomp_api = {
	.get_output = shim_nrf_lpcomp_get_output,
	.set_trigger = shim_nrf_lpcomp_set_trigger,
	.set_trigger_callback = shim_nrf_lpcomp_set_trigger_callback,
	.trigger_is_pending = shim_nrf_lpcomp_trigger_is_pending,
};

int comp_nrf_lpcomp_configure(const struct device *dev,
			      const struct comp_nrf_lpcomp_config *config)
{
	nrfx_lpcomp_config_t nrf = {};

	if (shim_nrf_lpcomp_config_to_nrf(config, &nrf)) {
		return -EINVAL;
	}

	memcpy(&shim_nrf_lpcomp_data0.config, &nrf, sizeof(shim_nrf_lpcomp_data0.config));

	shim_nrf_lpcomp_stop();
	shim_nrf_lpcomp_reconfigure();
	if (shim_nrf_lpcomp_is_resumed()) {
		shim_nrf_lpcomp_start();
	}

	return 0;
}

static void shim_nrf_lpcomp_event_handler(nrf_lpcomp_event_t event)
{
	ARG_UNUSED(event);

	if (shim_nrf_lpcomp_data0.callback == NULL) {
		atomic_set_bit(&shim_nrf_lpcomp_data0.triggered, 0);
		return;
	}

	shim_nrf_lpcomp_data0.callback(DEVICE_DT_INST_GET(0), shim_nrf_lpcomp_data0.user_data);
	atomic_clear_bit(&shim_nrf_lpcomp_data0.triggered, 0);
}

static int shim_nrf_lpcomp_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    nrfx_isr,
		    nrfx_lpcomp_irq_handler,
		    0);

	irq_enable(DT_INST_IRQN(0));

	(void)shim_nrf_lpcomp_config_to_nrf(&shim_nrf_lpcomp_config0,
					    &shim_nrf_lpcomp_data0.config);

	if (nrfx_lpcomp_init(&shim_nrf_lpcomp_data0.config,
			     shim_nrf_lpcomp_event_handler) != NRFX_SUCCESS) {
		return -ENODEV;
	}

	return pm_device_driver_init(dev, shim_nrf_lpcomp_pm_callback);
}

PM_DEVICE_DT_INST_DEFINE(0, shim_nrf_lpcomp_pm_callback);

DEVICE_DT_INST_DEFINE(0,
		      shim_nrf_lpcomp_init,
		      PM_DEVICE_DT_INST_GET(0),
		      NULL,
		      NULL,
		      POST_KERNEL,
		      CONFIG_COMPARATOR_INIT_PRIORITY,
		      &shim_nrf_lpcomp_api);
