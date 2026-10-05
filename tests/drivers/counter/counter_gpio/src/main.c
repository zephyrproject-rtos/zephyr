/*
 * SPDX-FileCopyrightText: Copyright RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define PPS_NODE DT_NODELABEL(pps)

static const struct device *const counter_dev = DEVICE_DT_GET(PPS_NODE);
static const struct gpio_dt_spec line0 = GPIO_DT_SPEC_GET_BY_IDX(PPS_NODE, gpios, 0);
static const struct gpio_dt_spec line1 = GPIO_DT_SPEC_GET_BY_IDX(PPS_NODE, gpios, 1);

struct capture_record {
	uint32_t ticks;
	uint32_t count;
	uint8_t chan_id;
	counter_capture_flags_t flags;
};

static struct capture_record record[2];

static void capture_cb(const struct device *dev, uint8_t chan_id, counter_capture_flags_t flags,
		       uint32_t ticks, void *user_data)
{
	struct capture_record *rec = user_data;

	ARG_UNUSED(dev);

	rec->ticks = ticks;
	rec->chan_id = chan_id;
	rec->flags = flags;
	rec->count++;
}

static void pulse(const struct gpio_dt_spec *line)
{
	zassert_ok(gpio_emul_input_set(line->port, line->pin, 1));
	k_msleep(1);
	zassert_ok(gpio_emul_input_set(line->port, line->pin, 0));
	k_msleep(1);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(record, 0, sizeof(record));

	zassert_ok(gpio_emul_input_set(line0.port, line0.pin, 0));
	zassert_ok(gpio_emul_input_set(line1.port, line1.pin, 0));

	(void)counter_disable_capture(counter_dev, 0);
	(void)counter_disable_capture(counter_dev, 1);
}

ZTEST(counter_gpio, test_device_is_ready)
{
	zassert_true(device_is_ready(counter_dev));
	zassert_equal(counter_get_num_of_channels(counter_dev), 3);
	zassert_equal(counter_get_frequency(counter_dev), CONFIG_SYS_CLOCK_TICKS_PER_SEC);
}

ZTEST(counter_gpio, test_value_follows_uptime)
{
	uint32_t before_ticks;
	uint32_t after_ticks;

	zassert_ok(counter_get_value(counter_dev, &before_ticks));
	k_msleep(20);
	zassert_ok(counter_get_value(counter_dev, &after_ticks));

	zassert_true(after_ticks != before_ticks, "the counter did not advance");
}

ZTEST(counter_gpio, test_capture_on_rising_edge)
{
	uint32_t now;

	zassert_ok(counter_capture_configure(counter_dev, 0, COUNTER_CAPTURE_RISING_EDGE,
					     capture_cb, &record[0]));
	zassert_ok(counter_enable_capture(counter_dev, 0));

	pulse(&line0);

	zassert_equal(record[0].count, 1, "expected one capture, got %u", record[0].count);
	zassert_equal(record[0].chan_id, 0);
	zassert_equal(record[0].flags, COUNTER_CAPTURE_RISING_EDGE);

	zassert_ok(counter_get_value(counter_dev, &now));
	zassert_true(record[0].ticks != 0, "the capture carried no timestamp");
}

ZTEST(counter_gpio, test_capture_ignores_other_edge)
{
	zassert_ok(counter_capture_configure(counter_dev, 0, COUNTER_CAPTURE_FALLING_EDGE,
					     capture_cb, &record[0]));
	zassert_ok(counter_enable_capture(counter_dev, 0));

	pulse(&line0);

	/* One falling edge in the pulse, so one capture and not two. */
	zassert_equal(record[0].count, 1, "expected one capture, got %u", record[0].count);
}

ZTEST(counter_gpio, test_capture_on_both_edges)
{
	zassert_ok(counter_capture_configure(counter_dev, 0, COUNTER_CAPTURE_BOTH_EDGES, capture_cb,
					     &record[0]));
	zassert_ok(counter_enable_capture(counter_dev, 0));

	pulse(&line0);

	zassert_equal(record[0].count, 2, "expected two captures, got %u", record[0].count);
}

ZTEST(counter_gpio, test_timestamps_advance)
{
	uint32_t first;

	zassert_ok(counter_capture_configure(counter_dev, 0, COUNTER_CAPTURE_RISING_EDGE,
					     capture_cb, &record[0]));
	zassert_ok(counter_enable_capture(counter_dev, 0));

	pulse(&line0);
	first = record[0].ticks;

	k_msleep(20);
	pulse(&line0);

	zassert_equal(record[0].count, 2);
	zassert_true(record[0].ticks > first, "the second capture did not come later");
}

ZTEST(counter_gpio, test_single_shot_stops_after_one)
{
	zassert_ok(counter_capture_configure(
		counter_dev, 0, COUNTER_CAPTURE_RISING_EDGE | COUNTER_CAPTURE_SINGLE_SHOT,
		capture_cb, &record[0]));
	zassert_ok(counter_enable_capture(counter_dev, 0));

	pulse(&line0);
	pulse(&line0);

	zassert_equal(record[0].count, 1, "expected one capture, got %u", record[0].count);
}

ZTEST(counter_gpio, test_channels_are_independent)
{
	zassert_ok(counter_capture_configure(counter_dev, 0, COUNTER_CAPTURE_RISING_EDGE,
					     capture_cb, &record[0]));
	zassert_ok(counter_capture_configure(counter_dev, 1, COUNTER_CAPTURE_RISING_EDGE,
					     capture_cb, &record[1]));
	zassert_ok(counter_enable_capture(counter_dev, 0));
	zassert_ok(counter_enable_capture(counter_dev, 1));

	pulse(&line1);

	zassert_equal(record[0].count, 0, "channel 0 fired on channel 1 line");
	zassert_equal(record[1].count, 1);
	zassert_equal(record[1].chan_id, 1);
}

ZTEST(counter_gpio, test_disable_stops_capture)
{
	zassert_ok(counter_capture_configure(counter_dev, 0, COUNTER_CAPTURE_RISING_EDGE,
					     capture_cb, &record[0]));
	zassert_ok(counter_enable_capture(counter_dev, 0));
	pulse(&line0);
	zassert_equal(record[0].count, 1);

	zassert_ok(counter_disable_capture(counter_dev, 0));
	pulse(&line0);
	zassert_equal(record[0].count, 1, "a capture arrived after disable");
}

ZTEST(counter_gpio, test_configure_rejects_no_edge)
{
	zassert_equal(counter_capture_configure(counter_dev, 0, 0, capture_cb, &record[0]),
		      -EINVAL);
}

ZTEST(counter_gpio, test_enable_rejects_unconfigured)
{
	/* Channel 2 is left alone by every other test. */
	zassert_equal(counter_enable_capture(counter_dev, 2), -EINVAL);
}

ZTEST(counter_gpio, test_rejects_channel_out_of_range)
{
	zassert_equal(counter_capture_configure(counter_dev, 3, COUNTER_CAPTURE_RISING_EDGE,
						capture_cb, &record[0]),
		      -ENOTSUP);
}

ZTEST_SUITE(counter_gpio, NULL, NULL, before, NULL, NULL);
