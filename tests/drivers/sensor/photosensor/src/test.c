/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc/adc_fake.h>
#include <zephyr/drivers/gpio/gpio_fake.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_SENSOR_NODE DT_NODELABEL(test_photosensor)
#define TEST_ADC_NODE DT_NODELABEL(test_adc)
#define TEST_THRESHOLD_MS (MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define TEST_SETTLE_TIME_NS DT_PROP_OR(DT_NODELABEL(test_photosensor), settle_time_ns, 0)
#define TEST_SETTLE_TIME_MS MAX(TEST_SETTLE_TIME_NS / NSEC_PER_MSEC, TEST_THRESHOLD_MS)
#define TEST_RX_BUF_SIZE 32
#define TEST_ADC_READ_TIMEOUT_MS TEST_THRESHOLD_MS
#define TEST_ADC_READ_TIMEOUT K_MSEC(TEST_ADC_READ_TIMEOUT_MS)
#define TEST_SENSOR_HAS_ENABLE_PIN DT_NODE_HAS_PROP(TEST_SENSOR_NODE, enable_gpios)

DEFINE_FFF_GLOBALS;

static uint16_t test_adc_read_values[] = DT_PROP(DT_PATH(zephyr_user), adc_read_values);
static uint32_t test_lux_values[] = DT_PROP(DT_PATH(zephyr_user), lux_values);
static size_t test_values_idx;

SENSOR_DT_READ_IODEV(
	test_iodev,
	TEST_SENSOR_NODE,
	{SENSOR_CHAN_AMBIENT_LIGHT, 0}
);

static const struct sensor_decoder_api *test_decoder =
	SENSOR_DECODER_DT_GET(TEST_SENSOR_NODE);

RTIO_DEFINE(test_rtio, 2, 1);

ZTEST_SUITE(sensor_photosensor, NULL, NULL, NULL, NULL, NULL);

#if CONFIG_DEVICE_DEINIT_SUPPORT
static const struct device *test_sensor_dev = DEVICE_DT_GET(TEST_SENSOR_NODE);
#endif

static const struct device *test_adc_dev = DEVICE_DT_GET(TEST_ADC_NODE);
static adc_sequence_callback test_adc_read_done_callback;
static const struct adc_sequence *test_adc_read_done_sequence;
static int64_t test_adc_read_fake_async_uptime_0;
static int64_t test_adc_read_fake_async_uptime_1;

static void test_adc_read_timer_handler(struct k_timer *timer)
{
	enum adc_action action;
	uint16_t *value;

	value = (uint16_t *)test_adc_read_done_sequence->buffer;
	*value = test_adc_read_values[test_values_idx];

	action = test_adc_read_done_callback(test_adc_dev, test_adc_read_done_sequence, 0);
	zassert_equal(action, ADC_ACTION_CONTINUE);
}

static K_TIMER_DEFINE(test_adc_read_timer, test_adc_read_timer_handler, NULL);

static int test_adc_read_fake_async_0(const struct device *dev,
				      const struct adc_sequence *sequence,
				      struct k_poll_signal *async)
{
	zassert_equal(dev, test_adc_dev);
	zassert_not_null(sequence);
	zassert_is_null(async);

	test_adc_read_done_callback = sequence->options->callback;
	test_adc_read_done_sequence = sequence;
	test_adc_read_fake_async_uptime_0 = k_uptime_get();
	k_timer_start(&test_adc_read_timer, TEST_ADC_READ_TIMEOUT, K_FOREVER);
	return 0;
}

static int test_adc_read_fake_async_1(const struct device *dev,
				      const struct adc_sequence *sequence,
				      struct k_poll_signal *async)
{
	zassert_equal(dev, test_adc_dev);
	zassert_not_null(sequence);
	zassert_is_null(async);

	test_adc_read_done_callback = sequence->options->callback;
	test_adc_read_done_sequence = sequence;
	test_adc_read_fake_async_uptime_1 = k_uptime_get();
	k_timer_start(&test_adc_read_timer, TEST_ADC_READ_TIMEOUT, K_FOREVER);
	return 0;
}

