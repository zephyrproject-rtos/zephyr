
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

static inline bool is_channel_supported(const struct drv_config *cfg, enum sensor_channel channel)
{
	int32_t required_type;

	switch (channel) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		required_type = SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D;
		break;
	default:
		return false;
	}

	for (size_t i = 0; i < cfg->info_count; ++i) {
		if (cfg->info[i]->type == required_type) {
			return true;
		}
	}
	return false;
}

static int attribute_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	return 0;
}

struct raw_data {
	uint64_t timestamp_ns;
	uint8_t count;
	enum sensor_channel channels[0];
} __packed;

static int submit(const struct device *sensor, struct rtio_iodev_sqe *sqe)
{
	const struct drv_config *cfg = sensor->config;
	const struct sensor_read_config *read_cfg = sqe->sqe.iodev->data;
	uint8_t *buf;
	uint32_t buf_len;
	uint8_t channel_count = 0;
	int rc;

	if (read_cfg->is_streaming) {
		rtio_iodev_sqe_err(sqe, -ENOTSUP);
		return 0;
	}

	/* For now just assume the user wants to read all the channels */
	for (size_t i = 0; i < cfg->info_count; ++i) {
		switch (cfg->info[i]->type) {
		case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
		case SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D:
		case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
			channel_count += 3;
			break;
		case SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE:
			channel_count += 1;
			break;
		default:
			rtio_iodev_sqe_err(sqe, -ENOTSUP);
			return 0;
		}
	}

	const uint32_t desired_size =
		sizeof(struct raw_data) + channel_count * sizeof(enum sensor_channel);

	rc = rtio_sqe_rx_buf(sqe, desired_size, desired_size, &buf, &buf_len);
	if (rc != 0) {
		rtio_iodev_sqe_err(sqe, rc);
		return 0;
	}

	((struct raw_data *)buf)->timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	((struct raw_data *)buf)->count = channel_count;
	buf += sizeof(struct raw_data);

	int count = 0;
	for (size_t i = 0; i < cfg->info_count; ++i) {
		switch (cfg->info[i]->type) {
		case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
		case SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D:
			((enum sensor_channel *)buf)[count++] = SENSOR_CHAN_ACCEL_X;
			((enum sensor_channel *)buf)[count++] = SENSOR_CHAN_ACCEL_Y;
			((enum sensor_channel *)buf)[count++] = SENSOR_CHAN_ACCEL_Z;
			break;
		case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
			((enum sensor_channel *)buf)[count++] = SENSOR_CHAN_GYRO_X;
			((enum sensor_channel *)buf)[count++] = SENSOR_CHAN_GYRO_Y;
			((enum sensor_channel *)buf)[count++] = SENSOR_CHAN_GYRO_Z;
			break;
		case SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE:
			((enum sensor_channel *)buf)[count++] = SENSOR_CHAN_ROTATION;
			break;
		default:
			rtio_iodev_sqe_err(sqe, -ENOTSUP);
			return 0;
		}
	}

	rtio_iodev_sqe_ok(sqe, 0);
	return 0;
}

static int decoder_get_frame_count(const uint8_t *buffer, uint16_t *frame_count)
{
	ARG_UNUSED(buffer);
	*frame_count = 1;
	return 0;
}

static int decoder_get_timestamp(const uint8_t *buffer, uint64_t *timestamp_ns)
{
	*timestamp_ns = ((uint64_t *)buffer)[0];
	return 0;
}

static bool decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);
	return false;
}

static int decoder_get_shift(const uint8_t *buffer, enum sensor_channel channel_type, int8_t *shift)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(channel_type);
	*shift = 0;
	return 0;
}

static int decoder_decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
			  sensor_channel_iterator_t *cit, enum sensor_channel *channels,
			  q31_t *values, uint8_t max_count)
{
	if (*fit != 0) {
		return 0;
	}

	const struct raw_data *data = (const struct raw_data *)buffer;
	int count = 0;

	if (*cit >= data->count) {
		return -EINVAL;
	}

	while (*cit < data->count && count < max_count) {
		channels[count] = data->channels[*cit];
		values[count++] = INT32_MAX;
		*cit += 1;
	}

	if (*cit == data->count) {
		*fit += 1;
		*cit = 0;
	}
	return count;
}

SENSING_DMEM static const struct sensor_decoder_api decoder_api = {
	.get_frame_count = decoder_get_frame_count,
	.get_timestamp = decoder_get_timestamp,
	.has_trigger = decoder_has_trigger,
	.get_shift = decoder_get_shift,
	.decode = decoder_decode,
};

static int get_decoder(const struct device *dev, const struct sensor_decoder_api **api)
{
	ARG_UNUSED(dev);
	*api = &decoder_api;
	return 0;
}

SENSING_DMEM static const struct sensor_driver_api emul_api = {
	.attr_set = attribute_set,
	.attr_get = NULL,
	.get_decoder = get_decoder,
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
