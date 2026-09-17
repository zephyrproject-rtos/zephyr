/*
 * Copyright (c) 2023 Michal Morsisko
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_DECODER_H_

#include <stdint.h>

#include <zephyr/drivers/sensor.h>

#include "tmag5170.h"

/**
 * @brief Translate a sensor channel into a mask of enum tmag5170_result_idx bits.
 *
 * @param chan Channel to translate
 *
 * @return Mask of the result registers backing @p chan, 0 if unsupported
 */
uint8_t tmag5170_encode_channel(enum sensor_channel chan);

/**
 * @brief Fill in the header of an encoded data buffer.
 *
 * @param dev TMAG5170 instance
 * @param channels Requested channels
 * @param num_channels Number of requested channels
 * @param buf Buffer holding a struct tmag5170_encoded_data
 *
 * @retval 0 on success, negative errno otherwise
 */
int tmag5170_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		    const size_t num_channels, uint8_t *buf);

/**
 * @brief Get the decoder API of the TMAG5170.
 */
int tmag5170_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_DECODER_H_ */
