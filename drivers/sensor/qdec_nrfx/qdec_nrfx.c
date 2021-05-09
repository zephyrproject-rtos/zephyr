/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/sensor.h>

#include <nrfx_qdec.h>
#include <hal/nrf_gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(qdec_nrfx, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_qdec

#define FULL_ANGLE 360

/* limit range to avoid overflow when converting steps to degrees */
#define ACC_MAX (INT_MAX / FULL_ANGLE)
#define ACC_MIN (INT_MIN / FULL_ANGLE)


struct qdec_nrfx_data {
	int32_t                    acc;
	sensor_trigger_handler_t data_ready_handler;
#ifdef CONFIG_PM_DEVICE
	uint32_t                    pm_state;
#endif
};


static struct qdec_nrfx_data qdec_nrfx_data;

static void accumulate(struct qdec_nrfx_data *data, int16_t acc)
{
	unsigned int key = irq_lock();

	bool overflow = ((acc > 0) && (ACC_MAX - acc < data->acc)) ||
			((acc < 0) && (ACC_MIN - acc > data->acc));

	if (!overflow) {
		data->acc += acc;
	}

	irq_unlock(key);
}

static int qdec_nrfx_sample_fetch(const struct device *dev,
				  enum sensor_channel chan)
{
	struct qdec_nrfx_data *data = &qdec_nrfx_data;

	int16_t acc;
	int16_t accdbl;

	ARG_UNUSED(dev);

	LOG_DBG("");

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_ROTATION)) {
		return -ENOTSUP;
	}

	nrfx_qdec_accumulators_read(&acc, &accdbl);

	accumulate(data, acc);

	return 0;
}

static int qdec_nrfx_channel_get(const struct device *dev,
				 enum sensor_channel  chan,
				 struct sensor_value *val)
{
	struct qdec_nrfx_data *data = &qdec_nrfx_data;
	unsigned int key;
	int32_t acc;
	const int32_t steps = DT_INST_PROP(0, steps);

	ARG_UNUSED(dev);
	LOG_DBG("");

	if (chan != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	key = irq_lock();
	acc = data->acc;
	data->acc = 0;
	irq_unlock(key);

	BUILD_ASSERT(steps > 0, "only positive number valid");
	BUILD_ASSERT(steps <= 2148, "overflow possible");

	val->val1 = (acc * FULL_ANGLE) / steps;
	val->val2 = (acc * FULL_ANGLE) - (val->val1 * steps);
	if (val->val2 != 0) {
		val->val2 *= 1000000;
		val->val2 /= steps;
	}

	return 0;
}

static int qdec_nrfx_trigger_set(const struct device *dev,
				 const struct sensor_trigger *trig,
				 sensor_trigger_handler_t     handler)
{
	struct qdec_nrfx_data *data = &qdec_nrfx_data;
	unsigned int key;

	ARG_UNUSED(dev);
	LOG_DBG("");

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if ((trig->chan != SENSOR_CHAN_ALL) &&
	    (trig->chan != SENSOR_CHAN_ROTATION)) {
		return -ENOTSUP;
	}

	key = irq_lock();
	data->data_ready_handler = handler;
	irq_unlock(key);

	return 0;
}

static void qdec_nrfx_event_handler(nrfx_qdec_event_t event)
{
	sensor_trigger_handler_t handler;
	unsigned int key;

	switch (event.type) {
	case NRF_QDEC_EVENT_REPORTRDY:
		accumulate(&qdec_nrfx_data, event.data.report.acc);

		key = irq_lock();
		handler = qdec_nrfx_data.data_ready_handler;
		irq_unlock(key);

		if (handler) {
			struct sensor_trigger trig = {
				.type = SENSOR_TRIG_DATA_READY,
				.chan = SENSOR_CHAN_ROTATION,
			};

			handler(DEVICE_DT_INST_GET(0), &trig);
		}
		break;

	default:
		LOG_ERR("unhandled event (0x%x)", event.type);
		break;
	}
}

static void qdec_nrfx_gpio_ctrl(bool enable)
{
#if DT_INST_NODE_HAS_PROP(0, enable_pin)
	uint32_t val = (enable)?(0):(1);

	nrf_gpio_pin_write(DT_INST_PROP(0, enable_pin), val);
	nrf_gpio_cfg_output(DT_INST_PROP(0, enable_pin));
#endif
}

static int qdec_nrfx_init(const struct device *dev)
{
	static const nrfx_qdec_config_t config = {
		.reportper          = NRF_QDEC_REPORTPER_40,
		.sampleper          = NRF_QDEC_SAMPLEPER_2048us,
		.psela              = DT_INST_PROP(0, a_pin),
		.pselb              = DT_INST_PROP(0, b_pin),
#if DT_INST_NODE_HAS_PROP(0, led_pin)
		.pselled            = DT_INST_PROP(0, led_pin),
#else
		.pselled            = 0xFFFFFFFF, /* disabled */
#endif
		.ledpre             = DT_INST_PROP(0, led_pre),
		.ledpol             = NRF_QDEC_LEPOL_ACTIVE_HIGH,
		.interrupt_priority = NRFX_QDEC_DEFAULT_CONFIG_IRQ_PRIORITY,
		.dbfen              = 0, /* disabled */
		.sample_inten       = 0, /* disabled */
	};

	nrfx_err_t nerr;

	LOG_DBG("");

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, nrfx_qdec_irq_handler, 0);

