/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME app_temp
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "modules.h"

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/net/lwm2m.h>

static void *temperature_get_buf(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				 size_t *data_len)
{
	/* Last read temperature value, will use 25.5C if no sensor available */
	static double v = 25.5;
	const struct device *dev = NULL;

#if defined(CONFIG_FXOS8700_TEMP)
	dev = DEVICE_DT_GET_ONE(nxp_fxos8700);

	if (!device_is_ready(dev)) {
		LOG_ERR("%s: device not ready.", dev->name);
		return;
	}
#endif

	if (dev != NULL) {
		struct sensor_value val;

		if (sensor_sample_fetch(dev)) {
			LOG_ERR("temperature data update failed");
		}

		sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val);

		v = sensor_value_to_double(&val);

		LOG_DBG("LWM2M temperature set to %f", v);
	}

	/* echo the value back through the engine to update min/max values */
	lwm2m_engine_set_float("3303/0/5700", &v);
	*data_len = sizeof(v);
	return &v;
}

void init_temp_sensor(void)
{
	lwm2m_engine_create_obj_inst("3303/0");
	lwm2m_engine_register_read_callback("3303/0/5700", temperature_get_buf);
}
