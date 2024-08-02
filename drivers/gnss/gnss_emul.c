/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* Required for gmtime_r */

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <string.h>
#include <time.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gnss_emul, CONFIG_GNSS_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_gnss_emul

#define GNSS_EMUL_DEFAULT_FIX_INTERVAL_MS      1000
#define GNSS_EMUL_MIN_FIX_INTERVAL_MS          100
#define GNSS_EMUL_FIX_ACQUIRE_TIME_MS          5000
#define GNSS_EMUL_DEFAULT_NAV_MODE             GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS
#define GNSS_EMUL_SUPPORTED_SYSTEMS_MASK       0xFF
#define GNSS_EMUL_SUPPORTED_SYSTEMS_COUNT      8
#define GNSS_EMUL_DEFAULT_ENABLED_SYSTEMS_MASK GNSS_EMUL_SUPPORTED_SYSTEMS_MASK

struct gnss_emul_data {
	const struct device *dev;
	struct k_work_delayable data_dwork;
	struct k_sem lock;
	int64_t resume_timestamp_ms;
	int64_t fix_timestamp_ms;
	uint32_t fix_interval_ms;
	enum gnss_navigation_mode nav_mode;
	gnss_systems_t enabled_systems;
	struct gnss_data data;

#ifdef CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[GNSS_EMUL_SUPPORTED_SYSTEMS_COUNT];
	uint8_t satellites_len;
#endif
};

static void gnss_emul_lock_sem(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void gnss_emul_unlock_sem(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	k_sem_give(&data->lock);
}

static void gnss_emul_update_fix_timestamp(const struct device *dev, bool resuming)
{
	struct gnss_emul_data *data = dev->data;
	int64_t uptime_ms;

	uptime_ms = k_uptime_get();
	data->fix_timestamp_ms = ((uptime_ms / data->fix_interval_ms) + 1) * data->fix_interval_ms;

	if (resuming) {
		data->resume_timestamp_ms = data->fix_timestamp_ms;
	}
}

static bool gnss_emul_fix_is_acquired(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;
	int64_t time_since_resume;

	time_since_resume = data->fix_timestamp_ms - data->resume_timestamp_ms;
	return time_since_resume >= GNSS_EMUL_FIX_ACQUIRE_TIME_MS;
}

#ifdef CONFIG_PM_DEVICE
static void gnss_emul_clear_fix_timestamp(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	data->fix_timestamp_ms = 0;
}
#endif

static void gnss_emul_schedule_work(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	k_work_schedule(&data->data_dwork, K_TIMEOUT_ABS_MS(data->fix_timestamp_ms));
}

static bool gnss_emul_cancel_work(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;
	struct k_work_sync sync;

	return k_work_cancel_delayable_sync(&data->data_dwork, &sync);
}

static bool gnss_emul_is_resumed(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	return data->fix_timestamp_ms > 0;
}

static void gnss_emul_lock(const struct device *dev)
{
	gnss_emul_lock_sem(dev);
	gnss_emul_cancel_work(dev);
}

static void gnss_emul_unlock(const struct device *dev)
{
	if (gnss_emul_is_resumed(dev)) {
		gnss_emul_schedule_work(dev);
	}

	gnss_emul_unlock_sem(dev);
}

static int gnss_emul_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	struct gnss_emul_data *data = dev->data;

	if (fix_interval_ms < GNSS_EMUL_MIN_FIX_INTERVAL_MS) {
		return -EINVAL;
	}

	data->fix_interval_ms = fix_interval_ms;
	return 0;
}

static int gnss_emul_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	struct gnss_emul_data *data = dev->data;

	*fix_interval_ms = data->fix_interval_ms;
	return 0;
}

static int gnss_emul_set_navigation_mode(const struct device *dev,
					 enum gnss_navigation_mode mode)
{
	struct gnss_emul_data *data = dev->data;

	if (mode > GNSS_NAVIGATION_MODE_HIGH_DYNAMICS) {
		return -EINVAL;
	}

	data->nav_mode = mode;
	return 0;
}

static int gnss_emul_get_navigation_mode(const struct device *dev,
					 enum gnss_navigation_mode *mode)
{
	struct gnss_emul_data *data = dev->data;

	*mode = data->nav_mode;
	return 0;
}

static int gnss_emul_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	struct gnss_emul_data *data = dev->data;

	if (systems > GNSS_EMUL_SUPPORTED_SYSTEMS_MASK) {
		return -EINVAL;
	}

	data->enabled_systems = systems;
	return 0;
}

