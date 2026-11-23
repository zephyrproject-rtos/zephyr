/*
 * Copyright (c) 2026, Iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "sen0466_emul.h"

#define SEN0466_NODE DT_NODELABEL(sen0466)

/* Frame layout, written out so the test does not follow the driver. */
#define FRAME_LEN    9U
#define BYTE_HEAD    0U
#define BYTE_CMD     1U
#define BYTE_CONC_HI 2U
#define BYTE_CONC_LO 3U
#define BYTE_GAS     4U
#define BYTE_DEC     5U
#define BYTE_TEMP_HI 6U
#define BYTE_TEMP_LO 7U
#define BYTE_SUM     8U

#define FRAME_HEAD   0xFFU
#define CMD_SET_MODE 0x78U
#define CMD_GET_DATA 0x88U
#define GAS_CO       0x04U

/* Converter readings the tests use, and what the driver makes of them. */
#define ADC_25C                 512U
#define ADC_25C_MILLI_C         25002
#define ADC_25C_MILLI_PPM       39475
#define ADC_NEXT                513U
#define ADC_NEXT_MILLI_C        24900
#define ADC_OVER_40C            376U
#define ADC_OVER_40C_MILLI_C    40043
#define ADC_UNDER_40C           377U
#define ADC_UNDER_40C_MILLI_C   39920
#define ADC_UNDER_40C_MILLI_PPM 32240
#define ADC_BELOW_TABLE         300U
#define ADC_ABOVE_TABLE         904U

/* Raw concentration every test starts from, 42 ppm with no decimal places. */
#define SAMPLE_CONC 42U

/*
 * The NTC curve at full precision, in milli-degrees, worked out from
 *
 *	v   = 3 * adc / 1024
 *	rth = v * 10000 / (3 - v)
 *	t   = 1 / (1 / 298.15 + ln(rth / 10000) / 3380.13) - 273.15
 *
 * at readings that mostly fall between two table entries, so what is
 * measured is the interpolation rather than the entries themselves.
 */
static const struct {
	uint16_t adc;
	int32_t milli_c;
} ntc_reference[] = {
	{301, 49976},  {332, 45653},  {363, 41642},  {394, 37877}, {425, 34307}, {456, 30890},
	{487, 27593},  {518, 24385},  {549, 21240},  {580, 18135}, {611, 15044}, {642, 11944},
	{673, 8810},   {704, 5613},   {735, 2319},   {766, -1112}, {797, -4735}, {828, -8621},
	{859, -12876}, {890, -17668}, {903, -19899},
};

/* How far off the table is allowed to be, as the driver documents it. */
#define NTC_TOLERANCE_MILLI_C 32

static const struct device *dev = DEVICE_DT_GET(SEN0466_NODE);
static const struct emul *emul = EMUL_DT_GET(SEN0466_NODE);

static uint8_t frame_checksum(const uint8_t *frame)
{
	uint8_t sum = 0U;

	/* Bytes 1 to 6; the last byte of the reading is not covered. */
	for (uint8_t i = BYTE_CMD; i <= BYTE_TEMP_HI; i++) {
		sum += frame[i];
	}

	return (uint8_t)(~sum + 1U);
}

static void build_data_frame(uint8_t *frame, uint16_t concentration, uint8_t gas_type,
			     uint8_t decimals, uint16_t adc)
{
	frame[BYTE_HEAD] = FRAME_HEAD;
	frame[BYTE_CMD] = CMD_GET_DATA;
	frame[BYTE_CONC_HI] = (uint8_t)(concentration >> 8);
	frame[BYTE_CONC_LO] = (uint8_t)concentration;
	frame[BYTE_GAS] = gas_type;
	frame[BYTE_DEC] = decimals;
	frame[BYTE_TEMP_HI] = (uint8_t)(adc >> 8);
	frame[BYTE_TEMP_LO] = (uint8_t)adc;
	frame[BYTE_SUM] = frame_checksum(frame);
}

static int read_milli(enum sensor_channel chan, int32_t *milli)
{
	struct sensor_value val;
	int ret;

	ret = sensor_channel_get(dev, chan, &val);
	if (ret != 0) {
		return ret;
	}

	*milli = (val.val1 * 1000) + (val.val2 / 1000);

	return 0;
}

