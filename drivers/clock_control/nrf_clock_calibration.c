/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/sensor.h>
#include <drivers/clock_control.h>
#include "nrf_clock_calibration.h"
#include <drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_clock.h>
#include <logging/log.h>
#include <stdlib.h>

LOG_MODULE_DECLARE(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/**
 * Terms:
 * - calibration - overall process of LFRC clock calibration which is performed
 *   periodically, calibration may include temperature monitoring, hf XTAL
 *   starting and stopping.
 * - cycle - all calibration phases (waiting, temperature monitoring,
 *   calibration).
 * - process - calibration process which may consists of hf XTAL clock
 *   requesting, performing hw calibration and releasing hf clock.
 * - hw_cal - calibration action performed by the hardware.
 *
 * Those terms are later on used in function names.
 *
 * In order to ensure that low frequency clock is not released when calibration
 * is ongoing, it is requested by the calibration process and released when
 * calibration is done.
 */

static atomic_t cal_process_in_progress;
static int16_t prev_temperature; /* Previous temperature measurement. */
static uint8_t calib_skip_cnt; /* Counting down skipped calibrations. */
static volatile int total_cnt; /* Total number of calibrations. */
static volatile int total_skips_cnt; /* Total number of skipped calibrations. */


static void cal_hf_callback(struct onoff_manager *mgr,
			    struct onoff_client *cli,
			    uint32_t state, int res);
static void cal_lf_callback(struct onoff_manager *mgr,
			    struct onoff_client *cli,
			    uint32_t state, int res);

static struct onoff_client cli;
static struct onoff_manager *mgrs;

static struct device *temp_sensor;

static void measure_temperature(struct k_work *work);
static K_WORK_DEFINE(temp_measure_work, measure_temperature);

static void timeout_handler(struct k_timer *timer);
static K_TIMER_DEFINE(backoff_timer, timeout_handler, NULL);

static void clk_request(struct onoff_manager *mgr, struct onoff_client *cli,
			onoff_client_callback callback)
{
	int err;

	sys_notify_init_callback(&cli->notify, callback);
	err = onoff_request(mgr, cli);
	__ASSERT_NO_MSG(err >= 0);
}

static void clk_release(struct onoff_manager *mgr)
{
	int err;

	err = onoff_release(mgr);
	__ASSERT_NO_MSG(err >= 0);
}

static void hf_request(void)
{
	clk_request(&mgrs[CLOCK_CONTROL_NRF_TYPE_HFCLK], &cli, cal_hf_callback);
}

static void lf_request(void)
{
	clk_request(&mgrs[CLOCK_CONTROL_NRF_TYPE_LFCLK], &cli, cal_lf_callback);
}

static void hf_release(void)
{
	clk_release(&mgrs[CLOCK_CONTROL_NRF_TYPE_HFCLK]);
}

static void lf_release(void)
{
	clk_release(&mgrs[CLOCK_CONTROL_NRF_TYPE_LFCLK]);
}

static void cal_lf_callback(struct onoff_manager *mgr,
			    struct onoff_client *cli,
			    uint32_t state, int res)
{
	hf_request();
}

/* Start actual HW calibration assuming that HFCLK XTAL is on. */
static void start_hw_cal(void)
{
	/* Workaround for Errata 192 */
	if (IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)) {
		*(volatile uint32_t *)0x40000C34 = 0x00000002;
	}

	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_CAL);
	calib_skip_cnt = CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP;
}

/* Start cycle by starting backoff timer and releasing HFCLK XTAL. */
static void start_cycle(void)
{
	k_timer_start(&backoff_timer,
		      K_MSEC(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD),
		      K_NO_WAIT);
	hf_release();

	if (!IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_LF_ALWAYS_ON)) {
		lf_release();
	}

	cal_process_in_progress = 0;
}

static void start_cal_process(void)
{
	if (atomic_cas(&cal_process_in_progress, 0, 1) == false) {
		return;
	}

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_LF_ALWAYS_ON)) {
		hf_request();
	} else {
		/* LF clock is probably running but it is requested to ensure
		 * that it is not released while calibration process in ongoing.
		 * If system releases the clock during calibration process it
		 * will be released at the end of calibration process and
		 * stopped in consequence.
		 */
		lf_request();
	}
}