static int gnss_emul_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	struct gnss_emul_data *data = dev->data;

	*systems = data->enabled_systems;
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void gnss_emul_resume(const struct device *dev)
{
	gnss_emul_update_fix_timestamp(dev, true);
}

static void gnss_emul_suspend(const struct device *dev)
{
	gnss_emul_clear_fix_timestamp(dev);
}

static int gnss_emul_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	gnss_emul_lock(dev);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		gnss_emul_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		gnss_emul_resume(dev);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	gnss_emul_unlock(dev);
	return ret;
}
#endif

static int gnss_emul_api_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	int ret = -ENODEV;

	gnss_emul_lock(dev);

	if (!gnss_emul_is_resumed(dev)) {
		goto unlock_return;
	}

	ret = gnss_emul_set_fix_rate(dev, fix_interval_ms);

unlock_return:
	gnss_emul_unlock(dev);
	return ret;
}

static int gnss_emul_api_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	int ret = -ENODEV;

	gnss_emul_lock(dev);

	if (!gnss_emul_is_resumed(dev)) {
		goto unlock_return;
	}

	ret = gnss_emul_get_fix_rate(dev, fix_interval_ms);

unlock_return:
	gnss_emul_unlock(dev);
	return ret;
}

static int gnss_emul_api_set_navigation_mode(const struct device *dev,
					     enum gnss_navigation_mode mode)
{
	int ret = -ENODEV;

	gnss_emul_lock(dev);

	if (!gnss_emul_is_resumed(dev)) {
		goto unlock_return;
	}

	ret = gnss_emul_set_navigation_mode(dev, mode);

unlock_return:
	gnss_emul_unlock(dev);
	return ret;
}

static int gnss_emul_api_get_navigation_mode(const struct device *dev,
					 enum gnss_navigation_mode *mode)
{
	int ret = -ENODEV;

	gnss_emul_lock(dev);

	if (!gnss_emul_is_resumed(dev)) {
		goto unlock_return;
	}

	ret = gnss_emul_get_navigation_mode(dev, mode);

unlock_return:
	gnss_emul_unlock(dev);
	return ret;
}

static int gnss_emul_api_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	int ret = -ENODEV;

	gnss_emul_lock(dev);

	if (!gnss_emul_is_resumed(dev)) {
		goto unlock_return;
	}

	ret = gnss_emul_set_enabled_systems(dev, systems);

unlock_return:
	gnss_emul_unlock(dev);
	return ret;
}

static int gnss_emul_api_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	int ret = -ENODEV;

	gnss_emul_lock(dev);

	if (!gnss_emul_is_resumed(dev)) {
		goto unlock_return;
	}

	ret = gnss_emul_get_enabled_systems(dev, systems);

unlock_return:
	gnss_emul_unlock(dev);
	return ret;
}

static int gnss_emul_api_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	*systems = GNSS_EMUL_SUPPORTED_SYSTEMS_MASK;
	return 0;
}

static const struct gnss_driver_api api = {
	.set_fix_rate = gnss_emul_api_set_fix_rate,
	.get_fix_rate = gnss_emul_api_get_fix_rate,
	.set_navigation_mode = gnss_emul_api_set_navigation_mode,
	.get_navigation_mode = gnss_emul_api_get_navigation_mode,
	.set_enabled_systems = gnss_emul_api_set_enabled_systems,
	.get_enabled_systems = gnss_emul_api_get_enabled_systems,
	.get_supported_systems = gnss_emul_api_get_supported_systems,
};

static void gnss_emul_clear_data(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	memset(&data->data, 0, sizeof(data->data));
}

static void gnss_emul_set_fix(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	data->data.info.satellites_cnt = 8;
	data->data.info.hdop = 100;
	data->data.info.fix_status = GNSS_FIX_STATUS_GNSS_FIX;
	data->data.info.fix_quality = GNSS_FIX_QUALITY_GNSS_SPS;
}

static void gnss_emul_set_utc(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;
	time_t timestamp;
	struct tm datetime;
	uint16_t millisecond;

	timestamp = (time_t)(data->fix_timestamp_ms / 1000);
	gmtime_r(&timestamp, &datetime);

	millisecond = (uint16_t)(data->fix_timestamp_ms % 1000)
		    + (uint16_t)(datetime.tm_sec * 1000);

	data->data.utc.hour = datetime.tm_hour;
	data->data.utc.millisecond = millisecond;
	data->data.utc.minute = datetime.tm_min;
	data->data.utc.month = datetime.tm_mon + 1;
	data->data.utc.century_year = datetime.tm_year % 100;
}

