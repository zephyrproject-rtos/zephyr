#include <zephyr/sensing/transform.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensing_transform, CONFIG_SENSING_LOG_LEVEL);

static int decode_header(struct sensing_sensor_value_header *header,
			 const struct sensor_decoder_api *decoder, void *data)
{
	uint16_t frame_count;
	uint64_t timestamp_ns;
	int rc;

	rc = decoder->get_timestamp(data, &timestamp_ns);
	rc |= decoder->get_frame_count(data, &frame_count);
	if (rc != 0) {
		return -EINVAL;
	}
	header->base_timestamp = timestamp_ns;
	header->reading_count = frame_count;
	return 0;
}

int decode_three_axis_data(int32_t type, struct sensing_sensor_three_axis_data *out,
			   const struct sensor_decoder_api *decoder, void *data)
{
	uint16_t frame_count;
	int rc;

	rc = decoder->get_frame_count(data, &frame_count);
	if (rc != 0) {
		return rc;
	}

	__ASSERT_NO_MSG(frame_count == 1);
	LOG_DBG("Decoding 1 frame for 3 axis data from %p", data);

	sensor_frame_iterator_t fit = {0};
	sensor_channel_iterator_t cit = {0};
	enum sensor_channel channel;
	q31_t value;
	bool has_shift = false;

	rc = decode_header(&out->header, decoder, data);
	if (rc != 0) {
		return rc;
	}

	while (true) {
		rc = decoder->decode(data, &fit, &cit, &channel, &value, 1);
		if (rc < 0) {
			LOG_ERR("Failed to decode entry (%d)", rc);
			return rc;
		}
		if (rc == 0) {
			return 0;
		}
		switch (type) {
		case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
		case SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D:
			if (channel == SENSOR_CHAN_ACCEL_X || channel == SENSOR_CHAN_ACCEL_Y ||
			    channel == SENSOR_CHAN_ACCEL_Z) {
				LOG_DBG("Got [%d] for accel type, value=0x%08x", (int)channel,
					value);
				out->readings[0].values[channel] = value;
				if (!has_shift) {
					rc = decoder->get_shift(data, channel, &out->shift);
					if (rc != 0) {
						return rc;
					}
					has_shift = true;
					LOG_DBG("Got shift value %d", out->shift);
				}
			}
			break;
		case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
			if (channel == SENSOR_CHAN_GYRO_X || channel == SENSOR_CHAN_GYRO_Y ||
			    channel == SENSOR_CHAN_GYRO_Z) {
				out->readings[0].values[channel - SENSOR_CHAN_GYRO_X] = value;
				if (!has_shift) {
					rc = decoder->get_shift(data, channel, &out->shift);
					if (rc != 0) {
						return rc;
					}
					has_shift = true;
				}
			}
			break;
		}
	}
}

int decode_float_data(int32_t type, struct sensing_sensor_float_data *out,
		      const struct sensor_decoder_api *decoder, void *data)
{
	uint16_t frame_count;
	int rc;

	rc = decoder->get_frame_count(data, &frame_count);
	if (rc != 0) {
		return rc;
	}

	__ASSERT_NO_MSG(frame_count == 1);

	sensor_frame_iterator_t fit = {0};
	sensor_channel_iterator_t cit = {0};
	enum sensor_channel channel;
	bool has_shift = false;

	while (true) {
		rc = decoder->decode(data, &fit, &cit, &channel, &out->readings[0].v, 1);
		if (rc <= 0) {
			return rc;
		}
		if (type == SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE &&
		    channel == SENSOR_CHAN_ROTATION && !has_shift) {
			rc = decoder->get_shift(data, channel, &out->shift);
			if (rc != 0) {
				return rc;
			}
			has_shift = true;
		}
	}
}