static void sen0466_before(void *fixture)
{
	ARG_UNUSED(fixture);

	sen0466_emul_set_frame(emul, NULL);
	sen0466_emul_set_next_frame(emul, NULL);
	sen0466_emul_set_ack(emul, 1U);
	sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO, 0U, ADC_25C);
}

ZTEST_SUITE(sen0466, NULL, NULL, sen0466_before, NULL, NULL);

/* Reaching ready means init got an answer to its mode change. */
ZTEST(sen0466, test_device_is_ready)
{
	zassert_true(device_is_ready(dev), "Device is not ready");
}

ZTEST(sen0466, test_fetch_co_and_temperature)
{
	int32_t milli;

	zassert_ok(sensor_sample_fetch(dev));

	zassert_ok(read_milli(SENSOR_CHAN_CO, &milli));
	zassert_equal(milli, ADC_25C_MILLI_PPM, "Got %d milli-ppm", milli);

	zassert_ok(read_milli(SENSOR_CHAN_AMBIENT_TEMP, &milli));
	zassert_equal(milli, ADC_25C_MILLI_C, "Got %d milli-degrees", milli);
}

ZTEST(sen0466, test_fetch_one_channel)
{
	zassert_ok(sensor_sample_fetch_chan(dev, SENSOR_CHAN_CO));
	zassert_ok(sensor_sample_fetch_chan(dev, SENSOR_CHAN_AMBIENT_TEMP));
}

ZTEST(sen0466, test_checksum_leaves_out_the_last_byte)
{
	uint8_t frame[FRAME_LEN];
	int32_t milli;

	build_data_frame(frame, SAMPLE_CONC, GAS_CO, 0U, ADC_25C);
	sen0466_emul_set_frame(emul, frame);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(read_milli(SENSOR_CHAN_AMBIENT_TEMP, &milli));
	zassert_equal(milli, ADC_25C_MILLI_C, "Got %d milli-degrees", milli);

	/*
	 * Raise the low byte of the reading and leave the checksum as it was.
	 * The frame has to stay valid and the new reading has to come through.
	 */
	frame[BYTE_TEMP_LO]++;
	sen0466_emul_set_frame(emul, frame);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(read_milli(SENSOR_CHAN_AMBIENT_TEMP, &milli));
	zassert_equal(milli, ADC_NEXT_MILLI_C, "Got %d milli-degrees", milli);
}

ZTEST(sen0466, test_bad_checksum_is_rejected)
{
	uint8_t frame[FRAME_LEN];

	build_data_frame(frame, SAMPLE_CONC, GAS_CO, 0U, ADC_25C);
	frame[BYTE_SUM]++;
	sen0466_emul_set_frame(emul, frame);

	zassert_equal(sensor_sample_fetch(dev), -EIO);
}

ZTEST(sen0466, test_bad_head_is_rejected)
{
	uint8_t frame[FRAME_LEN];

	build_data_frame(frame, SAMPLE_CONC, GAS_CO, 0U, ADC_25C);
	frame[BYTE_HEAD] = (uint8_t)(FRAME_HEAD - 1U);
	sen0466_emul_set_frame(emul, frame);

	zassert_equal(sensor_sample_fetch(dev), -EIO);
}

ZTEST(sen0466, test_wrong_gas_type_is_rejected)
{
	sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO + 1U, 0U, ADC_25C);

	zassert_equal(sensor_sample_fetch(dev), -EIO);
}

ZTEST(sen0466, test_too_many_decimals_is_rejected)
{
	sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO, 3U, ADC_25C);

	zassert_equal(sensor_sample_fetch(dev), -EIO);
}

/* Every read answers with the wrong command, so the driver runs out of tries. */
ZTEST(sen0466, test_wrong_command_gives_up)
{
	uint8_t frame[FRAME_LEN];

	build_data_frame(frame, SAMPLE_CONC, GAS_CO, 0U, ADC_25C);
	frame[BYTE_CMD] = CMD_SET_MODE;
	frame[BYTE_SUM] = frame_checksum(frame);
	sen0466_emul_set_frame(emul, frame);

	zassert_equal(sensor_sample_fetch(dev), -EAGAIN);
}

