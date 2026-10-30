/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Darshan Maru <darshan.maru@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DT_DRV_COMPAT is required here: SENSOR_DECODER_API_DT_DEFINE() and
 * SENSOR_DECODER_NAME() are built from it.
 */
#define DT_DRV_COMPAT st_vl53l4cd

#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/vl53l4cd.h>
#include <zephyr/sys/util.h>

#include "vl53l4cd.h"

/**
 * @brief Check whether a channel can be decoded by this driver.
 *
 * @param chan_type Channel type to check.
 *
 * @return true if the channel is supported, false otherwise.
 */
static bool vl53l4cd_chan_supported(int chan_type)
{
	switch (chan_type) {
	case SENSOR_CHAN_DISTANCE:
	case SENSOR_CHAN_VL53L4CD_RANGE_STATUS:
	case SENSOR_CHAN_VL53L4CD_SPAD_COUNT:
	case SENSOR_CHAN_VL53L4CD_SIGNAL_RATE:
	case SENSOR_CHAN_VL53L4CD_AMBIENT_RATE:
	case SENSOR_CHAN_VL53L4CD_SIGNAL_PER_SPAD:
	case SENSOR_CHAN_VL53L4CD_AMBIENT_PER_SPAD:
	case SENSOR_CHAN_VL53L4CD_SIGMA:
		return true;
	default:
		return false;
	}
}

/**
 * @brief Convert an unsigned integer value to Q31.
 *
 * The result uses the VL53L4CD_RAW_Q31_SHIFT shift. The largest value the
 * sensor reports (524280 kcps) is below 2^20, so it always fits.
 *
 * @param value Integer value to convert.
 *
 * @return Value in Q31 format.
 */
static q31_t vl53l4cd_int_to_q31(uint32_t value)
{
	return (q31_t)((int64_t)value << (31 - VL53L4CD_RAW_Q31_SHIFT));
}

/**
 * @brief Convert a distance in millimeters to meters in Q31.
 *
 * The result uses the VL53L4CD_DISTANCE_Q31_SHIFT shift and is rounded to
 * the nearest Q31 step.
 *
 * @param mm Distance in millimeters.
 *
 * @return Distance in meters, in Q31 format.
 */
static q31_t vl53l4cd_mm_to_q31(uint16_t mm)
{
	int64_t scaled = (int64_t)mm * (1LL << (31 - VL53L4CD_DISTANCE_Q31_SHIFT));

	return (q31_t)((scaled + 500) / 1000);
}

/**
 * @brief Get the number of frames available for a channel.
 *
 * The sensor has no FIFO, so an encoded buffer always holds one frame.
 *
 * @param buffer      Encoded data buffer (unused).
 * @param chan_spec   Channel to query.
 * @param frame_count Output: number of frames.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the channel or channel index is not supported.
 */
static int vl53l4cd_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (chan_spec.chan_idx != 0 || !vl53l4cd_chan_supported(chan_spec.chan_type)) {
		return -ENOTSUP;
	}

	*frame_count = 1;
	return 0;
}

/**
 * @brief Get the decoded data sizes for a channel.
 *
 * @param chan_spec  Channel to query.
 * @param base_size  Output: size of the data header plus the first frame.
 * @param frame_size Output: size of each additional frame.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the channel is not supported.
 */
static int vl53l4cd_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	if (!vl53l4cd_chan_supported(chan_spec.chan_type)) {
		return -ENOTSUP;
	}

	*base_size = sizeof(struct sensor_q31_data);
	*frame_size = sizeof(struct sensor_q31_sample_data);
	return 0;
}

/**
 * @brief Decode an encoded sample into a Q31 sensor data structure.
 *
 * The decoder is stateless. Since there is only one frame per buffer, the
 * @p fit iterator is used only to report that the frame was already decoded.
 *
 * @param buffer     Encoded data buffer filled by the driver.
 * @param chan_spec  Channel to decode.
 * @param fit        Frame iterator, 0 for the first call.
 * @param max_count  Maximum number of frames @p data_out can hold.
 * @param data_out   Output: struct sensor_q31_data.
 *
 * @return Number of frames decoded (1), 0 when there is no more data.
 * @retval -EINVAL if @p max_count is 0 or the channel index is not 0.
 * @retval -ENOTSUP if the channel is not supported.
 */
static int vl53l4cd_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct vl53l4cd_encoded_data *edata = (const struct vl53l4cd_encoded_data *)buffer;
	struct sensor_q31_data *out = data_out;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	out->header.base_timestamp_ns = edata->timestamp_ns;
	out->header.reading_count = 1;
	out->readings[0].timestamp_delta = 0;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DISTANCE:
		out->shift = VL53L4CD_DISTANCE_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_mm_to_q31(edata->range_mm);
		break;
	case SENSOR_CHAN_VL53L4CD_RANGE_STATUS:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(edata->range_status);
		break;
	case SENSOR_CHAN_VL53L4CD_SPAD_COUNT:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(edata->spad_count);
		break;
	case SENSOR_CHAN_VL53L4CD_SIGNAL_RATE:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(edata->signal_rate_kcps);
		break;
	case SENSOR_CHAN_VL53L4CD_AMBIENT_RATE:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(edata->ambient_rate_kcps);
		break;
	case SENSOR_CHAN_VL53L4CD_SIGNAL_PER_SPAD:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(edata->signal_per_spad_kcps);
		break;
	case SENSOR_CHAN_VL53L4CD_AMBIENT_PER_SPAD:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(edata->ambient_per_spad_kcps);
		break;
	case SENSOR_CHAN_VL53L4CD_SIGMA:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(edata->sigma_mm);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;
	return 1;
}

/**
 * @brief Check whether an encoded buffer was produced by a trigger.
 *
 * The driver does not support triggers, so this always returns false.
 *
 * @param buffer  Encoded data buffer (unused).
 * @param trigger Trigger type to check (unused).
 *
 * @return Always false.
 */
static bool vl53l4cd_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);

	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = vl53l4cd_decoder_get_frame_count,
	.get_size_info = vl53l4cd_decoder_get_size_info,
	.decode = vl53l4cd_decoder_decode,
	.has_trigger = vl53l4cd_decoder_has_trigger,
};

/**
 * @brief Get the decoder API of the driver.
 *
 * @param dev     Sensor device (unused, the decoder is stateless).
 * @param decoder Output: pointer to the decoder API.
 *
 * @return Always 0.
 */
int vl53l4cd_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
