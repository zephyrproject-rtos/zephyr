/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_SHIELD_MIKROE_ADC_CLICK)
#define ADC_DEVICE_NODE		DT_INST(0, microchip_mcp3204)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_EXTERNAL0
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0
#define ADC_2ND_CHANNEL_ID	1

#elif defined(CONFIG_BOARD_NRF51DK_NRF51422)

#include <hal/nrf_adc.h>
#define ADC_DEVICE_NODE		DT_INST(0, nordic_nrf_adc)
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1_3
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0
#define ADC_1ST_CHANNEL_INPUT	NRF_ADC_CONFIG_INPUT_2
#define ADC_2ND_CHANNEL_ID	2
#define ADC_2ND_CHANNEL_INPUT	NRF_ADC_CONFIG_INPUT_3

#elif defined(CONFIG_BOARD_NRF21540DK_NRF52840) || \
	defined(CONFIG_BOARD_NRF52DK_NRF52832) || \
	defined(CONFIG_BOARD_EBYTE_E73_TBB_NRF52832) || \
	defined(CONFIG_BOARD_NRF52840DK_NRF52840) || \
	defined(CONFIG_BOARD_RAK4631_NRF52840) || \
	defined(CONFIG_BOARD_RAK5010_NRF52840) || \
	defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840) || \
	defined(CONFIG_BOARD_NRF52840_BLIP) || \
	defined(CONFIG_BOARD_NRF52840_PAPYR) || \
	defined(CONFIG_BOARD_NRF52833DK_NRF52833) || \
	defined(CONFIG_BOARD_BL652_DVK) || \
	defined(CONFIG_BOARD_BL653_DVK) || \
	defined(CONFIG_BOARD_BL654_DVK) || \
	defined(CONFIG_BOARD_BL654_SENSOR_BOARD) || \
	defined(CONFIG_BOARD_DEGU_EVK) || \
	defined(CONFIG_BOARD_ADAFRUIT_FEATHER_NRF52840)	|| \
	defined(CONFIG_BOARD_ADAFRUIT_ITSYBITSY_NRF52840) || \
	defined(CONFIG_BOARD_RUUVI_RUUVITAG) || \
	defined(CONFIG_BOARD_BT510) || \
	defined(CONFIG_BOARD_PINNACLE_100_DVK) || \
	defined(CONFIG_BOARD_MG100) || \
	defined(CONFIG_BOARD_ARDUINO_NANO_33_BLE) || \
	defined(CONFIG_BOARD_ARDUINO_NANO_33_BLE_SENSE) || \
	defined(CONFIG_BOARD_UBX_BMD300EVAL_NRF52832) || \
	defined(CONFIG_BOARD_UBX_BMD330EVAL_NRF52810) || \
	defined(CONFIG_BOARD_UBX_BMD340EVAL_NRF52840) || \
	defined(CONFIG_BOARD_UBX_BMD345EVAL_NRF52840) || \
	defined(CONFIG_BOARD_UBX_BMD360EVAL_NRF52811) || \
	defined(CONFIG_BOARD_UBX_BMD380EVAL_NRF52840) || \
	defined(CONFIG_BOARD_UBX_EVKANNAB1_NRF52832) || \
	defined(CONFIG_BOARD_UBX_EVKNINAB1_NRF52832) || \
	defined(CONFIG_BOARD_UBX_EVKNINAB3_NRF52840) || \
	defined(CONFIG_BOARD_UBX_EVKNINAB4_NRF52833) || \
	defined(CONFIG_BOARD_WE_PROTEUS2EV_NRF52832) || \
	defined(CONFIG_BOARD_WE_PROTEUS3EV_NRF52840) || \
	defined(CONFIG_BOARD_BT610) || \
	defined(CONFIG_BOARD_PAN1780_EVB) || \
	defined(CONFIG_BOARD_PAN1781_EVB) || \
	defined(CONFIG_BOARD_PAN1782_EVB) || \
	defined(CONFIG_BOARD_PAN1770_EVB) || \
	defined(CONFIG_BOARD_XIAO_BLE)	|| \
	defined(CONFIG_BOARD_RAYTAC_MDBT50Q_DB_33_NRF52833) || \
	defined(CONFIG_BOARD_RAYTAC_MDBT50Q_DB_40_NRF52840)

#include <hal/nrf_saadc.h>
#define ADC_DEVICE_NODE		DT_INST(0, nordic_nrf_saadc)
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1_6
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID	0
#define ADC_1ST_CHANNEL_INPUT	NRF_SAADC_INPUT_AIN1
#define ADC_2ND_CHANNEL_ID	2
#define ADC_2ND_CHANNEL_INPUT	NRF_SAADC_INPUT_AIN2

#elif defined(CONFIG_BOARD_FRDM_K22F)

