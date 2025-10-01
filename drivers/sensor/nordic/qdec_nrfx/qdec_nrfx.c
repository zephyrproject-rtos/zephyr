/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/sensor/qdec_nrf.h>
#include <soc.h>

#include <nrfx_qdec.h>
#include <hal/nrf_gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(qdec_nrfx, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_qdec

#define FULL_ANGLE 360

/* limit range to avoid overflow when converting steps to degrees */
#define ACC_MAX (INT_MAX / FULL_ANGLE)
#define ACC_MIN (INT_MIN / FULL_ANGLE)

BUILD_ASSERT(NRF_QDEC_SAMPLEPER_128US == SAMPLEPER_128US,
	     "Different SAMPLEPER register values in devicetree binding and nRF HAL");
BUILD_ASSERT(NRF_QDEC_SAMPLEPER_256US == SAMPLEPER_256US,
	     "Different SAMPLEPER register values in devicetree binding and nRF HAL");
BUILD_ASSERT(NRF_QDEC_SAMPLEPER_512US == SAMPLEPER_512US,
	     "Different SAMPLEPER register values in devicetree binding and nRF HAL");
BUILD_ASSERT(NRF_QDEC_SAMPLEPER_1024US == SAMPLEPER_1024US,
	     "Different SAMPLEPER register values in devicetree binding and nRF HAL");
BUILD_ASSERT(NRF_QDEC_SAMPLEPER_2048US == SAMPLEPER_2048US,
	     "Different SAMPLEPER register values in devicetree binding and nRF HAL");
BUILD_ASSERT(NRF_QDEC_SAMPLEPER_4096US == SAMPLEPER_4096US,
	     "Different SAMPLEPER register values in devicetree binding and nRF HAL");
BUILD_ASSERT(NRF_QDEC_SAMPLEPER_8192US == SAMPLEPER_8192US,
	     "Different SAMPLEPER register values in devicetree binding and nRF HAL");
BUILD_ASSERT(NRF_QDEC_SAMPLEPER_16384US == SAMPLEPER_16384US,
	     "Different SAMPLEPER register values in devicetree binding and nRF HAL");

struct qdec_nrfx_data {
	int32_t fetched_acc;
	int32_t acc;
	bool overflow;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
};

struct qdec_nrfx_config {
	nrfx_qdec_t qdec;
	nrfx_qdec_config_t config;
	void (*irq_connect)(void);
	const struct pinctrl_dev_config *pcfg;
	uint32_t enable_pin;
	int32_t steps;
};

static void accumulate(struct qdec_nrfx_data *data, int32_t acc)
{
	unsigned int key = irq_lock();

	bool overflow = ((acc > 0) && (ACC_MAX - acc < data->acc)) ||
			((acc < 0) && (ACC_MIN - acc > data->acc));

	if (!overflow) {
		data->acc += acc;
	} else {
		data->overflow = true;
	}

	irq_unlock(key);
}

static int qdec_nrfx_sample_fetch(const struct device *dev,
				  enum sensor_channel chan)
{
	const struct qdec_nrfx_config *config = dev->config;
	struct qdec_nrfx_data *data = dev->data;
	int32_t acc;
	uint32_t accdbl;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_ROTATION)) {
		return -ENOTSUP;
	}

	nrfx_qdec_accumulators_read(&config->qdec, &acc, &accdbl);

	accumulate(data, acc);

	unsigned int key = irq_lock();

	data->fetched_acc = data->acc;
	data->acc = 0;

	irq_unlock(key);

	if (data->overflow) {
		data->overflow = false;
		return -EOVERFLOW;
	}

	return 0;
}

static int qdec_nrfx_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct qdec_nrfx_data *data = dev->data;
	const struct qdec_nrfx_config *config = dev->config;
	unsigned int key;
	int32_t acc;

	if (chan != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	key = irq_lock();
	acc = data->fetched_acc;
	irq_unlock(key);

	val->val1 = (acc * FULL_ANGLE) / config->steps;
	val->val2 = (acc * FULL_ANGLE) - (val->val1 * config->steps);
	if (val->val2 != 0) {
		val->val2 *= 1000000;
		val->val2 /= config->steps;
	}

	return 0;
}

