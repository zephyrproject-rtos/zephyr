/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/realtime.h>
#include <zephyr/sys/internal/realtime.h>

static int64_t realtime_reference_ms;
static struct k_spinlock realtime_lock;

static void set_realtime_reference(const int64_t *reference_ms)
{
	K_SPINLOCK(&realtime_lock) {
		realtime_reference_ms = *reference_ms;
	}
}

static void get_realtime_reference(int64_t *reference_ms)
{
	K_SPINLOCK(&realtime_lock) {
		*reference_ms = realtime_reference_ms;
	}
}

int z_impl_sys_realtime_get_timestamp(int64_t *timestamp)
{
	get_realtime_reference(timestamp);

	/* Get uptime just before returning to minimize latency */
	*timestamp += k_uptime_get();
	return 0;
}

int z_impl_sys_realtime_set_timestamp(const int64_t *timestamp)
{
	int64_t reference_ms;

	/* Get uptime immediately to minimize latency */
	reference_ms = -k_uptime_get();

	if (!sys_realtime_validate_timestamp(timestamp)) {
		return -EINVAL;
	}

	reference_ms += *timestamp;
	set_realtime_reference(&reference_ms);
	return 0;
}

int z_impl_sys_realtime_get_datetime(struct sys_datetime *datetime)
{
	int64_t timestamp;
	int ret;

	ret = sys_realtime_get_timestamp(&timestamp);
	if (ret < 0) {
		return ret;
	}

	return sys_realtime_timestamp_to_datetime(datetime, &timestamp);
}

int z_impl_sys_realtime_set_datetime(const struct sys_datetime *datetime)
{
	int ret;
	int64_t timestamp_ms;

	ret = sys_realtime_datetime_to_timestamp(&timestamp_ms, datetime);
	if (ret < 0) {
		return ret;
	}

	return sys_realtime_set_timestamp(&timestamp_ms);
}
