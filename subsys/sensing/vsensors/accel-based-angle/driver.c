
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(accel_based_angle, CONFIG_SENSING_LOG_LEVEL);

#define DT_DRV_COMPAT sensing_accel_based_angle
struct drv_config {
	const struct sensing_sensor_info *plane0;
	const struct sensing_sensor_info *plane1;
};

struct drv_data {
	sensing_sensor_handle_t plane0;
	sensing_sensor_handle_t plane1;
	struct sensing_sensor_three_axis_data plane0_latest_sample;
	struct sensing_sensor_three_axis_data plane1_latest_sample;
	bool plane0_has_sample;
	bool plane1_has_sample;

	struct rtio_iodev_sqe *pending_read;
};

static int attribute_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	struct drv_data *data = dev->data;
	struct sensing_sensor_attribute attribute = {
		.attribute = attr,
		.value = 0,
		.shift = 0,
	};
	int rc;

	rc = sensing_set_attributes(data->plane0, SENSING_SENSOR_MODE_DONE, &attribute, 1);
	if (rc != 0) {
		LOG_ERR("Failed to set plane0 attribute");
		return rc;
	}
	rc = sensing_set_attributes(data->plane1, SENSING_SENSOR_MODE_DONE, &attribute, 1);
	if (rc != 0) {
		LOG_ERR("Failed to set plane1 attribute");
		return rc;
	}
	return rc;
}

static int submit(const struct device *sensor, struct rtio_iodev_sqe *sqe)
{
	struct drv_data *data = sensor->data;
	const struct sensor_read_config *read_cfg = sqe->sqe.iodev->data;
	int rc;

	if (read_cfg->is_streaming) {
		LOG_ERR("Streaming is not yet supported");
		rtio_iodev_sqe_err(sqe, -ENOTSUP);
		return -ENOTSUP;
	}

	rc = sensing_set_attributes(data->plane0, SENSING_SENSOR_MODE_ONE_SHOT, NULL, 0);
	rc |= sensing_set_attributes(data->plane1, SENSING_SENSOR_MODE_ONE_SHOT, NULL, 0);

	if (rc != 0) {
		rtio_iodev_sqe_err(sqe, rc);
		LOG_ERR("Failed to initiate read");
	} else {
		data->pending_read = sqe;
	}
	return 0;
}

struct encoded_data {
	uint64_t timestamp_ns;
	int8_t shift;
	q31_t val;
} __packed;

static int decoder_get_frame_count(const uint8_t *buffer, uint16_t *frame_count)
{
	ARG_UNUSED(buffer);
	*frame_count = 1;
	return 0;
}

static int decoder_get_timestamp(const uint8_t *buffer, uint64_t *timestamp_ns)
{
	*timestamp_ns = ((struct encoded_data *)buffer)->timestamp_ns;
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

	if (channel_type != SENSOR_CHAN_ROTATION) {
		return -EINVAL;
	}
	*shift = ((struct encoded_data *)buffer)->shift;
	return 0;
}

