//
// Created by peress on 05/07/23.
//

#include <zephyr/kernel.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>

#include "sensing/internal/sensing.h"

LOG_MODULE_REGISTER(sensing_processing, CONFIG_SENSING_LOG_LEVEL);

static int decode_three_axis_data(int32_t type, struct sensing_sensor_three_axis_data *out,
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
				LOG_DBG("Got [%d] for accel type, value=0x%08x", (int)channel, value);
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

static int decode_float_data(int32_t type, struct sensing_sensor_float_data *out,
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

static void processing_task(void *a, void *b, void *c)
{
	struct sensing_sensor_info *info;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int rc;
	int get_data_rc;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	if (IS_ENABLED(CONFIG_USERSPACE) && !k_is_user_context()) {
		rtio_access_grant(&sensing_rtio_ctx, k_current_get());
		k_thread_user_mode_enter(processing_task, a, b, c);
	}

	while (true) {
		struct rtio_cqe cqe;

		rc = rtio_cqe_copy_out(&sensing_rtio_ctx, &cqe, 1, K_FOREVER);

		if (rc < 1) {
			continue;
		}

		/* Cache the data from the CQE */
		rc = cqe.result;
		info = cqe.userdata;
		get_data_rc =
			rtio_cqe_get_mempool_buffer(&sensing_rtio_ctx, &cqe, &data, &data_len);

		/* Do something with the data */
		LOG_DBG("Processing data for [%d], rc=%d, has_data=%d, data=%p, len=%" PRIu32,
			(int)(info - STRUCT_SECTION_START(sensing_sensor_info)), rc,
			get_data_rc == 0, (void *)data, data_len);

		if (get_data_rc != 0 || data_len == 0) {
			continue;
		}

		const struct sensor_decoder_api *decoder = NULL;

		rc = sensor_get_decoder(info->dev, &decoder);
		if (rc != 0) {
			LOG_ERR("Failed to get decoder");
			goto end;
		}

		union {
			struct sensing_sensor_three_axis_data three_axis_data;
			struct sensing_sensor_float_data float_data;
		} decoded_data;

		switch (info->type) {
		case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
		case SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D:
		case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
			rc = decode_three_axis_data(info->type, &decoded_data.three_axis_data,
						    decoder, data);
			if (rc != 0) {
				LOG_ERR("Failed to decode");
				goto end;
			}
			break;
		case SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE:
			rc = decode_float_data(info->type, &decoded_data.float_data, decoder, data);
			if (rc != 0) {
				LOG_ERR("Failed to decode");
				goto end;
			}
			break;
		default:
			LOG_ERR("Sensor type not supported");
			goto end;
		}

		sys_mutex_lock(__sensing_connection_pool.lock, K_FOREVER);
		for (int i = 0; i < __CONNECTION_POOL_COUNT; ++i) {
			struct sensing_connection *connection =
				&STRUCT_SECTION_START(sensing_connection)[i];
			if (!__sensing_is_connected(info, connection)) {
				continue;
			}
			sensing_data_event_t cb = connection->cb_list->on_data_event;

			if (cb == NULL) {
				continue;
			}
			cb(connection, &decoded_data, connection->userdata);
		}
		sys_mutex_unlock(__sensing_connection_pool.lock);

	end:
		/* Release the memory */
		rtio_release_buffer(&sensing_rtio_ctx, data, data_len);
	}
}

K_THREAD_DEFINE(sensing_processor, CONFIG_SENSING_PROCESSING_THREAD_STACK_SIZE, processing_task,
		NULL, NULL, NULL, CONFIG_SENSING_PROCESSING_THREAD_PRIORITY, 0, 0);
