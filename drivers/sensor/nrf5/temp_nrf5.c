/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_temp

#include <device.h>
#include <init.h>
#include <drivers/sensor.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <logging/log.h>
#include <hal/nrf_temp.h>
#include <drivers/sensor/temp_nrf5.h>

LOG_MODULE_REGISTER(temp_nrf5, CONFIG_SENSOR_LOG_LEVEL);

/* The nRF5 temperature device returns measurements in 0.25C
 * increments.  Scale to mDegrees C.
 */
#define TEMP_NRF5_TEMP_SCALE (1000000 / 4)

static int32_t temperature;
static temp_nrf5_callback_t user_callback;
static void *ctx;
static struct device *clk;

static void hfclk_on_callback(struct device *dev, clock_control_subsys_t subsys,
			      void *user_data)
{
	nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_START);
}

static struct clock_control_async_data clkd = {
	.cb = hfclk_on_callback
};

/* Interrupt is enabled in init function thus it is used to determine if
 * device is initialized.
 */
static bool temp_nrf5_is_init(void)
{
	return nrf_temp_int_enable_check(NRF_TEMP, NRF_TEMP_INT_DATARDY_MASK);
}

int temp_nrf5_sample(temp_nrf5_callback_t callback, void *user_data)
{
	if (!temp_nrf5_is_init()) {
		return -EAGAIN;
	}

	if (atomic_cas((atomic_t *)&user_callback, 0, (uintptr_t)callback) ==
		false) {
		return -EBUSY;
	}

	ctx = user_data;
	nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY);

	if (IS_ENABLED(CONFIG_TEMP_NRF5_BETTER_ACCURACY)) {
		return clock_control_async_on(clk, CLOCK_CONTROL_NRF_SUBSYS_HF,
						&clkd);
	} else {
		nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_START);
		return 0;
	}
}

static void temp_callback(int32_t sample, void *user_data)
{
	struct k_sem *sem = user_data;

	temperature = sample;
	k_sem_give(sem);
}

static int temp_nrf5_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct k_sem sem = Z_SEM_INITIALIZER(sem, 0, 1);
	int err;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	do {
		err = temp_nrf5_sample(temp_callback, &sem);
		if (err >= 0) {
			break;
		} else if (err == -EBUSY) {
			/* if sensor is busy then sleep and retry. */
			k_sleep(K_MSEC(1));
		} else {
			return err;
		}
	} while (true);

	k_sem_take(&sem, K_FOREVER);

	return 0;
}

static int temp_nrf5_channel_get(struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	int32_t uval;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	uval = temperature * TEMP_NRF5_TEMP_SCALE;
	val->val1 = uval / 1000000;
	val->val2 = uval % 1000000;

	LOG_DBG("Temperature:%d,%d", val->val1, val->val2);

	return 0;
}

static void temp_nrf5_isr(void)
{
	temp_nrf5_callback_t callback;

	nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY);
	nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_STOP);

	if (IS_ENABLED(CONFIG_TEMP_NRF5_BETTER_ACCURACY)) {
		clock_control_off(clk, CLOCK_CONTROL_NRF_SUBSYS_HF);
	}
	callback = user_callback;
	user_callback = NULL;
	callback(nrf_temp_result_get(NRF_TEMP), ctx);
}

DEVICE_DECLARE(temp_nrf5);

static int temp_nrf5_init(struct device *dev)
{
	IRQ_CONNECT(
		DT_INST_IRQN(0),
		DT_INST_IRQ(0, priority),
		temp_nrf5_isr,
		DEVICE_GET(temp_nrf5),
		0);
	irq_enable(DT_INST_IRQN(0));

	nrf_temp_int_enable(NRF_TEMP, NRF_TEMP_INT_DATARDY_MASK);

	if (IS_ENABLED(CONFIG_TEMP_NRF5_BETTER_ACCURACY)) {
		clk =
		     device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
	}

	return (clk == NULL) ? -ENODEV : 0;
}
static const __unused struct sensor_driver_api temp_nrf5_driver_api = {
	.sample_fetch = temp_nrf5_sample_fetch,
	.channel_get = temp_nrf5_channel_get,
};

#ifdef CONFIG_TEMP_NRF5_SENSOR

DEVICE_AND_API_INIT(temp_nrf5, DT_INST_LABEL(0),
		    temp_nrf5_init, NULL, NULL,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &temp_nrf5_driver_api);
#else
SYS_INIT(temp_nrf5_init, POST_KERNEL, 0);
#endif