static int qdec_nrfx_trigger_set(const struct device *dev,
				 const struct sensor_trigger *trig,
				 sensor_trigger_handler_t handler)
{
	struct qdec_nrfx_data *data = dev->data;
	unsigned int key;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if ((trig->chan != SENSOR_CHAN_ALL) &&
	    (trig->chan != SENSOR_CHAN_ROTATION)) {
		return -ENOTSUP;
	}

	key = irq_lock();
	data->data_ready_handler = handler;
	data->data_ready_trigger = trig;
	irq_unlock(key);

	return 0;
}

static void qdec_nrfx_event_handler(nrfx_qdec_event_t event, void *p_context)
{
	const struct device *dev = p_context;
	struct qdec_nrfx_data *dev_data = dev->data;

	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trig;
	unsigned int key;

	switch (event.type) {
	case NRF_QDEC_EVENT_REPORTRDY:
		accumulate(dev_data, event.data.report.acc);

		key = irq_lock();
		handler = dev_data->data_ready_handler;
		trig = dev_data->data_ready_trigger;
		irq_unlock(key);

		if (handler) {
			handler(dev, trig);
		}
		break;

	case NRF_QDEC_EVENT_ACCOF:
		dev_data->overflow = true;
		break;

	default:
		LOG_ERR("unhandled event (0x%x)", event.type);
		break;
	}
}

static void qdec_nrfx_gpio_ctrl(const struct device *dev, bool enable)
{
	const struct qdec_nrfx_config *config = dev->config;

	if (config->enable_pin != NRF_QDEC_PIN_NOT_CONNECTED) {

		uint32_t val = (enable)?(0):(1);

		nrf_gpio_pin_write(config->enable_pin, val);
		nrf_gpio_cfg_output(config->enable_pin);
	}
}

static DEVICE_API(sensor, qdec_nrfx_driver_api) = {
	.sample_fetch = qdec_nrfx_sample_fetch,
	.channel_get  = qdec_nrfx_channel_get,
	.trigger_set  = qdec_nrfx_trigger_set,
};

static void qdec_pm_suspend(const struct device *dev)
{
	const struct qdec_nrfx_config *config = dev->config;

	nrfx_qdec_disable(&config->qdec);
	qdec_nrfx_gpio_ctrl(dev, false);

	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
}

static void qdec_pm_resume(const struct device *dev)
{
	const struct qdec_nrfx_config *config = dev->config;

	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	qdec_nrfx_gpio_ctrl(dev, true);
	nrfx_qdec_enable(&config->qdec);
}

static int qdec_nrfx_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		qdec_pm_resume(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		if (IS_ENABLED(CONFIG_PM_DEVICE)) {
			qdec_pm_suspend(dev);
		}
		break;
	default:
		return -ENOTSUP;
		break;
	}

	return 0;
}

static int qdec_nrfx_init(const struct device *dev)
{
	const struct qdec_nrfx_config *config = dev->config;
	nrfx_err_t nerr;

	config->irq_connect();

	nerr = nrfx_qdec_init(&config->qdec, &config->config, qdec_nrfx_event_handler, (void *)dev);
	if (nerr != NRFX_SUCCESS) {
		return (nerr == NRFX_ERROR_INVALID_STATE) ? -EBUSY : -EFAULT;
	}

	/* End up in suspend state. */
	qdec_nrfx_gpio_ctrl(dev, false);
	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	}

	return pm_device_driver_init(dev, qdec_nrfx_pm_action);
}

#define QDEC(idx)			DT_NODELABEL(qdec##idx)
#define QDEC_PROP(idx, prop)		DT_PROP(QDEC(idx), prop)