#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	14
#define ADC_1ST_CHANNEL_INPUT	0

#elif defined(CONFIG_BOARD_FRDM_K64F)

#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	14

#elif defined(CONFIG_BOARD_TLSR9518ADK80D)

#define ADC_DEVICE_NODE		DT_INST(0, telink_b91_adc)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1_4
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0
#define ADC_1ST_CHANNEL_INPUT	0x0f

#elif defined(CONFIG_BOARD_FRDM_K82F)

#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	15

#elif defined(CONFIG_BOARD_FRDM_KL25Z)

#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	12

#elif defined(CONFIG_BOARD_FRDM_KW41Z)

#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	3

#elif defined(CONFIG_BOARD_GD32A503V_EVAL)

#define ADC_DEVICE_NODE		DT_INST(0, gd_gd32_adc)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	1

#elif defined(CONFIG_BOARD_HEXIWEAR_K64)

#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	16

#elif defined(CONFIG_BOARD_FRDM_K82F)

#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	26

#elif defined(CONFIG_BOARD_HEXIWEAR_KW40Z)

#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	1

#elif defined(CONFIG_BOARD_SAM_E70_XPLAINED) || \
	defined(CONFIG_BOARD_SAM_V71_XULT)

#define ADC_DEVICE_NODE		DT_INST(0, atmel_sam_afec)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_EXTERNAL0
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0

#elif defined(CONFIG_BOARD_SAM4S_XPLAINED)

#define ADC_DEVICE_NODE		DT_INST(0, atmel_sam_adc)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_EXTERNAL0
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	15
#define ADC_1ST_CHANNEL_INPUT	15

#elif defined(CONFIG_SOC_FAMILY_SAM0)
#include <soc.h>
#define ADC_DEVICE_NODE         DT_INST(0, atmel_sam0_adc)
#define ADC_RESOLUTION          12
#define ADC_GAIN                ADC_GAIN_1
#define ADC_REFERENCE           ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME    ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID      0
#define ADC_1ST_CHANNEL_INPUT   ADC_INPUTCTRL_MUXPOS_SCALEDIOVCC_Val

#elif defined(CONFIG_BOARD_NUCLEO_F091RC) || \
	defined(CONFIG_BOARD_NUCLEO_F103RB) || \
	defined(CONFIG_BOARD_NUCLEO_F207ZG) || \
	defined(CONFIG_BOARD_STM32F3_DISCO) || \
	defined(CONFIG_BOARD_NUCLEO_F401RE) || \
	defined(CONFIG_BOARD_NUCLEO_F429ZI) || \
	defined(CONFIG_BOARD_NUCLEO_F746ZG) || \
	defined(CONFIG_BOARD_NUCLEO_G070RB) || \
	defined(CONFIG_BOARD_NUCLEO_G071RB) || \
	defined(CONFIG_BOARD_NUCLEO_L073RZ) || \
	defined(CONFIG_BOARD_NUCLEO_WL55JC) || \
	defined(CONFIG_BOARD_NUCLEO_L152RE) || \
	defined(CONFIG_BOARD_OLIMEX_STM32_H103) || \
	defined(CONFIG_BOARD_96B_AEROCORE2) || \
	defined(CONFIG_BOARD_STM32F103_MINI) || \
	defined(CONFIG_BOARD_STM32_MIN_DEV_BLUE) || \
	defined(CONFIG_BOARD_STM32_MIN_DEV_BLACK) || \
	defined(CONFIG_BOARD_WAVESHARE_OPEN103Z) || \
	defined(CONFIG_BOARD_RONOTH_LODEV) || \
	defined(CONFIG_BOARD_STM32L496G_DISCO) || \
	defined(CONFIG_BOARD_SWAN_R5)
#define ADC_DEVICE_NODE         DT_INST(0, st_stm32_adc)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0

#elif defined(CONFIG_BOARD_NUCLEO_F302R8) || \
	defined(CONFIG_BOARD_NUCLEO_G474RE) || \
	defined(CONFIG_BOARD_NUCLEO_WB55RG) || \
	defined(CONFIG_BOARD_STM32L562E_DK) || \
	defined(CONFIG_BOARD_STM32H573I_DK) || \
	defined(CONFIG_BOARD_NUCLEO_L552ZE_Q) || \
	defined(CONFIG_BOARD_NUCLEO_L412RB_P)
#define ADC_DEVICE_NODE         DT_INST(0, st_stm32_adc)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
/* Some F3 series SOCs do not have channel 0 connected to an external GPIO. */
#define ADC_1ST_CHANNEL_ID	1

#elif defined(CONFIG_BOARD_STM32H735G_DISCO)
#define ADC_DEVICE_NODE         DT_INST(0, st_stm32_adc)
#define ADC_RESOLUTION		16
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0

