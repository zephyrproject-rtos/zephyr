//
// Created by peress on 13/07/23.
//

#ifndef ZEPHYR_INCLUDE_ZEPHYR_SENSING_TRANSFORM_H
#define ZEPHYR_INCLUDE_ZEPHYR_SENSING_TRANSFORM_H

#include <zephyr/sensing/sensing.h>

int decode_three_axis_data(int32_t type, struct sensing_sensor_three_axis_data *out,
				  const struct sensor_decoder_api *decoder, void *data);

int decode_float_data(int32_t type, struct sensing_sensor_float_data *out,
			     const struct sensor_decoder_api *decoder, void *data);

#endif // ZEPHYR_INCLUDE_ZEPHYR_SENSING_TRANSFORM_H
