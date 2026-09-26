/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

#include "adc_test.h"

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),
#define CHANNEL_NODE_SPEC_AND_COMMA(node_id) ADC_DT_SPEC_FROM_CHANNEL_NODE(node_id),

const struct adc_dt_spec adc_test_channels[ADC_TEST_CHANNEL_COUNT] = {
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
#else
	ADC_DT_FOREACH_CHANNEL_NODE(CHANNEL_NODE_SPEC_AND_COMMA)
#endif
};

const struct device *get_adc_device(void)
{
	if (!adc_is_ready_dt(&adc_test_channels[0])) {
		printk("ADC device is not ready\n");
		return NULL;
	}

	return adc_test_channels[0].dev;
}
