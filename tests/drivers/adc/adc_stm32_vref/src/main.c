/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/ztest.h>

#define VREF_NODE       DT_INST(0, st_stm32_vref)
#define VREF_ADC_NODE   DT_IO_CHANNELS_CTLR(VREF_NODE)
#define VREFINT_CHANNEL DT_IO_CHANNELS_INPUT(VREF_NODE)

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(adc1)) && !DT_SAME_NODE(DT_NODELABEL(adc1), VREF_ADC_NODE)
#define OTHER_ADC_NODE DT_NODELABEL(adc1)
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(adc2)) &&                                               \
	!DT_SAME_NODE(DT_NODELABEL(adc2), VREF_ADC_NODE)
#define OTHER_ADC_NODE DT_NODELABEL(adc2)
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(adc3)) &&                                               \
	!DT_SAME_NODE(DT_NODELABEL(adc3), VREF_ADC_NODE)
#define OTHER_ADC_NODE DT_NODELABEL(adc3)
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(adc4)) &&                                               \
	!DT_SAME_NODE(DT_NODELABEL(adc4), VREF_ADC_NODE)
#define OTHER_ADC_NODE DT_NODELABEL(adc4)
#endif

ZTEST(adc_stm32_vref, test_ref_internal_after_boot)
{
	const struct device *adc = DEVICE_DT_GET(VREF_ADC_NODE);
	uint16_t mv;
	int ret;

	zassert_true(device_is_ready(adc), "VREFINT-owning ADC not ready");

	ret = adc_ref_get(adc, ADC_REF_INTERNAL, &mv);
	zassert_ok(ret, "adc_ref_get(INTERNAL) failed: %d", ret);
	zassert_true(mv > 0, "expected positive INTERNAL mV");
	zassert_equal(adc_ref_internal(adc), mv, "adc_ref_internal mismatch");

	ret = adc_ref_get(adc, ADC_REF_EXTERNAL0, &mv);
	zassert_equal(ret, -ENOTSUP, "EXTERNAL0 should be -ENOTSUP, got %d", ret);
}

ZTEST(adc_stm32_vref, test_calibrate_refresh_does_not_fail)
{
	const struct device *adc = DEVICE_DT_GET(VREF_ADC_NODE);
	int16_t buf;
	struct adc_sequence seq = {
		.channels = BIT(VREFINT_CHANNEL),
		.buffer = &buf,
		.buffer_size = sizeof(buf),
		.resolution = 12,
		.calibrate = true,
	};
	int ret;

	zassert_true(device_is_ready(adc), "VREFINT-owning ADC not ready");

	/* sequence.calibrate on the owner should complete and leave
	 * adc_ref_internal() usable. Skip if the sequence is rejected
	 * (pinmux / channel setup) rather than fail the suite.
	 */
	ret = adc_read(adc, &seq);
	if (ret == -EINVAL || ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret, "calibrate sequence failed: %d", ret);
	zassert_true(adc_ref_internal(adc) > 0, "ref after calibrate");
}

#ifndef CONFIG_ADC_STM32_VREFINT_CALIBRATE
ZTEST(adc_stm32_vref, test_ref_internal_matches_dt_vref_mv)
{
	const struct device *adc = DEVICE_DT_GET(VREF_ADC_NODE);
	uint16_t expected = DT_PROP(VREF_ADC_NODE, vref_mv);

	zassert_true(device_is_ready(adc), "VREFINT-owning ADC not ready");
	zassert_equal(adc_ref_internal(adc), expected, "adc_ref_internal should match DT vref-mv");
}
#endif /* CONFIG_ADC_STM32_VREFINT_CALIBRATE */

#ifdef OTHER_ADC_NODE
ZTEST(adc_stm32_vref, test_ref_internal_shared_across_adc_instances)
{
	const struct device *owner = DEVICE_DT_GET(VREF_ADC_NODE);
	const struct device *other = DEVICE_DT_GET(OTHER_ADC_NODE);

	zassert_true(device_is_ready(owner), "VREFINT-owning ADC not ready");
	zassert_true(device_is_ready(other), "second ADC not ready");
	zassert_equal(adc_ref_internal(owner), adc_ref_internal(other),
		      "INTERNAL ref should be shared across ADC instances");
}
#endif /* OTHER_ADC_NODE */

ZTEST_SUITE(adc_stm32_vref, NULL, NULL, NULL, NULL, NULL);
