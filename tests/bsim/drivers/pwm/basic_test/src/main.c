/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <bstests.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <babblekit/testcase.h>
#include <helpers/nrfx_gppi.h>
#include <hal/nrf_gpio.h>
#include <nrfx_pwm.h>
#include <nrfx_timer.h>
#include "NRF_GPIO.h"

/*
 * Testcase 1 uses the Zephyr PWM driver on the DT enabled PWM instance.
 * Testcase 2 uses nrfx directly on a PWM instance which is not enabled in DT, and routes its
 * PWMPERIODEND event to a TIMER (in counter mode) COUNT task through (D)PPI.
 */
#if defined(CONFIG_SOC_SERIES_NRF54L)
/* The PWM output pins must be in the same domain as the PWM instance (P1 for PWM2x) */
#define DRV_PWM_NODE  DT_NODELABEL(pwm20)
#define DRV_PWM_PORT  1 /* As set by pwm20_default */
#define DRV_PWM_PIN   10
#define RAW_PWM_REG   NRF_PWM21
#define RAW_PWM_PORT  1
#define RAW_PWM_PIN   11
#define RAW_TIMER_REG NRF_TIMER21
/* Only the nRF54L PWM has DMA events and registers */
#define HAS_PWM_DMA   1
#define DMA_TIMER_REG NRF_TIMER22
#else
#define DRV_PWM_NODE  DT_NODELABEL(pwm0)
#define DRV_PWM_PORT  0 /* As set by pwm0_default */
#define DRV_PWM_PIN   13
#define RAW_PWM_REG   NRF_PWM1
#define RAW_PWM_PORT  0
#define RAW_PWM_PIN   14
#define RAW_TIMER_REG NRF_TIMER2
#define HAS_PWM_DMA   0
#endif

#define TOLERANCE_US 100
#define MAX_EDGES    128

struct edge {
	int64_t time_us;
	bool level;
};

static struct edge edges[MAX_EDGES];
static volatile unsigned int n_edges;
static unsigned int watched_port;
static unsigned int watched_pin;

static int64_t now_us(void)
{
	return k_ticks_to_us_near64(k_uptime_ticks());
}

static void output_callback(unsigned int port, unsigned int n, bool value)
{
	if ((port != watched_port) || (n != watched_pin) || (n_edges >= MAX_EDGES)) {
		return;
	}
	edges[n_edges].time_us = now_us();
	edges[n_edges].level = value;
	n_edges++;
}

static void watch_pin(unsigned int port, unsigned int pin)
{
	n_edges = 0;
	watched_port = port;
	watched_pin = pin;
	nrf_gpio_test_register_out_callback(output_callback);
}

static bool near(int64_t value, int64_t expected)
{
	int64_t diff = value - expected;

	return (diff >= -TOLERANCE_US) && (diff <= TOLERANCE_US);
}

/* Check the recorded edges belong to a PWM signal with the given period and high time */
static void check_waveform(int64_t period_us, int64_t high_us, unsigned int min_periods)
{
	unsigned int periods = 0;
	int64_t last_rise = -1;

	TEST_ASSERT(n_edges >= 2 * min_periods - 1, "Too few edges: %u", n_edges);

	for (unsigned int i = 0; i < n_edges; i++) {
		if (edges[i].level) {
			if (last_rise >= 0) {
				TEST_ASSERT(near(edges[i].time_us - last_rise, period_us),
					    "Unexpected period %lld us (expected %lld)",
					    edges[i].time_us - last_rise, period_us);
				periods++;
			}
			last_rise = edges[i].time_us;
		} else if (last_rise >= 0) {
			TEST_ASSERT(near(edges[i].time_us - last_rise, high_us),
				    "Unexpected high time %lld us (expected %lld)",
				    edges[i].time_us - last_rise, high_us);
		}
	}
	TEST_ASSERT(periods + 1 >= min_periods, "Too few periods: %u", periods);
}

