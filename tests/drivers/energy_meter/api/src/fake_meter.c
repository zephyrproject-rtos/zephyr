/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/energy_meter.h>

#include "fake_meter.h"

#define PHASE_QUANTITIES                                                                           \
	(BIT(ENERGY_METER_VOLTAGE) | BIT(ENERGY_METER_CURRENT) | BIT(ENERGY_METER_ACTIVE_POWER) |  \
	 BIT(ENERGY_METER_POWER_FACTOR))

#define TOTAL_QUANTITIES                                                                           \
	(BIT(ENERGY_METER_ACTIVE_POWER) | BIT(ENERGY_METER_FREQUENCY) |                            \
	 BIT(ENERGY_METER_ACTIVE_ENERGY_IMPORT) | BIT(ENERGY_METER_ACTIVE_ENERGY_EXPORT))

static const uint32_t fake_quantities[ENERGY_METER_LINE_COUNT] = {
	[ENERGY_METER_LINE_A] = PHASE_QUANTITIES,
	[ENERGY_METER_LINE_B] = PHASE_QUANTITIES,
	[ENERGY_METER_LINE_C] = PHASE_QUANTITIES,
	[ENERGY_METER_LINE_N] = BIT(ENERGY_METER_CURRENT),
	[ENERGY_METER_LINE_TOTAL] = TOTAL_QUANTITIES,
};

static struct {
	int64_t energy;
	energy_meter_event_cb_t cb;
	void *user_data;
} fake_state;

int64_t fake_meter_value(uint8_t quantity, uint8_t line)
{
	switch (quantity) {
	case ENERGY_METER_VOLTAGE:
		return 230000000LL + line * 1000000LL;
	case ENERGY_METER_CURRENT:
		return 5000000LL + line * 250000LL;
	case ENERGY_METER_ACTIVE_POWER:
		return (line == ENERGY_METER_LINE_TOTAL) ? 3450000000LL : -1150000000LL;
	case ENERGY_METER_POWER_FACTOR:
		return 985000;
	case ENERGY_METER_FREQUENCY:
		return 50010000;
	case ENERGY_METER_ACTIVE_ENERGY_IMPORT:
	case ENERGY_METER_ACTIVE_ENERGY_EXPORT:
		return fake_state.energy;
	case FAKE_METER_STATUS:
		return FAKE_METER_STATUS_VALUE;
	default:
		return 0;
	}
}

void fake_meter_add_energy(int64_t uwh)
{
	fake_state.energy += uwh;
}

void fake_meter_raise(uint8_t type, uint8_t index)
{
	const struct energy_meter_event event = {.type = type, .index = index};

	if (fake_state.cb != NULL) {
		fake_state.cb(fake_meter, &event, fake_state.user_data);
	}
}

static bool fake_supported(const struct energy_meter_channel *channel)
{
	if (channel->line >= ENERGY_METER_LINE_COUNT) {
		return false;
	}

	if (channel->quantity == FAKE_METER_STATUS) {
		return channel->line == ENERGY_METER_LINE_TOTAL;
	}

	return (channel->quantity < ENERGY_METER_QUANTITY_COMMON_COUNT) &&
	       ((fake_quantities[channel->line] & BIT(channel->quantity)) != 0U);
}

static int fake_get_info(const struct device *dev, struct energy_meter_info *info)
{
	ARG_UNUSED(dev);

	info->phases = 3;
	for (size_t i = 0; i < ENERGY_METER_LINE_COUNT; i++) {
		info->quantities[i] = fake_quantities[i];
	}

	return 0;
}

static int fake_get(const struct device *dev, const struct energy_meter_channel *channels,
		    int64_t *values, size_t count)
{
	ARG_UNUSED(dev);

	for (size_t i = 0; i < count; i++) {
		if (!fake_supported(&channels[i])) {
			return -ENOTSUP;
		}
	}

	for (size_t i = 0; i < count; i++) {
		values[i] = fake_meter_value(channels[i].quantity, channels[i].line);
	}

	return 0;
}

static int fake_reset_energy(const struct device *dev)
{
	ARG_UNUSED(dev);

	fake_state.energy = 0;

	return 0;
}

static int fake_set_event_callback(const struct device *dev, energy_meter_event_cb_t cb,
				   void *user_data)
{
	ARG_UNUSED(dev);

	fake_state.cb = cb;
	fake_state.user_data = user_data;

	return 0;
}

static DEVICE_API(energy_meter, fake_api) = {
	.get_info = fake_get_info,
	.get = fake_get,
	.reset_energy = fake_reset_energy,
	.set_event_callback = fake_set_event_callback,
};

static int fake_faulty_get_info(const struct device *dev, struct energy_meter_info *info)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(info);

	return -EIO;
}

static int fake_faulty_get(const struct device *dev, const struct energy_meter_channel *channels,
			   int64_t *values, size_t count)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channels);
	ARG_UNUSED(values);
	ARG_UNUSED(count);

	return -EIO;
}

/* Fails every access without touching the output buffers */
static DEVICE_API(energy_meter, fake_faulty_api) = {
	.get_info = fake_faulty_get_info,
	.get = fake_faulty_get,
};

/* Implements only the mandatory operations */
static DEVICE_API(energy_meter, fake_minimal_api) = {
	.get_info = fake_get_info,
	.get = fake_get,
};

DEVICE_DEFINE(fake_meter, "fake_meter", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_ENERGY_METER_INIT_PRIORITY, &fake_api);

DEVICE_DEFINE(fake_meter_minimal, "fake_meter_minimal", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_ENERGY_METER_INIT_PRIORITY, &fake_minimal_api);

DEVICE_DEFINE(fake_meter_faulty, "fake_meter_faulty", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_ENERGY_METER_INIT_PRIORITY, &fake_faulty_api);

const struct device *const fake_meter = DEVICE_GET(fake_meter);
const struct device *const fake_meter_minimal = DEVICE_GET(fake_meter_minimal);
const struct device *const fake_meter_faulty = DEVICE_GET(fake_meter_faulty);
