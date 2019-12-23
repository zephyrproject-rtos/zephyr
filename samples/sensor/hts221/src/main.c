/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/util.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

static void process_sample(struct device *dev)
{
	static unsigned int obs;
	struct sensor_value temp, hum;

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
		LOG_ERR("Cannot read HTS221 temperature channel");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
		LOG_ERR("Cannot read HTS221 humidity channel");
		return;
	}

	obs++;
	LOG_INF("Observation:%u", obs);

	/* display temperature */
	LOG_INF("Temperature:%d.%08d C", temp.val1, temp.val2);

	/* display humidity */
	LOG_INF("Relative Humidity:%d.%08d", hum.val1, hum.val2);
}

static void sensor_callback(int res, void *user_data)
{
	struct device *sensor = user_data;
	process_sample(sensor);
}

static struct async_client cli;

static void expired(struct k_timer *timer)
{
	int err = sensor_sample_fetch_async_chan(k_timer_user_data_get(timer),
					SENSOR_CHAN_ALL, &cli);
	if (err != 0) {
		LOG_ERR("err:%d", err);
	}
}

K_TIMER_DEFINE(timer, expired, NULL);

static void hts221_handler(struct device *dev,
			   struct sensor_trigger *trig)
{
	process_sample(dev);
}

#ifdef CONFIG_PPI_TRACE
/* Using PPI trace from nrf connect sdk to output hw events on pins and
 * measure precisely CPU active period.
 */
#include <debug/ppi_trace.h>
#include <hal/nrf_power.h>

static void cpu_load_ppi_trace_setup(void)
{
	u32_t start_evt;
	u32_t stop_evt;
	void *handle;

	start_evt = nrf_power_event_address_get(NRF_POWER,
				NRF_POWER_EVENT_SLEEPEXIT);
	stop_evt = nrf_power_event_address_get(NRF_POWER,
				NRF_POWER_EVENT_SLEEPENTER);

	handle = ppi_trace_pair_config(4,
				start_evt, stop_evt);
	__ASSERT(handle != NULL, "Failed to configure PPI trace pair.\n");

	ppi_trace_enable(handle);
}
#endif

void main(void)
{
#ifdef CONFIG_PPI_TRACE
	cpu_load_ppi_trace_setup();
#endif
	struct device *dev = device_get_binding("HTS221");
	async_client_init_callback(&cli, sensor_callback, dev);

	if (dev == NULL) {
		LOG_ERR("Could not get HTS221 device");
		k_sleep(K_FOREVER);
		return;
	}

	if (IS_ENABLED(CONFIG_I2C_USE_MNGR)) {
		k_timer_user_data_set(&timer, dev);
		k_timer_start(&timer, K_MSEC(1000), K_MSEC(1000));

		/* Start by using synchronous read, rest of reads happen in
		 * system timer interrupt.
		 */
		if (sensor_sample_fetch(dev) < 0) {
			LOG_ERR("Sensor sample update error");
			return;
		}
		process_sample(dev);

		k_sleep(K_FOREVER);
		return;
	}

	if (IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ALL,
		};
		if (sensor_trigger_set(dev, &trig, hts221_handler) < 0) {
			printf("Cannot configure trigger");
			return;
		};
	}

	while (!IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
		if (sensor_sample_fetch(dev) < 0) {
			LOG_ERR("Sensor sample update error");
			return;
		}
		process_sample(dev);
		k_sleep(K_MSEC(2000));
	}
	k_sleep(K_FOREVER);
}