/* Macro determines PM actions interrupt safety level.
 *
 * Requesting/releasing QDEC device may be ISR safe, but it cannot be reliably known whether
 * managing its power domain is. It is then assumed that if power domains are used, device is
 * no longer ISR safe. This macro let's us check if we will be requesting/releasing
 * power domains and determines PM device ISR safety value.
 */
#define QDEC_PM_ISR_SAFE(idx)									\
	COND_CODE_1(										\
		UTIL_AND(									\
			IS_ENABLED(CONFIG_PM_DEVICE_POWER_DOMAIN),				\
			UTIL_AND(								\
				DT_NODE_HAS_PROP(QDEC(idx), power_domains),			\
				DT_NODE_HAS_STATUS_OKAY(DT_PHANDLE(QDEC(idx), power_domains))	\
			)									\
		),										\
		(0),										\
		(PM_DEVICE_ISR_SAFE)								\
	)

#define SENSOR_NRFX_QDEC_DEVICE(idx)							     \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(QDEC(idx));					     \
	BUILD_ASSERT(QDEC_PROP(idx, steps) > 0,						     \
		     "Wrong QDEC"#idx" steps setting in dts. Only positive number valid");   \
	BUILD_ASSERT(QDEC_PROP(idx, steps) <= 2048,					     \
		     "Wrong QDEC"#idx" steps setting in dts. Overflow possible");	     \
	static void irq_connect##idx(void)						     \
	{										     \
		IRQ_CONNECT(DT_IRQN(QDEC(idx)), DT_IRQ(QDEC(idx), priority),		     \
			    nrfx_isr, nrfx_qdec_##idx##_irq_handler, 0);		     \
	}										     \
	static struct qdec_nrfx_data qdec_##idx##_data;					     \
	PINCTRL_DT_DEFINE(QDEC(idx));							     \
	static struct qdec_nrfx_config qdec_##idx##_config = {				     \
		.qdec = NRFX_QDEC_INSTANCE(idx),					     \
		.config = {								     \
			.reportper = NRF_QDEC_REPORTPER_40,				     \
			.sampleper = DT_STRING_TOKEN(QDEC(idx), nordic_period),              \
			.skip_gpio_cfg = true,						     \
			.skip_psel_cfg = true,						     \
			.ledpre  = QDEC_PROP(idx, led_pre),				     \
			.ledpol  = NRF_QDEC_LEPOL_ACTIVE_HIGH,				     \
			.reportper_inten = true,					     \
		},									     \
		.irq_connect = irq_connect##idx,					     \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(QDEC(idx)),				     \
		.enable_pin = DT_PROP_OR(QDEC(idx), enable_pin, NRF_QDEC_PIN_NOT_CONNECTED), \
		.steps = QDEC_PROP(idx, steps),						     \
	};										     \
	PM_DEVICE_DT_DEFINE(QDEC(idx), qdec_nrfx_pm_action, QDEC_PM_ISR_SAFE(idx));	     \
	SENSOR_DEVICE_DT_DEFINE(QDEC(idx),						     \
				qdec_nrfx_init,						     \
				PM_DEVICE_DT_GET(QDEC(idx)),				     \
				&qdec_##idx##_data,					     \
				&qdec_##idx##_config,					     \
				POST_KERNEL,						     \
				CONFIG_SENSOR_INIT_PRIORITY,				     \
				&qdec_nrfx_driver_api)

#ifdef CONFIG_HAS_HW_NRF_QDEC0
SENSOR_NRFX_QDEC_DEVICE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_QDEC1
SENSOR_NRFX_QDEC_DEVICE(1);
#endif

#ifdef CONFIG_HAS_HW_NRF_QDEC20
SENSOR_NRFX_QDEC_DEVICE(20);
#endif

#ifdef CONFIG_HAS_HW_NRF_QDEC21
SENSOR_NRFX_QDEC_DEVICE(21);
#endif

#ifdef CONFIG_HAS_HW_NRF_QDEC130
SENSOR_NRFX_QDEC_DEVICE(130);
#endif

#ifdef CONFIG_HAS_HW_NRF_QDEC131
SENSOR_NRFX_QDEC_DEVICE(131);
#endif
