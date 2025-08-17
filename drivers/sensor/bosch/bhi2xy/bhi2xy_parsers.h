/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_PARSERS_H_
#define ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_PARSERS_H_

#include "bhy2.h"

void parse_3d_data(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref);
void parse_orientation(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref);
void parse_quaternion(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref);
void parse_pres(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref);
void parse_step_count(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref);

void parse_meta_event(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref);
void parse_debug_message(const struct bhy2_fifo_parse_data_info *callback_info, void *callback_ref);

#endif /* ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_PARSERS_H_ */
