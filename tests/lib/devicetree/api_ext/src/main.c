/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/hwspinlock.h>

#include <zephyr/linker/devicetree_regions.h>

#define TEST_SRAM1 DT_NODELABEL(test_sram1)
#define TEST_SRAM2 DT_NODELABEL(test_sram2)
#define TEST_TEMP  DT_NODELABEL(test_temp_sensor)
#define TEST_MISSING DT_NODELABEL(test_non_existing)

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
	zexpect_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME(TEST_TEMP, ch2);
	zexpect_equal(adc_spec.channel_id, 20, "");

	/* ADC_DT_SPEC_INST_GET_BY_NAME */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME(0, ch1);
	zexpect_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME(0, ch2);
	zexpect_equal(adc_spec.channel_id, 20, "");

	/* ADC_DT_SPEC_GET_BY_IDX */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_IDX(TEST_TEMP, 0);
	zexpect_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_IDX(TEST_TEMP, 1);
	zexpect_equal(adc_spec.channel_id, 20, "");

	/* ADC_DT_SPEC_INST_GET_BY_IDX */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_IDX(0, 0);
	zexpect_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_IDX(0, 1);
	zexpect_equal(adc_spec.channel_id, 20, "");

	/* ADC_DT_SPEC_GET_BY_NAME_OR */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME_OR(TEST_TEMP, ch1, {0});
	zexpect_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME_OR(TEST_TEMP, ch2, {0});
	zexpect_equal(adc_spec.channel_id, 20, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME_OR(TEST_TEMP, ch_missing, {0});
	zexpect_equal(adc_spec.channel_id, 0, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME_OR(TEST_MISSING, ch1, {0});
	zexpect_equal(adc_spec.channel_id, 0, "");

	/* ADC_DT_SPEC_INST_GET_BY_NAME_OR */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME_OR(0, ch1, {0});
	zexpect_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME_OR(0, ch2, {0});
	zexpect_equal(adc_spec.channel_id, 20, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME_OR(0, ch_missing, {0});
	zexpect_equal(adc_spec.channel_id, 0, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME_OR(100, ch1, {0});
	zexpect_equal(adc_spec.channel_id, 0, "");

	/* ADC_DT_SPEC_GET_BY_IDX_OR */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_IDX_OR(TEST_TEMP, 0, {0});
	zexpect_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_IDX_OR(TEST_TEMP, 1, {0});
	zexpect_equal(adc_spec.channel_id, 20, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_IDX_OR(TEST_TEMP, 100, {0});
	zexpect_equal(adc_spec.channel_id, 0, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_IDX_OR(TEST_MISSING, 0, {0});
	zexpect_equal(adc_spec.channel_id, 0, "");

	/* ADC_DT_SPEC_INST_GET_BY_IDX_OR */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_IDX_OR(0, 0, {0});
	zexpect_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_IDX_OR(0, 1, {0});
	zexpect_equal(adc_spec.channel_id, 20, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_IDX_OR(0, 100, {0});
	zexpect_equal(adc_spec.channel_id, 0, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_IDX_OR(100, 0, {0});
	zexpect_equal(adc_spec.channel_id, 0, "");
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

#define TEST_HWSPINLOCK \
	DT_NODELABEL(test_hwspinlock)

#define TEST_HWSPINLOCK_DEV \
	DT_NODELABEL(test_hwspinlock_dev)

#define HWSPINLOCK_BY_IDX(node_id, prop, idx) \
	HWSPINLOCK_DT_SPEC_GET_BY_IDX(node_id, idx)

static const struct hwspinlock_dt_spec spec[] = {
	DT_FOREACH_PROP_ELEM_SEP(TEST_HWSPINLOCK_DEV, hwlocks, HWSPINLOCK_BY_IDX, (,))
};

static const struct hwspinlock_dt_spec rd =
	HWSPINLOCK_DT_SPEC_GET_BY_NAME(TEST_HWSPINLOCK_DEV, rd);

static const struct hwspinlock_dt_spec wr =
	HWSPINLOCK_DT_SPEC_GET_BY_NAME(TEST_HWSPINLOCK_DEV, wr);

ZTEST(devicetree_api_ext, test_hwspinlock_dt_spec)
{
	for (int i = 0; i < ARRAY_SIZE(spec); i++) {
		zassert_equal(spec[i].dev, DEVICE_DT_GET(TEST_HWSPINLOCK));
		zassert_equal(spec[i].id, i + 1);
	}

	zassert_equal(rd.dev, DEVICE_DT_GET(TEST_HWSPINLOCK));
	zassert_equal(rd.id, 1);

	zassert_equal(wr.dev, DEVICE_DT_GET(TEST_HWSPINLOCK));
	zassert_equal(wr.id, 2);
}

ZTEST_SUITE(devicetree_api_ext, NULL, NULL, NULL, NULL, NULL);
