/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_DRIVERS_ADC_ADC_API_SRC_ADC_TEST_H_
#define TESTS_DRIVERS_ADC_ADC_API_SRC_ADC_TEST_H_

#include <zephyr/drivers/adc.h>

#define ADC_TEST_COUNT_ONE(node_id) + 1

/*
 * Number of channels the board makes available to the tests: the io-channels
 * of the zephyr,user node when present, otherwise every channel node of an
 * enabled ADC controller.
 */
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#define ADC_TEST_CHANNEL_COUNT DT_PROP_LEN(DT_PATH(zephyr_user), io_channels)
#else
#define ADC_TEST_CHANNEL_COUNT (0 ADC_DT_FOREACH_CHANNEL_NODE(ADC_TEST_COUNT_ONE))
#endif

BUILD_ASSERT(ADC_TEST_CHANNEL_COUNT > 0, "No ADC channel described in devicetree");

/* The channels, the first one being the one the tests exercise. */
extern const struct adc_dt_spec adc_test_channels[ADC_TEST_CHANNEL_COUNT];

/* The controller of the first channel, or NULL when it is not ready. */
const struct device *get_adc_device(void);

#endif /* TESTS_DRIVERS_ADC_ADC_API_SRC_ADC_TEST_H_ */
