/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include <memory_window/dynamic_slots.h>
#include <telemetry/telemetry_intel.h>
#include <zephyr/logging/log_backend_adsp_mtrace.h>

#define DEBUG_MEMORY_WINDOW_INIT_PRIORITY 38

int debug_memory_window_init(const struct device *d)
{
	ARG_UNUSED(d);
	int err = 0;

	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(mem_window2));

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	err = dynamic_slots_init(dev);
	if (err != 0) {
		return err;
	}

#if CONFIG_TELEMETRY_INTEL
	telemetry_init();
	err = dynamic_slots_map_slot(SLOT_TELEMETRY, dev, 0, telemetry_get_buffer().data);
	if (err < 0) {
		return err;
	}
#endif

#if CONFIG_LOG_BACKEND_ADSP_MTRACE
	log_backend_adsp_mtrace_init();
	uint32_t slot_index = dynamic_slots_map_slot(SLOT_DEBUG_LOG, dev, 0, 0);

	if (slot_index < 0) {
		err = slot_index;
		return err;
	}

	struct byte_array buffer;

	buffer.data = dynamic_slots_get_slot_address(dev, slot_index);
	buffer.size = MEMORY_WINDOW_SLOT_SIZE;
	log_backend_adsp_mtrace_set_buffer(buffer);
#endif /* CONFIG_LOG_BACKEND_ADSP_MTRACE */

	return 0;
}

SYS_INIT(debug_memory_window_init, EARLY, DEBUG_MEMORY_WINDOW_INIT_PRIORITY);