#elif defined(CONFIG_BOARD_NUCLEO_L476RG) || \
	defined(CONFIG_BOARD_BLACKPILL_F411CE) || \
	defined(CONFIG_BOARD_STM32F401_MINI) || \
	defined(CONFIG_BOARD_BLACKPILL_F401CE) || \
	defined(CONFIG_BOARD_BLACKPILL_F401CC) || \
	defined(CONFIG_BOARD_NUCLEO_L4R5ZI) || \
	defined(CONFIG_BOARD_MIKROE_CLICKER_2)
#define ADC_DEVICE_NODE         DT_INST(0, st_stm32_adc)
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	1

#elif defined(CONFIG_BOARD_DISCO_L475_IOT1)
#define ADC_DEVICE_NODE         DT_INST(0, st_stm32_adc)
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	5

#elif defined(CONFIG_BOARD_B_U585I_IOT02A) || \
	defined(CONFIG_BOARD_NUCLEO_H563ZI)
#define ADC_DEVICE_NODE         DT_INST(0, st_stm32_adc)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	15

#elif defined(CONFIG_BOARD_NUCLEO_H743ZI) || \
	defined(CONFIG_BOARD_NUCLEO_H753ZI) || \
	defined(CONFIG_BOARD_NUCLEO_H7A3ZI_Q)
#define ADC_DEVICE_NODE         DT_INST(0, st_stm32_adc)
#define ADC_RESOLUTION		16
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	12
#define ADC_2ND_CHANNEL_ID	15

#elif defined(CONFIG_BOARD_TWR_KE18F)
#define ADC_DEVICE_NODE		DT_INST(0, nxp_kinetis_adc12)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0
#define ADC_2ND_CHANNEL_ID	1

#endif

/* Invalid value that is not supposed to be written by the driver. It is used
 * to mark the sample buffer entries as empty. If needed, it can be overridden
 * for a particular board by providing a specific definition above.
 */
#if !defined(INVALID_ADC_VALUE)
#define INVALID_ADC_VALUE SHRT_MIN
#endif

#define BUFFER_SIZE  6
static ZTEST_BMEM int16_t m_sample_buffer[BUFFER_SIZE];

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};
static const int adc_channels_count = ARRAY_SIZE(adc_channels);
#else
#error "Unsupported board."
#endif

const struct device *get_adc_device(void)
{
	if (!device_is_ready(adc_channels[0].dev)) {
		printk("ADC device is not ready\n");
		return NULL;
	}

	return adc_channels[0].dev;
}

static void init_adc(void)
{
	int i, ret;

	zassert_true(device_is_ready(adc_channels[0].dev), "ADC device is not ready");

	for (int i = 0; i < adc_channels_count; i++) {
		ret = adc_channel_setup_dt(&adc_channels[i]);
		zassert_equal(ret, 0, "Setting up of channel %d failed with code %d", i, ret);
	}

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}
}

static void check_samples(int expected_count)
{
	int i;

	TC_PRINT("Samples read: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		int16_t sample_value = m_sample_buffer[i];

		TC_PRINT("0x%04x ", sample_value);
		if (i < expected_count) {
			zassert_not_equal(INVALID_ADC_VALUE, sample_value,
				"[%u] should be filled", i);
		} else {
			zassert_equal(INVALID_ADC_VALUE, sample_value,
				"[%u] should be empty", i);
		}
	}
	TC_PRINT("\n");
}

/*
 * test_adc_sample_one_channel
 */
