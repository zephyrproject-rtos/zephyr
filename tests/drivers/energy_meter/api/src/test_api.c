/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/drivers/energy_meter.h>
#include <zephyr/ztest.h>

#include "fake_meter.h"

ZTEST_USER(energy_meter_api, test_get_info)
{
	struct energy_meter_info info;

	zassert_ok(energy_meter_get_info(fake_meter, &info));
	zassert_equal(info.phases, 3);
	zassert_true((info.quantities[ENERGY_METER_LINE_B] & BIT(ENERGY_METER_VOLTAGE)) != 0U);
	zassert_true((info.quantities[ENERGY_METER_LINE_N] & BIT(ENERGY_METER_VOLTAGE)) == 0U);
	zassert_true((info.quantities[ENERGY_METER_LINE_TOTAL] & BIT(ENERGY_METER_FREQUENCY)) !=
		     0U);
}

ZTEST_USER(energy_meter_api, test_get_info_failure_keeps_buffer)
{
	struct energy_meter_info info;
	uint8_t expected[sizeof(info)];

	memset(&info, 0xa5, sizeof(info));
	memset(expected, 0xa5, sizeof(expected));

	zassert_equal(energy_meter_get_info(fake_meter_faulty, &info), -EIO);
	zassert_mem_equal(&info, expected, sizeof(info), "buffer changed by a failed call");
}

ZTEST_USER(energy_meter_api, test_get_info_padding_cleared)
{
	struct energy_meter_info info;
	const uint8_t *bytes = (const uint8_t *)&info;

	/* Only the syscall handler clears the padding; drivers fill the fields */
	Z_TEST_SKIP_IFNDEF(CONFIG_USERSPACE);

	memset(&info, 0xa5, sizeof(info));

	zassert_ok(energy_meter_get_info(fake_meter, &info));

	for (size_t i = sizeof(info.phases); i < offsetof(struct energy_meter_info, quantities);
	     i++) {
		zassert_equal(bytes[i], 0, "padding byte %zu not cleared", i);
	}
}

ZTEST_USER(energy_meter_api, test_get_batch)
{
	static const struct energy_meter_channel channels[] = {
		{ENERGY_METER_VOLTAGE, ENERGY_METER_LINE_A},
		{ENERGY_METER_VOLTAGE, ENERGY_METER_LINE_C},
		{ENERGY_METER_CURRENT, ENERGY_METER_LINE_N},
		{ENERGY_METER_ACTIVE_POWER, ENERGY_METER_LINE_TOTAL},
		{ENERGY_METER_FREQUENCY, ENERGY_METER_LINE_TOTAL},
	};
	int64_t values[ARRAY_SIZE(channels)];

	zassert_ok(energy_meter_get(fake_meter, channels, values, ARRAY_SIZE(channels)));

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		zassert_equal(values[i], fake_meter_value(channels[i].quantity, channels[i].line),
			      "channel %zu", i);
	}
}

/* More channels than the syscall handler passes to the driver at once */
ZTEST_USER(energy_meter_api, test_get_large_batch)
{
	struct energy_meter_channel channels[2 * ENERGY_METER_QUANTITY_COMMON_COUNT + 3];
	int64_t values[ARRAY_SIZE(channels)];

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		channels[i].quantity = ENERGY_METER_VOLTAGE;
		channels[i].line = i % 3;
	}

	zassert_ok(energy_meter_get(fake_meter, channels, values, ARRAY_SIZE(channels)));

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		zassert_equal(values[i], fake_meter_value(ENERGY_METER_VOLTAGE, i % 3),
			      "channel %zu", i);
	}

	/* An unsupported channel in a later group still fails the call */
	channels[ARRAY_SIZE(channels) - 1].line = ENERGY_METER_LINE_N;
	zassert_equal(energy_meter_get(fake_meter, channels, values, ARRAY_SIZE(channels)),
		      -ENOTSUP);
}

ZTEST_USER(energy_meter_api, test_get_one_and_device_specific_quantity)
{
	int64_t value;

	zassert_ok(energy_meter_get_one(fake_meter, ENERGY_METER_ACTIVE_POWER, ENERGY_METER_LINE_B,
					&value));
	zassert_equal(value, -1150000000LL, "negative power must survive the API");

	zassert_ok(energy_meter_get_one(fake_meter, FAKE_METER_STATUS, ENERGY_METER_LINE_TOTAL,
					&value));
	zassert_equal(value, FAKE_METER_STATUS_VALUE);
}

ZTEST_USER(energy_meter_api, test_unsupported_channel)
{
	static const struct energy_meter_channel channels[] = {
		{ENERGY_METER_VOLTAGE, ENERGY_METER_LINE_A},
		{ENERGY_METER_VOLTAGE, ENERGY_METER_LINE_N},
	};
	int64_t values[ARRAY_SIZE(channels)];
	int64_t value;

	zassert_equal(energy_meter_get(fake_meter, channels, values, ARRAY_SIZE(channels)),
		      -ENOTSUP);
	zassert_equal(energy_meter_get_one(fake_meter, ENERGY_METER_TEMPERATURE,
					   ENERGY_METER_LINE_TOTAL, &value),
		      -ENOTSUP);
}

