/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/input/input_mchp_touch_ptc_datastreamer.h>
#include <zephyr/input/input_mchp_touch_api.h>

#define DATASTREAMER_DATA_BUFFER_SIZE 19
#define DATASTREAMER_FRAME_START      0x55u
#define DATASTREAMER_FRAME_END        (uint8_t)~DATASTREAMER_FRAME_START

#define TOUCH_DETECTED     0x01u
#define TOUCH_NOT_DETECTED 0x00u

LOG_MODULE_REGISTER(input_mchp_touch_ptc_datastreamer, CONFIG_INPUT_LOG_LEVEL);

static const struct device *uart_dev;

static const uint8_t datastreamer_data[DATASTREAMER_DATA_BUFFER_SIZE] = {
	0x5F, 0xB4, 0x00, 0x86, 0x4A, 0x03, 0xEB, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xAA, 0x55, 0x01, 0x6E, 0xA0};

/* private function */
static inline void datastreamer_transmit(uint8_t data_byte)
{
	uart_poll_out(uart_dev, data_byte);
}

/* public function definitions */
int datastreamer_init(const struct device *dev)
{
	if (!device_is_ready(dev)) {
		uart_dev = NULL;
		LOG_ERR("Touch::Failed to open datastreamer port");
		return -ENODEV;
	}

	uart_dev = dev;

	return 0;
}

void datastreamer_output(const struct device *dev)
{
	int16_t temp_int_calc;
	static uint8_t sequence;
	uint16_t u16temp_output;
	uint8_t u8temp_output, send_header;
	uint16_t count_bytes_out;
	uint8_t max_iteration;

	if (uart_dev == NULL) {
		LOG_ERR("Touch::Initialize the datastreamer first");
		return;
	}

	send_header = sequence & 0x0fu;
	if (send_header == 0u) {
		for (count_bytes_out = 0u; count_bytes_out < DATASTREAMER_DATA_BUFFER_SIZE;
		     count_bytes_out++) {
			datastreamer_transmit(datastreamer_data[count_bytes_out]);
		}
	}

	max_iteration = get_def_no_of_sensors(dev);

	/* Start token */
	datastreamer_transmit(DATASTREAMER_FRAME_START);

	/* Frame Start */
	datastreamer_transmit(sequence);

	for (count_bytes_out = 0u; count_bytes_out < (uint16_t)max_iteration; count_bytes_out++) {
		/* Signals */
		u16temp_output = get_sensor_node_signal(dev, count_bytes_out);
		datastreamer_transmit((uint8_t)u16temp_output);
		datastreamer_transmit((uint8_t)(u16temp_output >> 8u));

		/* Reference */
		u16temp_output = get_sensor_node_reference(dev, count_bytes_out);
		datastreamer_transmit((uint8_t)u16temp_output);
		datastreamer_transmit((uint8_t)(u16temp_output >> 8u));

		/* Touch delta */
		temp_int_calc = (int16_t)get_sensor_node_signal(dev, count_bytes_out);
		temp_int_calc -= (int16_t)get_sensor_node_reference(dev, count_bytes_out);
		u16temp_output = (uint16_t)(temp_int_calc);
		datastreamer_transmit((uint8_t)u16temp_output);
		datastreamer_transmit((uint8_t)(u16temp_output >> 8u));

		/* Comp Caps */
		u16temp_output = get_sensor_cc_val(dev, count_bytes_out);
		datastreamer_transmit((uint8_t)u16temp_output);
		datastreamer_transmit((uint8_t)(u16temp_output >> 8u));

#if (ACQ_MODULE_AUTOTUNE_OUTPUT == 1)
#if (DEF_PTC_CAL_OPTION == CAL_AUTO_TUNE_CSD)
		/* CSD */
		u8temp_output = get_sensor_csd_val(dev, count_bytes_out);
		datastreamer_transmit(u8temp_output);
#else
		/* Prescalar */
		u8temp_output = get_sensor_prescaler_val(dev, count_bytes_out);
		datastreamer_transmit(u8temp_output);
#endif /* DEF_PTC_CAL_OPTION == CAL_AUTO_TUNE_CSD */
#endif /* (ACQ_MODULE_AUTOTUNE_OUTPUT == 1) */
		/* State */
		u8temp_output = get_sensor_state(dev, count_bytes_out);
		if (0u != (u8temp_output & KEY_TOUCHED_MASK)) {
			datastreamer_transmit(TOUCH_DETECTED);
		} else {
			datastreamer_transmit(TOUCH_NOT_DETECTED);
		}

		/* Threshold */
		u8temp_output = get_sensor_threshold_val(dev, count_bytes_out);
		datastreamer_transmit(u8temp_output);
	}

#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER)
	max_iteration = get_def_no_scrollers(dev);
	for (count_bytes_out = 0u; count_bytes_out < max_iteration; count_bytes_out++) {

		/* State */
		u8temp_output = get_scroller_state(dev, count_bytes_out);
		if (0u != (u8temp_output & TOUCH_DETECTED)) {
			datastreamer_transmit(TOUCH_DETECTED);
		} else {
			datastreamer_transmit(TOUCH_NOT_DETECTED);
		}

		/* Delta */
		u16temp_output = get_scroller_delta(dev, count_bytes_out);
		datastreamer_transmit((uint8_t)u16temp_output);
		datastreamer_transmit((uint8_t)(u16temp_output >> 8u));

		/* Threshold */
		u16temp_output = get_scroller_threshold_val(dev, count_bytes_out);
		datastreamer_transmit((uint8_t)u16temp_output);
		datastreamer_transmit((uint8_t)(u16temp_output >> 8u));

		/* filtered position */
		u16temp_output = get_scroller_position(dev, count_bytes_out);
		datastreamer_transmit((uint8_t)(u16temp_output & 0x00FFu));
		datastreamer_transmit((uint8_t)((u16temp_output & 0xFF00u) >> 8u));
	}
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_SCROLLER */

#if defined(CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE)
	max_iteration = get_def_no_frequencies(dev);

	/* Frequency selection - from acq module */
	u8temp_output = get_current_frequency(dev);
	datastreamer_transmit(u8temp_output);

	for (count_bytes_out = 0u; count_bytes_out < max_iteration; count_bytes_out++) {
		/* Frequencies */
		u8temp_output = get_frequency_by_index(dev, count_bytes_out);
		datastreamer_transmit(u8temp_output);
	}
#endif /* CONFIG_INPUT_MCHP_TOUCH_EN_FREQ_AUTO_TUNE */

	/* Other Debug Parameters */
	u8temp_output = get_error_code(dev);
	datastreamer_transmit(u8temp_output);

	/* Frame End */
	datastreamer_transmit(sequence++);

	/* End token */
	datastreamer_transmit(DATASTREAMER_FRAME_END);
}
