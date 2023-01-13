/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry_intel.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/mm/system_mm.h>

#if CONFIG_MM_DRV
#define PAGE_SIZE CONFIG_MM_DRV_PAGE_SIZE
#else
#define PAGE_SIZE 0x1000
#endif

__aligned(PAGE_SIZE)uint8_t telemetry_data_buffer[PAGE_SIZE];
struct byte_array shared_telemetry_buffer;

void telemetry_init(void)
{
	shared_telemetry_buffer.data =
		(__sparse_force uint32_t *)z_soc_cached_ptr(&telemetry_data_buffer);
	shared_telemetry_buffer.size = PAGE_SIZE;

	memset(shared_telemetry_buffer.data, 0, shared_telemetry_buffer.size);
	z_xtensa_cache_flush_inv(shared_telemetry_buffer.data, shared_telemetry_buffer.size);

	struct TelemetryWndData *telemetry_data =
		(struct TelemetryWndData *)(shared_telemetry_buffer.data);

	telemetry_data->separator_1 = 0x0000C0DE;
	telemetry_data->separator_2 = 0x0DEC0DE2;
	telemetry_data->separator_3 = 0x0DEC0DE3;
	telemetry_data->separator_4 = 0x0DEC0DE4;
	telemetry_data->separator_5 = 0x0DEC0DE5;
	telemetry_data->separator_6 = 0x0DEC0DE6;
	telemetry_data->separator_7 = 0x0DEC0DE7;
	telemetry_data->separator_8 = 0x0DEC0DE8;
	telemetry_data->separator_9 = 0x0DEC0DE9;
	telemetry_data->separator_10 = 0x0DEC0DEA;
	telemetry_data->separator_11 = 0x0DEC0DEB;

	z_xtensa_cache_flush_inv(shared_telemetry_buffer.data, shared_telemetry_buffer.size);
}

struct byte_array telemetry_get_buffer(void)
{
	return shared_telemetry_buffer;
}

struct TelemetryWndData *telemetry_get_data(void)
{
	struct TelemetryWndData *telemetry_data =
			(struct TelemetryWndData *)(shared_telemetry_buffer.data);
	return telemetry_data;
}
