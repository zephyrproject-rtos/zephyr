/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sensor.h>
#include <drivers/clock_control.h>
#include "nrf_clock_calibration.h"
#include <hal/nrf_clock.h>
#include <logging/log.h>
#include <stdlib.h>

LOG_MODULE_DECLARE(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* For platforms that do not have CTSTOPPED event CT timer can be started
 * immediately after stop. Redefined events to avoid ifdefs in the code,
 * CTSTOPPED interrupt handling will be removed during compilation.
 */
#ifndef CLOCK_EVENTS_CTSTOPPED_EVENTS_CTSTOPPED_Msk
#define NRF_CLOCK_EVENT_CTSTOPPED 0
#endif

#ifndef CLOCK_INTENSET_CTSTOPPED_Msk
#define NRF_CLOCK_INT_CTSTOPPED_MASK 0
#endif

#define TEMP_SENSOR_NAME \
	COND_CODE_1(CONFIG_TEMP_NRF5, (CONFIG_TEMP_NRF5_NAME), (NULL))

/* Calibration state enum */
enum nrf_cal_state {
	CAL_OFF,
	CAL_IDLE,	/* Calibration timer active, waiting for expiration. */
	CAL_HFCLK_REQ,	/* HFCLK XTAL requested. */
	CAL_TEMP_REQ,	/* Temperature measurement requested. */
	CAL_ACTIVE,	/* Ongoing calibration. */
	CAL_ACTIVE_OFF	/* Ongoing calibration, off requested. */
};

static enum nrf_cal_state cal_state; /* Calibration state. */
static s16_t prev_temperature; /* Previous temperature measurement. */
static u8_t calib_skip_cnt; /* Counting down skipped calibrations. */
static int total_cnt; /* Total number of calibrations. */
static int total_skips_cnt; /* Total number of skipped calibrations. */

/* Callback called on hfclk started. */
static void cal_hf_on_callback(struct device *dev, void *user_data);
static struct clock_control_async_data cal_hf_on_data = {
	.cb = cal_hf_on_callback
};

static struct device *hfclk_dev; /* Handler to hfclk device. */
static struct device *temp_sensor; /* Handler to temperature sensor device. */

static void measure_temperature(struct k_work *work);
static K_WORK_DEFINE(temp_measure_work, measure_temperature);

static bool clock_event_check_and_clean(u32_t evt, u32_t intmask)
{
	bool ret = nrf_clock_event_check(evt) &&
			nrf_clock_int_enable_check(intmask);

	if (ret) {
		nrf_clock_event_clear(evt);
	}

	return ret;
}

bool z_nrf_clock_calibration_start(struct device *dev)
{
	bool ret;
	int key = irq_lock();

	if (cal_state != CAL_ACTIVE_OFF) {
		ret = true;
	} else {
		ret = false;
	}

	cal_state = CAL_IDLE;

	irq_unlock(key);

	calib_skip_cnt = 0;

	return ret;
}

void z_nrf_clock_calibration_lfclk_started(struct device *dev)
{
	/* Trigger unconditional calibration when lfclk is started. */
	cal_state = CAL_HFCLK_REQ;
	clock_control_async_on(hfclk_dev, 0, &cal_hf_on_data);
}

bool z_nrf_clock_calibration_stop(struct device *dev)
{
	int key;
	bool ret = true;

	key = irq_lock();

	nrf_clock_task_trigger(NRF_CLOCK_TASK_CTSTOP);
	nrf_clock_event_clear(NRF_CLOCK_EVENT_CTTO);

	/* If calibration is active then pend until completed.
	 * Currently (and most likely in the future), LFCLK is never stopped so
	 * it is not an issue.
	 */
	if (cal_state == CAL_ACTIVE) {
		cal_state = CAL_ACTIVE_OFF;
		ret = false;
	} else {
		cal_state = CAL_OFF;
	}

	irq_unlock(key);
	LOG_DBG("Stop requested %s.", (cal_state == CAL_ACTIVE_OFF) ?
			"during ongoing calibration" : "");

	return ret;
}

void z_nrf_clock_calibration_init(struct device *dev)
{
	/* Anomaly 36: After watchdog timeout reset, CPU lockup reset, soft
	 * reset, or pin reset EVENTS_DONE and EVENTS_CTTO are not reset.
	 */
	nrf_clock_event_clear(NRF_CLOCK_EVENT_DONE);
	nrf_clock_event_clear(NRF_CLOCK_EVENT_CTTO);

	nrf_clock_int_enable(NRF_CLOCK_INT_DONE_MASK |
			     NRF_CLOCK_INT_CTTO_MASK |
			     NRF_CLOCK_INT_CTSTOPPED_MASK);
	nrf_clock_cal_timer_timeout_set(
			CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD);

	if (CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP != 0) {
		temp_sensor = device_get_binding(TEMP_SENSOR_NAME);
	}

	hfclk_dev = dev;
	total_cnt = 0;
	total_skips_cnt = 0;
}

/* Start calibration assuming that HFCLK XTAL is on. */
static void start_calibration(void)
{
	cal_state = CAL_ACTIVE;

	/* Workaround for Errata 192 */
	if (IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)) {
		*(volatile uint32_t *)0x40000C34 = 0x00000002;
	}

	nrf_clock_task_trigger(NRF_CLOCK_TASK_CAL);
	calib_skip_cnt = CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP;
}