static void timeout_handler(struct k_timer *timer)
{
	start_cal_process();
}

/* Called when HFCLK XTAL is on. Schedules temperature measurement or triggers
 * calibration.
 */
static void cal_hf_callback(struct onoff_manager *mgr,
			    struct onoff_client *cli,
			    uint32_t state, int res)
{
	if ((temp_sensor == NULL) || !IS_ENABLED(CONFIG_MULTITHREADING)) {
		start_hw_cal();
	} else {
		k_work_submit(&temp_measure_work);
	}
}

static void on_hw_cal_done(void)
{
	/* Workaround for Errata 192 */
	if (IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)) {
		*(volatile uint32_t *)0x40000C34 = 0x00000000;
	}

	total_cnt++;
	LOG_DBG("Calibration done.");

	start_cycle();
}

/* Convert sensor value to 0.25'C units. */
static inline int16_t sensor_value_to_temp_unit(struct sensor_value *val)
{
	return (int16_t)(4 * val->val1 + val->val2 / 250000);
}

/* Function reads from temperature sensor and converts to 0.25'C units. */
static int get_temperature(int16_t *tvp)
{
	struct sensor_value sensor_val;
	int rc = sensor_sample_fetch(temp_sensor);

	if (rc == 0) {
		rc = sensor_channel_get(temp_sensor, SENSOR_CHAN_DIE_TEMP,
					&sensor_val);
	}
	if (rc == 0) {
		*tvp = sensor_value_to_temp_unit(&sensor_val);
	}
	return rc;
}

/* Function determines if calibration should be performed based on temperature
 * measurement. Function is called from system work queue context. It is
 * reading temperature from TEMP sensor and compares with last measurement.
 */
static void measure_temperature(struct k_work *work)
{
	int16_t temperature = 0;
	int16_t diff = 0;
	bool started = false;
	int rc;

	rc = get_temperature(&temperature);

	if (rc != 0) {
		/* Temperature read failed, force calibration. */
		calib_skip_cnt = 0;
	} else {
		diff = abs(temperature - prev_temperature);
	}

	if ((calib_skip_cnt == 0) ||
		(diff >= CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_TEMP_DIFF)) {
		prev_temperature = temperature;
		started = true;
		start_hw_cal();
	} else {
		calib_skip_cnt--;
		total_skips_cnt++;
		start_cycle();
	}

	LOG_DBG("Calibration %s. Temperature diff: %d (in 0.25'C units).",
			started ? "started" : "skipped", diff);
}

#define TEMP_NODE DT_INST(0, nordic_nrf_temp)

#if DT_NODE_HAS_STATUS(TEMP_NODE, okay)
static inline struct device *temp_device(void)
{
	return device_get_binding(DT_LABEL(TEMP_NODE));
}
#else
#define temp_device() NULL
#endif

void z_nrf_clock_calibration_init(struct onoff_manager *onoff_mgrs)
{
	/* Anomaly 36: After watchdog timeout reset, CPU lockup reset, soft
	 * reset, or pin reset EVENTS_DONE and EVENTS_CTTO are not reset.
	 */
	nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_DONE);
	nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_DONE_MASK);

	mgrs = onoff_mgrs;
	total_cnt = 0;
	total_skips_cnt = 0;
}

#if CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP
static int temp_sensor_init(struct device *arg)
{
	temp_sensor = temp_device();

	return 0;
}

SYS_INIT(temp_sensor_init, APPLICATION, 0);
#endif /* CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP */

static void start_unconditional_cal_process(void)
{
	calib_skip_cnt = 0;
	start_cal_process();
}

void z_nrf_clock_calibration_force_start(void)
{
	/* if it's already in progress that is good enough. */
	if (cal_process_in_progress) {
		return;
	}

	start_unconditional_cal_process();
}

void z_nrf_clock_calibration_lfclk_started(void)
{
	start_unconditional_cal_process();
}

void z_nrf_clock_calibration_lfclk_stopped(void)
{
	k_timer_stop(&backoff_timer);
	LOG_DBG("Calibration stopped");
}

void z_nrf_clock_calibration_isr(void)
{
	if (nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_DONE)) {
		nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_DONE);
		on_hw_cal_done();
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
