/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <haly/nrfy_lpcomp.h>

#include <zephyr/drivers/comparator.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT nordic_nrf_lpcomp

#define SHIM_NRF_LPCOMP_DT_INST_REF_SEL(inst) \
	_CONCAT(NRF_LPCOMP_REF_, DT_INST_STRING_TOKEN(inst, reference_selection))

#define SHIM_NRF_LPCOMP_DT_INST_EXT_REF(inst) \
	_CONCAT(NRF_LPCOMP_EXT_REF_, DT_INST_STRING_TOKEN(inst, external_reference))

#define SHIM_NRF_LPCOMP_DT_INST_HYST(inst) \
	_CONCAT(NRF_LPCOMP_HYST_, DT_INST_STRING_TOKEN(inst, hysteresis))

#define SHIM_NRF_LPCOMP_DT_INST_POS_INPUT(inst) \
	_CONCAT(NRF_LPCOMP_, DT_INST_STRING_TOKEN(inst, positive_input))

struct shim_nrf_lpcomp_data {
	uint32_t int_mask;
	comparator_callback_t callback;
	void *user_data;
};

static const nrfy_lpcomp_config_t shim_nrf_lpcomp_config0 = {
	.reference = SHIM_NRF_LPCOMP_DT_INST_REF_SEL(0),
	.ext_ref = SHIM_NRF_LPCOMP_DT_INST_EXT_REF(0),
	.detection = NRF_LPCOMP_DETECT_CROSS,
#if NRF_LPCOMP_HAS_HYST
	.hyst = SHIM_NRF_LPCOMP_DT_INST_HYST(0),
#endif
	.input = SHIM_NRF_LPCOMP_DT_INST_POS_INPUT(0),
};

static struct shim_nrf_lpcomp_data shim_nrf_lpcomp_data0;

static int shim_nrf_lpcomp_get_output(const struct device *dev)
{
	ARG_UNUSED(dev);

	return nrfy_lpcomp_sample_check(NRF_LPCOMP) ? 1 : 0;
}

static int shim_nrf_lpcomp_set_trigger(const struct device *dev,
				       enum comparator_trigger trigger)
{
	nrf_lpcomp_detect_t detection;

	ARG_UNUSED(dev);

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		shim_nrf_lpcomp_data0.int_mask = 0;
		detection = NRF_LPCOMP_DETECT_CROSS;
		break;

	case COMPARATOR_TRIGGER_RISING_EDGE:
		shim_nrf_lpcomp_data0.int_mask = NRF_LPCOMP_INT_UP_MASK;
		detection = NRF_LPCOMP_DETECT_UP;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		shim_nrf_lpcomp_data0.int_mask = NRF_LPCOMP_INT_DOWN_MASK;
		detection = NRF_LPCOMP_DETECT_DOWN;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		shim_nrf_lpcomp_data0.int_mask = NRF_LPCOMP_INT_CROSS_MASK;
		detection = NRF_LPCOMP_DETECT_CROSS;
		break;

	default:
		return -EINVAL;
	}

	nrfy_lpcomp_int_disable(NRF_LPCOMP, UINT32_MAX);

	if (shim_nrf_lpcomp_data0.callback != NULL) {
		nrfy_lpcomp_int_enable(NRF_LPCOMP, shim_nrf_lpcomp_data0.int_mask);
	}

	/* Configure detection to match trigger to allow the comparator to wake up system */
	nrf_lpcomp_detection_set(NRF_LPCOMP, detection);
	return 0;
}

static int shim_nrf_lpcomp_set_trigger_callback(const struct device *dev,
						comparator_callback_t callback,
						void *user_data)
{
	if (shim_nrf_lpcomp_data0.int_mask) {
		nrfy_lpcomp_int_disable(NRF_LPCOMP, UINT32_MAX);
	}

	shim_nrf_lpcomp_data0.callback = callback;
	shim_nrf_lpcomp_data0.user_data = user_data;

	if (shim_nrf_lpcomp_data0.callback == NULL) {
		return 0;
	}

	if (shim_nrf_lpcomp_data0.int_mask) {
		nrfy_lpcomp_int_enable(NRF_LPCOMP, shim_nrf_lpcomp_data0.int_mask);
	}

	return 0;
}