typedef int (*test_adc_read_fake_async)(const struct device *dev,
					const struct adc_sequence *sequence,
					struct k_poll_signal *async);

static test_adc_read_fake_async test_adc_read_fake_async_seq_0[] = {
	test_adc_read_fake_async_0,
	test_adc_read_fake_async_1,
};

#if TEST_SENSOR_HAS_ENABLE_PIN && CONFIG_PM_DEVICE_RUNTIME
static int64_t test_gpio_pin_configure_fake_0_uptime;

static int test_gpio_pin_configure_fake_0(const struct device *port,
					  gpio_pin_t pin,
					  gpio_flags_t flags)
{
	zassert_true(flags & GPIO_OUTPUT);
	test_gpio_pin_configure_fake_0_uptime = k_uptime_get();
	return 0;
}

static int64_t test_gpio_pin_configure_fake_1_uptime;

static int test_gpio_pin_configure_fake_1(const struct device *port,
					  gpio_pin_t pin,
					  gpio_flags_t flags)
{
	zassert_false(flags & GPIO_OUTPUT);
	test_gpio_pin_configure_fake_1_uptime = k_uptime_get();
	return 0;
}

typedef int (*test_gpio_pin_configure_fake)(const struct device *port,
					    gpio_pin_t pin,
					    gpio_flags_t flags);

static test_gpio_pin_configure_fake test_gpio_pin_configure_fake_seq_0[] = {
	test_gpio_pin_configure_fake_0,
	test_gpio_pin_configure_fake_1,
};
#endif

static uint32_t test_lux_q31_to_u(q31_t light)
{
	return light / (INT32_MAX >> 14);
}

static void test_decode_rx_buf(const uint8_t *rx_buf, struct sensor_q31_data *data)
{
	struct sensor_decode_context decoder_ctx =
		SENSOR_DECODE_CONTEXT_INIT(test_decoder, rx_buf, SENSOR_CHAN_AMBIENT_LIGHT, 0);

	zassert_equal(sensor_decode(&decoder_ctx, data, 1), 1);
	zassert_equal(data->shift, 14);
}

static void test_validate_lux(uint32_t actual, uint32_t expected)
{
	if (actual == 0) {
		zassert_equal(actual, expected);
	} else {
		zassert_within(actual, expected, 1);
	}
}

static void test_single_read_by_idx(size_t idx)
{
	int ret;
	uint8_t rx_buf[TEST_RX_BUF_SIZE];
	struct sensor_q31_data data;
	uint32_t lux;

#if TEST_SENSOR_HAS_ENABLE_PIN && CONFIG_PM_DEVICE_RUNTIME
	int64_t settle_time_ms;
#endif

	test_values_idx = idx;

	adc_fake_read_async_fake.custom_fake = test_adc_read_fake_async_0;

#if TEST_SENSOR_HAS_ENABLE_PIN && CONFIG_PM_DEVICE_RUNTIME
	gpio_fake_pin_configure_fake.custom_fake_seq = test_gpio_pin_configure_fake_seq_0;
	gpio_fake_pin_configure_fake.custom_fake_seq_len =
		ARRAY_SIZE(test_gpio_pin_configure_fake_seq_0);
#endif

	ret = sensor_read(&test_iodev, &test_rtio, rx_buf, sizeof(rx_buf));
	zassert_ok(ret);
	zassert_equal(adc_fake_read_async_fake.call_count, 1);

#if TEST_SENSOR_HAS_ENABLE_PIN && CONFIG_PM_DEVICE_RUNTIME
	zassert_equal(gpio_fake_pin_configure_fake.call_count, 2);

	settle_time_ms = test_adc_read_fake_async_uptime_0 -
			 test_gpio_pin_configure_fake_0_uptime;

	zassert_within(settle_time_ms, TEST_SETTLE_TIME_MS, TEST_THRESHOLD_MS);
#endif

	test_decode_rx_buf(rx_buf, &data);
	lux = test_lux_q31_to_u(data.readings[0].light);
	test_validate_lux(lux, test_lux_values[test_values_idx]);

	RESET_FAKE(adc_fake_read_async);
	RESET_FAKE(gpio_fake_pin_configure);
}

