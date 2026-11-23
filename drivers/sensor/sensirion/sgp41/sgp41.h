/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal header for the Sensirion SGP41 sensor driver.
 *
 * Defines I2C commands, timing constants, private channel, and
 * shared data structures used by sgp41.c and sgp41_decoder.c.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SGP41_SGP41_H_
#define ZEPHYR_DRIVERS_SENSOR_SGP41_SGP41_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/sensor/sgp41.h>

#define SGP41_CMD_CONDITIONING 0x2612
#define SGP41_CMD_MEASURE_RAW  0x2619
#define SGP41_CMD_SELF_TEST    0x280E
#define SGP41_CMD_HEATER_OFF   0x3615

#define SGP41_MEASURE_WAIT_MS  50
#define SGP41_SELFTEST_WAIT_MS 320
#define SGP41_CONDITIONING_MS  10000

#define SGP41_CRC_POLY 0x31 /* CRC-8 polynomial (x^8 + x^5 + x^4 + 1) */
#define SGP41_CRC_INIT 0xFF /* CRC-8 initial value */

#define SGP41_DEFAULT_RH_TICKS 0x8000 /* Default relative-humidity ticks */
#define SGP41_DEFAULT_T_TICKS  0x6666 /* Default temperature ticks */

#define SGP41_RAW_SHIFT 16 /* Shift for 0..65535 raw ticks into Q31 */

/* Internal alias for the public NOx channel */
#define SGP41_CHAN_NOX SGP41_SENSOR_CHAN_NOX_RAW

/**
 * @brief Static driver configuration populated from Device Tree.
 */
struct sgp41_config {
	struct i2c_dt_spec bus; /* I2C bus and address from Device Tree */
	bool selftest;          /* Run on-chip self-test during init if true */
};

/**
 * @brief Runtime driver state.
 */
struct sgp41_data {
	struct k_work_delayable work;   /* Delayed work to read results after 50 ms */
	struct rtio_iodev_sqe *pending; /* RTIO SQE waiting for measurement, or NULL */
	const struct device *dev;       /* Back-pointer needed inside work handler */
	int64_t conditioning_end;       /* Uptime ms deadline; -1=not started, 0=done */
};

/**
 * @brief Encoded sensor data written into the RTIO buffer.
 */
struct sgp41_encoded_data {
	uint64_t timestamp; /* Capture time in nanoseconds (k_ticks_to_ns_floor64) */
	uint16_t sraw_voc;  /* Raw VOC ticks (0..65535), valid when has_voc == 1 */
	uint16_t sraw_nox;  /* Raw NOx ticks (0..65535), valid when has_nox == 1 */
	uint8_t has_voc: 1; /* 1 = sraw_voc contains a valid reading */
	uint8_t has_nox: 1; /* 1 = sraw_nox contains a valid reading */
};

/**
 * @brief Get the decoder API for the SGP41 sensor.
 *
 * @param dev Pointer to the sensor device (unused).
 * @param api Output pointer to the sensor_decoder_api.
 * @return 0 always.
 */
int sgp41_get_decoder(const struct device *dev, const struct sensor_decoder_api **api);

#endif /* ZEPHYR_DRIVERS_SENSOR_SGP41_SGP41_H_ */
