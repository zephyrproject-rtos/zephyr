/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/ztest.h>

#define COMP_NODE DT_NODELABEL(vph_pwr_comp)

static const struct device *const comp_dev = DEVICE_DT_GET(COMP_NODE);
static const struct adc_dt_spec comp_adc = ADC_DT_SPEC_GET(COMP_NODE);

static int adc_error_func(const struct device *dev, unsigned int chan, void *data, uint32_t *result)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	ARG_UNUSED(data);
	ARG_UNUSED(result);

	return -EIO;
}

ZTEST(adc_comparator, test_init)
{
	zassert_true(device_is_ready(comp_dev));
	zassert_true(adc_is_ready_dt(&comp_adc));
}

ZTEST(adc_comparator, test_get_output_above_threshold)
{
	/* 2500 mV at ADC pin scales to ~3031 mV > 2400 mV threshold */
	zassert_ok(adc_emul_const_value_set(comp_adc.dev, comp_adc.channel_id, 2500));
	zassert_equal(comparator_get_output(comp_dev), 1);
}

ZTEST(adc_comparator, test_get_output_below_threshold)
{
	/* 1500 mV at ADC pin scales to ~1819 mV <= 2400 mV threshold */
	zassert_ok(adc_emul_const_value_set(comp_adc.dev, comp_adc.channel_id, 1500));
	zassert_equal(comparator_get_output(comp_dev), 0);
}

ZTEST(adc_comparator, test_get_output_adc_error)
{
	zassert_ok(
		adc_emul_value_func_set(comp_adc.dev, comp_adc.channel_id, adc_error_func, NULL));
	zassert_equal(comparator_get_output(comp_dev), -EIO);
}

ZTEST(adc_comparator, test_triggers_unsupported)
{
	zassert_ok(comparator_set_trigger(comp_dev, COMPARATOR_TRIGGER_NONE));
	zassert_equal(comparator_set_trigger(comp_dev, COMPARATOR_TRIGGER_RISING_EDGE), -ENOTSUP);
	zassert_equal(comparator_set_trigger_callback(comp_dev, NULL, NULL), -ENOTSUP);
	zassert_equal(comparator_trigger_is_pending(comp_dev), 0);
}

ZTEST_SUITE(adc_comparator, NULL, NULL, NULL, NULL, NULL);
