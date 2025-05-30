/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AFBR_S50_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_AFBR_S50_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include <api/argus_res.h>

struct afbr_s50_edata {
	struct {
		uint64_t timestamp;
		uint8_t channels : 2;
		uint8_t events : 1;
	} header;
	argus_results_t payload;
};

uint8_t afbr_s50_encode_channel(uint16_t chan);
uint8_t afbr_s50_encode_event(enum sensor_trigger_type trigger);

int afbr_s50_get_decoder(const struct device *dev,
			 const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_AFBR_S50_DECODER_H_ */
