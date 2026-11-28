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
#include <zephyr/drivers/comparator/adc_comp.h>
#include <zephyr/ztest.h>

#define COMP_NODE      DT_NODELABEL(vph_pwr_comp)
#define DIFF_COMP_NODE DT_NODELABEL(diff_comp)

static const struct device *const comp_dev = DEVICE_DT_GET(COMP_NODE);
static const struct adc_dt_spec comp_adc = ADC_DT_SPEC_GET(COMP_NODE);
static const struct device *const diff_comp_dev = DEVICE_DT_GET(DIFF_COMP_NODE);
static const struct adc_dt_spec diff_comp_adc = ADC_DT_SPEC_GET(DIFF_COMP_NODE);

static int adc_error_func(const struct device *dev, unsigned int chan, void *data, uint32_t *result)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	ARG_UNUSED(data);
	ARG_UNUSED(result);

	return -EIO;
}

static void adc_comparator_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(comp_adc_comp_set_threshold(comp_dev, DT_PROP(COMP_NODE, threshold_mv)));
	zassert_ok(
		comp_adc_comp_set_threshold(diff_comp_dev, DT_PROP(DIFF_COMP_NODE, threshold_mv)));
}

ZTEST(adc_comparator, test_init)
{
	zassert_true(device_is_ready(comp_dev));
	zassert_true(adc_is_ready_dt(&comp_adc));
	zassert_true(device_is_ready(diff_comp_dev));
	zassert_true(adc_is_ready_dt(&diff_comp_adc));
}

ZTEST(adc_comparator, test_get_output_above_threshold)
{
	zassert_ok(adc_emul_const_value_set(comp_adc.dev, comp_adc.channel_id, 2500));
	zassert_equal(comparator_get_output(comp_dev), 1);
}

ZTEST(adc_comparator, test_get_output_at_threshold)
{
	/* Raw 768 on 10-bit 3200 mV channel -> exact 2400 mV == threshold */
	zassert_ok(adc_emul_const_raw_value_set(comp_adc.dev, comp_adc.channel_id, 768));
	zassert_equal(comparator_get_output(comp_dev), 0);

	/* Raw 769 -> 2403 mV > 2400 mV threshold */
	zassert_ok(adc_emul_const_raw_value_set(comp_adc.dev, comp_adc.channel_id, 769));
	zassert_equal(comparator_get_output(comp_dev), 1);
}

ZTEST(adc_comparator, test_get_output_below_threshold)
{
	zassert_ok(adc_emul_const_value_set(comp_adc.dev, comp_adc.channel_id, 1500));
	zassert_equal(comparator_get_output(comp_dev), 0);
}

ZTEST(adc_comparator, test_get_output_adc_error)
{
	zassert_ok(
		adc_emul_value_func_set(comp_adc.dev, comp_adc.channel_id, adc_error_func, NULL));
	zassert_equal(comparator_get_output(comp_dev), -EIO);
}

ZTEST(adc_comparator, test_set_threshold)
{
	zassert_ok(adc_emul_const_value_set(comp_adc.dev, comp_adc.channel_id, 2500));
	zassert_equal(comparator_get_output(comp_dev), 1);

	zassert_ok(comp_adc_comp_set_threshold(comp_dev, 2600));
	zassert_equal(comparator_get_output(comp_dev), 0);
}

ZTEST(adc_comparator, test_differential_channel)
{
	/* Raw -10240 on 16-bit diff 3200 mV channel -> -1000 mV <= -500 mV threshold */
	zassert_ok(adc_emul_const_raw_value_set(diff_comp_adc.dev, diff_comp_adc.channel_id,
						(uint16_t)((int16_t)-10240)));
	zassert_equal(comparator_get_output(diff_comp_dev), 0);

	/* Raw -5120 -> exact -500 mV == -500 mV threshold */
	zassert_ok(adc_emul_const_raw_value_set(diff_comp_adc.dev, diff_comp_adc.channel_id,
						(uint16_t)((int16_t)-5120)));
	zassert_equal(comparator_get_output(diff_comp_dev), 0);

	/* Raw -2048 -> -200 mV > -500 mV threshold */
	zassert_ok(adc_emul_const_raw_value_set(diff_comp_adc.dev, diff_comp_adc.channel_id,
						(uint16_t)((int16_t)-2048)));
	zassert_equal(comparator_get_output(diff_comp_dev), 1);
}

ZTEST(adc_comparator, test_triggers_unsupported)
{
	zassert_ok(comparator_set_trigger(comp_dev, COMPARATOR_TRIGGER_NONE));
	zassert_equal(comparator_set_trigger(comp_dev, COMPARATOR_TRIGGER_RISING_EDGE), -ENOTSUP);
	zassert_equal(comparator_set_trigger_callback(comp_dev, NULL, NULL), -ENOTSUP);
	zassert_equal(comparator_trigger_is_pending(comp_dev), 0);
}

ZTEST_SUITE(adc_comparator, NULL, NULL, adc_comparator_before, NULL, NULL);