/* Restart calibration timer, release HFCLK XTAL. */
static void to_idle(void)
{
	cal_state = CAL_IDLE;
	clock_control_off(hfclk_dev, 0);
	nrf_clock_task_trigger(NRF_CLOCK_TASK_CTSTART);
}

/* Convert sensor value to 0.25'C units. */
static inline s16_t sensor_value_to_temp_unit(struct sensor_value *val)
{
	return (s16_t)(4 * val->val1 + val->val2 / 250000);
}

/* Function reads from temperature sensor and converts to 0.25'C units. */
static s16_t get_temperature(void)
{
	struct sensor_value sensor_val;

	sensor_sample_fetch(temp_sensor);
	sensor_channel_get(temp_sensor, SENSOR_CHAN_DIE_TEMP, &sensor_val);

	return sensor_value_to_temp_unit(&sensor_val);
}

/* Function determines if calibration should be performed based on temperature
 * measurement. Function is called from system work queue context. It is
 * reading temperature from TEMP sensor and compares with last measurement.
 */
static void measure_temperature(struct k_work *work)
{
	s16_t temperature;
	s16_t diff;
	bool started = false;
	int key;

	temperature = get_temperature();
	diff = abs(temperature - prev_temperature);

	key = irq_lock();

	if (cal_state != CAL_OFF) {
		if ((calib_skip_cnt == 0) ||
		    (diff >= CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_TEMP_DIFF)) {
			prev_temperature = temperature;
			start_calibration();
			started = true;
		} else {
			to_idle();
			calib_skip_cnt--;
			total_skips_cnt++;
		}
	}

	irq_unlock(key);

	LOG_DBG("Calibration %s. Temperature diff: %d (in 0.25'C units).",
			started ? "started" : "skipped", diff);
}

/* Called when HFCLK XTAL is on. Schedules temperature measurement or triggers
 * calibration.
 */
static void cal_hf_on_callback(struct device *dev, void *user_data)
{
	int key = irq_lock();

	if (cal_state == CAL_HFCLK_REQ) {
		if ((CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP == 0) ||
		    (IS_ENABLED(CONFIG_MULTITHREADING) == false)) {
			start_calibration();
		} else {
			cal_state = CAL_TEMP_REQ;
			k_work_submit(&temp_measure_work);
		}
	} else {
		clock_control_off(hfclk_dev, 0);
	}

	irq_unlock(key);
}

static void on_cal_done(void)
{
	/* Workaround for Errata 192 */
	if (IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)) {
		*(volatile uint32_t *)0x40000C34 = 0x00000000;
	}

	total_cnt++;
	LOG_DBG("Calibration done.");

	int key = irq_lock();

	if (cal_state == CAL_ACTIVE_OFF) {
		clock_control_off(hfclk_dev, 0);
		nrf_clock_task_trigger(NRF_CLOCK_TASK_LFCLKSTOP);
		cal_state = CAL_OFF;
	} else {
		to_idle();
	}

	irq_unlock(key);
}

void z_nrf_clock_calibration_force_start(void)
{
	int key = irq_lock();

	calib_skip_cnt = 0;

	if (cal_state == CAL_IDLE) {
		cal_state = CAL_HFCLK_REQ;
		clock_control_async_on(hfclk_dev, 0, &cal_hf_on_data);
	}

	irq_unlock(key);
}

void z_nrf_clock_calibration_isr(void)
{
	if (clock_event_check_and_clean(NRF_CLOCK_EVENT_CTTO,
						NRF_CLOCK_INT_CTTO_MASK)) {
		LOG_DBG("Calibration timeout.");

		/* Start XTAL HFCLK. It is needed for temperature measurement
		 * and calibration.
		 */
		if (cal_state == CAL_IDLE) {
			cal_state = CAL_HFCLK_REQ;
			clock_control_async_on(hfclk_dev, 0, &cal_hf_on_data);
		}
	}

	if (clock_event_check_and_clean(NRF_CLOCK_EVENT_DONE,
					NRF_CLOCK_INT_DONE_MASK)) {
		on_cal_done();
	}

	if (NRF_CLOCK_INT_CTSTOPPED_MASK &&
	    clock_event_check_and_clean(NRF_CLOCK_EVENT_CTSTOPPED,
			NRF_CLOCK_INT_CTSTOPPED_MASK)) {
		LOG_INF("CT stopped.");
		if (cal_state == CAL_IDLE) {
			/* If LF clock was restarted then CT might not be
			 * started because it was not yet stopped.
			 */
			LOG_INF("restarting");
			nrf_clock_task_trigger(NRF_CLOCK_TASK_CTSTART);
		}
	}
}

int z_nrf_clock_calibration_count(void)
{
	if (!IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG)) {
		return -1;
	}

	return total_cnt;
}

int z_nrf_clock_calibration_skips_count(void)
{
	if (!IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG)) {
		return -1;
	}

	return total_skips_cnt;
}
