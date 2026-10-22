/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ADC_ADC_COMMON_H_
#define ZEPHYR_DRIVERS_ADC_ADC_COMMON_H_

#include <zephyr/drivers/adc.h>

#include <stddef.h>

/**
 * This function will validate the buffer size is large enough to hold
 * the requested samples. Returns -ENOMEM if the sequence buffer size is too
 * small, otherwise 0.
 */
int adc_sequence_validate_buffer(const struct adc_sequence *sequence,
				 uint8_t active_channels, size_t data_size);

#endif /* ZEPHYR_DRIVERS_ADC_ADC_COMMON_H_ */
