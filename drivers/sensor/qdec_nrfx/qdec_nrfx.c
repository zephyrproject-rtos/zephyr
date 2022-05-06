/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#include <nrfx_qdec.h>
#include <hal/nrf_gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(qdec_nrfx, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_qdec

#define FULL_ANGLE 360

/* limit range to avoid overflow when converting steps to degrees */
#define ACC_MAX (INT_MAX / FULL_ANGLE)
#define ACC_MIN (INT_MIN / FULL_ANGLE)


struct qdec_nrfx_data {
	int32_t                    acc;
	sensor_trigger_handler_t data_ready_handler;
};

static struct qdec_nrfx_data qdec_nrfx_data;

#ifdef CONFIG_PINCTRL
PINCTRL_DT_DEFINE(DT_DRV_INST(0));
static const struct pinctrl_dev_config *qdec_nrfx_pcfg =
	PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(0));
#endif

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
	BUILD_ASSERT(steps <= 2048, "overflow possible");

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

NRF_DT_CHECK_PIN_ASSIGNMENTS(DT_DRV_INST(0), 1, a_pin, b_pin, led_pin);

static int qdec_nrfx_init(const struct device *dev)
{
	static const nrfx_qdec_config_t config = {
		.reportper = NRF_QDEC_REPORTPER_40,
		.sampleper = NRF_QDEC_SAMPLEPER_2048us,
#ifdef CONFIG_PINCTRL
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
#else
		.psela   = DT_INST_PROP(0, a_pin),
		.pselb   = DT_INST_PROP(0, b_pin),
		.pselled = DT_INST_PROP_OR(0, led_pin,
					   NRF_QDEC_LED_NOT_CONNECTED),
#endif
		.ledpre  = DT_INST_PROP(0, led_pre),
		.ledpol  = NRF_QDEC_LEPOL_ACTIVE_HIGH,
	};

	nrfx_err_t nerr;

	LOG_DBG("");

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, nrfx_qdec_irq_handler, 0);

#ifdef CONFIG_PINCTRL
	int ret = pinctrl_apply_state(qdec_nrfx_pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		return ret;
	}
#endif

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

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int qdec_nrfx_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	int ret = 0;
	ARG_UNUSED(dev);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(qdec_nrfx_pcfg,
					  PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
#endif
		qdec_nrfx_gpio_ctrl(true);
		nrfx_qdec_enable();
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		/* device must be uninitialized */
		nrfx_qdec_uninit();
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(qdec_nrfx_pcfg,
					  PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
#endif
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		/* device must be suspended */
		nrfx_qdec_disable();
		qdec_nrfx_gpio_ctrl(false);
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(qdec_nrfx_pcfg,
					  PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */


static const struct sensor_driver_api qdec_nrfx_driver_api = {
	.sample_fetch = qdec_nrfx_sample_fetch,
	.channel_get  = qdec_nrfx_channel_get,
	.trigger_set  = qdec_nrfx_trigger_set,
};

PM_DEVICE_DT_INST_DEFINE(0, qdec_nrfx_pm_action);

DEVICE_DT_INST_DEFINE(0, qdec_nrfx_init,
		PM_DEVICE_DT_INST_GET(0), NULL, NULL, POST_KERNEL,
		CONFIG_SENSOR_INIT_PRIORITY, &qdec_nrfx_driver_api);