static int decoder_decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
			  sensor_channel_iterator_t *cit, enum sensor_channel *channels,
			  q31_t *values, uint8_t max_count)
{
	if (*fit != 0) {
		return 0;
	}
	if (*cit > 0) {
		return -EINVAL;
	}

	values[0] = ((struct encoded_data *)buffer)->val;
	channels[0] = SENSOR_CHAN_ROTATION;
	*fit = 1;
	*cit = 0;
	return 1;
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

SENSING_DMEM static const struct sensor_driver_api angle_api = {
	.attr_set = attribute_set,
	.attr_get = NULL,
	.get_decoder = get_decoder,
	.submit = submit,
};

static void on_data_event(sensing_sensor_handle_t handle, const void *buf, void *userdata)
{
	const struct device *dev = userdata;
	struct drv_data *data = dev->data;

	if (handle == data->plane0) {
		LOG_INF("Got data for plane0");
		memcpy(&data->plane0_latest_sample, buf,
		       sizeof(struct sensing_sensor_three_axis_data));
		data->plane0_has_sample = true;
	} else if (handle == data->plane1) {
		LOG_INF("Got data for plane1");
		memcpy(&data->plane1_latest_sample, buf,
		       sizeof(struct sensing_sensor_three_axis_data));
		data->plane1_has_sample = true;
	}

	if (!data->plane0_has_sample || !data->plane1_has_sample) {
		return;
	}

	/* We have 2 samples */
	LOG_DBG("plane0(0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ")",
		data->plane0_latest_sample.readings[0].x, data->plane0_latest_sample.readings[0].y,
		data->plane0_latest_sample.readings[0].z);
	LOG_DBG("plane1(0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ")",
		data->plane1_latest_sample.readings[0].x, data->plane1_latest_sample.readings[0].y,
		data->plane1_latest_sample.readings[0].z);
	int8_t shift = MAX(data->plane0_latest_sample.shift, data->plane1_latest_sample.shift);

	/* Check that they both use the same shift */
	if (data->plane0_latest_sample.shift != data->plane1_latest_sample.shift) {

		if (data->plane0_latest_sample.shift != shift) {
			int8_t new_shift = data->plane0_latest_sample.shift - shift;
			data->plane0_latest_sample.readings[0].x =
				(new_shift < 0)
					? (data->plane0_latest_sample.readings[0].x >> -new_shift)
					: (data->plane0_latest_sample.readings[0].x << new_shift);
			data->plane0_latest_sample.readings[0].y =
				(new_shift < 0)
					? (data->plane0_latest_sample.readings[0].y >> -new_shift)
					: (data->plane0_latest_sample.readings[0].y << new_shift);
			data->plane0_latest_sample.readings[0].z =
				(new_shift < 0)
					? (data->plane0_latest_sample.readings[0].z >> -new_shift)
					: (data->plane0_latest_sample.readings[0].z << new_shift);
		}

		if (data->plane1_latest_sample.shift != shift) {
			int8_t new_shift = data->plane1_latest_sample.shift - shift;
			data->plane1_latest_sample.readings[0].x =
				(new_shift < 0)
					? (data->plane1_latest_sample.readings[0].x >> -new_shift)
					: (data->plane1_latest_sample.readings[0].x << new_shift);
			data->plane1_latest_sample.readings[0].y =
				(new_shift < 0)
					? (data->plane1_latest_sample.readings[0].y >> -new_shift)
					: (data->plane1_latest_sample.readings[0].y << new_shift);
			data->plane1_latest_sample.readings[0].z =
				(new_shift < 0)
					? (data->plane1_latest_sample.readings[0].z >> -new_shift)
					: (data->plane1_latest_sample.readings[0].z << new_shift);
		}
	}

	LOG_DBG("plane0(0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ") (%d, %d, %d)",
		data->plane0_latest_sample.readings[0].x, data->plane0_latest_sample.readings[0].y,
		data->plane0_latest_sample.readings[0].z, data->plane0_latest_sample.readings[0].x,
		data->plane0_latest_sample.readings[0].y, data->plane0_latest_sample.readings[0].z);
	LOG_DBG("plane1(0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ") (%d, %d, %d)",
		data->plane1_latest_sample.readings[0].x, data->plane1_latest_sample.readings[0].y,
		data->plane1_latest_sample.readings[0].z, data->plane1_latest_sample.readings[0].x,
		data->plane1_latest_sample.readings[0].y, data->plane1_latest_sample.readings[0].z);

	/*
	 * 5.27 x 5.27 = 10.54
	 * 10.54 >> 14 = 24.40
	 */
	q63_t result;
	int32_t integer_part;

	zdsp_dot_prod_q31(data->plane0_latest_sample.readings[0].values,
			  data->plane1_latest_sample.readings[0].values, 3, &result);
	integer_part = (int32_t)FIELD_GET(GENMASK64(63, 48 - shift * 2), result);

	LOG_DBG("Dot product is 0x%016" PRIx64 " %d.%06u shift=%d", result, integer_part,
		(uint32_t)(FIELD_GET(GENMASK64(47 - shift * 2, 0), result) * INT64_C(1000000) /
			   ((INT64_C(1) << (48 - shift * 2)) - 1)),
		shift);

	data->plane0_has_sample = false;
	data->plane1_has_sample = false;

	uint8_t *out_buf;
	uint32_t buf_len;
	int rc = rtio_sqe_rx_buf(data->pending_read, sizeof(struct encoded_data),
				 sizeof(struct encoded_data), &out_buf, &buf_len);
	if (rc != 0) {
		rtio_iodev_sqe_err(data->pending_read, rc);
		data->pending_read = NULL;
		return;
	}

	struct encoded_data *edata = (struct encoded_data *)out_buf;
	edata->timestamp_ns = MAX(data->plane0_latest_sample.header.base_timestamp,
				       data->plane1_latest_sample.header.base_timestamp);
	edata->shift = ilog2(llabs(integer_part)) + 1;
	int8_t extra_shift = (47 - shift * 2) - (31 - edata->shift) + 1;
	edata->val = FIELD_GET(GENMASK(31, 0), result >> extra_shift);
	LOG_DBG("shift=%d, val=0x%08" PRIx32, edata->shift, edata->val);
	rtio_iodev_sqe_ok(data->pending_read, 0);
	data->pending_read = NULL;
}