static void pwm_driver_test(void)
{
	const struct device *pwm = DEVICE_DT_GET(DRV_PWM_NODE);
	const uint32_t period_ns = 1000000; /* 1 ms */
	int err;

	TEST_ASSERT(device_is_ready(pwm), "PWM device not ready");

	watch_pin(DRV_PWM_PORT, DRV_PWM_PIN);

	err = pwm_set(pwm, 0, period_ns, period_ns / 4, 0);
	TEST_ASSERT(err == 0, "pwm_set failed: %d", err);
	k_msleep(20);
	check_waveform(1000, 250, 15);

	/* Change the duty cycle while running */
	err = pwm_set(pwm, 0, period_ns, 3 * period_ns / 4, 0);
	TEST_ASSERT(err == 0, "pwm_set failed: %d", err);
	k_msleep(3); /* let the old duty cycle finish its last period */
	n_edges = 0;
	k_msleep(20);
	check_waveform(1000, 750, 15);

	/* 0% duty must leave the pin low and quiet */
	err = pwm_set(pwm, 0, period_ns, 0, 0);
	TEST_ASSERT(err == 0, "pwm_set failed: %d", err);
	k_msleep(5);
	n_edges = 0;
	k_msleep(10);
	TEST_ASSERT(n_edges == 0, "Pin toggled with 0%% duty (%u edges)", n_edges);
	TEST_ASSERT(!nrf_gpio_get_pin_level(DRV_PWM_PORT, DRV_PWM_PIN), "Pin should be low");

	TEST_PASS_AND_EXIT("PWM driver test passed");
}

static void timer_handler(nrf_timer_event_t event_type, void *p_context)
{
	ARG_UNUSED(event_type);
	ARG_UNUSED(p_context);
}

