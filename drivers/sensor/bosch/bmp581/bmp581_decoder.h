/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP581_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP581_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include "bmp581.h"

struct bmp581_encoded_header {
		uint8_t channels;
		uint8_t events;
		uint64_t timestamp;
		uint8_t press_en;
		uint8_t fifo_count;
};

struct bmp581_frame {
	uint8_t payload[6];
};

struct bmp581_encoded_data {
	struct bmp581_encoded_header header;
	union {
		uint8_t payload[6]; /* 3 bytes temp + 3 bytes press */
		struct bmp581_frame frame[0]; /* Used for FIFO frames */
	};
};

static inline enum bmp581_event bmp581_encode_events_bitmask(
					const struct sensor_stream_trigger *const triggers,
					size_t count)
{
	enum bmp581_event result = 0;

	for (size_t i = 0 ; i < count ; i++) {
		switch (triggers[i].trigger) {
		case SENSOR_TRIG_DATA_READY:
			result |= ((triggers[i].opt == SENSOR_STREAM_DATA_INCLUDE) ?
				   BMP581_EVENT_DRDY : 0);
			break;
		case SENSOR_TRIG_FIFO_WATERMARK:
			result |= ((triggers[i].opt == SENSOR_STREAM_DATA_INCLUDE) ?
				   BMP581_EVENT_FIFO_WM : 0);
			break;
		default:
			break;
		}
	}

	return result;
}

int bmp581_encode(const struct device *dev,
		   const struct sensor_read_config *read_config,
		   uint8_t trigger_status,
		   uint8_t *buf);

int bmp581_get_decoder(const struct device *dev,
		       const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP581_DECODER_H_ */
