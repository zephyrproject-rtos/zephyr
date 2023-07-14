
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensing_emul, CONFIG_SENSING_LOG_LEVEL);

#define DT_DRV_COMPAT sensing_emul
struct drv_config {
	const struct sensing_sensor_info **info;
	size_t info_count;
};

struct drv_data {
};

static int attribute_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	return 0;
}

static int is_channel_supported(enum sensor_channel channel, const struct drv_config *cfg)
{
	for (int i = 0; i < cfg->info_count; ++i) {
		int32_t type = cfg->info[i]->type;
		switch (channel) {
		case SENSOR_CHAN_ALL:
			return i;
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			if (type == SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D ||
			    type == SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D) {
				return i;
			}
			break;
		case SENSOR_CHAN_GYRO_X:
		case SENSOR_CHAN_GYRO_Y:
		case SENSOR_CHAN_GYRO_Z:
		case SENSOR_CHAN_GYRO_XYZ:
			if (type == SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D) {
				return i;
			}
			break;
		case SENSOR_CHAN_ROTATION:
			if (type == SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE) {
				return i;
			}
			break;
		default:
			break;
		}
	}

	return -1;
}

static int submit(const struct device *sensor, struct rtio_iodev_sqe *sqe)
{
	const struct drv_config *cfg = sensor->config;
	const struct sensor_read_config *read_cfg = sqe->sqe.iodev->data;
	const struct sensing_sensor_info *info = NULL;
	uint8_t *buf;
	uint32_t buf_len;
	int rc;

	if (read_cfg->is_streaming) {
		rtio_iodev_sqe_err(sqe, -ENOTSUP);
		return 0;
	}

	/* For now just assume the user wants to read all the channels */

	for (size_t i = 0; i < read_cfg->count; ++i) {
		int idx = is_channel_supported(read_cfg->channels[i], cfg);
		if (idx >= 0) {
			info = cfg->info[idx];
			break;
		}
	}

	if (info == NULL) {
		LOG_ERR("Invalid read request");
		rtio_iodev_sqe_err(sqe, -EINVAL);
		return 0;
	}

	switch (info->type) {
	case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
	case SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D:
		rc = rtio_sqe_rx_buf(sqe, sizeof(struct sensing_sensor_three_axis_data),
				     sizeof(struct sensing_sensor_three_axis_data), &buf, &buf_len);
		if (rc != 0) {
			rtio_iodev_sqe_err(sqe, rc);
		} else {
			struct sensing_sensor_three_axis_data *edata =
				(struct sensing_sensor_three_axis_data *)buf;
			edata->header.base_timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
			edata->header.reading_count = 1;
			edata->shift = 4;
			edata->readings[0].timestamp_delta = 0;
			edata->readings[0].x = (q31_t)((9.8f / 16.0f) * INT32_MAX);
			edata->readings[0].y = 0;
			edata->readings[0].z = 0;
			rtio_iodev_sqe_ok(sqe, 0);
		}
		break;
	case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
		rc = rtio_sqe_rx_buf(sqe, sizeof(struct sensing_sensor_three_axis_data),
				     sizeof(struct sensing_sensor_three_axis_data), &buf, &buf_len);
		if (rc != 0) {
			rtio_iodev_sqe_err(sqe, rc);
		} else {
			struct sensing_sensor_three_axis_data *edata =
				(struct sensing_sensor_three_axis_data *)buf;
			edata->header.base_timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
			edata->header.reading_count = 1;
			edata->shift = 0;
			edata->readings[0].timestamp_delta = 0;
			edata->readings[0].x = 0;
			edata->readings[0].y = 0;
			edata->readings[0].z = 0;
			rtio_iodev_sqe_ok(sqe, 0);
		}
		break;
	case SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE:
		rc = rtio_sqe_rx_buf(sqe, sizeof(struct sensing_sensor_float_data),
				     sizeof(struct sensing_sensor_float_data), &buf, &buf_len);
		if (rc != 0) {
			rtio_iodev_sqe_err(sqe, rc);
		} else {
			struct sensing_sensor_float_data *edata =
				(struct sensing_sensor_float_data *)buf;
			edata->header.base_timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
			edata->header.reading_count = 1;
			edata->shift = 0;
			edata->readings[0].timestamp_delta = 0;
			edata->readings[0].v = 0;
			rtio_iodev_sqe_ok(sqe, 0);
		}
		break;
	default:
		rtio_iodev_sqe_err(sqe, -ENOTSUP);
		break;
	}
	return 0;
}