static int test_task_one_channel(void)
{
	int ret;
	struct adc_sequence sequence = {
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	init_adc();
	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	ret = adc_read(adc_channels[0].dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1);

	return TC_PASS;
}

ZTEST_USER(adc_basic, test_adc_sample_one_channel)
{
	zassert_true(test_task_one_channel() == TC_PASS);
}

/*
 * test_adc_sample_multiple_channels
 */
static int test_task_multiple_channels(void)
{
	int ret;
	struct adc_sequence sequence = {
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	init_adc();
	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	for (int i = 1; i < adc_channels_count; i++) {
		sequence.channels |= BIT(adc_channels[i].channel_id);
	}

	ret = adc_read(adc_channels[0].dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(adc_channels_count);

	return TC_PASS;
}

ZTEST_USER(adc_basic, test_adc_sample_two_channels)
{
	if (adc_channels_count > 1) {
		zassert_true(test_task_multiple_channels() == TC_PASS);
	} else {
		ztest_test_skip();
	}
}

/*
 * test_adc_asynchronous_call
 */
#if defined(CONFIG_ADC_ASYNC)
struct k_poll_signal async_sig;

static int test_task_asynchronous_call(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.extra_samplings = 4,
		/* Start consecutive samplings as fast as possible. */
		.interval_us     = 0,
	};
	struct adc_sequence sequence = {
		.options     = &options,
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};
	struct k_poll_event  async_evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &async_sig);
	init_adc();

	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	ret = adc_read_async(adc_channels[0].dev, &sequence, &async_sig);
	zassert_equal(ret, 0, "adc_read_async() failed with code %d", ret);

	ret = k_poll(&async_evt, 1, K_MSEC(1000));
	zassert_equal(ret, 0, "k_poll failed with error %d", ret);

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}
#endif /* defined(CONFIG_ADC_ASYNC) */

ZTEST_USER(adc_basic, test_adc_asynchronous_call)
{
#if defined(CONFIG_ADC_ASYNC)
	zassert_true(test_task_asynchronous_call() == TC_PASS);
#else
	ztest_test_skip();
#endif /* defined(CONFIG_ADC_ASYNC) */
}

/*
 * test_adc_sample_with_interval
 */
static uint32_t my_sequence_identifier = 0x12345678;
static void *user_data = &my_sequence_identifier;

static enum adc_action sample_with_interval_callback(const struct device *dev,
						     const struct adc_sequence *sequence,
						     uint16_t sampling_index)
{
	if (sequence->options->user_data != &my_sequence_identifier) {
		user_data = sequence->options->user_data;
		return ADC_ACTION_FINISH;
	}

	TC_PRINT("%s: sampling %d\n", __func__, sampling_index);
	return ADC_ACTION_CONTINUE;
}

static int test_task_with_interval(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.interval_us     = 100 * 1000UL,
		.callback        = sample_with_interval_callback,
		.user_data       = user_data,
		.extra_samplings = 4,
	};
	struct adc_sequence sequence = {
		.options     = &options,
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	init_adc();

	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	ret = adc_read(adc_channels[0].dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	zassert_equal(user_data, sequence.options->user_data,
		"Invalid user data: %p, expected: %p",
		user_data, sequence.options->user_data);

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}

ZTEST(adc_basic, test_adc_sample_with_interval)
{
	zassert_true(test_task_with_interval() == TC_PASS);
}

/*
 * test_adc_repeated_samplings
 */
static uint8_t m_samplings_done;
static enum adc_action repeated_samplings_callback(const struct device *dev,
						   const struct adc_sequence *sequence,
						   uint16_t sampling_index)
{
	++m_samplings_done;
	TC_PRINT("%s: done %d\n", __func__, m_samplings_done);
	if (m_samplings_done == 1U) {
		check_samples(adc_channels_count);

		/* After first sampling continue normally. */
		return ADC_ACTION_CONTINUE;
	} else {
		check_samples(2 * adc_channels_count);

		/*
		 * The second sampling is repeated 9 times (the samples are
		 * written in the same place), then the sequence is finished
		 * prematurely.
		 */
		if (m_samplings_done < 10) {
			return ADC_ACTION_REPEAT;
		} else {
			return ADC_ACTION_FINISH;
		}
	}
}

static int test_task_repeated_samplings(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.callback        = repeated_samplings_callback,
		/*
		 * This specifies that 3 samplings are planned. However,
		 * the callback function above is constructed in such way
		 * that the first sampling is done normally, the second one
		 * is repeated 9 times, and then the sequence is finished.
		 * Hence, the third sampling will not take place.
		 */
		.extra_samplings = 2,
		/* Start consecutive samplings as fast as possible. */
		.interval_us     = 0,
	};
	struct adc_sequence sequence = {
		.options     = &options,
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	init_adc();
	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	if (adc_channels_count > 1) {
		sequence.channels |=  BIT(adc_channels[1].channel_id);
	}

	ret = adc_read(adc_channels[0].dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	return TC_PASS;
}

ZTEST(adc_basic, test_adc_repeated_samplings)
{
	zassert_true(test_task_repeated_samplings() == TC_PASS);
}

/*
 * test_adc_invalid_request
 */
static int test_task_invalid_request(void)
{
	int ret;
	struct adc_sequence sequence = {
		.channels    = BIT(adc_channels[0].channel_id),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = 0, /* intentionally invalid value */
	};

	init_adc();

	ret = adc_read(adc_channels[0].dev, &sequence);
	zassert_not_equal(ret, 0, "adc_read() unexpectedly succeeded");

#if defined(CONFIG_ADC_ASYNC)
	ret = adc_read_async(adc_channels[0].dev, &sequence, &async_sig);
	zassert_not_equal(ret, 0, "adc_read_async() unexpectedly succeeded");
#endif

	/*
	 * Make the sequence parameters valid, now the request should succeed.
	 */
	sequence.resolution = adc_channels[0].resolution;

	ret = adc_read(adc_channels[0].dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1);

	return TC_PASS;
}

ZTEST_USER(adc_basic, test_adc_invalid_request)
{
	zassert_true(test_task_invalid_request() == TC_PASS);
}