static const struct sensing_callback_list cb_list = {
	.on_data_event = on_data_event,
};

static int init(const struct device *dev)
{
	const struct drv_config *cfg = dev->config;
	struct drv_data *data = dev->data;
	int rc;

	rc = sensing_open_sensor(cfg->plane0, &cb_list, &data->plane0, (void *)dev);
	if (rc != 0) {
		LOG_ERR("Failed to open connection to plane0");
		return rc;
	}

	rc = sensing_open_sensor(cfg->plane1, &cb_list, &data->plane1, (void *)dev);
	if (rc != 0) {
		sensing_close_sensor(data->plane0);
		LOG_ERR("Failed to open connection to plane1");
		return rc;
	}

	return 0;
}

#define DECLARE_IODEV(node_id, inst)                                                               \
	SENSOR_DT_READ_IODEV(read_iodev_##inst, node_id, SENSOR_CHAN_ALL);                         \
	IF_ENABLED(CONFIG_SENSING_SHELL, (static char name_buffer_##inst[5];))                     \
	static struct sensor_info sensor_info_##inst = SENSOR_INFO_INITIALIZER(                    \
		DEVICE_DT_GET(node_id), DT_NODE_VENDOR_OR(node_id, NULL),                          \
		DT_NODE_MODEL_OR(node_id, NULL), DT_PROP_OR(node_id, friendly_name, NULL));        \
	const STRUCT_SECTION_ITERABLE(sensing_sensor_info, sensing_sensor_info_##inst) = {         \
		.info = &sensor_info_##inst,                                                       \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.type = SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE,                                    \
		.iodev = &read_iodev_##inst,                                                       \
		IF_ENABLED(CONFIG_SENSING_SHELL, (.shell_name = name_buffer_##inst, ))}

#define DRV_INIT(inst)                                                                             \
	static const struct drv_config cfg_##inst = {                                              \
		.plane0 = SENSING_SENSOR_INFO_GET(DT_INST_PHANDLE(inst, plane0),                   \
						  SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D),    \
		.plane1 = SENSING_SENSOR_INFO_GET(DT_INST_PHANDLE(inst, plane1),                   \
						  SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D),    \
	};                                                                                         \
	static struct drv_data data_##inst = {0};                                                  \
	DECLARE_IODEV(DT_DRV_INST(inst), inst);                                                    \
	DEVICE_DT_INST_DEFINE(inst, init, NULL, &data_##inst, &cfg_##inst, APPLICATION, 10,        \
			      &angle_api);

DT_INST_FOREACH_STATUS_OKAY(DRV_INIT)
