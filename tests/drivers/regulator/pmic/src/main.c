/*
 * Copyright 2021 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Based on regulator-fixed test and adc driver sample, with are
 * copyright Peter Bigot Consulting, LLC and Libre Solar Technologies GmbH,
 * respectively.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/regulator/consumer.h>
#include <zephyr/ztest.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define ADC_NODE		DT_PHANDLE(DT_PATH(zephyr_user), io_channels)

/* Common settings supported by most ADCs */
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT

static int16_t sample_buffer[1];

#define CHANNEL_ID DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 0)

struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = CHANNEL_ID,
	.differential = 0
};

struct adc_sequence sequence = {
	/* individual channels will be added below */
	.channels    = BIT(CHANNEL_ID),
	.buffer      = sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(sample_buffer),
	.resolution  = ADC_RESOLUTION,
};

static struct onoff_client cli;
static struct onoff_manager *callback_srv;
static struct onoff_client *callback_cli;
static uint32_t callback_state;
static int callback_res;
static onoff_client_callback callback_fn;

static void callback(struct onoff_manager *srv,
		     struct onoff_client *cli,
		     uint32_t state,
		     int res)
{
	onoff_client_callback cb = callback_fn;

	callback_srv = srv;
	callback_cli = cli;
	callback_state = state;
	callback_res = res;
	callback_fn = NULL;

	if (cb != NULL) {
		cb(srv, cli, state, res);
	}
}

static void reset_callback(void)
{
	callback_srv = NULL;
	callback_cli = NULL;
	callback_state = INT_MIN;
	callback_res = 0;
	callback_fn = NULL;
}

static void reset_client(void)
{
	cli = (struct onoff_client){};
	reset_callback();
	sys_notify_init_callback(&cli.notify, callback);
}

/* Returns voltage level of ADC in mV, or negative value on error */
static int adc_get_reading(const struct device *adc_dev)
{
	int rc, adc_vref, mv_value;

	adc_vref = adc_ref_internal(adc_dev);

	rc = adc_read(adc_dev, &sequence);
	if (rc) {
		return rc;
	}
	mv_value = sample_buffer[0];
	if (adc_vref > 0) {
		adc_raw_to_millivolts(adc_vref, ADC_GAIN, ADC_RESOLUTION, &mv_value);
	}
	TC_PRINT("ADC read %d mV\n", mv_value);
	return mv_value;
}

ZTEST(regulator_pmic, test_basic)
{
	const struct device *reg_dev, *adc_dev;
	int rc, adc_reading;

	adc_dev = DEVICE_DT_GET(ADC_NODE);
	reg_dev = DEVICE_DT_GET(DT_NODELABEL(test_regulator));

	zassert_true(device_is_ready(adc_dev), "ADC device is not ready");
	zassert_true(device_is_ready(reg_dev), "Regulator device is not ready");

	/* Configure ADC */
	adc_channel_setup(adc_dev, &channel_cfg);

	reset_client();

	/* Turn regulator on */
	rc = regulator_enable(reg_dev, &cli);
	zassert_true(rc >= 0,
		     "first enable failed: %d", rc);

	/* Wait for regulator to start */
	while (sys_notify_fetch_result(&cli.notify, &rc) == -EAGAIN) {
		k_yield();
	}
	rc = sys_notify_fetch_result(&cli.notify, &rc);
	zassert_true(rc == 0, "Could not fetch regulator enable result");

	zassert_equal(callback_cli, &cli,
		      "callback not invoked");
	zassert_equal(callback_res, 0,
		      "callback res: %d", callback_res);
	zassert_equal(callback_state, ONOFF_STATE_ON,
		      "callback state: 0x%x", callback_res);

	/* Read adc to ensure regulator actually booted */
	adc_reading = adc_get_reading(adc_dev);
	zassert_true(adc_reading > 200, /* Assume regulator provides at least 200mV */
		      "Regulator did not supply power, ADC read %d mV",
			  adc_reading);

	/* Turn it on again (another client) */

	reset_client();
	rc = regulator_enable(reg_dev, &cli);
	zassert_true(rc >= 0,
		     "second enable failed: %d", rc);

	zassert_equal(callback_cli, &cli,
		      "callback not invoked");
	zassert_true(callback_res >= 0,
		      "callback res: %d", callback_res);
	zassert_equal(callback_state, ONOFF_STATE_ON,
		      "callback state: 0x%x", callback_res);

	/* Make sure it's still on */

	adc_reading = adc_get_reading(adc_dev);
	zassert_true(adc_reading >= 200,
		      "Second on attempt failed, ADC read %d mV", adc_reading);

	/* Turn it off once (still has a client) */

	rc = regulator_disable(reg_dev);
	zassert_true(rc >= 0,
		     "first disable failed: %d", rc);

	/* Make sure it's still on */

	adc_reading = adc_get_reading(adc_dev);
	zassert_true(adc_reading >= 200,
		      "Regulator still has client, but ADC read %d mV", adc_reading);

	/* Turn it off again (no more clients) */

	rc = regulator_disable(reg_dev);
	zassert_true(rc >= 0,
		     "second disable failed: %d", rc);

	/* Verify the regulator is off */
	adc_reading = adc_get_reading(adc_dev);
	zassert_true(adc_reading <= 200,
		      "Regulator is on with no clients, ADC read %d mV", adc_reading);
}

ZTEST_SUITE(regulator_pmic, NULL, NULL, NULL, NULL, NULL);