static void gnss_emul_set_nav_data(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	data->data.nav_data.latitude = 10000000000;
	data->data.nav_data.longitude = -10000000000;
	data->data.nav_data.bearing = 3000;
	data->data.nav_data.speed = 0;
	data->data.nav_data.altitude = 20000;
}

#ifdef CONFIG_GNSS_SATELLITES
static void gnss_emul_clear_satellites(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	data->satellites_len = 0;
}

static bool gnss_emul_system_enabled(const struct device *dev, uint8_t system_bit)
{
	struct gnss_emul_data *data = dev->data;

	return BIT(system_bit) & data->enabled_systems;
}

static void gnss_emul_add_satellite(const struct device *dev, uint8_t system_bit)
{
	struct gnss_emul_data *data = dev->data;

	/* Unique values synthesized from GNSS system */
	data->satellites[data->satellites_len].prn = system_bit;
	data->satellites[data->satellites_len].snr = system_bit + 20;
	data->satellites[data->satellites_len].elevation = system_bit + 40;
	data->satellites[data->satellites_len].azimuth = system_bit + 60;
	data->satellites[data->satellites_len].system = BIT(system_bit);
	data->satellites[data->satellites_len].is_tracked = true;
	data->satellites_len++;
}

static void gnss_emul_set_satellites(const struct device *dev)
{
	gnss_emul_clear_satellites(dev);

	for (uint8_t i = 0; i < GNSS_EMUL_SUPPORTED_SYSTEMS_COUNT; i++) {
		if (!gnss_emul_system_enabled(dev, i)) {
			continue;
		}

		gnss_emul_add_satellite(dev, i);
	}
}
#endif

static void gnss_emul_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gnss_emul_data *data = CONTAINER_OF(dwork, struct gnss_emul_data, data_dwork);
	const struct device *dev = data->dev;

	if (!gnss_emul_fix_is_acquired(dev)) {
		gnss_emul_clear_data(dev);
	} else {
		gnss_emul_set_fix(dev);
		gnss_emul_set_utc(dev);
		gnss_emul_set_nav_data(dev);
	}

	gnss_publish_data(dev, &data->data);

#ifdef CONFIG_GNSS_SATELLITES
	gnss_emul_set_satellites(dev);
	gnss_publish_satellites(dev, data->satellites, data->satellites_len);
#endif

	gnss_emul_update_fix_timestamp(dev, false);
	gnss_emul_schedule_work(dev);
}

static void gnss_emul_init_data(const struct device *dev)
{
	struct gnss_emul_data *data = dev->data;

	data->dev = dev;
	k_sem_init(&data->lock, 1, 1);
	k_work_init_delayable(&data->data_dwork, gnss_emul_work_handler);
}

static int gnss_emul_init(const struct device *dev)
{
	gnss_emul_init_data(dev);

	if (pm_device_is_powered(dev)) {
		gnss_emul_update_fix_timestamp(dev, true);
		gnss_emul_schedule_work(dev);
	} else {
		pm_device_init_off(dev);
	}

	return pm_device_runtime_enable(dev);
}

#define GNSS_EMUL_NAME(inst, name) _CONCAT(name, inst)

#define GNSS_EMUL_DEVICE(inst)								\
	static struct gnss_emul_data GNSS_EMUL_NAME(inst, data) = {			\
		.fix_interval_ms = GNSS_EMUL_DEFAULT_FIX_INTERVAL_MS,			\
		.nav_mode = GNSS_EMUL_DEFAULT_NAV_MODE,					\
		.enabled_systems = GNSS_EMUL_DEFAULT_ENABLED_SYSTEMS_MASK,		\
	};										\
											\
	PM_DEVICE_DT_INST_DEFINE(inst, gnss_emul_pm_action);				\
											\
	DEVICE_DT_INST_DEFINE(								\
		inst,									\
		gnss_emul_init,								\
		PM_DEVICE_DT_INST_GET(inst),						\
		&GNSS_EMUL_NAME(inst, data),						\
		NULL,									\
		POST_KERNEL,								\
		CONFIG_GNSS_INIT_PRIORITY,						\
		&api									\
	);

DT_INST_FOREACH_STATUS_OKAY(GNSS_EMUL_DEVICE)
