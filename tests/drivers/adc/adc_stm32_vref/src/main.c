/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/ztest.h>

#define VREF_OWNER_ENTRY(vref_n)                                                                   \
	IF_ENABLED(DT_NODE_HAS_PROP(vref_n, io_channels),                                          \
		   ({                                                                              \
			    .dev = DEVICE_DT_GET_OR_NULL(DT_IO_CHANNELS_CTLR(vref_n)),             \
			    .channel = DT_IO_CHANNELS_INPUT(vref_n),                               \
		    }, ))

#define ADC_DEV_ENTRY(n) DEVICE_DT_GET(n),

#define ADC_DT_VREF_MV_ENTRY(n) DT_PROP(n, vref_mv),

struct vref_owner {
	const struct device *dev;
	uint8_t channel;
};

static const struct vref_owner owners[] = {
	DT_FOREACH_STATUS_OKAY(st_stm32_vref, VREF_OWNER_ENTRY)
};

static const struct device *const adcs[] = {
	DT_FOREACH_STATUS_OKAY(st_stm32_adc, ADC_DEV_ENTRY)
};

#ifndef CONFIG_ADC_STM32_VREFINT_CALIBRATE
static const uint16_t adc_dt_vref_mv[] = {
	DT_FOREACH_STATUS_OKAY(st_stm32_adc, ADC_DT_VREF_MV_ENTRY)
};
#endif

static void assert_owner_ready(const struct vref_owner *owner)
{
	zassert_not_null(owner->dev, "VREFINT ADC from io-channels is not instantiated");
	zassert_true(device_is_ready(owner->dev), "VREFINT-capable ADC not ready");
}

ZTEST(adc_stm32_vref, test_ref_internal_after_boot)
{
	uint16_t mv;
	uint16_t first = 0;
	int ret;

	zassert_true(ARRAY_SIZE(owners) > 0, "no okay st,stm32-vref node with io-channels");

	for (size_t i = 0; i < ARRAY_SIZE(owners); i++) {
		assert_owner_ready(&owners[i]);

		ret = adc_ref_get(owners[i].dev, ADC_REF_INTERNAL, &mv);
		zassert_ok(ret, "adc_ref_get(INTERNAL) failed on %s: %d", owners[i].dev->name,
			   ret);
		zassert_true(mv > 0, "expected positive INTERNAL mV on %s", owners[i].dev->name);
		zassert_equal(adc_ref_internal(owners[i].dev), mv, "adc_ref_internal mismatch");

		ret = adc_ref_get(owners[i].dev, ADC_REF_EXTERNAL0, &mv);
		zassert_equal(ret, -ENOTSUP, "EXTERNAL0 should be -ENOTSUP, got %d", ret);

		if (i == 0) {
			first = adc_ref_internal(owners[i].dev);
		} else {
			zassert_equal(adc_ref_internal(owners[i].dev), first,
				      "owners must share one cached rail");
		}
	}
}

ZTEST(adc_stm32_vref, test_calibrate_refresh_does_not_fail)
{
	int16_t buf;
	int ret;

	zassert_true(ARRAY_SIZE(owners) > 0, "no okay st,stm32-vref node with io-channels");

	for (size_t i = 0; i < ARRAY_SIZE(owners); i++) {
		struct adc_sequence seq = {
			.channels = BIT(owners[i].channel),
			.buffer = &buf,
			.buffer_size = sizeof(buf),
			.resolution = 12,
			.calibrate = true,
		};

		assert_owner_ready(&owners[i]);

		ret = adc_read(owners[i].dev, &seq);
		if (ret == -EINVAL || ret == -ENOTSUP) {
			ztest_test_skip();
		}
		zassert_ok(ret, "calibrate sequence failed on %s: %d", owners[i].dev->name, ret);
		zassert_true(adc_ref_internal(owners[i].dev) > 0, "ref after calibrate");
	}

	if (ARRAY_SIZE(owners) < 2) {
		return;
	}

	zassert_equal(adc_ref_internal(owners[0].dev), adc_ref_internal(owners[1].dev),
		      "calibrate on one owner must keep a shared cache");
}

#ifndef CONFIG_ADC_STM32_VREFINT_CALIBRATE
ZTEST(adc_stm32_vref, test_ref_internal_matches_dt_vref_mv)
{
	zassert_equal(ARRAY_SIZE(adcs), ARRAY_SIZE(adc_dt_vref_mv));

	for (size_t i = 0; i < ARRAY_SIZE(adcs); i++) {
		zassert_true(device_is_ready(adcs[i]), "ADC not ready");
		zassert_equal(adc_ref_internal(adcs[i]), adc_dt_vref_mv[i],
			      "adc_ref_internal should match DT vref-mv on %s", adcs[i]->name);
	}
}
#endif /* CONFIG_ADC_STM32_VREFINT_CALIBRATE */

ZTEST(adc_stm32_vref, test_ref_internal_shared_across_adc_instances)
{
	uint16_t expected;

	zassert_true(ARRAY_SIZE(adcs) > 0, "no okay STM32 ADC");
	zassert_true(device_is_ready(adcs[0]), "ADC not ready");

	if (ARRAY_SIZE(adcs) < 2) {
		ztest_test_skip();
	}

	expected = adc_ref_internal(adcs[0]);
	for (size_t i = 1; i < ARRAY_SIZE(adcs); i++) {
		zassert_true(device_is_ready(adcs[i]), "ADC not ready");
		zassert_equal(adc_ref_internal(adcs[i]), expected,
			      "INTERNAL ref should be shared across ADC instances");
	}
}

ZTEST_SUITE(adc_stm32_vref, NULL, NULL, NULL, NULL, NULL);
