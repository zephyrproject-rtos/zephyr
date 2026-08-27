/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * esp_intr_noniram_disable() masks every CPU line not marked
 * ESP_INTR_FLAG_IRAM in the devicetree, and esp_intr_noniram_enable() unmasks
 * it. A timer group alarm is level-triggered and stays pending until its ISR
 * clears it, so an alarm raised during the pause fires once on resume. Nothing
 * is asserted while paused: a failed assertion would leave the pause active.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#define COUNTER_NODE DT_INST(0, espressif_esp32_counter)

#if DT_NODE_HAS_STATUS_OKAY(COUNTER_NODE)

#define COUNTER_TIMER DT_PARENT(COUNTER_NODE)
#define COUNTER_IRQN  DT_IRQN(COUNTER_TIMER)
#define ALARM_US      100U
#define WAIT_US       2000U

BUILD_ASSERT((DT_IRQ(COUNTER_TIMER, flags) & ESP_INTR_FLAG_IRAM) == 0,
	     "the test needs a counter that is paused during flash operations");

static const struct device *const counter = DEVICE_DT_GET(COUNTER_NODE);
static volatile uint32_t alarm_hits;

static void alarm_cb(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);

	alarm_hits++;
}

static int arm_alarm(void)
{
	struct counter_alarm_cfg cfg = {
		.callback = alarm_cb,
		.ticks = counter_us_to_ticks(counter, ALARM_US),
	};

	return counter_set_channel_alarm(counter, 0, &cfg);
}

static void esp_noniram_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_true(device_is_ready(counter), "Device %s not ready", counter->name);
	(void)counter_start(counter);
	irq_enable(COUNTER_IRQN);
	alarm_hits = 0U;
}

static void esp_noniram_after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)counter_cancel_channel_alarm(counter, 0);
	irq_enable(COUNTER_IRQN);
}

ZTEST(esp_noniram, test_paused_source_fires_on_resume)
{
	uint32_t hits_paused;
	int enabled_paused;
	int err;

	esp_intr_noniram_disable();
	err = arm_alarm();
	k_busy_wait(WAIT_US);
	hits_paused = alarm_hits;
	enabled_paused = irq_is_enabled(COUNTER_IRQN);
	esp_intr_noniram_enable();
	k_busy_wait(WAIT_US);

	zassert_equal(err, 0, "alarm not set: %d", err);
	zassert_equal(hits_paused, 0U, "the alarm fired while paused");
	zassert_not_equal(enabled_paused, 0, "a paused IRQ must read as enabled");
	zassert_equal(alarm_hits, 1U, "the alarm must fire once on resume");
}

ZTEST(esp_noniram, test_disable_while_paused_holds_on_resume)
{
	uint32_t hits_disabled;
	int enabled_disabled;
	int err;

	esp_intr_noniram_disable();
	irq_disable(COUNTER_IRQN);
	esp_intr_noniram_enable();
	err = arm_alarm();
	k_busy_wait(WAIT_US);
	hits_disabled = alarm_hits;
	enabled_disabled = irq_is_enabled(COUNTER_IRQN);
	irq_enable(COUNTER_IRQN);
	k_busy_wait(WAIT_US);

	zassert_equal(err, 0, "alarm not set: %d", err);
	zassert_equal(hits_disabled, 0U, "the resume re-enabled a disabled IRQ");
	zassert_equal(enabled_disabled, 0, "a disabled IRQ must read as disabled");
	zassert_equal(alarm_hits, 1U, "the alarm must fire once on irq_enable()");
}

ZTEST(esp_noniram, test_enable_while_paused_waits_for_resume)
{
	uint32_t hits_paused;
	int err;

	irq_disable(COUNTER_IRQN);
	esp_intr_noniram_disable();
	irq_enable(COUNTER_IRQN);
	err = arm_alarm();
	k_busy_wait(WAIT_US);
	hits_paused = alarm_hits;
	esp_intr_noniram_enable();
	k_busy_wait(WAIT_US);

	zassert_equal(err, 0, "alarm not set: %d", err);
	zassert_equal(hits_paused, 0U, "irq_enable() bypassed the pause");
	zassert_equal(alarm_hits, 1U, "the alarm must fire once on resume");
}

ZTEST_SUITE(esp_noniram, NULL, NULL, esp_noniram_before, esp_noniram_after, NULL);

#endif /* counter okay */
