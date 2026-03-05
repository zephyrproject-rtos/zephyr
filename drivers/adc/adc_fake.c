/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc/adc_fake.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif

#define DT_DRV_COMPAT zephyr_adc_fake

DEFINE_FAKE_VALUE_FUNC(int,
		       adc_fake_channel_setup,
		       const struct device *,
		       const struct adc_channel_cfg *);

DEFINE_FAKE_VALUE_FUNC(int,
		       adc_fake_read,
		       const struct device *,
		       const struct adc_sequence *);

DEFINE_FAKE_VALUE_FUNC(int,
		       adc_fake_read_async,
		       const struct device *,
		       const struct adc_sequence *,
		       struct k_poll_signal *);

DEFINE_FAKE_VOID_FUNC(adc_fake_submit,
		      const struct device *,
		      struct rtio_iodev_sqe *);

DEFINE_FAKE_VALUE_FUNC(int,
		       adc_fake_get_decoder,
		       const struct device *,
		       const struct adc_decoder_api **);

static DEVICE_API(adc, adc_fake_api) = {
	.channel_setup = adc_fake_channel_setup,
	.read = adc_fake_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_fake_read_async,
#endif
#ifdef CONFIG_ADC_STREAM
	.submit = adc_fake_submit,
	.get_decoder = adc_fake_get_decoder,
#endif
	.ref_internal = CONFIG_ADC_FAKE_REF_INTERNAL,
};

#ifdef CONFIG_ZTEST
static void adc_fake_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(adc_fake_channel_setup);
	RESET_FAKE(adc_fake_read);
#ifdef CONFIG_ADC_ASYNC
	RESET_FAKE(adc_fake_read_async);
#endif
#ifdef CONFIG_ADC_STREAM
	RESET_FAKE(adc_fake_submit);
	RESET_FAKE(adc_fake_get_decoder);
#endif
}

ZTEST_RULE(adc_fake_reset_rule, adc_fake_reset_rule_before, NULL);
#endif

DEVICE_DT_INST_DEFINE(
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	CONFIG_ADC_INIT_PRIORITY,
	&adc_fake_api
);