/* One frame the part sent on its own, then the answer. */
ZTEST(sen0466, test_stale_frame_is_read_past)
{
	uint8_t frame[FRAME_LEN];
	int32_t milli;

	build_data_frame(frame, 9999U, GAS_CO, 0U, ADC_25C);
	frame[BYTE_CMD] = CMD_SET_MODE;
	frame[BYTE_SUM] = frame_checksum(frame);
	sen0466_emul_set_next_frame(emul, frame);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(read_milli(SENSOR_CHAN_CO, &milli));
	zassert_equal(milli, ADC_25C_MILLI_PPM, "Got %d milli-ppm", milli);
}

/* The same concentration written with 0, 1 and 2 decimal places. */
ZTEST(sen0466, test_decimal_places_scale_the_reading)
{
	static const uint16_t concentration[] = {42U, 420U, 4200U};
	int32_t milli;

	for (size_t i = 0U; i < ARRAY_SIZE(concentration); i++) {
		sen0466_emul_set_sample(emul, concentration[i], GAS_CO, (uint8_t)i, ADC_25C);

		zassert_ok(sensor_sample_fetch(dev));
		zassert_ok(read_milli(SENSOR_CHAN_CO, &milli));
		zassert_equal(milli, ADC_25C_MILLI_PPM, "%u decimals gave %d milli-ppm",
			      (unsigned int)i, milli);
	}
}

ZTEST(sen0466, test_no_co_above_the_compensated_range)
{
	struct sensor_value val;
	int32_t milli;

	/* Just over 40 degrees: no CO value, but the temperature still reads. */
	sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO, 0U, ADC_OVER_40C);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_CO, &val), -ERANGE);
	zassert_ok(read_milli(SENSOR_CHAN_AMBIENT_TEMP, &milli));
	zassert_equal(milli, ADC_OVER_40C_MILLI_C, "Got %d milli-degrees", milli);

	/* One converter count further and both values are there again. */
	sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO, 0U, ADC_UNDER_40C);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(read_milli(SENSOR_CHAN_CO, &milli));
	zassert_equal(milli, ADC_UNDER_40C_MILLI_PPM, "Got %d milli-ppm", milli);
	zassert_ok(read_milli(SENSOR_CHAN_AMBIENT_TEMP, &milli));
	zassert_equal(milli, ADC_UNDER_40C_MILLI_C, "Got %d milli-degrees", milli);
}

ZTEST(sen0466, test_no_values_outside_the_table)
{
	static const uint16_t adc[] = {ADC_BELOW_TABLE, ADC_ABOVE_TABLE};
	struct sensor_value val;

	for (size_t i = 0U; i < ARRAY_SIZE(adc); i++) {
		sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO, 0U, adc[i]);

		/* The transfer worked, so the fetch did too. */
		zassert_ok(sensor_sample_fetch(dev));
		zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_CO, &val), -ERANGE,
			      "Reading %u gave a CO value", adc[i]);
		zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val), -ERANGE,
			      "Reading %u gave a temperature", adc[i]);
	}
}

ZTEST(sen0466, test_values_return_after_one_out_of_range)
{
	struct sensor_value val;
	int32_t milli;

	sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO, 0U, ADC_ABOVE_TABLE);
	zassert_ok(sensor_sample_fetch(dev));
	zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_CO, &val), -ERANGE);

	sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO, 0U, ADC_25C);
	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(read_milli(SENSOR_CHAN_CO, &milli));
	zassert_equal(milli, ADC_25C_MILLI_PPM, "Got %d milli-ppm", milli);
}

ZTEST(sen0466, test_ntc_table_follows_the_curve)
{
	int32_t milli;

	for (size_t i = 0U; i < ARRAY_SIZE(ntc_reference); i++) {
		sen0466_emul_set_sample(emul, SAMPLE_CONC, GAS_CO, 0U, ntc_reference[i].adc);

		zassert_ok(sensor_sample_fetch(dev));
		zassert_ok(read_milli(SENSOR_CHAN_AMBIENT_TEMP, &milli));
		zassert_within(milli, ntc_reference[i].milli_c, NTC_TOLERANCE_MILLI_C,
			       "Reading %u gave %d milli-degrees", ntc_reference[i].adc, milli);
	}
}

ZTEST(sen0466, test_unsupported_channel)
{
	struct sensor_value val;

	zassert_equal(sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS), -ENOTSUP);
	zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_PRESS, &val), -ENOTSUP);
}