ZTEST_USER(energy_meter_api, test_reset_energy_syscall)
{
	int64_t value;

	zassert_ok(energy_meter_reset_energy(fake_meter));
	zassert_ok(energy_meter_get_one(fake_meter, ENERGY_METER_ACTIVE_ENERGY_IMPORT,
					ENERGY_METER_LINE_TOTAL, &value));
	zassert_equal(value, 0);

	zassert_equal(energy_meter_reset_energy(fake_meter_minimal), -ENOSYS);
}

/* Adding energy to the fake meter touches kernel memory, so this test runs in kernel mode */
ZTEST(energy_meter_api, test_reset_energy)
{
	int64_t value;

	fake_meter_add_energy(1500000);
	zassert_ok(energy_meter_get_one(fake_meter, ENERGY_METER_ACTIVE_ENERGY_IMPORT,
					ENERGY_METER_LINE_TOTAL, &value));
	zassert_true(value >= 1500000);

	zassert_ok(energy_meter_reset_energy(fake_meter));
	zassert_ok(energy_meter_get_one(fake_meter, ENERGY_METER_ACTIVE_ENERGY_IMPORT,
					ENERGY_METER_LINE_TOTAL, &value));
	zassert_equal(value, 0);
}

ZTEST(energy_meter_api, test_snapshot)
{
	struct energy_meter_snapshot snap;

	zassert_ok(energy_meter_get_snapshot(fake_meter, &snap));
	zassert_equal(snap.phases, 3);

	zassert_equal(snap.valid[ENERGY_METER_LINE_A],
		      BIT(ENERGY_METER_VOLTAGE) | BIT(ENERGY_METER_CURRENT) |
			      BIT(ENERGY_METER_ACTIVE_POWER) | BIT(ENERGY_METER_POWER_FACTOR));
	zassert_equal(snap.line[ENERGY_METER_LINE_B].voltage,
		      fake_meter_value(ENERGY_METER_VOLTAGE, ENERGY_METER_LINE_B));
	zassert_equal(snap.line[ENERGY_METER_LINE_C].power_factor, 985000);
	zassert_equal(snap.line[ENERGY_METER_LINE_N].current,
		      fake_meter_value(ENERGY_METER_CURRENT, ENERGY_METER_LINE_N));
	zassert_equal(snap.line[ENERGY_METER_LINE_TOTAL].active_power, 3450000000LL);
	zassert_equal(snap.line[ENERGY_METER_LINE_TOTAL].frequency, 50010000);

	/* Unsupported fields are zero and flagged invalid */
	zassert_equal(snap.line[ENERGY_METER_LINE_N].voltage, 0);
	zassert_equal(snap.valid[ENERGY_METER_LINE_N] & BIT(ENERGY_METER_VOLTAGE), 0);
	zassert_equal(snap.line[ENERGY_METER_LINE_TOTAL].temperature, 0);

	/* Named fields and the quantity index agree */
	zassert_equal(energy_meter_line_value(&snap.line[ENERGY_METER_LINE_TOTAL],
					      ENERGY_METER_FREQUENCY),
		      snap.line[ENERGY_METER_LINE_TOTAL].frequency);
}

struct event_record {
	const struct device *dev;
	struct energy_meter_event event;
	int calls;
};

static void event_cb(const struct device *dev, const struct energy_meter_event *event,
		     void *user_data)
{
	struct event_record *rec = user_data;

	rec->dev = dev;
	rec->event = *event;
	rec->calls++;
}

ZTEST(energy_meter_api, test_event_callback)
{
	struct event_record rec = {0};

	zassert_ok(energy_meter_set_event_callback(fake_meter, event_cb, &rec));
	fake_meter_raise(ENERGY_METER_EVENT_PULSE, 2);
	zassert_equal(rec.calls, 1);
	zassert_equal_ptr(rec.dev, fake_meter);
	zassert_equal(rec.event.type, ENERGY_METER_EVENT_PULSE);
	zassert_equal(rec.event.index, 2);

	zassert_ok(energy_meter_set_event_callback(fake_meter, NULL, NULL));
	fake_meter_raise(ENERGY_METER_EVENT_ALARM, 0);
	zassert_equal(rec.calls, 1);

	zassert_equal(energy_meter_set_event_callback(fake_meter_minimal, event_cb, &rec), -ENOSYS);
}

static void *energy_meter_api_setup(void)
{
	zassert_true(device_is_ready(fake_meter));

#ifdef CONFIG_USERSPACE
	k_object_access_all_grant(fake_meter);
	k_object_access_all_grant(fake_meter_minimal);
	k_object_access_all_grant(fake_meter_faulty);
#endif

	return NULL;
}

ZTEST_SUITE(energy_meter_api, NULL, energy_meter_api_setup, NULL, NULL, NULL);
