/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>

LOG_MODULE_REGISTER(sensor_clock, CONFIG_SENSOR_LOG_LEVEL);

static const struct device *external_sensor_clock = DEVICE_DT_GET(DT_CHOSEN(zephyr_sensor_clock));
static uint32_t freq;

static int external_sensor_clock_init(void)
{
	int rc;

	rc = counter_start(external_sensor_clock);
	if (rc != 0) {
		LOG_ERR("Failed to start sensor clock counter: %d\n", rc);
		return rc;
	}

	freq = counter_get_frequency(external_sensor_clock);
	if (freq == 0) {
		LOG_ERR("Sensor clock %s has no fixed frequency\n", external_sensor_clock->name);
		return -EINVAL;
	}

	return 0;
}

int sensor_clock_get_cycles(uint64_t *cycles)
{
	__ASSERT_NO_MSG(counter_is_counting_up(external_sensor_clock));

	int rc;
#ifdef COUNTER_64BITS_TICKS
	const struct counter_driver_api *api =
		(const struct counter_driver_api *)external_sensor_clock->api;

	if (api->get_value_64) {
		rc = counter_get_value_64(external_sensor_clock, cycles);
	} else
#endif /* COUNTER_64BITS_TICKS */
	{
		uint32_t result_32;

		rc = counter_get_value(external_sensor_clock, &result_32);
		*cycles = (uint64_t)result_32;
	}

	return rc;
}

/*
 * Convert external clock cycles to nanoseconds.
 *
 * Unlike for sys clocks, which have k_cyc_to_ns_near64(), there isn't a similar conversion macro
 * for external clocks. In practice this will always choose the slower conversion path since clock
 * frequency (f) is not known at compile time. The conversion is done in a way that doesn't overflow
 * the value.
 *
 * @param t Time in cycles
 * @param f Clock frequency
 * @return Time in nanoseconds rounded to nearest
 */
#define sensor_clk_ext_cyc_to_ns_near64(t, f) z_tmcvt_64(t, f, Z_HZ_ns, false, false, true)

uint64_t sensor_clock_cycles_to_ns(uint64_t cycles)
{
	return sensor_clk_ext_cyc_to_ns_near64(cycles, freq);
}

SYS_INIT(external_sensor_clock_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
