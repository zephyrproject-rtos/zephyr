/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <haly/nrfy_comp.h>

#include <zephyr/drivers/comparator.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT nordic_nrf_comp

#define SHIM_NRF_COMP_DT_INST_REF_SEL(inst) \
	_CONCAT(NRF_COMP_REF_, DT_INST_STRING_TOKEN(inst, reference_selection))

#define SHIM_NRF_COMP_DT_INST_EXT_REF(inst) \
	_CONCAT(NRF_COMP_, DT_INST_STRING_TOKEN(inst, external_reference))

#define SHIM_NRF_COMP_DT_INST_MAIN_MODE(inst) \
	_CONCAT(NRF_COMP_MAIN_MODE_, DT_INST_STRING_TOKEN(inst, main_mode))

#define SHIM_NRF_COMP_DT_INST_TH_DOWN(inst) \
	DT_INST_PROP(inst, threshold_down)

#define SHIM_NRF_COMP_DT_INST_TH_UP(inst) \
	DT_INST_PROP(inst, threshold_up)

#define SHIM_NRF_COMP_DT_INST_SPEED_MODE(inst) \
	_CONCAT(NRF_COMP_SP_MODE_, DT_INST_STRING_TOKEN(inst, speed_mode))

#define SHIM_NRF_COMP_DT_INST_HYST(inst) \
	_CONCAT(NRF_COMP_HYST_, DT_INST_STRING_TOKEN(inst, hysteresis))

#define SHIM_NRF_COMP_DT_INST_ISOURCE(inst) \
	_CONCAT(NRF_COMP_ISOURCE_, DT_INST_STRING_TOKEN(inst, isource))

#define SHIM_NRF_COMP_DT_INST_POS_INPUT(inst) \
	_CONCAT(NRF_COMP_, DT_INST_STRING_TOKEN(inst, positive_input))

struct shim_nrf_comp_data {
	uint32_t int_mask;
	comparator_callback_t callback;
	void *user_data;
};

static const nrfy_comp_config_t shim_nrf_comp_config0 = {
	.reference = SHIM_NRF_COMP_DT_INST_REF_SEL(0),
	.ext_ref = SHIM_NRF_COMP_DT_INST_EXT_REF(0),
	.main_mode = SHIM_NRF_COMP_DT_INST_MAIN_MODE(0),
	.threshold.th_down = SHIM_NRF_COMP_DT_INST_TH_DOWN(0),
	.threshold.th_up = SHIM_NRF_COMP_DT_INST_TH_UP(0),
	.speed_mode = SHIM_NRF_COMP_DT_INST_SPEED_MODE(0),
	.hyst = SHIM_NRF_COMP_DT_INST_HYST(0),
#if NRF_COMP_HAS_ISOURCE
	.isource = SHIM_NRF_COMP_DT_INST_ISOURCE(0),
#endif
	.input = SHIM_NRF_COMP_DT_INST_POS_INPUT(0),
};

static struct shim_nrf_comp_data shim_nrf_comp_data0;

static int shim_nrf_comp_get_output(const struct device *dev)
{
	ARG_UNUSED(dev);

	return nrfy_comp_sample(NRF_COMP);
}

static int shim_nrf_comp_set_trigger(const struct device *dev,
				enum comparator_trigger trigger)
{
	ARG_UNUSED(dev);

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		shim_nrf_comp_data0.int_mask = 0;
		break;

	case COMPARATOR_TRIGGER_RISING_EDGE:
		shim_nrf_comp_data0.int_mask = NRF_COMP_INT_UP_MASK;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		shim_nrf_comp_data0.int_mask = NRF_COMP_INT_DOWN_MASK;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		shim_nrf_comp_data0.int_mask = NRF_COMP_INT_CROSS_MASK;
		break;

	default:
		return -EINVAL;
	}

	nrfy_comp_int_disable(NRF_COMP, UINT32_MAX);

	if (shim_nrf_comp_data0.callback != NULL) {
		nrfy_comp_int_enable(NRF_COMP, shim_nrf_comp_data0.int_mask);
	}

	return 0;
}