ZTEST(sensor_photosensor, test_single_read)
{
	ARRAY_FOR_EACH(test_adc_read_values, idx) {
		test_single_read_by_idx(idx);
	}
}

static void test_chained_read_by_idx(size_t idx)
{
	uint8_t rx_buf[2][TEST_RX_BUF_SIZE];
	struct sensor_q31_data data[2];
	int64_t read_interval_ms;
	uint32_t lux;
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;

#if TEST_SENSOR_HAS_ENABLE_PIN && CONFIG_PM_DEVICE_RUNTIME
	int64_t settle_time_ms;
#endif

	test_values_idx = idx;

	adc_fake_read_async_fake.custom_fake_seq = test_adc_read_fake_async_seq_0;
	adc_fake_read_async_fake.custom_fake_seq_len =
		ARRAY_SIZE(test_adc_read_fake_async_seq_0);

#if TEST_SENSOR_HAS_ENABLE_PIN && CONFIG_PM_DEVICE_RUNTIME
	gpio_fake_pin_configure_fake.custom_fake_seq = test_gpio_pin_configure_fake_seq_0;
	gpio_fake_pin_configure_fake.custom_fake_seq_len =
		ARRAY_SIZE(test_gpio_pin_configure_fake_seq_0);
#endif

	sqe = rtio_sqe_acquire(&test_rtio);
	zassert_not_null(sqe);

	rtio_sqe_prep_read(sqe,
			   &test_iodev,
			   0,
			   rx_buf[0],
			   sizeof(rx_buf[0]),
			   NULL);

	sqe->flags |= RTIO_SQE_CHAINED;
	sqe->flags |= RTIO_SQE_NO_RESPONSE;

	sqe = rtio_sqe_acquire(&test_rtio);
	zassert_not_null(sqe);

	rtio_sqe_prep_read(sqe,
			   &test_iodev,
			   0,
			   rx_buf[1],
			   sizeof(rx_buf[1]),
			   NULL);

	rtio_submit(&test_rtio, 1);

	cqe = rtio_cqe_consume(&test_rtio);
	zassert_not_null(cqe);
	zassert_ok(cqe->result);
	rtio_cqe_release(&test_rtio, cqe);

	zassert_equal(adc_fake_read_async_fake.call_count, 2);

#if TEST_SENSOR_HAS_ENABLE_PIN && CONFIG_PM_DEVICE_RUNTIME
	zassert_equal(gpio_fake_pin_configure_fake.call_count, 2);

	settle_time_ms = test_adc_read_fake_async_uptime_0 -
			 test_gpio_pin_configure_fake_0_uptime;

	zassert_within(settle_time_ms, TEST_SETTLE_TIME_MS, TEST_THRESHOLD_MS);
#endif

	read_interval_ms = test_adc_read_fake_async_uptime_1 -
			   test_adc_read_fake_async_uptime_0;

	zassert_within(read_interval_ms, TEST_ADC_READ_TIMEOUT_MS, TEST_THRESHOLD_MS);

	ARRAY_FOR_EACH(rx_buf, i) {
		test_decode_rx_buf(rx_buf[i], &data[i]);
		lux = test_lux_q31_to_u(data[i].readings[0].light);
		test_validate_lux(lux, test_lux_values[test_values_idx]);
	}

	zassert_true(data[0].header.base_timestamp_ns < data[1].header.base_timestamp_ns);

	RESET_FAKE(adc_fake_read_async);
	RESET_FAKE(gpio_fake_pin_configure);
}

ZTEST(sensor_photosensor, test_chained_read)
{
	ARRAY_FOR_EACH(test_adc_read_values, idx) {
		test_chained_read_by_idx(idx);
	}
}

#if CONFIG_DEVICE_DEINIT_SUPPORT
ZTEST(sensor_photosensor, test_deinit_init)
{
	zassert_ok(device_deinit(test_sensor_dev));
	zassert_ok(device_init(test_sensor_dev));
}
#endif