static int shim_nrf_lpcomp_trigger_is_pending(const struct device *dev)
{
	int ret = 0;

	if (shim_nrf_lpcomp_data0.int_mask == NRF_LPCOMP_INT_UP_MASK &&
	    nrfy_lpcomp_event_check(NRF_LPCOMP, NRF_LPCOMP_EVENT_UP)) {
		ret = 1;
		nrf_lpcomp_event_clear(NRF_LPCOMP, NRF_LPCOMP_EVENT_UP);
	}

	if (shim_nrf_lpcomp_data0.int_mask == NRF_LPCOMP_INT_DOWN_MASK &&
	    nrfy_lpcomp_event_check(NRF_LPCOMP, NRF_LPCOMP_EVENT_DOWN)) {
		ret = 1;
		nrf_lpcomp_event_clear(NRF_LPCOMP, NRF_LPCOMP_EVENT_DOWN);
	}

	if (shim_nrf_lpcomp_data0.int_mask == NRF_LPCOMP_INT_CROSS_MASK &&
	    nrfy_lpcomp_event_check(NRF_LPCOMP, NRF_LPCOMP_EVENT_CROSS)) {
		ret = 1;
		nrf_lpcomp_event_clear(NRF_LPCOMP, NRF_LPCOMP_EVENT_CROSS);
	}

	return ret;
}

static const struct comparator_driver_api shim_nrf_lpcomp_api = {
	.get_output = shim_nrf_lpcomp_get_output,
	.set_trigger = shim_nrf_lpcomp_set_trigger,
	.set_trigger_callback = shim_nrf_lpcomp_set_trigger_callback,
	.trigger_is_pending = shim_nrf_lpcomp_trigger_is_pending,
};

static void shim_nrf_lpcomp_resume(void)
{
	nrfy_lpcomp_enable(NRF_LPCOMP);
	nrfy_lpcomp_task_trigger(NRF_LPCOMP, NRF_LPCOMP_TASK_START);
}

#if PM_DEVICE
static void shim_nrf_lpcomp_suspend(const struct device *dev)
{
	nrfy_lpcomp_task_trigger(NRF_LPCOMP, NRF_LPCOMP_TASK_STOP);
	nrfy_lpcomp_disable(NRF_LPCOMP);
}
#endif

static int shim_nrf_lpcomp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	ARG_UNUSED(dev);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		shim_nrf_lpcomp_resume();
		ret = 0;
		break;

#if PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		shim_nrf_lpcomp_suspend();
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

static void shim_nrf_lpcomp_irq_handler(const void *arg)
{
	ARG_UNUSED(arg);

	nrfy_lpcomp_event_clear(NRF_LPCOMP, NRF_LPCOMP_EVENT_UP);
	nrfy_lpcomp_event_clear(NRF_LPCOMP, NRF_LPCOMP_EVENT_DOWN);
	nrfy_lpcomp_event_clear(NRF_LPCOMP, NRF_LPCOMP_EVENT_CROSS);
	nrfy_lpcomp_event_clear(NRF_LPCOMP, NRF_LPCOMP_EVENT_READY);

	shim_nrf_lpcomp_data0.callback(DEVICE_DT_INST_GET(0), shim_nrf_lpcomp_data0.user_data);
}

static int shim_nrf_lpcomp_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    nrfx_isr,
		    shim_nrf_lpcomp_irq_handler,
		    0);

	nrfy_lpcomp_shorts_disable(NRF_LPCOMP, UINT32_MAX);
	nrfy_lpcomp_int_disable(NRF_LPCOMP, UINT32_MAX);
	irq_enable(DT_INST_IRQN(0));
	nrfy_lpcomp_periph_configure(NRF_LPCOMP, &shim_nrf_lpcomp_config0);
	return pm_device_driver_init(dev, shim_nrf_lpcomp_pm_callback);
}

PM_DEVICE_DT_INST_DEFINE(0, shim_nrf_lpcomp_pm_callback);

DEVICE_DT_INST_DEFINE(
	0,
	shim_nrf_lpcomp_init,
	PM_DEVICE_DT_INST_GET(0),
	NULL,
	NULL,
	POST_KERNEL,
	CONFIG_COMPARATOR_INIT_PRIORITY,
	&shim_nrf_lpcomp_api
);