	nerr = nrfx_qdec_init(&config, qdec_nrfx_event_handler);
	if (nerr == NRFX_ERROR_INVALID_STATE) {
		LOG_ERR("qdec already in use");
		return -EBUSY;
	} else if (nerr != NRFX_SUCCESS) {
		LOG_ERR("failed to initialize qdec");
		return -EFAULT;
	}

	qdec_nrfx_gpio_ctrl(true);
	nrfx_qdec_enable();

#ifdef CONFIG_PM_DEVICE
	struct qdec_nrfx_data *data = &qdec_nrfx_data;

	data->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int qdec_nrfx_pm_get_state(struct qdec_nrfx_data *data,
				  uint32_t                 *state)
{
	unsigned int key = irq_lock();
	*state = data->pm_state;
	irq_unlock(key);

	return 0;
}

static int qdec_nrfx_pm_set_state(struct qdec_nrfx_data *data,
				  uint32_t                  new_state)
{
	uint32_t old_state;
	unsigned int key;

	key = irq_lock();
	old_state = data->pm_state;
	irq_unlock(key);

	if (old_state == new_state) {
		/* leave unchanged */
		return 0;
	}

	if (old_state == DEVICE_PM_ACTIVE_STATE) {
		/* device must be suspended */
		nrfx_qdec_disable();
		qdec_nrfx_gpio_ctrl(false);
	}

	if (new_state == DEVICE_PM_OFF_STATE) {
		/* device must be uninitialized */
		nrfx_qdec_uninit();
	}

	if (new_state == DEVICE_PM_ACTIVE_STATE) {
		qdec_nrfx_gpio_ctrl(true);
		nrfx_qdec_enable();
	}

	/* record the new state */
	key = irq_lock();
	data->pm_state = new_state;
	irq_unlock(key);

	return 0;
}

static int qdec_nrfx_pm_control(const struct device *dev,
				uint32_t ctrl_command,
				void *context, device_pm_cb cb, void *arg)
{
	struct qdec_nrfx_data *data = &qdec_nrfx_data;
	int err;

	LOG_DBG("");

	switch (ctrl_command) {
	case DEVICE_PM_GET_POWER_STATE:
		err = qdec_nrfx_pm_get_state(data, context);
		break;

	case DEVICE_PM_SET_POWER_STATE:
		err = qdec_nrfx_pm_set_state(data, *((uint32_t *)context));
		break;

	default:
		err = -ENOTSUP;
		break;
	}

	if (cb) {
		cb(dev, err, context, arg);
	}

	return err;
}

#endif /* CONFIG_PM_DEVICE */


static const struct sensor_driver_api qdec_nrfx_driver_api = {
	.sample_fetch = qdec_nrfx_sample_fetch,
	.channel_get  = qdec_nrfx_channel_get,
	.trigger_set  = qdec_nrfx_trigger_set,
};

DEVICE_DT_INST_DEFINE(0, qdec_nrfx_init,
		qdec_nrfx_pm_control, NULL, NULL, POST_KERNEL,
		CONFIG_SENSOR_INIT_PRIORITY, &qdec_nrfx_driver_api);
