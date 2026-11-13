/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/energy_meter.h>

int energy_meter_get_snapshot(const struct device *dev, struct energy_meter_snapshot *snapshot)
{
	struct energy_meter_info info;
	int ret;

	memset(snapshot, 0, sizeof(*snapshot));

	ret = energy_meter_get_info(dev, &info);
	if (ret < 0) {
		return ret;
	}

	snapshot->phases = info.phases;

	for (uint8_t line = 0; line < ENERGY_METER_LINE_COUNT; line++) {
		struct energy_meter_channel channels[ENERGY_METER_QUANTITY_COMMON_COUNT];
		int64_t values[ENERGY_METER_QUANTITY_COMMON_COUNT];
		int64_t *fields = (int64_t *)&snapshot->line[line];
		size_t count = 0;

		for (uint8_t q = 0; q < ENERGY_METER_QUANTITY_COMMON_COUNT; q++) {
			if ((info.quantities[line] & BIT(q)) != 0U) {
				channels[count].quantity = q;
				channels[count].line = line;
				count++;
			}
		}

		if (count == 0U) {
			continue;
		}

		ret = energy_meter_get(dev, channels, values, count);
		if (ret < 0) {
			return ret;
		}

		for (size_t i = 0; i < count; i++) {
			fields[channels[i].quantity] = values[i];
		}

		snapshot->valid[line] = info.quantities[line];
	}

	return 0;
}