static int shim_nrf_comp_set_trigger_callback(const struct device *dev,
					      comparator_callback_t callback,
					      void *user_data)
{
	if (shim_nrf_comp_data0.int_mask) {
		nrfy_comp_int_disable(NRF_COMP, UINT32_MAX);
	}

	shim_nrf_comp_data0.callback = callback;
	shim_nrf_comp_data0.user_data = user_data;

	if (shim_nrf_comp_data0.callback == NULL) {
		return 0;
	}

	if (shim_nrf_comp_data0.int_mask) {
		nrfy_comp_int_enable(NRF_COMP, shim_nrf_comp_data0.int_mask);
	}

	return 0;
}

static int shim_nrf_comp_trigger_is_pending(const struct device *dev)
{
	int ret = 0;

	if (shim_nrf_comp_data0.int_mask == NRF_COMP_INT_UP_MASK &&
	    nrfy_comp_event_check(NRF_COMP, NRF_COMP_EVENT_UP)) {
		ret = 1;
		nrf_comp_event_clear(NRF_COMP, NRF_COMP_EVENT_UP);
	}

	if (shim_nrf_comp_data0.int_mask == NRF_COMP_INT_DOWN_MASK &&
	    nrfy_comp_event_check(NRF_COMP, NRF_COMP_EVENT_DOWN)) {
		ret = 1;
		nrf_comp_event_clear(NRF_COMP, NRF_COMP_EVENT_DOWN);
	}

	if (shim_nrf_comp_data0.int_mask == NRF_COMP_INT_CROSS_MASK &&
	    nrfy_comp_event_check(NRF_COMP, NRF_COMP_EVENT_CROSS)) {
		ret = 1;
		nrf_comp_event_clear(NRF_COMP, NRF_COMP_EVENT_CROSS);
	}

	return ret;
}

static const struct comparator_driver_api shim_nrf_comp_api = {
	.get_output = shim_nrf_comp_get_output,
	.set_trigger = shim_nrf_comp_set_trigger,
	.set_trigger_callback = shim_nrf_comp_set_trigger_callback,
	.trigger_is_pending = shim_nrf_comp_trigger_is_pending,
};

static void shim_nrf_comp_resume(void)
{
	nrfy_comp_enable(NRF_COMP);
	nrfy_comp_task_trigger(NRF_COMP, NRF_COMP_TASK_START);
}

#if PM_DEVICE
static void shim_nrf_comp_suspend(const struct device *dev)
{
	nrfy_comp_task_trigger(NRF_COMP, NRF_COMP_TASK_STOP);
	nrfy_comp_disable(NRF_COMP);
}
#endif

static int shim_nrf_comp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	ARG_UNUSED(dev);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		shim_nrf_comp_resume();
		ret = 0;
		break;

#if PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		shim_nrf_comp_suspend();
		ret = 0;
		break;
#endif

	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		ret = 0;
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static void shim_nrf_comp_irq_handler(const void *arg)
{
	ARG_UNUSED(arg);

	nrf_comp_event_clear(NRF_COMP, NRF_COMP_EVENT_UP);
	nrf_comp_event_clear(NRF_COMP, NRF_COMP_EVENT_DOWN);
	nrf_comp_event_clear(NRF_COMP, NRF_COMP_EVENT_CROSS);
	nrf_comp_event_clear(NRF_COMP, NRF_COMP_EVENT_READY);

	shim_nrf_comp_data0.callback(DEVICE_DT_INST_GET(0), shim_nrf_comp_data0.user_data);
}

static int shim_nrf_comp_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    nrfx_isr,
		    shim_nrf_comp_irq_handler,
		    0);

	nrfy_comp_shorts_disable(NRF_COMP, UINT32_MAX);
	nrfy_comp_int_disable(NRF_COMP, UINT32_MAX);
	irq_enable(DT_INST_IRQN(0));
	nrfy_comp_periph_configure(NRF_COMP, &shim_nrf_comp_config0);
	return pm_device_driver_init(dev, shim_nrf_comp_pm_callback);
}

PM_DEVICE_DT_INST_DEFINE(0, shim_nrf_comp_pm_callback);

DEVICE_DT_INST_DEFINE(
	0,
	shim_nrf_comp_init,
	PM_DEVICE_DT_INST_GET(0),
	NULL,
	NULL,
	POST_KERNEL,
	CONFIG_COMPARATOR_INIT_PRIORITY,
	&shim_nrf_comp_api
);
