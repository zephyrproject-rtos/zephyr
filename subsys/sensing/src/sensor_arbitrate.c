//
// Created by peress on 27/06/23.
//

#include <zephyr/sensing/sensing.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "sensing/internal/sensing.h"

LOG_MODULE_REGISTER(sensing_arbitrate, CONFIG_SENSING_LOG_LEVEL);

static q31_t arbitrate_attribute_value(enum sensor_attribute attribute, q31_t current_value,
				       q31_t new_value)
{
	switch (attribute) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return MAX(current_value, new_value);
	default:
		return current_value;
	}
}

static void set_arbitrated_value(const struct device *dev, int32_t type,
				 enum sensor_attribute attribute, q31_t value)
{
	enum sensor_channel chan;
	switch (type) {
	case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
		chan = SENSOR_CHAN_ACCEL_XYZ;
		break;
	case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
		chan = SENSOR_CHAN_GYRO_XYZ;
		break;
	default:
		return;
	}

	struct sensor_value val = {
		.val1 = FIELD_GET(GENMASK(31, 16), value),
		.val2 = (FIELD_GET(GENMASK(15, 0), value) * 1000000) / INT16_MAX,
	};
	LOG_DBG("Updating attribute chan=%d, value=%d/%d", chan, val.val1, val.val2);
	((const struct sensor_driver_api *)(dev->api))->attr_set(dev, chan, attribute, &val);
}

static int arbitrate_sensor_attribute(const struct sensing_sensor_info *info,
				      enum sensor_attribute attribute)
{
	const struct sensing_connection *connections = STRUCT_SECTION_START(sensing_connection);
	int connection_count = 0;
	q31_t value = 0;

	for (int i = 0; i < __CONNECTION_POOL_COUNT; ++i) {
		if (!__sensing_is_connected(info, &connections[i])) {
			continue;
		}
		if (FIELD_GET(BIT(attribute), connections[i].attribute_mask) == 0) {
			continue;
		}
		if (connection_count == 0) {
			/* First connection */
			value = connections[i].attributes[attribute];
			LOG_DBG("Arbitrating '%s'@%p type=%d attribute=%d", info->info->dev->name,
				info->info->dev, info->type, attribute);
			LOG_DBG("    First connection %d/%p, value=0x%08x", i, (void*)&connections[i], value);
		} else {
			value = arbitrate_attribute_value(attribute, value,
							  connections[i].attributes[attribute]);
			LOG_DBG("    Updating         %d/%p, value=0x%08x", i, (void*)&connections[i], value);
		}
		connection_count++;
	}

	if (connection_count != 0) {
		set_arbitrated_value(info->dev, info->type, attribute, value);
	}

	return connection_count;
}

static void arbitrate_sensor_instance(const struct sensing_sensor_info *info)
{
	int count = 0;

	for (int i = 0; i < SENSOR_ATTR_COMMON_COUNT; ++i) {
		count += arbitrate_sensor_attribute(info, i);
	}
	LOG_DBG("Arbitrated %p with %d connections", (void*)info, count);
}

void __sensing_arbitrate(void)
{
	STRUCT_SECTION_FOREACH(sensing_sensor_info, info) {
		arbitrate_sensor_instance(info);
	}
}
