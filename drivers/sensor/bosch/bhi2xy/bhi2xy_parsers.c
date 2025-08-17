/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bhi2xy_parsers.h"
#include "bhy2_parse.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(bhi2xy, CONFIG_SENSOR_LOG_LEVEL);

void parse_3d_data(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref)
{
	int16_t *data_frame = (int16_t *)callback_ref;
	struct bhy2_data_xyz data;
	if (callback_info->data_size != 7) /* Check for a valid payload size. Includes sensor ID */
	{
		return;
	}

	bhy2_parse_xyz(callback_info->data_ptr, &data);

	data_frame[0] = data.x;
	data_frame[1] = data.y;
	data_frame[2] = data.z;
}

void parse_orientation(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref)
{
	int16_t *euler_data = (int16_t *)callback_ref;
	struct bhy2_data_orientation data;
	if (callback_info->data_size != 7) /* Check for a valid payload size. Includes sensor ID */
	{
		return;
	}

	bhy2_parse_orientation(callback_info->data_ptr, &data);

	euler_data[0] = data.heading;
	euler_data[1] = data.roll;
	euler_data[2] = data.pitch;
}

void parse_quaternion(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref)
{
	int16_t *quaternion_data = (int16_t *)callback_ref;
	struct bhy2_data_quaternion data;
	if (callback_info->data_size != 11) /* Check for a valid payload size. Includes sensor ID */
	{
		return;
	}

	bhy2_parse_quaternion(callback_info->data_ptr, &data);

	quaternion_data[0] = data.x;
	quaternion_data[1] = data.y;
	quaternion_data[2] = data.z;
	quaternion_data[3] = data.w;
}

void parse_pres(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref)
{
	uint32_t *pres_data = (uint32_t *)callback_ref;
	uint32_t pres = 0;
	if (callback_info->data_size != 4) /* Check for a valid payload size. Includes sensor ID */
	{
		return;
	}

	pres = BHY2_LE2U24(callback_info->data_ptr);
	*pres_data = pres;
}

void parse_step_count(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref)
{
	uint32_t *step_count_data = (uint32_t *)callback_ref;
	uint32_t step_count = 0;
	if (callback_info->data_size != 5) /* Check for a valid payload size. Includes sensor ID */
	{
		return;
	}

	step_count = BHY2_LE2U32(callback_info->data_ptr);
	*step_count_data = step_count;
}

void parse_meta_event(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref)
{
	uint8_t meta_event_type = callback_info->data_ptr[0];
	uint8_t byte1 = callback_info->data_ptr[1];
	uint8_t byte2 = callback_info->data_ptr[2];
	char *event_text;

	if (callback_info->sensor_id == BHY2_SYS_ID_META_EVENT) {
		event_text = "[META EVENT]";
	} else if (callback_info->sensor_id == BHY2_SYS_ID_META_EVENT_WU) {
		event_text = "[META EVENT WAKE UP]";
	} else {
		return;
	}

	switch (meta_event_type) {
	case BHY2_META_EVENT_FLUSH_COMPLETE:
		LOG_DBG("%s Flush complete for sensor id %u", event_text, byte1);
		break;
	case BHY2_META_EVENT_SAMPLE_RATE_CHANGED:
		LOG_DBG("%s Sample rate changed for sensor id %u", event_text, byte1);
		break;
	case BHY2_META_EVENT_POWER_MODE_CHANGED:
		LOG_DBG("%s Power mode changed for sensor id %u", event_text, byte1);
		break;
	case BHY2_META_EVENT_ALGORITHM_EVENTS:
		LOG_DBG("%s Algorithm event", event_text);
		break;
	case BHY2_META_EVENT_SENSOR_STATUS:
		LOG_DBG("%s Accuracy for sensor id %u changed to %u", event_text, byte1, byte2);
		break;
	case BHY2_META_EVENT_BSX_DO_STEPS_MAIN:
		LOG_DBG("%s BSX event (do steps main)", event_text);
		break;
	case BHY2_META_EVENT_BSX_DO_STEPS_CALIB:
		LOG_DBG("%s BSX event (do steps calib)", event_text);
		break;
	case BHY2_META_EVENT_BSX_GET_OUTPUT_SIGNAL:
		LOG_DBG("%s BSX event (get output signal)", event_text);
		break;
	case BHY2_META_EVENT_SENSOR_ERROR:
		LOG_INF("%s Sensor id %u reported error 0x%02X", event_text, byte1, byte2);
		break;
	case BHY2_META_EVENT_FIFO_OVERFLOW:
		LOG_DBG("%s FIFO overflow", event_text);
		break;
	case BHY2_META_EVENT_DYNAMIC_RANGE_CHANGED:
		LOG_DBG("%s Dynamic range changed for sensor id %u", event_text, byte1);
		break;
	case BHY2_META_EVENT_FIFO_WATERMARK:
		LOG_DBG("%s FIFO watermark reached", event_text);
		break;
	case BHY2_META_EVENT_INITIALIZED:
		LOG_DBG("%s Firmware initialized. Firmware version %u", event_text,
			((uint16_t)byte2 << 8) | byte1);
		break;
	case BHY2_META_TRANSFER_CAUSE:
		LOG_DBG("%s Transfer cause for sensor id %u", event_text, byte1);
		break;
	case BHY2_META_EVENT_SENSOR_FRAMEWORK:
		LOG_DBG("%s Sensor framework event for sensor id %u", event_text, byte1);
		break;
	case BHY2_META_EVENT_RESET:
		LOG_DBG("%s Reset event", event_text);
		break;
	case BHY2_META_EVENT_SPACER:
		break;
	default:
		LOG_DBG("%s Unknown meta event with id: %u", event_text, meta_event_type);
		break;
	}
}

void parse_debug_message(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref)
{
	LOG_WRN("[DEBUG MSG]; flag: 0x%x, data: %s", callback_info->data_ptr[0],
		(char *)&callback_info->data_ptr[1]);
}
