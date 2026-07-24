/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ADC_ADC_COMMON_H_
#define ZEPHYR_DRIVERS_ADC_ADC_COMMON_H_

#include <zephyr/drivers/adc.h>

#include <stddef.h>

/**
 * This function will validate the buffer size is large enough to hold
 * the requested samples.
 */
int adc_sequence_validate_buffer(const struct adc_sequence *sequence, size_t data_size);

#endif /* ZEPHYR_DRIVERS_ADC_ADC_COMMON_H_ */
