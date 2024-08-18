/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/mbox.h>

#include <zephyr/linker/devicetree_regions.h>

#define TEST_SRAM1 DT_NODELABEL(test_sram1)
#define TEST_SRAM2 DT_NODELABEL(test_sram2)
#define TEST_TEMP  DT_NODELABEL(test_temp_sensor)

ZTEST(devicetree_api_ext, test_linker_regions)
{
	zassert_true(!strcmp(LINKER_DT_NODE_REGION_NAME(TEST_SRAM1), "SRAM_REGION"), "");
	zassert_true(!strcmp(LINKER_DT_NODE_REGION_NAME(TEST_SRAM2), "SRAM_REGION_2"), "");
}

#define DT_DRV_COMPAT vnd_adc_temp_sensor
ZTEST(devicetree_api_ext, test_adc_dt_spec)
{
	struct adc_dt_spec adc_spec;

	/* ADC_DT_SPEC_GET_BY_NAME */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME(TEST_TEMP, ch1);
	zassert_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME(TEST_TEMP, ch2);
	zassert_equal(adc_spec.channel_id, 20, "");

	/* ADC_DT_SPEC_INST_GET_BY_NAME */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME(0, ch1);
	zassert_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME(0, ch2);
	zassert_equal(adc_spec.channel_id, 20, "");
}

DEVICE_DT_DEFINE(DT_NODELABEL(test_mbox), NULL, NULL, NULL, NULL, POST_KERNEL, 90, NULL);
DEVICE_DT_DEFINE(DT_NODELABEL(test_mbox_zero_cell), NULL, NULL, NULL, NULL, POST_KERNEL, 90, NULL);
ZTEST(devicetree_api_ext, test_mbox_dt_spec)
{
	const struct mbox_dt_spec channel_tx = MBOX_DT_SPEC_GET(TEST_TEMP, tx);
	const struct mbox_dt_spec channel_rx = MBOX_DT_SPEC_GET(TEST_TEMP, rx);

	zassert_equal(channel_tx.channel_id, 1, "");
	zassert_equal(channel_rx.channel_id, 2, "");

	const struct mbox_dt_spec channel_zero = MBOX_DT_SPEC_GET(TEST_TEMP, zero);

	zassert_equal(channel_zero.channel_id, 0, "");
}

ZTEST_SUITE(devicetree_api_ext, NULL, NULL, NULL, NULL, NULL);