static void pwm_nrfx_gppi_test(void)
{
	static nrfx_pwm_t pwm = NRFX_PWM_INSTANCE(RAW_PWM_REG);
	static nrfx_timer_t timer = NRFX_TIMER_INSTANCE(RAW_TIMER_REG);
	/* 1 MHz clock, 1000 counts => 1 ms period.
	 * 25% duty (bit 15 set: starts high, falls at compare)
	 */
	static nrf_pwm_values_common_t values[] = {0x8000 | 250};
	static const nrf_pwm_sequence_t seq = {
		.values.p_common = values,
		.length = ARRAY_SIZE(values),
		.repeats = 0,
		.end_delay = 0,
	};
	const uint16_t n_periods = 10;
	nrfx_gppi_handle_t handle;
	int err;
#if HAS_PWM_DMA
	static nrfx_timer_t dma_timer = NRFX_TIMER_INSTANCE(DMA_TIMER_REG);
	nrfx_gppi_handle_t dma_handle;
#endif

	nrfx_pwm_config_t pwm_cfg = NRFX_PWM_DEFAULT_CONFIG(
		NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED,
		NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

	pwm_cfg.output_pins[0] = NRF_GPIO_PIN_MAP(RAW_PWM_PORT, RAW_PWM_PIN);
	pwm_cfg.base_clock = NRF_PWM_CLK_1MHz;
	pwm_cfg.top_value = 1000;

	nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG(1000000);

	timer_cfg.mode = NRF_TIMER_MODE_COUNTER;

	watch_pin(RAW_PWM_PORT, RAW_PWM_PIN);

	/* No handler: the driver then works without interrupts */
	err = nrfx_pwm_init(&pwm, &pwm_cfg, NULL, NULL);
	TEST_ASSERT(err == 0, "nrfx_pwm_init failed: %d", err);

	err = nrfx_timer_init(&timer, &timer_cfg, timer_handler);
	TEST_ASSERT(err == 0, "nrfx_timer_init failed: %d", err);
	nrfx_timer_clear(&timer);
	nrfx_timer_enable(&timer);

	uint32_t eep = nrfx_pwm_event_address_get(&pwm, NRF_PWM_EVENT_PWMPERIODEND);
	uint32_t tep = nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT);

	err = nrfx_gppi_conn_alloc(eep, tep, &handle);
	TEST_ASSERT(err >= 0, "nrfx_gppi_conn_alloc failed: %d", err);
	nrfx_gppi_conn_enable(handle);

#if HAS_PWM_DMA
	/* Count the DMA.SEQ[0].END events: each sequence fetch by the PWM DMA generates one */
	err = nrfx_timer_init(&dma_timer, &timer_cfg, timer_handler);
	TEST_ASSERT(err == 0, "nrfx_timer_init (DMA) failed: %d", err);
	nrfx_timer_clear(&dma_timer);
	nrfx_timer_enable(&dma_timer);

	uint32_t dma_eep = (uint32_t)&RAW_PWM_REG->EVENTS_DMA.SEQ[0].END;
	uint32_t dma_tep = nrfx_timer_task_address_get(&dma_timer, NRF_TIMER_TASK_COUNT);

	err = nrfx_gppi_conn_alloc(dma_eep, dma_tep, &dma_handle);
	TEST_ASSERT(err >= 0, "nrfx_gppi_conn_alloc (DMA) failed: %d", err);
	nrfx_gppi_conn_enable(dma_handle);
#endif

	/* Each of the n_periods sequence plays consists of one PWM period */
	(void)nrfx_pwm_simple_playback(&pwm, &seq, n_periods, NRFX_PWM_FLAG_STOP);

	k_msleep(n_periods + 10);

	TEST_ASSERT(nrfx_pwm_stopped_check(&pwm), "PWM should have stopped after the playback");

	/* The PPI connection must have counted exactly one event per PWM period */
	uint32_t count = nrfx_timer_capture(&timer, NRF_TIMER_CC_CHANNEL0);

	TEST_ASSERT(count == n_periods, "PWMPERIODEND routed %u times, expected %u", count,
		    n_periods);

	check_waveform(1000, 250, n_periods);

#if HAS_PWM_DMA
	/* simple_playback plays the sequence as SEQ0 and SEQ1 alternately: 5 loops of both */
	uint32_t dma_count = nrfx_timer_capture(&dma_timer, NRF_TIMER_CC_CHANNEL0);

	TEST_ASSERT(dma_count == n_periods / 2,
		    "DMA.SEQ[0].END routed %u times, expected %u", dma_count, n_periods / 2);

	/* The DMA must have read the whole sequence buffer (bytes) */
	TEST_ASSERT(RAW_PWM_REG->DMA.SEQ[0].MAXCNT == sizeof(values),
		    "DMA.SEQ[0].MAXCNT is %u", RAW_PWM_REG->DMA.SEQ[0].MAXCNT);
	TEST_ASSERT(RAW_PWM_REG->DMA.SEQ[0].AMOUNT == sizeof(values),
		    "DMA.SEQ[0].AMOUNT is %u", RAW_PWM_REG->DMA.SEQ[0].AMOUNT);
	TEST_ASSERT(RAW_PWM_REG->DMA.SEQ[0].PTR == (uint32_t)(uintptr_t)values,
		    "DMA.SEQ[0].PTR does not point to the sequence");

	nrfx_gppi_conn_disable(dma_handle);
	nrfx_gppi_conn_free(dma_eep, dma_tep, dma_handle);
	nrfx_timer_uninit(&dma_timer);
#endif

	nrfx_gppi_conn_disable(handle);
	nrfx_gppi_conn_free(eep, tep, handle);
	nrfx_timer_uninit(&timer);
	nrfx_pwm_uninit(&pwm);

	TEST_PASS_AND_EXIT("PWM nrfx with GPPI test passed");
}

static const struct bst_test_instance pwm_tests[] = {
	{
		.test_id = "pwm_driver",
		.test_descr = "Test the Zephyr PWM driver API",
		.test_main_f = pwm_driver_test
	},
	{
		.test_id = "pwm_nrfx_gppi",
		.test_descr = "Test nrfx_pwm directly, with GPPI",
		.test_main_f = pwm_nrfx_gppi_test
	},

	BSTEST_END_MARKER
};

struct bst_test_list *test_pwm_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, pwm_tests);
}

bst_test_install_t test_installers[] = {
	test_pwm_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
