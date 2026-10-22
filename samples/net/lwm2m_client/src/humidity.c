/*
 * Copyright (c) 2025 Clunky Machines
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_LWM2M_IPSO_HUMIDITY_SENSOR)

#define LOG_MODULE_NAME app_humidity
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "modules.h"

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/lwm2m_send_scheduler.h>
#include <zephyr/random/random.h>
#include <stdint.h>

static struct k_work_delayable humidity_work;
#define HUMIDITY_PERIOD K_MINUTES(2)

static struct lwm2m_time_series_elem humidity_cache[8];

static void humidity_work_cb(struct k_work *work)
{
	double v;

	v = 40.0 + (double)sys_rand32_get() / UINT32_MAX * 20.0;

	lwm2m_set_f64(&LWM2M_OBJ(3304, 0, 5700), v);

	k_work_schedule(&humidity_work, HUMIDITY_PERIOD);
}

void init_humidity_sensor(void)
{
	if (lwm2m_create_object_inst(&LWM2M_OBJ(3304, 0)) == 0) {
		if (IS_ENABLED(CONFIG_LWM2M_RESOURCE_DATA_CACHE_SUPPORT)) {
			int ret;

			ret = lwm2m_enable_cache(&LWM2M_OBJ(3304, 0, 5700), humidity_cache,
						 ARRAY_SIZE(humidity_cache));
			if (ret < 0) {
				LOG_WRN("Failed to enable cache for humidity sensor (%d)", ret);
			}

			if (IS_ENABLED(CONFIG_LWM2M_SEND_SCHEDULER)) {
				lwm2m_set_cache_filter(&LWM2M_OBJ(3304, 0, 5700),
						       lwm2m_send_sched_cache_filter);
			}
		}

		k_work_init_delayable(&humidity_work, humidity_work_cb);
		k_work_schedule(&humidity_work, K_NO_WAIT);
	}
}

#endif /* CONFIG_LWM2M_IPSO_HUMIDITY_SENSOR */