SENSING_DMEM static const struct sensor_driver_api emul_api = {
	.attr_set = attribute_set,
	.attr_get = NULL,
	.get_decoder = NULL,
	.submit = submit,
};

static int init(const struct device *dev)
{
	//	const struct drv_config *cfg = dev->config;
	//	struct drv_data *data = dev->data;

	return 0;
}

#define EMUL_SENSOR_INFO_NAME(node_id, type)                                                       \
	_CONCAT(_CONCAT(__emul_sensor_info, DEVICE_DT_NAME_GET(node_id)), type)

#define DECLARE_SENSOR_INFO_NAME(node_id, name, prop, idx, _iodev, _info)                          \
	IF_ENABLED(CONFIG_SENSING_SHELL, (static char node_id##_##idx##_name_buffer[5];))          \
	const STRUCT_SECTION_ITERABLE(sensing_sensor_info, name) = {                               \
		.info = &(_info),                                                                  \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.type = DT_PROP_BY_IDX(node_id, prop, idx),                                        \
		.iodev = &(_iodev),                                                                \
		IF_ENABLED(CONFIG_SENSING_SHELL, (.shell_name = node_id##_##idx##_name_buffer, ))}

#define DECLARE_SENSOR_INFO(node_id, prop, idx, _iodev, _info)                                     \
	DECLARE_SENSOR_INFO_NAME(                                                                  \
		node_id, SENSING_SENSOR_INFO_DT_NAME(node_id, DT_PROP_BY_IDX(node_id, prop, idx)), \
		prop, idx, _iodev, _info)

#define DECLARE_IODEV(node_id, inst)                                                               \
	SENSOR_DT_READ_IODEV(emul_read_iodev_##inst, node_id, SENSOR_CHAN_ALL);                    \
	static struct sensor_info sensor_info_##inst = SENSOR_INFO_INITIALIZER(                    \
		DEVICE_DT_GET(node_id), DT_NODE_VENDOR_OR(node_id, NULL),                          \
		DT_NODE_MODEL_OR(node_id, NULL), DT_PROP_OR(node_id, friendly_name, NULL));        \
	DT_FOREACH_PROP_ELEM_VARGS(node_id, sensor_types, DECLARE_SENSOR_INFO,                     \
				   emul_read_iodev_##inst, sensor_info_##inst)

#define SENSOR_INFO_ITEM(node_id, prop, idx)                                                       \
	&SENSING_SENSOR_INFO_DT_NAME(node_id, DT_PROP_BY_IDX(node_id, prop, idx))

#define DRV_INIT(inst)                                                                             \
	DECLARE_IODEV(DT_DRV_INST(inst), inst);                                                    \
	static const struct sensing_sensor_info *info_array_##inst[DT_INST_PROP_LEN(               \
		inst, sensor_types)] = {                                                           \
		DT_INST_FOREACH_PROP_ELEM_SEP(inst, sensor_types, SENSOR_INFO_ITEM, (, ))};        \
	static const struct drv_config cfg_##inst = {                                              \
		.info = (const struct sensing_sensor_info **)&info_array_##inst,                   \
		.info_count = DT_INST_PROP_LEN(inst, sensor_types),                                \
	};                                                                                         \
	static struct drv_data data_##inst = {};                                                   \
	DEVICE_DT_INST_DEFINE(inst, init, NULL, &data_##inst, &cfg_##inst, APPLICATION, 10,        \
			      &emul_api);

DT_INST_FOREACH_STATUS_OKAY(DRV_INIT)
