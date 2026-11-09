/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ADT7604 example — uses the sensor read/decode API (SENSOR_ASYNC_API).
 *
 * Two separate reads are issued per loop iteration:
 *   1. Temperature channels  (SENSOR_CHAN_AMBIENT_TEMP) — RTD and thermistor types
 *   2. Resistance channels   (SENSOR_CHAN_RESISTANCE)   — copper-trace types
 *
 * The decoder returns one reading per channel instance. The print functions
 * iterate chan_idx until get_frame_count returns an error to discover all instances.
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/dsp/print_format.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/sensor/adt7604.h>

LOG_MODULE_REGISTER(adt7604, LOG_LEVEL_INF);

/* One RTIO context is shared between both reads (sequential, not concurrent). */
RTIO_DEFINE(adt7604_ctx, 1, 1);

SENSOR_DT_READ_IODEV(temp_iodev, DT_NODELABEL(adt7604), {SENSOR_CHAN_AMBIENT_TEMP, 0},
		     {SENSOR_CHAN_AMBIENT_TEMP, 1});

SENSOR_DT_READ_IODEV(res_iodev, DT_NODELABEL(adt7604), {SENSOR_CHAN_RESISTANCE, 0},
		     {SENSOR_CHAN_RESISTANCE, 1}, {SENSOR_CHAN_RESISTANCE, 2},
		     {SENSOR_CHAN_RESISTANCE, 3}, {SENSOR_CHAN_RESISTANCE, 4});

/*
 * The encoded buffer must fit the full adt7604_encoded_data struct.
 * 256 bytes is generous; the driver's sizeof(adt7604_encoded_data) with
 * ADT7604_MAX_CHANNELS=20 channels is ~165 bytes.
 */
#define READ_BUF_SIZE 256

static void print_temp_readings(const struct sensor_decoder_api *decoder, const uint8_t *buf)
{
	bool found = false;

	for (uint16_t idx = 0;; idx++) {
		struct sensor_chan_spec chan = {SENSOR_CHAN_AMBIENT_TEMP, idx};
		uint16_t count = 0;

		if (decoder->get_frame_count(buf, chan, &count) < 0 || count == 0) {
			break;
		}

		struct sensor_q31_data out = {0};
		uint32_t fit = 0;
		int decoded = decoder->decode(buf, chan, &fit, 1, &out);

		if (decoded > 0) {
			printf("  temp[%d]: %" PRIq(6) " °C\n", idx,
			       PRIq_arg(out.readings[0].temperature, 6, out.shift));
			found = true;
		}
	}

	if (!found) {
		LOG_INF("No temperature readings in buffer");
	}
}

static void print_res_readings(const struct sensor_decoder_api *decoder, const uint8_t *buf)
{
	bool found = false;

	for (uint16_t idx = 0;; idx++) {
		struct sensor_chan_spec chan = {SENSOR_CHAN_RESISTANCE, idx};
		uint16_t count = 0;

		if (decoder->get_frame_count(buf, chan, &count) < 0 || count == 0) {
			break;
		}

		struct sensor_q31_data out = {0};
		uint32_t fit = 0;
		int decoded = decoder->decode(buf, chan, &fit, 1, &out);

		if (decoded > 0) {
			printf("  resistance[%d]: %" PRIq(6) " Ohm\n", idx,
			       PRIq_arg(out.readings[0].resistance, 6, out.shift));
			found = true;
		}
	}

	if (!found) {
		LOG_INF("No resistance readings in buffer");
	}
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(adt7604));
	const struct sensor_decoder_api *decoder;
	int rc;

	if (!device_is_ready(dev)) {
		LOG_ERR("ADT7604 device not ready");
		return -ENODEV;
	}

	/* Example copper trace sensor data table
	 * This is for a copper trace ≥1Ω sensor
	 * Format: resistance (mΩ) and temperature (Kelvin)
	 */
	struct adt7604_custom_sensor_data copper_trace_rows[] = {
		{.copper_trace = {.resistance_mohm = 0, .temp_kelvin = 0}},
		{.copper_trace = {.resistance_mohm = 51937, .temp_kelvin = 51200}},
		{.copper_trace = {.resistance_mohm = 454410, .temp_kelvin = 102400}},
		{.copper_trace = {.resistance_mohm = 856801, .temp_kelvin = 153600}},
		{.copper_trace = {.resistance_mohm = 1243136, .temp_kelvin = 202752}},
		{.copper_trace = {.resistance_mohm = 1645609, .temp_kelvin = 253952}},
		{.copper_trace = {.resistance_mohm = 2048000, .temp_kelvin = 305152}},
		{.copper_trace = {.resistance_mohm = 2466529, .temp_kelvin = 358400}},
		{.copper_trace = {.resistance_mohm = 2869002, .temp_kelvin = 409600}},
		{.copper_trace = {.resistance_mohm = 3456532, .temp_kelvin = 484352}},
	};

	struct adt7604_custom_sensor_data_table copper_trace_tbl = {
		.data = copper_trace_rows,
		.num_rows = ARRAY_SIZE(copper_trace_rows),
		.custom_addr_ptr = 0, /* Start at offset 0 from 0x250 */
	};

	/* Write copper trace custom sensor table (channel 8) */
	rc = adt7604_write_custom_sensor_data_table(dev, 8, &copper_trace_tbl);
	if (rc < 0) {
		printf("Failed to write copper trace custom data: %d\n", rc);
		return rc;
	}

	rc = sensor_get_decoder(dev, &decoder);

	if (rc < 0) {
		LOG_ERR("Failed to get decoder: %d", rc);
		return rc;
	}

	LOG_INF("ADT7604 sample started");

	uint8_t buf[READ_BUF_SIZE];

	while (true) {
		/* --- Temperature channels --- */
		rc = sensor_read(&temp_iodev, &adt7604_ctx, buf, sizeof(buf));
		if (rc < 0) {
			LOG_ERR("sensor_read (temp) failed: %d", rc);
		} else {
			printf("Temperature channels:\n");
			print_temp_readings(decoder, buf);
		}

		/* --- Resistance (copper-trace) channels --- */
		rc = sensor_read(&res_iodev, &adt7604_ctx, buf, sizeof(buf));
		if (rc < 0) {
			LOG_ERR("sensor_read (resistance) failed: %d", rc);
		} else {
			printf("Resistance channels:\n");
			print_res_readings(decoder, buf);
		}

		k_sleep(K_SECONDS(5));
	}

	return 0;
}
