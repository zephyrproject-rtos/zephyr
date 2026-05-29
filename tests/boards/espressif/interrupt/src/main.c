/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>

/* Test sets the three timer units, each with different interrupt priority level.
 * Each timer/counter is set to respond to three separate alarms.
 * The alarm times are set so each would trigger one after another separated by
 * the `ALARM_DIFF_US` time.
 * Each ISR sets its corresponding token into its token slots and wait for the
 * subsequent alarm ISR to be called.
 * After the last alarm ISR finishes the main loop checks if the ISR were called.
 */

#define TOKEN_A 0xBEEFBABE
#define TOKEN_B 0xC0FEB00B
#define TOKEN_C 0xDEAD5AA5

#define ALARM_DIFF_US 1000
#define ALARM_A_US    (1000 + ALARM_DIFF_US)
#define ALARM_B_US    (ALARM_A_US + ALARM_DIFF_US)
#define ALARM_C_US    (ALARM_B_US + ALARM_DIFF_US)
#define ISR_DELAY_US  (ALARM_DIFF_US * 3)
#define CYCLE_MS      10
#define TEST_CYCLES   1000

static const struct device *const timer0 = DEVICE_DT_GET(DT_INST(0, espressif_esp32_counter));
static const struct device *const timer1 = DEVICE_DT_GET(DT_INST(1, espressif_esp32_counter));
static const struct device *const timer2 = DEVICE_DT_GET(DT_INST(2, espressif_esp32_counter));

static struct counter_alarm_cfg alarm_a;
static struct counter_alarm_cfg alarm_b;
static struct counter_alarm_cfg alarm_c;

static int token[3];
static int success_cnt;
static int error_cnt;

static void alarm_handler_c(const struct device *dev, uint8_t chan_id, uint32_t counter,
			    void *user_data)
{
	if (dev == timer2 && token[0] == TOKEN_A && token[1] == TOKEN_B && token[2] == 0) {
		token[2] = TOKEN_C;
	}

	k_busy_wait(ISR_DELAY_US);
}

static void alarm_handler_b(const struct device *dev, uint8_t chan_id, uint32_t counter,
			    void *user_data)
{
	if (dev == timer1 && token[0] == TOKEN_A && token[1] == 0 && token[2] == 0) {
		token[1] = TOKEN_B;
	}

	k_busy_wait(ISR_DELAY_US);
}

static void alarm_handler_a(const struct device *dev, uint8_t chan_id, uint32_t counter,
			    void *user_data)
{
	if (dev == timer0 && token[0] == 0 && token[1] == 0 && token[2] == 0) {
		token[0] = TOKEN_A;
	}

	k_busy_wait(ISR_DELAY_US);
}

static void *esp_interrupt_suite_setup(void)
{
	zassert_true(device_is_ready(timer0), "Device %s not ready", timer0->name);
	zassert_true(device_is_ready(timer1), "Device %s not ready", timer1->name);
	zassert_true(device_is_ready(timer2), "Device %s not ready", timer2->name);

	zassert_false(counter_start(timer0), "Timer 0 failed to start");
	zassert_false(counter_start(timer1), "Timer 1 failed to start");
	zassert_false(counter_start(timer2), "Timer 2 failed to start");

	return NULL;
}


ZTEST(esp_interrupt, test_nested_isr)
{
	uint32_t cnt = TEST_CYCLES;

	success_cnt = 0;
	error_cnt = 0;

	while (cnt--) {
		alarm_a.ticks = counter_us_to_ticks(timer0, ALARM_A_US);
		alarm_a.callback = alarm_handler_a;

		alarm_b.ticks = counter_us_to_ticks(timer1, ALARM_B_US);
		alarm_b.callback = alarm_handler_b;

		alarm_c.ticks = counter_us_to_ticks(timer2, ALARM_C_US);
		alarm_c.callback = alarm_handler_c;

		counter_reset(timer0);
		counter_reset(timer1);
		counter_reset(timer2);

		zassert_false(counter_set_channel_alarm(timer0, 0, &alarm_a),
			      "Failed to set alarm A");
		zassert_false(counter_set_channel_alarm(timer1, 0, &alarm_b),
			      "Failed to set alarm B");
		zassert_false(counter_set_channel_alarm(timer2, 0, &alarm_c),
			      "Failed to set alarm C");

		k_msleep(CYCLE_MS);

		if (token[0] == TOKEN_A && token[1] == TOKEN_B && token[2] == TOKEN_C) {
			success_cnt++;
		} else {
			error_cnt++;
		}

		token[0] = 0;
		token[1] = 0;
		token[2] = 0;
	}

	zassert_true(error_cnt == 0, "Errors occurred (%d)", error_cnt);
	zassert_true(success_cnt == TEST_CYCLES, "Not all test cycles pssed (%d from %d)",
		     success_cnt, TEST_CYCLES);
}

ZTEST_SUITE(esp_interrupt, NULL, esp_interrupt_suite_setup, NULL, NULL, NULL);
