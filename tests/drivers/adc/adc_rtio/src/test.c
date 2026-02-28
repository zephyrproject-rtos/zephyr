/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/rtio.h>

static void test_submit_driver_api(const struct device *adc, struct rtio_iodev_sqe *iodev_sqe)
{

}

static DEVICE_API(adc, test_driver_api) = {
	.submit = test_submit_driver_api,
};

DEVICE_DT_DEFINE(
	DT_NODELABEL(test_adc0),
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	0,
	&test_driver_api
);

DEVICE_DT_DEFINE(
	DT_NODELABEL(test_adc1),
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	0,
	&test_driver_api
);

static const struct device *test_adc0_dev = DEVICE_DT_GET(DT_PATH(test_adc0));
static const struct device *test_adc1_dev = DEVICE_DT_GET(DT_PATH(test_adc1));

ADC_DT_IODEV_DEFINE(test_iodev_0, DT_PHANDLE_BY_IDX(DT_PATH(zephyr_user), io_channels, 0));
ADC_DT_IODEV_DEFINE(test_iodev_1, DT_PHANDLE_BY_IDX(DT_PATH(zephyr_user), io_channels, 1));
ADC_DT_CHAN_IODEV_DEFINE(test_chan_iodev, DT_PATH(zephyr_user));
ADC_DT_CHAN_IODEV_DEFINE_BY_IDX(test_chan_iodev_0, DT_PATH(zephyr_user), 0);
ADC_DT_CHAN_IODEV_DEFINE_BY_IDX(test_chan_iodev_1, DT_PATH(zephyr_user), 1);
ADC_DT_CHAN_IODEV_DEFINE_BY_NAME(test_chan_iodev_chan0, DT_PATH(zephyr_user), chan0);
ADC_DT_CHAN_IODEV_DEFINE_BY_NAME(test_chan_iodev_chan1, DT_PATH(zephyr_user), chan1);

RTIO_DEFINE(test_r, 1, 1);

ZTEST_SUITE(adc_rtio, NULL, NULL, NULL, NULL, NULL);

static void test_adc_dt_spec_equal_0(const struct adc_dt_spec *spec)
{
	zassert_equal(spec->dev, test_adc0_dev);
	zassert_false(spec->channel_cfg_dt_node_exists);
}

static void test_adc_dt_spec_equal_1(const struct adc_dt_spec *spec)
{
	zassert_equal(spec->dev, test_adc1_dev);
	zassert_false(spec->channel_cfg_dt_node_exists);
}

static void test_adc_dt_spec_equal_chan_0(const struct adc_dt_spec *spec)
{
	zassert_equal(spec->dev, test_adc0_dev);
	zassert_equal(spec->channel_id, 0);
}

static void test_adc_dt_spec_equal_chan_1(const struct adc_dt_spec *spec)
{
	zassert_equal(spec->dev, test_adc1_dev);
	zassert_equal(spec->channel_id, 1);
}

ZTEST(adc_rtio, test_adc_dt_iodev_define)
{
	zassert_equal(test_iodev_0.api, &adc_rtio_iodev_api);
	zassert_equal(test_iodev_1.api, &adc_rtio_iodev_api);
	zassert_equal(test_chan_iodev.api, &adc_rtio_iodev_api);
	zassert_equal(test_chan_iodev_0.api, &adc_rtio_iodev_api);
	zassert_equal(test_chan_iodev_1.api, &adc_rtio_iodev_api);
	zassert_equal(test_chan_iodev_chan0.api, &adc_rtio_iodev_api);
	zassert_equal(test_chan_iodev_chan1.api, &adc_rtio_iodev_api);

	test_adc_dt_spec_equal_0(test_iodev_0.data);
	test_adc_dt_spec_equal_1(test_iodev_1.data);
	test_adc_dt_spec_equal_chan_0(test_chan_iodev.data);
	test_adc_dt_spec_equal_chan_0(test_chan_iodev_0.data);
	test_adc_dt_spec_equal_chan_1(test_chan_iodev_1.data);
	test_adc_dt_spec_equal_chan_0(test_chan_iodev_chan0.data);
	test_adc_dt_spec_equal_chan_1(test_chan_iodev_chan1.data);
}

ZTEST(adc_rtio, test_adc_is_ready_iodev)
{
	zassert_true(adc_is_ready_iodev(&test_chan_iodev));
}
