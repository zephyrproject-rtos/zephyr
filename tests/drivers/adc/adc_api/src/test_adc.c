/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup test_adc_basic_operations
 * @{
 * @defgroup t_adc_basic_basic_operations test_adc_sample
 * @brief TestPurpose: verify ADC works well with different resolutions
 *			and sample mode
 * @details
 * - Test Steps
 *   -# Connect A0 to VCC3.3.
 *   -# Prepare ADC sequence table.
 *   -# Bind ADC device.
 *   -# Enable ADC device.
 *   -# Call adc_read() to fetch ADC sample.
 *   -# Dump the sample results.
 * - Expected Results
 *   -# ADC will return the sample result for VCC3.3. Different resolutions
 *      will all return almost the biggest value in each sample width.
 * @}
 */

#include <adc.h>
#include <zephyr.h>
#include <ztest.h>

#define ADC_DEV_NAME CONFIG_ADC_0_NAME
#define BUFFER_SIZE 5

static uint16_t seq_buffer[BUFFER_SIZE];

static struct adc_seq_entry entry = {
	.sampling_delay = 30,
	.channel_id = 10,
	.buffer = (void *)seq_buffer,
	.buffer_length = BUFFER_SIZE * sizeof(seq_buffer[0])
};

static struct adc_seq_table table = {
	.entries = &entry,
	.num_entries = 1,
};

static int test_task(void)
{
	int i;
	int ret;
	struct device *adc_dev = device_get_binding(ADC_DEV_NAME);

	if (!adc_dev) {
		TC_PRINT("Cannot get ADC device\n");
		return TC_FAIL;
	}

	/* 1. Verify adc_enable() */
	adc_enable(adc_dev);

	k_sleep(500);

	/* 2. Verify adc_read() */
	ret = adc_read(adc_dev, &table);
	if (ret != 0) {
		TC_PRINT("Failed to fetch sample data from ADC controller\n");
		return TC_FAIL;
	}

	TC_PRINT("Channel 10 ADC Sample: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		TC_PRINT("%d ", seq_buffer[i]);
	}
	TC_PRINT("\n");

	/* 3. Verify adc_disable() */
	adc_disable(adc_dev);

	return TC_PASS;
}

void test_adc_sample(void)
{
	assert_true(test_task() == TC_PASS, NULL);
}
